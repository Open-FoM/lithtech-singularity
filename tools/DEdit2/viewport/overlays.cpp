#include "viewport/overlays.h"

#include "editor_state.h"
#include "viewport_picking.h"
#include "viewport_gizmo.h"

#include "debuggeometry.h"
#include "imgui.h"
#include "ltvector.h"

#include <cstdint>
#include <cstdio>
#include <vector>

LTRGBColor MakeOverlayColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  LTRGBColor color{};
  color.rgb.r = r;
  color.rgb.g = g;
  color.rgb.b = b;
  color.rgb.a = a;
  return color;
}

namespace
{
void AddDebugLine(const float a[3], const float b[3], const LTRGBColor& color)
{
  getDebugGeometry().addLine(
    LTVector(a[0], a[1], a[2]),
    LTVector(b[0], b[1], b[2]),
    color,
    false);
}

void AddDebugAABB(const float bounds_min[3], const float bounds_max[3], const LTRGBColor& color)
{
  const float x0 = bounds_min[0];
  const float y0 = bounds_min[1];
  const float z0 = bounds_min[2];
  const float x1 = bounds_max[0];
  const float y1 = bounds_max[1];
  const float z1 = bounds_max[2];

  const float corners[8][3] = {
    {x0, y0, z0},
    {x1, y0, z0},
    {x1, y1, z0},
    {x0, y1, z0},
    {x0, y0, z1},
    {x1, y0, z1},
    {x1, y1, z1},
    {x0, y1, z1}
  };

  static const int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
  };

  for (const auto& edge : edges)
  {
    AddDebugLine(corners[edge[0]], corners[edge[1]], color);
  }
}

void AddDebugBrushWire(const NodeProperties& props, const LTRGBColor& color)
{
  const size_t vertex_count = props.brush_vertices.size() / 3;
  if (vertex_count == 0 || props.brush_indices.empty())
  {
    return;
  }

  for (size_t i = 0; i + 2 < props.brush_indices.size(); i += 3)
  {
    const uint32_t i0 = props.brush_indices[i];
    const uint32_t i1 = props.brush_indices[i + 1];
    const uint32_t i2 = props.brush_indices[i + 2];
    if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
    {
      continue;
    }
    const float v0[3] = {
      props.brush_vertices[static_cast<size_t>(i0) * 3 + 0],
      props.brush_vertices[static_cast<size_t>(i0) * 3 + 1],
      props.brush_vertices[static_cast<size_t>(i0) * 3 + 2]};
    const float v1[3] = {
      props.brush_vertices[static_cast<size_t>(i1) * 3 + 0],
      props.brush_vertices[static_cast<size_t>(i1) * 3 + 1],
      props.brush_vertices[static_cast<size_t>(i1) * 3 + 2]};
    const float v2[3] = {
      props.brush_vertices[static_cast<size_t>(i2) * 3 + 0],
      props.brush_vertices[static_cast<size_t>(i2) * 3 + 1],
      props.brush_vertices[static_cast<size_t>(i2) * 3 + 2]};

    AddDebugLine(v0, v1, color);
    AddDebugLine(v1, v2, color);
    AddDebugLine(v2, v0, color);
  }
}

void AddDebugOverlayForNode(const NodeProperties& props, const LTRGBColor& color)
{
  if (!props.brush_vertices.empty() && !props.brush_indices.empty())
  {
    AddDebugBrushWire(props, color);
    return;
  }

  float bounds_min[3];
  float bounds_max[3];
  if (TryGetNodeBounds(props, bounds_min, bounds_max))
  {
    AddDebugAABB(bounds_min, bounds_max, color);
  }
}
}

void RenderViewportOverlays(const ViewportOverlayState& state)
{
  CDebugGeometry& debug = getDebugGeometry();
  debug.clear();
  if (state.count <= 0)
  {
    return;
  }
  debug.setVisible(true);
  debug.setWidth(1.0f);

  for (int i = 0; i < state.count; ++i)
  {
    const ViewportOverlayItem& item = state.items[i];
    if (!item.props)
    {
      continue;
    }
    AddDebugOverlayForNode(*item.props, item.color);
  }

  drawAllDebugGeometry();
}

void DrawAABBOverlay(
  const float bounds_min[3],
  const float bounds_max[3],
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int color,
  float thickness)
{
  const float x0 = bounds_min[0];
  const float y0 = bounds_min[1];
  const float z0 = bounds_min[2];
  const float x1 = bounds_max[0];
  const float y1 = bounds_max[1];
  const float z1 = bounds_max[2];

  const float corners[8][3] = {
    {x0, y0, z0},
    {x1, y0, z0},
    {x1, y1, z0},
    {x0, y1, z0},
    {x0, y0, z1},
    {x1, y0, z1},
    {x1, y1, z1},
    {x0, y1, z1}
  };

  static const int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
  };

  ImVec2 screen[8];
  bool visible[8] = {};
  for (int i = 0; i < 8; ++i)
  {
    visible[i] = ProjectWorldToScreen(view_proj, corners[i], viewport_size, screen[i]);
    if (visible[i])
    {
      screen[i].x += viewport_pos.x;
      screen[i].y += viewport_pos.y;
    }
  }

  for (const auto& edge : edges)
  {
    const int a = edge[0];
    const int b = edge[1];
    if (visible[a] && visible[b])
    {
      draw_list->AddLine(screen[a], screen[b], color, thickness);
    }
  }
}

void DrawBrushOverlay(
  const NodeProperties& props,
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int color,
  float thickness)
{
  const size_t vertex_count = props.brush_vertices.size() / 3;
  if (vertex_count == 0 || props.brush_indices.empty())
  {
    return;
  }

  for (size_t i = 0; i + 2 < props.brush_indices.size(); i += 3)
  {
    const uint32_t i0 = props.brush_indices[i];
    const uint32_t i1 = props.brush_indices[i + 1];
    const uint32_t i2 = props.brush_indices[i + 2];
    if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
    {
      continue;
    }
    const float v0[3] = {
      props.brush_vertices[static_cast<size_t>(i0) * 3 + 0],
      props.brush_vertices[static_cast<size_t>(i0) * 3 + 1],
      props.brush_vertices[static_cast<size_t>(i0) * 3 + 2]};
    const float v1[3] = {
      props.brush_vertices[static_cast<size_t>(i1) * 3 + 0],
      props.brush_vertices[static_cast<size_t>(i1) * 3 + 1],
      props.brush_vertices[static_cast<size_t>(i1) * 3 + 2]};
    const float v2[3] = {
      props.brush_vertices[static_cast<size_t>(i2) * 3 + 0],
      props.brush_vertices[static_cast<size_t>(i2) * 3 + 1],
      props.brush_vertices[static_cast<size_t>(i2) * 3 + 2]};

    ImVec2 screen0;
    ImVec2 screen1;
    ImVec2 screen2;
    if (!ProjectWorldToScreen(view_proj, v0, viewport_size, screen0) ||
      !ProjectWorldToScreen(view_proj, v1, viewport_size, screen1) ||
      !ProjectWorldToScreen(view_proj, v2, viewport_size, screen2))
    {
      continue;
    }
    screen0.x += viewport_pos.x;
    screen0.y += viewport_pos.y;
    screen1.x += viewport_pos.x;
    screen1.y += viewport_pos.y;
    screen2.x += viewport_pos.x;
    screen2.y += viewport_pos.y;
    draw_list->AddLine(screen0, screen1, color, thickness);
    draw_list->AddLine(screen1, screen2, color, thickness);
    draw_list->AddLine(screen2, screen0, color, thickness);
  }
}

void DrawNodeOverlay(
  const NodeProperties& props,
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int color,
  float thickness)
{
  if (!props.brush_vertices.empty() && !props.brush_indices.empty())
  {
    DrawBrushOverlay(props, view_proj, viewport_pos, viewport_size, draw_list, color, thickness);
    return;
  }

  float bounds_min[3];
  float bounds_max[3];
  if (TryGetNodeBounds(props, bounds_min, bounds_max))
  {
    DrawAABBOverlay(bounds_min, bounds_max, view_proj, viewport_pos, viewport_size, draw_list, color, thickness);
  }
}

void DrawPolygonOutline(
  const float* vertices,
  size_t vertex_count,
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int line_color,
  unsigned int vertex_color,
  float thickness)
{
  if (vertex_count == 0 || vertices == nullptr || draw_list == nullptr)
  {
    return;
  }

  // Project all vertices to screen space
  std::vector<ImVec2> screen_verts(vertex_count);
  std::vector<bool> visible(vertex_count);

  for (size_t i = 0; i < vertex_count; ++i)
  {
    const float pos[3] = {
      vertices[i * 3 + 0],
      vertices[i * 3 + 1],
      vertices[i * 3 + 2]};
    visible[i] = ProjectWorldToScreen(view_proj, pos, viewport_size, screen_verts[i]);
    if (visible[i])
    {
      screen_verts[i].x += viewport_pos.x;
      screen_verts[i].y += viewport_pos.y;
    }
  }

  // Draw edges between consecutive vertices
  for (size_t i = 0; i < vertex_count; ++i)
  {
    const size_t next = (i + 1) % vertex_count;
    if (visible[i] && visible[next])
    {
      // Don't close the polygon until complete (3+ vertices and Enter pressed)
      // For now, draw all edges including the closing edge when 3+ verts
      if (next == 0 && vertex_count < 3)
      {
        continue;  // Don't close if less than 3 vertices
      }
      if (next == 0)
      {
        // Draw closing edge with dashed style (just dimmer for now)
        draw_list->AddLine(screen_verts[i], screen_verts[next], (line_color & 0x00FFFFFF) | 0x80000000, thickness);
      }
      else
      {
        draw_list->AddLine(screen_verts[i], screen_verts[next], line_color, thickness);
      }
    }
  }

  // Draw vertex markers
  constexpr float vertex_radius = 4.0f;
  for (size_t i = 0; i < vertex_count; ++i)
  {
    if (visible[i])
    {
      draw_list->AddCircleFilled(screen_verts[i], vertex_radius, vertex_color);
      // Draw index number next to vertex
      char label[8];
      snprintf(label, sizeof(label), "%zu", i + 1);
      draw_list->AddText(ImVec2(screen_verts[i].x + 6.0f, screen_verts[i].y - 6.0f), vertex_color, label);
    }
  }
}

void DrawSplitPlaneOverlay(
  const float plane_point[3],
  const float plane_normal[3],
  float half_extent,
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int fill_color,
  unsigned int outline_color,
  float thickness)
{
  if (draw_list == nullptr || half_extent <= 0.0f)
  {
    return;
  }

  // Normalize the plane normal
  float nx = plane_normal[0];
  float ny = plane_normal[1];
  float nz = plane_normal[2];
  const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (len < 1e-6f)
  {
    return;
  }
  nx /= len;
  ny /= len;
  nz /= len;

  // Create orthonormal basis for the plane
  // Find a vector not parallel to the normal
  float up[3] = {0.0f, 1.0f, 0.0f};
  if (std::abs(ny) > 0.99f)
  {
    up[0] = 1.0f;
    up[1] = 0.0f;
    up[2] = 0.0f;
  }

  // tangent = normalize(up - (up . normal) * normal)
  const float dot = up[0] * nx + up[1] * ny + up[2] * nz;
  float tx = up[0] - dot * nx;
  float ty = up[1] - dot * ny;
  float tz = up[2] - dot * nz;
  const float tlen = std::sqrt(tx * tx + ty * ty + tz * tz);
  if (tlen < 1e-6f)
  {
    return;
  }
  tx /= tlen;
  ty /= tlen;
  tz /= tlen;

  // bitangent = normal x tangent
  const float bx = ny * tz - nz * ty;
  const float by = nz * tx - nx * tz;
  const float bz = nx * ty - ny * tx;

  // Create four corners of the plane quad
  const float corners[4][3] = {
    {plane_point[0] - tx * half_extent - bx * half_extent,
     plane_point[1] - ty * half_extent - by * half_extent,
     plane_point[2] - tz * half_extent - bz * half_extent},
    {plane_point[0] + tx * half_extent - bx * half_extent,
     plane_point[1] + ty * half_extent - by * half_extent,
     plane_point[2] + tz * half_extent - bz * half_extent},
    {plane_point[0] + tx * half_extent + bx * half_extent,
     plane_point[1] + ty * half_extent + by * half_extent,
     plane_point[2] + tz * half_extent + bz * half_extent},
    {plane_point[0] - tx * half_extent + bx * half_extent,
     plane_point[1] - ty * half_extent + by * half_extent,
     plane_point[2] - tz * half_extent + bz * half_extent}
  };

  // Project corners to screen space
  ImVec2 screen[4];
  bool visible[4] = {};
  int visible_count = 0;

  for (int i = 0; i < 4; ++i)
  {
    visible[i] = ProjectWorldToScreen(view_proj, corners[i], viewport_size, screen[i]);
    if (visible[i])
    {
      screen[i].x += viewport_pos.x;
      screen[i].y += viewport_pos.y;
      ++visible_count;
    }
  }

  // Only draw if at least 3 corners are visible
  if (visible_count < 3)
  {
    return;
  }

  // Draw filled quad if all visible
  if (visible_count == 4)
  {
    draw_list->AddQuadFilled(screen[0], screen[1], screen[2], screen[3], fill_color);
  }

  // Draw outline edges
  static const int edges[4][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
  for (const auto& edge : edges)
  {
    if (visible[edge[0]] && visible[edge[1]])
    {
      draw_list->AddLine(screen[edge[0]], screen[edge[1]], outline_color, thickness);
    }
  }

  // Draw normal direction indicator (small arrow from center)
  const float center[3] = {
    plane_point[0],
    plane_point[1],
    plane_point[2]
  };
  const float arrow_end[3] = {
    plane_point[0] + nx * half_extent * 0.3f,
    plane_point[1] + ny * half_extent * 0.3f,
    plane_point[2] + nz * half_extent * 0.3f
  };

  ImVec2 center_screen;
  ImVec2 arrow_screen;
  if (ProjectWorldToScreen(view_proj, center, viewport_size, center_screen) &&
      ProjectWorldToScreen(view_proj, arrow_end, viewport_size, arrow_screen))
  {
    center_screen.x += viewport_pos.x;
    center_screen.y += viewport_pos.y;
    arrow_screen.x += viewport_pos.x;
    arrow_screen.y += viewport_pos.y;

    // Draw arrow line
    draw_list->AddLine(center_screen, arrow_screen, outline_color, thickness * 1.5f);

    // Draw arrowhead
    const float dx = arrow_screen.x - center_screen.x;
    const float dy = arrow_screen.y - center_screen.y;
    const float arrow_len = std::sqrt(dx * dx + dy * dy);
    if (arrow_len > 5.0f)
    {
      const float head_size = 8.0f;
      const float ndx = dx / arrow_len;
      const float ndy = dy / arrow_len;
      const ImVec2 p1(arrow_screen.x - ndx * head_size - ndy * head_size * 0.5f,
                      arrow_screen.y - ndy * head_size + ndx * head_size * 0.5f);
      const ImVec2 p2(arrow_screen.x - ndx * head_size + ndy * head_size * 0.5f,
                      arrow_screen.y - ndy * head_size - ndx * head_size * 0.5f);
      draw_list->AddTriangleFilled(arrow_screen, p1, p2, outline_color);
    }
  }
}
