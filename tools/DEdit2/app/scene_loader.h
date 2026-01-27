#pragma once

#include <string>
#include <vector>

#include "editor_state.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport/world_settings.h"

struct DiligentContext;

void LoadSceneWorld(
  const std::string& path,
  DiligentContext& diligent,
  const std::string& project_root,
  std::string& world_file,
  ViewportPanelState& viewport_panel,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  ScenePanelState& scene_panel,
  std::string& scene_error,
  SelectionTarget& active_target,
  WorldRenderSettingsCache& world_settings_cache);
