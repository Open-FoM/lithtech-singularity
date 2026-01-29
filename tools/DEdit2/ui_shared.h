#pragma once

#include "editor_state.h"
#include "undo_stack.h"

#include "imgui.h"

#include <string>
#include <vector>

using TreeNodeFilter = bool (*)(
	int node_id,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>* props,
	void* user);

struct ProjectContextAction
{
	bool load_world = false;
	std::string world_path;
};

enum class ExpandMode
{
	None,
	ExpandAll,
	CollapseAll
};

enum class SceneRowIcon
{
	Visibility,
	Freeze
};

bool IsNameEmptyOrWhitespace(const std::string& name);
bool HasDuplicateSiblingName(
	const std::vector<TreeNode>& nodes,
	int root_id,
	int node_id,
	const std::string& name);
std::string BuildNodePath(const std::vector<TreeNode>& nodes, int root_id, int target_id);
[[nodiscard]] std::vector<SceneRowIcon> BuildSceneRowIcons(const TreeNode& node, const NodeProperties& props);

void DrawTreeNodes(
	std::vector<TreeNode>& nodes,
	int node_id,
	int& selected_id,
	const ImGuiTextFilter& filter,
	ExpandMode expand_mode,
	TreeUiState& ui_state,
	bool is_scene,
	SelectionTarget target,
	SelectionTarget& active_target,
	const std::string* project_root,
	const std::vector<NodeProperties>* props,
	ProjectContextAction* project_action,
	UndoStack* undo_stack,
	TreeNodeFilter node_filter = nullptr,
	void* node_filter_user = nullptr);

void HandlePendingCreate(
	TreeUiState& ui_state,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int& selected_id,
	const char* default_folder,
	const char* default_object,
	const char* folder_type,
	const char* object_type,
	UndoTarget target,
	UndoStack* undo_stack);

void HandleRenamePopup(
	TreeUiState& ui_state,
	std::vector<TreeNode>& nodes,
	const char* popup_id,
	UndoTarget target,
	UndoStack* undo_stack);
