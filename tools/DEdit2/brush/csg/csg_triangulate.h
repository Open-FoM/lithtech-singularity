#pragma once

/// @file csg_triangulate.h
/// @brief Polygon triangulation algorithms for CSG operations.
///
/// Provides triangulation methods for converting N-gon polygons into triangles:
/// - Fan triangulation for convex polygons (fast, O(n))
/// - Ear clipping for arbitrary simple polygons (O(n^2))

#include "csg_types.h"

namespace csg {

/// Triangulate a convex polygon using fan triangulation.
/// All triangles share the first vertex. Only works correctly for convex polygons.
/// @param polygon The convex polygon to triangulate
/// @return Vector of triangles (each as CSGPolygon with 3 vertices)
[[nodiscard]] std::vector<CSGPolygon> TriangulateFan(const CSGPolygon& polygon);

/// Triangulate any simple polygon using ear clipping algorithm.
/// Works for both convex and concave polygons. O(n^2) complexity.
/// @param polygon The polygon to triangulate
/// @return Vector of triangles (each as CSGPolygon with 3 vertices)
[[nodiscard]] std::vector<CSGPolygon> TriangulateEarClipping(const CSGPolygon& polygon);

/// Triangulate a polygon using the appropriate method.
/// Uses fan triangulation for convex polygons, ear clipping otherwise.
/// @param polygon The polygon to triangulate
/// @return Vector of triangles
[[nodiscard]] std::vector<CSGPolygon> Triangulate(const CSGPolygon& polygon);

/// Check if a vertex is an "ear" (can be clipped without creating intersections).
/// An ear is a convex vertex whose triangle doesn't contain any other vertices.
/// @param polygon The polygon being triangulated
/// @param prev_idx Index of previous vertex
/// @param curr_idx Index of current vertex (potential ear tip)
/// @param next_idx Index of next vertex
/// @param remaining Indices of remaining vertices in the polygon
/// @return true if the vertex at curr_idx is an ear
[[nodiscard]] bool IsEar(const CSGPolygon& polygon, size_t prev_idx, size_t curr_idx, size_t next_idx,
                         const std::vector<size_t>& remaining);

/// Check if a point lies inside a triangle (in 3D, projected onto triangle plane).
/// @param point The point to test
/// @param a First triangle vertex
/// @param b Second triangle vertex
/// @param c Third triangle vertex
/// @param normal The triangle/polygon normal (for consistent winding check)
/// @return true if point is inside the triangle
[[nodiscard]] bool PointInTriangle(const CSGVertex& point, const CSGVertex& a, const CSGVertex& b, const CSGVertex& c,
                                    const CSGVertex& normal);

/// Check if a vertex is convex (interior angle < 180 degrees).
/// @param prev Previous vertex
/// @param curr Current vertex
/// @param next Next vertex
/// @param normal Polygon normal (defines which side is "outside")
/// @return true if the vertex at curr is convex
[[nodiscard]] bool IsConvexVertex(const CSGVertex& prev, const CSGVertex& curr, const CSGVertex& next,
                                   const CSGVertex& normal);

/// Triangulate all polygons in a brush.
/// @param brush The brush to triangulate
/// @return New brush with all polygons triangulated
[[nodiscard]] CSGBrush TriangulateBrush(const CSGBrush& brush);

} // namespace csg
