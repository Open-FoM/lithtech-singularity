#pragma once

/// @file csg_split.h
/// @brief Brush split operation for CSG.
///
/// Provides the public API for splitting brushes along planes.
/// This is the foundation operation used by Carve.

#include "csg_types.h"

namespace csg {

/// Result of a brush split operation.
struct BrushSplitResult {
  bool success = false;
  std::string error_message;
  CSGBrush front; ///< Brush on front side of plane (may be empty)
  CSGBrush back;  ///< Brush on back side of plane (may be empty)

  /// Check if split actually produced two parts.
  [[nodiscard]] bool HasBothParts() const { return !front.polygons.empty() && !back.polygons.empty(); }

  /// Check if brush was entirely in front of plane.
  [[nodiscard]] bool AllFront() const { return !front.polygons.empty() && back.polygons.empty(); }

  /// Check if brush was entirely behind plane.
  [[nodiscard]] bool AllBack() const { return front.polygons.empty() && !back.polygons.empty(); }
};

/// Split a brush along a plane.
/// The brush is divided into two parts at the plane, with cap faces generated
/// to close the split surfaces.
/// @param brush The brush to split
/// @param plane The splitting plane (normal points to "front" side)
/// @return Split result containing front and back brushes
[[nodiscard]] BrushSplitResult SplitBrush(const CSGBrush& brush, const CSGPlane& plane);

/// Split a brush from triangle mesh representation.
/// Convenience overload that converts from/to triangle mesh format.
/// @param vertices Brush vertex positions (XYZ triplets)
/// @param indices Triangle indices
/// @param plane_normal Normal of the splitting plane
/// @param plane_distance Distance from origin to the splitting plane
/// @return Operation result with up to 2 brushes (front and back)
[[nodiscard]] CSGOperationResult SplitBrush(const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
                                             const float plane_normal[3], float plane_distance);

} // namespace csg
