#pragma once

/// @file csg_convex_hull.h
/// @brief 3D convex hull computation using Quickhull algorithm.
///
/// Computes the convex hull of a set of 3D points, returning the result
/// as a CSGBrush with polygonal faces.

#include "csg_types.h"

namespace csg {

/// Result of convex hull computation.
struct ConvexHullResult {
  bool success = false;
  std::string error_message;
  CSGBrush hull; ///< The computed convex hull as a brush
};

/// Compute the 3D convex hull of a set of points using Quickhull.
/// @param points The input points
/// @return Result containing the convex hull brush
[[nodiscard]] ConvexHullResult ComputeConvexHull(const std::vector<CSGVertex>& points);

/// Compute the convex hull of multiple brushes' vertices.
/// @param brushes The input brushes
/// @return Result containing the convex hull brush
[[nodiscard]] ConvexHullResult ComputeConvexHull(const std::vector<CSGBrush>& brushes);

/// Check if a brush is convex.
/// @param brush The brush to test
/// @return true if the brush is convex
[[nodiscard]] bool IsConvexBrush(const CSGBrush& brush);

/// Internal: Find extreme points in each direction.
/// @param points Input points
/// @return Array of 6 indices: [minX, maxX, minY, maxY, minZ, maxZ]
[[nodiscard]] std::array<size_t, 6> FindExtremePoints(const std::vector<CSGVertex>& points);

/// Internal: Build initial tetrahedron for Quickhull.
/// @param points All input points
/// @param extreme_indices Indices of extreme points
/// @return Indices of 4 points forming the initial tetrahedron, or empty if degenerate
[[nodiscard]] std::vector<size_t> BuildInitialTetrahedron(const std::vector<CSGVertex>& points,
                                                           const std::array<size_t, 6>& extreme_indices);

} // namespace csg
