#include "brush/csg_dialogs/csg_dialog_helpers.h"

#include <gtest/gtest.h>

// =============================================================================
// ExtractBrushGeometry Tests
// =============================================================================

TEST(CSGDialogHelpers, ExtractBrushGeometry_ValidBrush) {
  NodeProperties props{};
  props.type = "Brush";
  // Simple triangle
  props.brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props.brush_indices = {0, 1, 2};

  std::vector<float> out_verts;
  std::vector<uint32_t> out_indices;

  EXPECT_TRUE(ExtractBrushGeometry(props, out_verts, out_indices));
  EXPECT_EQ(out_verts.size(), 9u);
  EXPECT_EQ(out_indices.size(), 3u);
}

TEST(CSGDialogHelpers, ExtractBrushGeometry_EmptyVertices) {
  NodeProperties props{};
  props.type = "Brush";
  props.brush_vertices = {};
  props.brush_indices = {0, 1, 2};

  std::vector<float> out_verts;
  std::vector<uint32_t> out_indices;

  EXPECT_FALSE(ExtractBrushGeometry(props, out_verts, out_indices));
}

TEST(CSGDialogHelpers, ExtractBrushGeometry_EmptyIndices) {
  NodeProperties props{};
  props.type = "Brush";
  props.brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props.brush_indices = {};

  std::vector<float> out_verts;
  std::vector<uint32_t> out_indices;

  EXPECT_FALSE(ExtractBrushGeometry(props, out_verts, out_indices));
}

TEST(CSGDialogHelpers, ExtractBrushGeometry_PartialVertices) {
  // Note: ExtractBrushGeometry doesn't validate vertex count, just non-empty
  NodeProperties props{};
  props.type = "Brush";
  // Only 2 floats (less than one complete XYZ triplet)
  props.brush_vertices = {0.0f, 0.0f};
  props.brush_indices = {0};

  std::vector<float> out_verts;
  std::vector<uint32_t> out_indices;

  // Function returns true because vectors are non-empty (validation happens later)
  EXPECT_TRUE(ExtractBrushGeometry(props, out_verts, out_indices));
  EXPECT_EQ(out_verts.size(), 2u);
  EXPECT_EQ(out_indices.size(), 1u);
}

TEST(CSGDialogHelpers, ExtractBrushGeometry_NonBrushType) {
  NodeProperties props{};
  props.type = "Object"; // Not a brush
  props.brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props.brush_indices = {0, 1, 2};

  std::vector<float> out_verts;
  std::vector<uint32_t> out_indices;

  EXPECT_FALSE(ExtractBrushGeometry(props, out_verts, out_indices));
}

// =============================================================================
// IsValidCSGBrush Tests
// =============================================================================

TEST(CSGDialogHelpers, IsValidCSGBrush_ValidBrush) {
  TreeNode node{};
  node.name = "TestBrush";
  node.is_folder = false;
  node.deleted = false;

  NodeProperties props{};
  props.type = "Brush";
  props.brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props.brush_indices = {0, 1, 2};

  EXPECT_TRUE(IsValidCSGBrush(node, props));
}

TEST(CSGDialogHelpers, IsValidCSGBrush_DeletedNode) {
  TreeNode node{};
  node.name = "TestBrush";
  node.is_folder = false;
  node.deleted = true;

  NodeProperties props{};
  props.type = "Brush";
  props.brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props.brush_indices = {0, 1, 2};

  EXPECT_FALSE(IsValidCSGBrush(node, props));
}

TEST(CSGDialogHelpers, IsValidCSGBrush_FolderNode) {
  TreeNode node{};
  node.name = "TestFolder";
  node.is_folder = true;
  node.deleted = false;

  NodeProperties props{};
  props.type = "Brush";
  props.brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props.brush_indices = {0, 1, 2};

  EXPECT_FALSE(IsValidCSGBrush(node, props));
}

TEST(CSGDialogHelpers, IsValidCSGBrush_NonBrushType) {
  TreeNode node{};
  node.name = "TestObject";
  node.is_folder = false;
  node.deleted = false;

  NodeProperties props{};
  props.type = "Object";
  props.brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props.brush_indices = {0, 1, 2};

  EXPECT_FALSE(IsValidCSGBrush(node, props));
}

TEST(CSGDialogHelpers, IsValidCSGBrush_NoGeometry) {
  TreeNode node{};
  node.name = "TestBrush";
  node.is_folder = false;
  node.deleted = false;

  NodeProperties props{};
  props.type = "Brush";
  props.brush_vertices = {};
  props.brush_indices = {};

  EXPECT_FALSE(IsValidCSGBrush(node, props));
}

// =============================================================================
// GetSelectedCSGBrushIds Tests
// =============================================================================

TEST(CSGDialogHelpers, GetSelectedCSGBrushIds_NoSelection) {
  ScenePanelState scene_panel{};
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;

  auto result = GetSelectedCSGBrushIds(scene_panel, nodes, props);
  EXPECT_TRUE(result.empty());
}

TEST(CSGDialogHelpers, GetSelectedCSGBrushIds_SingleValidBrush) {
  ScenePanelState scene_panel{};
  scene_panel.selected_ids = {0};

  std::vector<TreeNode> nodes(1);
  nodes[0].name = "Brush1";
  nodes[0].is_folder = false;
  nodes[0].deleted = false;

  std::vector<NodeProperties> props(1);
  props[0].type = "Brush";
  props[0].brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props[0].brush_indices = {0, 1, 2};

  auto result = GetSelectedCSGBrushIds(scene_panel, nodes, props);
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], 0);
}

TEST(CSGDialogHelpers, GetSelectedCSGBrushIds_FiltersInvalid) {
  ScenePanelState scene_panel{};
  scene_panel.selected_ids = {0, 1, 2};

  std::vector<TreeNode> nodes(3);
  nodes[0].name = "ValidBrush";
  nodes[0].is_folder = false;
  nodes[0].deleted = false;
  nodes[1].name = "DeletedBrush";
  nodes[1].is_folder = false;
  nodes[1].deleted = true;
  nodes[2].name = "Folder";
  nodes[2].is_folder = true;
  nodes[2].deleted = false;

  std::vector<NodeProperties> props(3);
  for (int i = 0; i < 3; ++i) {
    props[i].type = "Brush";
    props[i].brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
    props[i].brush_indices = {0, 1, 2};
  }

  auto result = GetSelectedCSGBrushIds(scene_panel, nodes, props);
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], 0);
}

TEST(CSGDialogHelpers, GetSelectedCSGBrushIds_OutOfRangeId) {
  ScenePanelState scene_panel{};
  scene_panel.selected_ids = {0, 100}; // 100 is out of range

  std::vector<TreeNode> nodes(1);
  nodes[0].name = "Brush1";
  nodes[0].is_folder = false;
  nodes[0].deleted = false;

  std::vector<NodeProperties> props(1);
  props[0].type = "Brush";
  props[0].brush_vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  props[0].brush_indices = {0, 1, 2};

  auto result = GetSelectedCSGBrushIds(scene_panel, nodes, props);
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], 0);
}

// =============================================================================
// CreateBrushFromCSGResult Tests
// =============================================================================

TEST(CSGDialogHelpers, CreateBrushFromCSGResult_Valid) {
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  std::vector<uint32_t> indices = {0, 1, 2};

  int id = CreateBrushFromCSGResult(nodes, props, vertices, indices, "NewBrush");

  EXPECT_GE(id, 0);
  // Note: CreateBrushFromCSGResult creates a root "World" node first if empty
  EXPECT_EQ(nodes.size(), 2u); // Root + NewBrush
  EXPECT_EQ(props.size(), 2u);
  EXPECT_EQ(nodes[id].name, "NewBrush");
  EXPECT_EQ(props[id].type, "Brush");
  EXPECT_EQ(props[id].brush_vertices, vertices);
  EXPECT_EQ(props[id].brush_indices, indices);
}

TEST(CSGDialogHelpers, CreateBrushFromCSGResult_EmptyVertices) {
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;

  std::vector<float> vertices;
  std::vector<uint32_t> indices = {0, 1, 2};

  int id = CreateBrushFromCSGResult(nodes, props, vertices, indices, "EmptyBrush");

  EXPECT_EQ(id, -1);
  EXPECT_TRUE(nodes.empty());
  EXPECT_TRUE(props.empty());
}

TEST(CSGDialogHelpers, CreateBrushFromCSGResult_EmptyIndices) {
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  std::vector<uint32_t> indices;

  int id = CreateBrushFromCSGResult(nodes, props, vertices, indices, "NoIndicesBrush");

  EXPECT_EQ(id, -1);
  EXPECT_TRUE(nodes.empty());
  EXPECT_TRUE(props.empty());
}

TEST(CSGDialogHelpers, CreateBrushFromCSGResult_MultipleCreations) {
  std::vector<TreeNode> nodes;
  std::vector<NodeProperties> props;

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  std::vector<uint32_t> indices = {0, 1, 2};

  int id1 = CreateBrushFromCSGResult(nodes, props, vertices, indices, "Brush1");
  int id2 = CreateBrushFromCSGResult(nodes, props, vertices, indices, "Brush2");

  // Note: CreateBrushFromCSGResult creates a root "World" node first if empty
  EXPECT_EQ(id1, 1); // After root
  EXPECT_EQ(id2, 2);
  EXPECT_EQ(nodes.size(), 3u); // Root + 2 brushes
  EXPECT_EQ(props.size(), 3u);
  EXPECT_EQ(nodes[1].name, "Brush1");
  EXPECT_EQ(nodes[2].name, "Brush2");
}
