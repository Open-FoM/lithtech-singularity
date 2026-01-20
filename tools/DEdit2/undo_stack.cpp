#include "undo_stack.h"

#include <algorithm>

void UndoStack::Clear()
{
	actions_.clear();
	cursor_ = 0;
}

bool UndoStack::CanUndo() const
{
	return cursor_ > 0;
}

bool UndoStack::CanRedo() const
{
	return cursor_ < actions_.size();
}

void UndoStack::PushCreate(UndoTarget target, int node_id)
{
	UndoAction action;
	action.type = UndoActionType::CreateNode;
	action.target = target;
	action.node_id = node_id;
	Push(std::move(action));
}

void UndoStack::PushDelete(UndoTarget target, int node_id, bool prev_deleted)
{
	UndoAction action;
	action.type = UndoActionType::DeleteNode;
	action.target = target;
	action.node_id = node_id;
	action.prev_deleted = prev_deleted;
	Push(std::move(action));
}

void UndoStack::PushRename(UndoTarget target, int node_id, std::string before, std::string after)
{
	UndoAction action;
	action.type = UndoActionType::RenameNode;
	action.target = target;
	action.node_id = node_id;
	action.before_name = std::move(before);
	action.after_name = std::move(after);
	Push(std::move(action));
}

void UndoStack::PushMove(UndoTarget target, int node_id, int old_parent, int new_parent)
{
	UndoAction action;
	action.type = UndoActionType::MoveNode;
	action.target = target;
	action.node_id = node_id;
	action.old_parent = old_parent;
	action.new_parent = new_parent;
	Push(std::move(action));
}

void UndoStack::Undo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes)
{
	if (!CanUndo())
	{
		return;
	}

	cursor_ -= 1;
	Apply(actions_[cursor_], true, project_nodes, scene_nodes);
}

void UndoStack::Redo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes)
{
	if (!CanRedo())
	{
		return;
	}

	Apply(actions_[cursor_], false, project_nodes, scene_nodes);
	cursor_ += 1;
}

void UndoStack::Push(UndoAction action)
{
	if (cursor_ < actions_.size())
	{
		actions_.erase(actions_.begin() + static_cast<long>(cursor_), actions_.end());
	}
	actions_.push_back(std::move(action));
	cursor_ = actions_.size();
}

void UndoStack::Apply(
	const UndoAction& action,
	bool undo,
	std::vector<TreeNode>& project_nodes,
	std::vector<TreeNode>& scene_nodes)
{
	std::vector<TreeNode>* nodes = ResolveNodes(action.target, project_nodes, scene_nodes);
	if (!nodes)
	{
		return;
	}
	if (action.node_id < 0 || action.node_id >= static_cast<int>(nodes->size()))
	{
		return;
	}

	TreeNode& node = (*nodes)[action.node_id];
	switch (action.type)
	{
		case UndoActionType::CreateNode:
			node.deleted = undo ? true : false;
			break;
		case UndoActionType::DeleteNode:
			node.deleted = undo ? action.prev_deleted : true;
			break;
		case UndoActionType::RenameNode:
			node.name = undo ? action.before_name : action.after_name;
			break;
		case UndoActionType::MoveNode:
			MoveNode(*nodes, action.node_id, undo ? action.new_parent : action.old_parent,
				undo ? action.old_parent : action.new_parent);
			break;
	}
}

std::vector<TreeNode>* UndoStack::ResolveNodes(
	UndoTarget target,
	std::vector<TreeNode>& project_nodes,
	std::vector<TreeNode>& scene_nodes)
{
	return target == UndoTarget::Project ? &project_nodes : &scene_nodes;
}

void UndoStack::MoveNode(std::vector<TreeNode>& nodes, int node_id, int old_parent, int new_parent)
{
	if (node_id < 0 || node_id >= static_cast<int>(nodes.size()))
	{
		return;
	}
	if (old_parent >= 0 && old_parent < static_cast<int>(nodes.size()))
	{
		auto& children = nodes[old_parent].children;
		children.erase(std::remove(children.begin(), children.end(), node_id), children.end());
	}
	if (new_parent >= 0 && new_parent < static_cast<int>(nodes.size()))
	{
		auto& children = nodes[new_parent].children;
		if (std::find(children.begin(), children.end(), node_id) == children.end())
		{
			children.push_back(node_id);
		}
	}
}
