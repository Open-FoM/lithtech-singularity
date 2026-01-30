#pragma once

/// @file flip_normal.h
/// @brief Operations for flipping polygon normals in brushes.

#include "brush/csg/csg_types.h"

#include <cstdint>
#include <vector>

namespace geometry_ops {

/// Result of a flip normal operation.
struct FlipNormalResult {
  bool success = false;                ///< Whether the operation succeeded
  csg::CSGBrush result_brush;          ///< The resulting brush with flipped normals
  size_t faces_flipped = 0;            ///< Number of faces that were flipped
  std::string error_message;           ///< Error message if operation failed
};

/// Flip the normals of specific faces in a brush.
///
/// This reverses the vertex winding order of the specified faces, effectively
/// flipping their normals to point in the opposite direction.
///
/// @param brush The input brush to operate on.
/// @param face_indices Indices of polygons to flip (0-based into brush.polygons).
/// @return FlipNormalResult containing the modified brush and operation status.
///
/// Example:
/// @code
///   csg::CSGBrush brush = ...;
///   std::vector<uint32_t> faces = {0, 2, 5};
///   auto result = FlipNormals(brush, faces);
///   if (result.success) {
///     brush = std::move(result.result_brush);
///   }
/// @endcode
[[nodiscard]] FlipNormalResult FlipNormals(const csg::CSGBrush& brush, const std::vector<uint32_t>& face_indices);

/// Flip all normals in a brush (invert the entire brush).
///
/// This reverses the vertex winding order of all faces in the brush,
/// effectively turning it inside-out.
///
/// @param brush The input brush to invert.
/// @return FlipNormalResult containing the inverted brush.
[[nodiscard]] FlipNormalResult FlipEntireBrush(const csg::CSGBrush& brush);

} // namespace geometry_ops
