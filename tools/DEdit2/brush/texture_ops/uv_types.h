#pragma once

/// @file uv_types.h
/// @brief Core data structures for texture mapping and UV coordinates.
///
/// Provides UV coordinate types, texture mapping parameters, and surface flags
/// for the DEdit2 texturing system (EPIC-10).

#include <cmath>
#include <cstdint>
#include <string>

namespace texture_ops {

/// Forward declaration for tolerance constant.
struct Tolerance {
  static constexpr float kEpsilon = 1e-5f;
};

/// UV texture coordinate pair.
struct UV {
  float u = 0.0f;
  float v = 0.0f;

  UV() = default;
  UV(float u_, float v_) : u(u_), v(v_) {}

  /// Vector addition.
  [[nodiscard]] UV operator+(const UV& other) const { return UV(u + other.u, v + other.v); }

  /// Vector subtraction.
  [[nodiscard]] UV operator-(const UV& other) const { return UV(u - other.u, v - other.v); }

  /// Scalar multiplication.
  [[nodiscard]] UV operator*(float s) const { return UV(u * s, v * s); }

  /// Scalar division.
  [[nodiscard]] UV operator/(float s) const {
    float inv = 1.0f / s;
    return UV(u * inv, v * inv);
  }

  /// Compound addition.
  UV& operator+=(const UV& other) {
    u += other.u;
    v += other.v;
    return *this;
  }

  /// Compound subtraction.
  UV& operator-=(const UV& other) {
    u -= other.u;
    v -= other.v;
    return *this;
  }

  /// Compound scalar multiplication.
  UV& operator*=(float s) {
    u *= s;
    v *= s;
    return *this;
  }

  /// Negation.
  [[nodiscard]] UV operator-() const { return UV(-u, -v); }

  /// Linear interpolation between two UV coordinates.
  /// @param a Start UV.
  /// @param b End UV.
  /// @param t Interpolation factor (0 = a, 1 = b).
  /// @return Interpolated UV.
  [[nodiscard]] static UV Lerp(const UV& a, const UV& b, float t) {
    return UV(a.u + (b.u - a.u) * t, a.v + (b.v - a.v) * t);
  }

  /// Check if two UVs are nearly equal within epsilon tolerance.
  [[nodiscard]] bool NearlyEquals(const UV& other, float epsilon = Tolerance::kEpsilon) const {
    return std::abs(u - other.u) < epsilon && std::abs(v - other.v) < epsilon;
  }
};

/// Per-face texture mapping parameters.
/// Controls how a texture is projected onto a polygon face.
struct TextureMapping {
  float offset_u = 0.0f; ///< Texture offset along U axis
  float offset_v = 0.0f; ///< Texture offset along V axis
  float scale_u = 1.0f;  ///< Texture scale along U axis (1.0 = no scaling)
  float scale_v = 1.0f;  ///< Texture scale along V axis
  float rotation = 0.0f; ///< Rotation in degrees (counter-clockwise)

  TextureMapping() = default;
  TextureMapping(float ou, float ov, float su, float sv, float rot)
      : offset_u(ou), offset_v(ov), scale_u(su), scale_v(sv), rotation(rot) {}

  /// Check if this mapping equals identity (no transformation).
  [[nodiscard]] bool IsIdentity() const {
    return std::abs(offset_u) < Tolerance::kEpsilon && std::abs(offset_v) < Tolerance::kEpsilon &&
           std::abs(scale_u - 1.0f) < Tolerance::kEpsilon && std::abs(scale_v - 1.0f) < Tolerance::kEpsilon &&
           std::abs(rotation) < Tolerance::kEpsilon;
  }

  /// Linear interpolation between two texture mappings.
  /// Used when splitting polygons to interpolate mapping parameters.
  [[nodiscard]] static TextureMapping Lerp(const TextureMapping& a, const TextureMapping& b, float t) {
    return TextureMapping(a.offset_u + (b.offset_u - a.offset_u) * t, a.offset_v + (b.offset_v - a.offset_v) * t,
                          a.scale_u + (b.scale_u - a.scale_u) * t, a.scale_v + (b.scale_v - a.scale_v) * t,
                          a.rotation + (b.rotation - a.rotation) * t);
  }

  /// Apply this mapping transform to a UV coordinate.
  /// Order: scale around origin, rotate around origin, then offset.
  /// @param uv Input UV coordinate.
  /// @return Transformed UV coordinate.
  [[nodiscard]] UV Apply(const UV& uv) const {
    // Scale
    float scaled_u = uv.u * scale_u;
    float scaled_v = uv.v * scale_v;

    // Rotate (convert degrees to radians)
    float rad = rotation * (3.14159265358979323846f / 180.0f);
    float cos_r = std::cos(rad);
    float sin_r = std::sin(rad);
    float rotated_u = scaled_u * cos_r - scaled_v * sin_r;
    float rotated_v = scaled_u * sin_r + scaled_v * cos_r;

    // Offset
    return UV(rotated_u + offset_u, rotated_v + offset_v);
  }
};

/// Surface rendering and physics flags.
/// Bit flags that control how a surface is rendered and interacts with physics.
enum class SurfaceFlags : uint32_t {
  None = 0,
  Solid = (1 << 0),       ///< Solid surface (collides with objects)
  Invisible = (1 << 2),   ///< Don't render (nodraw)
  Transparent = (1 << 3), ///< Alpha blending enabled
  Sky = (1 << 4),         ///< Sky portal - renders skybox through this face
  Fullbright = (1 << 5),  ///< Ignore lighting (full intensity)
  FlatShade = (1 << 6),   ///< Use flat shading instead of smooth
  Lightmap = (1 << 7),    ///< Generate lightmap for this surface
  Portal = (1 << 13),     ///< Visibility portal (can be opened/closed)
  Mirror = (1 << 23),     ///< Reflective surface

  // Common presets
  Default = Solid,
  NoDraw = Solid | Invisible,
  SkyPortal = Sky,
  FullbrightSolid = Solid | Fullbright,
};

/// Bitwise OR for SurfaceFlags.
[[nodiscard]] inline constexpr SurfaceFlags operator|(SurfaceFlags a, SurfaceFlags b) {
  return static_cast<SurfaceFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/// Bitwise AND for SurfaceFlags.
[[nodiscard]] inline constexpr SurfaceFlags operator&(SurfaceFlags a, SurfaceFlags b) {
  return static_cast<SurfaceFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/// Bitwise XOR for SurfaceFlags.
[[nodiscard]] inline constexpr SurfaceFlags operator^(SurfaceFlags a, SurfaceFlags b) {
  return static_cast<SurfaceFlags>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}

/// Bitwise NOT for SurfaceFlags.
[[nodiscard]] inline constexpr SurfaceFlags operator~(SurfaceFlags a) {
  return static_cast<SurfaceFlags>(~static_cast<uint32_t>(a));
}

/// Compound OR assignment for SurfaceFlags.
inline constexpr SurfaceFlags& operator|=(SurfaceFlags& a, SurfaceFlags b) { return a = a | b; }

/// Compound AND assignment for SurfaceFlags.
inline constexpr SurfaceFlags& operator&=(SurfaceFlags& a, SurfaceFlags b) { return a = a & b; }

/// Compound XOR assignment for SurfaceFlags.
inline constexpr SurfaceFlags& operator^=(SurfaceFlags& a, SurfaceFlags b) { return a = a ^ b; }

/// Check if a flag is set.
/// @param flags The flags to check.
/// @param check The flag to test for.
/// @return true if the flag is set.
[[nodiscard]] inline constexpr bool HasFlag(SurfaceFlags flags, SurfaceFlags check) {
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(check)) != 0;
}

/// Set a flag.
/// @param flags The flags to modify.
/// @param flag The flag to set.
inline constexpr void SetFlag(SurfaceFlags& flags, SurfaceFlags flag) { flags |= flag; }

/// Clear a flag.
/// @param flags The flags to modify.
/// @param flag The flag to clear.
inline constexpr void ClearFlag(SurfaceFlags& flags, SurfaceFlags flag) { flags &= ~flag; }

/// Toggle a flag.
/// @param flags The flags to modify.
/// @param flag The flag to toggle.
inline constexpr void ToggleFlag(SurfaceFlags& flags, SurfaceFlags flag) { flags ^= flag; }

/// Complete surface properties for a polygon face.
/// Combines texture reference, mapping, flags, and alpha settings.
struct FaceProperties {
  std::string texture_name;                    ///< Texture file name (without path/extension)
  uint32_t material_id = 0;                    ///< Material ID for shader binding
  TextureMapping mapping;                      ///< UV transformation parameters
  SurfaceFlags flags = SurfaceFlags::Default;  ///< Surface render/physics flags
  uint8_t alpha_ref = 0;                       ///< Alpha test reference value (0-255)

  FaceProperties() = default;

  /// Construct with texture name.
  explicit FaceProperties(std::string_view tex_name) : texture_name(tex_name) {}

  /// Construct with texture name and material ID.
  FaceProperties(std::string_view tex_name, uint32_t mat_id) : texture_name(tex_name), material_id(mat_id) {}

  /// Check if face should be rendered.
  [[nodiscard]] bool IsVisible() const { return !HasFlag(flags, SurfaceFlags::Invisible); }

  /// Check if face uses alpha testing.
  [[nodiscard]] bool HasAlphaTest() const { return alpha_ref > 0; }

  /// Check if face is fullbright (ignores lighting).
  [[nodiscard]] bool IsFullbright() const { return HasFlag(flags, SurfaceFlags::Fullbright); }

  /// Check if face is a sky portal.
  [[nodiscard]] bool IsSky() const { return HasFlag(flags, SurfaceFlags::Sky); }

  /// Check if face is solid (collides).
  [[nodiscard]] bool IsSolid() const { return HasFlag(flags, SurfaceFlags::Solid); }
};

} // namespace texture_ops
