#include "musicdriver.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ltpvalue.h"
#include "console.h"
#include "soundmgr.h"
#include "iltclient.h"
#include "ltjs_audio_decoder.h"
#include "ltjs_audio_utils.h"
#include "bibendovsky_spul_file_stream.h"

namespace
{
namespace ul = bibendovsky::spul;

struct MusicSong
{
	std::string name;
	std::string path;
};

struct MusicPlayList
{
	std::string name;
	std::vector<std::shared_ptr<MusicSong>> songs;
};

struct MusicState
{
	SMusicMgr* mgr = nullptr;
	ILTSoundSys* sound_sys = nullptr;
	LtjsLtSoundSysGenericStream* stream = nullptr;

	std::string data_dir;
	std::unordered_map<std::string, std::shared_ptr<MusicSong>> songs;
	std::unordered_map<std::string, std::shared_ptr<MusicPlayList>> playlists;

	std::thread worker;
	std::mutex mutex;
	std::condition_variable cv;

	bool quit = false;
	bool paused = false;
	bool loop = false;
	bool has_request = false;
	int volume = 100;

	std::shared_ptr<MusicPlayList> current_list;
	std::size_t current_index = 0;
	uint64_t request_id = 0;
	std::shared_ptr<MusicSong> transition_song;
	bool transition_pending = false;

	std::shared_ptr<MusicPlayList> queued_list;
	std::shared_ptr<MusicSong> queued_transition;
	bool queued_loop = false;
	bool queue_after_song = false;

	bool stop_after_song = false;
	bool stop_after_list = false;
	bool pause_after_song = false;
	bool pause_after_list = false;

	std::string motif_name;
	bool motif_loop = false;
	uint64_t motif_request_id = 0;

	std::string instrument_dls;
	std::string instrument_style;
};

MusicState g_music;

static void* music_CreatePlayList(const char* szPlayListName);
static void music_RemoveList(void* pPlayList);
static bool music_AddSongToList(void* pPlayList, void* pSong);
static bool music_PlayList(void* pPlayList, void* pTransition, bool bLoop, uint32 dwPlayBoundaryFlags);

static std::shared_ptr<MusicSong> find_song_locked(MusicSong* song)
{
	if (!song)
	{
		return {};
	}

	auto it = g_music.songs.find(song->name);
	if (it == g_music.songs.end() || it->second.get() != song)
	{
		return {};
	}

	return it->second;
}

static std::shared_ptr<MusicPlayList> find_playlist_locked(MusicPlayList* playlist)
{
	if (!playlist)
	{
		return {};
	}

	auto it = g_music.playlists.find(playlist->name);
	if (it == g_music.playlists.end() || it->second.get() != playlist)
	{
		return {};
	}

	return it->second;
}

void music_ConsolePrint(const char* pMsg, ...)
{
	va_list marker;
	char str[500];

	va_start(marker, pMsg);
	LTVSNPrintF(str, sizeof(str), pMsg, marker);
	va_end(marker);

	con_PrintString(CONRGB(100, 255, 100), 0, str);
}

static bool is_absolute_path(const std::string& path)
{
	if (path.empty())
	{
		return false;
	}

	if (path[0] == '/' || path[0] == '\\')
	{
		return true;
	}

	if (path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':')
	{
		return true;
	}

	return false;
}

static std::string build_song_path(const char* name)
{
	if (!name)
	{
		return {};
	}

	std::string path{name};
	if (path.empty())
	{
		return path;
	}

	if (is_absolute_path(path) || g_music.data_dir.empty())
	{
		return path;
	}

	if (g_music.data_dir.back() == '/' || g_music.data_dir.back() == '\\')
	{
		return g_music.data_dir + path;
	}

	return g_music.data_dir + "/" + path;
}

static int volume_to_level_mb(int volume)
{
	const auto clamped = std::max(0, std::min(100, volume));
	const auto range = ltjs::AudioUtils::max_generic_stream_level_mb - ltjs::AudioUtils::min_generic_stream_level_mb;
	return ltjs::AudioUtils::min_generic_stream_level_mb + (range * clamped) / 100;
}

static void close_stream()
{
	LtjsLtSoundSysGenericStream* old_stream = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_music.mutex);
		old_stream = g_music.stream;
		g_music.stream = nullptr;
	}

	if (g_music.sound_sys && old_stream)
	{
		g_music.sound_sys->ltjs_close_generic_stream(old_stream);
	}
}

static void stop_playback_locked()
{
	g_music.current_list = nullptr;
	g_music.current_index = 0;
	g_music.paused = false;
	g_music.has_request = false;
	g_music.request_id++;
	g_music.transition_song = nullptr;
	g_music.transition_pending = false;
	g_music.queued_list = nullptr;
	g_music.queued_transition = nullptr;
	g_music.queued_loop = false;
	g_music.queue_after_song = false;
	g_music.stop_after_song = false;
	g_music.stop_after_list = false;
	g_music.pause_after_song = false;
	g_music.pause_after_list = false;
}

static bool open_stream_for_decoder(const ltjs::AudioDecoder& decoder, int& buffer_size)
{
	const auto sample_rate = decoder.get_dst_sample_rate();
	const auto sample_size = decoder.get_dst_sample_size();
	static constexpr auto mix_size_ms = 50;

	if (sample_rate <= 0 || sample_size <= 0)
	{
		return false;
	}

	auto mix_size = (sample_size * sample_rate * mix_size_ms) / 1000;
	mix_size += sample_size - 1;
	mix_size /= sample_size;
	mix_size *= sample_size;

	buffer_size = mix_size;

	auto* new_stream = g_music.sound_sys->ltjs_open_generic_stream(sample_rate, buffer_size);
	if (!new_stream)
	{
		return false;
	}

	new_stream->set_volume(volume_to_level_mb(g_music.volume));
	new_stream->set_pause(false);

	LtjsLtSoundSysGenericStream* old_stream = nullptr;
	{
		std::lock_guard<std::mutex> lock(g_music.mutex);
		old_stream = g_music.stream;
		g_music.stream = new_stream;
	}

	if (g_music.sound_sys && old_stream)
	{
		g_music.sound_sys->ltjs_close_generic_stream(old_stream);
	}
	return true;
}

static void music_worker()
{
	ltjs::AudioDecoder decoder;
	ltjs::AudioDecoder motif_decoder;
	ul::FileStream file_stream;
	ul::FileStream motif_stream;
	std::vector<std::uint8_t> buffer;
	std::vector<std::uint8_t> motif_buffer;
	std::string motif_name;
	bool motif_loop = false;
	bool motif_active = false;
	uint64_t motif_request_id = 0;
	int motif_buffer_size = 0;
	bool transition_pending = false;
	std::shared_ptr<MusicSong> transition_song;

	while (true)
	{
		std::shared_ptr<MusicPlayList> playlist;
		std::size_t song_index = 0;
		bool loop = false;
		uint64_t request_id = 0;

		{
			std::unique_lock<std::mutex> lock(g_music.mutex);
			g_music.cv.wait(lock, [] { return g_music.quit || g_music.current_list || !g_music.motif_name.empty(); });

			if (g_music.quit)
			{
				break;
			}

			playlist = g_music.current_list;
			song_index = g_music.current_index;
			loop = g_music.loop;
			request_id = g_music.request_id;
			motif_request_id = g_music.motif_request_id;
			motif_name = g_music.motif_name;
			motif_loop = g_music.motif_loop;
			transition_pending = g_music.transition_pending;
			transition_song = g_music.transition_song;
		}

		while (true)
		{
			std::shared_ptr<MusicSong> song;
			bool is_transition_song = false;

			{
				std::unique_lock<std::mutex> lock(g_music.mutex);
				if (g_music.quit)
				{
					break;
				}

				if (g_music.request_id != request_id || g_music.current_list != playlist)
				{
					break;
				}

				if (g_music.paused)
				{
					if (g_music.stream)
					{
						g_music.stream->set_pause(true);
					}
					lock.unlock();
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}
				if (!playlist || playlist->songs.empty())
				{
					if (motif_name.empty())
					{
						stop_playback_locked();
						playlist.reset();
					}
				}
				else if (song_index >= playlist->songs.size())
				{
					if (loop)
					{
						song_index = 0;
					}
					else
					{
						if (g_music.pause_after_list)
						{
							g_music.pause_after_list = false;
							g_music.paused = true;
							if (g_music.stream)
							{
								g_music.stream->set_pause(true);
							}
						}

						if (g_music.stop_after_list)
						{
							g_music.stop_after_list = false;
							stop_playback_locked();
							g_music.motif_name.clear();
							g_music.motif_loop = false;
							g_music.motif_request_id++;
							playlist.reset();
						}
						else if (g_music.queued_list && !g_music.queue_after_song)
						{
							g_music.current_list = g_music.queued_list;
							g_music.current_index = 0;
							g_music.loop = g_music.queued_loop;
							g_music.transition_song = g_music.queued_transition;
							g_music.transition_pending = (g_music.queued_transition != nullptr);
							g_music.request_id++;
							playlist = g_music.current_list;
							song_index = 0;
							loop = g_music.loop;
							request_id = g_music.request_id;
							transition_pending = g_music.transition_pending;
							transition_song = g_music.transition_song;
							g_music.queued_list.reset();
							g_music.queued_transition.reset();
							g_music.queued_loop = false;
							g_music.queue_after_song = false;
						}
						else
						{
							stop_playback_locked();
							playlist.reset();
						}
					}
				}

				if (playlist)
				{
					if (transition_pending && transition_song)
					{
						song = transition_song;
						is_transition_song = true;
						transition_pending = false;
						if (g_music.request_id == request_id)
						{
							g_music.transition_pending = false;
						}
					}
					else if (song_index < playlist->songs.size())
					{
						song = playlist->songs[song_index];
					}
				}
			}

			if (!song && motif_name.empty())
			{
				break;
			}

			if (song)
			{
				file_stream.close();
				if (!file_stream.open(song->path.c_str(), ul::Stream::OpenMode::read))
				{
					++song_index;
					continue;
				}

				auto decoder_param = ltjs::AudioDecoder::OpenParam{};
				decoder_param.dst_channel_count_ = 2;
				decoder_param.dst_bit_depth_ = 16;
				decoder_param.dst_sample_rate_ = 0;
				decoder_param.stream_ptr_ = &file_stream;

				decoder.close();
				if (!decoder.open(decoder_param))
				{
					file_stream.close();
					++song_index;
					continue;
				}
			}

			int buffer_size = 0;
			if (song)
			{
				if (!open_stream_for_decoder(decoder, buffer_size))
				{
					decoder.close();
					file_stream.close();
					++song_index;
					continue;
				}
			}
			else
			{
				buffer_size = motif_buffer_size;
				bool need_sleep = false;
				{
					std::lock_guard<std::mutex> lock(g_music.mutex);
					if (!g_music.stream)
					{
						need_sleep = true;
					}
				}

				if (need_sleep)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}
			}

			buffer.assign(static_cast<std::size_t>(buffer_size), 0);

			bool song_finished = false;
			while (!song_finished)
			{
				{
					std::unique_lock<std::mutex> lock(g_music.mutex);
					if (g_music.quit)
					{
						song_finished = true;
						break;
					}

					if (g_music.request_id != request_id || g_music.current_list != playlist)
					{
						song_finished = true;
						break;
					}

					if (g_music.motif_request_id != motif_request_id)
					{
						motif_request_id = g_music.motif_request_id;
						motif_name = g_music.motif_name;
						motif_loop = g_music.motif_loop;
						motif_active = false;
						motif_decoder.close();
						motif_stream.close();
					}

					if (g_music.paused)
					{
						if (g_music.stream)
						{
							g_music.stream->set_pause(true);
						}
						lock.unlock();
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						continue;
					}
				}

				if (!motif_name.empty())
				{
					if (!motif_active)
					{
						motif_stream.close();
						const auto motif_path = build_song_path(motif_name.c_str());
						if (motif_stream.open(motif_path.c_str(), ul::Stream::OpenMode::read))
						{
							auto motif_param = ltjs::AudioDecoder::OpenParam{};
							motif_param.dst_channel_count_ = 2;
							motif_param.dst_bit_depth_ = 16;
							motif_param.dst_sample_rate_ = song ? decoder.get_dst_sample_rate() : 0;
							motif_param.stream_ptr_ = &motif_stream;

							motif_decoder.close();
							if (motif_decoder.open(motif_param))
							{
								motif_active = true;
							}
							else
							{
								motif_stream.close();
								{
									std::lock_guard<std::mutex> lock(g_music.mutex);
									if (g_music.motif_name == motif_name)
									{
										g_music.motif_name.clear();
										g_music.motif_loop = false;
										g_music.motif_request_id++;
									}
								}
								motif_name.clear();
							}
						}
						else
						{
							{
								std::lock_guard<std::mutex> lock(g_music.mutex);
								if (g_music.motif_name == motif_name)
								{
									g_music.motif_name.clear();
									g_music.motif_loop = false;
									g_music.motif_request_id++;
								}
							}
							motif_name.clear();
						}

						if (motif_active && !song)
						{
							if (!open_stream_for_decoder(motif_decoder, motif_buffer_size))
							{
								motif_active = false;
								motif_decoder.close();
								motif_stream.close();
								{
									std::lock_guard<std::mutex> lock(g_music.mutex);
									if (g_music.motif_name == motif_name)
									{
										g_music.motif_name.clear();
										g_music.motif_loop = false;
										g_music.motif_request_id++;
									}
								}
								motif_name.clear();
							}
							else
							{
								buffer_size = motif_buffer_size;
								buffer.assign(static_cast<std::size_t>(buffer_size), 0);
							}
						}
					}

					if (motif_active)
					{
						if (static_cast<int>(motif_buffer.size()) != buffer_size)
						{
							motif_buffer.assign(static_cast<std::size_t>(buffer_size), 0);
						}
					}
				}

				if (!song && !motif_active)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}

				bool wait_for_buffer = false;
				{
					std::lock_guard<std::mutex> lock(g_music.mutex);
					if (!g_music.stream)
					{
						song_finished = true;
						break;
					}

					if (g_music.stream->get_free_buffer_count() <= 0)
					{
						wait_for_buffer = true;
					}
				}

				if (wait_for_buffer)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(2));
					continue;
				}

				int decoded = 0;
				if (song)
				{
					decoded = decoder.decode(buffer.data(), buffer_size);
					if (decoded <= 0)
					{
						std::fill(buffer.begin(), buffer.end(), 0);
						{
							std::lock_guard<std::mutex> lock(g_music.mutex);
							if (g_music.stream)
							{
								g_music.stream->enqueue_buffer(buffer.data());
							}
						}
						song_finished = true;
						break;
					}

					if (decoded < buffer_size)
					{
						std::fill(buffer.begin() + decoded, buffer.end(), 0);
						song_finished = true;
					}
				}
				else
				{
					std::fill(buffer.begin(), buffer.end(), 0);
				}

				if (motif_active)
				{
					auto motif_decoded = motif_decoder.decode(motif_buffer.data(), buffer_size);

					if (motif_decoded <= 0)
					{
						if (motif_loop && motif_decoder.rewind())
						{
							motif_decoded = motif_decoder.decode(motif_buffer.data(), buffer_size);
						}
						else
						{
							motif_active = false;
							motif_decoder.close();
							motif_stream.close();
							{
								std::lock_guard<std::mutex> lock(g_music.mutex);
								if (g_music.motif_name == motif_name)
								{
									g_music.motif_name.clear();
									g_music.motif_loop = false;
									g_music.motif_request_id++;
								}
							}
							motif_name.clear();
						}
					}

					if (motif_decoded > 0)
					{
						if (motif_decoded < buffer_size)
						{
							std::fill(motif_buffer.begin() + motif_decoded, motif_buffer.end(), 0);
							if (!motif_loop)
							{
								motif_active = false;
								{
									std::lock_guard<std::mutex> lock(g_music.mutex);
									if (g_music.motif_name == motif_name)
									{
										g_music.motif_name.clear();
										g_music.motif_loop = false;
										g_music.motif_request_id++;
									}
								}
							}
						}

						auto* out_samples = reinterpret_cast<int16_t*>(buffer.data());
						const auto* motif_samples = reinterpret_cast<const int16_t*>(motif_buffer.data());
						const auto sample_count = buffer_size / static_cast<int>(sizeof(int16_t));
						for (int i = 0; i < sample_count; ++i)
						{
							const int mixed = static_cast<int>(out_samples[i]) + static_cast<int>(motif_samples[i]);
							out_samples[i] = static_cast<int16_t>(std::max(-32768, std::min(32767, mixed)));
						}
					}
				}

				{
					std::lock_guard<std::mutex> lock(g_music.mutex);
					if (g_music.stream)
					{
						g_music.stream->enqueue_buffer(buffer.data());
					}
				}
			}

			if (song)
			{
				decoder.close();
				file_stream.close();
				if (is_transition_song)
				{
					continue;
				}

				{
					std::unique_lock<std::mutex> lock(g_music.mutex);
					if (g_music.pause_after_song)
					{
						g_music.pause_after_song = false;
						g_music.paused = true;
						if (g_music.stream)
						{
							g_music.stream->set_pause(true);
						}
					}

					if (g_music.stop_after_song)
					{
						g_music.stop_after_song = false;
						stop_playback_locked();
						g_music.motif_name.clear();
						g_music.motif_loop = false;
						g_music.motif_request_id++;
						playlist = nullptr;
					}
					else if (g_music.queued_list && g_music.queue_after_song)
					{
						g_music.current_list = g_music.queued_list;
						g_music.current_index = 0;
						g_music.loop = g_music.queued_loop;
						g_music.transition_song = g_music.queued_transition;
						g_music.transition_pending = (g_music.queued_transition != nullptr);
						g_music.request_id++;
						playlist = g_music.current_list;
						song_index = 0;
						loop = g_music.loop;
						request_id = g_music.request_id;
						transition_pending = g_music.transition_pending;
						transition_song = g_music.transition_song;
						g_music.queued_list = nullptr;
						g_music.queued_transition = nullptr;
						g_music.queued_loop = false;
						g_music.queue_after_song = false;
					}
				}

				if (playlist)
				{
					++song_index;
				}
			}
			else
			{
				break;
			}
		}
	}

	motif_decoder.close();
	motif_stream.close();
	close_stream();
}

static bool music_Init(SMusicMgr* pMusicMgr)
{
	if (!pMusicMgr)
	{
		return false;
	}

	g_music.sound_sys = GetSoundSys();
	if (!g_music.sound_sys)
	{
		return false;
	}

	g_music.mgr = pMusicMgr;
	g_music.quit = false;
	g_music.paused = false;
	g_music.loop = false;
	g_music.volume = 100;
	g_music.current_list = nullptr;
	g_music.current_index = 0;
	g_music.request_id = 0;
	g_music.transition_song = nullptr;
	g_music.transition_pending = false;
	g_music.queued_list = nullptr;
	g_music.queued_transition = nullptr;
	g_music.queued_loop = false;
	g_music.queue_after_song = false;
	g_music.stop_after_song = false;
	g_music.stop_after_list = false;
	g_music.pause_after_song = false;
	g_music.pause_after_list = false;

	if (!g_music.worker.joinable())
	{
		g_music.worker = std::thread(music_worker);
	}

	pMusicMgr->m_bValid = true;
	pMusicMgr->m_bEnabled = true;
	pMusicMgr->m_ulCaps = MUSIC_VOLUMECONTROL | MUSIC_MOTIFSAVAIL;
	return true;
}

static void music_Term()
{
	{
		std::unique_lock<std::mutex> lock(g_music.mutex);
		g_music.quit = true;
		stop_playback_locked();
	}
	g_music.cv.notify_all();

	if (g_music.worker.joinable())
	{
		g_music.worker.join();
	}

	close_stream();
	g_music.sound_sys = nullptr;
	g_music.mgr = nullptr;
	g_music.songs.clear();
	g_music.playlists.clear();
}

static bool music_SetDataDirectory(const char* szDataDir)
{
	std::lock_guard<std::mutex> lock(g_music.mutex);
	g_music.data_dir = szDataDir ? szDataDir : "";
	return true;
}

static bool music_InitInstruments(const char* szDLSFile, const char* szStyleFile)
{
	if (!szDLSFile || !szStyleFile || !szDLSFile[0] || !szStyleFile[0])
	{
		return false;
	}

	const auto dls_path = build_song_path(szDLSFile);
	const auto style_path = build_song_path(szStyleFile);

	auto dls_stream = ul::FileStream{};
	auto style_stream = ul::FileStream{};

	if (!dls_stream.open(dls_path.c_str(), ul::Stream::OpenMode::read))
	{
		return false;
	}

	if (!style_stream.open(style_path.c_str(), ul::Stream::OpenMode::read))
	{
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(g_music.mutex);
		g_music.instrument_dls = dls_path;
		g_music.instrument_style = style_path;
		if (g_music.mgr)
		{
			g_music.mgr->m_ulCaps |= MUSIC_INSTRUMENTSET;
		}
	}

	return true;
}

static void* music_CreateSong(const char* pFileName)
{
	if (!pFileName)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto it = g_music.songs.find(pFileName);
	if (it != g_music.songs.end())
	{
		return it->second.get();
	}

	auto song = std::make_shared<MusicSong>();
	song->name = pFileName;
	song->path = build_song_path(pFileName);
	auto* song_ptr = song.get();
	g_music.songs.emplace(song->name, song);
	return song_ptr;
}

static void* music_GetSong(const char* pFileName)
{
	if (!pFileName)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto it = g_music.songs.find(pFileName);
	return (it != g_music.songs.end()) ? it->second.get() : nullptr;
}

static void music_DestroySong(void* pSong)
{
	if (!pSong)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto* song_ptr = static_cast<MusicSong*>(pSong);
	auto song = find_song_locked(song_ptr);
	if (!song)
	{
		return;
	}

	if (g_music.current_list)
	{
		auto& songs = g_music.current_list->songs;
		if (g_music.current_index < songs.size() && songs[g_music.current_index].get() == song_ptr)
		{
			stop_playback_locked();
		}
	}
	for (auto& playlist : g_music.playlists)
	{
		auto& songs = playlist.second->songs;
		songs.erase(std::remove_if(songs.begin(), songs.end(),
			[song_ptr](const std::shared_ptr<MusicSong>& entry)
			{
				return entry.get() == song_ptr;
			}),
			songs.end());
	}

	g_music.songs.erase(song->name);
}

static bool music_PlayBreak(void* pSong, uint32 dwPlayBoundaryFlags)
{
	if (!pSong)
	{
		return false;
	}

	auto list = music_CreatePlayList("_single");
	music_RemoveList(list);
	list = music_CreatePlayList("_single");
	music_AddSongToList(list, pSong);
	return music_PlayList(list, nullptr, false, dwPlayBoundaryFlags);
}

static void* music_CreatePlayList(const char* szPlayListName)
{
	if (!szPlayListName)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto it = g_music.playlists.find(szPlayListName);
	if (it != g_music.playlists.end())
	{
		return it->second.get();
	}

	auto playlist = std::make_shared<MusicPlayList>();
	playlist->name = szPlayListName;
	auto* playlist_ptr = playlist.get();
	g_music.playlists.emplace(playlist->name, playlist);
	return playlist_ptr;
}

static void* music_GetPlayList(const char* szPlayListName)
{
	if (!szPlayListName)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto it = g_music.playlists.find(szPlayListName);
	return (it != g_music.playlists.end()) ? it->second.get() : nullptr;
}

static bool music_AddSongToList(void* pPlayList, void* pSong)
{
	if (!pPlayList || !pSong)
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto* playlist_ptr = static_cast<MusicPlayList*>(pPlayList);
	auto* song_ptr = static_cast<MusicSong*>(pSong);
	auto playlist = find_playlist_locked(playlist_ptr);
	auto song = find_song_locked(song_ptr);
	if (!playlist || !song)
	{
		return false;
	}

	playlist->songs.push_back(song);
	return true;
}

static void* music_GetSongFromList(void* pPlayList, unsigned long dwIndex)
{
	if (!pPlayList)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto* playlist_ptr = static_cast<MusicPlayList*>(pPlayList);
	auto playlist = find_playlist_locked(playlist_ptr);
	if (!playlist || dwIndex >= playlist->songs.size())
	{
		return nullptr;
	}

	return playlist->songs[dwIndex].get();
}

static void music_RemoveList(void* pPlayList)
{
	if (!pPlayList)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(g_music.mutex);
	auto* playlist_ptr = static_cast<MusicPlayList*>(pPlayList);
	auto playlist = find_playlist_locked(playlist_ptr);
	if (!playlist)
	{
		return;
	}

	if (g_music.current_list == playlist)
	{
		stop_playback_locked();
	}
	g_music.playlists.erase(playlist->name);
}

static bool music_PlayList(void* pPlayList, void* pTransition, bool bLoop, uint32 dwPlayBoundaryFlags)
{
	static_cast<void>(pTransition);

	if (!pPlayList)
	{
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(g_music.mutex);
		auto* list_ptr = static_cast<MusicPlayList*>(pPlayList);
		auto list = find_playlist_locked(list_ptr);
		auto transition_song = find_song_locked(static_cast<MusicSong*>(pTransition));
		if (!list || list->songs.empty())
		{
			stop_playback_locked();
			return false;
		}

		const auto has_current = (g_music.current_list != nullptr);
		const auto queue_after_song = (dwPlayBoundaryFlags == MUSIC_NEXTMEASURE || dwPlayBoundaryFlags == MUSIC_NEXTSONG);
		const auto queue_after_list = (dwPlayBoundaryFlags == MUSIC_QUEUE);

		if (has_current && (queue_after_song || queue_after_list))
		{
			g_music.queued_list = list;
			g_music.queued_transition = transition_song;
			g_music.queued_loop = bLoop;
			g_music.queue_after_song = queue_after_song;
		}
		else
		{
			g_music.current_list = list;
			g_music.current_index = 0;
			g_music.loop = bLoop;
			g_music.paused = false;
			g_music.transition_song = transition_song;
			g_music.transition_pending = (transition_song != nullptr);
			g_music.queued_list.reset();
			g_music.queued_transition.reset();
			g_music.queued_loop = false;
			g_music.queue_after_song = false;
			g_music.stop_after_song = false;
			g_music.stop_after_list = false;
			g_music.pause_after_song = false;
			g_music.pause_after_list = false;
			g_music.request_id++;
		}
	}
	g_music.cv.notify_all();
	return true;
}

static bool music_PlayMotif(const char* szMotifName, bool bLoop)
{
	if (!szMotifName || !szMotifName[0])
	{
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(g_music.mutex);
		g_music.motif_name = szMotifName;
		g_music.motif_loop = bLoop;
		g_music.motif_request_id++;
	}
	g_music.cv.notify_all();
	return true;
}

static bool music_StopMotif(const char* szMotifName)
{
	std::lock_guard<std::mutex> lock(g_music.mutex);
	if (!szMotifName || g_music.motif_name == szMotifName)
	{
		g_music.motif_name.clear();
		g_music.motif_loop = false;
		g_music.motif_request_id++;
		return true;
	}

	return false;
}

static void music_DestroyAllSongs()
{
	std::lock_guard<std::mutex> lock(g_music.mutex);
	g_music.playlists.clear();
	g_music.songs.clear();
	g_music.motif_name.clear();
	g_music.motif_loop = false;
	g_music.motif_request_id++;
	stop_playback_locked();
}

static void music_Stop(uint32 dwPlayBoundaryFlags)
{
	std::lock_guard<std::mutex> lock(g_music.mutex);
	if (dwPlayBoundaryFlags == MUSIC_NEXTMEASURE || dwPlayBoundaryFlags == MUSIC_NEXTSONG)
	{
		g_music.stop_after_song = true;
		return;
	}

	if (dwPlayBoundaryFlags == MUSIC_QUEUE)
	{
		g_music.stop_after_list = true;
		return;
	}

	stop_playback_locked();
	g_music.motif_name.clear();
	g_music.motif_loop = false;
	g_music.motif_request_id++;
}

static bool music_Pause(uint32 dwPlayBoundaryFlags)
{
	std::lock_guard<std::mutex> lock(g_music.mutex);
	if (dwPlayBoundaryFlags == MUSIC_NEXTMEASURE || dwPlayBoundaryFlags == MUSIC_NEXTSONG)
	{
		g_music.pause_after_song = true;
		return true;
	}

	if (dwPlayBoundaryFlags == MUSIC_QUEUE)
	{
		g_music.pause_after_list = true;
		return true;
	}

	g_music.paused = true;
	if (g_music.stream)
	{
		g_music.stream->set_pause(true);
	}
	return true;
}

static bool music_Resume()
{
	std::lock_guard<std::mutex> lock(g_music.mutex);
	g_music.paused = false;
	if (g_music.stream)
	{
		g_music.stream->set_pause(false);
	}
	g_music.cv.notify_all();
	return true;
}

static uint16 music_GetVolume()
{
	std::lock_guard<std::mutex> lock(g_music.mutex);
	return static_cast<uint16>(g_music.volume);
}

static void music_SetVolume(unsigned short vol)
{
	{
		std::lock_guard<std::mutex> lock(g_music.mutex);
		g_music.volume = std::min<unsigned short>(vol, 100);
		if (g_music.stream)
		{
			g_music.stream->set_volume(volume_to_level_mb(g_music.volume));
		}
	}
}

} // namespace

musicdriver_status music_InitDriver(char* pMusicDLLName, SMusicMgr* pMusicMgr)
{
	static_cast<void>(pMusicDLLName);
	if (!pMusicMgr)
	{
		return MUSICDRIVER_INVALIDOPTIONS;
	}

	pMusicMgr->Init = music_Init;
	pMusicMgr->Term = music_Term;
	pMusicMgr->SetDataDirectory = music_SetDataDirectory;
	pMusicMgr->InitInstruments = music_InitInstruments;
	pMusicMgr->CreateSong = music_CreateSong;
	pMusicMgr->GetSong = music_GetSong;
	pMusicMgr->DestroySong = music_DestroySong;
	pMusicMgr->PlayBreak = music_PlayBreak;
	pMusicMgr->CreatePlayList = music_CreatePlayList;
	pMusicMgr->GetPlayList = music_GetPlayList;
	pMusicMgr->AddSongToList = music_AddSongToList;
	pMusicMgr->GetSongFromList = music_GetSongFromList;
	pMusicMgr->RemoveList = music_RemoveList;
	pMusicMgr->PlayList = music_PlayList;
	pMusicMgr->PlayMotif = music_PlayMotif;
	pMusicMgr->StopMotif = music_StopMotif;
	pMusicMgr->DestroyAllSongs = music_DestroyAllSongs;
	pMusicMgr->Stop = music_Stop;
	pMusicMgr->Pause = music_Pause;
	pMusicMgr->Resume = music_Resume;
	pMusicMgr->GetVolume = music_GetVolume;
	pMusicMgr->SetVolume = music_SetVolume;
	pMusicMgr->ConsolePrint = music_ConsolePrint;

	pMusicMgr->m_bValid = false;
	pMusicMgr->m_bEnabled = false;

	if (!pMusicMgr->Init(pMusicMgr))
	{
		return MUSICDRIVER_INVALIDOPTIONS;
	}

	return MUSICDRIVER_OK;
}

void music_TermDriver()
{
	music_Term();
}
