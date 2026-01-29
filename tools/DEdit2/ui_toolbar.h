#pragma once

#include "ui_dock.h"
#include "ui_tools_dock.h"

/// Result from drawing the toolbar.
struct ToolbarResult {
  bool tool_changed = false;
  EditorTool new_tool = EditorTool::None;
  PrimitiveType create_primitive = PrimitiveType::None;
  bool undo_requested = false;
  bool redo_requested = false;
  bool snap_toggled = false;  ///< True if snap button was clicked
};

/// Draw the main toolbar below the menu bar.
/// @param state Tools panel state (shared with dockable Tools panel).
/// @param can_undo Whether undo is available.
/// @param can_redo Whether redo is available.
/// @param snap_enabled Whether snapping is currently enabled.
/// @return Result containing any actions triggered.
ToolbarResult DrawToolbar(ToolsPanelState& state, bool can_undo, bool can_redo, bool snap_enabled = false);

/// Returns the height of the toolbar.
[[nodiscard]] float GetToolbarHeight();
