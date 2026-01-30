#include "texture_applicator.h"

#include <cmath>

namespace texture_ops {

int GetBestProjectionAxis(const csg::CSGVertex& normal) {
  float abs_x = std::abs(normal.x);
  float abs_y = std::abs(normal.y);
  float abs_z = std::abs(normal.z);

  if (abs_x >= abs_y && abs_x >= abs_z) {
    return 0; // X-axis dominant, project onto YZ plane
  }
  if (abs_y >= abs_x && abs_y >= abs_z) {
    return 1; // Y-axis dominant, project onto XZ plane
  }
  return 2; // Z-axis dominant, project onto XY plane
}

std::vector<UV> GeneratePlanarUVs(const csg::CSGPolygon& polygon, float scale) {
  std::vector<UV> uvs;
  uvs.reserve(polygon.vertices.size());

  if (polygon.vertices.empty()) {
    return uvs;
  }

  // Determine projection axis based on polygon normal
  int axis = GetBestProjectionAxis(polygon.plane.normal);

  for (const auto& v : polygon.vertices) {
    UV uv;
    switch (axis) {
    case 0: // Project onto YZ plane (X dominant)
      uv.u = v.y * scale;
      uv.v = v.z * scale;
      break;
    case 1: // Project onto XZ plane (Y dominant)
      uv.u = v.x * scale;
      uv.v = v.z * scale;
      break;
    case 2: // Project onto XY plane (Z dominant)
    default:
      uv.u = v.x * scale;
      uv.v = v.y * scale;
      break;
    }
    uvs.push_back(uv);
  }

  return uvs;
}

void ApplyTextureToPolygon(csg::CSGPolygon& polygon, const std::string& texture_name, float scale) {
  polygon.face_props.texture_name = texture_name;
  polygon.uvs = GeneratePlanarUVs(polygon, scale);
}

size_t ApplyTextureToBrushFaces(std::vector<csg::CSGPolygon>& polygons,
                                 const std::vector<uint32_t>& face_indices,
                                 const std::string& texture_name,
                                 float scale) {
  size_t modified = 0;
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      ApplyTextureToPolygon(polygons[idx], texture_name, scale);
      ++modified;
    }
  }
  return modified;
}

size_t ApplyTextureToAllFaces(std::vector<csg::CSGPolygon>& polygons,
                               const std::string& texture_name,
                               float scale) {
  for (auto& polygon : polygons) {
    ApplyTextureToPolygon(polygon, texture_name, scale);
  }
  return polygons.size();
}

} // namespace texture_ops
