#include "ui_status_bar.h"

#include "imgui.h"

#include <cstdio>

void DrawStatusBar(const StatusBarInfo& info)
{
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float status_bar_height = ImGui::GetFrameHeight() + 4.0f;

  // Position the status bar at the bottom of the viewport
  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - status_bar_height));
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, status_bar_height));

  ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoNav |
    ImGuiWindowFlags_NoFocusOnAppearing;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 2.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));

  if (ImGui::Begin("##StatusBar", nullptr, flags))
  {
    // Grid spacing
    ImGui::TextDisabled("Grid:");
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::Text("%.0f", info.grid_spacing);

    ImGui::SameLine(0.0f, 16.0f);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0.0f, 16.0f);

    // Cursor position
    if (info.cursor_valid)
    {
      ImGui::Text("X: %.1f  Y: %.1f  Z: %.1f",
        info.cursor_pos[0], info.cursor_pos[1], info.cursor_pos[2]);
    }
    else
    {
      ImGui::TextDisabled("X: ---  Y: ---  Z: ---");
    }

    ImGui::SameLine(0.0f, 16.0f);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0.0f, 16.0f);

    // Selection count
    if (info.selection_count > 0)
    {
      ImGui::Text("%zu selected", info.selection_count);
    }
    else
    {
      ImGui::TextDisabled("No selection");
    }

    ImGui::SameLine(0.0f, 16.0f);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0.0f, 16.0f);

    // Current tool
    const char* tool_name = ToolName(info.current_tool);
    ImGui::Text("%s Tool", tool_name);

    // Selection filter status
    if (info.filter_active)
    {
      ImGui::SameLine(0.0f, 16.0f);
      ImGui::TextDisabled("|");
      ImGui::SameLine(0.0f, 16.0f);
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Filter: %s",
        info.filter_status != nullptr ? info.filter_status : "Active");
    }

    // Depth cycle status
    if (info.depth_cycle_status != nullptr && info.depth_cycle_status[0] != '\0')
    {
      ImGui::SameLine(0.0f, 16.0f);
      ImGui::TextDisabled("|");
      ImGui::SameLine(0.0f, 16.0f);
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Cycle: %s", info.depth_cycle_status);
    }

    // FPS (right-aligned)
    if (info.show_fps)
    {
      char fps_text[32];
      snprintf(fps_text, sizeof(fps_text), "%.1f FPS", info.fps);
      const float fps_width = ImGui::CalcTextSize(fps_text).x;
      const float avail_width = ImGui::GetContentRegionAvail().x;
      if (avail_width > fps_width + 16.0f)
      {
        ImGui::SameLine(ImGui::GetWindowWidth() - fps_width - 16.0f);
        ImGui::Text("%s", fps_text);
      }
    }

    // Hint (right-aligned if no FPS, or after FPS)
    if (info.hint != nullptr && info.hint[0] != '\0')
    {
      const float hint_width = ImGui::CalcTextSize(info.hint).x;
      const float avail_width = ImGui::GetContentRegionAvail().x;
      if (avail_width > hint_width + 32.0f)
      {
        ImGui::SameLine(0.0f, 16.0f);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0.0f, 16.0f);
        ImGui::TextDisabled("%s", info.hint);
      }
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}
