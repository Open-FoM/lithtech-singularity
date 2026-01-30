#pragma once

/// @file uv_fit.h
/// @brief Fit texture to polygon UV operations (US-10-04).
///
/// Provides functions to fit texture UVs to polygons with options
/// for maintaining aspect ratio, centering, and padding.

#include "brush/csg/csg_types.h"
#include "uv_types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace texture_ops {

/// Options for fitting texture to polygon.
struct FitTextureOptions {
  uint32_t texture_width = 256;    ///< Texture width in pixels
  uint32_t texture_height = 256;   ///< Texture height in pixels
  bool maintain_aspect = true;     ///< Maintain texture aspect ratio
  bool center = true;              ///< Center UVs in [0,1] range
  float padding = 0.0f;            ///< Padding from edges (0.0 to 0.5)
};

/// Result of a fit texture operation.
struct FitTextureResult {
  bool success = false;
  size_t faces_modified = 0;
  std::string error_message;

  [[nodiscard]] static FitTextureResult Success(size_t count) {
    FitTextureResult result;
    result.success = true;
    result.faces_modified = count;
    return result;
  }

  [[nodiscard]] static FitTextureResult Failure(std::string message) {
    FitTextureResult result;
    result.success = false;
    result.error_message = std::move(message);
    return result;
  }
};

/// Compute UV bounds for a polygon.
/// @param uvs Vector of UVs.
/// @param out_min Minimum UV corner.
/// @param out_max Maximum UV corner.
void ComputeUVBounds(const std::vector<UV>& uvs, UV& out_min, UV& out_max);

/// Fit existing UVs to [0,1] range with optional aspect ratio preservation.
/// @param polygon The polygon to fit (modified in place).
/// @param options Fit options.
void FitPolygonUVs(csg::CSGPolygon& polygon, const FitTextureOptions& options);

/// Fit texture UVs to polygon by generating new UVs that fill the polygon.
/// This creates UVs where the entire texture fits the polygon bounds.
/// @param polygon The polygon to fit (modified in place).
/// @param options Fit options.
void FitTextureToPolygon(csg::CSGPolygon& polygon, const FitTextureOptions& options);

/// Fit texture UVs to specific faces of a brush.
/// @param polygons Vector of polygons.
/// @param face_indices Indices of faces to fit.
/// @param options Fit options.
/// @return Number of faces modified.
size_t FitTextureToBrushFaces(std::vector<csg::CSGPolygon>& polygons,
                               const std::vector<uint32_t>& face_indices,
                               const FitTextureOptions& options);

/// Fit texture UVs to all faces of a brush.
/// @param polygons Vector of polygons.
/// @param options Fit options.
/// @return Number of faces modified.
size_t FitTextureToAllFaces(std::vector<csg::CSGPolygon>& polygons,
                             const FitTextureOptions& options);

/// Normalize UVs to [0,1] range.
/// @param polygon The polygon to normalize (modified in place).
void NormalizePolygonUVs(csg::CSGPolygon& polygon);

} // namespace texture_ops
