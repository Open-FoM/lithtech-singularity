#include "diligent_2d_draw.h"

#include "diligent_device.h"
#include "diligent_state.h"
#include "diligent_utils.h"

#include "diligent_shaders_generated.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"

#include <array>
#include <cmath>
#include <cstring>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
constexpr uint32 kDiligentSurfaceFlagOptimized = 1u << 0;
constexpr uint32 kDiligentSurfaceFlagDirty = 1u << 1;

struct DiligentSurface
{
	uint32 width;
	uint32 height;
	uint32 pitch;
	uint32 flags;
	uint32 optimized_transparent_color;
	uint32 data[1];
};

struct DiligentSurfaceGpuData
{
	Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> srv;
	uint32 width = 0;
	uint32 height = 0;
	uint32 transparent_color = OPTIMIZE_NO_TRANSPARENCY;
};

struct DiligentOptimized2DPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct DiligentOptimized2DResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader;
	std::unordered_map<int32, DiligentOptimized2DPipeline> pipelines;
	std::unordered_map<int32, DiligentOptimized2DPipeline> clamped_pipelines;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	uint32 vertex_buffer_size = 0;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	float output_is_srgb = -1.0f;
};

struct DiligentOptimized2DConstants
{
	float output_params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

DiligentOptimized2DResources g_optimized_2d_resources;
std::unordered_map<HLTBUFFER, DiligentSurfaceGpuData> g_surface_cache;
LTSurfaceBlend g_optimized_2d_blend = LTSURFACEBLEND_ALPHA;
HLTCOLOR g_optimized_2d_color = 0xFFFFFFFF;

Diligent::BLEND_FACTOR diligent_map_blend_factor(LTSurfaceBlend blend, bool is_source)
{
	switch (blend)
	{
		case LTSURFACEBLEND_ALPHA:
			return is_source ? Diligent::BLEND_FACTOR_SRC_ALPHA : Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
		case LTSURFACEBLEND_SOLID:
			return is_source ? Diligent::BLEND_FACTOR_ONE : Diligent::BLEND_FACTOR_ZERO;
		case LTSURFACEBLEND_ADD:
			return Diligent::BLEND_FACTOR_ONE;
		case LTSURFACEBLEND_MULTIPLY:
			return is_source ? Diligent::BLEND_FACTOR_DEST_COLOR : Diligent::BLEND_FACTOR_ZERO;
		case LTSURFACEBLEND_MULTIPLY2:
			return is_source ? Diligent::BLEND_FACTOR_DEST_COLOR : Diligent::BLEND_FACTOR_SRC_COLOR;
		case LTSURFACEBLEND_MASK:
			return is_source ? Diligent::BLEND_FACTOR_ZERO : Diligent::BLEND_FACTOR_INV_SRC_COLOR;
		case LTSURFACEBLEND_MASKADD:
			return is_source ? Diligent::BLEND_FACTOR_ONE : Diligent::BLEND_FACTOR_INV_SRC_COLOR;
		default:
			return is_source ? Diligent::BLEND_FACTOR_SRC_ALPHA : Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
	}
}

bool diligent_ensure_optimized_2d_shaders()
{
	if (g_optimized_2d_resources.vertex_shader && g_optimized_2d_resources.pixel_shader)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	Diligent::ShaderDesc vertex_desc;
	vertex_desc.Name = "ltjs_optimized_2d_vs";
	vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
	shader_info.Desc = vertex_desc;
	shader_info.EntryPoint = "VSMain";
	shader_info.Source = diligent_get_optimized_2d_vertex_shader_source();
	g_diligent_state.render_device->CreateShader(shader_info, &g_optimized_2d_resources.vertex_shader);
	if (!g_optimized_2d_resources.vertex_shader)
	{
		return false;
	}

	Diligent::ShaderDesc pixel_desc;
	pixel_desc.Name = "ltjs_optimized_2d_ps";
	pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	shader_info.Desc = pixel_desc;
	shader_info.EntryPoint = "PSMain";
	shader_info.Source = diligent_get_optimized_2d_pixel_shader_source();
	g_diligent_state.render_device->CreateShader(shader_info, &g_optimized_2d_resources.pixel_shader);
	return g_optimized_2d_resources.pixel_shader != nullptr;
}

bool diligent_ensure_optimized_2d_constants()
{
	if (g_optimized_2d_resources.constant_buffer)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_optimized_2d_constants";
	desc.Size = sizeof(DiligentOptimized2DConstants);
	desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_optimized_2d_resources.constant_buffer);
	return g_optimized_2d_resources.constant_buffer != nullptr;
}

bool diligent_update_optimized_2d_constants(float output_is_srgb)
{
	if (!diligent_ensure_optimized_2d_constants() || !g_diligent_state.immediate_context)
	{
		return false;
	}

	if (std::fabs(g_optimized_2d_resources.output_is_srgb - output_is_srgb) < 0.001f)
	{
		return true;
	}

	DiligentOptimized2DConstants constants{};
	constants.output_params[0] = output_is_srgb;

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_optimized_2d_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_optimized_2d_resources.constant_buffer, Diligent::MAP_WRITE);
	g_optimized_2d_resources.output_is_srgb = output_is_srgb;
	return true;
}

bool diligent_ensure_optimized_2d_vertex_buffer(uint32 size)
{
	if (g_optimized_2d_resources.vertex_buffer && g_optimized_2d_resources.vertex_buffer_size >= size)
	{
		return true;
	}

	if (!g_diligent_state.render_device || size == 0)
	{
		return false;
	}

	g_optimized_2d_resources.vertex_buffer.Release();
	g_optimized_2d_resources.vertex_buffer_size = 0;

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_optimized_2d_vertices";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = sizeof(DiligentOptimized2DVertex);

	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_optimized_2d_resources.vertex_buffer);
	if (!g_optimized_2d_resources.vertex_buffer)
	{
		return false;
	}

	g_optimized_2d_resources.vertex_buffer_size = size;
	return true;
}

DiligentOptimized2DPipeline* diligent_get_optimized_2d_pipeline(LTSurfaceBlend blend, bool clamp_sampler)
{
	const uint32 sample_count = diligent_get_active_sample_count();
	const int32 pipeline_key = static_cast<int32>((static_cast<uint32>(blend) & 0xFFFFu) | (sample_count << 16));
	auto& pipelines = clamp_sampler ? g_optimized_2d_resources.clamped_pipelines : g_optimized_2d_resources.pipelines;
	auto it = pipelines.find(pipeline_key);
	if (it != pipelines.end())
	{
		return &it->second;
	}

	if (!diligent_ensure_optimized_2d_shaders())
	{
		return nullptr;
	}

	if (!g_diligent_state.render_device || !g_diligent_state.swap_chain)
	{
		return nullptr;
	}

	const auto swap_desc = g_diligent_state.swap_chain->GetDesc();

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_optimized_2d_pso";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = swap_desc.ColorBufferFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = swap_desc.DepthBufferFormat;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pipeline_info.GraphicsPipeline.SmplDesc.Count = static_cast<Diligent::Uint8>(sample_count);

	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));

	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	pipeline_info.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;

	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;

	diligent_fill_optimized_2d_blend_desc(blend, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);

	pipeline_info.pVS = g_optimized_2d_resources.vertex_shader;
	pipeline_info.pPS = g_optimized_2d_resources.pixel_shader;

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::ShaderResourceVariableDesc variables[] = {
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};
	pipeline_info.PSODesc.ResourceLayout.Variables = variables;
	pipeline_info.PSODesc.ResourceLayout.NumVariables = static_cast<uint32>(std::size(variables));

	Diligent::ImmutableSamplerDesc sampler_desc;
	sampler_desc.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.AddressU = clamp_sampler ? Diligent::TEXTURE_ADDRESS_CLAMP : Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc.Desc.AddressV = clamp_sampler ? Diligent::TEXTURE_ADDRESS_CLAMP : Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc.Desc.AddressW = clamp_sampler ? Diligent::TEXTURE_ADDRESS_CLAMP : Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc.SamplerOrTextureName = "g_Texture_sampler";
	diligent_apply_anisotropy(sampler_desc.Desc);
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler_desc;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

	DiligentOptimized2DPipeline pipeline;
	g_diligent_state.render_device->CreatePipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	if (!diligent_ensure_optimized_2d_constants())
	{
		return nullptr;
	}

	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Optimized2DConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_optimized_2d_resources.constant_buffer);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	auto result = pipelines.emplace(pipeline_key, std::move(pipeline));
	return result.second ? &result.first->second : nullptr;
}

bool diligent_build_surface_upload_data(
	const DiligentSurface& surface,
	uint32 transparent_color,
	std::vector<uint32>& out_data,
	uint32& out_stride)
{
	if (surface.width == 0 || surface.height == 0)
	{
		return false;
	}

	out_stride = surface.width * sizeof(uint32);
	out_data.resize(static_cast<size_t>(surface.width) * surface.height);

	const uint32 transparent_mask = transparent_color & 0x00FFFFFFu;
	const bool use_transparency = transparent_color != OPTIMIZE_NO_TRANSPARENCY;
	const size_t pixel_count = out_data.size();

	for (size_t i = 0; i < pixel_count; ++i)
	{
		uint32 pixel = surface.data[i] & 0x00FFFFFFu;
		uint32 alpha = 0xFF000000u;
		if (use_transparency && pixel == transparent_mask)
		{
			alpha = 0x00000000u;
		}
		out_data[i] = pixel | alpha;
	}

	return true;
}

bool diligent_upload_surface_texture(DiligentSurface& surface, DiligentSurfaceGpuData& gpu_data)
{
	if (!g_diligent_state.render_device)
	{
		return false;
	}

	std::vector<uint32> converted;
	uint32 stride = 0;
	if (!diligent_build_surface_upload_data(surface, surface.optimized_transparent_color, converted, stride))
	{
		return false;
	}

	const bool can_update = gpu_data.texture && gpu_data.width == surface.width && gpu_data.height == surface.height;
	if (can_update)
	{
		if (!g_diligent_state.immediate_context)
		{
			return false;
		}

		Diligent::Box dst_box{0, surface.width, 0, surface.height};
		Diligent::TextureSubResData subresource;
		subresource.pData = converted.data();
		subresource.Stride = static_cast<Diligent::Uint64>(stride);
		subresource.DepthStride = 0;

		g_diligent_state.immediate_context->UpdateTexture(
			gpu_data.texture,
			0,
			0,
			dst_box,
			subresource,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		if (!gpu_data.srv)
		{
			gpu_data.srv = gpu_data.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		gpu_data.transparent_color = surface.optimized_transparent_color;
		surface.flags &= ~kDiligentSurfaceFlagDirty;
		return gpu_data.srv != nullptr;
	}

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_optimized_surface";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = surface.width;
	desc.Height = surface.height;
	desc.MipLevels = 1;
	desc.Format = Diligent::TEX_FORMAT_BGRA8_UNORM;
	desc.Usage = Diligent::USAGE_DEFAULT;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	Diligent::TextureSubResData subresource;
	subresource.pData = converted.data();
	subresource.Stride = static_cast<Diligent::Uint64>(stride);
	subresource.DepthStride = 0;

	Diligent::TextureData init_data{&subresource, 1};
	gpu_data.texture.Release();
	gpu_data.srv.Release();
	g_diligent_state.render_device->CreateTexture(desc, &init_data, &gpu_data.texture);
	if (!gpu_data.texture)
	{
		return false;
	}

	gpu_data.srv = gpu_data.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	gpu_data.width = surface.width;
	gpu_data.height = surface.height;
	gpu_data.transparent_color = surface.optimized_transparent_color;
	surface.flags &= ~kDiligentSurfaceFlagDirty;
	return gpu_data.srv != nullptr;
}
}

void diligent_fill_optimized_2d_blend_desc(LTSurfaceBlend blend, Diligent::RenderTargetBlendDesc& blend_desc)
{
	blend_desc.BlendEnable = true;
	blend_desc.RenderTargetWriteMask = Diligent::COLOR_MASK_ALL;
	blend_desc.SrcBlend = diligent_map_blend_factor(blend, true);
	blend_desc.DestBlend = diligent_map_blend_factor(blend, false);
	blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
	blend_desc.SrcBlendAlpha = blend_desc.SrcBlend;
	blend_desc.DestBlendAlpha = blend_desc.DestBlend;
	blend_desc.BlendOpAlpha = blend_desc.BlendOp;
}

bool diligent_upload_optimized_2d_vertices(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	Diligent::IBuffer*& out_buffer)
{
	out_buffer = nullptr;
	if (!vertices || vertex_count == 0 || !g_diligent_state.immediate_context)
	{
		return false;
	}

	const uint32 data_size = vertex_count * sizeof(DiligentOptimized2DVertex);
	if (!diligent_ensure_optimized_2d_vertex_buffer(data_size))
	{
		return false;
	}

	void* mapped_vertices = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	if (!mapped_vertices)
	{
		return false;
	}
	std::memcpy(mapped_vertices, vertices, data_size);
	g_diligent_state.immediate_context->UnmapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE);
	out_buffer = g_optimized_2d_resources.vertex_buffer;
	return true;
}

bool diligent_draw_optimized_2d_vertices(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view)
{
	if (!vertices || vertex_count == 0 || !texture_view)
	{
		return false;
	}

	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	auto* pipeline = diligent_get_optimized_2d_pipeline(blend, false);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	if (!diligent_update_optimized_2d_constants(diligent_get_swapchain_output_is_srgb()))
	{
		return false;
	}

	Diligent::IBuffer* vertex_buffer = nullptr;
	if (!diligent_upload_optimized_2d_vertices(vertices, vertex_count, vertex_buffer))
	{
		return false;
	}

	auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture");
	if (texture_var)
	{
		texture_var->Set(texture_view);
	}

	auto* render_target = diligent_get_active_render_target();
	auto* depth_target = diligent_get_active_depth_target();
	g_diligent_state.immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);

	Diligent::IBuffer* buffers[] = {vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

	g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = vertex_count;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->Draw(draw_attribs);
	return true;
}

bool diligent_draw_optimized_2d_vertices_to_target(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target,
	bool clamp_sampler)
{
	if (!vertices || vertex_count == 0 || !texture_view || !render_target)
	{
		return false;
	}

	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	auto* pipeline = diligent_get_optimized_2d_pipeline(blend, clamp_sampler);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	if (!diligent_update_optimized_2d_constants(0.0f))
	{
		return false;
	}

	Diligent::IBuffer* vertex_buffer = nullptr;
	if (!diligent_upload_optimized_2d_vertices(vertices, vertex_count, vertex_buffer))
	{
		return false;
	}

	auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture");
	if (texture_var)
	{
		texture_var->Set(texture_view);
	}

	g_diligent_state.immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);

	Diligent::IBuffer* buffers[] = {vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

	g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = vertex_count;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->Draw(draw_attribs);
	return true;
}

HLTBUFFER diligent_CreateSurface(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return LTNULL;
	}

	const auto pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
	const auto surface_size = sizeof(DiligentSurface) + (pixel_count - 1) * sizeof(uint32);
	DiligentSurface* surface = nullptr;
	LT_MEM_TRACK_ALLOC(surface = static_cast<DiligentSurface*>(LTMemAlloc(surface_size)), LT_MEM_TYPE_RENDERER);
	if (!surface)
	{
		return LTNULL;
	}

	surface->width = static_cast<uint32>(width);
	surface->height = static_cast<uint32>(height);
	surface->pitch = static_cast<uint32>(width) * sizeof(uint32);
	surface->flags = 0;
	surface->optimized_transparent_color = OPTIMIZE_NO_TRANSPARENCY;
	std::memset(surface->data, 0, pixel_count * sizeof(uint32));
	return static_cast<HLTBUFFER>(surface);
}

void diligent_DeleteSurface(HLTBUFFER surface_handle)
{
	if (!surface_handle)
	{
		return;
	}

	g_surface_cache.erase(surface_handle);
	LTMemFree(surface_handle);
}

void diligent_GetSurfaceInfo(HLTBUFFER surface_handle, uint32* width, uint32* height)
{
	const auto* surface = static_cast<const DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return;
	}

	if (width)
	{
		*width = surface->width;
	}

	if (height)
	{
		*height = surface->height;
	}
}

void* diligent_LockSurface(HLTBUFFER surface_handle, uint32& pitch)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		pitch = 0;
		return nullptr;
	}

	pitch = surface->pitch;
	return surface->data;
}

void diligent_UnlockSurface(HLTBUFFER surface_handle)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (surface)
	{
		surface->flags |= kDiligentSurfaceFlagDirty;
	}
}

bool diligent_OptimizeSurface(HLTBUFFER surface_handle, uint32 transparent_color)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return false;
	}

	surface->flags |= kDiligentSurfaceFlagOptimized | kDiligentSurfaceFlagDirty;
	surface->optimized_transparent_color = transparent_color;
	return true;
}

void diligent_UnoptimizeSurface(HLTBUFFER surface_handle)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return;
	}

	surface->flags &= ~kDiligentSurfaceFlagOptimized;
	g_surface_cache.erase(surface_handle);
}

bool diligent_GetScreenFormat(PFormat* format)
{
	if (!format)
	{
		return false;
	}

	format->Init(BPP_32, 0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu);
	return true;
}

bool diligent_StartOptimized2D()
{
	g_diligent_state.is_in_optimized_2d = true;
	if (g_diligent_state.render_struct)
	{
		++g_diligent_state.render_struct->m_nInOptimized2D;
	}
	return true;
}

void diligent_EndOptimized2D()
{
	g_diligent_state.is_in_optimized_2d = false;
	if (g_diligent_state.render_struct && g_diligent_state.render_struct->m_nInOptimized2D > 0)
	{
		--g_diligent_state.render_struct->m_nInOptimized2D;
	}
}

bool diligent_IsInOptimized2D()
{
	return g_diligent_state.is_in_optimized_2d;
}

bool diligent_SetOptimized2DBlend(LTSurfaceBlend blend)
{
	g_optimized_2d_blend = blend;
	return true;
}

bool diligent_GetOptimized2DBlend(LTSurfaceBlend& blend)
{
	blend = g_optimized_2d_blend;
	return true;
}

bool diligent_SetOptimized2DColor(HLTCOLOR color)
{
	g_optimized_2d_color = 0xFF000000u | (color & 0x00FFFFFFu);
	return true;
}

bool diligent_GetOptimized2DColor(HLTCOLOR& color)
{
	color = g_optimized_2d_color;
	return true;
}

void diligent_BlitToScreen(BlitRequest* request)
{
	if (!request || !request->m_hBuffer || !request->m_pSrcRect || !request->m_pDestRect)
	{
		return;
	}

	if (g_diligent_state.screen_width == 0 || g_diligent_state.screen_height == 0)
	{
		return;
	}

	auto* surface = static_cast<DiligentSurface*>(request->m_hBuffer);
	if (!surface)
	{
		return;
	}

	if ((surface->flags & kDiligentSurfaceFlagOptimized) == 0)
	{
		const uint32 transparent_color = (request->m_BlitOptions & BLIT_TRANSPARENT)
			? request->m_TransparentColor.dwVal
			: OPTIMIZE_NO_TRANSPARENCY;
		diligent_OptimizeSurface(request->m_hBuffer, transparent_color);
	}

	auto& gpu_data = g_surface_cache[request->m_hBuffer];
	const bool needs_upload = !gpu_data.srv || gpu_data.width != surface->width || gpu_data.height != surface->height ||
		gpu_data.transparent_color != surface->optimized_transparent_color || (surface->flags & kDiligentSurfaceFlagDirty);
	if (needs_upload && !diligent_upload_surface_texture(*surface, gpu_data))
	{
		return;
	}

	if (!gpu_data.srv)
	{
		return;
	}

	const float red = static_cast<float>((g_optimized_2d_color >> 16) & 0xFF) / 255.0f;
	const float green = static_cast<float>((g_optimized_2d_color >> 8) & 0xFF) / 255.0f;
	const float blue = static_cast<float>(g_optimized_2d_color & 0xFF) / 255.0f;
	const float alpha = request->m_Alpha;

	const auto& src = *request->m_pSrcRect;
	const auto& dest = *request->m_pDestRect;
	const float u0 = static_cast<float>(src.left) / static_cast<float>(surface->width);
	const float v0 = static_cast<float>(src.top) / static_cast<float>(surface->height);
	const float u1 = static_cast<float>(src.right) / static_cast<float>(surface->width);
	const float v1 = static_cast<float>(src.bottom) / static_cast<float>(surface->height);

	float left = 0.0f;
	float top = 0.0f;
	float right = 0.0f;
	float bottom = 0.0f;
	float right_top = 0.0f;
	float right_bottom = 0.0f;
	diligent_to_clip_space(static_cast<float>(dest.left), static_cast<float>(dest.top), left, top);
	diligent_to_clip_space(static_cast<float>(dest.right), static_cast<float>(dest.bottom), right_bottom, bottom);
	diligent_to_clip_space(static_cast<float>(dest.right), static_cast<float>(dest.top), right_top, top);

	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{left, top}, {red, green, blue, alpha}, {u0, v0}};
	vertices[1] = {{right_top, top}, {red, green, blue, alpha}, {u1, v0}};
	vertices[2] = {{left, bottom}, {red, green, blue, alpha}, {u0, v1}};
	vertices[3] = {{right_bottom, bottom}, {red, green, blue, alpha}, {u1, v1}};

	diligent_draw_optimized_2d_vertices(vertices, 4, g_optimized_2d_blend, gpu_data.srv);
}

void diligent_BlitFromScreen(BlitRequest*)
{
}

bool diligent_WarpToScreen(BlitRequest* request)
{
	if (!request || !request->m_hBuffer || !request->m_pSrcRect || !request->m_pWarpPts)
	{
		return false;
	}

	if (request->m_nWarpPts < 4)
	{
		return false;
	}

	if (g_diligent_state.screen_width == 0 || g_diligent_state.screen_height == 0)
	{
		return false;
	}

	auto* surface = static_cast<DiligentSurface*>(request->m_hBuffer);
	if (!surface)
	{
		return false;
	}

	if ((surface->flags & kDiligentSurfaceFlagOptimized) == 0)
	{
		const uint32 transparent_color = (request->m_BlitOptions & BLIT_TRANSPARENT)
			? request->m_TransparentColor.dwVal
			: OPTIMIZE_NO_TRANSPARENCY;
		diligent_OptimizeSurface(request->m_hBuffer, transparent_color);
	}

	auto& gpu_data = g_surface_cache[request->m_hBuffer];
	const bool needs_upload = !gpu_data.srv || gpu_data.width != surface->width || gpu_data.height != surface->height ||
		gpu_data.transparent_color != surface->optimized_transparent_color || (surface->flags & kDiligentSurfaceFlagDirty);
	if (needs_upload && !diligent_upload_surface_texture(*surface, gpu_data))
	{
		return false;
	}

	if (!gpu_data.srv)
	{
		return false;
	}

	const float red = static_cast<float>((g_optimized_2d_color >> 16) & 0xFF) / 255.0f;
	const float green = static_cast<float>((g_optimized_2d_color >> 8) & 0xFF) / 255.0f;
	const float blue = static_cast<float>(g_optimized_2d_color & 0xFF) / 255.0f;
	const float alpha = request->m_Alpha;

	const auto& src = *request->m_pSrcRect;
	const float u0 = static_cast<float>(src.left) / static_cast<float>(surface->width);
	const float v0 = static_cast<float>(src.top) / static_cast<float>(surface->height);
	const float u1 = static_cast<float>(src.right) / static_cast<float>(surface->width);
	const float v1 = static_cast<float>(src.bottom) / static_cast<float>(surface->height);

	const LTWarpPt& p0 = request->m_pWarpPts[0];
	const LTWarpPt& p1 = request->m_pWarpPts[1];
	const LTWarpPt& p2 = request->m_pWarpPts[2];
	const LTWarpPt& p3 = request->m_pWarpPts[3];

	float x0 = 0.0f;
	float y0 = 0.0f;
	float x1 = 0.0f;
	float y1 = 0.0f;
	float x2 = 0.0f;
	float y2 = 0.0f;
	float x3 = 0.0f;
	float y3 = 0.0f;
	diligent_to_clip_space(p0.dest_x, p0.dest_y, x0, y0);
	diligent_to_clip_space(p1.dest_x, p1.dest_y, x1, y1);
	diligent_to_clip_space(p2.dest_x, p2.dest_y, x2, y2);
	diligent_to_clip_space(p3.dest_x, p3.dest_y, x3, y3);

	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{x0, y0}, {red, green, blue, alpha}, {u0, v0}};
	vertices[1] = {{x1, y1}, {red, green, blue, alpha}, {u1, v0}};
	vertices[2] = {{x3, y3}, {red, green, blue, alpha}, {u0, v1}};
	vertices[3] = {{x2, y2}, {red, green, blue, alpha}, {u1, v1}};

	return diligent_draw_optimized_2d_vertices(vertices, 4, g_optimized_2d_blend, gpu_data.srv);
}

void diligent_release_optimized_2d_resources()
{
	g_surface_cache.clear();
	g_optimized_2d_resources.pipelines.clear();
	g_optimized_2d_resources.clamped_pipelines.clear();
	g_optimized_2d_resources.vertex_shader.Release();
	g_optimized_2d_resources.pixel_shader.Release();
	g_optimized_2d_resources.vertex_buffer.Release();
	g_optimized_2d_resources.vertex_buffer_size = 0;
	g_optimized_2d_resources.constant_buffer.Release();
	g_optimized_2d_resources.output_is_srgb = -1.0f;
}
