#include "face_extrude.h"

#include <unordered_set>

namespace geometry_ops {

namespace {

/// Create a quad polygon from 4 vertices (in CCW order when viewed from front).
csg::CSGPolygon CreateQuad(const csg::CSGVertex& v0, const csg::CSGVertex& v1, const csg::CSGVertex& v2,
                           const csg::CSGVertex& v3, uint32_t material_id = 0) {
  csg::CSGPolygon quad({v0, v1, v2, v3}, material_id);
  return quad;
}

/// Extrude a single polygon and create side walls.
/// Returns the extruded face and any side wall faces.
std::vector<csg::CSGPolygon> ExtrudeSingleFace(const csg::CSGPolygon& face, float distance,
                                                const csg::CSGVertex& direction, bool create_side_walls) {
  std::vector<csg::CSGPolygon> result;

  if (face.vertices.size() < 3) {
    return result;
  }

  // Compute offset for each vertex
  csg::CSGVertex offset = direction * distance;

  // Create the extruded face (offset all vertices)
  std::vector<csg::CSGVertex> extruded_verts;
  extruded_verts.reserve(face.vertices.size());
  for (const auto& v : face.vertices) {
    extruded_verts.push_back(v + offset);
  }
  csg::CSGPolygon extruded_face(extruded_verts, face.material_id);
  result.push_back(extruded_face);

  // Create side walls if requested
  if (create_side_walls) {
    size_t n = face.vertices.size();
    for (size_t i = 0; i < n; ++i) {
      size_t next = (i + 1) % n;

      // Original edge: face.vertices[i] to face.vertices[next]
      // Extruded edge: extruded_verts[i] to extruded_verts[next]
      // Create a quad connecting them

      // For proper CCW winding when viewed from outside:
      // We need to determine the winding based on the face normal and extrusion direction
      const csg::CSGVertex& v0 = face.vertices[i];
      const csg::CSGVertex& v1 = face.vertices[next];
      const csg::CSGVertex& v2 = extruded_verts[next];
      const csg::CSGVertex& v3 = extruded_verts[i];

      // The quad should face outward from the side
      // Order: v0 -> v1 -> v2 -> v3 creates a quad where:
      // - v0, v1 are on the original face
      // - v2, v3 are on the extruded face
      // This gives CCW winding when viewed from outside for positive extrusion
      if (distance >= 0) {
        result.push_back(CreateQuad(v0, v3, v2, v1, face.material_id));
      } else {
        // For negative extrusion (inward), reverse the winding
        result.push_back(CreateQuad(v0, v1, v2, v3, face.material_id));
      }
    }
  }

  return result;
}

} // namespace

FaceExtrudeResult ExtrudeFaces(const csg::CSGBrush& brush, const std::vector<uint32_t>& face_indices,
                                const FaceExtrudeOptions& options) {
  FaceExtrudeResult result;

  if (brush.polygons.empty()) {
    result.success = false;
    result.error_message = "Cannot extrude faces on empty brush";
    return result;
  }

  if (face_indices.empty()) {
    result.success = true;
    result.result_brush = brush.Clone();
    return result;
  }

  // Zero distance is a no-op
  if (std::abs(options.distance) < csg::Tolerance::kEpsilon) {
    result.success = true;
    result.result_brush = brush.Clone();
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

  // Create set of faces to extrude for fast lookup
  std::unordered_set<uint32_t> extrude_set(face_indices.begin(), face_indices.end());

  // Build result brush
  std::vector<csg::CSGPolygon> new_polygons;
  new_polygons.reserve(brush.polygons.size() + face_indices.size() * 4); // Rough estimate

  for (size_t i = 0; i < brush.polygons.size(); ++i) {
    const auto& poly = brush.polygons[i];

    if (extrude_set.count(static_cast<uint32_t>(i)) == 0) {
      // Not being extruded - keep as-is
      new_polygons.push_back(poly);
    } else {
      // Extrude this face
      csg::CSGVertex direction;
      if (options.along_normal) {
        direction = poly.plane.normal.Normalized();
      } else {
        direction = options.direction.Normalized();
      }

      auto extruded = ExtrudeSingleFace(poly, options.distance, direction, options.create_side_walls);

      // Add all resulting polygons
      for (auto& p : extruded) {
        new_polygons.push_back(std::move(p));
      }

      ++result.faces_extruded;

      // Count side walls (total polygons minus the extruded face itself)
      if (!extruded.empty()) {
        result.side_faces_created += extruded.size() - 1;
      }
    }
  }

  result.result_brush = csg::CSGBrush(std::move(new_polygons));
  result.success = true;

  return result;
}

} // namespace geometry_ops
