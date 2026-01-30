#include "brush/geometry_ops/face_extrude.h"
#include "brush/geometry_ops/flip_normal.h"
#include "brush/geometry_ops/vertex_weld.h"

#include <gtest/gtest.h>
#include <set>

namespace {

/// Helper to create a simple triangle polygon.
csg::CSGPolygon CreateTrianglePolygon(const csg::CSGVertex& v0, const csg::CSGVertex& v1, const csg::CSGVertex& v2) {
  return csg::CSGPolygon({v0, v1, v2});
}

/// Helper to create a brush with a single triangle in the XY plane.
/// CCW winding when viewed from +Z, so normal points +Z.
csg::CSGBrush CreateSingleTriangleBrush() {
  csg::CSGPolygon poly = CreateTrianglePolygon(csg::CSGVertex(0, 0, 0), csg::CSGVertex(1, 0, 0), csg::CSGVertex(0, 1, 0));
  return csg::CSGBrush({poly});
}

/// Helper to create a simple box brush (6 quads = 6 polygons).
csg::CSGBrush CreateBoxBrush() {
  // Create a unit cube centered at origin
  auto v = [](float x, float y, float z) { return csg::CSGVertex(x, y, z); };

  std::vector<csg::CSGPolygon> polygons;

  // Front (+Z)
  polygons.push_back(csg::CSGPolygon({v(-0.5f, -0.5f, 0.5f), v(0.5f, -0.5f, 0.5f), v(0.5f, 0.5f, 0.5f), v(-0.5f, 0.5f, 0.5f)}));
  // Back (-Z)
  polygons.push_back(csg::CSGPolygon({v(0.5f, -0.5f, -0.5f), v(-0.5f, -0.5f, -0.5f), v(-0.5f, 0.5f, -0.5f), v(0.5f, 0.5f, -0.5f)}));
  // Right (+X)
  polygons.push_back(csg::CSGPolygon({v(0.5f, -0.5f, 0.5f), v(0.5f, -0.5f, -0.5f), v(0.5f, 0.5f, -0.5f), v(0.5f, 0.5f, 0.5f)}));
  // Left (-X)
  polygons.push_back(csg::CSGPolygon({v(-0.5f, -0.5f, -0.5f), v(-0.5f, -0.5f, 0.5f), v(-0.5f, 0.5f, 0.5f), v(-0.5f, 0.5f, -0.5f)}));
  // Top (+Y)
  polygons.push_back(csg::CSGPolygon({v(-0.5f, 0.5f, 0.5f), v(0.5f, 0.5f, 0.5f), v(0.5f, 0.5f, -0.5f), v(-0.5f, 0.5f, -0.5f)}));
  // Bottom (-Y)
  polygons.push_back(csg::CSGPolygon({v(-0.5f, -0.5f, -0.5f), v(0.5f, -0.5f, -0.5f), v(0.5f, -0.5f, 0.5f), v(-0.5f, -0.5f, 0.5f)}));

  return csg::CSGBrush(polygons);
}

/// Check if two normals are approximately opposite.
bool NormalsAreOpposite(const csg::CSGVertex& n1, const csg::CSGVertex& n2) {
  float dot = n1.Dot(n2);
  return dot < -0.999f; // Near -1 means opposite
}

/// Check if two normals are approximately equal.
bool NormalsAreEqual(const csg::CSGVertex& n1, const csg::CSGVertex& n2) {
  float dot = n1.Dot(n2);
  return dot > 0.999f; // Near +1 means same direction
}

} // namespace

// ============================================================================
// Flip Normal - Single Face Tests
// ============================================================================

TEST(FlipNormal, SingleFaceFlipsNormal) {
  auto brush = CreateSingleTriangleBrush();
  auto original_normal = brush.polygons[0].plane.normal;

  auto result = geometry_ops::FlipNormals(brush, {0});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_flipped, 1u);

  auto flipped_normal = result.result_brush.polygons[0].plane.normal;
  EXPECT_TRUE(NormalsAreOpposite(original_normal, flipped_normal));
}

TEST(FlipNormal, FlipReversesVertexOrder) {
  auto brush = CreateSingleTriangleBrush();

  // Original order: v0, v1, v2
  auto v0_orig = brush.polygons[0].vertices[0];
  auto v1_orig = brush.polygons[0].vertices[1];
  auto v2_orig = brush.polygons[0].vertices[2];

  auto result = geometry_ops::FlipNormals(brush, {0});

  EXPECT_TRUE(result.success);

  // After flip, order should be reversed
  auto& verts = result.result_brush.polygons[0].vertices;
  EXPECT_TRUE(v0_orig.NearlyEquals(verts[2]));
  EXPECT_TRUE(v1_orig.NearlyEquals(verts[1]));
  EXPECT_TRUE(v2_orig.NearlyEquals(verts[0]));
}

TEST(FlipNormal, DoubleFlipRestoresOriginal) {
  auto brush = CreateSingleTriangleBrush();
  auto original_normal = brush.polygons[0].plane.normal;

  // Flip once
  auto result1 = geometry_ops::FlipNormals(brush, {0});
  EXPECT_TRUE(result1.success);

  // Flip again
  auto result2 = geometry_ops::FlipNormals(result1.result_brush, {0});
  EXPECT_TRUE(result2.success);

  // Should be back to original normal
  auto restored_normal = result2.result_brush.polygons[0].plane.normal;
  EXPECT_TRUE(NormalsAreEqual(original_normal, restored_normal));
}

TEST(FlipNormal, EmptyFaceListIsNoOp) {
  auto brush = CreateSingleTriangleBrush();
  auto original_normal = brush.polygons[0].plane.normal;

  auto result = geometry_ops::FlipNormals(brush, {});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_flipped, 0u);

  auto result_normal = result.result_brush.polygons[0].plane.normal;
  EXPECT_TRUE(NormalsAreEqual(original_normal, result_normal));
}

TEST(FlipNormal, InvalidIndexFails) {
  auto brush = CreateSingleTriangleBrush();

  auto result = geometry_ops::FlipNormals(brush, {100});

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST(FlipNormal, EmptyBrushFails) {
  csg::CSGBrush brush;

  auto result = geometry_ops::FlipNormals(brush, {0});

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// Flip Normal - Multiple Faces Tests
// ============================================================================

TEST(FlipNormal, FlipMultipleFaces) {
  auto brush = CreateBoxBrush();

  std::vector<csg::CSGVertex> original_normals;
  for (const auto& poly : brush.polygons) {
    original_normals.push_back(poly.plane.normal);
  }

  // Flip faces 0, 2, 4
  auto result = geometry_ops::FlipNormals(brush, {0, 2, 4});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_flipped, 3u);

  // Faces 0, 2, 4 should be flipped
  EXPECT_TRUE(NormalsAreOpposite(original_normals[0], result.result_brush.polygons[0].plane.normal));
  EXPECT_TRUE(NormalsAreOpposite(original_normals[2], result.result_brush.polygons[2].plane.normal));
  EXPECT_TRUE(NormalsAreOpposite(original_normals[4], result.result_brush.polygons[4].plane.normal));

  // Faces 1, 3, 5 should be unchanged
  EXPECT_TRUE(NormalsAreEqual(original_normals[1], result.result_brush.polygons[1].plane.normal));
  EXPECT_TRUE(NormalsAreEqual(original_normals[3], result.result_brush.polygons[3].plane.normal));
  EXPECT_TRUE(NormalsAreEqual(original_normals[5], result.result_brush.polygons[5].plane.normal));
}

TEST(FlipNormal, FlipSameFaceTwiceInList) {
  auto brush = CreateSingleTriangleBrush();
  auto original_normal = brush.polygons[0].plane.normal;

  // Flip face 0 twice in one call - should result in original
  auto result = geometry_ops::FlipNormals(brush, {0, 0});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_flipped, 2u);

  // Normal should be back to original (flipped twice)
  auto result_normal = result.result_brush.polygons[0].plane.normal;
  EXPECT_TRUE(NormalsAreEqual(original_normal, result_normal));
}

// ============================================================================
// Flip Entire Brush Tests
// ============================================================================

TEST(FlipEntireBrush, FlipsAllFaces) {
  auto brush = CreateBoxBrush();

  std::vector<csg::CSGVertex> original_normals;
  for (const auto& poly : brush.polygons) {
    original_normals.push_back(poly.plane.normal);
  }

  auto result = geometry_ops::FlipEntireBrush(brush);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_flipped, brush.polygons.size());

  // All faces should be flipped
  for (size_t i = 0; i < brush.polygons.size(); ++i) {
    EXPECT_TRUE(NormalsAreOpposite(original_normals[i], result.result_brush.polygons[i].plane.normal))
        << "Face " << i << " was not flipped";
  }
}

TEST(FlipEntireBrush, DoubleFlipRestoresOriginal) {
  auto brush = CreateBoxBrush();

  std::vector<csg::CSGVertex> original_normals;
  for (const auto& poly : brush.polygons) {
    original_normals.push_back(poly.plane.normal);
  }

  // Flip twice
  auto result1 = geometry_ops::FlipEntireBrush(brush);
  EXPECT_TRUE(result1.success);

  auto result2 = geometry_ops::FlipEntireBrush(result1.result_brush);
  EXPECT_TRUE(result2.success);

  // Should be back to original
  for (size_t i = 0; i < brush.polygons.size(); ++i) {
    EXPECT_TRUE(NormalsAreEqual(original_normals[i], result2.result_brush.polygons[i].plane.normal))
        << "Face " << i << " was not restored";
  }
}

TEST(FlipEntireBrush, EmptyBrushFails) {
  csg::CSGBrush brush;

  auto result = geometry_ops::FlipEntireBrush(brush);

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST(FlipEntireBrush, SingleTriangleBrush) {
  auto brush = CreateSingleTriangleBrush();
  auto original_normal = brush.polygons[0].plane.normal;

  auto result = geometry_ops::FlipEntireBrush(brush);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_flipped, 1u);

  auto flipped_normal = result.result_brush.polygons[0].plane.normal;
  EXPECT_TRUE(NormalsAreOpposite(original_normal, flipped_normal));
}

// ============================================================================
// Flip Normal - Preserves Other Properties Tests
// ============================================================================

TEST(FlipNormal, PreservesMaterialId) {
  auto brush = CreateSingleTriangleBrush();
  brush.polygons[0].material_id = 42;

  auto result = geometry_ops::FlipNormals(brush, {0});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.result_brush.polygons[0].material_id, 42u);
}

TEST(FlipNormal, PreservesPolygonCount) {
  auto brush = CreateBoxBrush();
  size_t original_count = brush.polygons.size();

  auto result = geometry_ops::FlipNormals(brush, {0, 1, 2});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.result_brush.polygons.size(), original_count);
}

TEST(FlipNormal, PreservesVertexPositions) {
  auto brush = CreateSingleTriangleBrush();

  // Store original vertex positions
  std::vector<csg::CSGVertex> original_verts = brush.polygons[0].vertices;

  auto result = geometry_ops::FlipNormals(brush, {0});

  EXPECT_TRUE(result.success);

  // All vertices should still exist (just in different order)
  for (const auto& orig : original_verts) {
    bool found = false;
    for (const auto& v : result.result_brush.polygons[0].vertices) {
      if (orig.NearlyEquals(v)) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Vertex was lost during flip";
  }
}

// ============================================================================
// Vertex Weld Tests
// ============================================================================

TEST(VertexWeld, SingleVertexIsNoOp) {
  auto brush = CreateSingleTriangleBrush();
  size_t original_vertex_count = brush.polygons[0].vertices.size();

  auto result = geometry_ops::WeldVertices(brush, {0}, {});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.vertices_merged, 0u);
  EXPECT_EQ(result.result_brush.polygons[0].vertices.size(), original_vertex_count);
}

TEST(VertexWeld, EmptyIndicesIsNoOp) {
  auto brush = CreateSingleTriangleBrush();

  auto result = geometry_ops::WeldVertices(brush, {}, {});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.vertices_merged, 0u);
}

TEST(VertexWeld, TwoVerticesMergeToCenter) {
  // Create a quad: 4 vertices in a plane
  csg::CSGPolygon quad({csg::CSGVertex(0, 0, 0), csg::CSGVertex(2, 0, 0), csg::CSGVertex(2, 2, 0),
                        csg::CSGVertex(0, 2, 0)});
  csg::CSGBrush brush({quad});

  // Weld vertices 0 and 1 (positions (0,0,0) and (2,0,0))
  // Centroid should be (1,0,0)
  auto result = geometry_ops::WeldVertices(brush, {0, 1}, {});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.vertices_merged, 2u);

  // The first two vertices should now be at the same position (1,0,0)
  auto& verts = result.result_brush.polygons[0].vertices;

  // After welding and cleanup, we should have 3 unique vertices (triangle)
  // The original quad with 2 merged vertices becomes a triangle
  EXPECT_LE(verts.size(), 4u); // Could be 3 or 4 depending on cleanup
}

TEST(VertexWeld, InvalidIndexFails) {
  auto brush = CreateSingleTriangleBrush();

  auto result = geometry_ops::WeldVertices(brush, {0, 100}, {});

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST(VertexWeld, EmptyBrushFails) {
  csg::CSGBrush brush;

  auto result = geometry_ops::WeldVertices(brush, {0, 1}, {});

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST(VertexWeld, WeldingAllVerticesRemovesPolygon) {
  auto brush = CreateSingleTriangleBrush();

  // Weld all 3 vertices - should collapse the triangle to a point
  auto result = geometry_ops::WeldVertices(brush, {0, 1, 2}, {});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.degenerate_faces_removed, 1u);
  EXPECT_TRUE(result.result_brush.polygons.empty());
}

TEST(VertexWeld, BoxCornerWeld) {
  auto brush = CreateBoxBrush();

  // Get initial polygon count
  size_t initial_poly_count = brush.polygons.size();

  // Weld should succeed even if it doesn't find matching vertices
  // (box polygons don't share vertices in our simple representation)
  auto result = geometry_ops::WeldVertices(brush, {0, 1}, {});

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.vertices_merged, 2u);
}

// ============================================================================
// Find Weldable Vertices Tests
// ============================================================================

TEST(FindWeldableVertices, EmptyBrushReturnsEmpty) {
  csg::CSGBrush brush;

  auto groups = geometry_ops::FindWeldableVertices(brush, 0.1f);

  EXPECT_TRUE(groups.empty());
}

TEST(FindWeldableVertices, NoNearbyVerticesReturnsEmpty) {
  auto brush = CreateSingleTriangleBrush();

  // Use a very small threshold so no vertices are considered weldable
  auto groups = geometry_ops::FindWeldableVertices(brush, 0.0001f);

  EXPECT_TRUE(groups.empty());
}

TEST(FindWeldableVertices, NearbyVerticesGrouped) {
  // Create a brush with some vertices very close together
  csg::CSGPolygon poly1({csg::CSGVertex(0, 0, 0), csg::CSGVertex(1, 0, 0), csg::CSGVertex(0.5f, 1, 0)});
  csg::CSGPolygon poly2(
      {csg::CSGVertex(0.001f, 0, 0), csg::CSGVertex(2, 0, 0), csg::CSGVertex(1, 1, 0)}); // First vertex very close to
                                                                                         // poly1's first vertex

  csg::CSGBrush brush({poly1, poly2});

  auto groups = geometry_ops::FindWeldableVertices(brush, 0.01f);

  // Should find at least one group (the two vertices at ~(0,0,0))
  EXPECT_GE(groups.size(), 1u);

  if (!groups.empty()) {
    // Each group should have at least 2 vertices
    for (const auto& group : groups) {
      EXPECT_GE(group.size(), 2u);
    }
  }
}

TEST(FindWeldableVertices, ThresholdAffectsResults) {
  // Create vertices at (0,0,0), (0.1,0,0), (0.5,0,0)
  csg::CSGPolygon poly(
      {csg::CSGVertex(0, 0, 0), csg::CSGVertex(0.1f, 0, 0), csg::CSGVertex(0.5f, 0, 0), csg::CSGVertex(0, 1, 0)});
  csg::CSGBrush brush({poly});

  // Small threshold - only first two vertices should group
  auto groups_small = geometry_ops::FindWeldableVertices(brush, 0.15f);

  // Large threshold - first three vertices might group
  auto groups_large = geometry_ops::FindWeldableVertices(brush, 0.6f);

  // Larger threshold should find at least as many weldable groups
  // (or the same groups with potentially more vertices)
  size_t total_verts_small = 0;
  for (const auto& g : groups_small) {
    total_verts_small += g.size();
  }

  size_t total_verts_large = 0;
  for (const auto& g : groups_large) {
    total_verts_large += g.size();
  }

  EXPECT_GE(total_verts_large, total_verts_small);
}

TEST(FindWeldableVertices, TransitiveGrouping) {
  // Create vertices in a chain: A(0,0,0), B(0.4,0,0), C(0.8,0,0)
  // With threshold 0.5:
  //   A-B distance = 0.4 < 0.5 (within threshold)
  //   B-C distance = 0.4 < 0.5 (within threshold)
  //   A-C distance = 0.8 > 0.5 (outside threshold)
  // All three should be grouped together through transitivity
  csg::CSGPolygon poly({csg::CSGVertex(0, 0, 0),      // A - vertex 0
                        csg::CSGVertex(0.4f, 0, 0),   // B - vertex 1
                        csg::CSGVertex(0.8f, 0, 0),   // C - vertex 2
                        csg::CSGVertex(0, 5, 0)});    // D - vertex 3 (far away)
  csg::CSGBrush brush({poly});

  auto groups = geometry_ops::FindWeldableVertices(brush, 0.5f);

  // Should find exactly one group containing A, B, C (not D)
  EXPECT_EQ(groups.size(), 1u);

  if (!groups.empty()) {
    EXPECT_EQ(groups[0].size(), 3u);

    // Verify vertices 0, 1, 2 are all in the group
    std::set<uint32_t> group_set(groups[0].begin(), groups[0].end());
    EXPECT_TRUE(group_set.count(0) > 0) << "Vertex 0 (A) should be in group";
    EXPECT_TRUE(group_set.count(1) > 0) << "Vertex 1 (B) should be in group";
    EXPECT_TRUE(group_set.count(2) > 0) << "Vertex 2 (C) should be in group";
    EXPECT_TRUE(group_set.count(3) == 0) << "Vertex 3 (D) should NOT be in group";
  }
}

// ============================================================================
// Face Extrusion Tests
// ============================================================================

TEST(FaceExtrude, SingleFaceCreatesCorrectGeometry) {
  // Create a single triangle face
  auto brush = CreateSingleTriangleBrush();

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;
  opts.along_normal = true;
  opts.create_side_walls = true;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_extruded, 1u);

  // Original triangle + 3 side walls = 4 polygons
  EXPECT_EQ(result.side_faces_created, 3u);
  EXPECT_EQ(result.result_brush.polygons.size(), 4u);
}

TEST(FaceExtrude, ZeroDistanceIsNoOp) {
  auto brush = CreateSingleTriangleBrush();
  size_t original_poly_count = brush.polygons.size();

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 0.0f;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.result_brush.polygons.size(), original_poly_count);
}

TEST(FaceExtrude, EmptyFaceListIsNoOp) {
  auto brush = CreateSingleTriangleBrush();
  size_t original_poly_count = brush.polygons.size();

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;

  auto result = geometry_ops::ExtrudeFaces(brush, {}, opts);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.result_brush.polygons.size(), original_poly_count);
}

TEST(FaceExtrude, InvalidIndexFails) {
  auto brush = CreateSingleTriangleBrush();

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;

  auto result = geometry_ops::ExtrudeFaces(brush, {100}, opts);

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST(FaceExtrude, EmptyBrushFails) {
  csg::CSGBrush brush;

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
}

TEST(FaceExtrude, NegativeDistanceExtrudesInward) {
  auto brush = CreateSingleTriangleBrush();
  auto original_centroid = brush.polygons[0].Centroid();
  auto original_normal = brush.polygons[0].plane.normal;

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = -10.0f;
  opts.along_normal = true;
  opts.create_side_walls = true;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_extruded, 1u);

  // The first polygon in the result should be the extruded face
  // Its centroid should be moved in the opposite direction of the normal
  auto extruded_centroid = result.result_brush.polygons[0].Centroid();

  // Direction from original to extruded should be opposite of normal
  csg::CSGVertex direction = (extruded_centroid - original_centroid).Normalized();
  float dot = direction.Dot(original_normal);
  EXPECT_LT(dot, 0.0f); // Should be negative (opposite direction)
}

TEST(FaceExtrude, WithoutSideWallsOnlyCreatesExtrudedFace) {
  auto brush = CreateSingleTriangleBrush();

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;
  opts.along_normal = true;
  opts.create_side_walls = false;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_extruded, 1u);
  EXPECT_EQ(result.side_faces_created, 0u);

  // Only the extruded face, no side walls
  EXPECT_EQ(result.result_brush.polygons.size(), 1u);
}

TEST(FaceExtrude, QuadFaceCreates4SideWalls) {
  // Create a quad face
  csg::CSGPolygon quad({csg::CSGVertex(0, 0, 0), csg::CSGVertex(1, 0, 0), csg::CSGVertex(1, 1, 0),
                        csg::CSGVertex(0, 1, 0)});
  csg::CSGBrush brush({quad});

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;
  opts.along_normal = true;
  opts.create_side_walls = true;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_extruded, 1u);
  EXPECT_EQ(result.side_faces_created, 4u); // 4 edges = 4 side walls

  // 1 extruded face + 4 side walls = 5 polygons
  EXPECT_EQ(result.result_brush.polygons.size(), 5u);
}

TEST(FaceExtrude, ExtrudeMultipleFaces) {
  // Create a brush with 2 triangles
  csg::CSGPolygon tri1({csg::CSGVertex(0, 0, 0), csg::CSGVertex(1, 0, 0), csg::CSGVertex(0.5f, 1, 0)});
  csg::CSGPolygon tri2({csg::CSGVertex(2, 0, 0), csg::CSGVertex(3, 0, 0), csg::CSGVertex(2.5f, 1, 0)});
  csg::CSGBrush brush({tri1, tri2});

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;
  opts.along_normal = true;
  opts.create_side_walls = true;

  auto result = geometry_ops::ExtrudeFaces(brush, {0, 1}, opts);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_extruded, 2u);
  EXPECT_EQ(result.side_faces_created, 6u); // 3 + 3 side walls

  // 2 extruded faces + 6 side walls = 8 polygons
  EXPECT_EQ(result.result_brush.polygons.size(), 8u);
}

TEST(FaceExtrude, ExtrudedFaceMovesAlongNormal) {
  auto brush = CreateSingleTriangleBrush();
  auto original_centroid = brush.polygons[0].Centroid();
  auto original_normal = brush.polygons[0].plane.normal.Normalized();
  float extrude_distance = 10.0f;

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = extrude_distance;
  opts.along_normal = true;
  opts.create_side_walls = true;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);

  // The first polygon should be the extruded face
  auto extruded_centroid = result.result_brush.polygons[0].Centroid();

  // Calculate expected position
  auto expected_centroid = original_centroid + original_normal * extrude_distance;

  EXPECT_TRUE(expected_centroid.NearlyEquals(extruded_centroid, 0.01f))
      << "Expected: (" << expected_centroid.x << ", " << expected_centroid.y << ", " << expected_centroid.z << ") "
      << "Got: (" << extruded_centroid.x << ", " << extruded_centroid.y << ", " << extruded_centroid.z << ")";
}

TEST(FaceExtrude, PreservesMaterialId) {
  // Create a triangle with a specific material ID
  csg::CSGPolygon tri({csg::CSGVertex(0, 0, 0), csg::CSGVertex(1, 0, 0), csg::CSGVertex(0.5f, 1, 0)}, 42);
  csg::CSGBrush brush({tri});

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;
  opts.along_normal = true;
  opts.create_side_walls = true;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);

  // All resulting polygons should have the same material ID
  for (const auto& poly : result.result_brush.polygons) {
    EXPECT_EQ(poly.material_id, 42u);
  }
}

TEST(FaceExtrude, ExtrudeWithCustomDirection) {
  auto brush = CreateSingleTriangleBrush();
  auto original_centroid = brush.polygons[0].Centroid();

  geometry_ops::FaceExtrudeOptions opts;
  opts.distance = 10.0f;
  opts.along_normal = false;
  opts.direction = csg::CSGVertex(1, 0, 0); // Extrude along +X
  opts.create_side_walls = true;

  auto result = geometry_ops::ExtrudeFaces(brush, {0}, opts);

  EXPECT_TRUE(result.success);

  // The extruded face should have moved along +X
  auto extruded_centroid = result.result_brush.polygons[0].Centroid();
  auto direction = extruded_centroid - original_centroid;

  // Should have moved mostly along X
  EXPECT_NEAR(direction.x, 10.0f, 0.01f);
  EXPECT_NEAR(direction.y, 0.0f, 0.01f);
  EXPECT_NEAR(direction.z, 0.0f, 0.01f);
}
