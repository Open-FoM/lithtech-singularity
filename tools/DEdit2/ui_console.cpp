#include "ui_console.h"

#include "dedit2_concommand.h"

#include "imgui.h"

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

	const ImGuiWindowFlags scroll_flags = ImGuiWindowFlags_HorizontalScrollbar;
	const float footer_height = ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("ConsoleScroll", ImVec2(0, -footer_height), false, scroll_flags);

	const auto& log = DEdit2_GetLog();
	for (const auto& line : log)
	{
		ImGui::TextUnformatted(line.c_str());
	}

	if (state.scroll_to_bottom ||
		(state.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
	{
		ImGui::SetScrollHereY(1.0f);
		state.scroll_to_bottom = false;
	}

	ImGui::EndChild();
	ImGui::Separator();

	ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
	if (ImGui::InputText("Input", state.input, sizeof(state.input), input_flags))
	{
		if (state.input[0] != '\0')
		{
			DEdit2_CommandHandler(state.input);
			state.input[0] = '\0';
			state.scroll_to_bottom = true;
		}
	}

	ImGui::End();
}
