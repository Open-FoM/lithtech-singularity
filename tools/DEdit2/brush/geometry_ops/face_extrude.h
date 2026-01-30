#pragma once

/// @file face_extrude.h
/// @brief Operations for extruding faces in brushes.

#include "brush/csg/csg_types.h"

#include <cstdint>
#include <vector>

namespace geometry_ops {

/// Options for face extrusion operation.
struct FaceExtrudeOptions {
  float distance = 16.0f;       ///< Distance to extrude along normal (negative for inward)
  bool along_normal = true;     ///< If true, extrude along face normal; if false, use global direction
  csg::CSGVertex direction{0, 1, 0}; ///< Direction for extrusion if along_normal is false
  bool create_side_walls = true; ///< Whether to create connecting geometry between original and extruded faces
};

/// Result of a face extrusion operation.
struct FaceExtrudeResult {
  bool success = false;                ///< Whether the operation succeeded
  csg::CSGBrush result_brush;          ///< The resulting brush with extruded faces
  size_t faces_extruded = 0;           ///< Number of faces that were extruded
  size_t side_faces_created = 0;       ///< Number of side wall faces created
  std::string error_message;           ///< Error message if operation failed
};

/// Extrude specified faces along their normals.
///
/// This operation moves the specified faces outward (or inward for negative distance)
/// along their normals and creates connecting side walls between the original edge
/// positions and the new edge positions.
///
/// For a single face with N vertices, this creates:
/// - 1 extruded face (the original face moved along its normal)
/// - N side wall quads connecting old and new edges
///
/// The original face is removed and replaced by the extruded geometry.
///
/// @param brush The input brush to operate on.
/// @param face_indices Indices of polygons to extrude (0-based into brush.polygons).
/// @param options Extrusion options including distance and direction.
/// @return FaceExtrudeResult containing the modified brush and operation status.
///
/// Example:
/// @code
///   csg::CSGBrush brush = ...;
///   std::vector<uint32_t> faces = {0}; // Extrude the first face
///   FaceExtrudeOptions opts;
///   opts.distance = 32.0f;
///   auto result = ExtrudeFaces(brush, faces, opts);
///   if (result.success) {
///     brush = std::move(result.result_brush);
///   }
/// @endcode
[[nodiscard]] FaceExtrudeResult ExtrudeFaces(const csg::CSGBrush& brush, const std::vector<uint32_t>& face_indices,
                                              const FaceExtrudeOptions& options);

} // namespace geometry_ops
