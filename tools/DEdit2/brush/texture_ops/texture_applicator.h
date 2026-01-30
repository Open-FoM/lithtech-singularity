#pragma once

/// @file texture_applicator.h
/// @brief Apply textures to brush faces with UV generation (US-10-02).
///
/// Provides functions to apply textures to specific faces or entire brushes,
/// automatically generating planar UV coordinates based on face normals.

#include "brush/csg/csg_types.h"
#include "uv_types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace texture_ops {

/// Reference to a specific face of a brush.
struct FaceRef {
  int brush_id = -1;       ///< Index of the brush in the scene
  uint32_t face_index = 0; ///< Index of the face within the brush
};

/// Result of a texture application operation.
struct ApplyTextureResult {
  bool success = false;
  size_t faces_modified = 0;
  std::string error_message;

  /// Create a success result.
  [[nodiscard]] static ApplyTextureResult Success(size_t count) {
    ApplyTextureResult result;
    result.success = true;
    result.faces_modified = count;
    return result;
  }

  /// Create a failure result.
  [[nodiscard]] static ApplyTextureResult Failure(std::string message) {
    ApplyTextureResult result;
    result.success = false;
    result.error_message = std::move(message);
    return result;
  }
};

/// Generate planar UV coordinates for a polygon based on its normal.
/// Projects vertices onto the most appropriate axis-aligned plane.
/// @param polygon The polygon to generate UVs for.
/// @param scale UV scale factor (1.0 = 1 world unit per UV unit).
/// @return Vector of UV coordinates, one per vertex.
[[nodiscard]] std::vector<UV> GeneratePlanarUVs(const csg::CSGPolygon& polygon, float scale = 0.0625f);

/// Determine the best projection axis for a normal.
/// Returns 0 for X-axis (YZ plane), 1 for Y-axis (XZ plane), 2 for Z-axis (XY plane).
/// @param normal The face normal.
/// @return Axis index (0=X, 1=Y, 2=Z).
[[nodiscard]] int GetBestProjectionAxis(const csg::CSGVertex& normal);

/// Apply a texture to a single polygon and generate UVs.
/// @param polygon The polygon to modify (modified in place).
/// @param texture_name The texture path/name to apply.
/// @param scale UV scale factor.
void ApplyTextureToPolygon(csg::CSGPolygon& polygon, const std::string& texture_name, float scale = 0.0625f);

/// Apply a texture to specific polygons in a brush.
/// @param polygons Vector of polygons in the brush.
/// @param face_indices Indices of faces to modify.
/// @param texture_name The texture path/name to apply.
/// @param scale UV scale factor.
/// @return Number of faces modified.
size_t ApplyTextureToBrushFaces(std::vector<csg::CSGPolygon>& polygons,
                                 const std::vector<uint32_t>& face_indices,
                                 const std::string& texture_name,
                                 float scale = 0.0625f);

/// Apply a texture to all polygons in a brush.
/// @param polygons Vector of polygons in the brush.
/// @param texture_name The texture path/name to apply.
/// @param scale UV scale factor.
/// @return Number of faces modified.
size_t ApplyTextureToAllFaces(std::vector<csg::CSGPolygon>& polygons,
                               const std::string& texture_name,
                               float scale = 0.0625f);

} // namespace texture_ops
