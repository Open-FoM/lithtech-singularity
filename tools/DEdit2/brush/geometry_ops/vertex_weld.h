#pragma once

/// @file vertex_weld.h
/// @brief Operations for welding (joining) vertices in brushes.

#include "brush/csg/csg_types.h"

#include <cstdint>
#include <vector>

namespace geometry_ops {

/// Options for vertex welding operation.
struct VertexWeldOptions {
  float weld_threshold = 0.001f;  ///< Distance threshold for auto-welding nearby vertices
  bool preserve_uv = true;        ///< Whether to preserve UV coordinates (not yet implemented)
};

/// Result of a vertex weld operation.
struct VertexWeldResult {
  bool success = false;                ///< Whether the operation succeeded
  csg::CSGBrush result_brush;          ///< The resulting brush with welded vertices
  size_t vertices_merged = 0;          ///< Number of vertices that were merged
  size_t degenerate_faces_removed = 0; ///< Number of degenerate faces removed after welding
  std::string error_message;           ///< Error message if operation failed
};

/// Weld specified vertices to their centroid.
///
/// This merges the specified vertices into a single vertex at their centroid
/// (average position). All polygon edges referencing the original vertices
/// are updated to reference the new merged vertex. Degenerate polygons
/// (those that collapse to lines or points) are removed.
///
/// @param brush The input brush to operate on.
/// @param vertex_indices Indices of vertices to weld (as polygon-local indices).
///        These are indices into the flat vertex array across all polygons.
/// @param options Welding options.
/// @return VertexWeldResult containing the modified brush and operation status.
///
/// Note: The vertex indices are global indices that count vertices sequentially
/// across all polygons. For example, if polygon 0 has 4 vertices (indices 0-3)
/// and polygon 1 has 3 vertices (indices 4-6), then index 5 refers to polygon 1's
/// second vertex.
///
/// Example:
/// @code
///   csg::CSGBrush brush = ...;
///   std::vector<uint32_t> verts = {0, 4, 8}; // Weld three corner vertices
///   auto result = WeldVertices(brush, verts, {});
///   if (result.success) {
///     brush = std::move(result.result_brush);
///   }
/// @endcode
[[nodiscard]] VertexWeldResult WeldVertices(const csg::CSGBrush& brush, const std::vector<uint32_t>& vertex_indices,
                                             const VertexWeldOptions& options);

/// Find all vertices within the weld threshold distance of each other.
///
/// This can be used to discover vertices that could be welded together
/// to clean up a brush.
///
/// @param brush The brush to analyze.
/// @param threshold Maximum distance between vertices to consider for welding.
/// @return Groups of vertex indices that are within threshold distance.
[[nodiscard]] std::vector<std::vector<uint32_t>> FindWeldableVertices(const csg::CSGBrush& brush, float threshold);

} // namespace geometry_ops
