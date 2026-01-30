#include "geometry/edit_mode.h"
#include "geometry/geometry_mode_state.h"
#include "geometry/subobject_refs.h"
#include "geometry/subobject_selection.h"

#include <gtest/gtest.h>

namespace {

// =============================================================================
// EditMode Tests
// =============================================================================

TEST(EditMode, DefaultIsObject) {
  GeometryModeState state;
  EXPECT_EQ(state.current_mode, EditMode::Object);
}

TEST(EditMode, IsGeometryModeReturnsFalseForObject) { EXPECT_FALSE(IsGeometryMode(EditMode::Object)); }

TEST(EditMode, IsGeometryModeReturnsTrueForVertex) { EXPECT_TRUE(IsGeometryMode(EditMode::Vertex)); }

TEST(EditMode, IsGeometryModeReturnsTrueForEdge) { EXPECT_TRUE(IsGeometryMode(EditMode::Edge)); }

TEST(EditMode, IsGeometryModeReturnsTrueForFace) { EXPECT_TRUE(IsGeometryMode(EditMode::Face)); }

TEST(EditMode, GetEditModeNameReturnsCorrectStrings) {
  EXPECT_STREQ(GetEditModeName(EditMode::Object), "Object");
  EXPECT_STREQ(GetEditModeName(EditMode::Vertex), "Vertex");
  EXPECT_STREQ(GetEditModeName(EditMode::Edge), "Edge");
  EXPECT_STREQ(GetEditModeName(EditMode::Face), "Face");
}

// =============================================================================
// GeometryModeState Tests
// =============================================================================

TEST(GeometryModeState, SetModeToFace) {
  GeometryModeState state;
  state.SetMode(EditMode::Face);
  EXPECT_EQ(state.current_mode, EditMode::Face);
}

TEST(GeometryModeState, SetModeToVertex) {
  GeometryModeState state;
  state.SetMode(EditMode::Vertex);
  EXPECT_EQ(state.current_mode, EditMode::Vertex);
}

TEST(GeometryModeState, SetModeToEdge) {
  GeometryModeState state;
  state.SetMode(EditMode::Edge);
  EXPECT_EQ(state.current_mode, EditMode::Edge);
}

TEST(GeometryModeState, SetModeToObjectClearsSelection) {
  GeometryModeState state;
  state.SetMode(EditMode::Face);

  // Add some faces to selection
  FaceRef face{1, 0};
  state.selection.AddFace(face);
  EXPECT_TRUE(state.selection.HasFaceSelection());

  // Return to object mode should clear selection
  state.SetMode(EditMode::Object);
  EXPECT_EQ(state.current_mode, EditMode::Object);
  EXPECT_FALSE(state.selection.HasFaceSelection());
}

TEST(GeometryModeState, SetModeSameModeTwiceIsNoOp) {
  GeometryModeState state;
  state.SetMode(EditMode::Face);

  FaceRef face{1, 0};
  state.selection.AddFace(face);

  // Setting to same mode should not clear selection
  state.SetMode(EditMode::Face);
  EXPECT_TRUE(state.selection.HasFaceSelection());
}

TEST(GeometryModeState, ToggleGeometryModeFromObject) {
  GeometryModeState state;
  state.previous_geometry_mode = EditMode::Face;

  state.ToggleGeometryMode();
  EXPECT_EQ(state.current_mode, EditMode::Face);
}

TEST(GeometryModeState, ToggleGeometryModeFromGeometryMode) {
  GeometryModeState state;
  state.SetMode(EditMode::Vertex);

  state.ToggleGeometryMode();
  EXPECT_EQ(state.current_mode, EditMode::Object);
}

TEST(GeometryModeState, ToggleRemembersPreviousMode) {
  GeometryModeState state;

  state.SetMode(EditMode::Vertex);
  state.SetMode(EditMode::Object);
  EXPECT_EQ(state.previous_geometry_mode, EditMode::Vertex);

  state.ToggleGeometryMode();
  EXPECT_EQ(state.current_mode, EditMode::Vertex);
}

TEST(GeometryModeState, IsInGeometryModeReturnsCorrectValue) {
  GeometryModeState state;

  EXPECT_FALSE(state.IsInGeometryMode());

  state.SetMode(EditMode::Face);
  EXPECT_TRUE(state.IsInGeometryMode());

  state.SetMode(EditMode::Object);
  EXPECT_FALSE(state.IsInGeometryMode());
}

TEST(GeometryModeState, ResetClearsAllState) {
  GeometryModeState state;
  state.SetMode(EditMode::Face);
  state.selection.AddFace({1, 0});
  state.constrain_to_normal = true;
  state.preserve_uvs = false;

  state.Reset();

  EXPECT_EQ(state.current_mode, EditMode::Object);
  EXPECT_FALSE(state.selection.HasAnySelection());
  EXPECT_FALSE(state.constrain_to_normal);
  EXPECT_TRUE(state.preserve_uvs);
}

TEST(GeometryModeState, SwitchBetweenGeometryModesClearsOldSelection) {
  GeometryModeState state;

  state.SetMode(EditMode::Face);
  state.selection.AddFace({1, 0});
  EXPECT_TRUE(state.selection.HasFaceSelection());

  // Switch to vertex mode
  state.SetMode(EditMode::Vertex);
  EXPECT_FALSE(state.selection.HasFaceSelection());
  EXPECT_FALSE(state.selection.HasVertexSelection());
}

// =============================================================================
// VertexRef Tests
// =============================================================================

TEST(VertexRef, DefaultIsInvalid) {
  VertexRef ref;
  EXPECT_FALSE(ref.IsValid());
}

TEST(VertexRef, ValidWithPositiveNodeId) {
  VertexRef ref{0, 5};
  EXPECT_TRUE(ref.IsValid());
}

TEST(VertexRef, EqualityComparison) {
  VertexRef a{1, 5};
  VertexRef b{1, 5};
  VertexRef c{1, 6};
  VertexRef d{2, 5};

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
  EXPECT_NE(a, d);
}

// =============================================================================
// EdgeRef Tests
// =============================================================================

TEST(EdgeRef, DefaultIsInvalid) {
  EdgeRef ref;
  EXPECT_FALSE(ref.IsValid());
}

TEST(EdgeRef, CreateNormalizesVertexOrder) {
  EdgeRef ref1 = EdgeRef::Create(1, 5, 3);
  EXPECT_EQ(ref1.vertex_a, 3u);
  EXPECT_EQ(ref1.vertex_b, 5u);

  EdgeRef ref2 = EdgeRef::Create(1, 3, 5);
  EXPECT_EQ(ref2.vertex_a, 3u);
  EXPECT_EQ(ref2.vertex_b, 5u);
}

TEST(EdgeRef, EqualityAfterNormalization) {
  EdgeRef a = EdgeRef::Create(1, 5, 3);
  EdgeRef b = EdgeRef::Create(1, 3, 5);
  EXPECT_EQ(a, b);
}

TEST(EdgeRef, IsInvalidWithSameVertices) {
  EdgeRef ref = EdgeRef::Create(1, 5, 5);
  EXPECT_FALSE(ref.IsValid());
}

// =============================================================================
// FaceRef Tests
// =============================================================================

TEST(FaceRef, DefaultIsInvalid) {
  FaceRef ref;
  EXPECT_FALSE(ref.IsValid());
}

TEST(FaceRef, ValidWithPositiveNodeId) {
  FaceRef ref{0, 5};
  EXPECT_TRUE(ref.IsValid());
}

TEST(FaceRef, EqualityComparison) {
  FaceRef a{1, 5};
  FaceRef b{1, 5};
  FaceRef c{1, 6};

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

// =============================================================================
// SubObjectSelection Tests
// =============================================================================

TEST(SubObjectSelection, InitiallyEmpty) {
  SubObjectSelection sel;
  EXPECT_FALSE(sel.HasVertexSelection());
  EXPECT_FALSE(sel.HasEdgeSelection());
  EXPECT_FALSE(sel.HasFaceSelection());
  EXPECT_FALSE(sel.HasAnySelection());
}

TEST(SubObjectSelection, SelectVertex) {
  SubObjectSelection sel;
  VertexRef ref{1, 5};

  sel.SelectVertex(ref);

  EXPECT_TRUE(sel.HasVertexSelection());
  EXPECT_EQ(sel.VertexCount(), 1u);
  EXPECT_TRUE(sel.IsVertexSelected(ref));
  EXPECT_EQ(sel.primary_vertex, ref);
}

TEST(SubObjectSelection, AddMultipleVertices) {
  SubObjectSelection sel;
  VertexRef ref1{1, 5};
  VertexRef ref2{1, 6};

  sel.AddVertex(ref1);
  sel.AddVertex(ref2);

  EXPECT_EQ(sel.VertexCount(), 2u);
  EXPECT_TRUE(sel.IsVertexSelected(ref1));
  EXPECT_TRUE(sel.IsVertexSelected(ref2));
  EXPECT_EQ(sel.primary_vertex, ref1);  // First added is primary
}

TEST(SubObjectSelection, RemoveVertex) {
  SubObjectSelection sel;
  VertexRef ref1{1, 5};
  VertexRef ref2{1, 6};

  sel.AddVertex(ref1);
  sel.AddVertex(ref2);
  sel.RemoveVertex(ref1);

  EXPECT_EQ(sel.VertexCount(), 1u);
  EXPECT_FALSE(sel.IsVertexSelected(ref1));
  EXPECT_TRUE(sel.IsVertexSelected(ref2));
}

TEST(SubObjectSelection, RemovePrimaryVertexUpdatesPrimary) {
  SubObjectSelection sel;
  VertexRef ref1{1, 5};
  VertexRef ref2{1, 6};

  sel.AddVertex(ref1);
  sel.AddVertex(ref2);
  sel.RemoveVertex(ref1);

  EXPECT_TRUE(sel.primary_vertex.has_value());
  EXPECT_EQ(*sel.primary_vertex, ref2);
}

TEST(SubObjectSelection, ToggleVertex) {
  SubObjectSelection sel;
  VertexRef ref{1, 5};

  sel.ToggleVertex(ref);
  EXPECT_TRUE(sel.IsVertexSelected(ref));

  sel.ToggleVertex(ref);
  EXPECT_FALSE(sel.IsVertexSelected(ref));
}

TEST(SubObjectSelection, SelectFace) {
  SubObjectSelection sel;
  FaceRef ref{1, 5};

  sel.SelectFace(ref);

  EXPECT_TRUE(sel.HasFaceSelection());
  EXPECT_EQ(sel.FaceCount(), 1u);
  EXPECT_TRUE(sel.IsFaceSelected(ref));
}

TEST(SubObjectSelection, SelectEdge) {
  SubObjectSelection sel;
  EdgeRef ref = EdgeRef::Create(1, 3, 5);

  sel.SelectEdge(ref);

  EXPECT_TRUE(sel.HasEdgeSelection());
  EXPECT_EQ(sel.EdgeCount(), 1u);
  EXPECT_TRUE(sel.IsEdgeSelected(ref));
}

TEST(SubObjectSelection, ClearVertices) {
  SubObjectSelection sel;
  sel.AddVertex({1, 5});
  sel.AddVertex({1, 6});
  sel.hovered_vertex = VertexRef{1, 5};

  sel.ClearVertices();

  EXPECT_FALSE(sel.HasVertexSelection());
  EXPECT_FALSE(sel.hovered_vertex.has_value());
  EXPECT_FALSE(sel.primary_vertex.has_value());
}

TEST(SubObjectSelection, ClearAll) {
  SubObjectSelection sel;
  sel.AddVertex({1, 5});
  sel.AddEdge(EdgeRef::Create(1, 3, 5));
  sel.AddFace({1, 0});

  sel.ClearAll();

  EXPECT_FALSE(sel.HasAnySelection());
}

TEST(SubObjectSelection, ClearForNode) {
  SubObjectSelection sel;
  sel.AddVertex({1, 5});
  sel.AddVertex({2, 5});
  sel.AddFace({1, 0});
  sel.AddFace({2, 0});

  sel.ClearForNode(1);

  EXPECT_EQ(sel.VertexCount(), 1u);
  EXPECT_EQ(sel.FaceCount(), 1u);
  EXPECT_FALSE(sel.IsVertexSelected({1, 5}));
  EXPECT_TRUE(sel.IsVertexSelected({2, 5}));
}

TEST(SubObjectSelection, GetNodesWithSelectedVertices) {
  SubObjectSelection sel;
  sel.AddVertex({1, 5});
  sel.AddVertex({1, 6});
  sel.AddVertex({2, 5});
  sel.AddVertex({3, 0});

  auto nodes = sel.GetNodesWithSelectedVertices();

  EXPECT_EQ(nodes.size(), 3u);
  // Order is not guaranteed, just check all are present
  auto contains = [&nodes](int id) {
    return std::find(nodes.begin(), nodes.end(), id) != nodes.end();
  };
  EXPECT_TRUE(contains(1));
  EXPECT_TRUE(contains(2));
  EXPECT_TRUE(contains(3));
}

TEST(SubObjectSelection, SelectReplacesExisting) {
  SubObjectSelection sel;
  VertexRef ref1{1, 5};
  VertexRef ref2{1, 6};

  sel.AddVertex(ref1);
  sel.SelectVertex(ref2);  // Should replace, not add

  EXPECT_EQ(sel.VertexCount(), 1u);
  EXPECT_FALSE(sel.IsVertexSelected(ref1));
  EXPECT_TRUE(sel.IsVertexSelected(ref2));
}

TEST(SubObjectSelection, AddSameVertexTwiceNoDuplicate) {
  SubObjectSelection sel;
  VertexRef ref{1, 5};

  sel.AddVertex(ref);
  sel.AddVertex(ref);

  EXPECT_EQ(sel.VertexCount(), 1u);
}

} // namespace
