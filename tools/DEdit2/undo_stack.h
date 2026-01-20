#pragma once

#include "editor_state.h"

#include <string>
#include <vector>

enum class UndoTarget
{
	Project,
	Scene
};

enum class UndoActionType
{
	CreateNode,
	DeleteNode,
	RenameNode,
	MoveNode
};

struct UndoAction
{
	UndoActionType type = UndoActionType::CreateNode;
	UndoTarget target = UndoTarget::Project;
	int node_id = -1;
	bool prev_deleted = false;
	std::string before_name;
	std::string after_name;
	int old_parent = -1;
	int new_parent = -1;
};

class UndoStack
{
public:
	void Clear();
	bool CanUndo() const;
	bool CanRedo() const;

	void PushCreate(UndoTarget target, int node_id);
	void PushDelete(UndoTarget target, int node_id, bool prev_deleted);
	void PushRename(UndoTarget target, int node_id, std::string before, std::string after);
	void PushMove(UndoTarget target, int node_id, int old_parent, int new_parent);

	void Undo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes);
	void Redo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes);

private:
	void Push(UndoAction action);
	void Apply(const UndoAction& action, bool undo, std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes);
	static std::vector<TreeNode>* ResolveNodes(
		UndoTarget target,
		std::vector<TreeNode>& project_nodes,
		std::vector<TreeNode>& scene_nodes);
	static void MoveNode(std::vector<TreeNode>& nodes, int node_id, int old_parent, int new_parent);

	std::vector<UndoAction> actions_;
	size_t cursor_ = 0;
};
