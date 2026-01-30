#pragma once

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "ltbasedefs.h"
#include "renderstruct.h"

#include <cstdint>

struct ImDrawList;
struct ImVec2;
struct NodeProperties;

struct ViewportOverlayItem
{
  const NodeProperties* props = nullptr;
  LTRGBColor color{};
};

struct ViewportOverlayState
{
  static constexpr int kMaxOverlays = 32;
  ViewportOverlayItem items[kMaxOverlays];
  int count = 0;
};

LTRGBColor MakeOverlayColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void RenderViewportOverlays(const ViewportOverlayState& state);

void DrawAABBOverlay(
  const float bounds_min[3],
  const float bounds_max[3],
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int color,
  float thickness);

void DrawBrushOverlay(
  const NodeProperties& props,
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int color,
  float thickness);

void DrawNodeOverlay(
  const NodeProperties& props,
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int color,
  float thickness);

/// Draw a polygon outline during manual polygon drawing mode.
/// @param vertices Array of XYZ triplets (world-space vertex positions).
/// @param vertex_count Number of vertices (size of array / 3).
/// @param view_proj The view-projection matrix.
/// @param viewport_pos Top-left corner of the viewport in screen coordinates.
/// @param viewport_size Size of the viewport.
/// @param draw_list ImGui draw list to render to.
/// @param line_color Color of the polygon edges.
/// @param vertex_color Color of the vertex markers.
/// @param thickness Line thickness.
void DrawPolygonOutline(
  const float* vertices,
  size_t vertex_count,
  const Diligent::float4x4& view_proj,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  ImDrawList* draw_list,
  unsigned int line_color,
  unsigned int vertex_color,
  float thickness);

/// Draw a split plane preview overlay.
/// The plane is rendered as a semi-transparent quad with an outline.
/// @param plane_point A point on the plane (world space).
/// @param plane_normal The plane normal (normalized).
/// @param half_extent The half-size of the plane quad.
/// @param view_proj The view-projection matrix.
/// @param viewport_pos Top-left corner of the viewport in screen coordinates.
/// @param viewport_size Size of the viewport.
/// @param draw_list ImGui draw list to render to.
/// @param fill_color Color of the plane fill (with alpha for transparency).
/// @param outline_color Color of the plane outline.
/// @param thickness Line thickness for the outline.
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
  float thickness);
