#include "csg_triangulate.h"

#include <algorithm>
#include <cmath>

namespace csg {

bool IsConvexVertex(const CSGVertex& prev, const CSGVertex& curr, const CSGVertex& next, const CSGVertex& normal) {
  CSGVertex e1 = curr - prev;
  CSGVertex e2 = next - curr;
  CSGVertex cross = e1.Cross(e2);

  // Vertex is convex if cross product points in same direction as normal
  return cross.Dot(normal) >= -Tolerance::kEpsilon;
}

bool PointInTriangle(const CSGVertex& point, const CSGVertex& a, const CSGVertex& b, const CSGVertex& c,
                      const CSGVertex& normal) {
  // Use barycentric coordinates approach with cross products
  // Point is inside if all cross products point in same direction as normal

  auto sameDirection = [&normal](const CSGVertex& cross) -> bool { return cross.Dot(normal) >= -Tolerance::kEpsilon; };

  CSGVertex ab = b - a;
  CSGVertex ap = point - a;
  CSGVertex cross1 = ab.Cross(ap);
  if (!sameDirection(cross1)) {
    return false;
  }

  CSGVertex bc = c - b;
  CSGVertex bp = point - b;
  CSGVertex cross2 = bc.Cross(bp);
  if (!sameDirection(cross2)) {
    return false;
  }

  CSGVertex ca = a - c;
  CSGVertex cp = point - c;
  CSGVertex cross3 = ca.Cross(cp);
  if (!sameDirection(cross3)) {
    return false;
  }

  return true;
}

bool IsEar(const CSGPolygon& polygon, size_t prev_idx, size_t curr_idx, size_t next_idx,
           const std::vector<size_t>& remaining) {
  const CSGVertex& prev = polygon.vertices[prev_idx];
  const CSGVertex& curr = polygon.vertices[curr_idx];
  const CSGVertex& next = polygon.vertices[next_idx];
  const CSGVertex& normal = polygon.plane.normal;

  // First check if vertex is convex
  if (!IsConvexVertex(prev, curr, next, normal)) {
    return false;
  }

  // Check if any other vertex is inside the ear triangle
  for (size_t idx : remaining) {
    if (idx == prev_idx || idx == curr_idx || idx == next_idx) {
      continue;
    }

    const CSGVertex& p = polygon.vertices[idx];

    // Skip if point is very close to any triangle vertex
    if (p.NearlyEquals(prev, Tolerance::kVertexWeld) || p.NearlyEquals(curr, Tolerance::kVertexWeld) ||
        p.NearlyEquals(next, Tolerance::kVertexWeld)) {
      continue;
    }

    if (PointInTriangle(p, prev, curr, next, normal)) {
      return false;
    }
  }

  return true;
}

std::vector<CSGPolygon> TriangulateFan(const CSGPolygon& polygon) {
  std::vector<CSGPolygon> triangles;

  if (polygon.vertices.size() < 3) {
    return triangles;
  }

  if (polygon.vertices.size() == 3) {
    // Already a triangle
    triangles.push_back(polygon);
    return triangles;
  }

  triangles.reserve(polygon.vertices.size() - 2);

  const CSGVertex& v0 = polygon.vertices[0];

  for (size_t i = 1; i < polygon.vertices.size() - 1; ++i) {
    CSGPolygon tri;
    tri.vertices = {v0, polygon.vertices[i], polygon.vertices[i + 1]};
    tri.material_id = polygon.material_id;
    tri.plane = polygon.plane;
    triangles.push_back(std::move(tri));
  }

  return triangles;
}

std::vector<CSGPolygon> TriangulateEarClipping(const CSGPolygon& polygon) {
  std::vector<CSGPolygon> triangles;

  if (polygon.vertices.size() < 3) {
    return triangles;
  }

  if (polygon.vertices.size() == 3) {
    // Already a triangle
    triangles.push_back(polygon);
    return triangles;
  }

  triangles.reserve(polygon.vertices.size() - 2);

  // Build list of remaining vertex indices
  std::vector<size_t> remaining;
  remaining.reserve(polygon.vertices.size());
  for (size_t i = 0; i < polygon.vertices.size(); ++i) {
    remaining.push_back(i);
  }

  // Keep clipping ears until only a triangle remains
  size_t max_iterations = polygon.vertices.size() * polygon.vertices.size(); // Prevent infinite loops
  size_t iterations = 0;

  while (remaining.size() > 3 && iterations < max_iterations) {
    bool ear_found = false;

    for (size_t i = 0; i < remaining.size(); ++i) {
      size_t prev_i = (i + remaining.size() - 1) % remaining.size();
      size_t next_i = (i + 1) % remaining.size();

      size_t prev_idx = remaining[prev_i];
      size_t curr_idx = remaining[i];
      size_t next_idx = remaining[next_i];

      if (IsEar(polygon, prev_idx, curr_idx, next_idx, remaining)) {
        // Create triangle from ear
        CSGPolygon tri;
        tri.vertices = {polygon.vertices[prev_idx], polygon.vertices[curr_idx], polygon.vertices[next_idx]};
        tri.material_id = polygon.material_id;
        tri.plane = polygon.plane;

        // Only add if valid
        if (tri.Area() > Tolerance::kDegenerateArea) {
          triangles.push_back(std::move(tri));
        }

        // Remove the ear tip vertex
        remaining.erase(remaining.begin() + static_cast<long>(i));
        ear_found = true;
        break;
      }
    }

    if (!ear_found) {
      // No ear found - polygon might be degenerate or have numerical issues
      // Fall back to fan triangulation for remaining vertices
      if (remaining.size() >= 3) {
        CSGPolygon remaining_poly;
        for (size_t idx : remaining) {
          remaining_poly.vertices.push_back(polygon.vertices[idx]);
        }
        remaining_poly.material_id = polygon.material_id;
        remaining_poly.plane = polygon.plane;

        auto fan_tris = TriangulateFan(remaining_poly);
        for (auto& t : fan_tris) {
          triangles.push_back(std::move(t));
        }
      }
      break;
    }

    ++iterations;
  }

  // Handle last triangle
  if (remaining.size() == 3) {
    CSGPolygon tri;
    tri.vertices = {polygon.vertices[remaining[0]], polygon.vertices[remaining[1]], polygon.vertices[remaining[2]]};
    tri.material_id = polygon.material_id;
    tri.plane = polygon.plane;

    if (tri.Area() > Tolerance::kDegenerateArea) {
      triangles.push_back(std::move(tri));
    }
  }

  return triangles;
}

std::vector<CSGPolygon> Triangulate(const CSGPolygon& polygon) {
  if (polygon.vertices.size() < 3) {
    return {};
  }

  if (polygon.vertices.size() == 3) {
    return {polygon};
  }

  // Use fan triangulation for convex polygons (faster)
  if (polygon.IsConvex()) {
    return TriangulateFan(polygon);
  }

  // Use ear clipping for non-convex polygons
  return TriangulateEarClipping(polygon);
}

CSGBrush TriangulateBrush(const CSGBrush& brush) {
  CSGBrush result;

  for (const auto& poly : brush.polygons) {
    auto triangles = Triangulate(poly);
    for (auto& tri : triangles) {
      result.polygons.push_back(std::move(tri));
    }
  }

  return result;
}

} // namespace csg
