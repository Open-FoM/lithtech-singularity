#include "csg_carve.h"

#include "csg_polygon.h"

namespace csg {

bool BrushesIntersect(const CSGBrush& a, const CSGBrush& b) {
  CSGVertex a_min, a_max, b_min, b_max;
  a.ComputeBounds(a_min, a_max);
  b.ComputeBounds(b_min, b_max);

  // Check for AABB overlap
  if (a_max.x < b_min.x || b_max.x < a_min.x) {
    return false;
  }
  if (a_max.y < b_min.y || b_max.y < a_min.y) {
    return false;
  }
  if (a_max.z < b_min.z || b_max.z < a_min.z) {
    return false;
  }
  return true;
}

namespace {

/// Split a brush by a plane for carving purposes.
/// Unlike SplitBrushByPlane, this treats coplanar polygons facing the same direction
/// as part of the "inside" (back) rather than "outside" (front).
std::pair<CSGBrush, CSGBrush> SplitBrushByPlaneForCarve(const CSGBrush& brush, const CSGPlane& plane) {
  CSGBrush front_brush;
  CSGBrush back_brush;

  for (const auto& poly : brush.polygons) {
    SplitResult split = SplitPolygonByPlane(poly, plane);

    for (auto& p : split.front) {
      front_brush.polygons.push_back(std::move(p));
    }
    for (auto& p : split.back) {
      back_brush.polygons.push_back(std::move(p));
    }
    // For carving: coplanar polygons facing same direction as cutter plane
    // are on the cutter boundary, so treat them as "inside" (back)
    for (auto& p : split.coplanar_front) {
      back_brush.polygons.push_back(std::move(p));
    }
    for (auto& p : split.coplanar_back) {
      back_brush.polygons.push_back(std::move(p));
    }
  }

  // Generate cap faces only for the front brush
  if (!front_brush.polygons.empty() && !back_brush.polygons.empty()) {
    auto front_cap = GenerateSplitCap(brush, plane, false);
    if (front_cap) {
      front_brush.polygons.push_back(std::move(*front_cap));
    }
  }

  return {std::move(front_brush), std::move(back_brush)};
}

} // namespace

std::vector<CSGBrush> CarveBrush(const CSGBrush& target, const CSGBrush& cutter) {
  std::vector<CSGBrush> results;

  if (!target.IsValid() || !cutter.IsValid()) {
    results.push_back(target);
    return results;
  }

  // Quick bounding box test
  if (!BrushesIntersect(target, cutter)) {
    results.push_back(target);
    return results;
  }

  // The carve algorithm:
  // For each plane of the cutter, split the target.
  // Keep fragments that are "outside" (in front of) the cutter.
  // Discard fragments that are "inside" (behind all) cutter planes.

  // Start with the target as the only "inside" fragment
  std::vector<CSGBrush> inside_fragments;
  inside_fragments.push_back(target.Clone());

  std::vector<CSGBrush> outside_fragments;

  // Split by each cutter face plane
  for (const auto& cutter_poly : cutter.polygons) {
    const CSGPlane& plane = cutter_poly.plane;

    std::vector<CSGBrush> new_inside;

    for (auto& fragment : inside_fragments) {
      auto [front, back] = SplitBrushByPlaneForCarve(fragment, plane);

      // Front (outside cutter) goes to outside fragments
      if (!front.polygons.empty()) {
        outside_fragments.push_back(std::move(front));
      }

      // Back (inside or potentially inside cutter) continues to be split
      if (!back.polygons.empty()) {
        new_inside.push_back(std::move(back));
      }
    }

    inside_fragments = std::move(new_inside);

    // If nothing remains inside, we're done
    if (inside_fragments.empty()) {
      break;
    }
  }

  // The outside_fragments are the result (the carved target)
  // The inside_fragments are discarded (the carved-away portion)

  if (outside_fragments.empty()) {
    // Target was entirely carved away
    return results;
  }

  return outside_fragments;
}

CarveResult CarveBrushes(const std::vector<CSGBrush>& targets, const CSGBrush& cutter, const CarveOptions& options) {
  CarveResult result;

  if (targets.empty()) {
    result.error_message = "No target brushes to carve";
    return result;
  }

  if (!cutter.IsValid()) {
    result.error_message = "Invalid cutter brush";
    return result;
  }

  for (const auto& target : targets) {
    if (!target.IsValid()) {
      // Keep invalid targets unchanged
      result.carved_brushes.push_back(target);
      continue;
    }

    // Quick intersection test
    if (!BrushesIntersect(target, cutter)) {
      // No intersection - keep original
      result.carved_brushes.push_back(target);
      continue;
    }

    // Perform carve
    auto carved = CarveBrush(target, cutter);

    if (carved.empty()) {
      // Target was fully carved away
      if (options.keep_empty_results) {
        // Keep nothing for this target
      }
      ++result.affected_count;
    } else if (carved.size() == 1 && carved[0].polygons.size() == target.polygons.size()) {
      // Likely no actual carving occurred (same brush returned)
      // This is a simplistic check - a proper implementation would compare geometry
      result.carved_brushes.push_back(std::move(carved[0]));
    } else {
      // Carving produced new fragments
      for (auto& b : carved) {
        result.carved_brushes.push_back(std::move(b));
      }
      ++result.affected_count;
    }
  }

  result.success = true;
  return result;
}

CSGOperationResult CarveBrush(const std::vector<float>& target_vertices, const std::vector<uint32_t>& target_indices,
                               const std::vector<float>& cutter_vertices, const std::vector<uint32_t>& cutter_indices) {
  // Validate input
  if (target_vertices.empty() || target_vertices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid target vertex data");
  }
  if (target_indices.empty() || target_indices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid target index data");
  }
  if (cutter_vertices.empty() || cutter_vertices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid cutter vertex data");
  }
  if (cutter_indices.empty() || cutter_indices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid cutter index data");
  }

  // Convert to CSG representation
  CSGBrush target = CSGBrush::FromTriangleMesh(target_vertices, target_indices);
  if (!target.IsValid()) {
    return CSGOperationResult::Failure("Failed to create target brush from mesh");
  }

  CSGBrush cutter = CSGBrush::FromTriangleMesh(cutter_vertices, cutter_indices);
  if (!cutter.IsValid()) {
    return CSGOperationResult::Failure("Failed to create cutter brush from mesh");
  }

  // Perform carve
  auto carved = CarveBrush(target, cutter);

  if (carved.empty()) {
    return CSGOperationResult::Failure("Target was entirely carved away");
  }

  return CSGOperationResult::Success(std::move(carved));
}

} // namespace csg
