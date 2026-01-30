#include "subobject_picking.h"

#include "editor_state.h"

#include <cmath>
#include <set>

namespace {

using Vec2 = std::array<float, 2>;
using Vec3 = std::array<float, 3>;

Vec3 Cross(const Vec3& a, const Vec3& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

Vec3 Sub(const Vec3& a, const Vec3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }

Vec3 Add(const Vec3& a, const Vec3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }

Vec3 Scale(const Vec3& v, float s) { return {v[0] * s, v[1] * s, v[2] * s}; }

float Dot(const Vec3& a, const Vec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

float Length(const Vec3& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

Vec3 Normalize(const Vec3& v) {
  float len = Length(v);
  if (len < 1e-6f) {
    return {0.0f, 0.0f, 0.0f};
  }
  return Scale(v, 1.0f / len);
}

/// Ray-triangle intersection using Möller–Trumbore algorithm.
/// Returns true if hit, with t being the ray parameter.
bool RayIntersectsTriangle(const SubObjectPickRay& ray, const Vec3& v0, const Vec3& v1, const Vec3& v2, float& out_t) {
  const float eps = 1e-6f;
  const Vec3 e1 = Sub(v1, v0);
  const Vec3 e2 = Sub(v2, v0);
  const Vec3 p = Cross(ray.dir, e2);
  const float det = Dot(e1, p);
  if (std::fabs(det) < eps) {
    return false;
  }
  const float inv_det = 1.0f / det;
  const Vec3 tvec = Sub(ray.origin, v0);
  const float u = Dot(tvec, p) * inv_det;
  if (u < 0.0f || u > 1.0f) {
    return false;
  }
  const Vec3 q = Cross(tvec, e1);
  const float v = Dot(ray.dir, q) * inv_det;
  if (v < 0.0f || (u + v) > 1.0f) {
    return false;
  }
  const float t = Dot(e2, q) * inv_det;
  if (t < 0.0f) {
    return false;
  }
  out_t = t;
  return true;
}

/// Project a 3D world point to 2D screen coordinates.
/// @param view_proj Row-major 4x4 view-projection matrix.
/// Returns nullopt if the point is behind the camera or outside the view frustum.
std::optional<Vec2> ProjectToScreen(const Vec3& world_pos, const std::array<float, 16>& view_proj,
                                    const Vec2& viewport_pos, const Vec2& viewport_size) {
  // Transform to clip space (row-major matrix, so row vectors multiplied on the left)
  // clip = world_pos * view_proj
  float clip_x = world_pos[0] * view_proj[0] + world_pos[1] * view_proj[4] + world_pos[2] * view_proj[8] + view_proj[12];
  float clip_y =
      world_pos[0] * view_proj[1] + world_pos[1] * view_proj[5] + world_pos[2] * view_proj[9] + view_proj[13];
  float clip_z =
      world_pos[0] * view_proj[2] + world_pos[1] * view_proj[6] + world_pos[2] * view_proj[10] + view_proj[14];
  float clip_w =
      world_pos[0] * view_proj[3] + world_pos[1] * view_proj[7] + world_pos[2] * view_proj[11] + view_proj[15];

  // Behind camera check
  if (clip_w <= 0.0f) {
    return std::nullopt;
  }

  // Perspective divide to NDC
  float ndc_x = clip_x / clip_w;
  float ndc_y = clip_y / clip_w;
  float ndc_z = clip_z / clip_w;

  // Reject points outside view frustum (NDC z should be in [0, 1] for Vulkan/Metal)
  if (ndc_z < 0.0f || ndc_z > 1.0f) {
    return std::nullopt;
  }

  // Convert NDC to screen coordinates
  float screen_x = viewport_pos[0] + (ndc_x * 0.5f + 0.5f) * viewport_size[0];
  float screen_y = viewport_pos[1] + (1.0f - (ndc_y * 0.5f + 0.5f)) * viewport_size[1];

  return Vec2{screen_x, screen_y};
}

/// Compute distance from a point to a line segment in 2D.
/// Also returns the parametric t value (0-1) for the closest point on the segment.
float PointToSegmentDistance2D(const Vec2& point, const Vec2& seg_start, const Vec2& seg_end, float& out_t) {
  Vec2 seg = {seg_end[0] - seg_start[0], seg_end[1] - seg_start[1]};
  Vec2 to_point = {point[0] - seg_start[0], point[1] - seg_start[1]};

  float seg_len_sq = seg[0] * seg[0] + seg[1] * seg[1];
  if (seg_len_sq < 1e-6f) {
    out_t = 0.0f;
    return std::sqrt(to_point[0] * to_point[0] + to_point[1] * to_point[1]);
  }

  float t = (to_point[0] * seg[0] + to_point[1] * seg[1]) / seg_len_sq;
  t = std::max(0.0f, std::min(1.0f, t));
  out_t = t;

  Vec2 closest = {seg_start[0] + t * seg[0], seg_start[1] + t * seg[1]};
  Vec2 diff = {point[0] - closest[0], point[1] - closest[1]};
  return std::sqrt(diff[0] * diff[0] + diff[1] * diff[1]);
}

} // namespace

FacePickResult PickFace(const SubObjectPickRay& ray, const NodeProperties& props, int node_id) {
  FacePickResult result;

  if (props.brush_vertices.empty() || props.brush_indices.empty()) {
    return result;
  }

  const size_t vertex_count = props.brush_vertices.size() / 3;
  const size_t triangle_count = props.brush_indices.size() / 3;

  float best_t = 1.0e30f;

  for (size_t tri_idx = 0; tri_idx < triangle_count; ++tri_idx) {
    size_t idx_base = tri_idx * 3;
    const uint32_t i0 = props.brush_indices[idx_base];
    const uint32_t i1 = props.brush_indices[idx_base + 1];
    const uint32_t i2 = props.brush_indices[idx_base + 2];

    if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count) {
      continue;
    }

    const size_t base0 = static_cast<size_t>(i0) * 3;
    const size_t base1 = static_cast<size_t>(i1) * 3;
    const size_t base2 = static_cast<size_t>(i2) * 3;

    const Vec3 v0 = {props.brush_vertices[base0], props.brush_vertices[base0 + 1], props.brush_vertices[base0 + 2]};
    const Vec3 v1 = {props.brush_vertices[base1], props.brush_vertices[base1 + 1], props.brush_vertices[base1 + 2]};
    const Vec3 v2 = {props.brush_vertices[base2], props.brush_vertices[base2 + 1], props.brush_vertices[base2 + 2]};

    float t = 0.0f;
    if (RayIntersectsTriangle(ray, v0, v1, v2, t)) {
      if (t < best_t) {
        best_t = t;
        result.hit = true;
        result.face.node_id = node_id;
        result.face.triangle_index = static_cast<uint32_t>(tri_idx);
        result.distance = t;
        result.hit_position = Add(ray.origin, Scale(ray.dir, t));

        // Compute face normal
        Vec3 e1 = Sub(v1, v0);
        Vec3 e2 = Sub(v2, v0);
        result.face_normal = Normalize(Cross(e1, e2));
      }
    }
  }

  return result;
}

VertexPickResult PickVertex(const std::array<float, 2>& mouse_screen, const NodeProperties& props, int node_id,
                            const std::array<float, 16>& view_proj, const std::array<float, 2>& viewport_pos,
                            const std::array<float, 2>& viewport_size, float threshold_pixels) {
  VertexPickResult result;

  if (props.brush_vertices.empty()) {
    return result;
  }

  const size_t vertex_count = props.brush_vertices.size() / 3;
  float best_dist = threshold_pixels;

  for (size_t vi = 0; vi < vertex_count; ++vi) {
    size_t base = vi * 3;
    Vec3 world_pos = {props.brush_vertices[base], props.brush_vertices[base + 1], props.brush_vertices[base + 2]};

    auto screen_pos = ProjectToScreen(world_pos, view_proj, viewport_pos, viewport_size);
    if (!screen_pos) {
      continue;
    }

    float dx = mouse_screen[0] - (*screen_pos)[0];
    float dy = mouse_screen[1] - (*screen_pos)[1];
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < best_dist) {
      best_dist = dist;
      result.hit = true;
      result.vertex.node_id = node_id;
      result.vertex.vertex_index = static_cast<uint32_t>(vi);
      result.screen_distance = dist;
      result.world_position = world_pos;
    }
  }

  return result;
}

EdgePickResult PickEdge(const std::array<float, 2>& mouse_screen, const NodeProperties& props, int node_id,
                        const std::array<float, 16>& view_proj, const std::array<float, 2>& viewport_pos,
                        const std::array<float, 2>& viewport_size, float threshold_pixels) {
  EdgePickResult result;

  auto edges = CollectBrushEdges(props);
  if (edges.empty()) {
    return result;
  }

  float best_dist = threshold_pixels;

  for (const auto& edge : edges) {
    auto positions = GetEdgePositions(props, edge.first, edge.second);
    if (!positions) {
      continue;
    }

    auto screen_a = ProjectToScreen(positions->first, view_proj, viewport_pos, viewport_size);
    auto screen_b = ProjectToScreen(positions->second, view_proj, viewport_pos, viewport_size);

    if (!screen_a || !screen_b) {
      continue;
    }

    float edge_t = 0.0f;
    float dist = PointToSegmentDistance2D(mouse_screen, *screen_a, *screen_b, edge_t);

    if (dist < best_dist) {
      best_dist = dist;
      result.hit = true;
      result.edge = EdgeRef::Create(node_id, edge.first, edge.second);
      result.screen_distance = dist;
      result.edge_t = edge_t;

      // Compute closest point on edge in world space
      Vec3 edge_vec = Sub(positions->second, positions->first);
      result.closest_point = Add(positions->first, Scale(edge_vec, edge_t));
    }
  }

  return result;
}

std::vector<std::pair<uint32_t, uint32_t>> CollectBrushEdges(const NodeProperties& props) {
  std::set<std::pair<uint32_t, uint32_t>> unique_edges;

  if (props.brush_indices.empty()) {
    return {};
  }

  const size_t triangle_count = props.brush_indices.size() / 3;

  for (size_t tri_idx = 0; tri_idx < triangle_count; ++tri_idx) {
    size_t base = tri_idx * 3;
    uint32_t i0 = props.brush_indices[base];
    uint32_t i1 = props.brush_indices[base + 1];
    uint32_t i2 = props.brush_indices[base + 2];

    // Add edges with normalized order (smaller index first)
    auto add_edge = [&unique_edges](uint32_t a, uint32_t b) {
      if (a > b)
        std::swap(a, b);
      unique_edges.insert({a, b});
    };

    add_edge(i0, i1);
    add_edge(i1, i2);
    add_edge(i2, i0);
  }

  return std::vector<std::pair<uint32_t, uint32_t>>(unique_edges.begin(), unique_edges.end());
}

std::optional<std::array<float, 3>> GetVertexPosition(const NodeProperties& props, uint32_t vertex_index) {
  size_t base = static_cast<size_t>(vertex_index) * 3;
  if (base + 2 >= props.brush_vertices.size()) {
    return std::nullopt;
  }
  return std::array<float, 3>{props.brush_vertices[base], props.brush_vertices[base + 1],
                              props.brush_vertices[base + 2]};
}

std::optional<std::pair<std::array<float, 3>, std::array<float, 3>>> GetEdgePositions(const NodeProperties& props,
                                                                                       uint32_t vertex_a,
                                                                                       uint32_t vertex_b) {
  auto pos_a = GetVertexPosition(props, vertex_a);
  auto pos_b = GetVertexPosition(props, vertex_b);

  if (!pos_a || !pos_b) {
    return std::nullopt;
  }

  return std::make_pair(*pos_a, *pos_b);
}

std::optional<std::array<float, 3>> GetFaceCentroid(const NodeProperties& props, uint32_t triangle_index) {
  size_t idx_base = static_cast<size_t>(triangle_index) * 3;
  if (idx_base + 2 >= props.brush_indices.size()) {
    return std::nullopt;
  }

  uint32_t i0 = props.brush_indices[idx_base];
  uint32_t i1 = props.brush_indices[idx_base + 1];
  uint32_t i2 = props.brush_indices[idx_base + 2];

  auto v0 = GetVertexPosition(props, i0);
  auto v1 = GetVertexPosition(props, i1);
  auto v2 = GetVertexPosition(props, i2);

  if (!v0 || !v1 || !v2) {
    return std::nullopt;
  }

  return std::array<float, 3>{((*v0)[0] + (*v1)[0] + (*v2)[0]) / 3.0f, ((*v0)[1] + (*v1)[1] + (*v2)[1]) / 3.0f,
                              ((*v0)[2] + (*v1)[2] + (*v2)[2]) / 3.0f};
}

std::optional<std::array<float, 3>> GetFaceNormal(const NodeProperties& props, uint32_t triangle_index) {
  size_t idx_base = static_cast<size_t>(triangle_index) * 3;
  if (idx_base + 2 >= props.brush_indices.size()) {
    return std::nullopt;
  }

  uint32_t i0 = props.brush_indices[idx_base];
  uint32_t i1 = props.brush_indices[idx_base + 1];
  uint32_t i2 = props.brush_indices[idx_base + 2];

  auto v0 = GetVertexPosition(props, i0);
  auto v1 = GetVertexPosition(props, i1);
  auto v2 = GetVertexPosition(props, i2);

  if (!v0 || !v1 || !v2) {
    return std::nullopt;
  }

  Vec3 e1 = Sub(*v1, *v0);
  Vec3 e2 = Sub(*v2, *v0);
  return Normalize(Cross(e1, e2));
}
