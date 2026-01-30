#include "brush/texture_ops/uv_types.h"

#include <gtest/gtest.h>

namespace texture_ops {

// =============================================================================
// UV Tests
// =============================================================================

TEST(UV, DefaultConstruction) {
  UV uv;
  EXPECT_FLOAT_EQ(uv.u, 0.0f);
  EXPECT_FLOAT_EQ(uv.v, 0.0f);
}

TEST(UV, ParameterizedConstruction) {
  UV uv(0.5f, 0.75f);
  EXPECT_FLOAT_EQ(uv.u, 0.5f);
  EXPECT_FLOAT_EQ(uv.v, 0.75f);
}

TEST(UV, Addition) {
  UV a(1.0f, 2.0f);
  UV b(3.0f, 4.0f);
  UV c = a + b;
  EXPECT_FLOAT_EQ(c.u, 4.0f);
  EXPECT_FLOAT_EQ(c.v, 6.0f);
}

TEST(UV, Subtraction) {
  UV a(5.0f, 7.0f);
  UV b(2.0f, 3.0f);
  UV c = a - b;
  EXPECT_FLOAT_EQ(c.u, 3.0f);
  EXPECT_FLOAT_EQ(c.v, 4.0f);
}

TEST(UV, ScalarMultiplication) {
  UV a(2.0f, 3.0f);
  UV b = a * 2.0f;
  EXPECT_FLOAT_EQ(b.u, 4.0f);
  EXPECT_FLOAT_EQ(b.v, 6.0f);
}

TEST(UV, ScalarDivision) {
  UV a(4.0f, 6.0f);
  UV b = a / 2.0f;
  EXPECT_FLOAT_EQ(b.u, 2.0f);
  EXPECT_FLOAT_EQ(b.v, 3.0f);
}

TEST(UV, Negation) {
  UV a(1.0f, -2.0f);
  UV b = -a;
  EXPECT_FLOAT_EQ(b.u, -1.0f);
  EXPECT_FLOAT_EQ(b.v, 2.0f);
}

TEST(UV, CompoundAddition) {
  UV a(1.0f, 2.0f);
  a += UV(3.0f, 4.0f);
  EXPECT_FLOAT_EQ(a.u, 4.0f);
  EXPECT_FLOAT_EQ(a.v, 6.0f);
}

TEST(UV, CompoundSubtraction) {
  UV a(5.0f, 7.0f);
  a -= UV(2.0f, 3.0f);
  EXPECT_FLOAT_EQ(a.u, 3.0f);
  EXPECT_FLOAT_EQ(a.v, 4.0f);
}

TEST(UV, CompoundScalarMultiplication) {
  UV a(2.0f, 3.0f);
  a *= 2.0f;
  EXPECT_FLOAT_EQ(a.u, 4.0f);
  EXPECT_FLOAT_EQ(a.v, 6.0f);
}

TEST(UV, Lerp_AtZero) {
  UV a(0.0f, 0.0f);
  UV b(1.0f, 2.0f);
  UV result = UV::Lerp(a, b, 0.0f);
  EXPECT_FLOAT_EQ(result.u, 0.0f);
  EXPECT_FLOAT_EQ(result.v, 0.0f);
}

TEST(UV, Lerp_AtOne) {
  UV a(0.0f, 0.0f);
  UV b(1.0f, 2.0f);
  UV result = UV::Lerp(a, b, 1.0f);
  EXPECT_FLOAT_EQ(result.u, 1.0f);
  EXPECT_FLOAT_EQ(result.v, 2.0f);
}

TEST(UV, Lerp_AtHalf) {
  UV a(0.0f, 0.0f);
  UV b(1.0f, 2.0f);
  UV result = UV::Lerp(a, b, 0.5f);
  EXPECT_FLOAT_EQ(result.u, 0.5f);
  EXPECT_FLOAT_EQ(result.v, 1.0f);
}

TEST(UV, Lerp_WithNegatives) {
  UV a(-1.0f, -2.0f);
  UV b(1.0f, 2.0f);
  UV result = UV::Lerp(a, b, 0.5f);
  EXPECT_FLOAT_EQ(result.u, 0.0f);
  EXPECT_FLOAT_EQ(result.v, 0.0f);
}

TEST(UV, NearlyEquals_SameValues) {
  UV a(1.0f, 2.0f);
  UV b(1.0f, 2.0f);
  EXPECT_TRUE(a.NearlyEquals(b));
}

TEST(UV, NearlyEquals_WithinEpsilon) {
  UV a(1.0f, 2.0f);
  UV b(1.0f + 1e-6f, 2.0f - 1e-6f);
  EXPECT_TRUE(a.NearlyEquals(b));
}

TEST(UV, NearlyEquals_OutsideEpsilon) {
  UV a(1.0f, 2.0f);
  UV b(1.1f, 2.0f);
  EXPECT_FALSE(a.NearlyEquals(b));
}

// =============================================================================
// TextureMapping Tests
// =============================================================================

TEST(TextureMapping, DefaultConstruction) {
  TextureMapping mapping;
  EXPECT_FLOAT_EQ(mapping.offset_u, 0.0f);
  EXPECT_FLOAT_EQ(mapping.offset_v, 0.0f);
  EXPECT_FLOAT_EQ(mapping.scale_u, 1.0f);
  EXPECT_FLOAT_EQ(mapping.scale_v, 1.0f);
  EXPECT_FLOAT_EQ(mapping.rotation, 0.0f);
}

TEST(TextureMapping, ParameterizedConstruction) {
  TextureMapping mapping(1.0f, 2.0f, 0.5f, 0.25f, 45.0f);
  EXPECT_FLOAT_EQ(mapping.offset_u, 1.0f);
  EXPECT_FLOAT_EQ(mapping.offset_v, 2.0f);
  EXPECT_FLOAT_EQ(mapping.scale_u, 0.5f);
  EXPECT_FLOAT_EQ(mapping.scale_v, 0.25f);
  EXPECT_FLOAT_EQ(mapping.rotation, 45.0f);
}

TEST(TextureMapping, DefaultIsIdentity) {
  TextureMapping mapping;
  EXPECT_TRUE(mapping.IsIdentity());
}

TEST(TextureMapping, NonZeroOffsetIsNotIdentity) {
  TextureMapping mapping(1.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  EXPECT_FALSE(mapping.IsIdentity());
}

TEST(TextureMapping, NonUnityScaleIsNotIdentity) {
  TextureMapping mapping(0.0f, 0.0f, 2.0f, 1.0f, 0.0f);
  EXPECT_FALSE(mapping.IsIdentity());
}

TEST(TextureMapping, NonZeroRotationIsNotIdentity) {
  TextureMapping mapping(0.0f, 0.0f, 1.0f, 1.0f, 45.0f);
  EXPECT_FALSE(mapping.IsIdentity());
}

TEST(TextureMapping, Lerp_AtZero) {
  TextureMapping a(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  TextureMapping b(10.0f, 20.0f, 2.0f, 3.0f, 90.0f);
  TextureMapping result = TextureMapping::Lerp(a, b, 0.0f);
  EXPECT_FLOAT_EQ(result.offset_u, 0.0f);
  EXPECT_FLOAT_EQ(result.offset_v, 0.0f);
  EXPECT_FLOAT_EQ(result.scale_u, 1.0f);
  EXPECT_FLOAT_EQ(result.scale_v, 1.0f);
  EXPECT_FLOAT_EQ(result.rotation, 0.0f);
}

TEST(TextureMapping, Lerp_AtOne) {
  TextureMapping a(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  TextureMapping b(10.0f, 20.0f, 2.0f, 3.0f, 90.0f);
  TextureMapping result = TextureMapping::Lerp(a, b, 1.0f);
  EXPECT_FLOAT_EQ(result.offset_u, 10.0f);
  EXPECT_FLOAT_EQ(result.offset_v, 20.0f);
  EXPECT_FLOAT_EQ(result.scale_u, 2.0f);
  EXPECT_FLOAT_EQ(result.scale_v, 3.0f);
  EXPECT_FLOAT_EQ(result.rotation, 90.0f);
}

TEST(TextureMapping, Lerp_AtHalf) {
  TextureMapping a(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  TextureMapping b(10.0f, 20.0f, 2.0f, 3.0f, 90.0f);
  TextureMapping result = TextureMapping::Lerp(a, b, 0.5f);
  EXPECT_FLOAT_EQ(result.offset_u, 5.0f);
  EXPECT_FLOAT_EQ(result.offset_v, 10.0f);
  EXPECT_FLOAT_EQ(result.scale_u, 1.5f);
  EXPECT_FLOAT_EQ(result.scale_v, 2.0f);
  EXPECT_FLOAT_EQ(result.rotation, 45.0f);
}

TEST(TextureMapping, Apply_Identity) {
  TextureMapping mapping;
  UV input(0.5f, 0.75f);
  UV output = mapping.Apply(input);
  EXPECT_FLOAT_EQ(output.u, 0.5f);
  EXPECT_FLOAT_EQ(output.v, 0.75f);
}

TEST(TextureMapping, Apply_OffsetOnly) {
  TextureMapping mapping(0.25f, 0.5f, 1.0f, 1.0f, 0.0f);
  UV input(0.0f, 0.0f);
  UV output = mapping.Apply(input);
  EXPECT_FLOAT_EQ(output.u, 0.25f);
  EXPECT_FLOAT_EQ(output.v, 0.5f);
}

TEST(TextureMapping, Apply_ScaleOnly) {
  TextureMapping mapping(0.0f, 0.0f, 2.0f, 0.5f, 0.0f);
  UV input(1.0f, 1.0f);
  UV output = mapping.Apply(input);
  EXPECT_FLOAT_EQ(output.u, 2.0f);
  EXPECT_FLOAT_EQ(output.v, 0.5f);
}

TEST(TextureMapping, Apply_Rotation90) {
  TextureMapping mapping(0.0f, 0.0f, 1.0f, 1.0f, 90.0f);
  UV input(1.0f, 0.0f);
  UV output = mapping.Apply(input);
  EXPECT_NEAR(output.u, 0.0f, 1e-5f);
  EXPECT_NEAR(output.v, 1.0f, 1e-5f);
}

TEST(TextureMapping, Apply_Rotation180) {
  TextureMapping mapping(0.0f, 0.0f, 1.0f, 1.0f, 180.0f);
  UV input(1.0f, 0.0f);
  UV output = mapping.Apply(input);
  EXPECT_NEAR(output.u, -1.0f, 1e-5f);
  EXPECT_NEAR(output.v, 0.0f, 1e-5f);
}

// =============================================================================
// SurfaceFlags Tests
// =============================================================================

TEST(SurfaceFlags, DefaultIsNone) {
  SurfaceFlags flags = SurfaceFlags::None;
  EXPECT_EQ(static_cast<uint32_t>(flags), 0u);
}

TEST(SurfaceFlags, BitwiseOr) {
  SurfaceFlags flags = SurfaceFlags::Solid | SurfaceFlags::Lightmap;
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Lightmap));
  EXPECT_FALSE(HasFlag(flags, SurfaceFlags::Sky));
}

TEST(SurfaceFlags, BitwiseAnd) {
  SurfaceFlags flags = SurfaceFlags::Solid | SurfaceFlags::Lightmap | SurfaceFlags::Sky;
  SurfaceFlags result = flags & SurfaceFlags::Solid;
  EXPECT_TRUE(HasFlag(result, SurfaceFlags::Solid));
  EXPECT_FALSE(HasFlag(result, SurfaceFlags::Lightmap));
}

TEST(SurfaceFlags, BitwiseXor) {
  SurfaceFlags flags = SurfaceFlags::Solid | SurfaceFlags::Lightmap;
  SurfaceFlags result = flags ^ SurfaceFlags::Lightmap;
  EXPECT_TRUE(HasFlag(result, SurfaceFlags::Solid));
  EXPECT_FALSE(HasFlag(result, SurfaceFlags::Lightmap));
}

TEST(SurfaceFlags, BitwiseNot) {
  SurfaceFlags flags = SurfaceFlags::Solid;
  SurfaceFlags inverted = ~flags;
  EXPECT_FALSE(HasFlag(inverted, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(inverted, SurfaceFlags::Lightmap));
}

TEST(SurfaceFlags, CompoundOr) {
  SurfaceFlags flags = SurfaceFlags::Solid;
  flags |= SurfaceFlags::Lightmap;
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Lightmap));
}

TEST(SurfaceFlags, CompoundAnd) {
  SurfaceFlags flags = SurfaceFlags::Solid | SurfaceFlags::Lightmap;
  flags &= SurfaceFlags::Solid;
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
  EXPECT_FALSE(HasFlag(flags, SurfaceFlags::Lightmap));
}

TEST(SurfaceFlags, HasFlag_SingleFlag) {
  SurfaceFlags flags = SurfaceFlags::Sky;
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Sky));
  EXPECT_FALSE(HasFlag(flags, SurfaceFlags::Solid));
}

TEST(SurfaceFlags, HasFlag_MultipleFlags) {
  SurfaceFlags flags = SurfaceFlags::Solid | SurfaceFlags::Fullbright | SurfaceFlags::Mirror;
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Fullbright));
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Mirror));
  EXPECT_FALSE(HasFlag(flags, SurfaceFlags::Sky));
  EXPECT_FALSE(HasFlag(flags, SurfaceFlags::Portal));
}

TEST(SurfaceFlags, SetFlag) {
  SurfaceFlags flags = SurfaceFlags::None;
  SetFlag(flags, SurfaceFlags::Solid);
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
}

TEST(SurfaceFlags, ClearFlag) {
  SurfaceFlags flags = SurfaceFlags::Solid | SurfaceFlags::Lightmap;
  ClearFlag(flags, SurfaceFlags::Lightmap);
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
  EXPECT_FALSE(HasFlag(flags, SurfaceFlags::Lightmap));
}

TEST(SurfaceFlags, ToggleFlag) {
  SurfaceFlags flags = SurfaceFlags::Solid;
  ToggleFlag(flags, SurfaceFlags::Lightmap);
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Lightmap));
  ToggleFlag(flags, SurfaceFlags::Lightmap);
  EXPECT_FALSE(HasFlag(flags, SurfaceFlags::Lightmap));
}

TEST(SurfaceFlags, DefaultPreset) {
  SurfaceFlags flags = SurfaceFlags::Default;
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
}

TEST(SurfaceFlags, NoDrawPreset) {
  SurfaceFlags flags = SurfaceFlags::NoDraw;
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Solid));
  EXPECT_TRUE(HasFlag(flags, SurfaceFlags::Invisible));
}

// =============================================================================
// FaceProperties Tests
// =============================================================================

TEST(FaceProperties, DefaultConstruction) {
  FaceProperties props;
  EXPECT_TRUE(props.texture_name.empty());
  EXPECT_EQ(props.material_id, 0u);
  EXPECT_TRUE(props.mapping.IsIdentity());
  EXPECT_EQ(props.flags, SurfaceFlags::Default);
  EXPECT_EQ(props.alpha_ref, 0u);
}

TEST(FaceProperties, ConstructWithTextureName) {
  FaceProperties props("brick_wall");
  EXPECT_EQ(props.texture_name, "brick_wall");
  EXPECT_EQ(props.material_id, 0u);
}

TEST(FaceProperties, ConstructWithTextureNameAndMaterialId) {
  FaceProperties props("brick_wall", 42);
  EXPECT_EQ(props.texture_name, "brick_wall");
  EXPECT_EQ(props.material_id, 42u);
}

TEST(FaceProperties, IsVisible_Default) {
  FaceProperties props;
  EXPECT_TRUE(props.IsVisible());
}

TEST(FaceProperties, IsVisible_Invisible) {
  FaceProperties props;
  props.flags = SurfaceFlags::Invisible;
  EXPECT_FALSE(props.IsVisible());
}

TEST(FaceProperties, HasAlphaTest_Default) {
  FaceProperties props;
  EXPECT_FALSE(props.HasAlphaTest());
}

TEST(FaceProperties, HasAlphaTest_WithValue) {
  FaceProperties props;
  props.alpha_ref = 128;
  EXPECT_TRUE(props.HasAlphaTest());
}

TEST(FaceProperties, IsFullbright_Default) {
  FaceProperties props;
  EXPECT_FALSE(props.IsFullbright());
}

TEST(FaceProperties, IsFullbright_WithFlag) {
  FaceProperties props;
  props.flags = SurfaceFlags::Fullbright;
  EXPECT_TRUE(props.IsFullbright());
}

TEST(FaceProperties, IsSky_Default) {
  FaceProperties props;
  EXPECT_FALSE(props.IsSky());
}

TEST(FaceProperties, IsSky_WithFlag) {
  FaceProperties props;
  props.flags = SurfaceFlags::Sky;
  EXPECT_TRUE(props.IsSky());
}

TEST(FaceProperties, IsSolid_Default) {
  FaceProperties props;
  EXPECT_TRUE(props.IsSolid());
}

TEST(FaceProperties, IsSolid_NotSolid) {
  FaceProperties props;
  props.flags = SurfaceFlags::None;
  EXPECT_FALSE(props.IsSolid());
}

} // namespace texture_ops

// =============================================================================
// BrushFaceTextureData Tests (editor_state.h)
// =============================================================================

#include "editor_state.h"

TEST(BrushFaceTextureData, DefaultConstruction) {
  BrushFaceTextureData data;
  EXPECT_TRUE(data.texture_name.empty());
  EXPECT_TRUE(data.mapping.IsIdentity());
  EXPECT_EQ(data.surface_flags, static_cast<uint32_t>(texture_ops::SurfaceFlags::Default));
  EXPECT_EQ(data.alpha_ref, 0u);
}

TEST(BrushFaceTextureData, FromFaceProperties) {
  texture_ops::FaceProperties props;
  props.texture_name = "brick_wall";
  props.mapping = texture_ops::TextureMapping(0.5f, 0.25f, 2.0f, 1.5f, 45.0f);
  props.flags = texture_ops::SurfaceFlags::Solid | texture_ops::SurfaceFlags::Fullbright;
  props.alpha_ref = 128;

  BrushFaceTextureData data = BrushFaceTextureData::FromFaceProperties(props);

  EXPECT_EQ(data.texture_name, "brick_wall");
  EXPECT_FLOAT_EQ(data.mapping.offset_u, 0.5f);
  EXPECT_FLOAT_EQ(data.mapping.offset_v, 0.25f);
  EXPECT_FLOAT_EQ(data.mapping.scale_u, 2.0f);
  EXPECT_FLOAT_EQ(data.mapping.scale_v, 1.5f);
  EXPECT_FLOAT_EQ(data.mapping.rotation, 45.0f);
  EXPECT_EQ(data.surface_flags, static_cast<uint32_t>(props.flags));
  EXPECT_EQ(data.alpha_ref, 128u);
}

TEST(BrushFaceTextureData, ToFaceProperties) {
  BrushFaceTextureData data;
  data.texture_name = "stone_floor";
  data.mapping = texture_ops::TextureMapping(1.0f, 2.0f, 0.5f, 0.5f, 90.0f);
  data.surface_flags = static_cast<uint32_t>(texture_ops::SurfaceFlags::Sky | texture_ops::SurfaceFlags::Portal);
  data.alpha_ref = 64;

  texture_ops::FaceProperties props = data.ToFaceProperties();

  EXPECT_EQ(props.texture_name, "stone_floor");
  EXPECT_FLOAT_EQ(props.mapping.offset_u, 1.0f);
  EXPECT_FLOAT_EQ(props.mapping.offset_v, 2.0f);
  EXPECT_FLOAT_EQ(props.mapping.scale_u, 0.5f);
  EXPECT_FLOAT_EQ(props.mapping.scale_v, 0.5f);
  EXPECT_FLOAT_EQ(props.mapping.rotation, 90.0f);
  EXPECT_EQ(props.flags, texture_ops::SurfaceFlags::Sky | texture_ops::SurfaceFlags::Portal);
  EXPECT_EQ(props.alpha_ref, 64u);
}

TEST(BrushFaceTextureData, RoundTrip) {
  texture_ops::FaceProperties original;
  original.texture_name = "metal_grate";
  original.mapping = texture_ops::TextureMapping(0.1f, 0.2f, 3.0f, 4.0f, 180.0f);
  original.flags = texture_ops::SurfaceFlags::Solid | texture_ops::SurfaceFlags::Mirror;
  original.alpha_ref = 200;

  BrushFaceTextureData data = BrushFaceTextureData::FromFaceProperties(original);
  texture_ops::FaceProperties restored = data.ToFaceProperties();

  EXPECT_EQ(restored.texture_name, original.texture_name);
  EXPECT_FLOAT_EQ(restored.mapping.offset_u, original.mapping.offset_u);
  EXPECT_FLOAT_EQ(restored.mapping.offset_v, original.mapping.offset_v);
  EXPECT_FLOAT_EQ(restored.mapping.scale_u, original.mapping.scale_u);
  EXPECT_FLOAT_EQ(restored.mapping.scale_v, original.mapping.scale_v);
  EXPECT_FLOAT_EQ(restored.mapping.rotation, original.mapping.rotation);
  EXPECT_EQ(restored.flags, original.flags);
  EXPECT_EQ(restored.alpha_ref, original.alpha_ref);
}

TEST(NodeProperties, BrushTextureDataDefaultEmpty) {
  NodeProperties props;
  EXPECT_TRUE(props.brush_uvs.empty());
  EXPECT_TRUE(props.brush_face_textures.empty());
}
