#pragma once

#include "editor_state.h"
#include "undo_stack.h"
#include "ui_console.h"
#include "ui_project.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "ui_worlds.h"
#include "viewport/world_settings.h"

#include <string>
#include <vector>

struct EditorSession
{
  std::string project_root;
  std::string world_file;
  std::string project_error;
  std::string scene_error;
  std::vector<std::string> recent_projects;

  std::vector<NodeProperties> project_props;
  std::vector<NodeProperties> scene_props;
  std::vector<TreeNode> project_nodes;
  std::vector<TreeNode> scene_nodes;

  ProjectPanelState project_panel;
  WorldBrowserState world_browser;
  ScenePanelState scene_panel;
  ConsolePanelState console_panel;
  ViewportPanelState viewport_panel;

  SelectionTarget active_target = SelectionTarget::Project;
  WorldRenderSettingsCache world_settings_cache;

  UndoStack undo_stack;
};
