#pragma once

#include "brush/brush_primitive.h"
#include "editor_state.h"
#include "multi_viewport.h"
#include "undo_stack.h"
#include "ui_console.h"
#include "ui_dock.h"
#include "ui_tools_dock.h"
#include "ui_project.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "ui_worlds.h"
#include "viewport/world_settings.h"

#include <string>
#include <vector>

/// State for the primitive creation dialog.
struct PrimitiveDialogState {
  bool open = false;
  PrimitiveType type = PrimitiveType::None;

  // Last-used parameters for each primitive type
  BoxParams box_params;
  CylinderParams cylinder_params;
  PyramidParams pyramid_params;
  SphereParams sphere_params;
  PlaneParams plane_params;
};

/// Panel visibility settings for the editor.
struct PanelVisibility {
  bool show_project = true;
  bool show_worlds = true;
  bool show_scene = true;
  bool show_properties = true;
  bool show_console = true;
  bool show_tools = true;
};

struct EditorSession
{
  std::string project_root;
  std::string world_file;
  std::string project_error;
  std::string scene_error;
  std::vector<std::string> recent_projects;

  /// Panel visibility settings.
  PanelVisibility panel_visibility;

  std::vector<NodeProperties> project_props;
  std::vector<NodeProperties> scene_props;
  std::vector<TreeNode> project_nodes;
  std::vector<TreeNode> scene_nodes;

  ProjectPanelState project_panel;
  WorldBrowserState world_browser;
  ScenePanelState scene_panel;
  ConsolePanelState console_panel;
  MultiViewportState multi_viewport;

  /// Convenience accessor for the active viewport (backward compatible).
  [[nodiscard]] ViewportPanelState& viewport_panel() { return multi_viewport.ActiveViewport(); }
  [[nodiscard]] const ViewportPanelState& viewport_panel() const { return multi_viewport.ActiveViewport(); }

  SelectionTarget active_target = SelectionTarget::Project;
  WorldRenderSettingsCache world_settings_cache;

  UndoStack undo_stack;

  PrimitiveDialogState primitive_dialog;
  ToolsPanelState tools_panel;
};
