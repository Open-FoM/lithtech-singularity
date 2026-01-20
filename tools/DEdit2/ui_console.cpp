#include "ui_console.h"

#include "dedit2_concommand.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cfloat>

void DrawConsolePanel(ConsolePanelState& state)
{
	if (!ImGui::Begin("Console"))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button("Clear"))
	{
		DEdit2_ClearLog();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &state.auto_scroll);

	ImGui::Separator();

	const float footer_height = ImGui::GetFrameHeightWithSpacing();

	const auto& log = DEdit2_GetLog();
	if (state.log_line_count != log.size() ||
		(!log.empty() && state.log_last_line != log.back()))
	{
		state.log_line_count = log.size();
		state.log_last_line = log.empty() ? std::string() : log.back();
		state.log_buffer.clear();
		state.log_buffer.reserve(log.size() * 64);
		for (const auto& line : log)
		{
			state.log_buffer.append(line);
			state.log_buffer.push_back('\n');
		}
		if (state.auto_scroll)
		{
			state.scroll_to_bottom = true;
			state.scroll_frames = 3;
		}
	}

	ImGuiInputTextFlags log_flags = ImGuiInputTextFlags_ReadOnly;
	ImGui::InputTextMultiline(
		"##ConsoleLog",
		state.log_buffer.data(),
		static_cast<int>(state.log_buffer.size() + 1),
		ImVec2(-FLT_MIN, -footer_height),
		log_flags);
	if (state.scroll_to_bottom || state.scroll_frames > 0)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImGuiID child_id = ImGui::GetID("##ConsoleLog");
		char child_name[256];
		ImFormatString(child_name, IM_ARRAYSIZE(child_name), "%s/%s_%08X", window->Name, "##ConsoleLog", child_id);
		if (ImGuiWindow* child = ImGui::FindWindowByName(child_name))
		{
			child->ScrollTarget.y = FLT_MAX;
			child->ScrollTargetCenterRatio.y = 0.0f;
			child->ScrollTargetEdgeSnapDist.y = 0.0f;
			child->Scroll.y = child->ScrollMax.y;
		}
		if (state.scroll_frames > 0)
		{
			--state.scroll_frames;
		}
	}
	state.scroll_to_bottom = false;

	ImGui::Separator();

	ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
	if (ImGui::InputText("Input", state.input, sizeof(state.input), input_flags))
	{
		if (state.input[0] != '\0')
		{
			DEdit2_CommandHandler(state.input);
			state.input[0] = '\0';
			state.scroll_to_bottom = true;
			state.scroll_frames = 3;
		}
	}

	ImGui::End();
}
