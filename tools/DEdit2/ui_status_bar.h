#pragma once

#include "ui_tools_dock.h"

#include <array>
#include <cstddef>

/// Information displayed in the status bar.
struct StatusBarInfo {
  float grid_spacing = 64.0f;
  bool cursor_valid = false;
  std::array<float, 3> cursor_pos = {0.0f, 0.0f, 0.0f};
  size_t selection_count = 0;
  EditorTool current_tool = EditorTool::Select;
  const char* hint = nullptr;
  float fps = 0.0f;
  bool show_fps = false;
};

/// Draw the status bar at the bottom of the main window.
/// @param info Status bar information to display.
void DrawStatusBar(const StatusBarInfo& info);
