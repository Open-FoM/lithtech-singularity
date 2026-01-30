#pragma once

/// @file uv_projection.h
/// @brief UV coordinate projection algorithms (US-10-05).
///
/// Provides various projection methods for generating UV coordinates:
/// planar (X/Y/Z axis-aligned or auto-detect), cylindrical, spherical, and box.

#include "brush/csg/csg_types.h"
#include "uv_types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace texture_ops {

/// Type of UV projection to apply.
enum class ProjectionType {
  PlanarX,       ///< Planar projection onto YZ plane (X-axis view)
  PlanarY,       ///< Planar projection onto XZ plane (Y-axis view)
  PlanarZ,       ///< Planar projection onto XY plane (Z-axis view)
  PlanarAuto,    ///< Planar projection based on face normal
  Cylindrical,   ///< Cylindrical projection around an axis
  Spherical,     ///< Spherical projection from a center point
  Box            ///< Box/cube mapping (per-face planar based on normal)
};

/// Options for UV projection.
struct ProjectionOptions {
  ProjectionType type = ProjectionType::PlanarAuto;
  UV scale{1.0f, 1.0f};              ///< Scale factor for UV coordinates
  csg::CSGVertex axis{0.0f, 1.0f, 0.0f};  ///< Axis for cylindrical/spherical
  csg::CSGVertex center{0.0f, 0.0f, 0.0f}; ///< Center for cylindrical/spherical
  float radius = 64.0f;              ///< Radius for cylindrical projection
};

/// Result of a UV projection operation.
struct ProjectionResult {
  bool success = false;
  size_t faces_modified = 0;
  std::string error_message;

  [[nodiscard]] static ProjectionResult Success(size_t count) {
    ProjectionResult result;
    result.success = true;
    result.faces_modified = count;
    return result;
  }

  [[nodiscard]] static ProjectionResult Failure(std::string message) {
    ProjectionResult result;
    result.success = false;
    result.error_message = std::move(message);
    return result;
  }
};

/// Compute the best projection axis for a face normal.
/// @param normal The face normal vector.
/// @return Axis index: 0=X (YZ plane), 1=Y (XZ plane), 2=Z (XY plane).
[[nodiscard]] int ComputeBestProjectionAxis(const csg::CSGVertex& normal);

/// Compute planar UV for a single point.
/// @param point The 3D point.
/// @param axis Projection axis (0=X, 1=Y, 2=Z).
/// @param scale UV scale factor.
/// @return Computed UV coordinate.
[[nodiscard]] UV ComputePlanarUV(const csg::CSGVertex& point, int axis, const UV& scale);

/// Compute cylindrical UV for a single point.
/// @param point The 3D point.
/// @param center Cylinder center.
/// @param axis Cylinder axis (normalized).
/// @param radius Cylinder radius for V scaling.
/// @param scale UV scale factor.
/// @return Computed UV coordinate.
[[nodiscard]] UV ComputeCylindricalUV(const csg::CSGVertex& point,
                                       const csg::CSGVertex& center,
                                       const csg::CSGVertex& axis,
                                       float radius,
                                       const UV& scale);

/// Compute spherical UV for a single point.
/// @param point The 3D point.
/// @param center Sphere center.
/// @param axis "Up" axis for poles (normalized).
/// @param scale UV scale factor.
/// @return Computed UV coordinate.
[[nodiscard]] UV ComputeSphericalUV(const csg::CSGVertex& point,
                                     const csg::CSGVertex& center,
                                     const csg::CSGVertex& axis,
                                     const UV& scale);

/// Project UVs onto a polygon using specified options.
/// @param polygon The polygon to project (modified in place).
/// @param options Projection options.
void ProjectPolygonUVs(csg::CSGPolygon& polygon, const ProjectionOptions& options);

/// Project UVs onto specific faces of a brush.
/// @param polygons Vector of polygons.
/// @param face_indices Indices of faces to project.
/// @param options Projection options.
/// @return Number of faces modified.
size_t ProjectBrushFaceUVs(std::vector<csg::CSGPolygon>& polygons,
                            const std::vector<uint32_t>& face_indices,
                            const ProjectionOptions& options);

/// Project UVs onto all faces of a brush.
/// @param polygons Vector of polygons.
/// @param options Projection options.
/// @return Number of faces modified.
size_t ProjectAllBrushUVs(std::vector<csg::CSGPolygon>& polygons,
                           const ProjectionOptions& options);

} // namespace texture_ops
