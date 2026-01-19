#include "bdefs.h"


#if defined(LTJS_USE_FFMPEG_VIDEO_MGR)


#include "ltjs_ffmpeg_video_mgr_impl.h"
#include <cstring>
#include <memory>
#include <utility>
#include "bibendovsky_spul_file_stream.h"
#include "ltjs_fmv_player.h"
#include "render.h"
#include "soundmgr.h"


namespace ltjs
{


namespace ul = bibendovsky::spul;


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// FfmpegVideoInst::Impl
//

class FfmpegVideoInst::Impl
{
public:
	Impl(
		VideoMgr* video_mgr_ptr);

	~Impl();


	bool api_initialize(
		const char* const file_name);

	void api_release(
		VideoInst* video_inst_ptr);

	void api_on_render_init();

	void api_on_render_term();

	LTRESULT api_update();

	LTRESULT api_draw_video();

	LTRESULT api_get_video_status();


private:
	static constexpr auto audio_buffer_size_ms = 50;
	static constexpr auto default_format_string = "[FfmpegVideoInst] %s";


	bool is_initialized_;

	VideoMgr* video_mgr_ptr_;
	FmvPlayer fmv_player_;
	ul::FileStream file_stream_;
	bool is_audio_stream_started_;
	LtjsLtSoundSysGenericStream* audio_stream_;
	ILTSoundSys* sound_sys_ptr_;
	int audio_max_buffer_count_;

#if defined(LTJS_USE_DILIGENT_RENDER)
	HLTBUFFER surface_;
	std::uint32_t surface_width_;
	std::uint32_t surface_height_;
#endif


	bool initialize_internal(
		const char* const file_name);

	void uninitialize_internal();


	int io_read_func(
		std::uint8_t* buffer,
		const int buffer_size);

	static int io_read_func_proxy(
		void* user_data,
		std::uint8_t* buffer,
		const int buffer_size);

	std::int64_t io_seek_func(
		const std::int64_t offset,
		const FmvPlayer::SeekOrigin whence);

	static std::int64_t io_seek_func_proxy(
		void* user_data,
		const std::int64_t offset,
		const FmvPlayer::SeekOrigin whence);

	void audio_present_func(
		const void* data,
		const int data_size);

	static void audio_present_func_proxy(
		void* user_data,
		const void* data,
		const int data_size);

	int audio_get_free_buffer_count_func();

	static int audio_get_free_buffer_count_func_proxy(
		void* user_data);

	void video_present_func(
		const void* data,
		const int width,
		const int height);

	static void video_present_func_proxy(
		void* user_data,
		const void* data,
		const int width,
		const int height);

	bool clear_surface();

	LTRESULT init_screen();

	LTRESULT update_on_screen(
		const void* const data_ptr,
		const int width,
		const int height);
}; // FfmpegVideoInst::Impl


FfmpegVideoInst::Impl::Impl(
	VideoMgr* video_mgr_ptr)
	:
	is_initialized_{},
	video_mgr_ptr_{video_mgr_ptr},
	fmv_player_{},
	file_stream_{},
	is_audio_stream_started_{},
	audio_stream_{},
	sound_sys_ptr_{},
	audio_max_buffer_count_{},
#if defined(LTJS_USE_DILIGENT_RENDER)
	surface_{},
	surface_width_{},
	surface_height_{}
#endif
{
}

FfmpegVideoInst::Impl::~Impl()
{
}

bool FfmpegVideoInst::Impl::api_initialize(
	const char* const file_name)
{
	uninitialize_internal();

	if (!initialize_internal(file_name))
	{
		uninitialize_internal();
		return false;
	}

	return true;
}

void FfmpegVideoInst::Impl::api_release(
	VideoInst* video_inst_ptr)
{
	if (video_mgr_ptr_->m_Videos.FindElement(video_inst_ptr) != BAD_INDEX)
	{
		video_mgr_ptr_->m_Videos.RemoveAt(&video_inst_ptr->m_Link);
	}

	uninitialize_internal();
	api_on_render_term();

	delete this;
}

void FfmpegVideoInst::Impl::api_on_render_init()
{
	if( init_screen() != LT_OK )
		api_on_render_term();
}

void FfmpegVideoInst::Impl::api_on_render_term()
{
#if defined(LTJS_USE_DILIGENT_RENDER)
	if (surface_)
	{
		::r_GetRenderStruct()->DeleteSurface(surface_);
		surface_ = nullptr;
	}

	surface_width_ = 0;
	surface_height_ = 0;
#endif
}

LTRESULT FfmpegVideoInst::Impl::api_update()
{
	return LT_FINISHED;
}

LTRESULT FfmpegVideoInst::Impl::api_draw_video()
{
	if (!fmv_player_.is_initialized())
	{
		return LT_FINISHED;
	}

	if (!fmv_player_.present_frame())
	{
		return LT_FINISHED;
	}

	if (sound_sys_ptr_ && !is_audio_stream_started_)
	{
		is_audio_stream_started_ = true;
		audio_stream_->set_pause(false);
	}

	return LT_OK;
}

LTRESULT FfmpegVideoInst::Impl::api_get_video_status()
{
	if (!fmv_player_.is_initialized() || fmv_player_.is_presentation_finished())
	{
		return LT_FINISHED;
	}

	return LT_OK;
}

bool FfmpegVideoInst::Impl::initialize_internal(
	const char* const file_name)
{
	if (!file_name)
	{
		::dsi_ConsolePrint(default_format_string, "No file name.");
		return false;
	}

	const auto file_stream_result = file_stream_.open(file_name, ul::Stream::OpenMode::read);

	if (!file_stream_result)
	{
		const auto error_message = "Failed to open \"" + std::string{file_name} + "\"";
		::dsi_ConsolePrint(default_format_string, error_message.c_str());
		return false;
	}


	sound_sys_ptr_ = GetSoundSys();

	if (sound_sys_ptr_)
	{
		audio_max_buffer_count_ = sound_sys_ptr_->ltjs_get_generic_stream_queue_size();

		if (audio_max_buffer_count_ <= 0)
		{
			::dsi_ConsolePrint(default_format_string, "Failed to get generic stream queue size.");
			return false;
		}
	}

	const auto is_ignore_audio = (sound_sys_ptr_ == nullptr);

	auto initialize_param = FmvPlayer::InitializeParam{};
	initialize_param.is_ignore_audio_ = is_ignore_audio;

	if (!is_ignore_audio)
	{
		initialize_param.dst_sample_rate_ = 0;
		initialize_param.audio_buffer_size_ms_ = audio_buffer_size_ms;
		initialize_param.audio_max_buffer_count_ = audio_max_buffer_count_;
	}

	initialize_param.user_data_ = this;

	initialize_param.io_read_func_ = io_read_func_proxy;
	initialize_param.io_seek_func_ = io_seek_func_proxy;
	initialize_param.is_cancelled_func_ = nullptr;
	initialize_param.video_present_func_ = video_present_func_proxy;
	initialize_param.audio_present_func_ = audio_present_func_proxy;
	initialize_param.audio_get_free_buffer_count_func_ = audio_get_free_buffer_count_func_proxy;

	if (!fmv_player_.initialize(initialize_param))
	{
		::dsi_ConsolePrint(default_format_string, "Failed to initialize.");
		return false;
	}

	if (!is_ignore_audio)
	{
		const auto sample_rate = fmv_player_.get_sample_rate();
		const auto buffer_size = fmv_player_.get_audio_buffer_size();

		audio_stream_ = sound_sys_ptr_->ltjs_open_generic_stream(sample_rate, buffer_size);

		if (!audio_stream_)
		{
			::dsi_ConsolePrint(default_format_string, "Failed to open generic audio stream.");
			return false;
		}
	}

	api_on_render_init();

	is_initialized_ = true;

	return true;
}

void FfmpegVideoInst::Impl::uninitialize_internal()
{
	is_initialized_ = false;

	fmv_player_.uninitialize();
	file_stream_.close();

	if (sound_sys_ptr_)
	{
		is_audio_stream_started_ = false;

		if (audio_stream_)
		{
			sound_sys_ptr_->ltjs_close_generic_stream(audio_stream_);
			audio_stream_ = nullptr;
		}

		sound_sys_ptr_ = nullptr;
	}

	audio_max_buffer_count_ = 0;
}

int FfmpegVideoInst::Impl::io_read_func(
	std::uint8_t* buffer,
	const int buffer_size)
{
	return file_stream_.read(buffer, buffer_size);
}

int FfmpegVideoInst::Impl::io_read_func_proxy(
	void* user_data,
	std::uint8_t* buffer,
	const int buffer_size)
{
	auto instance = static_cast<FfmpegVideoInst::Impl*>(user_data);

	return instance->io_read_func(buffer, buffer_size);
}

std::int64_t FfmpegVideoInst::Impl::io_seek_func(
	const std::int64_t offset,
	const FmvPlayer::SeekOrigin whence)
{
	auto origin = ul::Stream::Origin{};

	switch (whence)
	{
	case ltjs::FmvPlayer::SeekOrigin::begin:
		origin = ul::Stream::Origin::begin;
		break;

	case ltjs::FmvPlayer::SeekOrigin::current:
		origin = ul::Stream::Origin::current;
		break;

	case ltjs::FmvPlayer::SeekOrigin::end:
		origin = ul::Stream::Origin::end;
		break;

	case ltjs::FmvPlayer::SeekOrigin::size:
		return file_stream_.get_size();

	case ltjs::FmvPlayer::SeekOrigin::none:
	default:
		return -1;
	}

	return file_stream_.set_position(offset, origin);
}

std::int64_t FfmpegVideoInst::Impl::io_seek_func_proxy(
	void* user_data,
	const std::int64_t offset,
	const FmvPlayer::SeekOrigin whence)
{
	auto instance = static_cast<FfmpegVideoInst::Impl*>(user_data);

	return instance->io_seek_func(offset, whence);
}

void FfmpegVideoInst::Impl::audio_present_func(
	const void* data,
	const int data_size)
{
	static_cast<void>(data_size);

	audio_stream_->enqueue_buffer(data);
}

void FfmpegVideoInst::Impl::audio_present_func_proxy(
	void* user_data,
	const void* data,
	const int data_size)
{
	auto instance = static_cast<FfmpegVideoInst::Impl*>(user_data);

	return instance->audio_present_func(data, data_size);
}

int FfmpegVideoInst::Impl::audio_get_free_buffer_count_func()
{
	return audio_stream_->get_free_buffer_count();
}

int FfmpegVideoInst::Impl::audio_get_free_buffer_count_func_proxy(
	void* user_data)
{
	auto instance = static_cast<FfmpegVideoInst::Impl*>(user_data);

	return instance->audio_get_free_buffer_count_func();
}

void FfmpegVideoInst::Impl::video_present_func(
	const void* data,
	const int width,
	const int height)
{
	static_cast<void>(update_on_screen(data, width, height));
}

void FfmpegVideoInst::Impl::video_present_func_proxy(
	void* user_data,
	const void* data,
	const int width,
	const int height)
{
	auto instance = static_cast<FfmpegVideoInst::Impl*>(user_data);

	return instance->video_present_func(data, width, height);
}

bool FfmpegVideoInst::Impl::clear_surface()
{
#if defined(LTJS_USE_DILIGENT_RENDER)
	if (!surface_)
	{
		return false;
	}

	uint32 pitch = 0;
	auto* data = static_cast<std::uint8_t*>(::r_GetRenderStruct()->LockSurface(surface_, pitch));
	if (!data)
	{
		return false;
	}

	for (auto i_line = 0U; i_line < surface_height_; ++i_line)
	{
		std::memset(data + i_line * pitch, 0, surface_width_ * 4);
	}

	::r_GetRenderStruct()->UnlockSurface(surface_);
	return true;
#else
	return false;
#endif
}

LTRESULT FfmpegVideoInst::Impl::init_screen()
{
	api_on_render_term();

	if (!::r_IsRenderInitted())
	{
		RETURN_ERROR(1, FfmpegVideoInst::Impl::init_screen, LT_NOTINITIALIZED);
	}

#if defined(LTJS_USE_DILIGENT_RENDER)
	const auto fmv_width = fmv_player_.get_width();
	const auto fmv_height = fmv_player_.get_height();

	surface_ = ::r_GetRenderStruct()->CreateSurface(fmv_width, fmv_height);
	if (!surface_)
	{
		RETURN_ERROR_PARAM(1, FfmpegVideoInst::Impl::init_screen, LT_ERROR, "Failed to create video surface.");
	}

	surface_width_ = static_cast<std::uint32_t>(fmv_width);
	surface_height_ = static_cast<std::uint32_t>(fmv_height);

	if (!clear_surface())
	{
		::r_GetRenderStruct()->DeleteSurface(surface_);
		surface_ = nullptr;
		surface_width_ = 0;
		surface_height_ = 0;
		RETURN_ERROR_PARAM(1, FfmpegVideoInst::Impl::init_screen, LT_ERROR, "Failed to clear video surface.");
	}

	return LT_OK;
#else
	RETURN_ERROR_PARAM(1, FfmpegVideoInst::Impl::init_screen, LT_NOTSUPPORTED,
		"FFmpeg video requires the Diligent renderer.");
#endif
}

LTRESULT FfmpegVideoInst::Impl::update_on_screen(
	const void* const data_ptr,
	const int width,
	const int height)
{
#if defined(LTJS_USE_DILIGENT_RENDER)
	if (!::r_IsRenderInitted() || !surface_)
	{
		RETURN_ERROR(1, FfmpegVideoInst::Impl::update_on_screen, LT_NOTINITIALIZED);
	}

	uint32 pitch = 0;
	auto* dst_data_ptr = static_cast<std::uint8_t*>(::r_GetRenderStruct()->LockSurface(surface_, pitch));
	if (!dst_data_ptr)
	{
		RETURN_ERROR_PARAM(1, FfmpegVideoInst::Impl::update_on_screen, LT_ERROR, "Failed to lock video surface.");
	}

	const auto src_pitch = 4 * width;
	for (auto i = 0; i < height; ++i)
	{
		const auto src_line = static_cast<const std::uint8_t*>(data_ptr) + (i * src_pitch);
		const auto dst_line = dst_data_ptr + (i * pitch);
		std::uninitialized_copy_n(src_line, src_pitch, dst_line);
	}

	::r_GetRenderStruct()->UnlockSurface(surface_);

	bool is_set_3d_state = !::r_GetRenderStruct()->IsIn3D();
	if (is_set_3d_state)
	{
		::r_GetRenderStruct()->Start3D();

		LTRGBColor color;
		color.dwordVal = 0x00000000;
		::r_GetRenderStruct()->Clear(nullptr, CLEARSCREEN_SCREEN, color);
	}

	const auto screen_width = ::r_GetRenderStruct()->m_Width;
	const auto screen_height = ::r_GetRenderStruct()->m_Height;

	auto image_width = screen_width;
	auto image_height = (image_width * height) / width;
	if (image_height > screen_height)
	{
		image_height = screen_height;
		image_width = (image_height * width) / height;
	}

	const auto x_offset = (screen_width - image_width) / 2;
	const auto y_offset = (screen_height - image_height) / 2;

	LTRect src_rect;
	src_rect.left = 0;
	src_rect.top = 0;
	src_rect.right = width;
	src_rect.bottom = height;

	LTRect dest_rect;
	dest_rect.left = x_offset;
	dest_rect.top = y_offset;
	dest_rect.right = x_offset + image_width;
	dest_rect.bottom = y_offset + image_height;

	BlitRequest request;
	request.m_hBuffer = surface_;
	request.m_pSrcRect = &src_rect;
	request.m_pDestRect = &dest_rect;
	request.m_Alpha = 1.0f;
	request.m_TransparentColor.dwVal = 0;
	request.m_BlitOptions = 0;
	::r_GetRenderStruct()->BlitToScreen(&request);

	if (is_set_3d_state)
	{
		::r_GetRenderStruct()->End3D();
	}

	return LT_OK;
#else
	RETURN_ERROR_PARAM(1, FfmpegVideoInst::Impl::update_on_screen, LT_NOTSUPPORTED,
		"FFmpeg video requires the Diligent renderer.");
#endif
}

//
// FfmpegVideoInst::Impl
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// FfmpegVideoMgr::Impl
//

class FfmpegVideoMgr::Impl
{
public:
	Impl();

	~Impl();


	LTRESULT api_initialize();

	LTRESULT api_create_screen_video(
		VideoMgr* video_mgr_ptr,
		const char* const pFilename,
		const uint32 flags,
		VideoInst*& pVideo);
}; // FfmpegVideoMgr::Impl


FfmpegVideoMgr::Impl::Impl()
{
}

FfmpegVideoMgr::Impl::~Impl()
{
}

LTRESULT FfmpegVideoMgr::Impl::api_initialize()
{
	ltjs::FmvPlayer::initialize_current_thread();

	return LT_OK;
}

LTRESULT FfmpegVideoMgr::Impl::api_create_screen_video(
	VideoMgr* video_mgr_ptr,
	const char* const file_name,
	const uint32 flags,
	VideoInst*& video_inst_ptr)
{
	static_cast<void>(flags);

	video_inst_ptr = nullptr;

	auto ffmpeg_video_inst_ptr = std::make_unique<FfmpegVideoInst>(video_mgr_ptr);

	if (!ffmpeg_video_inst_ptr)
	{
		return LT_ERROR;
	}

	if (!ffmpeg_video_inst_ptr->initialize(file_name))
	{
		return LT_ERROR;
	}

	video_inst_ptr = ffmpeg_video_inst_ptr.release();

	video_mgr_ptr->m_Videos.AddTail(video_inst_ptr, &video_inst_ptr->m_Link);

	return LT_OK;
}

//
// FfmpegVideoMgr::Impl
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// FfmpegVideoMgr
//

FfmpegVideoMgr::FfmpegVideoMgr()
	:
	impl_{std::make_unique<FfmpegVideoMgr::Impl>()}
{
}

FfmpegVideoMgr::FfmpegVideoMgr(
	FfmpegVideoMgr&& that)
	:
	impl_{std::move(that.impl_)}
{
}

FfmpegVideoMgr::~FfmpegVideoMgr()
{
}

LTRESULT FfmpegVideoMgr::initialize()
{
	return impl_->api_initialize();
}

LTRESULT FfmpegVideoMgr::CreateScreenVideo(
	const char* const file_name,
	const uint32 flags,
	VideoInst*& video_inst_ptr)
{
	return impl_->api_create_screen_video(this, file_name, flags, video_inst_ptr);
}

//
// FfmpegVideoMgr
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// FfmpegVideoInst
//

FfmpegVideoInst::FfmpegVideoInst(
	VideoMgr* video_mgr_ptr)
	:
	impl_{std::make_unique<Impl>(video_mgr_ptr)}
{
}

FfmpegVideoInst::FfmpegVideoInst(
	FfmpegVideoInst&& that)
	:
	impl_{std::move(that.impl_)}
{
}

FfmpegVideoInst::~FfmpegVideoInst()
{
}

bool FfmpegVideoInst::initialize(
	const char* const file_name)
{
	return impl_->api_initialize(file_name);
}

void FfmpegVideoInst::Release()
{
	impl_->api_release(this);
}

void FfmpegVideoInst::OnRenderInit()
{
	//impl_->api_on_render_init();
}

void FfmpegVideoInst::OnRenderTerm()
{
	impl_->api_on_render_term();
}

LTRESULT FfmpegVideoInst::Update()
{
	return impl_->api_update();
}

LTRESULT FfmpegVideoInst::DrawVideo()
{
	return impl_->api_draw_video();
}

LTRESULT FfmpegVideoInst::GetVideoStatus()
{
	return impl_->api_get_video_status();
}

//
// FfmpegVideoInst
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


} // ltjs


#endif // LTJS_USE_FFMPEG_VIDEO_MGR
