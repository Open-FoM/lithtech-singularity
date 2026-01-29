#pragma once

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Shader.h"

struct ViewportPanelState;

/// Grid plane orientation for view-aware grid rendering.
enum class GridPlane
{
	XZ, ///< Horizontal plane (Y=0) - for Top/Bottom/Perspective views
	XY, ///< Vertical plane (Z=0) - for Front/Back views
	YZ  ///< Vertical plane (X=0) - for Left/Right views
};

struct ViewportGridRenderer
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	Diligent::IRenderDevice* device = nullptr; ///< Cached device for buffer rebuild
	uint32_t grid_vertex_count = 0;
	uint32_t axis_vertex_count = 0;
	float grid_half_extent = 2048.0f;
	float grid_spacing = 64.0f;
	int major_line_every = 4;

	/// Cached state for detecting when grid needs rebuild
	GridPlane cached_plane = GridPlane::XZ;
	float cached_spacing = 64.0f;
};

bool InitViewportGridRenderer(
	Diligent::IRenderDevice* device,
	Diligent::TEXTURE_FORMAT rtv_format,
	Diligent::TEXTURE_FORMAT dsv_format,
	ViewportGridRenderer& renderer);

void UpdateViewportGridConstants(
	Diligent::IDeviceContext* context,
	ViewportGridRenderer& renderer,
	const Diligent::float4x4& view_proj,
	const Diligent::float3& grid_origin);

void DrawViewportGrid(
	Diligent::IDeviceContext* context,
	ViewportGridRenderer& renderer,
	bool draw_grid,
	bool draw_axes);

/// Rebuild grid vertex buffer if spacing or plane has changed.
/// Call before drawing to ensure grid matches current viewport state.
void RebuildGridIfNeeded(
	ViewportGridRenderer& renderer,
	GridPlane plane,
	float spacing);

/// Get the appropriate grid plane for a viewport view mode.
[[nodiscard]] GridPlane GetGridPlaneForViewMode(const ViewportPanelState& state);

Diligent::float4x4 ComputeViewportViewProj(
	const ViewportPanelState& state,
	float aspect_ratio);
