#include "csg_polygon.h"

#include <algorithm>
#include <cmath>

namespace csg {

std::optional<CSGVertex> ComputeEdgePlaneIntersection(const CSGVertex& v0, const CSGVertex& v1, const CSGPlane& plane,
                                                       float* out_t) {
  float d0 = plane.DistanceTo(v0);
  float d1 = plane.DistanceTo(v1);

  // Check if edge crosses or touches the plane
  float denom = d0 - d1;
  if (std::abs(denom) < Tolerance::kEpsilon) {
    // Edge is parallel to or lies on plane
    if (out_t) {
      *out_t = 0.5f;
    }
    return std::nullopt;
  }

  float t = d0 / denom;

  // Clamp t to valid range with small tolerance for numerical stability
  if (t < -Tolerance::kEpsilon || t > 1.0f + Tolerance::kEpsilon) {
    return std::nullopt;
  }

  t = std::clamp(t, 0.0f, 1.0f);

  if (out_t) {
    *out_t = t;
  }

  return CSGVertex::Lerp(v0, v1, t);
}

SplitResult SplitPolygonByPlane(const CSGPolygon& polygon, const CSGPlane& plane) {
  SplitResult result;

  if (polygon.vertices.size() < 3) {
    return result;
  }

  // First, classify the entire polygon
  PlaneSide classification = polygon.ClassifyAgainstPlane(plane);

  // Handle simple cases
  if (classification == PlaneSide::Front) {
    result.front.push_back(polygon);
    return result;
  }
  if (classification == PlaneSide::Back) {
    result.back.push_back(polygon);
    return result;
  }
  if (classification == PlaneSide::On) {
    // Coplanar - classify by normal direction
    float dot = polygon.plane.normal.Dot(plane.normal);
    if (dot > 0) {
      result.coplanar_front.push_back(polygon);
    } else {
      result.coplanar_back.push_back(polygon);
    }
    return result;
  }

  // Spanning case - need to split the polygon
  std::vector<CSGVertex> front_verts;
  std::vector<CSGVertex> back_verts;

  front_verts.reserve(polygon.vertices.size() + 1);
  back_verts.reserve(polygon.vertices.size() + 1);

  for (size_t i = 0; i < polygon.vertices.size(); ++i) {
    const CSGVertex& curr = polygon.vertices[i];
    const CSGVertex& next = polygon.vertices[(i + 1) % polygon.vertices.size()];

    PlaneSide curr_side = plane.ClassifyPoint(curr);
    PlaneSide next_side = plane.ClassifyPoint(next);

    // Add current vertex to appropriate list(s)
    if (curr_side == PlaneSide::Front) {
      front_verts.push_back(curr);
    } else if (curr_side == PlaneSide::Back) {
      back_verts.push_back(curr);
    } else {
      // On plane - add to both
      front_verts.push_back(curr);
      back_verts.push_back(curr);
    }

    // Check if edge crosses the plane
    bool crosses = (curr_side == PlaneSide::Front && next_side == PlaneSide::Back) ||
                   (curr_side == PlaneSide::Back && next_side == PlaneSide::Front);

    if (crosses) {
      float t;
      auto intersection = ComputeEdgePlaneIntersection(curr, next, plane, &t);
      if (intersection) {
        // Add intersection point to both polygons
        front_verts.push_back(*intersection);
        back_verts.push_back(*intersection);
      }
    }
  }

  // Create front polygon if valid
  if (front_verts.size() >= 3) {
    CSGPolygon front_poly(std::move(front_verts), polygon.material_id);
    if (front_poly.IsValid()) {
      result.front.push_back(std::move(front_poly));
    }
  }

  // Create back polygon if valid
  if (back_verts.size() >= 3) {
    CSGPolygon back_poly(std::move(back_verts), polygon.material_id);
    if (back_poly.IsValid()) {
      result.back.push_back(std::move(back_poly));
    }
  }

  return result;
}

std::optional<CSGPolygon> ClipPolygonToFront(const CSGPolygon& polygon, const CSGPlane& plane) {
  SplitResult split = SplitPolygonByPlane(polygon, plane);

  // Return front polygon if exists
  if (!split.front.empty()) {
    return split.front[0];
  }
  if (!split.coplanar_front.empty()) {
    return split.coplanar_front[0];
  }
  return std::nullopt;
}

std::optional<CSGPolygon> ClipPolygonToBack(const CSGPolygon& polygon, const CSGPlane& plane) {
  SplitResult split = SplitPolygonByPlane(polygon, plane);

  // Return back polygon if exists
  if (!split.back.empty()) {
    return split.back[0];
  }
  if (!split.coplanar_back.empty()) {
    return split.coplanar_back[0];
  }
  return std::nullopt;
}

std::vector<CSGVertex> CollectPlaneIntersectionPoints(const CSGBrush& brush, const CSGPlane& plane) {
  std::vector<CSGVertex> points;

  for (const auto& poly : brush.polygons) {
    for (size_t i = 0; i < poly.vertices.size(); ++i) {
      const CSGVertex& v0 = poly.vertices[i];
      const CSGVertex& v1 = poly.vertices[(i + 1) % poly.vertices.size()];

      PlaneSide s0 = plane.ClassifyPoint(v0);
      PlaneSide s1 = plane.ClassifyPoint(v1);

      // Check for vertices on the plane
      if (s0 == PlaneSide::On) {
        // Check if we already have this point
        bool found = false;
        for (const auto& p : points) {
          if (v0.NearlyEquals(p, Tolerance::kVertexWeld)) {
            found = true;
            break;
          }
        }
        if (!found) {
          points.push_back(v0);
        }
      }

      // Check for edge crossings
      if ((s0 == PlaneSide::Front && s1 == PlaneSide::Back) ||
          (s0 == PlaneSide::Back && s1 == PlaneSide::Front)) {
        auto intersection = ComputeEdgePlaneIntersection(v0, v1, plane);
        if (intersection) {
          // Check if we already have this point
          bool found = false;
          for (const auto& p : points) {
            if (intersection->NearlyEquals(p, Tolerance::kVertexWeld)) {
              found = true;
              break;
            }
          }
          if (!found) {
            points.push_back(*intersection);
          }
        }
      }
    }
  }

  return points;
}

std::vector<CSGVertex> OrderPointsIntoPolygon(const std::vector<CSGVertex>& points, const CSGPlane& plane) {
  if (points.size() < 3) {
    return {};
  }

  // Compute centroid
  CSGVertex centroid(0.0f, 0.0f, 0.0f);
  for (const auto& p : points) {
    centroid += p;
  }
  centroid = centroid / static_cast<float>(points.size());

  // Build orthonormal basis in plane
  CSGVertex n = plane.normal;

  // Find a vector not parallel to normal
  CSGVertex ref(1.0f, 0.0f, 0.0f);
  if (std::abs(n.Dot(ref)) > 0.9f) {
    ref = CSGVertex(0.0f, 1.0f, 0.0f);
  }

  CSGVertex u = n.Cross(ref).Normalized();
  CSGVertex v = n.Cross(u).Normalized();

  // Sort points by angle around centroid
  std::vector<std::pair<float, size_t>> angles;
  angles.reserve(points.size());

  for (size_t i = 0; i < points.size(); ++i) {
    CSGVertex d = points[i] - centroid;
    float angle = std::atan2(d.Dot(v), d.Dot(u));
    angles.push_back({angle, i});
  }

  std::sort(angles.begin(), angles.end());

  // Build ordered result
  std::vector<CSGVertex> ordered;
  ordered.reserve(points.size());
  for (const auto& [angle, idx] : angles) {
    ordered.push_back(points[idx]);
  }

  return ordered;
}

std::optional<CSGPolygon> GenerateSplitCap(const CSGBrush& brush, const CSGPlane& plane, bool face_front) {
  // Collect all intersection points
  auto points = CollectPlaneIntersectionPoints(brush, plane);

  if (points.size() < 3) {
    return std::nullopt;
  }

  // Order points into polygon
  auto ordered = OrderPointsIntoPolygon(points, plane);

  if (ordered.size() < 3) {
    return std::nullopt;
  }

  // Create polygon
  CSGPolygon cap(std::move(ordered));

  // Ensure correct facing
  float dot = cap.plane.normal.Dot(plane.normal);
  if ((face_front && dot < 0) || (!face_front && dot > 0)) {
    cap.Flip();
  }

  if (!cap.IsValid()) {
    return std::nullopt;
  }

  return cap;
}

std::pair<CSGBrush, CSGBrush> SplitBrushByPlane(const CSGBrush& brush, const CSGPlane& plane) {
  CSGBrush front_brush;
  CSGBrush back_brush;

  // Split each polygon
  for (const auto& poly : brush.polygons) {
    SplitResult split = SplitPolygonByPlane(poly, plane);

    for (auto& p : split.front) {
      front_brush.polygons.push_back(std::move(p));
    }
    for (auto& p : split.back) {
      back_brush.polygons.push_back(std::move(p));
    }
    for (auto& p : split.coplanar_front) {
      front_brush.polygons.push_back(std::move(p));
    }
    for (auto& p : split.coplanar_back) {
      back_brush.polygons.push_back(std::move(p));
    }
  }

  // Generate cap faces to close the split surfaces
  if (!front_brush.polygons.empty() && !back_brush.polygons.empty()) {
    // Front brush gets cap facing toward back (normal opposite to plane)
    auto front_cap = GenerateSplitCap(brush, plane, false);
    if (front_cap) {
      front_brush.polygons.push_back(std::move(*front_cap));
    }

    // Back brush gets cap facing toward front (normal same as plane)
    auto back_cap = GenerateSplitCap(brush, plane, true);
    if (back_cap) {
      back_brush.polygons.push_back(std::move(*back_cap));
    }
  }

  return {std::move(front_brush), std::move(back_brush)};
}

} // namespace csg
