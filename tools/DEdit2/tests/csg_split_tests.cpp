#include "brush/csg/csg_split.h"

#include <gtest/gtest.h>

namespace csg {

// =============================================================================
// Basic Split Tests
// =============================================================================

TEST(CSGSplit, SplitBrush_SimpleQuad) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(-1.0f, -1.0f, 0.0f), CSGVertex(1.0f, -1.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(-1.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  CSGPlane split_plane(CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.HasBothParts());
}

TEST(CSGSplit, SplitBrush_AllFront) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(2.0f, 0.0f, 0.0f), CSGVertex(2.0f, 1.0f, 0.0f),
                   CSGVertex(1.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  CSGPlane split_plane(CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.AllFront());
  EXPECT_FALSE(result.HasBothParts());
}

TEST(CSGSplit, SplitBrush_AllBack) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(-2.0f, 0.0f, 0.0f), CSGVertex(-1.0f, 0.0f, 0.0f), CSGVertex(-1.0f, 1.0f, 0.0f),
                   CSGVertex(-2.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  CSGPlane split_plane(CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.AllBack());
}

// =============================================================================
// Triangle Mesh API Tests
// =============================================================================

TEST(CSGSplit, SplitBrush_TriangleMeshAPI) {
  // Simple quad as two triangles
  std::vector<float> vertices = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f};
  std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

  float normal[3] = {1.0f, 0.0f, 0.0f};
  CSGOperationResult result = SplitBrush(vertices, indices, normal, 0.0f);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.results.size(), 2u);
}

TEST(CSGSplit, SplitBrush_InvalidVertexData) {
  std::vector<float> vertices; // Empty
  std::vector<uint32_t> indices = {0, 1, 2};

  float normal[3] = {1.0f, 0.0f, 0.0f};
  CSGOperationResult result = SplitBrush(vertices, indices, normal, 0.0f);

  EXPECT_FALSE(result.success);
}

TEST(CSGSplit, SplitBrush_InvalidNormal) {
  std::vector<float> vertices = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f};
  std::vector<uint32_t> indices = {0, 1, 2};

  float normal[3] = {0.0f, 0.0f, 0.0f}; // Zero normal
  CSGOperationResult result = SplitBrush(vertices, indices, normal, 0.0f);

  EXPECT_FALSE(result.success);
}

// =============================================================================
// Axis-Aligned Split Tests
// =============================================================================

TEST(CSGSplit, SplitBrush_XYPlane) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(0.0f, 0.0f, -1.0f), CSGVertex(1.0f, 0.0f, -1.0f), CSGVertex(1.0f, 1.0f, 1.0f),
                   CSGVertex(0.0f, 1.0f, 1.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.HasBothParts());
}

TEST(CSGSplit, SplitBrush_XZPlane) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(0.0f, -1.0f, 0.0f), CSGVertex(1.0f, -1.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  CSGPlane split_plane(CSGVertex(0.0f, 1.0f, 0.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.HasBothParts());
}

TEST(CSGSplit, SplitBrush_YZPlane) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(-1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(-1.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  CSGPlane split_plane(CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.HasBothParts());
}

// =============================================================================
// Diagonal Plane Split Tests
// =============================================================================

TEST(CSGSplit, SplitBrush_DiagonalPlane) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(2.0f, 0.0f, 0.0f), CSGVertex(2.0f, 2.0f, 0.0f),
                   CSGVertex(0.0f, 2.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  // Diagonal plane (45 degrees)
  CSGVertex normal(1.0f, 1.0f, 0.0f);
  normal = normal.Normalized();
  CSGPlane split_plane(normal, 1.0f);

  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
  // Depending on where the plane falls, may or may not split
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(CSGSplit, SplitBrush_PlaneOnVertex) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  // Plane passes through corner vertex
  CSGPlane split_plane(CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
}

TEST(CSGSplit, SplitBrush_PlaneOnEdge) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(-1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(-1.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  // Plane along bottom edge
  CSGPlane split_plane(CSGVertex(0.0f, 1.0f, 0.0f), 0.0f);
  BrushSplitResult result = SplitBrush(brush, split_plane);

  EXPECT_TRUE(result.success);
}

} // namespace csg
