#include "brush/texture_ops/uv_fit.h"

#include <gtest/gtest.h>

namespace texture_ops {

// =============================================================================
// ComputeUVBounds Tests
// =============================================================================

TEST(UVFit, ComputeUVBounds_EmptyVector) {
  std::vector<UV> uvs;
  UV min_uv, max_uv;
  ComputeUVBounds(uvs, min_uv, max_uv);
  EXPECT_FLOAT_EQ(min_uv.u, 0.0f);
  EXPECT_FLOAT_EQ(min_uv.v, 0.0f);
  EXPECT_FLOAT_EQ(max_uv.u, 0.0f);
  EXPECT_FLOAT_EQ(max_uv.v, 0.0f);
}

TEST(UVFit, ComputeUVBounds_SinglePoint) {
  std::vector<UV> uvs = {UV(0.5f, 0.75f)};
  UV min_uv, max_uv;
  ComputeUVBounds(uvs, min_uv, max_uv);
  EXPECT_FLOAT_EQ(min_uv.u, 0.5f);
  EXPECT_FLOAT_EQ(min_uv.v, 0.75f);
  EXPECT_FLOAT_EQ(max_uv.u, 0.5f);
  EXPECT_FLOAT_EQ(max_uv.v, 0.75f);
}

TEST(UVFit, ComputeUVBounds_MultiplePoints) {
  std::vector<UV> uvs = {
      UV(0.0f, 0.0f),
      UV(1.0f, 0.0f),
      UV(1.0f, 1.0f),
      UV(0.0f, 1.0f)
  };
  UV min_uv, max_uv;
  ComputeUVBounds(uvs, min_uv, max_uv);
  EXPECT_FLOAT_EQ(min_uv.u, 0.0f);
  EXPECT_FLOAT_EQ(min_uv.v, 0.0f);
  EXPECT_FLOAT_EQ(max_uv.u, 1.0f);
  EXPECT_FLOAT_EQ(max_uv.v, 1.0f);
}

TEST(UVFit, ComputeUVBounds_NegativeValues) {
  std::vector<UV> uvs = {
      UV(-1.0f, -2.0f),
      UV(1.0f, 2.0f)
  };
  UV min_uv, max_uv;
  ComputeUVBounds(uvs, min_uv, max_uv);
  EXPECT_FLOAT_EQ(min_uv.u, -1.0f);
  EXPECT_FLOAT_EQ(min_uv.v, -2.0f);
  EXPECT_FLOAT_EQ(max_uv.u, 1.0f);
  EXPECT_FLOAT_EQ(max_uv.v, 2.0f);
}

// =============================================================================
// NormalizePolygonUVs Tests
// =============================================================================

TEST(UVFit, NormalizePolygonUVs_ToUnitRange) {
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  polygon.uvs = {UV(0.0f, 0.0f), UV(4.0f, 0.0f), UV(4.0f, 4.0f)};

  NormalizePolygonUVs(polygon);

  EXPECT_NEAR(polygon.uvs[0].u, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[0].v, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].u, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].v, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[2].u, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[2].v, 1.0f, 1e-5f);
}

// =============================================================================
// FitPolygonUVs Tests
// =============================================================================

TEST(UVFit, FitPolygonUVs_BasicFit) {
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  polygon.uvs = {UV(0.0f, 0.0f), UV(4.0f, 0.0f), UV(4.0f, 4.0f)};

  FitTextureOptions options;
  options.maintain_aspect = false;
  options.center = false;
  options.padding = 0.0f;

  FitPolygonUVs(polygon, options);

  // UVs should now be in [0, 1] range
  UV min_uv, max_uv;
  ComputeUVBounds(polygon.uvs, min_uv, max_uv);
  EXPECT_NEAR(min_uv.u, 0.0f, 1e-4f);
  EXPECT_NEAR(min_uv.v, 0.0f, 1e-4f);
  EXPECT_NEAR(max_uv.u, 1.0f, 1e-4f);
  EXPECT_NEAR(max_uv.v, 1.0f, 1e-4f);
}

TEST(UVFit, FitPolygonUVs_WithPadding) {
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  polygon.uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(1.0f, 1.0f)};

  FitTextureOptions options;
  options.maintain_aspect = false;
  options.center = false;
  options.padding = 0.1f;

  FitPolygonUVs(polygon, options);

  UV min_uv, max_uv;
  ComputeUVBounds(polygon.uvs, min_uv, max_uv);

  // Should have padding of 0.1 on each side
  EXPECT_NEAR(min_uv.u, 0.1f, 1e-4f);
  EXPECT_NEAR(min_uv.v, 0.1f, 1e-4f);
}

TEST(UVFit, FitPolygonUVs_Centered) {
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  polygon.uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(1.0f, 1.0f)};

  FitTextureOptions options;
  options.maintain_aspect = false;
  options.center = true;
  options.padding = 0.0f;

  FitPolygonUVs(polygon, options);

  UV min_uv, max_uv;
  ComputeUVBounds(polygon.uvs, min_uv, max_uv);
  float center_u = (min_uv.u + max_uv.u) * 0.5f;
  float center_v = (min_uv.v + max_uv.v) * 0.5f;

  EXPECT_NEAR(center_u, 0.5f, 1e-4f);
  EXPECT_NEAR(center_v, 0.5f, 1e-4f);
}

// =============================================================================
// FitTextureToPolygon Tests
// =============================================================================

TEST(UVFit, FitTextureToPolygon_SquareToSquare) {
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}, {0.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  FitTextureOptions options;
  options.texture_width = 256;
  options.texture_height = 256;
  options.maintain_aspect = true;
  options.center = true;

  FitTextureToPolygon(polygon, options);

  ASSERT_EQ(polygon.uvs.size(), 4u);

  UV min_uv, max_uv;
  ComputeUVBounds(polygon.uvs, min_uv, max_uv);
  float center_u = (min_uv.u + max_uv.u) * 0.5f;
  float center_v = (min_uv.v + max_uv.v) * 0.5f;

  EXPECT_NEAR(center_u, 0.5f, 1e-4f);
  EXPECT_NEAR(center_v, 0.5f, 1e-4f);
}

// =============================================================================
// FitTextureToBrushFaces Tests
// =============================================================================

TEST(UVFit, FitTextureToBrushFaces_SelectiveFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
    polygons.emplace_back(std::move(verts));
    polygons.back().plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  }

  FitTextureOptions options;
  std::vector<uint32_t> indices = {0, 2};
  size_t modified = FitTextureToBrushFaces(polygons, indices, options);

  EXPECT_EQ(modified, 2u);
  EXPECT_FALSE(polygons[0].uvs.empty());
  EXPECT_TRUE(polygons[1].uvs.empty()); // Not modified
  EXPECT_FALSE(polygons[2].uvs.empty());
}

TEST(UVFit, FitTextureToBrushFaces_EmptyIndices) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));

  FitTextureOptions options;
  std::vector<uint32_t> indices;
  size_t modified = FitTextureToBrushFaces(polygons, indices, options);

  EXPECT_EQ(modified, 0u);
}

// =============================================================================
// FitTextureResult Tests
// =============================================================================

TEST(FitTextureResult, Success) {
  auto result = FitTextureResult::Success(5);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_modified, 5u);
}

TEST(FitTextureResult, Failure) {
  auto result = FitTextureResult::Failure("Error");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_message, "Error");
}

} // namespace texture_ops
