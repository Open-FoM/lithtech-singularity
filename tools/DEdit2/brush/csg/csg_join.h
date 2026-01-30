#pragma once

/// @file csg_join.h
/// @brief Join brushes operation.
///
/// Merges multiple brushes into a single brush by computing their convex hull.
/// Warns if the result would be non-convex (which can cause rendering issues).

#include "csg_types.h"

namespace csg {

/// Options for the join operation.
struct JoinOptions {
  bool allow_non_convex = false; ///< Allow joining even if result is non-convex
  bool warn_non_convex = true;   ///< Generate warning if result is non-convex
};

/// Result of the join operation.
struct JoinResult {
  bool success = false;
  std::string error_message;
  CSGBrush joined_brush;     ///< The joined brush (convex hull of inputs)
  bool is_convex = true;     ///< Whether the result is convex
  bool had_warning = false;  ///< Whether a warning was generated
  std::string warning;       ///< Warning message if any
};

/// Join multiple brushes into one by computing their convex hull.
/// @param brushes The brushes to join (must be at least 2)
/// @param options Join parameters
/// @return Result containing the joined brush
[[nodiscard]] JoinResult JoinBrushes(const std::vector<CSGBrush>& brushes, const JoinOptions& options = {});

/// Join brushes from triangle mesh representation.
/// @param all_vertices Vector of vertex arrays (one per brush)
/// @param all_indices Vector of index arrays (one per brush)
/// @return Operation result with joined brush
[[nodiscard]] CSGOperationResult JoinBrushes(const std::vector<std::vector<float>>& all_vertices,
                                              const std::vector<std::vector<uint32_t>>& all_indices);

} // namespace csg
