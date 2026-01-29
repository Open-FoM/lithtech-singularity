#include "node_grouping.h"

namespace
{

/// Recursively search for a node's parent starting from root_id.
int FindParentIdRecursive(const std::vector<TreeNode>& nodes, int current_id, int target_id)
{
  if (current_id < 0 || current_id >= static_cast<int>(nodes.size()))
  {
    return -1;
  }

  const TreeNode& node = nodes[current_id];
  if (node.deleted)
  {
    return -1;
  }

  for (int child_id : node.children)
  {
    if (child_id == target_id)
    {
      return current_id;
    }
    int nested = FindParentIdRecursive(nodes, child_id, target_id);
    if (nested != -1)
    {
      return nested;
    }
  }
  return -1;
}

/// Remove a child from a parent's children list.
void RemoveChildFromParent(std::vector<TreeNode>& nodes, int parent_id, int child_id)
{
  if (parent_id < 0 || parent_id >= static_cast<int>(nodes.size()))
  {
    return;
  }
  auto& children = nodes[parent_id].children;
  children.erase(std::remove(children.begin(), children.end(), child_id), children.end());
}

/// Add a child to a parent's children list.
void AddChildToParent(std::vector<TreeNode>& nodes, int parent_id, int child_id)
{
  if (parent_id < 0 || parent_id >= static_cast<int>(nodes.size()))
  {
    return;
  }
  nodes[parent_id].children.push_back(child_id);
}

} // namespace

int FindParentId(const std::vector<TreeNode>& nodes, int root_id, int node_id)
{
  return FindParentIdRecursive(nodes, root_id, node_id);
}

bool IsDescendantOf(const std::vector<TreeNode>& nodes, int node_id, int potential_ancestor)
{
  if (node_id < 0 || potential_ancestor < 0)
  {
    return false;
  }
  if (node_id == potential_ancestor)
  {
    return false; // A node is not its own descendant
  }

  // Walk up from node_id looking for potential_ancestor
  int current = node_id;
  while (true)
  {
    int parent = FindParentId(nodes, 0, current);
    if (parent < 0)
    {
      return false;
    }
    if (parent == potential_ancestor)
    {
      return true;
    }
    current = parent;
  }
}

int FindCommonParent(const std::vector<int>& node_ids, const std::vector<TreeNode>& nodes)
{
  if (node_ids.empty())
  {
    return -1;
  }

  // Get parent of first node
  int common_parent = FindParentId(nodes, 0, node_ids[0]);
  if (common_parent < 0)
  {
    return -1;
  }

  // Check if all other nodes have the same parent
  for (size_t i = 1; i < node_ids.size(); ++i)
  {
    int parent = FindParentId(nodes, 0, node_ids[i]);
    if (parent != common_parent)
    {
      return -1; // Different parents
    }
  }

  return common_parent;
}

GroupResult GroupSelectedNodes(
    ScenePanelState& scene_panel,
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    UndoStack* undo_stack)
{
  GroupResult result;

  // Get selected node IDs
  std::vector<int> selected_ids(scene_panel.selected_ids.begin(), scene_panel.selected_ids.end());
  if (selected_ids.empty())
  {
    result.error = "No nodes selected";
    return result;
  }

  // Filter out deleted nodes and the root node (0)
  std::vector<int> valid_ids;
  for (int id : selected_ids)
  {
    if (id <= 0 || id >= static_cast<int>(nodes.size()))
    {
      continue;
    }
    if (nodes[id].deleted)
    {
      continue;
    }
    valid_ids.push_back(id);
  }

  if (valid_ids.empty())
  {
    result.error = "No valid nodes to group";
    return result;
  }

  // Find common parent
  int common_parent = FindCommonParent(valid_ids, nodes);
  if (common_parent < 0)
  {
    result.error = "Selected nodes must share the same parent";
    return result;
  }

  // Create a new folder node as child of common parent
  const int new_group_id = static_cast<int>(nodes.size());
  TreeNode group_node;
  group_node.name = "Group";
  group_node.is_folder = true;
  group_node.deleted = false;
  nodes.push_back(group_node);

  // Add properties for the new node
  NodeProperties group_props;
  group_props.type = "Folder";
  props.push_back(group_props);

  // Add the group to the common parent
  AddChildToParent(nodes, common_parent, new_group_id);

  // Record undo for creating the group node
  if (undo_stack != nullptr)
  {
    undo_stack->PushCreate(UndoTarget::Scene, new_group_id);
  }

  // Reparent all selected nodes to the new group
  for (int node_id : valid_ids)
  {
    int old_parent = FindParentId(nodes, 0, node_id);
    if (old_parent < 0)
    {
      continue;
    }

    // Remove from old parent
    RemoveChildFromParent(nodes, old_parent, node_id);

    // Add to new group
    nodes[new_group_id].children.push_back(node_id);

    // Record undo for move
    if (undo_stack != nullptr)
    {
      undo_stack->PushMove(UndoTarget::Scene, node_id, old_parent, new_group_id);
    }
  }

  // Update selection to the new group
  scene_panel.selected_ids.clear();
  scene_panel.selected_ids.insert(new_group_id);
  scene_panel.primary_selection = new_group_id;

  result.success = true;
  result.group_id = new_group_id;
  return result;
}

bool UngroupContainer(
    int container_id,
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    ScenePanelState& scene_panel,
    UndoStack* undo_stack)
{
  if (container_id <= 0 || container_id >= static_cast<int>(nodes.size()))
  {
    return false;
  }

  TreeNode& container = nodes[container_id];
  if (container.deleted)
  {
    return false;
  }

  if (!container.is_folder)
  {
    return false; // Can only ungroup folder/container nodes
  }

  // Find the container's parent
  int parent_id = FindParentId(nodes, 0, container_id);
  if (parent_id < 0)
  {
    return false; // Container has no parent (shouldn't happen for valid containers)
  }

  // Collect children to reparent
  std::vector<int> children_to_move = container.children;

  // Move each child to the container's parent
  for (int child_id : children_to_move)
  {
    // Add to parent
    AddChildToParent(nodes, parent_id, child_id);

    // Record undo for move
    if (undo_stack != nullptr)
    {
      undo_stack->PushMove(UndoTarget::Scene, child_id, container_id, parent_id);
    }
  }

  // Clear container's children
  container.children.clear();

  // Remove container from its parent
  RemoveChildFromParent(nodes, parent_id, container_id);

  // Mark container as deleted
  bool prev_deleted = container.deleted;
  container.deleted = true;
  if (undo_stack != nullptr)
  {
    undo_stack->PushDelete(UndoTarget::Scene, container_id, prev_deleted);
  }

  // Update selection to the ungrouped children
  scene_panel.selected_ids.clear();
  for (int child_id : children_to_move)
  {
    scene_panel.selected_ids.insert(child_id);
  }
  if (!children_to_move.empty())
  {
    scene_panel.primary_selection = children_to_move[0];
  }
  else
  {
    scene_panel.primary_selection = -1;
  }

  (void)props; // Props not modified in ungroup

  return true;
}
