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
	MoveNode,
	ChangeVisibility,
	ChangeFrozen,
	TransformNode
};

/// Stores the previous visibility/frozen state for a single node.
struct NodeStateChange
{
	int node_id = -1;
	bool previous_value = false;
};

/// Stores the complete transform state for a node.
struct TransformState
{
	float position[3] = {0.0f, 0.0f, 0.0f};
	float rotation[3] = {0.0f, 0.0f, 0.0f};
	float scale[3] = {1.0f, 1.0f, 1.0f};
};

/// Stores transform change for a single node (before/after states).
struct TransformChange
{
	int node_id = -1;
	TransformState before;
	TransformState after;
	std::vector<float> before_vertices;      ///< Brush vertices before transform
	std::vector<float> after_vertices;       ///< Brush vertices after transform
	std::vector<uint32_t> before_indices;    ///< Brush indices before transform (for mirror winding)
	std::vector<uint32_t> after_indices;     ///< Brush indices after transform (for mirror winding)
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

	/// For ChangeVisibility/ChangeFrozen: batch of affected nodes and their previous values.
	std::vector<NodeStateChange> state_changes;

	/// For TransformNode: batch of transform changes (supports multi-selection).
	std::vector<TransformChange> transform_changes;
};

class UndoStack
{
public:
	void Clear();
	bool CanUndo() const;
	bool CanRedo() const;

	/// Returns the current undo position (for dirty state tracking).
	[[nodiscard]] size_t GetPosition() const { return cursor_; }

	void PushCreate(UndoTarget target, int node_id);
	void PushDelete(UndoTarget target, int node_id, bool prev_deleted);
	void PushRename(UndoTarget target, int node_id, std::string before, std::string after);
	void PushMove(UndoTarget target, int node_id, int old_parent, int new_parent);

	/// Push a batch visibility change (visible flag).
	/// @param target Which tree the nodes belong to.
	/// @param changes Vector of node IDs and their previous visible values.
	void PushVisibility(UndoTarget target, std::vector<NodeStateChange> changes);

	/// Push a batch frozen change.
	/// @param target Which tree the nodes belong to.
	/// @param changes Vector of node IDs and their previous frozen values.
	void PushFrozen(UndoTarget target, std::vector<NodeStateChange> changes);

	/// Push a batch transform change (supports multi-selection).
	/// @param target Which tree the nodes belong to.
	/// @param changes Vector of transform changes with before/after states.
	void PushTransform(UndoTarget target, std::vector<TransformChange> changes);

	void Undo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes,
	          std::vector<NodeProperties>& project_props, std::vector<NodeProperties>& scene_props);
	void Redo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes,
	          std::vector<NodeProperties>& project_props, std::vector<NodeProperties>& scene_props);

	/// Legacy overloads for backward compatibility (no-op for visibility/frozen actions).
	void Undo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes);
	void Redo(std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes);

private:
	void Push(UndoAction action);
	void Apply(const UndoAction& action, bool undo,
	           std::vector<TreeNode>& project_nodes, std::vector<TreeNode>& scene_nodes,
	           std::vector<NodeProperties>* project_props, std::vector<NodeProperties>* scene_props);
	static std::vector<TreeNode>* ResolveNodes(
		UndoTarget target,
		std::vector<TreeNode>& project_nodes,
		std::vector<TreeNode>& scene_nodes);
	static void MoveNode(std::vector<TreeNode>& nodes, int node_id, int old_parent, int new_parent);

	std::vector<UndoAction> actions_;
	size_t cursor_ = 0;
};
