#include "brush/texture_ops/surface_flags.h"

#include <gtest/gtest.h>

namespace texture_ops {

// =============================================================================
// ApplyFlagsToPolygon Tests
// =============================================================================

TEST(SurfaceFlags, ApplyFlags_SetFlag) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.flags = SurfaceFlags::Solid;

  SetFlagsOptions options;
  options.flags_to_set = SurfaceFlags::Fullbright;

  ApplyFlagsToPolygon(polygon, options);

  EXPECT_TRUE(HasFlag(polygon.face_props.flags, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(polygon.face_props.flags, SurfaceFlags::Fullbright));
}

TEST(SurfaceFlags, ApplyFlags_ClearFlag) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.flags = SurfaceFlags::Solid | SurfaceFlags::Fullbright;

  SetFlagsOptions options;
  options.flags_to_clear = SurfaceFlags::Fullbright;

  ApplyFlagsToPolygon(polygon, options);

  EXPECT_TRUE(HasFlag(polygon.face_props.flags, SurfaceFlags::Solid));
  EXPECT_FALSE(HasFlag(polygon.face_props.flags, SurfaceFlags::Fullbright));
}

TEST(SurfaceFlags, ApplyFlags_ReplaceAll) {
  std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  csg::CSGPolygon polygon(std::move(verts));
  polygon.face_props.flags = SurfaceFlags::Solid | SurfaceFlags::Fullbright | SurfaceFlags::Sky;

  SetFlagsOptions options;
  options.flags_to_set = SurfaceFlags::Mirror;
  options.replace_all = true;

  ApplyFlagsToPolygon(polygon, options);

  EXPECT_EQ(polygon.face_props.flags, SurfaceFlags::Mirror);
}

// =============================================================================
// SetFlagsOnFaces Tests
// =============================================================================

TEST(SurfaceFlags, SetFlagsOnFaces_SelectiveFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 4; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = SurfaceFlags::Solid;
  }

  SetFlagsOptions options;
  options.flags_to_set = SurfaceFlags::Fullbright;

  std::vector<uint32_t> indices = {1, 3};
  size_t modified = SetFlagsOnFaces(polygons, indices, options);

  EXPECT_EQ(modified, 2u);
  EXPECT_FALSE(HasFlag(polygons[0].face_props.flags, SurfaceFlags::Fullbright));
  EXPECT_TRUE(HasFlag(polygons[1].face_props.flags, SurfaceFlags::Fullbright));
  EXPECT_FALSE(HasFlag(polygons[2].face_props.flags, SurfaceFlags::Fullbright));
  EXPECT_TRUE(HasFlag(polygons[3].face_props.flags, SurfaceFlags::Fullbright));
}

TEST(SurfaceFlags, SetFlagsOnAllFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = SurfaceFlags::Solid;
  }

  SetFlagsOptions options;
  options.flags_to_set = SurfaceFlags::Sky;

  size_t modified = SetFlagsOnAllFaces(polygons, options);

  EXPECT_EQ(modified, 3u);
  for (const auto& polygon : polygons) {
    EXPECT_TRUE(HasFlag(polygon.face_props.flags, SurfaceFlags::Sky));
  }
}

// =============================================================================
// SetFlagOnFaces / ClearFlagOnFaces / ToggleFlagOnFaces Tests
// =============================================================================

TEST(SurfaceFlags, SetFlagOnFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 2; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = SurfaceFlags::None;
  }

  std::vector<uint32_t> indices = {0, 1};
  SetFlagOnFaces(polygons, indices, SurfaceFlags::Fullbright);

  EXPECT_TRUE(HasFlag(polygons[0].face_props.flags, SurfaceFlags::Fullbright));
  EXPECT_TRUE(HasFlag(polygons[1].face_props.flags, SurfaceFlags::Fullbright));
}

TEST(SurfaceFlags, ClearFlagOnFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 2; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = SurfaceFlags::Solid | SurfaceFlags::Fullbright;
  }

  std::vector<uint32_t> indices = {0};
  ClearFlagOnFaces(polygons, indices, SurfaceFlags::Fullbright);

  EXPECT_FALSE(HasFlag(polygons[0].face_props.flags, SurfaceFlags::Fullbright));
  EXPECT_TRUE(HasFlag(polygons[1].face_props.flags, SurfaceFlags::Fullbright));
}

TEST(SurfaceFlags, ToggleFlagOnFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 2; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
  }
  polygons[0].face_props.flags = SurfaceFlags::Fullbright;
  polygons[1].face_props.flags = SurfaceFlags::None;

  std::vector<uint32_t> indices = {0, 1};
  ToggleFlagOnFaces(polygons, indices, SurfaceFlags::Fullbright);

  EXPECT_FALSE(HasFlag(polygons[0].face_props.flags, SurfaceFlags::Fullbright)); // Was on, now off
  EXPECT_TRUE(HasFlag(polygons[1].face_props.flags, SurfaceFlags::Fullbright));  // Was off, now on
}

// =============================================================================
// Alpha Reference Tests
// =============================================================================

TEST(SurfaceFlags, SetAlphaRefOnFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.alpha_ref = 0;
  }

  std::vector<uint32_t> indices = {0, 2};
  SetAlphaRefOnFaces(polygons, indices, 128);

  EXPECT_EQ(polygons[0].face_props.alpha_ref, 128);
  EXPECT_EQ(polygons[1].face_props.alpha_ref, 0);
  EXPECT_EQ(polygons[2].face_props.alpha_ref, 128);
}

TEST(SurfaceFlags, SetAlphaRefOnAllFaces) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.alpha_ref = 0;
  }

  SetAlphaRefOnAllFaces(polygons, 200);

  for (const auto& polygon : polygons) {
    EXPECT_EQ(polygon.face_props.alpha_ref, 200);
  }
}

TEST(SurfaceFlags, GetCommonAlphaRef_Same) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.alpha_ref = 128;
  }

  std::vector<uint32_t> indices = {0, 1, 2};
  EXPECT_EQ(GetCommonAlphaRef(polygons, indices), 128);
}

TEST(SurfaceFlags, GetCommonAlphaRef_Different) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.alpha_ref = static_cast<uint8_t>(i * 50);
  }

  std::vector<uint32_t> indices = {0, 1, 2};
  EXPECT_EQ(GetCommonAlphaRef(polygons, indices), -1);
}

TEST(SurfaceFlags, GetCommonAlphaRef_Empty) {
  std::vector<csg::CSGPolygon> polygons;
  std::vector<uint32_t> indices;
  EXPECT_EQ(GetCommonAlphaRef(polygons, indices), -1);
}

// =============================================================================
// GetCommonFlags / GetUnionFlags Tests
// =============================================================================

TEST(SurfaceFlags, GetCommonFlags) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
  }
  polygons[0].face_props.flags = SurfaceFlags::Solid | SurfaceFlags::Fullbright;
  polygons[1].face_props.flags = SurfaceFlags::Solid | SurfaceFlags::Sky;
  polygons[2].face_props.flags = SurfaceFlags::Solid;

  std::vector<uint32_t> indices = {0, 1, 2};
  auto common = GetCommonFlags(polygons, indices);

  EXPECT_TRUE(HasFlag(common, SurfaceFlags::Solid));
  EXPECT_FALSE(HasFlag(common, SurfaceFlags::Fullbright));
  EXPECT_FALSE(HasFlag(common, SurfaceFlags::Sky));
}

TEST(SurfaceFlags, GetUnionFlags) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
  }
  polygons[0].face_props.flags = SurfaceFlags::Solid;
  polygons[1].face_props.flags = SurfaceFlags::Fullbright;
  polygons[2].face_props.flags = SurfaceFlags::Sky;

  std::vector<uint32_t> indices = {0, 1, 2};
  auto result = GetUnionFlags(polygons, indices);

  EXPECT_TRUE(HasFlag(result, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(result, SurfaceFlags::Fullbright));
  EXPECT_TRUE(HasFlag(result, SurfaceFlags::Sky));
}

// =============================================================================
// AllFacesHaveFlag / AnyFaceHasFlag Tests
// =============================================================================

TEST(SurfaceFlags, AllFacesHaveFlag_True) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = SurfaceFlags::Solid | SurfaceFlags::Fullbright;
  }

  std::vector<uint32_t> indices = {0, 1, 2};
  EXPECT_TRUE(AllFacesHaveFlag(polygons, indices, SurfaceFlags::Solid));
  EXPECT_TRUE(AllFacesHaveFlag(polygons, indices, SurfaceFlags::Fullbright));
}

TEST(SurfaceFlags, AllFacesHaveFlag_False) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = (i == 1) ? SurfaceFlags::Solid : SurfaceFlags::None;
  }

  std::vector<uint32_t> indices = {0, 1, 2};
  EXPECT_FALSE(AllFacesHaveFlag(polygons, indices, SurfaceFlags::Solid));
}

TEST(SurfaceFlags, AnyFaceHasFlag_True) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = (i == 1) ? SurfaceFlags::Fullbright : SurfaceFlags::None;
  }

  std::vector<uint32_t> indices = {0, 1, 2};
  EXPECT_TRUE(AnyFaceHasFlag(polygons, indices, SurfaceFlags::Fullbright));
}

TEST(SurfaceFlags, AnyFaceHasFlag_False) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 3; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = SurfaceFlags::Solid;
  }

  std::vector<uint32_t> indices = {0, 1, 2};
  EXPECT_FALSE(AnyFaceHasFlag(polygons, indices, SurfaceFlags::Fullbright));
}

// =============================================================================
// FindFacesWithFlag / FindFacesWithAlphaTest Tests
// =============================================================================

TEST(SurfaceFlags, FindFacesWithFlag) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 5; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.flags = (i % 2 == 0) ? SurfaceFlags::Fullbright : SurfaceFlags::None;
  }

  auto faces = FindFacesWithFlag(polygons, SurfaceFlags::Fullbright);

  ASSERT_EQ(faces.size(), 3u);
  EXPECT_EQ(faces[0], 0u);
  EXPECT_EQ(faces[1], 2u);
  EXPECT_EQ(faces[2], 4u);
}

TEST(SurfaceFlags, FindFacesWithAlphaTest) {
  std::vector<csg::CSGPolygon> polygons;
  for (int i = 0; i < 5; ++i) {
    std::vector<csg::CSGVertex> verts = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
    polygons.emplace_back(std::move(verts));
    polygons.back().face_props.alpha_ref = (i == 1 || i == 3) ? 128 : 0;
  }

  auto faces = FindFacesWithAlphaTest(polygons);

  ASSERT_EQ(faces.size(), 2u);
  EXPECT_EQ(faces[0], 1u);
  EXPECT_EQ(faces[1], 3u);
}

// =============================================================================
// FlagsToString / StringToFlags Tests
// =============================================================================

TEST(SurfaceFlags, FlagsToString_None) {
  EXPECT_EQ(FlagsToString(SurfaceFlags::None), "None");
}

TEST(SurfaceFlags, FlagsToString_SingleFlag) {
  EXPECT_EQ(FlagsToString(SurfaceFlags::Solid), "Solid");
  EXPECT_EQ(FlagsToString(SurfaceFlags::Fullbright), "Fullbright");
  EXPECT_EQ(FlagsToString(SurfaceFlags::Sky), "Sky");
}

TEST(SurfaceFlags, FlagsToString_MultipleFlags) {
  auto str = FlagsToString(SurfaceFlags::Solid | SurfaceFlags::Fullbright);
  EXPECT_NE(str.find("Solid"), std::string::npos);
  EXPECT_NE(str.find("Fullbright"), std::string::npos);
}

TEST(SurfaceFlags, StringToFlags_Single) {
  EXPECT_EQ(StringToFlags("Solid"), SurfaceFlags::Solid);
  EXPECT_EQ(StringToFlags("fullbright"), SurfaceFlags::Fullbright); // Case insensitive
  EXPECT_EQ(StringToFlags("SKY"), SurfaceFlags::Sky);
}

TEST(SurfaceFlags, StringToFlags_Multiple) {
  auto flags = StringToFlags("Solid, Fullbright, Sky");
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Fullbright));
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Sky));
}

TEST(SurfaceFlags, StringToFlags_Nodraw) {
  // "nodraw" is an alias for Invisible
  auto flags = StringToFlags("nodraw");
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Invisible));
}

TEST(SurfaceFlags, StringToFlags_Invalid) {
  EXPECT_EQ(StringToFlags("invalid"), SurfaceFlags::None);
  EXPECT_EQ(StringToFlags(""), SurfaceFlags::None);
}

// =============================================================================
// SurfaceFlagsResult Tests
// =============================================================================

TEST(SurfaceFlagsResult, Success) {
  auto result = SurfaceFlagsResult::Success(5);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.faces_modified, 5u);
}

TEST(SurfaceFlagsResult, Failure) {
  auto result = SurfaceFlagsResult::Failure("Error");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.error_message, "Error");
}

} // namespace texture_ops
