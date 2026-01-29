#pragma once

#include "editor_state.h"
#include "undo_stack.h"
#include "ui_scene.h"

#include <string>
#include <vector>

/// Result of a group operation.
struct GroupResult
{
  bool success = false;
  int group_id = -1;    ///< ID of the created group node
  std::string error;    ///< Error message if success is false
};

/// Group the selected nodes into a new container.
/// Creates a new folder node and reparents all selected nodes into it.
/// @param scene_panel The scene panel state (provides selection).
/// @param nodes The scene tree nodes.
/// @param props The node properties.
/// @param undo_stack Optional undo stack for recording the operation.
/// @return GroupResult with the new group node ID.
[[nodiscard]] GroupResult GroupSelectedNodes(
    ScenePanelState& scene_panel,
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    UndoStack* undo_stack = nullptr);

/// Ungroup a container, moving its children to the container's parent.
/// Deletes the container node after reparenting children.
/// @param container_id The container node to ungroup.
/// @param nodes The scene tree nodes.
/// @param props The node properties.
/// @param scene_panel Scene panel for updating selection.
/// @param undo_stack Optional undo stack for recording the operation.
/// @return true if ungrouping succeeded.
bool UngroupContainer(
    int container_id,
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    ScenePanelState& scene_panel,
    UndoStack* undo_stack = nullptr);

/// Find the common parent of multiple nodes.
/// @param node_ids The node IDs to check.
/// @param nodes The scene tree nodes.
/// @return Parent ID, or -1 if nodes have different parents or no common parent.
[[nodiscard]] int FindCommonParent(
    const std::vector<int>& node_ids,
    const std::vector<TreeNode>& nodes);

/// Find the parent of a node in the tree.
/// @param nodes The scene tree nodes.
/// @param root_id The root node ID to search from.
/// @param node_id The node ID to find the parent of.
/// @return Parent ID, or -1 if not found.
[[nodiscard]] int FindParentId(
    const std::vector<TreeNode>& nodes,
    int root_id,
    int node_id);

/// Check if potential_ancestor is an ancestor of node_id.
/// @return true if node_id is a descendant of potential_ancestor.
[[nodiscard]] bool IsDescendantOf(
    const std::vector<TreeNode>& nodes,
    int node_id,
    int potential_ancestor);
