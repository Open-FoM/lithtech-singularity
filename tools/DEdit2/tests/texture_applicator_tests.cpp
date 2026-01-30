#include "brush/texture_ops/texture_applicator.h"

#include <gtest/gtest.h>

namespace texture_ops {

// =============================================================================
// GetBestProjectionAxis Tests
// =============================================================================

TEST(TextureApplicator, GetBestProjectionAxis_XDominant) {
  // Normal pointing along X-axis should project onto YZ plane
  csg::CSGVertex normal(1.0f, 0.0f, 0.0f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 0);

  // Negative X-axis
  normal = csg::CSGVertex(-1.0f, 0.0f, 0.0f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 0);

  // X dominant with small Y/Z components
  normal = csg::CSGVertex(0.9f, 0.3f, 0.2f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 0);
}

TEST(TextureApplicator, GetBestProjectionAxis_YDominant) {
  // Normal pointing along Y-axis should project onto XZ plane
  csg::CSGVertex normal(0.0f, 1.0f, 0.0f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 1);

  // Negative Y-axis
  normal = csg::CSGVertex(0.0f, -1.0f, 0.0f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 1);

  // Y dominant with small X/Z components
  normal = csg::CSGVertex(0.2f, 0.9f, 0.3f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 1);
}

TEST(TextureApplicator, GetBestProjectionAxis_ZDominant) {
  // Normal pointing along Z-axis should project onto XY plane
  csg::CSGVertex normal(0.0f, 0.0f, 1.0f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 2);

  // Negative Z-axis
  normal = csg::CSGVertex(0.0f, 0.0f, -1.0f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 2);

  // Z dominant with small X/Y components
  normal = csg::CSGVertex(0.2f, 0.3f, 0.9f);
  EXPECT_EQ(GetBestProjectionAxis(normal), 2);
}

// =============================================================================
// GeneratePlanarUVs Tests
// =============================================================================

TEST(TextureApplicator, GeneratePlanarUVs_EmptyPolygon) {
  csg::CSGPolygon polygon;
  auto uvs = GeneratePlanarUVs(polygon);
  EXPECT_TRUE(uvs.empty());
}

TEST(TextureApplicator, GeneratePlanarUVs_HorizontalPlane) {
  // Create a square on the XZ plane (floor)
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f},
      {64.0f, 0.0f, 0.0f},
      {64.0f, 0.0f, 64.0f},
      {0.0f, 0.0f, 64.0f}
  };
  csg::CSGPolygon polygon(std::move(verts));
  // Y-up normal for floor
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 1.0f, 0.0f), 0.0f);

  auto uvs = GeneratePlanarUVs(polygon, 0.0625f); // 1/16 scale

  ASSERT_EQ(uvs.size(), 4u);

  // Y-dominant, project onto XZ plane
  // UV should be (x * scale, z * scale)
  EXPECT_FLOAT_EQ(uvs[0].u, 0.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uvs[0].v, 0.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uvs[1].u, 64.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uvs[1].v, 0.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uvs[2].u, 64.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uvs[2].v, 64.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uvs[3].u, 0.0f * 0.0625f);
  EXPECT_FLOAT_EQ(uvs[3].v, 64.0f * 0.0625f);
}

TEST(TextureApplicator, GeneratePlanarUVs_VerticalWallXY) {
  // Create a wall on the XY plane (facing Z)
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f},
      {64.0f, 0.0f, 0.0f},
      {64.0f, 64.0f, 0.0f},
      {0.0f, 64.0f, 0.0f}
  };
  csg::CSGPolygon polygon(std::move(verts));
  // Z-forward normal for wall
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  auto uvs = GeneratePlanarUVs(polygon, 1.0f);

  ASSERT_EQ(uvs.size(), 4u);

  // Z-dominant, project onto XY plane
  // UV should be (x, y)
  EXPECT_FLOAT_EQ(uvs[0].u, 0.0f);
  EXPECT_FLOAT_EQ(uvs[0].v, 0.0f);
  EXPECT_FLOAT_EQ(uvs[1].u, 64.0f);
  EXPECT_FLOAT_EQ(uvs[1].v, 0.0f);
  EXPECT_FLOAT_EQ(uvs[2].u, 64.0f);
  EXPECT_FLOAT_EQ(uvs[2].v, 64.0f);
  EXPECT_FLOAT_EQ(uvs[3].u, 0.0f);
  EXPECT_FLOAT_EQ(uvs[3].v, 64.0f);
}

TEST(TextureApplicator, GeneratePlanarUVs_VerticalWallYZ) {
  // Create a wall on the YZ plane (facing X)
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 64.0f},
      {0.0f, 64.0f, 64.0f},
      {0.0f, 64.0f, 0.0f}
  };
  csg::CSGPolygon polygon(std::move(verts));
  // X-forward normal for wall
  polygon.plane = csg::CSGPlane(csg::CSGVertex(1.0f, 0.0f, 0.0f), 0.0f);

  auto uvs = GeneratePlanarUVs(polygon, 1.0f);

  ASSERT_EQ(uvs.size(), 4u);

  // X-dominant, project onto YZ plane
  // UV should be (y, z)
  EXPECT_FLOAT_EQ(uvs[0].u, 0.0f);
  EXPECT_FLOAT_EQ(uvs[0].v, 0.0f);
  EXPECT_FLOAT_EQ(uvs[1].u, 0.0f);
  EXPECT_FLOAT_EQ(uvs[1].v, 64.0f);
  EXPECT_FLOAT_EQ(uvs[2].u, 64.0f);
  EXPECT_FLOAT_EQ(uvs[2].v, 64.0f);
  EXPECT_FLOAT_EQ(uvs[3].u, 64.0f);
  EXPECT_FLOAT_EQ(uvs[3].v, 0.0f);
}

// =============================================================================
// ApplyTextureToPolygon Tests
// =============================================================================

TEST(TextureApplicator, ApplyTextureToPolygon_SetsTextureName) {
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f},
      {64.0f, 0.0f, 0.0f},
      {32.0f, 0.0f, 64.0f}
  };
  csg::CSGPolygon polygon(std::move(verts));

  ApplyTextureToPolygon(polygon, "brick_wall");

  EXPECT_EQ(polygon.face_props.texture_name, "brick_wall");
  EXPECT_EQ(polygon.uvs.size(), 3u);
}

TEST(TextureApplicator, ApplyTextureToPolygon_GeneratesUVs) {
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f},
      {64.0f, 0.0f, 0.0f},
      {64.0f, 64.0f, 0.0f}
  };
  csg::CSGPolygon polygon(std::move(verts));
  polygon.plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  ApplyTextureToPolygon(polygon, "stone", 0.5f);

  EXPECT_EQ(polygon.face_props.texture_name, "stone");
  ASSERT_EQ(polygon.uvs.size(), 3u);

  // Z-dominant, XY projection with 0.5 scale
  EXPECT_FLOAT_EQ(polygon.uvs[0].u, 0.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[0].v, 0.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[1].u, 32.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[1].v, 0.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[2].u, 32.0f);
  EXPECT_FLOAT_EQ(polygon.uvs[2].v, 32.0f);
}

// =============================================================================
// ApplyTextureToBrushFaces Tests
// =============================================================================

TEST(TextureApplicator, ApplyTextureToBrushFaces_AppliesSelectedFaces) {
  std::vector<csg::CSGPolygon> polygons;

  // Create 3 simple triangles
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.5f, 1.0f, 0.0f}
    };
    polygons.emplace_back(std::move(verts));
    polygons.back().plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  }

  // Apply to first and third faces only
  std::vector<uint32_t> indices = {0, 2};
  size_t modified = ApplyTextureToBrushFaces(polygons, indices, "metal");

  EXPECT_EQ(modified, 2u);
  EXPECT_EQ(polygons[0].face_props.texture_name, "metal");
  EXPECT_TRUE(polygons[1].face_props.texture_name.empty());
  EXPECT_EQ(polygons[2].face_props.texture_name, "metal");
}

TEST(TextureApplicator, ApplyTextureToBrushFaces_IgnoresInvalidIndices) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));
  polygons.back().plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);

  std::vector<uint32_t> indices = {0, 5, 10}; // 5 and 10 are invalid
  size_t modified = ApplyTextureToBrushFaces(polygons, indices, "test");

  EXPECT_EQ(modified, 1u);
}

TEST(TextureApplicator, ApplyTextureToBrushFaces_EmptyIndices) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));

  std::vector<uint32_t> indices;
  size_t modified = ApplyTextureToBrushFaces(polygons, indices, "test");

  EXPECT_EQ(modified, 0u);
}

// =============================================================================
// ApplyTextureToAllFaces Tests
// =============================================================================

TEST(TextureApplicator, ApplyTextureToAllFaces_AppliesAll) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 5; ++i) {
    std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
    polygons.emplace_back(std::move(verts));
    polygons.back().plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  }

  size_t modified = ApplyTextureToAllFaces(polygons, "all_tex");

  EXPECT_EQ(modified, 5u);
  for (const auto& poly : polygons) {
    EXPECT_EQ(poly.face_props.texture_name, "all_tex");
    EXPECT_FALSE(poly.uvs.empty());
  }
}

TEST(TextureApplicator, ApplyTextureToAllFaces_EmptyBrush) {
  std::vector<csg::CSGPolygon> polygons;
  size_t modified = ApplyTextureToAllFaces(polygons, "test");
  EXPECT_EQ(modified, 0u);
}

// =============================================================================
// ApplyTextureResult Tests
// =============================================================================

TEST(ApplyTextureResult, Success) {
  auto result = ApplyTextureResult::Success(5);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_modified, 5u);
  EXPECT_TRUE(result.error_message.empty());
}

TEST(ApplyTextureResult, Failure) {
  auto result = ApplyTextureResult::Failure("Test error");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.faces_modified, 0u);
  EXPECT_EQ(result.error_message, "Test error");
}

} // namespace texture_ops
