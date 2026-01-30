#include "uv_projection.h"

#include <cmath>

namespace texture_ops {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
} // namespace

int ComputeBestProjectionAxis(const csg::CSGVertex& normal) {
  float abs_x = std::abs(normal.x);
  float abs_y = std::abs(normal.y);
  float abs_z = std::abs(normal.z);

  if (abs_x >= abs_y && abs_x >= abs_z) {
    return 0; // X dominant -> project onto YZ
  }
  if (abs_y >= abs_x && abs_y >= abs_z) {
    return 1; // Y dominant -> project onto XZ
  }
  return 2; // Z dominant -> project onto XY
}

UV ComputePlanarUV(const csg::CSGVertex& point, int axis, const UV& scale) {
  UV uv;
  switch (axis) {
  case 0: // X dominant, project onto YZ
    uv.u = point.y * scale.u;
    uv.v = point.z * scale.v;
    break;
  case 1: // Y dominant, project onto XZ
    uv.u = point.x * scale.u;
    uv.v = point.z * scale.v;
    break;
  case 2: // Z dominant, project onto XY
  default:
    uv.u = point.x * scale.u;
    uv.v = point.y * scale.v;
    break;
  }
  return uv;
}

UV ComputeCylindricalUV(const csg::CSGVertex& point,
                         const csg::CSGVertex& center,
                         const csg::CSGVertex& axis,
                         float radius,
                         const UV& scale) {
  // Vector from center to point
  csg::CSGVertex v = point - center;

  // Project onto axis to get height
  float height = v.Dot(axis);

  // Remove axis component to get radial direction
  csg::CSGVertex radial = v - axis * height;

  // Find perpendicular basis vectors
  csg::CSGVertex ref(1.0f, 0.0f, 0.0f);
  if (std::abs(axis.Dot(ref)) > 0.9f) {
    ref = csg::CSGVertex(0.0f, 1.0f, 0.0f);
  }
  csg::CSGVertex tangent = axis.Cross(ref).Normalized();
  csg::CSGVertex bitangent = axis.Cross(tangent).Normalized();

  // Compute angle
  float angle = std::atan2(radial.Dot(bitangent), radial.Dot(tangent));

  // Map angle to [0, 1]
  float u = (angle + kPi) / kTwoPi;

  // Map height to V (normalized by radius)
  float v_coord = height / radius * 0.5f + 0.5f;

  return UV(u * scale.u, v_coord * scale.v);
}

UV ComputeSphericalUV(const csg::CSGVertex& point,
                       const csg::CSGVertex& center,
                       const csg::CSGVertex& axis,
                       const UV& scale) {
  // Vector from center to point
  csg::CSGVertex v = (point - center).Normalized();

  // Latitude: angle from equator (axis is up)
  float latitude = std::asin(std::clamp(v.Dot(axis), -1.0f, 1.0f));

  // Find perpendicular basis vectors for longitude
  csg::CSGVertex ref(1.0f, 0.0f, 0.0f);
  if (std::abs(axis.Dot(ref)) > 0.9f) {
    ref = csg::CSGVertex(0.0f, 1.0f, 0.0f);
  }
  csg::CSGVertex tangent = axis.Cross(ref).Normalized();
  csg::CSGVertex bitangent = axis.Cross(tangent).Normalized();

  // Project onto equator plane
  csg::CSGVertex equator = v - axis * v.Dot(axis);
  float eq_len = equator.Length();

  float longitude = 0.0f;
  if (eq_len > Tolerance::kEpsilon) {
    equator = equator / eq_len;
    longitude = std::atan2(equator.Dot(bitangent), equator.Dot(tangent));
  }

  // Map to UV [0, 1]
  float u = (longitude + kPi) / kTwoPi;
  float v_coord = (latitude / kPi) + 0.5f; // [-pi/2, pi/2] -> [0, 1]

  return UV(u * scale.u, v_coord * scale.v);
}

void ProjectPolygonUVs(csg::CSGPolygon& polygon, const ProjectionOptions& options) {
  polygon.uvs.clear();
  polygon.uvs.reserve(polygon.vertices.size());

  int axis = 2; // Default to Z

  switch (options.type) {
  case ProjectionType::PlanarX:
    axis = 0;
    for (const auto& v : polygon.vertices) {
      polygon.uvs.push_back(ComputePlanarUV(v, axis, options.scale));
    }
    break;

  case ProjectionType::PlanarY:
    axis = 1;
    for (const auto& v : polygon.vertices) {
      polygon.uvs.push_back(ComputePlanarUV(v, axis, options.scale));
    }
    break;

  case ProjectionType::PlanarZ:
    axis = 2;
    for (const auto& v : polygon.vertices) {
      polygon.uvs.push_back(ComputePlanarUV(v, axis, options.scale));
    }
    break;

  case ProjectionType::PlanarAuto:
  case ProjectionType::Box:
    // Use face normal to determine projection axis
    axis = ComputeBestProjectionAxis(polygon.plane.normal);
    for (const auto& v : polygon.vertices) {
      polygon.uvs.push_back(ComputePlanarUV(v, axis, options.scale));
    }
    break;

  case ProjectionType::Cylindrical:
    for (const auto& v : polygon.vertices) {
      polygon.uvs.push_back(ComputeCylindricalUV(
          v, options.center, options.axis.Normalized(), options.radius, options.scale));
    }
    break;

  case ProjectionType::Spherical:
    for (const auto& v : polygon.vertices) {
      polygon.uvs.push_back(ComputeSphericalUV(
          v, options.center, options.axis.Normalized(), options.scale));
    }
    break;
  }
}

size_t ProjectBrushFaceUVs(std::vector<csg::CSGPolygon>& polygons,
                            const std::vector<uint32_t>& face_indices,
                            const ProjectionOptions& options) {
  size_t modified = 0;
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      ProjectPolygonUVs(polygons[idx], options);
      ++modified;
    }
  }
  return modified;
}

size_t ProjectAllBrushUVs(std::vector<csg::CSGPolygon>& polygons,
                           const ProjectionOptions& options) {
  for (auto& polygon : polygons) {
    ProjectPolygonUVs(polygon, options);
  }
  return polygons.size();
}

} // namespace texture_ops
