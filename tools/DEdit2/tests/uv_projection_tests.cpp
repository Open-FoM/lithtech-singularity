#include "brush/texture_ops/uv_projection.h"

#include <gtest/gtest.h>

namespace texture_ops {

// =============================================================================
// ComputeBestProjectionAxis Tests
// =============================================================================

TEST(UVProjection, BestAxis_XYPlane_ReturnsZ) {
  // Normal pointing up (Y) - but actually Z-facing XY plane should return Z
  csg::CSGVertex normal(0.0f, 0.0f, 1.0f);
  EXPECT_EQ(ComputeBestProjectionAxis(normal), 2);
}

TEST(UVProjection, BestAxis_XZPlane_ReturnsY) {
  csg::CSGVertex normal(0.0f, 1.0f, 0.0f);
  EXPECT_EQ(ComputeBestProjectionAxis(normal), 1);
}

TEST(UVProjection, BestAxis_YZPlane_ReturnsX) {
  csg::CSGVertex normal(1.0f, 0.0f, 0.0f);
  EXPECT_EQ(ComputeBestProjectionAxis(normal), 0);
}

TEST(UVProjection, BestAxis_DiagonalNormal) {
  // Diagonal normal with Z slightly dominant
  csg::CSGVertex normal(0.5f, 0.5f, 0.7f);
  EXPECT_EQ(ComputeBestProjectionAxis(normal), 2);

  // Diagonal normal with X dominant
  normal = csg::CSGVertex(0.8f, 0.4f, 0.4f);
  EXPECT_EQ(ComputeBestProjectionAxis(normal), 0);
}

// =============================================================================
// ComputePlanarUV Tests
// =============================================================================

TEST(UVProjection, PlanarZ_XYPlane_MapsCorrectly) {
  UV scale(0.0625f, 0.0625f); // 1/16

  csg::CSGVertex point(64.0f, 32.0f, 0.0f);
  UV uv = ComputePlanarUV(point, 2, scale);

  EXPECT_FLOAT_EQ(uv.u, 64.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uv.v, 32.0f * 0.0625f);
}

TEST(UVProjection, PlanarY_XZPlane_MapsCorrectly) {
  UV scale(1.0f, 1.0f);

  csg::CSGVertex point(10.0f, 0.0f, 20.0f);
  UV uv = ComputePlanarUV(point, 1, scale);

  EXPECT_FLOAT_EQ(uv.u, 10.0f);
  EXPECT_FLOAT_EQ(uv.v, 20.0f);
}

TEST(UVProjection, PlanarX_YZPlane_MapsCorrectly) {
  UV scale(1.0f, 1.0f);

  csg::CSGVertex point(0.0f, 15.0f, 25.0f);
  UV uv = ComputePlanarUV(point, 0, scale);

  EXPECT_FLOAT_EQ(uv.u, 15.0f);
  EXPECT_FLOAT_EQ(uv.v, 25.0f);
}

TEST(UVProjection, PlanarZ_WithScale) {
  UV scale(2.0f, 0.5f);

  csg::CSGVertex point(10.0f, 20.0f, 0.0f);
  UV uv = ComputePlanarUV(point, 2, scale);

  EXPECT_FLOAT_EQ(uv.u, 10.0f * 2.0f);
  EXPECT_FLOAT_EQ(uv.v, 20.0f * 0.5f);
}

// =============================================================================
// ComputeCylindricalUV Tests
// =============================================================================

TEST(UVProjection, Cylindrical_PointOnXAxis_HasU50Percent) {
  csg::CSGVertex center(0.0f, 0.0f, 0.0f);
  csg::CSGVertex axis(0.0f, 1.0f, 0.0f); // Y-up
  csg::CSGVertex point(64.0f, 0.0f, 0.0f); // On X axis
  UV scale(1.0f, 1.0f);

  UV uv = ComputeCylindricalUV(point, center, axis, 64.0f, scale);

  // U should be around 0.5 (depends on reference vector choice)
  // V should be 0.5 (at height 0 with radius normalization)
  EXPECT_NEAR(uv.v, 0.5f, 1e-4f);
}

TEST(UVProjection, Cylindrical_PointAtHeight_HasCorrectV) {
  csg::CSGVertex center(0.0f, 0.0f, 0.0f);
  csg::CSGVertex axis(0.0f, 1.0f, 0.0f);
  csg::CSGVertex point(64.0f, 64.0f, 0.0f); // Up from X axis
  UV scale(1.0f, 1.0f);

  UV uv = ComputeCylindricalUV(point, center, axis, 64.0f, scale);

  // V = (height/radius * 0.5 + 0.5) = (64/64 * 0.5 + 0.5) = 1.0
  EXPECT_NEAR(uv.v, 1.0f, 1e-4f);
}

// =============================================================================
// ComputeSphericalUV Tests
// =============================================================================

TEST(UVProjection, Spherical_NorthPole_HasV0OrV1) {
  csg::CSGVertex center(0.0f, 0.0f, 0.0f);
  csg::CSGVertex axis(0.0f, 1.0f, 0.0f); // Y-up
  csg::CSGVertex point(0.0f, 100.0f, 0.0f); // North pole
  UV scale(1.0f, 1.0f);

  UV uv = ComputeSphericalUV(point, center, axis, scale);

  // At north pole, latitude = pi/2, V = (pi/2)/pi + 0.5 = 1.0
  EXPECT_NEAR(uv.v, 1.0f, 1e-4f);
}

TEST(UVProjection, Spherical_SouthPole_HasV0) {
  csg::CSGVertex center(0.0f, 0.0f, 0.0f);
  csg::CSGVertex axis(0.0f, 1.0f, 0.0f);
  csg::CSGVertex point(0.0f, -100.0f, 0.0f); // South pole
  UV scale(1.0f, 1.0f);

  UV uv = ComputeSphericalUV(point, center, axis, scale);

  // At south pole, latitude = -pi/2, V = (-pi/2)/pi + 0.5 = 0.0
  EXPECT_NEAR(uv.v, 0.0f, 1e-4f);
}

TEST(UVProjection, Spherical_Equator_HasV50Percent) {
  csg::CSGVertex center(0.0f, 0.0f, 0.0f);
  csg::CSGVertex axis(0.0f, 1.0f, 0.0f);
  csg::CSGVertex point(100.0f, 0.0f, 0.0f); // On equator
  UV scale(1.0f, 1.0f);

  UV uv = ComputeSphericalUV(point, center, axis, scale);

  // At equator, latitude = 0, V = 0/pi + 0.5 = 0.5
  EXPECT_NEAR(uv.v, 0.5f, 1e-4f);
}

// =============================================================================
// ProjectPolygonUVs Tests
// =============================================================================

TEST(UVProjection, ProjectPolygonUVs_PlanarAuto) {
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f},
      {64.0f, 0.0f, 0.0f},
      {64.0f, 64.0f, 0.0f},
      {0.0f, 64.0f, 0.0f}
  };
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  ProjectionOptions options;
  options.type = ProjectionType::PlanarAuto;
  options.scale = UV(0.0625f, 0.0625f);

  ProjectPolygonUVs(polygon, options);

  ASSERT_EQ(polygon.uvs.size(), 4u);
  EXPECT_FLOAT_EQ(polygon.uvs[0].u, 0.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[0].v, 0.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[1].u, 4.0f);  // 64 * 0.0625
  EXPECT_FLOAT_EQ(polygon.uvs[1].v, 0.0f);
}

TEST(UVProjection, ProjectPolygonUVs_PlanarX) {
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f},
      {0.0f, 64.0f, 0.0f},
      {0.0f, 64.0f, 64.0f},
      {0.0f, 0.0f, 64.0f}
  };
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);

  ProjectionOptions options;
  options.type = ProjectionType::PlanarX;
  options.scale = UV(1.0f, 1.0f);

  ProjectPolygonUVs(polygon, options);

  ASSERT_EQ(polygon.uvs.size(), 4u);
  // Planar X projects onto YZ plane
  EXPECT_FLOAT_EQ(polygon.uvs[0].u, 0.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[0].v, 0.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[1].u, 64.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[1].v, 0.0f);
}

// =============================================================================
// ProjectBrushFaceUVs Tests
// =============================================================================

TEST(UVProjection, ProjectBrushFaceUVs_SelectiveFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
    polygons.emplace_back(std::move(verts));
    polygons.back().plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  }

  ProjectionOptions options;
  options.type = ProjectionType::PlanarZ;
  options.scale = UV(1.0f, 1.0f);

  std::vector<uint32_t> indices = {0, 2};
  size_t modified = ProjectBrushFaceUVs(polygons, indices, options);

  EXPECT_EQ(modified, 2u);
  EXPECT_FALSE(polygons[0].uvs.empty());
  EXPECT_TRUE(polygons[1].uvs.empty()); // Not modified
  EXPECT_FALSE(polygons[2].uvs.empty());
}

TEST(UVProjection, ProjectBrushFaceUVs_EmptyIndices) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));

  ProjectionOptions options;
  std::vector<uint32_t> indices;
  size_t modified = ProjectBrushFaceUVs(polygons, indices, options);

  EXPECT_EQ(modified, 0u);
}

// =============================================================================
// ProjectionResult Tests
// =============================================================================

TEST(ProjectionResult, Success) {
  auto result = ProjectionResult::Success(5);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_modified, 5u);
}

TEST(ProjectionResult, Failure) {
  auto result = ProjectionResult::Failure("Error");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_message, "Error");
}

} // namespace texture_ops
