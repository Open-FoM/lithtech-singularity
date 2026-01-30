#include "brush/csg/csg_types.h"

#include <gtest/gtest.h>

#include <cmath>

namespace csg {

// =============================================================================
// CSGVertex Tests
// =============================================================================

TEST(CSGVertex, DefaultConstruction) {
  CSGVertex v;
  EXPECT_FLOAT_EQ(v.x, 0.0f);
  EXPECT_FLOAT_EQ(v.y, 0.0f);
  EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(CSGVertex, ValueConstruction) {
  CSGVertex v(1.0f, 2.0f, 3.0f);
  EXPECT_FLOAT_EQ(v.x, 1.0f);
  EXPECT_FLOAT_EQ(v.y, 2.0f);
  EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST(CSGVertex, Addition) {
  CSGVertex a(1.0f, 2.0f, 3.0f);
  CSGVertex b(4.0f, 5.0f, 6.0f);
  CSGVertex c = a + b;
  EXPECT_FLOAT_EQ(c.x, 5.0f);
  EXPECT_FLOAT_EQ(c.y, 7.0f);
  EXPECT_FLOAT_EQ(c.z, 9.0f);
}

TEST(CSGVertex, Subtraction) {
  CSGVertex a(4.0f, 5.0f, 6.0f);
  CSGVertex b(1.0f, 2.0f, 3.0f);
  CSGVertex c = a - b;
  EXPECT_FLOAT_EQ(c.x, 3.0f);
  EXPECT_FLOAT_EQ(c.y, 3.0f);
  EXPECT_FLOAT_EQ(c.z, 3.0f);
}

TEST(CSGVertex, ScalarMultiplication) {
  CSGVertex v(1.0f, 2.0f, 3.0f);
  CSGVertex c = v * 2.0f;
  EXPECT_FLOAT_EQ(c.x, 2.0f);
  EXPECT_FLOAT_EQ(c.y, 4.0f);
  EXPECT_FLOAT_EQ(c.z, 6.0f);
}

TEST(CSGVertex, DotProduct) {
  CSGVertex a(1.0f, 0.0f, 0.0f);
  CSGVertex b(0.0f, 1.0f, 0.0f);
  EXPECT_FLOAT_EQ(a.Dot(b), 0.0f);

  CSGVertex c(1.0f, 2.0f, 3.0f);
  CSGVertex d(4.0f, 5.0f, 6.0f);
  EXPECT_FLOAT_EQ(c.Dot(d), 32.0f); // 1*4 + 2*5 + 3*6 = 32
}

TEST(CSGVertex, CrossProduct) {
  CSGVertex x(1.0f, 0.0f, 0.0f);
  CSGVertex y(0.0f, 1.0f, 0.0f);
  CSGVertex z = x.Cross(y);
  EXPECT_FLOAT_EQ(z.x, 0.0f);
  EXPECT_FLOAT_EQ(z.y, 0.0f);
  EXPECT_FLOAT_EQ(z.z, 1.0f);
}

TEST(CSGVertex, Length) {
  CSGVertex v(3.0f, 4.0f, 0.0f);
  EXPECT_FLOAT_EQ(v.Length(), 5.0f);
}

TEST(CSGVertex, Normalize) {
  CSGVertex v(3.0f, 4.0f, 0.0f);
  CSGVertex n = v.Normalized();
  EXPECT_FLOAT_EQ(n.Length(), 1.0f);
  EXPECT_FLOAT_EQ(n.x, 0.6f);
  EXPECT_FLOAT_EQ(n.y, 0.8f);
}

TEST(CSGVertex, NormalizeZeroVector) {
  CSGVertex v(0.0f, 0.0f, 0.0f);
  CSGVertex n = v.Normalized();
  EXPECT_FLOAT_EQ(n.x, 0.0f);
  EXPECT_FLOAT_EQ(n.y, 0.0f);
  EXPECT_FLOAT_EQ(n.z, 0.0f);
}

TEST(CSGVertex, NearlyEquals) {
  CSGVertex a(1.0f, 2.0f, 3.0f);
  CSGVertex b(1.0f + 1e-6f, 2.0f, 3.0f);
  EXPECT_TRUE(a.NearlyEquals(b));

  CSGVertex c(1.1f, 2.0f, 3.0f);
  EXPECT_FALSE(a.NearlyEquals(c));
}

TEST(CSGVertex, Lerp) {
  CSGVertex a(0.0f, 0.0f, 0.0f);
  CSGVertex b(10.0f, 20.0f, 30.0f);

  CSGVertex mid = CSGVertex::Lerp(a, b, 0.5f);
  EXPECT_FLOAT_EQ(mid.x, 5.0f);
  EXPECT_FLOAT_EQ(mid.y, 10.0f);
  EXPECT_FLOAT_EQ(mid.z, 15.0f);
}

// =============================================================================
// CSGPlane Tests
// =============================================================================

TEST(CSGPlane, FromPoints_ValidTriangle) {
  CSGVertex a(0.0f, 0.0f, 0.0f);
  CSGVertex b(1.0f, 0.0f, 0.0f);
  CSGVertex c(0.0f, 1.0f, 0.0f);

  auto plane = CSGPlane::FromPoints(a, b, c);
  ASSERT_TRUE(plane.has_value());

  // Normal should point up (Z direction)
  EXPECT_NEAR(plane->normal.z, 1.0f, 0.001f);
  EXPECT_FLOAT_EQ(plane->distance, 0.0f);
}

TEST(CSGPlane, FromPoints_CollinearPoints) {
  CSGVertex a(0.0f, 0.0f, 0.0f);
  CSGVertex b(1.0f, 0.0f, 0.0f);
  CSGVertex c(2.0f, 0.0f, 0.0f);

  auto plane = CSGPlane::FromPoints(a, b, c);
  EXPECT_FALSE(plane.has_value());
}

TEST(CSGPlane, DistanceTo) {
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 5.0f);

  EXPECT_FLOAT_EQ(plane.DistanceTo(CSGVertex(0.0f, 0.0f, 5.0f)), 0.0f);
  EXPECT_FLOAT_EQ(plane.DistanceTo(CSGVertex(0.0f, 0.0f, 10.0f)), 5.0f);
  EXPECT_FLOAT_EQ(plane.DistanceTo(CSGVertex(0.0f, 0.0f, 0.0f)), -5.0f);
}

TEST(CSGPlane, ClassifyPoint) {
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  EXPECT_EQ(plane.ClassifyPoint(CSGVertex(0.0f, 0.0f, 1.0f)), PlaneSide::Front);
  EXPECT_EQ(plane.ClassifyPoint(CSGVertex(0.0f, 0.0f, -1.0f)), PlaneSide::Back);
  EXPECT_EQ(plane.ClassifyPoint(CSGVertex(0.0f, 0.0f, 0.0f)), PlaneSide::On);
}

TEST(CSGPlane, Flipped) {
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 5.0f);
  CSGPlane flipped = plane.Flipped();

  EXPECT_FLOAT_EQ(flipped.normal.z, -1.0f);
  EXPECT_FLOAT_EQ(flipped.distance, -5.0f);
}

TEST(CSGPlane, Offset) {
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 5.0f);
  CSGPlane offset = plane.Offset(3.0f);

  EXPECT_FLOAT_EQ(offset.distance, 8.0f);
}

// =============================================================================
// CSGPolygon Tests
// =============================================================================

TEST(CSGPolygon, Triangle_IsValid) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(0.0f, 1.0f, 0.0f)};
  poly.ComputePlane();

  EXPECT_TRUE(poly.IsValid());
  EXPECT_TRUE(poly.IsConvex());
  EXPECT_TRUE(poly.IsTriangle());
}

TEST(CSGPolygon, Quad_IsConvex) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(1.0f, 1.0f, 0.0f),
                   CSGVertex(0.0f, 1.0f, 0.0f)};
  poly.ComputePlane();

  EXPECT_TRUE(poly.IsValid());
  EXPECT_TRUE(poly.IsConvex());
  EXPECT_FALSE(poly.IsTriangle());
}

TEST(CSGPolygon, Area_Triangle) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(2.0f, 0.0f, 0.0f), CSGVertex(0.0f, 2.0f, 0.0f)};
  poly.ComputePlane();

  EXPECT_FLOAT_EQ(poly.Area(), 2.0f); // 0.5 * base * height = 0.5 * 2 * 2 = 2
}

TEST(CSGPolygon, Centroid) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(3.0f, 0.0f, 0.0f), CSGVertex(0.0f, 3.0f, 0.0f)};
  poly.ComputePlane();

  CSGVertex center = poly.Centroid();
  EXPECT_FLOAT_EQ(center.x, 1.0f);
  EXPECT_FLOAT_EQ(center.y, 1.0f);
  EXPECT_FLOAT_EQ(center.z, 0.0f);
}

TEST(CSGPolygon, Flip) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(0.0f, 1.0f, 0.0f)};
  poly.ComputePlane();

  float original_z = poly.plane.normal.z;
  poly.Flip();

  EXPECT_FLOAT_EQ(poly.plane.normal.z, -original_z);
  // After reverse: [(0,1,0), (1,0,0), (0,0,0)]
  EXPECT_EQ(poly.vertices[0].y, 1.0f); // Was vertex[2] before flip
  EXPECT_EQ(poly.vertices[2].x, 0.0f); // Was vertex[0] before flip
}

TEST(CSGPolygon, ClassifyAgainstPlane_AllFront) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 1.0f), CSGVertex(1.0f, 0.0f, 1.0f), CSGVertex(0.0f, 1.0f, 1.0f)};
  poly.ComputePlane();

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  EXPECT_EQ(poly.ClassifyAgainstPlane(split_plane), PlaneSide::Front);
}

TEST(CSGPolygon, ClassifyAgainstPlane_AllBack) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, -1.0f), CSGVertex(1.0f, 0.0f, -1.0f), CSGVertex(0.0f, 1.0f, -1.0f)};
  poly.ComputePlane();

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  EXPECT_EQ(poly.ClassifyAgainstPlane(split_plane), PlaneSide::Back);
}

TEST(CSGPolygon, ClassifyAgainstPlane_Spanning) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, -1.0f), CSGVertex(1.0f, 0.0f, -1.0f), CSGVertex(0.0f, 1.0f, 1.0f)};
  poly.ComputePlane();

  CSGPlane split_plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  EXPECT_EQ(poly.ClassifyAgainstPlane(split_plane), PlaneSide::Spanning);
}

// =============================================================================
// CSGPolygon UV Tests
// =============================================================================

TEST(CSGPolygon, HasUVs_Empty) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0, 0, 0), CSGVertex(1, 0, 0), CSGVertex(0, 1, 0)};
  EXPECT_FALSE(poly.HasUVs());
}

TEST(CSGPolygon, HasUVs_Valid) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0, 0, 0), CSGVertex(1, 0, 0), CSGVertex(0, 1, 0)};
  poly.uvs = {UV(0, 0), UV(1, 0), UV(0, 1)};
  EXPECT_TRUE(poly.HasUVs());
}

TEST(CSGPolygon, HasUVs_Mismatch) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0, 0, 0), CSGVertex(1, 0, 0), CSGVertex(0, 1, 0)};
  poly.uvs = {UV(0, 0), UV(1, 0)}; // Only 2 UVs for 3 vertices
  EXPECT_FALSE(poly.HasUVs());
}

TEST(CSGPolygon, EnsureUVSize_ResizesToVertexCount) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0, 0, 0), CSGVertex(1, 0, 0), CSGVertex(0, 1, 0)};
  poly.EnsureUVSize();
  EXPECT_EQ(poly.uvs.size(), 3u);
  EXPECT_TRUE(poly.HasUVs());
}

TEST(CSGPolygon, EnsureUVSize_DoesNotShrink) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0, 0, 0), CSGVertex(1, 0, 0)};
  poly.uvs = {UV(0, 0), UV(1, 0), UV(0.5f, 1), UV(0.25f, 0.5f)};
  poly.EnsureUVSize();
  EXPECT_EQ(poly.uvs.size(), 4u); // Should not shrink
}

TEST(CSGPolygon, UVCentroid_Empty) {
  CSGPolygon poly;
  UV centroid = poly.UVCentroid();
  EXPECT_FLOAT_EQ(centroid.u, 0.0f);
  EXPECT_FLOAT_EQ(centroid.v, 0.0f);
}

TEST(CSGPolygon, UVCentroid_Valid) {
  CSGPolygon poly;
  poly.uvs = {UV(0, 0), UV(1, 0), UV(0, 1)};
  UV centroid = poly.UVCentroid();
  EXPECT_NEAR(centroid.u, 1.0f / 3.0f, 0.001f);
  EXPECT_NEAR(centroid.v, 1.0f / 3.0f, 0.001f);
}

TEST(CSGPolygon, Flip_ReversesUVs) {
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0, 0, 0), CSGVertex(1, 0, 0), CSGVertex(0, 1, 0)};
  poly.uvs = {UV(0, 0), UV(1, 0), UV(0, 1)};
  poly.ComputePlane();

  poly.Flip();

  // UVs should be reversed along with vertices
  EXPECT_FLOAT_EQ(poly.uvs[0].u, 0.0f);
  EXPECT_FLOAT_EQ(poly.uvs[0].v, 1.0f);
  EXPECT_FLOAT_EQ(poly.uvs[1].u, 1.0f);
  EXPECT_FLOAT_EQ(poly.uvs[1].v, 0.0f);
  EXPECT_FLOAT_EQ(poly.uvs[2].u, 0.0f);
  EXPECT_FLOAT_EQ(poly.uvs[2].v, 0.0f);
}

TEST(CSGPolygon, ConstructWithUVsAndFaceProps) {
  texture_ops::FaceProperties props("test_texture");
  props.material_id = 42;

  CSGPolygon poly({CSGVertex(0, 0, 0), CSGVertex(1, 0, 0), CSGVertex(0, 1, 0)}, {UV(0, 0), UV(1, 0), UV(0, 1)}, props);

  EXPECT_TRUE(poly.HasUVs());
  EXPECT_EQ(poly.uvs.size(), 3u);
  EXPECT_EQ(poly.material_id, 42u);
  EXPECT_EQ(poly.face_props.texture_name, "test_texture");
  EXPECT_TRUE(poly.IsValid());
}

// =============================================================================
// CSGBrush Tests
// =============================================================================

TEST(CSGBrush, FromTriangleMesh_Box) {
  // Create a simple box mesh (2 triangles per face, 6 faces = 12 triangles)
  std::vector<float> vertices = {
      // Front face
      -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
      // Back face
      -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f};
  std::vector<uint32_t> indices = {
      0, 1, 2, 0, 2, 3, // Front
      4, 5, 6, 4, 6, 7  // Back (just 2 faces for simplicity)
  };

  CSGBrush brush = CSGBrush::FromTriangleMesh(vertices, indices);
  EXPECT_TRUE(brush.IsValid());
  EXPECT_EQ(brush.PolygonCount(), 4u); // 4 triangles
}

TEST(CSGBrush, ComputeBounds) {
  CSGBrush brush;
  CSGPolygon poly;
  poly.vertices = {CSGVertex(-5.0f, -5.0f, -5.0f), CSGVertex(5.0f, -5.0f, -5.0f), CSGVertex(5.0f, 5.0f, -5.0f),
                   CSGVertex(-5.0f, 5.0f, -5.0f)};
  poly.ComputePlane();
  brush.polygons.push_back(poly);

  CSGVertex min_bound, max_bound;
  brush.ComputeBounds(min_bound, max_bound);

  EXPECT_FLOAT_EQ(min_bound.x, -5.0f);
  EXPECT_FLOAT_EQ(max_bound.x, 5.0f);
  EXPECT_FLOAT_EQ(min_bound.y, -5.0f);
  EXPECT_FLOAT_EQ(max_bound.y, 5.0f);
}

TEST(CSGBrush, Invert) {
  CSGBrush brush;
  CSGPolygon poly;
  poly.vertices = {CSGVertex(0.0f, 0.0f, 0.0f), CSGVertex(1.0f, 0.0f, 0.0f), CSGVertex(0.0f, 1.0f, 0.0f)};
  poly.ComputePlane();
  float original_z = poly.plane.normal.z;
  brush.polygons.push_back(poly);

  brush.Invert();

  EXPECT_FLOAT_EQ(brush.polygons[0].plane.normal.z, -original_z);
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST(CSGUtility, ComputeLinePlaneIntersection_Crossing) {
  CSGVertex v0(0.0f, 0.0f, -1.0f);
  CSGVertex v1(0.0f, 0.0f, 1.0f);
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  auto result = ComputeLinePlaneIntersection(v0, v1, plane);
  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->z, 0.0f, 0.001f);
}

TEST(CSGUtility, ComputeLinePlaneIntersection_Parallel) {
  CSGVertex v0(0.0f, 0.0f, 1.0f);
  CSGVertex v1(1.0f, 0.0f, 1.0f);
  CSGPlane plane(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  auto result = ComputeLinePlaneIntersection(v0, v1, plane);
  EXPECT_FALSE(result.has_value());
}

TEST(CSGUtility, ComputeThreePlaneIntersection_Valid) {
  CSGPlane p1(CSGVertex(1.0f, 0.0f, 0.0f), 1.0f); // x = 1
  CSGPlane p2(CSGVertex(0.0f, 1.0f, 0.0f), 2.0f); // y = 2
  CSGPlane p3(CSGVertex(0.0f, 0.0f, 1.0f), 3.0f); // z = 3

  auto result = ComputeThreePlaneIntersection(p1, p2, p3);
  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->x, 1.0f, 0.001f);
  EXPECT_NEAR(result->y, 2.0f, 0.001f);
  EXPECT_NEAR(result->z, 3.0f, 0.001f);
}

TEST(CSGUtility, ComputeThreePlaneIntersection_Parallel) {
  CSGPlane p1(CSGVertex(1.0f, 0.0f, 0.0f), 1.0f);
  CSGPlane p2(CSGVertex(1.0f, 0.0f, 0.0f), 2.0f); // Parallel to p1
  CSGPlane p3(CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  auto result = ComputeThreePlaneIntersection(p1, p2, p3);
  EXPECT_FALSE(result.has_value());
}

} // namespace csg
