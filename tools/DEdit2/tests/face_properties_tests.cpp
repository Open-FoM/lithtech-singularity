#include "ui_face_properties.h"

#include "editor_state.h"
#include "geometry/subobject_selection.h"
#include "undo_stack.h"

#include <gtest/gtest.h>

#include <array>

namespace {

/// Build a NodeProperties holding a single axis-aligned quad (two triangles) in the XY plane.
/// Vertices (CCW): (0,0,0) (4,0,0) (4,2,0) (0,2,0). Triangles: (0,1,2) and (0,2,3).
/// brush_face_textures is sized to 2 with distinct, non-identity data so changes are detectable.
NodeProperties MakeQuadBrush() {
  NodeProperties props;
  props.type = "Brush";
  props.brush_vertices = {
      0.0f, 0.0f, 0.0f, // 0
      4.0f, 0.0f, 0.0f, // 1
      4.0f, 2.0f, 0.0f, // 2
      0.0f, 2.0f, 0.0f, // 3
  };
  props.brush_indices = {0, 1, 2, 0, 2, 3};

  BrushFaceTextureData face0;
  face0.texture_name = "tex_a";
  face0.mapping = texture_ops::TextureMapping(0.1f, 0.2f, 2.0f, 3.0f, 10.0f);
  face0.surface_flags = static_cast<uint32_t>(texture_ops::SurfaceFlags::Solid);
  face0.alpha_ref = 5;

  BrushFaceTextureData face1;
  face1.texture_name = "tex_b";
  face1.mapping = texture_ops::TextureMapping(0.5f, 0.6f, 1.5f, 1.5f, 45.0f);
  face1.surface_flags = static_cast<uint32_t>(texture_ops::SurfaceFlags::Sky);
  face1.alpha_ref = 9;

  props.brush_face_textures = {face0, face1};
  return props;
}

/// Build a single-brush scene at node index 1 (node 0 is a World folder), mirroring transform_tests.
void BuildBrushScene(std::vector<TreeNode> &nodes, std::vector<NodeProperties> &props) {
  nodes.clear();
  props.clear();

  TreeNode world;
  world.name = "World";
  world.is_folder = true;
  world.children = {1};
  nodes.push_back(world);
  NodeProperties world_props;
  world_props.type = "World";
  props.push_back(world_props);

  TreeNode brush;
  brush.name = "Brush01";
  nodes.push_back(brush);
  props.push_back(MakeQuadBrush());
}

/// Build a selection containing a single face on the given node/triangle.
SubObjectSelection SelectFace(int node_id, uint32_t triangle_index) {
  SubObjectSelection sel;
  sel.selected_faces.insert(FaceRef{node_id, triangle_index});
  return sel;
}

} // namespace

// ---- ApplyFacePropertiesAction ----

TEST(FacePropertiesTest, ApplyOffsetUpdatesOnlySelectedFace) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);

  FacePropertiesAction action;
  action.offset_changed = true;
  action.new_offset_u = 7.0f;
  action.new_offset_v = 8.0f;

  EXPECT_TRUE(ApplyFacePropertiesAction(action, sel, props));

  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.offset_u, 7.0f);
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.offset_v, 8.0f);
  // Non-selected face is unchanged.
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[1].mapping.offset_u, 0.5f);
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[1].mapping.offset_v, 0.6f);
}

TEST(FacePropertiesTest, ApplyScaleAndRotation) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 1);

  FacePropertiesAction action;
  action.scale_changed = true;
  action.new_scale_u = 4.0f;
  action.new_scale_v = 5.0f;
  action.rotation_changed = true;
  action.new_rotation = 90.0f;

  EXPECT_TRUE(ApplyFacePropertiesAction(action, sel, props));
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[1].mapping.scale_u, 4.0f);
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[1].mapping.scale_v, 5.0f);
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[1].mapping.rotation, 90.0f);
  // Face 0 unchanged.
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.scale_u, 2.0f);
}

TEST(FacePropertiesTest, FlipUNegatesScaleU) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);
  const float before = props[0].brush_face_textures[0].mapping.scale_u;

  FacePropertiesAction action;
  action.flip_u_requested = true;
  EXPECT_TRUE(ApplyFacePropertiesAction(action, sel, props));
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.scale_u, -before);
}

TEST(FacePropertiesTest, FlipVNegatesScaleV) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);
  const float before = props[0].brush_face_textures[0].mapping.scale_v;

  FacePropertiesAction action;
  action.flip_v_requested = true;
  EXPECT_TRUE(ApplyFacePropertiesAction(action, sel, props));
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.scale_v, -before);
}

TEST(FacePropertiesTest, ResetRestoresIdentityMapping) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);

  FacePropertiesAction action;
  action.reset_requested = true;
  EXPECT_TRUE(ApplyFacePropertiesAction(action, sel, props));
  EXPECT_TRUE(props[0].brush_face_textures[0].mapping.IsIdentity());
}

TEST(FacePropertiesTest, FlagsAndAlphaChange) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);

  FacePropertiesAction action;
  action.flags_changed = true;
  action.new_flags = static_cast<uint32_t>(texture_ops::SurfaceFlags::Mirror);
  action.alpha_changed = true;
  action.new_alpha = 200;

  EXPECT_TRUE(ApplyFacePropertiesAction(action, sel, props));
  EXPECT_EQ(props[0].brush_face_textures[0].surface_flags, static_cast<uint32_t>(texture_ops::SurfaceFlags::Mirror));
  EXPECT_EQ(props[0].brush_face_textures[0].alpha_ref, 200);
}

TEST(FacePropertiesTest, EmptyActionReturnsFalse) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);
  FacePropertiesAction action; // nothing set
  EXPECT_FALSE(ApplyFacePropertiesAction(action, sel, props));
}

TEST(FacePropertiesTest, NoSelectionReturnsFalse) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel; // no faces
  FacePropertiesAction action;
  action.offset_changed = true;
  action.new_offset_u = 1.0f;
  EXPECT_FALSE(ApplyFacePropertiesAction(action, sel, props));
}

// ---- PickTextureFromFace ----

TEST(FacePropertiesTest, PickTextureFromValidFace) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  PickedTextureState picked;
  EXPECT_TRUE(PickTextureFromFace(FaceRef{0, 1}, props, picked));
  EXPECT_TRUE(picked.has_pick);
  EXPECT_EQ(picked.texture_name, "tex_b");
  EXPECT_FLOAT_EQ(picked.mapping.offset_u, 0.5f);
  EXPECT_FLOAT_EQ(picked.mapping.rotation, 45.0f);
  EXPECT_EQ(picked.surface_flags, static_cast<uint32_t>(texture_ops::SurfaceFlags::Sky));
  EXPECT_EQ(picked.alpha_ref, 9);
}

TEST(FacePropertiesTest, PickTextureFromInvalidNodeFails) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  PickedTextureState picked;
  EXPECT_FALSE(PickTextureFromFace(FaceRef{5, 0}, props, picked));
  EXPECT_FALSE(picked.has_pick);
}

TEST(FacePropertiesTest, PickTextureFromInvalidTriangleFails) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  PickedTextureState picked;
  EXPECT_FALSE(PickTextureFromFace(FaceRef{0, 99}, props, picked));
  EXPECT_FALSE(picked.has_pick);
}

// ---- ApplyPickedTextureToFaces ----

TEST(FacePropertiesTest, ApplyPickedTextureNameAndMappingAndFlags) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);

  PickedTextureState picked;
  picked.has_pick = true;
  picked.texture_name = "pasted";
  picked.mapping = texture_ops::TextureMapping(9.0f, 8.0f, 7.0f, 6.0f, 5.0f);
  picked.surface_flags = static_cast<uint32_t>(texture_ops::SurfaceFlags::Portal);
  picked.alpha_ref = 123;

  const size_t count = ApplyPickedTextureToFaces(picked, sel, props, true, true);
  EXPECT_EQ(count, 1u);
  EXPECT_EQ(props[0].brush_face_textures[0].texture_name, "pasted");
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.offset_u, 9.0f);
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.scale_v, 6.0f);
  EXPECT_EQ(props[0].brush_face_textures[0].surface_flags, static_cast<uint32_t>(texture_ops::SurfaceFlags::Portal));
  EXPECT_EQ(props[0].brush_face_textures[0].alpha_ref, 123);
}

TEST(FacePropertiesTest, ApplyPickedTextureNameOnly) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);
  const texture_ops::TextureMapping original = props[0].brush_face_textures[0].mapping;

  PickedTextureState picked;
  picked.has_pick = true;
  picked.texture_name = "name_only";
  picked.mapping = texture_ops::TextureMapping(9.0f, 9.0f, 9.0f, 9.0f, 9.0f);

  const size_t count = ApplyPickedTextureToFaces(picked, sel, props, false, false);
  EXPECT_EQ(count, 1u);
  EXPECT_EQ(props[0].brush_face_textures[0].texture_name, "name_only");
  // Mapping untouched because apply_mapping=false.
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.offset_u, original.offset_u);
  EXPECT_FLOAT_EQ(props[0].brush_face_textures[0].mapping.scale_u, original.scale_u);
}

TEST(FacePropertiesTest, ApplyPickedReturnsZeroWhenNoPick) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);
  PickedTextureState picked; // has_pick=false
  EXPECT_EQ(ApplyPickedTextureToFaces(picked, sel, props, true, true), 0u);
}

TEST(FacePropertiesTest, ApplyPickedReturnsZeroWhenNoSelection) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel; // empty
  PickedTextureState picked;
  picked.has_pick = true;
  picked.texture_name = "x";
  EXPECT_EQ(ApplyPickedTextureToFaces(picked, sel, props, true, true), 0u);
}

// ---- ComputeFaceFitMapping ----

TEST(FacePropertiesTest, ComputeFitMappingMatchesBoundingBox) {
  // Right triangle with in-plane bbox 4 (u) x 2 (v) in the XY plane.
  std::array<float, 3> v0 = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> v1 = {4.0f, 0.0f, 0.0f};
  std::array<float, 3> v2 = {0.0f, 2.0f, 0.0f};

  texture_ops::TextureMapping m = ComputeFaceFitMapping(v0, v1, v2);
  EXPECT_NEAR(m.scale_u, 1.0f / 4.0f, 1e-4f);
  EXPECT_NEAR(m.scale_v, 1.0f / 2.0f, 1e-4f);
  EXPECT_NEAR(m.rotation, 0.0f, 1e-4f);
  // u axis along v0->v1, min_u == 0 so offset_u == 0; v range starts at 0 too.
  EXPECT_NEAR(m.offset_u, 0.0f, 1e-4f);
  EXPECT_NEAR(m.offset_v, 0.0f, 1e-4f);
}

TEST(FacePropertiesTest, ComputeFitMappingDegenerateIdenticalVerts) {
  std::array<float, 3> v = {1.0f, 2.0f, 3.0f};
  texture_ops::TextureMapping m = ComputeFaceFitMapping(v, v, v);
  EXPECT_TRUE(m.IsIdentity());
}

TEST(FacePropertiesTest, ComputeFitMappingDegenerateColinear) {
  std::array<float, 3> v0 = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> v1 = {1.0f, 0.0f, 0.0f};
  std::array<float, 3> v2 = {2.0f, 0.0f, 0.0f};
  texture_ops::TextureMapping m = ComputeFaceFitMapping(v0, v1, v2);
  EXPECT_TRUE(m.IsIdentity());
}

// ---- ComputeFaceFitMappingForFace ----

TEST(FacePropertiesTest, ComputeFitForValidTriangle) {
  NodeProperties props = MakeQuadBrush();
  auto m = ComputeFaceFitMappingForFace(props, 0);
  ASSERT_TRUE(m.has_value());
  // Triangle 0 = (0,0,0),(4,0,0),(4,2,0): u along (4,0,0) so bbox u=4, v=2.
  EXPECT_NEAR(m->scale_u, 1.0f / 4.0f, 1e-4f);
  EXPECT_NEAR(m->scale_v, 1.0f / 2.0f, 1e-4f);
}

TEST(FacePropertiesTest, ComputeFitForInvalidTriangleReturnsNullopt) {
  NodeProperties props = MakeQuadBrush();
  EXPECT_FALSE(ComputeFaceFitMappingForFace(props, 99).has_value());
}

// ---- ApplyFacePropertiesAction Fit path ----

TEST(FacePropertiesTest, FitRequestedAppliesComputedMapping) {
  std::vector<NodeProperties> props = {MakeQuadBrush()};
  SubObjectSelection sel = SelectFace(0, 0);

  FacePropertiesAction action;
  action.fit_requested = true;
  EXPECT_TRUE(ApplyFacePropertiesAction(action, sel, props));
  EXPECT_NEAR(props[0].brush_face_textures[0].mapping.scale_u, 1.0f / 4.0f, 1e-4f);
  EXPECT_NEAR(props[0].brush_face_textures[0].mapping.scale_v, 1.0f / 2.0f, 1e-4f);
}

// ---- Capture / Finalize ----

TEST(FacePropertiesTest, CaptureAndFinalizeDropsUnchanged) {
  std::vector<NodeProperties> props;
  std::vector<TreeNode> nodes;
  BuildBrushScene(nodes, props);

  SubObjectSelection sel;
  sel.selected_faces.insert(FaceRef{1, 0});
  sel.selected_faces.insert(FaceRef{1, 1});

  std::vector<FaceTextureChange> changes = CaptureFaceTextureStates(sel, props);
  ASSERT_EQ(changes.size(), 2u);

  // Mutate only triangle 0.
  props[1].brush_face_textures[0].alpha_ref = 222;

  FinalizeFaceTextureStates(changes, props);
  ASSERT_EQ(changes.size(), 1u);
  EXPECT_EQ(changes[0].triangle_index, 0u);
  EXPECT_EQ(changes[0].after.alpha_ref, 222);
  EXPECT_EQ(changes[0].before.alpha_ref, 5);
}

// ---- UndoStack FaceTexture round-trip ----

TEST(FacePropertiesTest, UndoStackFaceTextureRoundTrip) {
  std::vector<TreeNode> project_nodes, scene_nodes;
  std::vector<NodeProperties> project_props, scene_props;
  BuildBrushScene(scene_nodes, scene_props);

  const std::string before_name = scene_props[1].brush_face_textures[0].texture_name;
  const uint8_t before_alpha = scene_props[1].brush_face_textures[0].alpha_ref;

  SubObjectSelection sel;
  sel.selected_faces.insert(FaceRef{1, 0});

  std::vector<FaceTextureChange> changes = CaptureFaceTextureStates(sel, scene_props);
  ASSERT_EQ(changes.size(), 1u);

  // Mutate the face.
  scene_props[1].brush_face_textures[0].texture_name = "changed";
  scene_props[1].brush_face_textures[0].alpha_ref = 99;

  FinalizeFaceTextureStates(changes, scene_props);
  ASSERT_EQ(changes.size(), 1u);

  UndoStack stack;
  stack.PushFaceTexture(UndoTarget::Scene, changes);
  EXPECT_TRUE(stack.CanUndo());

  // Undo -> reverts to before.
  stack.Undo(project_nodes, scene_nodes, project_props, scene_props);
  EXPECT_EQ(scene_props[1].brush_face_textures[0].texture_name, before_name);
  EXPECT_EQ(scene_props[1].brush_face_textures[0].alpha_ref, before_alpha);

  // Redo -> back to after.
  stack.Redo(project_nodes, scene_nodes, project_props, scene_props);
  EXPECT_EQ(scene_props[1].brush_face_textures[0].texture_name, "changed");
  EXPECT_EQ(scene_props[1].brush_face_textures[0].alpha_ref, 99);
}
