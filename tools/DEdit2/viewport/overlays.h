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
  ViewportOverlayItem items[2];
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
