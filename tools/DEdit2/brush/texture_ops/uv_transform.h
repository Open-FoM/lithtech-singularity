#pragma once

/// @file uv_transform.h
/// @brief UV coordinate transformation operations (US-10-03).
///
/// Provides functions to transform UV coordinates including offset, scale,
/// rotation, and flip operations.

#include "brush/csg/csg_types.h"
#include "uv_types.h"

#include <cstdint>
#include <vector>

namespace texture_ops {

/// UV transformation parameters.
struct UVTransform {
  UV offset{0.0f, 0.0f};       ///< UV offset
  UV scale{1.0f, 1.0f};        ///< UV scale factor
  float rotation = 0.0f;        ///< Rotation in degrees
  UV pivot{0.5f, 0.5f};        ///< Rotation pivot point

  UVTransform() = default;
  UVTransform(const UV& off, const UV& scl, float rot = 0.0f, const UV& piv = UV(0.5f, 0.5f))
      : offset(off), scale(scl), rotation(rot), pivot(piv) {}

  /// Check if this is an identity transform.
  [[nodiscard]] bool IsIdentity() const;

  /// Apply this transform to a UV coordinate.
  [[nodiscard]] UV Apply(const UV& uv) const;
};

/// Result of a UV transformation operation.
struct UVTransformResult {
  bool success = false;
  size_t faces_modified = 0;
  std::string error_message;

  [[nodiscard]] static UVTransformResult Success(size_t count) {
    UVTransformResult result;
    result.success = true;
    result.faces_modified = count;
    return result;
  }

  [[nodiscard]] static UVTransformResult Failure(std::string message) {
    UVTransformResult result;
    result.success = false;
    result.error_message = std::move(message);
    return result;
  }
};

/// Transform UV coordinates on a single polygon.
/// @param polygon The polygon to transform (modified in place).
/// @param transform The transformation to apply.
void TransformPolygonUVs(csg::CSGPolygon& polygon, const UVTransform& transform);

/// Transform UV coordinates on specific faces of a brush.
/// @param polygons Vector of polygons to transform.
/// @param face_indices Indices of faces to transform.
/// @param transform The transformation to apply.
/// @return Number of faces modified.
size_t TransformBrushFaceUVs(std::vector<csg::CSGPolygon>& polygons,
                              const std::vector<uint32_t>& face_indices,
                              const UVTransform& transform);

/// Transform UV coordinates on all faces of a brush.
/// @param polygons Vector of polygons to transform.
/// @param transform The transformation to apply.
/// @return Number of faces modified.
size_t TransformAllBrushUVs(std::vector<csg::CSGPolygon>& polygons,
                             const UVTransform& transform);

/// Reset UVs to default planar projection.
/// @param polygons Vector of polygons to reset.
/// @param face_indices Indices of faces to reset.
/// @param scale Scale factor for UV generation.
/// @return Number of faces modified.
size_t ResetFaceUVs(std::vector<csg::CSGPolygon>& polygons,
                     const std::vector<uint32_t>& face_indices,
                     float scale = 0.0625f);

/// Flip UVs horizontally (mirror U coordinate).
/// @param polygons Vector of polygons to flip.
/// @param face_indices Indices of faces to flip.
/// @return Number of faces modified.
size_t FlipUVsHorizontal(std::vector<csg::CSGPolygon>& polygons,
                          const std::vector<uint32_t>& face_indices);

/// Flip UVs vertically (mirror V coordinate).
/// @param polygons Vector of polygons to flip.
/// @param face_indices Indices of faces to flip.
/// @return Number of faces modified.
size_t FlipUVsVertical(std::vector<csg::CSGPolygon>& polygons,
                        const std::vector<uint32_t>& face_indices);

/// Rotate UVs around their centroid.
/// @param polygon The polygon to rotate.
/// @param degrees Rotation angle in degrees.
void RotatePolygonUVs(csg::CSGPolygon& polygon, float degrees);

/// Flip a single UV coordinate horizontally around pivot.
/// @param uv The UV to flip.
/// @param pivot_u The U pivot point (default 0.5).
/// @return Flipped UV.
[[nodiscard]] UV FlipUVHorizontal(const UV& uv, float pivot_u = 0.5f);

/// Flip a single UV coordinate vertically around pivot.
/// @param uv The UV to flip.
/// @param pivot_v The V pivot point (default 0.5).
/// @return Flipped UV.
[[nodiscard]] UV FlipUVVertical(const UV& uv, float pivot_v = 0.5f);

} // namespace texture_ops
