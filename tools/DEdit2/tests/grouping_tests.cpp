#include "grouping/node_grouping.h"
#include "editor_state.h"
#include "ui_scene.h"

#include <gtest/gtest.h>

namespace
{

// Helper to build a simple scene tree for testing:
// 0: World (root folder)
//   1: Brush01
//   2: Brush02
//   3: Light01
//   4: Folder (is_folder)
//     5: NestedBrush
void BuildTestScene(
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    ScenePanelState& panel)
{
  nodes.clear();
  props.clear();
  panel = ScenePanelState{};

  // 0: World (root)
  TreeNode world;
  world.name = "World";
  world.is_folder = true;
  world.children = {1, 2, 3, 4};
  nodes.push_back(world);
  NodeProperties world_props;
  world_props.type = "World";
  props.push_back(world_props);

  // 1: Brush01
  TreeNode brush1;
  brush1.name = "Brush01";
  nodes.push_back(brush1);
  NodeProperties brush1_props;
  brush1_props.type = "Brush";
  props.push_back(brush1_props);

  // 2: Brush02
  TreeNode brush2;
  brush2.name = "Brush02";
  nodes.push_back(brush2);
  NodeProperties brush2_props;
  brush2_props.type = "Brush";
  props.push_back(brush2_props);

  // 3: Light01
  TreeNode light1;
  light1.name = "Light01";
  nodes.push_back(light1);
  NodeProperties light1_props;
  light1_props.type = "Light";
  props.push_back(light1_props);

  // 4: Folder
  TreeNode folder;
  folder.name = "Folder";
  folder.is_folder = true;
  folder.children = {5};
  nodes.push_back(folder);
  NodeProperties folder_props;
  folder_props.type = "Folder";
  props.push_back(folder_props);

  // 5: NestedBrush
  TreeNode nested;
  nested.name = "NestedBrush";
  nodes.push_back(nested);
  NodeProperties nested_props;
  nested_props.type = "Brush";
  props.push_back(nested_props);
}

} // namespace

// ============================================================================
// FindParentId Tests
// ============================================================================

TEST(NodeGrouping, FindParentId_RootHasNoParent)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  int parent = FindParentId(nodes, 0, 0);
  EXPECT_EQ(parent, -1);
}

TEST(NodeGrouping, FindParentId_DirectChild)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Brush01 (1) is a direct child of World (0)
  int parent = FindParentId(nodes, 0, 1);
  EXPECT_EQ(parent, 0);

  // Light01 (3) is also a direct child of World (0)
  parent = FindParentId(nodes, 0, 3);
  EXPECT_EQ(parent, 0);
}

TEST(NodeGrouping, FindParentId_NestedChild)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // NestedBrush (5) is a child of Folder (4)
  int parent = FindParentId(nodes, 0, 5);
  EXPECT_EQ(parent, 4);
}

TEST(NodeGrouping, FindParentId_NotFound)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Invalid node ID
  int parent = FindParentId(nodes, 0, 99);
  EXPECT_EQ(parent, -1);
}

// ============================================================================
// IsDescendantOf Tests
// ============================================================================

TEST(NodeGrouping, IsDescendantOf_DirectChild)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Brush01 (1) is a descendant of World (0)
  EXPECT_TRUE(IsDescendantOf(nodes, 1, 0));
}

TEST(NodeGrouping, IsDescendantOf_NestedChild)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // NestedBrush (5) is a descendant of World (0) and Folder (4)
  EXPECT_TRUE(IsDescendantOf(nodes, 5, 0));
  EXPECT_TRUE(IsDescendantOf(nodes, 5, 4));
}

TEST(NodeGrouping, IsDescendantOf_NotDescendant)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Brush01 (1) is not a descendant of Folder (4)
  EXPECT_FALSE(IsDescendantOf(nodes, 1, 4));

  // Folder (4) is not a descendant of Brush01 (1)
  EXPECT_FALSE(IsDescendantOf(nodes, 4, 1));
}

TEST(NodeGrouping, IsDescendantOf_SameNode)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // A node is not its own descendant
  EXPECT_FALSE(IsDescendantOf(nodes, 1, 1));
  EXPECT_FALSE(IsDescendantOf(nodes, 0, 0));
}

// ============================================================================
// FindCommonParent Tests
// ============================================================================

TEST(NodeGrouping, FindCommonParent_Siblings)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Brush01 (1) and Brush02 (2) share World (0) as parent
  std::vector<int> node_ids = {1, 2};
  int common = FindCommonParent(node_ids, nodes);
  EXPECT_EQ(common, 0);
}

TEST(NodeGrouping, FindCommonParent_MultipleSiblings)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Brush01 (1), Brush02 (2), Light01 (3) share World (0) as parent
  std::vector<int> node_ids = {1, 2, 3};
  int common = FindCommonParent(node_ids, nodes);
  EXPECT_EQ(common, 0);
}

TEST(NodeGrouping, FindCommonParent_DifferentParents)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Brush01 (1) has parent World (0), NestedBrush (5) has parent Folder (4)
  std::vector<int> node_ids = {1, 5};
  int common = FindCommonParent(node_ids, nodes);
  EXPECT_EQ(common, -1);
}

TEST(NodeGrouping, FindCommonParent_Empty)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  std::vector<int> node_ids;
  int common = FindCommonParent(node_ids, nodes);
  EXPECT_EQ(common, -1);
}

TEST(NodeGrouping, FindCommonParent_SingleNode)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  std::vector<int> node_ids = {1};
  int common = FindCommonParent(node_ids, nodes);
  EXPECT_EQ(common, 0);
}

// ============================================================================
// GroupSelectedNodes Tests
// ============================================================================

TEST(NodeGrouping, GroupSelectedNodes_TwoSiblings)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Select Brush01 and Brush02
  panel.selected_ids.insert(1);
  panel.selected_ids.insert(2);
  panel.primary_selection = 1;

  GroupResult result = GroupSelectedNodes(panel, nodes, props, nullptr);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.group_id, 0);
  EXPECT_TRUE(result.error.empty());

  // The new group should be a folder
  EXPECT_TRUE(nodes[result.group_id].is_folder);
  EXPECT_EQ(nodes[result.group_id].name, "Group");

  // The new group should have 2 children
  EXPECT_EQ(nodes[result.group_id].children.size(), 2u);

  // Brush01 and Brush02 should now be children of the group
  auto& group_children = nodes[result.group_id].children;
  EXPECT_TRUE(std::find(group_children.begin(), group_children.end(), 1) != group_children.end());
  EXPECT_TRUE(std::find(group_children.begin(), group_children.end(), 2) != group_children.end());

  // The group should be a child of the original parent (World)
  auto& world_children = nodes[0].children;
  EXPECT_TRUE(std::find(world_children.begin(), world_children.end(), result.group_id) != world_children.end());

  // Brush01 and Brush02 should no longer be direct children of World
  EXPECT_TRUE(std::find(world_children.begin(), world_children.end(), 1) == world_children.end());
  EXPECT_TRUE(std::find(world_children.begin(), world_children.end(), 2) == world_children.end());

  // Selection should now be the new group
  EXPECT_EQ(panel.selected_ids.size(), 1u);
  EXPECT_TRUE(panel.selected_ids.count(result.group_id) > 0);
}

TEST(NodeGrouping, GroupSelectedNodes_NoSelection)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  GroupResult result = GroupSelectedNodes(panel, nodes, props, nullptr);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.group_id, -1);
  EXPECT_FALSE(result.error.empty());
}

TEST(NodeGrouping, GroupSelectedNodes_DifferentParents)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Select Brush01 (child of World) and NestedBrush (child of Folder)
  panel.selected_ids.insert(1);
  panel.selected_ids.insert(5);
  panel.primary_selection = 1;

  GroupResult result = GroupSelectedNodes(panel, nodes, props, nullptr);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.group_id, -1);
  EXPECT_FALSE(result.error.empty());
}

TEST(NodeGrouping, GroupSelectedNodes_RootNodeIgnored)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Select root node (should be ignored)
  panel.selected_ids.insert(0);
  panel.primary_selection = 0;

  GroupResult result = GroupSelectedNodes(panel, nodes, props, nullptr);

  EXPECT_FALSE(result.success);
}

// ============================================================================
// UngroupContainer Tests
// ============================================================================

TEST(NodeGrouping, UngroupContainer_MoveChildrenToParent)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  size_t original_world_children_count = nodes[0].children.size();

  // Ungroup Folder (4), which contains NestedBrush (5)
  bool success = UngroupContainer(4, nodes, props, panel, nullptr);

  EXPECT_TRUE(success);

  // Folder should be deleted
  EXPECT_TRUE(nodes[4].deleted);

  // NestedBrush should now be a child of World
  auto& world_children = nodes[0].children;
  EXPECT_TRUE(std::find(world_children.begin(), world_children.end(), 5) != world_children.end());

  // Folder should no longer be a child of World
  EXPECT_TRUE(std::find(world_children.begin(), world_children.end(), 4) == world_children.end());

  // Selection should be the ungrouped children
  EXPECT_EQ(panel.selected_ids.size(), 1u);
  EXPECT_TRUE(panel.selected_ids.count(5) > 0);
}

TEST(NodeGrouping, UngroupContainer_NotAFolder)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Try to ungroup Brush01, which is not a folder
  bool success = UngroupContainer(1, nodes, props, panel, nullptr);

  EXPECT_FALSE(success);
  EXPECT_FALSE(nodes[1].deleted);
}

TEST(NodeGrouping, UngroupContainer_InvalidId)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  bool success = UngroupContainer(99, nodes, props, panel, nullptr);
  EXPECT_FALSE(success);

  success = UngroupContainer(-1, nodes, props, panel, nullptr);
  EXPECT_FALSE(success);
}

TEST(NodeGrouping, UngroupContainer_RootNode)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Try to ungroup root node (0) - should fail
  bool success = UngroupContainer(0, nodes, props, panel, nullptr);

  EXPECT_FALSE(success);
  EXPECT_FALSE(nodes[0].deleted);
}

// ============================================================================
// Integration: Group then Ungroup
// ============================================================================

TEST(NodeGrouping, GroupThenUngroup_RestoresStructure)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTestScene(nodes, props, panel);

  // Save original state
  size_t original_world_children_count = nodes[0].children.size();
  std::vector<int> original_world_children = nodes[0].children;

  // Group Brush01 and Brush02
  panel.selected_ids.insert(1);
  panel.selected_ids.insert(2);
  panel.primary_selection = 1;

  GroupResult group_result = GroupSelectedNodes(panel, nodes, props, nullptr);
  ASSERT_TRUE(group_result.success);
  int group_id = group_result.group_id;

  // Now ungroup
  bool ungroup_success = UngroupContainer(group_id, nodes, props, panel, nullptr);
  ASSERT_TRUE(ungroup_success);

  // Group should be deleted
  EXPECT_TRUE(nodes[group_id].deleted);

  // Brush01 and Brush02 should be back as children of World
  auto& world_children = nodes[0].children;
  EXPECT_TRUE(std::find(world_children.begin(), world_children.end(), 1) != world_children.end());
  EXPECT_TRUE(std::find(world_children.begin(), world_children.end(), 2) != world_children.end());

  // Selection should be the ungrouped children
  EXPECT_EQ(panel.selected_ids.size(), 2u);
  EXPECT_TRUE(panel.selected_ids.count(1) > 0);
  EXPECT_TRUE(panel.selected_ids.count(2) > 0);
}
