#include "csg_join.h"

#include "csg_convex_hull.h"

namespace csg {

JoinResult JoinBrushes(const std::vector<CSGBrush>& brushes, const JoinOptions& options) {
  JoinResult result;

  if (brushes.size() < 2) {
    result.error_message = "Need at least 2 brushes to join";
    return result;
  }

  // Validate all input brushes
  for (size_t i = 0; i < brushes.size(); ++i) {
    if (!brushes[i].IsValid()) {
      result.error_message = "Brush " + std::to_string(i) + " is invalid";
      return result;
    }
  }

  // Compute convex hull of all brushes
  ConvexHullResult hull_result = ComputeConvexHull(brushes);

  if (!hull_result.success) {
    result.error_message = "Failed to compute convex hull: " + hull_result.error_message;
    return result;
  }

  result.joined_brush = std::move(hull_result.hull);

  // Check convexity
  result.is_convex = IsConvexBrush(result.joined_brush);

  if (!result.is_convex) {
    if (options.warn_non_convex) {
      result.had_warning = true;
      result.warning = "Joined brush is not convex - this may cause rendering and collision issues";
    }

    if (!options.allow_non_convex) {
      result.error_message = "Join would produce non-convex brush";
      result.success = false;
      return result;
    }
  }

  result.success = true;
  return result;
}

CSGOperationResult JoinBrushes(const std::vector<std::vector<float>>& all_vertices,
                                const std::vector<std::vector<uint32_t>>& all_indices) {
  if (all_vertices.size() < 2) {
    return CSGOperationResult::Failure("Need at least 2 brushes to join");
  }

  if (all_vertices.size() != all_indices.size()) {
    return CSGOperationResult::Failure("Vertex and index arrays must have same length");
  }

  // Convert to CSG brushes
  std::vector<CSGBrush> brushes;
  brushes.reserve(all_vertices.size());

  for (size_t i = 0; i < all_vertices.size(); ++i) {
    const auto& verts = all_vertices[i];
    const auto& indices = all_indices[i];

    if (verts.empty() || verts.size() % 3 != 0) {
      return CSGOperationResult::Failure("Invalid vertex data for brush " + std::to_string(i));
    }
    if (indices.empty() || indices.size() % 3 != 0) {
      return CSGOperationResult::Failure("Invalid index data for brush " + std::to_string(i));
    }

    CSGBrush brush = CSGBrush::FromTriangleMesh(verts, indices);
    if (!brush.IsValid()) {
      return CSGOperationResult::Failure("Failed to create brush " + std::to_string(i) + " from mesh");
    }

    brushes.push_back(std::move(brush));
  }

  // Perform join
  JoinOptions options;
  options.allow_non_convex = true; // Allow but warn

  JoinResult join_result = JoinBrushes(brushes, options);

  if (!join_result.success) {
    return CSGOperationResult::Failure(join_result.error_message);
  }

  std::vector<CSGBrush> results;
  results.push_back(std::move(join_result.joined_brush));

  auto result = CSGOperationResult::Success(std::move(results));

  if (join_result.had_warning) {
    // Note: Warning is lost in current API. Consider adding warnings to CSGOperationResult.
  }

  return result;
}

} // namespace csg
