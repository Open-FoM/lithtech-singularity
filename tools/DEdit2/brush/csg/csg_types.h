#pragma once

/// @file csg_types.h
/// @brief Core data structures for CSG (Constructive Solid Geometry) operations.
///
/// This module provides polygon-based geometry types for CSG operations including
/// hollow, carve, split, join, and triangulate. The types are designed to convert
/// to/from the flat triangle mesh representation used by DEdit2 brushes.

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "brush/texture_ops/uv_types.h"

namespace csg {

// Import UV type from texture_ops namespace for convenience
using texture_ops::UV;

/// Tolerance constants for robust geometry operations.
struct Tolerance {
  /// General floating-point comparison epsilon.
  static constexpr float kEpsilon = 1e-5f;

  /// Distance threshold for classifying a point as "on" a plane.
  static constexpr float kPlaneThickness = 0.01f;

  /// Dot product threshold for considering normals as coplanar (approximately 0.5 degrees).
  static constexpr float kCoplanarNormal = 0.9999f;

  /// Minimum valid polygon area (below this is considered degenerate).
  static constexpr float kDegenerateArea = 1e-8f;

  /// Distance threshold for welding duplicate vertices.
  static constexpr float kVertexWeld = 0.001f;
};

/// Classification of a point or polygon relative to a plane.
enum class PlaneSide {
  Front,     ///< In front of plane (positive distance)
  Back,      ///< Behind plane (negative distance)
  On,        ///< On the plane (within tolerance)
  Spanning   ///< Polygon spans both sides of plane
};

/// 3D vertex with position. May be extended with UV/normal in future.
struct CSGVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  CSGVertex() = default;
  CSGVertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

  /// Vector addition.
  [[nodiscard]] CSGVertex operator+(const CSGVertex& other) const {
    return CSGVertex(x + other.x, y + other.y, z + other.z);
  }

  /// Vector subtraction.
  [[nodiscard]] CSGVertex operator-(const CSGVertex& other) const {
    return CSGVertex(x - other.x, y - other.y, z - other.z);
  }

  /// Scalar multiplication.
  [[nodiscard]] CSGVertex operator*(float s) const { return CSGVertex(x * s, y * s, z * s); }

  /// Scalar division.
  [[nodiscard]] CSGVertex operator/(float s) const {
    float inv = 1.0f / s;
    return CSGVertex(x * inv, y * inv, z * inv);
  }

  /// Compound addition.
  CSGVertex& operator+=(const CSGVertex& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  /// Compound subtraction.
  CSGVertex& operator-=(const CSGVertex& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  /// Compound scalar multiplication.
  CSGVertex& operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  /// Negation.
  [[nodiscard]] CSGVertex operator-() const { return CSGVertex(-x, -y, -z); }

  /// Dot product.
  [[nodiscard]] float Dot(const CSGVertex& other) const { return x * other.x + y * other.y + z * other.z; }

  /// Cross product.
  [[nodiscard]] CSGVertex Cross(const CSGVertex& other) const {
    return CSGVertex(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
  }

  /// Squared length (magnitude squared).
  [[nodiscard]] float LengthSquared() const { return x * x + y * y + z * z; }

  /// Length (magnitude).
  [[nodiscard]] float Length() const { return std::sqrt(LengthSquared()); }

  /// Normalized vector (unit length). Returns zero vector if length is near zero.
  [[nodiscard]] CSGVertex Normalized() const {
    float len = Length();
    if (len < Tolerance::kEpsilon) {
      return CSGVertex(0.0f, 0.0f, 0.0f);
    }
    return *this / len;
  }

  /// Check if two vertices are nearly equal within epsilon tolerance.
  [[nodiscard]] bool NearlyEquals(const CSGVertex& other, float epsilon = Tolerance::kEpsilon) const {
    return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon && std::abs(z - other.z) < epsilon;
  }

  /// Linear interpolation between two vertices.
  [[nodiscard]] static CSGVertex Lerp(const CSGVertex& a, const CSGVertex& b, float t) {
    return CSGVertex(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
  }
};

/// Plane in 3D space defined by normal and distance from origin.
/// The plane equation is: normal.Dot(point) = distance
struct CSGPlane {
  CSGVertex normal;
  float distance = 0.0f;

  CSGPlane() = default;
  CSGPlane(const CSGVertex& n, float d) : normal(n), distance(d) {}

  /// Create plane from three points (CCW winding when viewed from front).
  /// Returns nullopt if points are collinear.
  [[nodiscard]] static std::optional<CSGPlane> FromPoints(const CSGVertex& a, const CSGVertex& b, const CSGVertex& c);

  /// Create plane from a point on the plane and a normal.
  [[nodiscard]] static CSGPlane FromPointAndNormal(const CSGVertex& point, const CSGVertex& normal);

  /// Signed distance from point to plane.
  /// Positive = front, Negative = back, Zero = on plane.
  [[nodiscard]] float DistanceTo(const CSGVertex& point) const { return normal.Dot(point) - distance; }

  /// Classify point relative to plane.
  [[nodiscard]] PlaneSide ClassifyPoint(const CSGVertex& point,
                                        float thickness = Tolerance::kPlaneThickness) const;

  /// Flip plane orientation (reverse normal and negate distance).
  [[nodiscard]] CSGPlane Flipped() const { return CSGPlane(-normal, -distance); }

  /// Offset plane along its normal by the given distance.
  [[nodiscard]] CSGPlane Offset(float offset) const { return CSGPlane(normal, distance + offset); }

  /// Check if two planes are nearly parallel.
  [[nodiscard]] bool IsParallelTo(const CSGPlane& other, float threshold = Tolerance::kCoplanarNormal) const {
    float dot = std::abs(normal.Dot(other.normal));
    return dot > threshold;
  }

  /// Check if two planes are nearly coincident (parallel and same distance).
  [[nodiscard]] bool IsCoincidentWith(const CSGPlane& other, float normalThreshold = Tolerance::kCoplanarNormal,
                                      float distThreshold = Tolerance::kPlaneThickness) const {
    if (!IsParallelTo(other, normalThreshold)) {
      return false;
    }
    // Check if they have the same distance (considering normal direction)
    float dot = normal.Dot(other.normal);
    float adjustedDist = (dot > 0) ? other.distance : -other.distance;
    return std::abs(distance - adjustedDist) < distThreshold;
  }
};

/// Convex polygon with coplanar vertices (CCW winding when viewed from front).
struct CSGPolygon {
  std::vector<CSGVertex> vertices;
  std::vector<UV> uvs;                             ///< Per-vertex UV coordinates (parallel to vertices)
  CSGPlane plane;
  uint32_t material_id = 0;                        ///< For texture/material preservation (backward compat)
  texture_ops::FaceProperties face_props;          ///< Full texture and surface properties

  CSGPolygon() = default;

  /// Construct from vertices only (no UVs, default properties).
  explicit CSGPolygon(std::vector<CSGVertex> verts) : vertices(std::move(verts)) { ComputePlane(); }

  /// Construct with vertices and material ID.
  CSGPolygon(std::vector<CSGVertex> verts, uint32_t material) : vertices(std::move(verts)), material_id(material) {
    face_props.material_id = material;
    ComputePlane();
  }

  /// Construct with vertices, UVs, and face properties.
  CSGPolygon(std::vector<CSGVertex> verts, std::vector<UV> tex_coords, const texture_ops::FaceProperties& props)
      : vertices(std::move(verts)), uvs(std::move(tex_coords)), material_id(props.material_id), face_props(props) {
    ComputePlane();
    EnsureUVSize();
  }

  /// Check if polygon has valid UV coordinates (same count as vertices).
  [[nodiscard]] bool HasUVs() const { return !uvs.empty() && uvs.size() == vertices.size(); }

  /// Ensure UV array matches vertex count (fills with defaults if needed).
  void EnsureUVSize() {
    if (uvs.size() < vertices.size()) {
      uvs.resize(vertices.size());
    }
  }

  /// Compute UV centroid (average of all UVs).
  [[nodiscard]] UV UVCentroid() const {
    if (uvs.empty()) {
      return UV();
    }
    UV sum;
    for (const auto& uv : uvs) {
      sum += uv;
    }
    return sum / static_cast<float>(uvs.size());
  }

  /// Compute polygon plane from vertices using Newell's method.
  /// Returns true if a valid plane was computed.
  bool ComputePlane();

  /// Check if polygon is valid (>= 3 vertices, non-degenerate area).
  [[nodiscard]] bool IsValid() const;

  /// Check if polygon is convex.
  [[nodiscard]] bool IsConvex() const;

  /// Compute polygon area using the shoelace formula in 3D.
  [[nodiscard]] float Area() const;

  /// Compute polygon centroid.
  [[nodiscard]] CSGVertex Centroid() const;

  /// Reverse vertex order (flip normal). Also reverses UVs.
  void Flip();

  /// Classify entire polygon against a plane.
  [[nodiscard]] PlaneSide ClassifyAgainstPlane(const CSGPlane& splitPlane) const;

  /// Get the number of vertices.
  [[nodiscard]] size_t VertexCount() const { return vertices.size(); }

  /// Check if this polygon is a triangle.
  [[nodiscard]] bool IsTriangle() const { return vertices.size() == 3; }
};

/// Result of splitting geometry by a plane.
struct SplitResult {
  std::vector<CSGPolygon> front;          ///< Polygons on front side of plane
  std::vector<CSGPolygon> back;           ///< Polygons on back side of plane
  std::vector<CSGPolygon> coplanar_front; ///< Coplanar, facing same direction as plane
  std::vector<CSGPolygon> coplanar_back;  ///< Coplanar, facing opposite direction

  /// Check if the result is empty (nothing was split).
  [[nodiscard]] bool IsEmpty() const {
    return front.empty() && back.empty() && coplanar_front.empty() && coplanar_back.empty();
  }

  /// Check if only front side has geometry.
  [[nodiscard]] bool AllFront() const { return !front.empty() && back.empty(); }

  /// Check if only back side has geometry.
  [[nodiscard]] bool AllBack() const { return front.empty() && !back.empty(); }
};

/// Brush representation for CSG operations.
/// Stores polygons rather than triangles for efficient CSG.
struct CSGBrush {
  std::vector<CSGPolygon> polygons;

  CSGBrush() = default;
  explicit CSGBrush(std::vector<CSGPolygon> polys) : polygons(std::move(polys)) {}

  /// Create from triangle mesh by merging coplanar adjacent triangles.
  /// @param vertices Flat array of XYZ positions (3 floats per vertex)
  /// @param indices Triangle indices (3 indices per triangle)
  [[nodiscard]] static CSGBrush FromTriangleMesh(const std::vector<float>& vertices,
                                                  const std::vector<uint32_t>& indices);

  /// Export to triangle mesh by triangulating all polygons.
  /// @param out_vertices Output flat array of XYZ positions
  /// @param out_indices Output triangle indices
  void ToTriangleMesh(std::vector<float>& out_vertices, std::vector<uint32_t>& out_indices) const;

  /// Compute axis-aligned bounding box.
  void ComputeBounds(CSGVertex& out_min, CSGVertex& out_max) const;

  /// Check if brush is valid (has polygons, all valid).
  [[nodiscard]] bool IsValid() const;

  /// Check if brush forms a closed manifold (each edge shared by exactly 2 faces).
  [[nodiscard]] bool IsManifold() const;

  /// Flip all polygon orientations (inside-out).
  void Invert();

  /// Clone the brush.
  [[nodiscard]] CSGBrush Clone() const;

  /// Get total vertex count across all polygons.
  [[nodiscard]] size_t TotalVertexCount() const;

  /// Get total polygon count.
  [[nodiscard]] size_t PolygonCount() const { return polygons.size(); }
};

/// Result of a CSG operation.
struct CSGOperationResult {
  bool success = false;
  std::string error_message;
  std::vector<CSGBrush> results; ///< Resulting brushes from the operation

  /// Create a success result with given brushes.
  [[nodiscard]] static CSGOperationResult Success(std::vector<CSGBrush> brushes) {
    CSGOperationResult result;
    result.success = true;
    result.results = std::move(brushes);
    return result;
  }

  /// Create a failure result with error message.
  [[nodiscard]] static CSGOperationResult Failure(std::string message) {
    CSGOperationResult result;
    result.success = false;
    result.error_message = std::move(message);
    return result;
  }
};

/// Compute intersection point of a line segment with a plane.
/// @param v0 Start of line segment
/// @param v1 End of line segment
/// @param plane The plane to intersect
/// @return Intersection point, or nullopt if segment is parallel to plane
[[nodiscard]] std::optional<CSGVertex> ComputeLinePlaneIntersection(const CSGVertex& v0, const CSGVertex& v1,
                                                                     const CSGPlane& plane);

/// Compute intersection of three planes.
/// @return Intersection point, or nullopt if planes don't intersect at a single point
[[nodiscard]] std::optional<CSGVertex> ComputeThreePlaneIntersection(const CSGPlane& p1, const CSGPlane& p2,
                                                                      const CSGPlane& p3);

} // namespace csg
