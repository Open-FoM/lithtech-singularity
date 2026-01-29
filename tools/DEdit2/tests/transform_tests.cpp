#include "undo_stack.h"
#include "editor_state.h"
#include "ui_scene.h"

#include <gtest/gtest.h>

#include <cmath>

namespace
{

// Helper to build a simple scene for transform testing
void BuildTransformTestScene(
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    ScenePanelState& panel)
{
  nodes.clear();
  props.clear();
  panel = ScenePanelState{};

  // 0: World (root folder)
  TreeNode world;
  world.name = "World";
  world.is_folder = true;
  world.children = {1, 2, 3};
  nodes.push_back(world);
  NodeProperties world_props;
  world_props.type = "World";
  props.push_back(world_props);

  // 1: Brush01 at position (100, 0, 0)
  TreeNode brush1;
  brush1.name = "Brush01";
  nodes.push_back(brush1);
  NodeProperties brush1_props;
  brush1_props.type = "Brush";
  brush1_props.position[0] = 100.0f;
  brush1_props.position[1] = 0.0f;
  brush1_props.position[2] = 0.0f;
  brush1_props.rotation[0] = 0.0f;
  brush1_props.rotation[1] = 0.0f;
  brush1_props.rotation[2] = 0.0f;
  brush1_props.scale[0] = 1.0f;
  brush1_props.scale[1] = 1.0f;
  brush1_props.scale[2] = 1.0f;
  props.push_back(brush1_props);

  // 2: Brush02 at position (200, 0, 0)
  TreeNode brush2;
  brush2.name = "Brush02";
  nodes.push_back(brush2);
  NodeProperties brush2_props;
  brush2_props.type = "Brush";
  brush2_props.position[0] = 200.0f;
  brush2_props.position[1] = 0.0f;
  brush2_props.position[2] = 0.0f;
  brush2_props.rotation[0] = 0.0f;
  brush2_props.rotation[1] = 0.0f;
  brush2_props.rotation[2] = 0.0f;
  brush2_props.scale[0] = 1.0f;
  brush2_props.scale[1] = 1.0f;
  brush2_props.scale[2] = 1.0f;
  props.push_back(brush2_props);

  // 3: Light01 at position (0, 100, 0)
  TreeNode light1;
  light1.name = "Light01";
  nodes.push_back(light1);
  NodeProperties light1_props;
  light1_props.type = "Light";
  light1_props.position[0] = 0.0f;
  light1_props.position[1] = 100.0f;
  light1_props.position[2] = 0.0f;
  props.push_back(light1_props);
}

} // namespace

// Test TransformState initialization
TEST(TransformTest, TransformStateDefaultValues)
{
  TransformState state;
  EXPECT_FLOAT_EQ(state.position[0], 0.0f);
  EXPECT_FLOAT_EQ(state.position[1], 0.0f);
  EXPECT_FLOAT_EQ(state.position[2], 0.0f);
  EXPECT_FLOAT_EQ(state.rotation[0], 0.0f);
  EXPECT_FLOAT_EQ(state.rotation[1], 0.0f);
  EXPECT_FLOAT_EQ(state.rotation[2], 0.0f);
  EXPECT_FLOAT_EQ(state.scale[0], 1.0f);
  EXPECT_FLOAT_EQ(state.scale[1], 1.0f);
  EXPECT_FLOAT_EQ(state.scale[2], 1.0f);
}

// Test TransformChange initialization
TEST(TransformTest, TransformChangeDefaultValues)
{
  TransformChange change;
  EXPECT_EQ(change.node_id, -1);
  EXPECT_TRUE(change.before_vertices.empty());
  EXPECT_TRUE(change.after_vertices.empty());
}

// Test PushTransform and Undo
TEST(TransformTest, PushTransformAndUndo)
{
  std::vector<TreeNode> project_nodes, scene_nodes;
  std::vector<NodeProperties> project_props, scene_props;
  ScenePanelState panel;
  BuildTransformTestScene(scene_nodes, scene_props, panel);

  UndoStack undo_stack;

  // Save original position
  const float original_x = scene_props[1].position[0];
  const float original_y = scene_props[1].position[1];
  const float original_z = scene_props[1].position[2];

  // Create a transform change
  std::vector<TransformChange> changes;
  TransformChange change;
  change.node_id = 1;
  change.before.position[0] = original_x;
  change.before.position[1] = original_y;
  change.before.position[2] = original_z;

  // Modify the position
  scene_props[1].position[0] = 150.0f;
  scene_props[1].position[1] = 50.0f;
  scene_props[1].position[2] = 25.0f;

  change.after.position[0] = scene_props[1].position[0];
  change.after.position[1] = scene_props[1].position[1];
  change.after.position[2] = scene_props[1].position[2];
  changes.push_back(change);

  // Push the transform
  undo_stack.PushTransform(UndoTarget::Scene, std::move(changes));

  EXPECT_TRUE(undo_stack.CanUndo());
  EXPECT_FALSE(undo_stack.CanRedo());

  // Verify modified position
  EXPECT_FLOAT_EQ(scene_props[1].position[0], 150.0f);
  EXPECT_FLOAT_EQ(scene_props[1].position[1], 50.0f);
  EXPECT_FLOAT_EQ(scene_props[1].position[2], 25.0f);

  // Undo
  undo_stack.Undo(project_nodes, scene_nodes, project_props, scene_props);

  // Verify position is restored
  EXPECT_FLOAT_EQ(scene_props[1].position[0], original_x);
  EXPECT_FLOAT_EQ(scene_props[1].position[1], original_y);
  EXPECT_FLOAT_EQ(scene_props[1].position[2], original_z);

  EXPECT_FALSE(undo_stack.CanUndo());
  EXPECT_TRUE(undo_stack.CanRedo());
}

// Test PushTransform and Redo
TEST(TransformTest, PushTransformAndRedo)
{
  std::vector<TreeNode> project_nodes, scene_nodes;
  std::vector<NodeProperties> project_props, scene_props;
  ScenePanelState panel;
  BuildTransformTestScene(scene_nodes, scene_props, panel);

  UndoStack undo_stack;

  // Create a transform change
  std::vector<TransformChange> changes;
  TransformChange change;
  change.node_id = 1;
  change.before.position[0] = scene_props[1].position[0];
  change.before.position[1] = scene_props[1].position[1];
  change.before.position[2] = scene_props[1].position[2];

  // Modify the position
  scene_props[1].position[0] = 150.0f;
  change.after.position[0] = 150.0f;
  change.after.position[1] = scene_props[1].position[1];
  change.after.position[2] = scene_props[1].position[2];
  changes.push_back(change);

  undo_stack.PushTransform(UndoTarget::Scene, std::move(changes));

  // Undo
  undo_stack.Undo(project_nodes, scene_nodes, project_props, scene_props);
  EXPECT_FLOAT_EQ(scene_props[1].position[0], 100.0f);

  // Redo
  undo_stack.Redo(project_nodes, scene_nodes, project_props, scene_props);
  EXPECT_FLOAT_EQ(scene_props[1].position[0], 150.0f);
}

// Test multi-node transform undo
TEST(TransformTest, MultiNodeTransformUndo)
{
  std::vector<TreeNode> project_nodes, scene_nodes;
  std::vector<NodeProperties> project_props, scene_props;
  ScenePanelState panel;
  BuildTransformTestScene(scene_nodes, scene_props, panel);

  UndoStack undo_stack;

  // Create transform changes for two nodes
  std::vector<TransformChange> changes;

  // Node 1
  TransformChange change1;
  change1.node_id = 1;
  change1.before.position[0] = scene_props[1].position[0];
  scene_props[1].position[0] = 150.0f;
  change1.after.position[0] = 150.0f;
  changes.push_back(change1);

  // Node 2
  TransformChange change2;
  change2.node_id = 2;
  change2.before.position[0] = scene_props[2].position[0];
  scene_props[2].position[0] = 250.0f;
  change2.after.position[0] = 250.0f;
  changes.push_back(change2);

  undo_stack.PushTransform(UndoTarget::Scene, std::move(changes));

  // Verify modified positions
  EXPECT_FLOAT_EQ(scene_props[1].position[0], 150.0f);
  EXPECT_FLOAT_EQ(scene_props[2].position[0], 250.0f);

  // Undo should restore both
  undo_stack.Undo(project_nodes, scene_nodes, project_props, scene_props);
  EXPECT_FLOAT_EQ(scene_props[1].position[0], 100.0f);
  EXPECT_FLOAT_EQ(scene_props[2].position[0], 200.0f);
}

// Test selection center computation
TEST(TransformTest, SelectionCenterComputation)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTransformTestScene(nodes, props, panel);

  // Select nodes 1 and 2 (at x=100 and x=200)
  SelectNode(panel, 1);
  ModifySelection(panel, 2, SelectionMode::Add);

  std::array<float, 3> center = ComputeSelectionCenter(panel, props);

  // Center should be at (150, 0, 0)
  EXPECT_FLOAT_EQ(center[0], 150.0f);
  EXPECT_FLOAT_EQ(center[1], 0.0f);
  EXPECT_FLOAT_EQ(center[2], 0.0f);
}

// Test selection center with single selection
TEST(TransformTest, SelectionCenterSingleNode)
{
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;
  ScenePanelState panel;
  BuildTransformTestScene(nodes, props, panel);

  // Select only node 1
  SelectNode(panel, 1);

  std::array<float, 3> center = ComputeSelectionCenter(panel, props);

  // Center should be at the node's position
  EXPECT_FLOAT_EQ(center[0], 100.0f);
  EXPECT_FLOAT_EQ(center[1], 0.0f);
  EXPECT_FLOAT_EQ(center[2], 0.0f);
}

// Test brush vertex transform undo
TEST(TransformTest, BrushVertexTransformUndo)
{
  std::vector<TreeNode> project_nodes, scene_nodes;
  std::vector<NodeProperties> project_props, scene_props;
  ScenePanelState panel;
  BuildTransformTestScene(scene_nodes, scene_props, panel);

  // Add brush vertices to node 1
  scene_props[1].brush_vertices = {0.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 10.0f, 10.0f, 0.0f};

  UndoStack undo_stack;

  std::vector<TransformChange> changes;
  TransformChange change;
  change.node_id = 1;
  change.before_vertices = scene_props[1].brush_vertices;

  // Translate vertices
  for (size_t i = 0; i < scene_props[1].brush_vertices.size(); i += 3)
  {
    scene_props[1].brush_vertices[i] += 50.0f;
  }
  change.after_vertices = scene_props[1].brush_vertices;
  changes.push_back(change);

  undo_stack.PushTransform(UndoTarget::Scene, std::move(changes));

  // Verify translated
  EXPECT_FLOAT_EQ(scene_props[1].brush_vertices[0], 50.0f);
  EXPECT_FLOAT_EQ(scene_props[1].brush_vertices[3], 60.0f);

  // Undo
  undo_stack.Undo(project_nodes, scene_nodes, project_props, scene_props);

  // Verify restored
  EXPECT_FLOAT_EQ(scene_props[1].brush_vertices[0], 0.0f);
  EXPECT_FLOAT_EQ(scene_props[1].brush_vertices[3], 10.0f);
}
