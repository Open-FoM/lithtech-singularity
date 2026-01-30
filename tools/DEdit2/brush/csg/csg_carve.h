#pragma once

/// @file csg_carve.h
/// @brief CSG carve (subtraction) operation.
///
/// The carve operation subtracts one brush from another, creating holes
/// for doorways, windows, and complex shapes through boolean subtraction.

#include "csg_types.h"

namespace csg {

/// Options for the carve operation.
struct CarveOptions {
  bool delete_cutter = true;       ///< Delete the cutting brush after operation
  bool keep_empty_results = false; ///< Keep target brushes that are fully carved away
};

/// Result of the carve operation.
struct CarveResult {
  bool success = false;
  std::string error_message;
  std::vector<CSGBrush> carved_brushes; ///< Resulting brushes after carving
  size_t affected_count = 0;            ///< Number of brushes that were actually carved
};

/// Carve (subtract) a cutter brush from target brushes.
/// The cutter acts as a cookie cutter, removing its volume from all targets.
/// @param targets Brushes to carve
/// @param cutter The cutting brush
/// @param options Carve parameters
/// @return Result containing carved brushes
[[nodiscard]] CarveResult CarveBrushes(const std::vector<CSGBrush>& targets, const CSGBrush& cutter,
                                        const CarveOptions& options = {});

/// Carve a single target brush with a cutter.
/// @param target The brush to carve
/// @param cutter The cutting brush
/// @return Vector of resulting brushes (empty if target is fully carved away)
[[nodiscard]] std::vector<CSGBrush> CarveBrush(const CSGBrush& target, const CSGBrush& cutter);

/// Carve using triangle mesh representation.
/// @param target_vertices Target brush vertices
/// @param target_indices Target brush indices
/// @param cutter_vertices Cutter brush vertices
/// @param cutter_indices Cutter brush indices
/// @return Operation result with carved brushes
[[nodiscard]] CSGOperationResult CarveBrush(const std::vector<float>& target_vertices,
                                             const std::vector<uint32_t>& target_indices,
                                             const std::vector<float>& cutter_vertices,
                                             const std::vector<uint32_t>& cutter_indices);

/// Check if two brushes intersect (bounding box test).
/// @param a First brush
/// @param b Second brush
/// @return true if bounding boxes overlap
[[nodiscard]] bool BrushesIntersect(const CSGBrush& a, const CSGBrush& b);

} // namespace csg
