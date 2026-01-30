#include "brush/csg/csg_triangulate.h"

#include <gtest/gtest.h>

namespace csg {

// =============================================================================
// Triangle Fan Tests
// =============================================================================

TEST(CSGTriangulate, FanTriangulate_Triangle) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(0.5f, 1.0f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateFan(poly);

  EXPECT_EQ(triangles.size(), 1u);
  EXPECT_EQ(triangles[0].vertices.size(), 3u);
}

TEST(CSGTriangulate, FanTriangulate_Quad) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateFan(poly);

  EXPECT_EQ(triangles.size(), 2u);
  for (const auto& tri : triangles) {
    EXPECT_EQ(tri.vertices.size(), 3u);
  }
}

TEST(CSGTriangulate, FanTriangulate_Pentagon) {
  CSGPolygon poly;
  // Regular pentagon-ish
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.5f, 0.5f, 0.0f),
                   CSGVertex(0.75f, 1.0f, 0.0f), CSGVertex(-0.25f, 0.5f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateFan(poly);

  // N-gon produces N-2 triangles
  EXPECT_EQ(triangles.size(), 3u);
}

TEST(CSGTriangulate, FanTriangulate_Hexagon) {
  CSGPolygon poly;
  // Regular hexagon-ish
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.5f, 0.5f, 0.0f),
                   CSGVertex(1.0f, 1.0f, 0.0f), CSGVertex(0.0f, 1.0f, 0.0f), CSGVertex(-0.5f, 0.5f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateFan(poly);

  EXPECT_EQ(triangles.size(), 4u);
}

// =============================================================================
// Ear Clipping Tests
// =============================================================================

TEST(CSGTriangulate, EarClipping_Triangle) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(0.5f, 1.0f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateEarClipping(poly);

  EXPECT_EQ(triangles.size(), 1u);
}

TEST(CSGTriangulate, EarClipping_Quad) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateEarClipping(poly);

  EXPECT_EQ(triangles.size(), 2u);
}

TEST(CSGTriangulate, EarClipping_ConvexPolygon) {
  CSGPolygon poly;
  // Convex pentagon
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(2.0f, 0.0f, 0.0f), CSGVertex(2.5f, 1.0f, 0.0f),
                   CSGVertex(1.0f, 2.0f, 0.0f), CSGVertex(-0.5f, 1.0f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateEarClipping(poly);

  EXPECT_EQ(triangles.size(), 3u);
}

TEST(CSGTriangulate, EarClipping_ConcavePolygon) {
  // L-shaped concave polygon
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(2.0f, 0.0f, 0.0f), CSGVertex(2.0f, 1.0f, 0.0f),
                   CSGVertex(1.0f, 1.0f, 0.0f), // Inward vertex
                   CSGVertex(1.0f, 2.0f, 0.0f), CSGVertex(0.0f, 2.0f, 0.0f)};
  poly.ComputePlane();

  auto triangles = TriangulateEarClipping(poly);

  // 6-gon should produce 4 triangles
  EXPECT_EQ(triangles.size(), 4u);

  // All triangles should have 3 vertices
  for (const auto& tri : triangles) {
    EXPECT_EQ(tri.vertices.size(), 3u);
  }
}

// =============================================================================
// Helper Function Tests
// =============================================================================

TEST(CSGTriangulate, IsConvexVertex_Convex) {
  CSGVertex prev(0.0f, 0.0f, 0.0f);
  CSGVertex curr(1.0f, 0.0f, 0.0f);
  CSGVertex next(1.0f, 1.0f, 0.0f);
  CSGVertex normal(0.0f, 0.0f, 1.0f);

  EXPECT_TRUE(IsConvexVertex(prev, curr, next, normal));
}

TEST(CSGTriangulate, IsConvexVertex_Reflex) {
  CSGVertex prev(0.0f, 0.0f, 0.0f);
  CSGVertex curr(1.0f, 0.0f, 0.0f);
  CSGVertex next(0.5f, -0.5f, 0.0f); // Makes a reflex angle
  CSGVertex normal(0.0f, 0.0f, 1.0f);

  EXPECT_FALSE(IsConvexVertex(prev, curr, next, normal));
}

TEST(CSGTriangulate, PointInTriangle_Inside) {
  CSGVertex a(0.0f, 0.0f, 0.0f);
  CSGVertex b(2.0f, 0.0f, 0.0f);
  CSGVertex c(1.0f, 2.0f, 0.0f);
  CSGVertex normal(0.0f, 0.0f, 1.0f);

  CSGVertex inside(1.0f, 0.5f, 0.0f);
  EXPECT_TRUE(PointInTriangle(inside, a, b, c, normal));
}

TEST(CSGTriangulate, PointInTriangle_Outside) {
  CSGVertex a(0.0f, 0.0f, 0.0f);
  CSGVertex b(2.0f, 0.0f, 0.0f);
  CSGVertex c(1.0f, 2.0f, 0.0f);
  CSGVertex normal(0.0f, 0.0f, 1.0f);

  CSGVertex outside(-1.0f, 0.0f, 0.0f);
  EXPECT_FALSE(PointInTriangle(outside, a, b, c, normal));
}

TEST(CSGTriangulate, PointInTriangle_OnEdge) {
  CSGVertex a(0.0f, 0.0f, 0.0f);
  CSGVertex b(2.0f, 0.0f, 0.0f);
  CSGVertex c(1.0f, 2.0f, 0.0f);
  CSGVertex normal(0.0f, 0.0f, 1.0f);

  CSGVertex on_edge(1.0f, 0.0f, 0.0f);
  // Should be considered inside or on boundary
  EXPECT_TRUE(PointInTriangle(on_edge, a, b, c, normal));
}

// =============================================================================
// Triangulate Function Tests (auto-selection)
// =============================================================================

TEST(CSGTriangulate, Triangulate_ConvexUsesFan) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  poly.ComputePlane();

  auto triangles = Triangulate(poly);

  EXPECT_EQ(triangles.size(), 2u);
}

TEST(CSGTriangulate, Triangulate_PreservesArea) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(2.0f, 0.0f, 0.0f), CSGVertex(2.0f, 2.0f, 0.0f),
                   CSGVertex(0.0f, 2.0f, 0.0f)};
  poly.ComputePlane();

  float original_area = poly.Area();
  auto triangles = Triangulate(poly);

  float total_area = 0.0f;
  for (const auto& tri : triangles) {
    total_area += tri.Area();
  }

  EXPECT_NEAR(total_area, original_area, 0.001f);
}

// =============================================================================
// Brush Triangulation Tests
// =============================================================================

TEST(CSGTriangulate, TriangulateBrush_AllTriangles) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  quad.ComputePlane();
  brush.polygons.push_back(quad);
  brush.polygons.push_back(quad);

  CSGBrush triangulated = TriangulateBrush(brush);

  // 2 quads -> 4 triangles
  EXPECT_EQ(triangulated.polygons.size(), 4u);

  for (const auto& poly : triangulated.polygons) {
    EXPECT_TRUE(poly.IsTriangle());
  }
}

TEST(CSGTriangulate, TriangulateBrush_PreservesMaterial) {
  CSGBrush brush;

  CSGPolygon quad;
  quad.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  quad.material_id = 42;
  quad.ComputePlane();
  brush.polygons.push_back(quad);

  CSGBrush triangulated = TriangulateBrush(brush);

  for (const auto& poly : triangulated.polygons) {
    EXPECT_EQ(poly.material_id, 42u);
  }
}

} // namespace csg
