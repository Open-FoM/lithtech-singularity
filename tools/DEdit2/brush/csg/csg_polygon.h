#pragma once

/// @file csg_polygon.h
/// @brief Polygon clipping and splitting operations for CSG.
///
/// Provides operations for clipping polygons against planes, computing
/// edge-plane intersections, and splitting polygons into front/back parts.

#include "csg_types.h"

namespace csg {

/// Split a polygon by a plane into front and back parts.
/// The result includes any coplanar portions classified by normal direction.
/// @param polygon The polygon to split
/// @param plane The splitting plane
/// @return SplitResult containing front, back, and coplanar polygons
[[nodiscard]] SplitResult SplitPolygonByPlane(const CSGPolygon& polygon, const CSGPlane& plane);

/// Clip a polygon to the front side of a plane (keep only what's in front).
/// @param polygon The polygon to clip
/// @param plane The clipping plane
/// @return The clipped polygon, or nullopt if entirely behind plane
[[nodiscard]] std::optional<CSGPolygon> ClipPolygonToFront(const CSGPolygon& polygon, const CSGPlane& plane);

/// Clip a polygon to the back side of a plane (keep only what's behind).
/// @param polygon The polygon to clip
/// @param plane The clipping plane
/// @return The clipped polygon, or nullopt if entirely in front of plane
[[nodiscard]] std::optional<CSGPolygon> ClipPolygonToBack(const CSGPolygon& polygon, const CSGPlane& plane);

/// Compute the intersection point between an edge and a plane.
/// @param v0 Start vertex of edge
/// @param v1 End vertex of edge
/// @param plane The plane to intersect
/// @param out_t Output parameter for interpolation factor (0 = v0, 1 = v1)
/// @return The intersection point, or nullopt if edge is parallel to plane
[[nodiscard]] std::optional<CSGVertex> ComputeEdgePlaneIntersection(const CSGVertex& v0, const CSGVertex& v1,
                                                                     const CSGPlane& plane, float* out_t = nullptr);

/// Split a brush by a plane into two parts.
/// @param brush The brush to split
/// @param plane The splitting plane
/// @return Pair of brushes (front, back). Either may be empty if brush is entirely on one side.
[[nodiscard]] std::pair<CSGBrush, CSGBrush> SplitBrushByPlane(const CSGBrush& brush, const CSGPlane& plane);

/// Generate a cap polygon for a brush split.
/// The cap fills the hole created when a brush is split by a plane.
/// @param brush The brush being split (used to compute cap boundary)
/// @param plane The splitting plane
/// @param face_front If true, cap faces front of plane; if false, faces back
/// @return The cap polygon, or nullopt if no valid cap can be generated
[[nodiscard]] std::optional<CSGPolygon> GenerateSplitCap(const CSGBrush& brush, const CSGPlane& plane, bool face_front);

/// Collect all edge intersection points with a plane from a brush.
/// Used for generating split caps.
/// @param brush The brush to analyze
/// @param plane The plane to check intersections against
/// @return Vector of intersection points in order around the cap boundary
[[nodiscard]] std::vector<CSGVertex> CollectPlaneIntersectionPoints(const CSGBrush& brush, const CSGPlane& plane);

/// Order a set of coplanar points into a convex polygon.
/// Uses angular sorting around centroid.
/// @param points The points to order
/// @param plane The plane the points lie on (for computing winding)
/// @return Ordered points forming a convex polygon, or empty if degenerate
[[nodiscard]] std::vector<CSGVertex> OrderPointsIntoPolygon(const std::vector<CSGVertex>& points,
                                                            const CSGPlane& plane);

} // namespace csg
