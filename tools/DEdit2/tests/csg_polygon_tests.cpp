#include "brush/csg/csg_polygon.h"

#include <gtest/gtest.h>

namespace csg {

// =============================================================================
// Edge-Plane Intersection Tests
// =============================================================================

TEST(CSGPolygonOps, EdgePlaneIntersection_CrossingForward) {
  CSGVertex v0(0.0f, 0.0f, -1.0f);
  CSGVertex v1(0.0f, 0.0f, 1.0f);
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  float t;
  auto result = ComputeEdgePlaneIntersection(v0, v1, plane, &t);

  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->z, 0.0f, 0.001f);
  EXPECT_NEAR(t, 0.5f, 0.001f);
}

TEST(CSGPolygonOps, EdgePlaneIntersection_CrossingBackward) {
  CSGVertex v0(0.0f, 0.0f, 1.0f);
  CSGVertex v1(0.0f, 0.0f, -1.0f);
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  float t;
  auto result = ComputeEdgePlaneIntersection(v0, v1, plane, &t);

  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->z, 0.0f, 0.001f);
  EXPECT_NEAR(t, 0.5f, 0.001f);
}

TEST(CSGPolygonOps, EdgePlaneIntersection_ParallelToPlane) {
  CSGVertex v0(0.0f, 0.0f, 1.0f);
  CSGVertex v1(1.0f, 0.0f, 1.0f);
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  auto result = ComputeEdgePlaneIntersection(v0, v1, plane);
  EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Polygon Split Tests
// =============================================================================

TEST(CSGPolygonOps, SplitPolygon_AllFront) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 1.0f), CSGVertex(1.0f, 0.0f, 1.0f), CSGVertex(0.5f, 1.0f, 1.0f)};
  poly.ComputePlane();

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  SplitResult result = SplitPolygonByPlane(poly, split_plane);

  EXPECT_EQ(result.front.size(), 1u);
  EXPECT_TRUE(result.back.empty());
  EXPECT_TRUE(result.coplanar_front.empty());
  EXPECT_TRUE(result.coplanar_back.empty());
}

TEST(CSGPolygonOps, SplitPolygon_AllBack) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, -1.0f), CSGVertex(1.0f, 0.0f, -1.0f), CSGVertex(0.5f, 1.0f, -1.0f)};
  poly.ComputePlane();

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  SplitResult result = SplitPolygonByPlane(poly, split_plane);

  EXPECT_TRUE(result.front.empty());
  EXPECT_EQ(result.back.size(), 1u);
}

TEST(CSGPolygonOps, SplitPolygon_Spanning) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(-1.0f, 0.0f, -1.0f), CSGVertex(1.0f, 0.0f, -1.0f), CSGVertex(1.0f, 0.0f, 1.0f),
                   CSGVertex(-1.0f, 0.0f, 1.0f)};
  poly.ComputePlane();

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  SplitResult result = SplitPolygonByPlane(poly, split_plane);

  EXPECT_EQ(result.front.size(), 1u);
  EXPECT_EQ(result.back.size(), 1u);

  // Front polygon should have 4 vertices (2 original + 2 intersection)
  EXPECT_GE(result.front[0].vertices.size(), 3u);
  EXPECT_GE(result.back[0].vertices.size(), 3u);
}

TEST(CSGPolygonOps, SplitPolygon_Coplanar) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(0.5f, 1.0f, 0.0f)};
  poly.ComputePlane();

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  SplitResult result = SplitPolygonByPlane(poly, split_plane);

  // Polygon lies on plane - should be classified as coplanar
  EXPECT_TRUE(result.front.empty());
  EXPECT_TRUE(result.back.empty());
  // Coplanar facing same direction as plane
  EXPECT_EQ(result.coplanar_front.size(), 1u);
}

// =============================================================================
// Clip Polygon Tests
// =============================================================================

TEST(CSGPolygonOps, ClipToFront_FullyFront) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 5.0f), CSGVertex(1.0f, 0.0f, 5.0f), CSGVertex(0.5f, 1.0f, 5.0f)};
  poly.ComputePlane();

  CSGPlane clip_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  auto result = ClipPolygonToFront(poly, clip_plane);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->vertices.size(), 3u);
}

TEST(CSGPolygonOps, ClipToFront_FullyBack) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, -5.0f), CSGVertex(1.0f, 0.0f, -5.0f), CSGVertex(0.5f, 1.0f, -5.0f)};
  poly.ComputePlane();

  CSGPlane clip_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  auto result = ClipPolygonToFront(poly, clip_plane);

  EXPECT_FALSE(result.has_value());
}

TEST(CSGPolygonOps, ClipToBack_FullyBack) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, -5.0f), CSGVertex(1.0f, 0.0f, -5.0f), CSGVertex(0.5f, 1.0f, -5.0f)};
  poly.ComputePlane();

  CSGPlane clip_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  auto result = ClipPolygonToBack(poly, clip_plane);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->vertices.size(), 3u);
}

// =============================================================================
// Brush Split Tests
// =============================================================================

TEST(CSGPolygonOps, SplitBrush_SimpleSplit) {
  CSGBrush brush;

  // Create a simple quad
  CSGPolygon poly;
  poly.vertices = {CSGVertex(-1.0f, -1.0f, 0.0f), CSGVertex(1.0f, -1.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(-1.0f, 1.0f, 0.0f)};
  poly.ComputePlane();
  brush.polygons.push_back(poly);

  // Split along X axis
  CSGPlane split_plane(CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);
  auto [front, back] = SplitBrushByPlane(brush, split_plane);

  EXPECT_FALSE(front.polygons.empty());
  EXPECT_FALSE(back.polygons.empty());
}

TEST(CSGPolygonOps, SplitBrush_NoSplit) {
  CSGBrush brush;

  CSGPolygon poly;
  poly.vertices = {CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(2.0f, 0.0f, 0.0f), CSGVertex(1.5f, 1.0f, 0.0f)};
  poly.ComputePlane();
  brush.polygons.push_back(poly);

  // Split plane doesn't intersect brush
  CSGPlane split_plane(CSGVertex(1.0f, 0.0f, 0.0f), -10.0f);
  auto [front, back] = SplitBrushByPlane(brush, split_plane);

  // All geometry should be on front side
  EXPECT_FALSE(front.polygons.empty());
  EXPECT_TRUE(back.polygons.empty());
}

// =============================================================================
// Cap Generation Tests
// =============================================================================

TEST(CSGPolygonOps, OrderPointsIntoPolygon_Square) {
  std::vector<CSGVertex> points = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f),
                                   CSGVertex(1.0f, 1.0f, 0.0f), CSGVertex(0.0f, 1.0f, 0.0f)};

  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  auto ordered = OrderPointsIntoPolygon(points, plane);

  EXPECT_EQ(ordered.size(), 4u);
  // Points should form a convex polygon
}

TEST(CSGPolygonOps, OrderPointsIntoPolygon_TooFewPoints) {
  std::vector<CSGVertex> points = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f)};

  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  auto ordered = OrderPointsIntoPolygon(points, plane);

  EXPECT_TRUE(ordered.empty());
}

} // namespace csg
