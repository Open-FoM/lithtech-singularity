#include "csg_split.h"

#include "csg_polygon.h"

namespace csg {

BrushSplitResult SplitBrush(const CSGBrush& brush, const CSGPlane& plane) {
  BrushSplitResult result;

  if (!brush.IsValid()) {
    result.error_message = "Invalid input brush";
    return result;
  }

  // Use the core splitting function
  auto [front, back] = SplitBrushByPlane(brush, plane);

  result.front = std::move(front);
  result.back = std::move(back);
  result.success = true;

  return result;
}

CSGOperationResult SplitBrush(const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
                               const float plane_normal[3], float plane_distance) {
  // Validate input
  if (vertices.empty() || vertices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid vertex data");
  }
  if (indices.empty() || indices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid index data");
  }

  // Convert to CSG representation
  CSGBrush brush = CSGBrush::FromTriangleMesh(vertices, indices);
  if (!brush.IsValid()) {
    return CSGOperationResult::Failure("Failed to create brush from mesh");
  }

  // Create plane
  CSGVertex normal(plane_normal[0], plane_normal[1], plane_normal[2]);
  float len = normal.Length();
  if (len < Tolerance::kEpsilon) {
    return CSGOperationResult::Failure("Invalid plane normal (zero length)");
  }
  normal = normal / len;
  CSGPlane plane(normal, plane_distance);

  // Perform split
  BrushSplitResult split = SplitBrush(brush, plane);
  if (!split.success) {
    return CSGOperationResult::Failure(split.error_message);
  }

  // Collect results
  std::vector<CSGBrush> results;
  if (!split.front.polygons.empty()) {
    results.push_back(std::move(split.front));
  }
  if (!split.back.polygons.empty()) {
    results.push_back(std::move(split.back));
  }

  if (results.empty()) {
    return CSGOperationResult::Failure("Split produced no valid geometry");
  }

  return CSGOperationResult::Success(std::move(results));
}

} // namespace csg
