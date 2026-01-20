#pragma once

#include "editor_state.h"

#include "imgui.h"

#include <unordered_map>
#include <string>

struct ScenePanelState
{
	int selected_id = -1;
	ImGuiTextFilter filter;
	TreeUiState tree_ui;
	std::string error;
	bool isolate_selected = false;
	std::unordered_map<std::string, bool> type_visibility;
	std::unordered_map<std::string, bool> class_visibility;
};

class UndoStack;

bool SceneNodePassesFilters(
	const ScenePanelState& state,
	int node_id,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props);

void DrawScenePanel(
	ScenePanelState& state,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	SelectionTarget& active_target,
	UndoStack* undo_stack);
