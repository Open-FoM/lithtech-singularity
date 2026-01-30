#pragma once

#include "subobject_refs.h"

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

// Forward declarations
struct NodeProperties;

/// Simple 3D ray for picking operations.
struct SubObjectPickRay {
  std::array<float, 3> origin{0.0f, 0.0f, 0.0f};
  std::array<float, 3> dir{0.0f, 0.0f, 1.0f};
};

/// Result of a face pick operation.
struct FacePickResult {
  bool hit = false;
  FaceRef face;
  float distance = 0.0f;                      ///< Ray parameter t at intersection
  std::array<float, 3> hit_position{};        ///< World position of intersection
  std::array<float, 3> face_normal{};         ///< Face normal at intersection
};

/// Result of a vertex pick operation.
struct VertexPickResult {
  bool hit = false;
  VertexRef vertex;
  float screen_distance = 0.0f;               ///< Distance in screen pixels
  std::array<float, 3> world_position{};      ///< World position of the vertex
};

/// Result of an edge pick operation.
struct EdgePickResult {
  bool hit = false;
  EdgeRef edge;
  float screen_distance = 0.0f;               ///< Distance in screen pixels
  float edge_t = 0.0f;                        ///< Parametric position along edge (0-1)
  std::array<float, 3> closest_point{};       ///< Closest point on edge to pick ray
};

/// Pick a face (triangle) using ray-triangle intersection.
/// @param ray The pick ray from camera.
/// @param props Node properties for brush being picked.
/// @param node_id The node ID of the brush.
/// @return FacePickResult with hit information.
[[nodiscard]] FacePickResult PickFace(const SubObjectPickRay& ray, const NodeProperties& props, int node_id);

/// Pick a vertex using screen-space distance threshold.
/// @param mouse_screen Mouse position in screen coordinates (x, y).
/// @param props Node properties for brush being picked.
/// @param node_id The node ID of the brush.
/// @param view_proj View-projection matrix (row-major, 16 floats).
/// @param viewport_pos Viewport top-left position in screen coordinates (x, y).
/// @param viewport_size Viewport dimensions (width, height).
/// @param threshold_pixels Screen distance threshold for selection (default 8 pixels).
/// @return VertexPickResult with hit information.
[[nodiscard]] VertexPickResult PickVertex(const std::array<float, 2>& mouse_screen, const NodeProperties& props,
                                          int node_id, const std::array<float, 16>& view_proj,
                                          const std::array<float, 2>& viewport_pos,
                                          const std::array<float, 2>& viewport_size, float threshold_pixels = 8.0f);

/// Pick an edge using screen-space line distance.
/// @param mouse_screen Mouse position in screen coordinates (x, y).
/// @param props Node properties for brush being picked.
/// @param node_id The node ID of the brush.
/// @param view_proj View-projection matrix (row-major, 16 floats).
/// @param viewport_pos Viewport top-left position in screen coordinates (x, y).
/// @param viewport_size Viewport dimensions (width, height).
/// @param threshold_pixels Screen distance threshold for selection (default 6 pixels).
/// @return EdgePickResult with hit information.
[[nodiscard]] EdgePickResult PickEdge(const std::array<float, 2>& mouse_screen, const NodeProperties& props,
                                      int node_id, const std::array<float, 16>& view_proj,
                                      const std::array<float, 2>& viewport_pos, const std::array<float, 2>& viewport_size,
                                      float threshold_pixels = 6.0f);

/// Collect all unique edges from a brush's triangle list.
/// Edges are normalized so that vertex_a < vertex_b.
/// @param props Node properties with brush_vertices and brush_indices.
/// @return Vector of unique edges as (vertex_a, vertex_b) pairs.
[[nodiscard]] std::vector<std::pair<uint32_t, uint32_t>> CollectBrushEdges(const NodeProperties& props);

/// Get the world position of a vertex.
/// @param props Node properties with brush_vertices.
/// @param vertex_index Index of the vertex.
/// @return World position as array, or nullopt if index is invalid.
[[nodiscard]] std::optional<std::array<float, 3>> GetVertexPosition(const NodeProperties& props, uint32_t vertex_index);

/// Get the world positions of an edge's endpoints.
/// @param props Node properties with brush_vertices.
/// @param vertex_a First vertex index.
/// @param vertex_b Second vertex index.
/// @return Pair of world positions, or nullopt if indices are invalid.
[[nodiscard]] std::optional<std::pair<std::array<float, 3>, std::array<float, 3>>> GetEdgePositions(
    const NodeProperties& props, uint32_t vertex_a, uint32_t vertex_b);

/// Compute the centroid of a face (triangle).
/// @param props Node properties with brush_vertices and brush_indices.
/// @param triangle_index Index of the triangle.
/// @return Centroid position, or nullopt if index is invalid.
[[nodiscard]] std::optional<std::array<float, 3>> GetFaceCentroid(const NodeProperties& props, uint32_t triangle_index);

/// Compute the normal of a face (triangle).
/// @param props Node properties with brush_vertices and brush_indices.
/// @param triangle_index Index of the triangle.
/// @return Normal vector (normalized), or nullopt if index is invalid.
[[nodiscard]] std::optional<std::array<float, 3>> GetFaceNormal(const NodeProperties& props, uint32_t triangle_index);
