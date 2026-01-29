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

void UndoStack::PushVisibility(UndoTarget target, std::vector<NodeStateChange> changes)
{
	if (changes.empty())
	{
		return;
	}
	UndoAction action;
	action.type = UndoActionType::ChangeVisibility;
	action.target = target;
	action.state_changes = std::move(changes);
	Push(std::move(action));
}

void UndoStack::PushFrozen(UndoTarget target, std::vector<NodeStateChange> changes)
{
	if (changes.empty())
	{
		return;
	}
	UndoAction action;
	action.type = UndoActionType::ChangeFrozen;
	action.target = target;
	action.state_changes = std::move(changes);
	Push(std::move(action));
}

void UndoStack::PushTransform(UndoTarget target, std::vector<TransformChange> changes)
{
	if (changes.empty())
	{
		return;
	}
	UndoAction action;
	action.type = UndoActionType::TransformNode;
	action.target = target;
	action.transform_changes = std::move(changes);
	Push(std::move(action));
}

void UndoStack::Undo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes,
                     std::vector<NodeProperties>& project_props, std::vector<NodeProperties>& scene_props)
{
	if (!CanUndo())
	{
		return;
	}

	cursor_ -= 1;
	Apply(actions_[cursor_], true, project_nodes, scene_nodes, &project_props, &scene_props);
}

void UndoStack::Redo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes,
                     std::vector<NodeProperties>& project_props, std::vector<NodeProperties>& scene_props)
{
	if (!CanRedo())
	{
		return;
	}

	Apply(actions_[cursor_], false, project_nodes, scene_nodes, &project_props, &scene_props);
	cursor_ += 1;
}

void UndoStack::Undo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes)
{
	if (!CanUndo())
	{
		return;
	}

	cursor_ -= 1;
	Apply(actions_[cursor_], true, project_nodes, scene_nodes, nullptr, nullptr);
}

void UndoStack::Redo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes)
{
	if (!CanRedo())
	{
		return;
	}

	Apply(actions_[cursor_], false, project_nodes, scene_nodes, nullptr, nullptr);
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
	std::vector<TreeNode>& scene_nodes,
	std::vector<NodeProperties>* project_props,
	std::vector<NodeProperties>* scene_props)
{
	std::vector<TreeNode>* nodes = ResolveNodes(action.target, project_nodes, scene_nodes);
	if (nodes == nullptr)
	{
		return;
	}

	// Handle visibility and frozen changes (need props)
	if (action.type == UndoActionType::ChangeVisibility ||
	    action.type == UndoActionType::ChangeFrozen)
	{
		std::vector<NodeProperties>* props = (action.target == UndoTarget::Project)
			? project_props : scene_props;
		if (props == nullptr)
		{
			return;  // Can't apply visibility/frozen without props
		}
		for (const auto& change : action.state_changes)
		{
			if (change.node_id >= 0 && change.node_id < static_cast<int>(props->size()))
			{
				NodeProperties& p = (*props)[change.node_id];
				if (action.type == UndoActionType::ChangeVisibility)
				{
					// Undo restores previous value, redo applies the opposite
					p.visible = undo ? change.previous_value : !change.previous_value;
				}
				else
				{
					p.frozen = undo ? change.previous_value : !change.previous_value;
				}
			}
		}
		return;
	}

	// Handle transform changes (need props)
	if (action.type == UndoActionType::TransformNode)
	{
		std::vector<NodeProperties>* props = (action.target == UndoTarget::Project)
			? project_props : scene_props;
		if (props == nullptr)
		{
			return;  // Can't apply transform without props
		}
		for (const auto& change : action.transform_changes)
		{
			if (change.node_id < 0 || change.node_id >= static_cast<int>(props->size()))
			{
				continue;
			}
			NodeProperties& p = (*props)[change.node_id];
			const TransformState& state = undo ? change.before : change.after;
			p.position[0] = state.position[0];
			p.position[1] = state.position[1];
			p.position[2] = state.position[2];
			p.rotation[0] = state.rotation[0];
			p.rotation[1] = state.rotation[1];
			p.rotation[2] = state.rotation[2];
			p.scale[0] = state.scale[0];
			p.scale[1] = state.scale[1];
			p.scale[2] = state.scale[2];

			// Restore brush vertices if present
			const std::vector<float>& verts = undo ? change.before_vertices : change.after_vertices;
			if (!verts.empty())
			{
				p.brush_vertices = verts;
			}

			// Restore brush indices if present (for mirror winding)
			const std::vector<uint32_t>& indices = undo ? change.before_indices : change.after_indices;
			if (!indices.empty())
			{
				p.brush_indices = indices;
			}
		}
		return;
	}

	// Handle node-based actions
	if (action.node_id < 0 || action.node_id >= static_cast<int>(nodes->size()))
	{
		return;
	}

	TreeNode& node = (*nodes)[action.node_id];
	switch (action.type)
	{
		case UndoActionType::CreateNode:
			node.deleted = undo;
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
		case UndoActionType::ChangeVisibility:
		case UndoActionType::ChangeFrozen:
		case UndoActionType::TransformNode:
			// Already handled above
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
