#pragma once

/// Marker (construction origin) renderer for DEdit2 viewports.
///
/// The marker is a 3D crosshair that indicates the construction origin point
/// where new primitives will be created. It appears as a yellow cross in all
/// viewports and maintains a consistent screen-space size.

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"

struct MarkerRenderer
{
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline;
  Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
  Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
  Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
  uint32_t vertex_count = 0;
};

/// Initialize the marker renderer with graphics resources.
/// @param device Render device for creating GPU resources.
/// @param rtv_format Render target format.
/// @param dsv_format Depth stencil format.
/// @param renderer Output renderer to initialize.
/// @return true if initialization succeeded.
bool InitMarkerRenderer(
    Diligent::IRenderDevice* device,
    Diligent::TEXTURE_FORMAT rtv_format,
    Diligent::TEXTURE_FORMAT dsv_format,
    MarkerRenderer& renderer);

/// Update marker constants (position and view-projection matrix).
/// @param context Device context for buffer updates.
/// @param renderer Marker renderer to update.
/// @param marker_position World-space marker position [3].
/// @param view_proj View-projection matrix.
/// @param scale Scale factor for marker size (based on camera distance).
void UpdateMarkerConstants(
    Diligent::IDeviceContext* context,
    MarkerRenderer& renderer,
    const float marker_position[3],
    const Diligent::float4x4& view_proj,
    float scale);

/// Draw the marker crosshair.
/// @param context Device context for rendering.
/// @param renderer Marker renderer to draw with.
/// @param visible Whether the marker should be drawn.
void DrawMarker(
    Diligent::IDeviceContext* context,
    MarkerRenderer& renderer,
    bool visible);
