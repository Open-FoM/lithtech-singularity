#include "brush/texture_ops/texture_replace.h"

#include <gtest/gtest.h>

namespace texture_ops {

// =============================================================================
// TextureNamesEqual Tests
// =============================================================================

TEST(TextureReplace, TextureNamesEqual_SameCase) {
  EXPECT_TRUE(TextureNamesEqual("brick_wall", "brick_wall", true));
  EXPECT_TRUE(TextureNamesEqual("brick_wall", "brick_wall", false));
}

TEST(TextureReplace, TextureNamesEqual_DifferentCase) {
  EXPECT_FALSE(TextureNamesEqual("Brick_Wall", "brick_wall", true));
  EXPECT_TRUE(TextureNamesEqual("Brick_Wall", "brick_wall", false));
}

TEST(TextureReplace, TextureNamesEqual_Different) {
  EXPECT_FALSE(TextureNamesEqual("brick_wall", "stone_floor", true));
  EXPECT_FALSE(TextureNamesEqual("brick_wall", "stone_floor", false));
}

TEST(TextureReplace, TextureNamesEqual_Empty) {
  EXPECT_TRUE(TextureNamesEqual("", "", true));
  EXPECT_FALSE(TextureNamesEqual("", "something", true));
}

// =============================================================================
// MatchesTexturePattern Tests
// =============================================================================

TEST(TextureReplace, MatchesPattern_ExactMatch) {
  EXPECT_TRUE(MatchesTexturePattern("brick_wall", "brick_wall"));
  EXPECT_FALSE(MatchesTexturePattern("brick_wall", "brick"));
}

TEST(TextureReplace, MatchesPattern_StarWildcard) {
  EXPECT_TRUE(MatchesTexturePattern("brick_wall", "brick*"));
  EXPECT_TRUE(MatchesTexturePattern("brick_wall", "*wall"));
  EXPECT_TRUE(MatchesTexturePattern("brick_wall", "brick*wall"));
  EXPECT_TRUE(MatchesTexturePattern("brick_wall", "*"));
  EXPECT_FALSE(MatchesTexturePattern("brick_wall", "stone*"));
}

TEST(TextureReplace, MatchesPattern_QuestionWildcard) {
  EXPECT_TRUE(MatchesTexturePattern("brick", "bric?"));
  EXPECT_TRUE(MatchesTexturePattern("brick", "?rick"));
  EXPECT_FALSE(MatchesTexturePattern("brick", "bric??"));
}

TEST(TextureReplace, MatchesPattern_CaseInsensitive) {
  EXPECT_TRUE(MatchesTexturePattern("BRICK_WALL", "brick*", false));
  EXPECT_FALSE(MatchesTexturePattern("BRICK_WALL", "brick*", true));
}

// =============================================================================
// ReplacePolygonTexture Tests
// =============================================================================

TEST(TextureReplace, ReplacePolygonTexture_Match) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.texture_name = "old_texture";

  ReplaceTextureOptions options;
  options.old_texture = "old_texture";
  options.new_texture = "new_texture";

  EXPECT_TRUE(ReplacePolygonTexture(polygon, options.old_texture, options.new_texture, options));
  EXPECT_EQ(polygon.face_props.texture_name, "new_texture");
}

TEST(TextureReplace, ReplacePolygonTexture_NoMatch) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.texture_name = "other_texture";

  ReplaceTextureOptions options;
  options.old_texture = "old_texture";
  options.new_texture = "new_texture";

  EXPECT_FALSE(ReplacePolygonTexture(polygon, options.old_texture, options.new_texture, options));
  EXPECT_EQ(polygon.face_props.texture_name, "other_texture");
}

TEST(TextureReplace, ReplacePolygonTexture_Pattern) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.texture_name = "brick_dirty";

  ReplaceTextureOptions options;
  options.old_texture = "brick*";
  options.new_texture = "stone_clean";
  options.use_pattern = true;

  EXPECT_TRUE(ReplacePolygonTexture(polygon, options.old_texture, options.new_texture, options));
  EXPECT_EQ(polygon.face_props.texture_name, "stone_clean");
}

TEST(TextureReplace, ReplacePolygonTexture_PreserveMapping) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.texture_name = "old_texture";
  polygon.face_props.mapping.offset_u = 0.5f;
  polygon.face_props.mapping.scale_u = 2.0f;

  ReplaceTextureOptions options;
  options.old_texture = "old_texture";
  options.new_texture = "new_texture";
  options.preserve_mapping = true;

  ReplacePolygonTexture(polygon, options.old_texture, options.new_texture, options);

  EXPECT_FLOAT_EQ(polygon.face_props.mapping.offset_u, 0.5f);
  EXPECT_FLOAT_EQ(polygon.face_props.mapping.scale_u, 2.0f);
}

TEST(TextureReplace, ReplacePolygonTexture_ResetMapping) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.texture_name = "old_texture";
  polygon.face_props.mapping.offset_u = 0.5f;
  polygon.face_props.mapping.scale_u = 2.0f;

  ReplaceTextureOptions options;
  options.old_texture = "old_texture";
  options.new_texture = "new_texture";
  options.preserve_mapping = false;

  ReplacePolygonTexture(polygon, options.old_texture, options.new_texture, options);

  EXPECT_FLOAT_EQ(polygon.face_props.mapping.offset_u, 0.0f);
  EXPECT_FLOAT_EQ(polygon.face_props.mapping.scale_u, 1.0f);
}

// =============================================================================
// ReplaceTextureInPolygons Tests
// =============================================================================

TEST(TextureReplace, ReplaceTextureInPolygons_MultipleMatches) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 5; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.texture_name = (i % 2 == 0) ? "brick" : "stone";
  }

  ReplaceTextureOptions options;
  options.old_texture = "brick";
  options.new_texture = "concrete";

  size_t modified = ReplaceTextureInPolygons(polygons, options);

  EXPECT_EQ(modified, 3u);
  EXPECT_EQ(polygons[0].face_props.texture_name, "concrete");
  EXPECT_EQ(polygons[1].face_props.texture_name, "stone");
  EXPECT_EQ(polygons[2].face_props.texture_name, "concrete");
}

TEST(TextureReplace, ReplaceTextureInPolygons_NoMatches) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  polygons.emplace_back(std::move(verts));
  polygons.back().face_props.texture_name = "stone";

  ReplaceTextureOptions options;
  options.old_texture = "brick";
  options.new_texture = "concrete";

  size_t modified = ReplaceTextureInPolygons(polygons, options);

  EXPECT_EQ(modified, 0u);
  EXPECT_EQ(polygons[0].face_props.texture_name, "stone");
}

// =============================================================================
// ReplaceTextureInFaces Tests
// =============================================================================

TEST(TextureReplace, ReplaceTextureInFaces_SelectiveFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 4; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.texture_name = "brick";
  }

  ReplaceTextureOptions options;
  options.old_texture = "brick";
  options.new_texture = "concrete";

  std::vector<uint32_t> indices = {1, 3};
  size_t modified = ReplaceTextureInFaces(polygons, indices, options);

  EXPECT_EQ(modified, 2u);
  EXPECT_EQ(polygons[0].face_props.texture_name, "brick"); // Not in indices
  EXPECT_EQ(polygons[1].face_props.texture_name, "concrete");
  EXPECT_EQ(polygons[2].face_props.texture_name, "brick"); // Not in indices
  EXPECT_EQ(polygons[3].face_props.texture_name, "concrete");
}

TEST(TextureReplace, ReplaceTextureInFaces_EmptyIndices) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  polygons.emplace_back(std::move(verts));
  polygons.back().face_props.texture_name = "brick";

  ReplaceTextureOptions options;
  options.old_texture = "brick";
  options.new_texture = "concrete";

  std::vector<uint32_t> indices;
  size_t modified = ReplaceTextureInFaces(polygons, indices, options);

  EXPECT_EQ(modified, 0u);
}

// =============================================================================
// FindFacesWithTexture Tests
// =============================================================================

TEST(TextureReplace, FindFacesWithTexture_Found) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 5; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.texture_name = (i == 1 || i == 3) ? "brick" : "stone";
  }

  auto faces = FindFacesWithTexture(polygons, "brick");

  ASSERT_EQ(faces.size(), 2u);
  EXPECT_EQ(faces[0], 1u);
  EXPECT_EQ(faces[1], 3u);
}

TEST(TextureReplace, FindFacesWithTexture_NotFound) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  polygons.emplace_back(std::move(verts));
  polygons.back().face_props.texture_name = "stone";

  auto faces = FindFacesWithTexture(polygons, "brick");

  EXPECT_TRUE(faces.empty());
}

// =============================================================================
// FindFacesMatchingPattern Tests
// =============================================================================

TEST(TextureReplace, FindFacesMatchingPattern_Wildcard) {
  std::vector<csg::CSGPolygon> polygons;
  const char* textures[] = {"brick_dirty", "brick_clean", "stone", "brick_old"};
  for (const char* tex : textures) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.texture_name = tex;
  }

  auto faces = FindFacesMatchingPattern(polygons, "brick*");

  ASSERT_EQ(faces.size(), 3u);
  EXPECT_EQ(faces[0], 0u);
  EXPECT_EQ(faces[1], 1u);
  EXPECT_EQ(faces[2], 3u);
}

// =============================================================================
// GetUniqueTextures Tests
// =============================================================================

TEST(TextureReplace, GetUniqueTextures_Returns_Sorted) {
  std::vector<csg::CSGPolygon> polygons;
  const char* textures[] = {"brick", "stone", "brick", "wood", "stone"};
  for (const char* tex : textures) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.texture_name = tex;
  }

  auto unique = GetUniqueTextures(polygons);

  ASSERT_EQ(unique.size(), 3u);
  EXPECT_EQ(unique[0], "brick");
  EXPECT_EQ(unique[1], "stone");
  EXPECT_EQ(unique[2], "wood");
}

TEST(TextureReplace, GetUniqueTextures_Empty) {
  std::vector<csg::CSGPolygon> polygons;
  auto unique = GetUniqueTextures(polygons);
  EXPECT_TRUE(unique.empty());
}

// =============================================================================
// CountFacesWithTexture Tests
// =============================================================================

TEST(TextureReplace, CountFacesWithTexture) {
  std::vector<csg::CSGPolygon> polygons;
  const char* textures[] = {"brick", "stone", "brick", "wood", "brick"};
  for (const char* tex : textures) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.texture_name = tex;
  }

  EXPECT_EQ(CountFacesWithTexture(polygons, "brick"), 3u);
  EXPECT_EQ(CountFacesWithTexture(polygons, "stone"), 1u);
  EXPECT_EQ(CountFacesWithTexture(polygons, "metal"), 0u);
}

// =============================================================================
// ReplaceTextureResult Tests
// =============================================================================

TEST(ReplaceTextureResult, Success) {
  auto result = ReplaceTextureResult::Success(5, 2);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_modified, 5u);
  EXPECT_EQ(result.brushes_modified, 2u);
}

TEST(ReplaceTextureResult, Failure) {
  auto result = ReplaceTextureResult::Failure("Error message");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_message, "Error message");
}

} // namespace texture_ops
