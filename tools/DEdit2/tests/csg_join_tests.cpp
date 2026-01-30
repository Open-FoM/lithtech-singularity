#include "brush/csg/csg_convex_hull.h"
#include "brush/csg/csg_join.h"

#include <gtest/gtest.h>

namespace csg {

namespace {

// Helper to create a simple box brush
CSGBrush MakeBoxBrush(float cx, float cy, float cz, float size) {
  float half = size / 2.0f;
  CSGBrush brush;

  // Front face
  CSGPolygon front;
  front.vertices = {CSGVertex(cx - half, cy - half, cz + half), CSGVertex(cx + half, cy - half, cz + half),
                    CSGVertex(cx + half, cy + half, cz + half), CSGVertex(cx - half, cy + half, cz + half)};
  front.ComputePlane();
  brush.polygons.push_back(front);

  // Back face
  CSGPolygon back;
  back.vertices = {CSGVertex(cx + half, cy - half, cz - half), CSGVertex(cx - half, cy - half, cz - half),
                   CSGVertex(cx - half, cy + half, cz - half), CSGVertex(cx + half, cy + half, cz - half)};
  back.ComputePlane();
  brush.polygons.push_back(back);

  // Left face
  CSGPolygon left;
  left.vertices = {CSGVertex(cx - half, cy - half, cz - half), CSGVertex(cx - half, cy - half, cz + half),
                   CSGVertex(cx - half, cy + half, cz + half), CSGVertex(cx - half, cy + half, cz - half)};
  left.ComputePlane();
  brush.polygons.push_back(left);

  // Right face
  CSGPolygon right;
  right.vertices = {CSGVertex(cx + half, cy - half, cz + half), CSGVertex(cx + half, cy - half, cz - half),
                    CSGVertex(cx + half, cy + half, cz - half), CSGVertex(cx + half, cy + half, cz + half)};
  right.ComputePlane();
  brush.polygons.push_back(right);

  // Top face
  CSGPolygon top;
  top.vertices = {CSGVertex(cx - half, cy + half, cz + half), CSGVertex(cx + half, cy + half, cz + half),
                  CSGVertex(cx + half, cy + half, cz - half), CSGVertex(cx - half, cy + half, cz - half)};
  top.ComputePlane();
  brush.polygons.push_back(top);

  // Bottom face
  CSGPolygon bottom;
  bottom.vertices = {CSGVertex(cx - half, cy - half, cz - half), CSGVertex(cx + half, cy - half, cz - half),
                     CSGVertex(cx + half, cy - half, cz + half), CSGVertex(cx - half, cy - half, cz + half)};
  bottom.ComputePlane();
  brush.polygons.push_back(bottom);

  return brush;
}

} // namespace

// =============================================================================
// Convex Hull Tests
// =============================================================================

TEST(CSGConvexHull, ComputeHull_Tetrahedron) {
  std::vector<CSGVertex> points = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(10.0f, 0.0f, 0.0f),
                                   CSGVertex(5.0f, 10.0f, 0.0f), CSGVertex(5.0f, 5.0f, 10.0f)};

  ConvexHullResult result = ComputeConvexHull(points);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.hull.polygons.size(), 4u); // Tetrahedron has 4 faces
}

TEST(CSGConvexHull, ComputeHull_Cube) {
  std::vector<CSGVertex> points = {CSGVertex(-1.0f, -1.0f, -1.0f), CSGVertex(1.0f, -1.0f, -1.0f),
                                   CSGVertex(1.0f, 1.0f, -1.0f),   CSGVertex(-1.0f, 1.0f, -1.0f),
                                   CSGVertex(-1.0f, -1.0f, 1.0f),  CSGVertex(1.0f, -1.0f, 1.0f),
                                   CSGVertex(1.0f, 1.0f, 1.0f),    CSGVertex(-1.0f, 1.0f, 1.0f)};

  ConvexHullResult result = ComputeConvexHull(points);

  EXPECT_TRUE(result.success);
  // Cube has 6 faces, but hull is triangulated so 12 triangles
  EXPECT_GE(result.hull.polygons.size(), 6u);
}

TEST(CSGConvexHull, ComputeHull_TooFewPoints) {
  std::vector<CSGVertex> points = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f),
                                   CSGVertex(2.0f, 0.0f, 0.0f)};

  ConvexHullResult result = ComputeConvexHull(points);

  EXPECT_FALSE(result.success);
}

TEST(CSGConvexHull, ComputeHull_CoplanarPoints) {
  std::vector<CSGVertex> points = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f),
                                   CSGVertex(1.0f, 1.0f, 0.0f), CSGVertex(0.0f, 1.0f, 0.0f)};

  ConvexHullResult result = ComputeConvexHull(points);

  EXPECT_FALSE(result.success); // All coplanar - degenerate
}

TEST(CSGConvexHull, ComputeHull_DuplicatePoints) {
  std::vector<CSGVertex> points = {CSGVertex(0.0f, 0.0f, 0.0f),  CSGVertex(10.0f, 0.0f, 0.0f),
                                   CSGVertex(5.0f, 10.0f, 0.0f), CSGVertex(5.0f, 5.0f, 10.0f),
                                   CSGVertex(0.0f, 0.0f, 0.0f),  // Duplicate
                                   CSGVertex(10.0f, 0.0f, 0.0f), // Duplicate
                                   CSGVertex(5.0f, 5.0f, 10.0f)  // Duplicate
  };

  ConvexHullResult result = ComputeConvexHull(points);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.hull.polygons.size(), 4u);
}

// =============================================================================
// Convexity Check Tests
// =============================================================================

TEST(CSGConvexHull, IsConvexBrush_ConvexBox) {
  CSGBrush brush = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
  EXPECT_TRUE(IsConvexBrush(brush));
}

TEST(CSGConvexHull, IsConvexBrush_EmptyBrush) {
  CSGBrush brush;
  EXPECT_FALSE(IsConvexBrush(brush));
}

// =============================================================================
// Extreme Points Tests
// =============================================================================

TEST(CSGConvexHull, FindExtremePoints_Box) {
  std::vector<CSGVertex> points = {CSGVertex(-5.0f, -3.0f, -7.0f), CSGVertex(5.0f, 3.0f, 7.0f),
                                   CSGVertex(0.0f, 0.0f, 0.0f)};

  auto extremes = FindExtremePoints(points);

  // Check min X
  EXPECT_FLOAT_EQ(points[extremes[0]].x, -5.0f);
  // Check max X
  EXPECT_FLOAT_EQ(points[extremes[1]].x, 5.0f);
  // Check min Y
  EXPECT_FLOAT_EQ(points[extremes[2]].y, -3.0f);
  // Check max Y
  EXPECT_FLOAT_EQ(points[extremes[3]].y, 3.0f);
  // Check min Z
  EXPECT_FLOAT_EQ(points[extremes[4]].z, -7.0f);
  // Check max Z
  EXPECT_FLOAT_EQ(points[extremes[5]].z, 7.0f);
}

// =============================================================================
// Join Brushes Tests
// =============================================================================

TEST(CSGJoin, JoinBrushes_TwoAdjacent) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(-32.0f, 0.0f, 0.0f, 64.0f));
  brushes.push_back(MakeBoxBrush(32.0f, 0.0f, 0.0f, 64.0f));

  JoinResult result = JoinBrushes(brushes);

  EXPECT_TRUE(result.success);
  EXPECT_FALSE(result.joined_brush.polygons.empty());
}

TEST(CSGJoin, JoinBrushes_SingleBrush) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f));

  JoinResult result = JoinBrushes(brushes);

  EXPECT_FALSE(result.success); // Need at least 2 brushes
}

TEST(CSGJoin, JoinBrushes_Overlapping) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f));
  brushes.push_back(MakeBoxBrush(16.0f, 16.0f, 16.0f, 64.0f));

  JoinResult result = JoinBrushes(brushes);

  EXPECT_TRUE(result.success);
}

TEST(CSGJoin, JoinBrushes_Disjoint) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(-100.0f, 0.0f, 0.0f, 32.0f));
  brushes.push_back(MakeBoxBrush(100.0f, 0.0f, 0.0f, 32.0f));

  JoinOptions options;
  options.allow_non_convex = true;

  JoinResult result = JoinBrushes(brushes, options);

  EXPECT_TRUE(result.success);
  // Convex hull should enclose both
}

TEST(CSGJoin, JoinBrushes_ResultIsConvex) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(-16.0f, 0.0f, 0.0f, 32.0f));
  brushes.push_back(MakeBoxBrush(16.0f, 0.0f, 0.0f, 32.0f));

  JoinResult result = JoinBrushes(brushes);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.is_convex);
}

// =============================================================================
// Triangle Mesh API Tests
// =============================================================================

TEST(CSGJoin, JoinBrushes_TriangleMeshAPI) {
  std::vector<std::vector<float>> all_vertices;
  std::vector<std::vector<uint32_t>> all_indices;

  {
    CSGBrush brush = MakeBoxBrush(-32.0f, 0.0f, 0.0f, 32.0f);
    std::vector<float> verts;
    std::vector<uint32_t> inds;
    brush.ToTriangleMesh(verts, inds);
    all_vertices.push_back(verts);
    all_indices.push_back(inds);
  }

  {
    CSGBrush brush = MakeBoxBrush(32.0f, 0.0f, 0.0f, 32.0f);
    std::vector<float> verts;
    std::vector<uint32_t> inds;
    brush.ToTriangleMesh(verts, inds);
    all_vertices.push_back(verts);
    all_indices.push_back(inds);
  }

  CSGOperationResult result = JoinBrushes(all_vertices, all_indices);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.results.size(), 1u);
}

TEST(CSGJoin, JoinBrushes_InvalidInput) {
  std::vector<std::vector<float>> all_vertices;
  std::vector<std::vector<uint32_t>> all_indices;

  // Only one brush (need at least 2)
  all_vertices.push_back({0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f});
  all_indices.push_back({0, 1, 2});

  CSGOperationResult result = JoinBrushes(all_vertices, all_indices);

  EXPECT_FALSE(result.success);
}

// =============================================================================
// Options Tests
// =============================================================================

TEST(CSGJoin, JoinBrushes_RejectNonConvex) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(-100.0f, 0.0f, 0.0f, 32.0f));
  brushes.push_back(MakeBoxBrush(100.0f, 0.0f, 0.0f, 32.0f));

  JoinOptions options;
  options.allow_non_convex = false;

  JoinResult result = JoinBrushes(brushes, options);

  // The convex hull is still convex even for disjoint inputs
  EXPECT_TRUE(result.success);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(CSGJoin, JoinBrushes_EmptyInput) {
  std::vector<CSGBrush> brushes;

  JoinResult result = JoinBrushes(brushes);

  EXPECT_FALSE(result.success);
}

TEST(CSGJoin, JoinBrushes_InvalidBrush) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f));
  brushes.push_back(CSGBrush{}); // Empty/invalid

  JoinResult result = JoinBrushes(brushes);

  EXPECT_FALSE(result.success);
}

TEST(CSGJoin, JoinBrushes_IdenticalBrushes) {
  std::vector<CSGBrush> brushes;
  brushes.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f));
  brushes.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f)); // Same position and size

  JoinResult result = JoinBrushes(brushes);

  EXPECT_TRUE(result.success);
  // Result should be equivalent to single box
}

} // namespace csg
