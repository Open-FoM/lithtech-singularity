#pragma once

#include "editor_state.h"

#include "imgui.h"

#include <string>

struct ScenePanelState
{
	int selected_id = -1;
	ImGuiTextFilter filter;
	TreeUiState tree_ui;
	std::string error;
};

class UndoStack;

void DrawScenePanel(
	ScenePanelState& state,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	SelectionTarget& active_target,
	UndoStack* undo_stack);
