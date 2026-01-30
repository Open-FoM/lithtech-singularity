#include "csg_convex_hull.h"

#include <algorithm>
#include <cmath>
#include <list>
#include <set>

namespace csg {

namespace {

/// Face in the convex hull (triangular).
struct HullFace {
  size_t v[3];           // Vertex indices
  CSGPlane plane;        // Face plane
  std::vector<size_t> outside_points; // Points outside this face
  bool alive = true;     // False if face has been removed

  HullFace() = default;
  HullFace(size_t a, size_t b, size_t c, const std::vector<CSGVertex>& points) {
    v[0] = a;
    v[1] = b;
    v[2] = c;

    auto plane_opt = CSGPlane::FromPoints(points[a], points[b], points[c]);
    if (plane_opt) {
      plane = *plane_opt;
    }
  }
};

/// Edge used for horizon detection.
struct HullEdge {
  size_t a, b;
  size_t face_idx;

  HullEdge(size_t v0, size_t v1, size_t face) : a(std::min(v0, v1)), b(std::max(v0, v1)), face_idx(face) {}

  bool operator<(const HullEdge& other) const {
    if (a != other.a) {
      return a < other.a;
    }
    return b < other.b;
  }

  bool SameEdge(const HullEdge& other) const { return a == other.a && b == other.b; }
};

/// Find the furthest point from a face.
size_t FindFurthestPoint(const HullFace& face, const std::vector<CSGVertex>& points) {
  float max_dist = -1.0f;
  size_t best_idx = 0;

  for (size_t idx : face.outside_points) {
    float dist = face.plane.DistanceTo(points[idx]);
    if (dist > max_dist) {
      max_dist = dist;
      best_idx = idx;
    }
  }

  return best_idx;
}

/// Compute signed volume of tetrahedron.
float TetrahedronVolume(const CSGVertex& a, const CSGVertex& b, const CSGVertex& c, const CSGVertex& d) {
  CSGVertex ab = b - a;
  CSGVertex ac = c - a;
  CSGVertex ad = d - a;
  return ab.Dot(ac.Cross(ad)) / 6.0f;
}

} // namespace

std::array<size_t, 6> FindExtremePoints(const std::vector<CSGVertex>& points) {
  std::array<size_t, 6> result = {0, 0, 0, 0, 0, 0};

  if (points.empty()) {
    return result;
  }

  for (size_t i = 1; i < points.size(); ++i) {
    const CSGVertex& p = points[i];
    if (p.x < points[result[0]].x) {
      result[0] = i;
    }
    if (p.x > points[result[1]].x) {
      result[1] = i;
    }
    if (p.y < points[result[2]].y) {
      result[2] = i;
    }
    if (p.y > points[result[3]].y) {
      result[3] = i;
    }
    if (p.z < points[result[4]].z) {
      result[4] = i;
    }
    if (p.z > points[result[5]].z) {
      result[5] = i;
    }
  }

  return result;
}

std::vector<size_t> BuildInitialTetrahedron(const std::vector<CSGVertex>& points,
                                             const std::array<size_t, 6>& extreme_indices) {
  if (points.size() < 4) {
    return {};
  }

  // Find two most distant points (check all pairs of extreme points, plus scan all points)
  float max_dist_sq = 0.0f;
  size_t idx0 = 0, idx1 = 0;

  // First check extreme points
  for (size_t i = 0; i < 6; ++i) {
    for (size_t j = i + 1; j < 6; ++j) {
      CSGVertex diff = points[extreme_indices[i]] - points[extreme_indices[j]];
      float dist_sq = diff.LengthSquared();
      if (dist_sq > max_dist_sq) {
        max_dist_sq = dist_sq;
        idx0 = extreme_indices[i];
        idx1 = extreme_indices[j];
      }
    }
  }

  // Also check all points against extremes for better coverage
  for (size_t i = 0; i < points.size(); ++i) {
    for (size_t j = 0; j < 6; ++j) {
      CSGVertex diff = points[i] - points[extreme_indices[j]];
      float dist_sq = diff.LengthSquared();
      if (dist_sq > max_dist_sq) {
        max_dist_sq = dist_sq;
        idx0 = i;
        idx1 = extreme_indices[j];
      }
    }
  }

  if (max_dist_sq < Tolerance::kDegenerateArea) {
    return {}; // All points are coincident
  }

  // Find point furthest from the line idx0-idx1
  CSGVertex line_dir = (points[idx1] - points[idx0]).Normalized();
  float max_line_dist_sq = 0.0f;
  size_t idx2 = 0;
  bool found_idx2 = false;

  for (size_t i = 0; i < points.size(); ++i) {
    if (i == idx0 || i == idx1) {
      continue;
    }

    CSGVertex to_point = points[i] - points[idx0];
    CSGVertex proj = line_dir * to_point.Dot(line_dir);
    CSGVertex perp = to_point - proj;
    float dist_sq = perp.LengthSquared();

    if (dist_sq > max_line_dist_sq) {
      max_line_dist_sq = dist_sq;
      idx2 = i;
      found_idx2 = true;
    }
  }

  if (!found_idx2 || max_line_dist_sq < Tolerance::kDegenerateArea) {
    return {}; // All points are collinear
  }

  // Find point furthest from the plane defined by idx0, idx1, idx2
  auto plane_opt = CSGPlane::FromPoints(points[idx0], points[idx1], points[idx2]);
  if (!plane_opt) {
    return {};
  }

  float max_plane_dist = 0.0f;
  size_t idx3 = 0;
  bool found_idx3 = false;

  for (size_t i = 0; i < points.size(); ++i) {
    if (i == idx0 || i == idx1 || i == idx2) {
      continue;
    }

    float dist = std::abs(plane_opt->DistanceTo(points[i]));
    if (dist > max_plane_dist) {
      max_plane_dist = dist;
      idx3 = i;
      found_idx3 = true;
    }
  }

  if (!found_idx3 || max_plane_dist < Tolerance::kPlaneThickness) {
    return {}; // All points are coplanar
  }

  // Ensure consistent orientation (idx3 should be on positive side of plane idx0-idx1-idx2)
  if (plane_opt->DistanceTo(points[idx3]) < 0) {
    std::swap(idx1, idx2);
  }

  return {idx0, idx1, idx2, idx3};
}

ConvexHullResult ComputeConvexHull(const std::vector<CSGVertex>& points) {
  ConvexHullResult result;

  if (points.size() < 4) {
    result.error_message = "Need at least 4 points for a 3D convex hull";
    return result;
  }

  // Remove duplicate points
  std::vector<CSGVertex> unique_points;
  unique_points.reserve(points.size());

  for (const auto& p : points) {
    bool is_duplicate = false;
    for (const auto& existing : unique_points) {
      if (p.NearlyEquals(existing, Tolerance::kVertexWeld)) {
        is_duplicate = true;
        break;
      }
    }
    if (!is_duplicate) {
      unique_points.push_back(p);
    }
  }

  if (unique_points.size() < 4) {
    result.error_message = "Less than 4 unique points";
    return result;
  }

  // Find extreme points and build initial tetrahedron
  auto extremes = FindExtremePoints(unique_points);
  auto tet_indices = BuildInitialTetrahedron(unique_points, extremes);

  if (tet_indices.size() < 4) {
    result.error_message = "Points are degenerate (collinear or coplanar)";
    return result;
  }

  // Initialize faces of tetrahedron (4 triangular faces)
  std::vector<HullFace> faces;
  faces.reserve(64); // Initial capacity

  // Compute centroid of tetrahedron for consistent normal orientation
  CSGVertex tet_centroid = (unique_points[tet_indices[0]] + unique_points[tet_indices[1]] +
                             unique_points[tet_indices[2]] + unique_points[tet_indices[3]]) /
                            4.0f;

  // Create faces with outward-facing normals (away from centroid)
  auto addFace = [&](size_t a, size_t b, size_t c) {
    HullFace face(a, b, c, unique_points);
    // Ensure normal points away from centroid
    if (face.plane.DistanceTo(tet_centroid) > 0) {
      std::swap(face.v[0], face.v[1]);
      face.plane = face.plane.Flipped();
    }
    faces.push_back(std::move(face));
  };

  addFace(tet_indices[0], tet_indices[1], tet_indices[2]);
  addFace(tet_indices[0], tet_indices[3], tet_indices[1]);
  addFace(tet_indices[1], tet_indices[3], tet_indices[2]);
  addFace(tet_indices[2], tet_indices[3], tet_indices[0]);

  // Mark tetrahedron vertices as used
  std::set<size_t> hull_vertices(tet_indices.begin(), tet_indices.end());

  // Assign remaining points to faces
  for (size_t i = 0; i < unique_points.size(); ++i) {
    if (hull_vertices.count(i) > 0) {
      continue;
    }

    for (auto& face : faces) {
      if (face.alive && face.plane.DistanceTo(unique_points[i]) > Tolerance::kPlaneThickness) {
        face.outside_points.push_back(i);
        break;
      }
    }
  }

  // Main Quickhull loop
  bool progress = true;
  while (progress) {
    progress = false;

    for (size_t face_idx = 0; face_idx < faces.size(); ++face_idx) {
      HullFace& face = faces[face_idx];

      if (!face.alive || face.outside_points.empty()) {
        continue;
      }

      // Find furthest point
      size_t furthest = FindFurthestPoint(face, unique_points);
      hull_vertices.insert(furthest);

      // Find horizon edges (boundary of visible region)
      std::vector<HullEdge> horizon;
      std::vector<size_t> visible_faces;

      for (size_t fi = 0; fi < faces.size(); ++fi) {
        if (!faces[fi].alive) {
          continue;
        }

        if (faces[fi].plane.DistanceTo(unique_points[furthest]) > Tolerance::kPlaneThickness) {
          visible_faces.push_back(fi);
        }
      }

      // Build horizon from visible face edges
      std::vector<HullEdge> all_edges;
      for (size_t vi : visible_faces) {
        HullFace& vf = faces[vi];
        all_edges.emplace_back(vf.v[0], vf.v[1], vi);
        all_edges.emplace_back(vf.v[1], vf.v[2], vi);
        all_edges.emplace_back(vf.v[2], vf.v[0], vi);
      }

      // Horizon edges appear only once (non-shared edges of visible region)
      for (const auto& edge : all_edges) {
        int count = 0;
        for (const auto& other : all_edges) {
          if (edge.SameEdge(other)) {
            ++count;
          }
        }
        if (count == 1) {
          horizon.push_back(edge);
        }
      }

      // Collect orphaned points from visible faces
      std::vector<size_t> orphans;
      for (size_t vi : visible_faces) {
        for (size_t pt : faces[vi].outside_points) {
          if (pt != furthest) {
            orphans.push_back(pt);
          }
        }
        faces[vi].alive = false;
      }

      // Create new faces from horizon to furthest point
      size_t new_face_start = faces.size();
      for (const auto& edge : horizon) {
        // New face: edge.a, edge.b, furthest (ensure correct winding)
        HullFace new_face(edge.a, edge.b, furthest, unique_points);

        // Check winding: centroid of hull should be "behind" the face
        CSGVertex centroid(0.0f, 0.0f, 0.0f);
        for (size_t vi : hull_vertices) {
          centroid += unique_points[vi];
        }
        centroid = centroid / static_cast<float>(hull_vertices.size());

        if (new_face.plane.DistanceTo(centroid) > 0) {
          std::swap(new_face.v[0], new_face.v[1]);
          new_face.plane = new_face.plane.Flipped();
        }

        faces.push_back(std::move(new_face));
      }

      // Reassign orphaned points to new faces
      for (size_t pt : orphans) {
        for (size_t fi = new_face_start; fi < faces.size(); ++fi) {
          if (faces[fi].alive && faces[fi].plane.DistanceTo(unique_points[pt]) > Tolerance::kPlaneThickness) {
            faces[fi].outside_points.push_back(pt);
            break;
          }
        }
      }

      progress = true;
      break; // Restart the outer loop
    }
  }

  // Build result brush from remaining faces
  for (const auto& face : faces) {
    if (!face.alive) {
      continue;
    }

    CSGPolygon poly;
    poly.vertices = {unique_points[face.v[0]], unique_points[face.v[1]], unique_points[face.v[2]]};
    poly.plane = face.plane;
    result.hull.polygons.push_back(std::move(poly));
  }

  if (result.hull.polygons.empty()) {
    result.error_message = "Failed to build convex hull";
    return result;
  }

  result.success = true;
  return result;
}

ConvexHullResult ComputeConvexHull(const std::vector<CSGBrush>& brushes) {
  // Collect all vertices from all brushes
  std::vector<CSGVertex> all_points;

  for (const auto& brush : brushes) {
    for (const auto& poly : brush.polygons) {
      for (const auto& v : poly.vertices) {
        all_points.push_back(v);
      }
    }
  }

  return ComputeConvexHull(all_points);
}

bool IsConvexBrush(const CSGBrush& brush) {
  if (!brush.IsValid()) {
    return false;
  }

  // A brush is convex if all vertices are on or behind all face planes
  for (const auto& poly : brush.polygons) {
    for (const auto& other_poly : brush.polygons) {
      for (const auto& v : other_poly.vertices) {
        float dist = poly.plane.DistanceTo(v);
        if (dist > Tolerance::kPlaneThickness) {
          return false;
        }
      }
    }
  }

  return true;
}

} // namespace csg
