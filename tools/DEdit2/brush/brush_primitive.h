#pragma once

#include <cstdint>
#include <vector>

/// Primitive creation parameters and result structures.
/// These functions generate brush geometry (vertices + indices) that can be
/// assigned to a NodeProperties brush_vertices/brush_indices.

/// Result of a primitive creation operation.
struct PrimitiveResult {
  std::vector<float> vertices;    ///< XYZ vertex positions (size = num_verts * 3)
  std::vector<uint32_t> indices;  ///< Triangle indices (size = num_tris * 3)
  bool success = false;
};

/// Parameters for box primitive creation.
struct BoxParams {
  float center[3] = {0.0f, 0.0f, 0.0f};
  float size[3] = {64.0f, 64.0f, 64.0f};  ///< Width (X), Height (Y), Depth (Z)
};

/// Parameters for cylinder primitive creation.
struct CylinderParams {
  float center[3] = {0.0f, 0.0f, 0.0f};
  float height = 128.0f;
  float radius = 64.0f;
  int sides = 8;  ///< Number of sides (3-32)
};

/// Parameters for pyramid primitive creation.
struct PyramidParams {
  float center[3] = {0.0f, 0.0f, 0.0f};
  float height = 128.0f;
  float radius = 64.0f;  ///< Base radius
  int sides = 4;         ///< Number of base sides (3-32)
};

/// Parameters for sphere primitive creation.
struct SphereParams {
  float center[3] = {0.0f, 0.0f, 0.0f};
  float radius = 64.0f;
  int horizontal_subdivisions = 8;  ///< Longitude divisions (4-32)
  int vertical_subdivisions = 4;    ///< Latitude divisions (2-16)
  bool dome = false;                ///< If true, create hemisphere (dome)
};

/// Parameters for plane primitive creation.
struct PlaneParams {
  float center[3] = {0.0f, 0.0f, 0.0f};
  float width = 256.0f;
  float height = 256.0f;
  float normal[3] = {0.0f, 1.0f, 0.0f};  ///< Plane normal direction
};

/// Create a box (rectangular cuboid) brush primitive.
/// @param params Box parameters including center and size.
/// @return PrimitiveResult with vertices and indices, or success=false on error.
[[nodiscard]] PrimitiveResult CreatePrimitiveBox(const BoxParams& params);

/// Create a cylinder brush primitive.
/// @param params Cylinder parameters including center, height, radius, sides.
/// @return PrimitiveResult with vertices and indices, or success=false on error.
[[nodiscard]] PrimitiveResult CreatePrimitiveCylinder(const CylinderParams& params);

/// Create a pyramid brush primitive.
/// @param params Pyramid parameters including center, height, radius, sides.
/// @return PrimitiveResult with vertices and indices, or success=false on error.
[[nodiscard]] PrimitiveResult CreatePrimitivePyramid(const PyramidParams& params);

/// Create a sphere (or dome) brush primitive.
/// @param params Sphere parameters including center, radius, subdivisions, dome flag.
/// @return PrimitiveResult with vertices and indices, or success=false on error.
[[nodiscard]] PrimitiveResult CreatePrimitiveSphere(const SphereParams& params);

/// Create a plane brush primitive (single quad).
/// @param params Plane parameters including center, dimensions, and normal.
/// @return PrimitiveResult with vertices and indices, or success=false on error.
[[nodiscard]] PrimitiveResult CreatePrimitivePlane(const PlaneParams& params);
