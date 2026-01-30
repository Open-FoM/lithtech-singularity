#include "flip_normal.h"

namespace geometry_ops {

FlipNormalResult FlipNormals(const csg::CSGBrush& brush, const std::vector<uint32_t>& face_indices) {
  FlipNormalResult result;

  if (brush.polygons.empty()) {
    result.success = false;
    result.error_message = "Cannot flip normals on empty brush";
    return result;
  }

  if (face_indices.empty()) {
    result.success = true;
    result.result_brush = brush.Clone();
    result.faces_flipped = 0;
    return result;
  }

  // Validate all indices first
  for (uint32_t idx : face_indices) {
    if (idx >= brush.polygons.size()) {
      result.success = false;
      result.error_message = "Face index " + std::to_string(idx) + " is out of range (brush has " +
                             std::to_string(brush.polygons.size()) + " polygons)";
      return result;
    }
  }

  // Clone the brush and flip specified faces
  result.result_brush = brush.Clone();

  for (uint32_t idx : face_indices) {
    result.result_brush.polygons[idx].Flip();
    ++result.faces_flipped;
  }

  result.success = true;
  return result;
}

FlipNormalResult FlipEntireBrush(const csg::CSGBrush& brush) {
  FlipNormalResult result;

  if (brush.polygons.empty()) {
    result.success = false;
    result.error_message = "Cannot flip normals on empty brush";
    return result;
  }

  result.result_brush = brush.Clone();
  result.result_brush.Invert();
  result.faces_flipped = brush.polygons.size();
  result.success = true;

  return result;
}

} // namespace geometry_ops
