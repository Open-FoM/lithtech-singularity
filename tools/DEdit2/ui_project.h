#pragma once

#include "editor_state.h"

#include "imgui.h"

#include <string>

struct ProjectPanelState
{
	int selected_id = -1;
	ImGuiTextFilter filter;
	TreeUiState tree_ui;
	std::string error;
};

struct ProjectContextAction;
class UndoStack;

void DrawProjectPanel(
	ProjectPanelState& state,
	const std::string& project_root,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	SelectionTarget& active_target,
	ProjectContextAction* project_action,
	UndoStack* undo_stack);
