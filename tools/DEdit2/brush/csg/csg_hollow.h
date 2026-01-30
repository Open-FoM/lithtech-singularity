#pragma once

/// @file csg_hollow.h
/// @brief Hollow operation for converting solid brushes to rooms.
///
/// The hollow operation creates walls of a specified thickness from a solid brush.
/// This is commonly used to create interior spaces from solid geometry.

#include "csg_types.h"

namespace csg {

/// Options for the hollow operation.
struct HollowOptions {
  float wall_thickness = 16.0f;  ///< Thickness of walls (positive = inward, negative = outward)
  bool hollow_all_faces = true;  ///< If true, hollow all faces; if false, use face_mask
  uint32_t face_mask = 0;        ///< Bitmask of faces to hollow (when hollow_all_faces = false)
};

/// Result of the hollow operation.
struct HollowResult {
  bool success = false;
  std::string error_message;
  std::vector<CSGBrush> wall_brushes; ///< Individual wall brushes
};

/// Hollow a brush to create walls of specified thickness.
/// Creates one brush per original face (6 brushes for a box, etc.)
/// @param brush The solid brush to hollow
/// @param options Hollow parameters
/// @return Result containing wall brushes
[[nodiscard]] HollowResult HollowBrush(const CSGBrush& brush, const HollowOptions& options);

/// Hollow a brush from triangle mesh representation.
/// Convenience overload that converts from/to triangle mesh format.
/// @param vertices Brush vertex positions (XYZ triplets)
/// @param indices Triangle indices
/// @param wall_thickness Wall thickness (positive = inward)
/// @return Operation result with wall brushes
[[nodiscard]] CSGOperationResult HollowBrush(const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
                                              float wall_thickness);

/// Create a wall slab brush between an outer face and its inward offset.
/// @param outer_poly The outer face polygon
/// @param thickness Wall thickness
/// @param adjacent_planes Planes of adjacent faces (for correct corner computation)
/// @return Wall brush, or empty brush if invalid
[[nodiscard]] CSGBrush CreateWallSlab(const CSGPolygon& outer_poly, float thickness,
                                       const std::vector<CSGPlane>& adjacent_planes);

/// Compute the inward offset of a vertex using adjacent plane intersections.
/// Each vertex is the intersection of 3+ planes. Offset each plane inward
/// and find the new intersection.
/// @param vertex The vertex to offset
/// @param planes Planes meeting at this vertex
/// @param thickness Distance to offset each plane
/// @return The offset vertex position, or nullopt if planes don't intersect properly
[[nodiscard]] std::optional<CSGVertex> ComputeOffsetVertex(const CSGVertex& vertex, const std::vector<CSGPlane>& planes,
                                                            float thickness);

/// Validate that a hollow operation is possible with the given thickness.
/// Checks that thickness doesn't exceed half the brush's smallest dimension.
/// @param brush The brush to check
/// @param thickness The proposed wall thickness
/// @return Empty string if valid, otherwise error message
[[nodiscard]] std::string ValidateHollowThickness(const CSGBrush& brush, float thickness);

} // namespace csg
