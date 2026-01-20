#pragma once

#include "editor_state.h"

void DrawPropertiesPanel(
	SelectionTarget active_target,
	std::vector<TreeNode>& project_nodes,
	std::vector<NodeProperties>& project_props,
	int project_selected_id,
	std::vector<TreeNode>& scene_nodes,
	std::vector<NodeProperties>& scene_props,
	int scene_selected_id);
