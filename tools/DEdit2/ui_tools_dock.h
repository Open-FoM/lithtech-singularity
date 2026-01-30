#pragma once

#include "ui_dock.h"

/// Available tools in the tools panel.
enum class EditorTool
{
  None = -1,
  // Selection/Transform tools
  Select,
  Move,
  Rotate,
  Scale,
  // Primitive creation tools
  Box,
  Cylinder,
  Pyramid,
  Sphere,
  Dome,
  Plane,

  Count
};

/// Returns the display name for a tool.
[[nodiscard]] const char* ToolName(EditorTool tool);

/// Returns the shortcut hint for a tool (if any).
[[nodiscard]] const char* ToolShortcut(EditorTool tool);

/// Returns the corresponding primitive type for a tool, or None if not a primitive tool.
[[nodiscard]] PrimitiveType ToolToPrimitiveType(EditorTool tool);

/// State for the tools panel.
struct ToolsPanelState
{
  bool visible = true;           ///< Whether the panel is visible
  EditorTool selected_tool = EditorTool::Select; ///< Currently selected tool
};

/// Result from drawing the tools panel.
struct ToolsPanelResult
{
  bool tool_changed = false;           ///< True if user selected a different tool
  EditorTool new_tool = EditorTool::None; ///< The newly selected tool
  PrimitiveType create_primitive = PrimitiveType::None; ///< Primitive to create (if any)

  // CSG operation requests (trigger dialogs)
  bool csg_hollow = false;
  bool csg_carve = false;
  bool csg_split = false;
  bool csg_join = false;
  bool csg_triangulate = false;

  // Geometry operation requests (EPIC-09)
  bool geometry_flip = false;
  bool geometry_weld = false;
  bool geometry_extrude = false;
};

/// Draw the dockable Tools panel.
/// @param state The tools panel state.
/// @return Result containing any tool changes or primitive creation requests.
ToolsPanelResult DrawToolsPanel(ToolsPanelState& state);

/// Draw a quick primitive popup (invoked by Shift+A).
/// @param result Output for primitive creation request.
/// @return True if popup is open.
bool DrawPrimitivePopup(PrimitiveType& result);
