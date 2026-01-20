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

struct ViewportGridRenderer
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	uint32_t grid_vertex_count = 0;
	uint32_t axis_vertex_count = 0;
	float grid_half_extent = 2048.0f;
	float grid_spacing = 64.0f;
	int major_line_every = 4;
};

bool InitViewportGridRenderer(
	Diligent::IRenderDevice* device,
	Diligent::TEXTURE_FORMAT rtv_format,
	Diligent::TEXTURE_FORMAT dsv_format,
	ViewportGridRenderer& renderer);

void UpdateViewportGridConstants(
	Diligent::IDeviceContext* context,
	ViewportGridRenderer& renderer,
	const Diligent::float4x4& view_proj);

void DrawViewportGrid(
	Diligent::IDeviceContext* context,
	ViewportGridRenderer& renderer,
	bool draw_grid,
	bool draw_axes);

Diligent::float4x4 ComputeViewportViewProj(
	const ViewportPanelState& state,
	float aspect_ratio);
