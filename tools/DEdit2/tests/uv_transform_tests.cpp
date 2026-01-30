#include "brush/texture_ops/uv_transform.h"

#include <gtest/gtest.h>

namespace texture_ops {

// =============================================================================
// UVTransform Tests
// =============================================================================

TEST(UVTransform, DefaultIsIdentity) {
  UVTransform transform;
  EXPECT_TRUE(transform.IsIdentity());
}

TEST(UVTransform, WithOffset_NotIdentity) {
  UVTransform transform;
  transform.offset = UV(0.1f, 0.0f);
  EXPECT_FALSE(transform.IsIdentity());
}

TEST(UVTransform, WithScale_NotIdentity) {
  UVTransform transform;
  transform.scale = UV(2.0f, 1.0f);
  EXPECT_FALSE(transform.IsIdentity());
}

TEST(UVTransform, WithRotation_NotIdentity) {
  UVTransform transform;
  transform.rotation = 45.0f;
  EXPECT_FALSE(transform.IsIdentity());
}

TEST(UVTransform, Apply_Identity) {
  UVTransform transform;
  UV input(0.25f, 0.75f);
  UV output = transform.Apply(input);
  EXPECT_NEAR(output.u, 0.25f, 1e-5f);
  EXPECT_NEAR(output.v, 0.75f, 1e-5f);
}

TEST(UVTransform, Apply_OffsetOnly) {
  UVTransform transform;
  transform.offset = UV(0.5f, 0.25f);
  UV input(0.0f, 0.0f);
  UV output = transform.Apply(input);
  EXPECT_NEAR(output.u, 0.5f, 1e-5f);
  EXPECT_NEAR(output.v, 0.25f, 1e-5f);
}

TEST(UVTransform, Apply_ScaleOnly) {
  UVTransform transform;
  transform.scale = UV(2.0f, 0.5f);
  transform.pivot = UV(0.0f, 0.0f); // Scale from origin
  UV input(1.0f, 1.0f);
  UV output = transform.Apply(input);
  EXPECT_NEAR(output.u, 2.0f, 1e-5f);
  EXPECT_NEAR(output.v, 0.5f, 1e-5f);
}

TEST(UVTransform, Apply_Rotation90) {
  UVTransform transform;
  transform.rotation = 90.0f;
  transform.pivot = UV(0.0f, 0.0f); // Rotate around origin
  UV input(1.0f, 0.0f);
  UV output = transform.Apply(input);
  EXPECT_NEAR(output.u, 0.0f, 1e-5f);
  EXPECT_NEAR(output.v, 1.0f, 1e-5f);
}

TEST(UVTransform, Apply_Rotation180) {
  UVTransform transform;
  transform.rotation = 180.0f;
  transform.pivot = UV(0.0f, 0.0f);
  UV input(1.0f, 0.0f);
  UV output = transform.Apply(input);
  EXPECT_NEAR(output.u, -1.0f, 1e-5f);
  EXPECT_NEAR(output.v, 0.0f, 1e-5f);
}

TEST(UVTransform, Apply_RotationAroundPivot) {
  UVTransform transform;
  transform.rotation = 90.0f;
  transform.pivot = UV(0.5f, 0.5f);
  UV input(1.0f, 0.5f); // Point to the right of center
  UV output = transform.Apply(input);
  EXPECT_NEAR(output.u, 0.5f, 1e-5f);  // Now at center x
  EXPECT_NEAR(output.v, 1.0f, 1e-5f);  // Rotated up
}

TEST(UVTransform, Apply_Combined) {
  UVTransform transform;
  transform.offset = UV(0.1f, 0.1f);
  transform.scale = UV(2.0f, 2.0f);
  transform.pivot = UV(0.0f, 0.0f);
  UV input(0.5f, 0.5f);
  UV output = transform.Apply(input);
  // Scale then offset: (0.5 * 2, 0.5 * 2) + (0.1, 0.1) = (1.1, 1.1)
  EXPECT_NEAR(output.u, 1.1f, 1e-5f);
  EXPECT_NEAR(output.v, 1.1f, 1e-5f);
}

// =============================================================================
// FlipUV Tests
// =============================================================================

TEST(UVTransform, FlipUVHorizontal_AroundCenter) {
  UV input(0.25f, 0.5f);
  UV output = FlipUVHorizontal(input, 0.5f);
  EXPECT_NEAR(output.u, 0.75f, 1e-5f);
  EXPECT_NEAR(output.v, 0.5f, 1e-5f);
}

TEST(UVTransform, FlipUVHorizontal_AtCenter) {
  UV input(0.5f, 0.5f);
  UV output = FlipUVHorizontal(input, 0.5f);
  EXPECT_NEAR(output.u, 0.5f, 1e-5f);
  EXPECT_NEAR(output.v, 0.5f, 1e-5f);
}

TEST(UVTransform, FlipUVVertical_AroundCenter) {
  UV input(0.5f, 0.25f);
  UV output = FlipUVVertical(input, 0.5f);
  EXPECT_NEAR(output.u, 0.5f, 1e-5f);
  EXPECT_NEAR(output.v, 0.75f, 1e-5f);
}

TEST(UVTransform, FlipUVVertical_AtCenter) {
  UV input(0.5f, 0.5f);
  UV output = FlipUVVertical(input, 0.5f);
  EXPECT_NEAR(output.u, 0.5f, 1e-5f);
  EXPECT_NEAR(output.v, 0.5f, 1e-5f);
}

// =============================================================================
// TransformPolygonUVs Tests
// =============================================================================

TEST(UVTransform, TransformPolygonUVs_Identity) {
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(1.0f, 1.0f)};

  UVTransform transform; // Identity
  TransformPolygonUVs(polygon, transform);

  // Should be unchanged
  EXPECT_NEAR(polygon.uvs[0].u, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[0].v, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].u, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].v, 0.0f, 1e-5f);
}

TEST(UVTransform, TransformPolygonUVs_WithOffset) {
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(1.0f, 1.0f)};

  UVTransform transform;
  transform.offset = UV(0.5f, 0.25f);
  TransformPolygonUVs(polygon, transform);

  EXPECT_NEAR(polygon.uvs[0].u, 0.5f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[0].v, 0.25f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].u, 1.5f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].v, 0.25f, 1e-5f);
}

// =============================================================================
// TransformBrushFaceUVs Tests
// =============================================================================

TEST(UVTransform, TransformBrushFaceUVs_SelectiveFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
    polygons.emplace_back(std::move(verts));
    polygons.back().uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(0.5f, 1.0f)};
  }

  UVTransform transform;
  transform.offset = UV(10.0f, 10.0f);
  std::vector<uint32_t> indices = {0, 2}; // Transform first and third only

  size_t modified = TransformBrushFaceUVs(polygons, indices, transform);

  EXPECT_EQ(modified, 2u);
  EXPECT_NEAR(polygons[0].uvs[0].u, 10.0f, 1e-5f); // Modified
  EXPECT_NEAR(polygons[1].uvs[0].u, 0.0f, 1e-5f);  // Not modified
  EXPECT_NEAR(polygons[2].uvs[0].u, 10.0f, 1e-5f); // Modified
}

TEST(UVTransform, TransformBrushFaceUVs_Identity) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));
  polygons.back().uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(0.5f, 1.0f)};

  UVTransform transform; // Identity
  size_t modified = TransformBrushFaceUVs(polygons, {0}, transform);

  EXPECT_EQ(modified, 0u); // Identity should modify nothing
}

// =============================================================================
// FlipUVs Tests
// =============================================================================

TEST(UVTransform, FlipUVsHorizontal_FlipsAroundCentroid) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));
  // UVs from 0 to 1
  polygons.back().uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(1.0f, 1.0f)};

  size_t modified = FlipUVsHorizontal(polygons, {0});

  EXPECT_EQ(modified, 1u);
  // Centroid U = (0 + 1 + 1) / 3 = 0.6667
  // Flip: 2 * 0.6667 - u
  float centroid_u = (0.0f + 1.0f + 1.0f) / 3.0f;
  EXPECT_NEAR(polygons[0].uvs[0].u, 2.0f * centroid_u - 0.0f, 1e-4f);
  EXPECT_NEAR(polygons[0].uvs[1].u, 2.0f * centroid_u - 1.0f, 1e-4f);
  EXPECT_NEAR(polygons[0].uvs[2].u, 2.0f * centroid_u - 1.0f, 1e-4f);
}

TEST(UVTransform, FlipUVsVertical_FlipsAroundCentroid) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));
  polygons.back().uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(1.0f, 1.0f)};

  size_t modified = FlipUVsVertical(polygons, {0});

  EXPECT_EQ(modified, 1u);
  // Centroid V = (0 + 0 + 1) / 3 = 0.3333
  float centroid_v = (0.0f + 0.0f + 1.0f) / 3.0f;
  EXPECT_NEAR(polygons[0].uvs[0].v, 2.0f * centroid_v - 0.0f, 1e-4f);
  EXPECT_NEAR(polygons[0].uvs[1].v, 2.0f * centroid_v - 0.0f, 1e-4f);
  EXPECT_NEAR(polygons[0].uvs[2].v, 2.0f * centroid_v - 1.0f, 1e-4f);
}

// =============================================================================
// ResetFaceUVs Tests
// =============================================================================

TEST(UVTransform, ResetFaceUVs_ResetsToDefault) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}};
  polygons.emplace_back(std::move(verts));
  polygons.back().plane = csg::CSGPlane(csg::CSGVertex(0.0f, 0.0f, 1.0f), 0.0f);
  polygons.back().uvs = {UV(99.0f, 99.0f), UV(99.0f, 99.0f), UV(99.0f, 99.0f)};

  size_t modified = ResetFaceUVs(polygons, {0}, 1.0f);

  EXPECT_EQ(modified, 1u);
  // Should regenerate planar UVs (Z-dominant, XY projection)
  EXPECT_NEAR(polygons[0].uvs[0].u, 0.0f, 1e-5f);
  EXPECT_NEAR(polygons[0].uvs[0].v, 0.0f, 1e-5f);
  EXPECT_NEAR(polygons[0].uvs[1].u, 64.0f, 1e-5f);
  EXPECT_NEAR(polygons[0].uvs[1].v, 0.0f, 1e-5f);
}

// =============================================================================
// RotatePolygonUVs Tests
// =============================================================================

TEST(UVTransform, RotatePolygonUVs_90Degrees) {
  std::vector<csg::CSGVertex> verts = {
      {0.0f, 0.0f, 0.0f}, {64.0f, 0.0f, 0.0f}, {64.0f, 64.0f, 0.0f}, {0.0f, 64.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  // Square UVs
  polygon.uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(1.0f, 1.0f), UV(0.0f, 1.0f)};

  RotatePolygonUVs(polygon, 90.0f);

  // Centroid is (0.5, 0.5)
  // (0,0) rotated 90 around (0.5,0.5) -> (0.5 - 0.5, 0.5 + (-0.5)) = (0, 0) -> rotated = (-0.5, 0.5) + (0.5, 0.5) = (0, 1)
  // Actually: u' = (u - 0.5) * cos(90) - (v - 0.5) * sin(90) + 0.5 = (v - 0.5) + 0.5 = v (wait, cos90=0, sin90=1)
  // u' = -(v - 0.5) + 0.5 = 0.5 - v + 0.5 = 1 - v
  // Hmm, let me recalculate: cos(90) = 0, sin(90) = 1
  // u' = (u-p.u)*cos - (v-p.v)*sin + p.u = 0 - (v - 0.5) * 1 + 0.5 = -v + 0.5 + 0.5 = 1 - v
  // v' = (u-p.u)*sin + (v-p.v)*cos + p.v = (u - 0.5) * 1 + 0 + 0.5 = u
  // So (0,0) -> (1-0, 0) = (1, 0)
  // (1,0) -> (1-0, 1) = (1, 1)
  // (1,1) -> (1-1, 1) = (0, 1)
  // (0,1) -> (1-1, 0) = (0, 0)
  EXPECT_NEAR(polygon.uvs[0].u, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[0].v, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].u, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].v, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[2].u, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[2].v, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[3].u, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[3].v, 0.0f, 1e-5f);
}

TEST(UVTransform, RotatePolygonUVs_ZeroDegrees) {
  std::vector<csg::CSGVertex> verts = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.uvs = {UV(0.0f, 0.0f), UV(1.0f, 0.0f), UV(0.5f, 1.0f)};

  RotatePolygonUVs(polygon, 0.0f);

  // Should be unchanged
  EXPECT_NEAR(polygon.uvs[0].u, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[0].v, 0.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].u, 1.0f, 1e-5f);
  EXPECT_NEAR(polygon.uvs[1].v, 0.0f, 1e-5f);
}

// =============================================================================
// UVTransformResult Tests
// =============================================================================

TEST(UVTransformResult, Success) {
  auto result = UVTransformResult::Success(10);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_modified, 10u);
  EXPECT_TRUE(result.error_message.empty());
}

TEST(UVTransformResult, Failure) {
  auto result = UVTransformResult::Failure("Test error");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.faces_modified, 0u);
  EXPECT_EQ(result.error_message, "Test error");
}

} // namespace texture_ops
