#include "viewport/overlays.h"

#include "editor_state.h"
#include "viewport_picking.h"
#include "viewport_gizmo.h"

#include "debuggeometry.h"
#include "imgui.h"
#include "ltvector.h"

#include <cstdint>

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
