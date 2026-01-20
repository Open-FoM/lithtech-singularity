#include "ui_worlds.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace
{
namespace fs = std::filesystem;

std::string ToLower(std::string value)
{
	for (char& ch : value)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return value;
}

bool IsWorldExtension(const fs::path& path)
{
	const std::string ext = ToLower(path.extension().string());
	return ext == ".ltc" || ext == ".lta" || ext == ".tbw" || ext == ".dat";
}

std::string FormatBytes(uintmax_t bytes)
{
	const char* suffixes[] = {"B", "KB", "MB", "GB"};
	double value = static_cast<double>(bytes);
	size_t suffix = 0;
	while (value >= 1024.0 && suffix + 1 < std::size(suffixes))
	{
		value /= 1024.0;
		++suffix;
	}

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(value < 10.0 && suffix > 0 ? 1 : 0) << value << " " << suffixes[suffix];
	return oss.str();
}

std::string FormatTime(const fs::file_time_type& time)
{
	using namespace std::chrono;
	const auto sys_time = time_point_cast<system_clock::duration>(
		time - fs::file_time_type::clock::now() + system_clock::now());
	std::time_t ctime = system_clock::to_time_t(sys_time);
	std::tm tm{};
#if defined(_WIN32)
	localtime_s(&tm, &ctime);
#else
	localtime_r(&ctime, &tm);
#endif
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
	return oss.str();
}

void RefreshWorlds(WorldBrowserState& state, const std::string& project_root)
{
	state.entries.clear();
	state.error.clear();
	state.refresh = false;
	state.last_root = project_root;
	state.selected_index = -1;

	if (project_root.empty())
	{
		state.error = "Project root is empty.";
		return;
	}

	const fs::path worlds_dir = fs::path(project_root) / "Worlds";
	std::error_code ec;
	if (!fs::exists(worlds_dir, ec) || !fs::is_directory(worlds_dir, ec))
	{
		state.error = "Worlds folder not found.";
		return;
	}

	for (const auto& entry : fs::directory_iterator(worlds_dir, fs::directory_options::skip_permission_denied, ec))
	{
		if (ec)
		{
			continue;
		}
		if (!entry.is_regular_file(ec))
		{
			continue;
		}
		const fs::path path = entry.path();
		if (!IsWorldExtension(path))
		{
			continue;
		}

		WorldBrowserEntry item{};
		item.name = path.filename().string();
		item.path = path.string();
		item.size_label = FormatBytes(entry.file_size(ec));
		if (ec)
		{
			item.size_label = "-";
		}
		const auto mod_time = entry.last_write_time(ec);
		if (ec)
		{
			item.modified_label = "-";
		}
		else
		{
			item.modified_label = FormatTime(mod_time);
		}
		state.entries.push_back(std::move(item));
	}

	std::sort(state.entries.begin(), state.entries.end(),
		[](const WorldBrowserEntry& a, const WorldBrowserEntry& b)
		{
			return a.name < b.name;
		});
}
} // namespace

void DrawWorldBrowserPanel(
	WorldBrowserState& state,
	const std::string& project_root,
	WorldBrowserAction& action)
{
	action.load_world = false;
	action.world_path.clear();

	if (project_root != state.last_root)
	{
		state.refresh = true;
	}

	if (ImGui::Begin("Worlds"))
	{
		ImGui::TextUnformatted("Worlds Folder");
		ImGui::SameLine();
		if (ImGui::SmallButton("Refresh"))
		{
			state.refresh = true;
		}

		if (state.refresh)
		{
			RefreshWorlds(state, project_root);
		}

		if (!state.error.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", state.error.c_str());
		}

		ImGui::Separator();
		if (ImGui::BeginTable(
			"WorldList",
			3,
			ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable |
				ImGuiTableFlags_ScrollY,
			ImVec2(0.0f, 0.0f)))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableHeadersRow();

			for (int i = 0; i < static_cast<int>(state.entries.size()); ++i)
			{
				const WorldBrowserEntry& entry = state.entries[static_cast<size_t>(i)];
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				const bool selected = (state.selected_index == i);
				if (ImGui::Selectable(entry.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
				{
					state.selected_index = i;
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					action.load_world = true;
					action.world_path = entry.path;
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(entry.size_label.c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(entry.modified_label.c_str());
			}

			ImGui::EndTable();
		}
	}
	ImGui::End();
}
