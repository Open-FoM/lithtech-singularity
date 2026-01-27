#include "diligent_ssgi_fx.h"

#include "diligent_2d_draw.h"
#include "diligent_device.h"
#include "diligent_internal.h"
#include "diligent_state.h"
#include "diligent_utils.h"

#include "bdefs.h"
#include "renderstruct.h"

#include "../rendererconsolevars.h"

#include "diligent_shaders_generated.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "MapHelper.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace
{
struct DiligentSsgiConstants
{
	float inv_target_size[2] = {0.0f, 0.0f};
	float radius = 12.0f;
	float intensity = 1.0f;
	float depth_reject = 0.02f;
	float normal_reject = 0.4f;
	int32 sample_count = 8;
	float pad0 = 0.0f;
	float pad1 = 0.0f;
};

struct DiligentSsgiTemporalConstants
{
	float inv_target_size[2] = {0.0f, 0.0f};
	float blend_factor = 0.85f;
	float depth_reject = 0.02f;
	float normal_reject = 0.4f;
	float pad0[3] = {0.0f, 0.0f, 0.0f};
};

struct DiligentSsgiTargets
{
	uint32 width = 0;
	uint32 height = 0;
	Diligent::TEXTURE_FORMAT format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::RefCntAutoPtr<Diligent::ITexture> current;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> current_rtv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> current_srv;
	Diligent::RefCntAutoPtr<Diligent::ITexture> history;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> history_rtv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> history_srv;
	bool history_valid = false;
};

struct DiligentSsgiPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	Diligent::TEXTURE_FORMAT rtv_format = Diligent::TEX_FORMAT_UNKNOWN;
};

struct DiligentSsgiResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> ssgi_pixel_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> temporal_pixel_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> apply_pixel_shader;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> ssgi_constants;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> temporal_constants;
	DiligentSsgiPipeline ssgi_pipeline;
	DiligentSsgiPipeline temporal_pipeline;
	DiligentSsgiPipeline apply_pipeline;
};

DiligentSsgiTargets g_ssgi_targets;
DiligentSsgiResources g_ssgi_resources;

bool diligent_ssgi_ensure_targets(uint32 width, uint32 height, Diligent::TEXTURE_FORMAT format)
{
	if (g_ssgi_targets.width == width && g_ssgi_targets.height == height &&
		g_ssgi_targets.format == format && g_ssgi_targets.current && g_ssgi_targets.history)
	{
		return true;
	}

	g_ssgi_targets = {};

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	g_ssgi_targets.width = width;
	g_ssgi_targets.height = height;
	g_ssgi_targets.format = format;

	Diligent::TextureDesc desc;
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.Usage = Diligent::USAGE_DEFAULT;
	desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

	desc.Name = "ltjs_ssgi_current";
	g_diligent_state.render_device->CreateTexture(desc, nullptr, &g_ssgi_targets.current);
	if (!g_ssgi_targets.current)
	{
		return false;
	}
	g_ssgi_targets.current_rtv = g_ssgi_targets.current->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	g_ssgi_targets.current_srv = g_ssgi_targets.current->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!g_ssgi_targets.current_rtv || !g_ssgi_targets.current_srv)
	{
		return false;
	}

	desc.Name = "ltjs_ssgi_history";
	g_diligent_state.render_device->CreateTexture(desc, nullptr, &g_ssgi_targets.history);
	if (!g_ssgi_targets.history)
	{
		return false;
	}
	g_ssgi_targets.history_rtv = g_ssgi_targets.history->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	g_ssgi_targets.history_srv = g_ssgi_targets.history->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!g_ssgi_targets.history_rtv || !g_ssgi_targets.history_srv)
	{
		return false;
	}

	g_ssgi_targets.history_valid = false;
	return true;
}

bool diligent_ssgi_ensure_resources()
{
	if (!g_diligent_state.render_device)
	{
		return false;
	}

	if (!g_ssgi_resources.vertex_shader)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "VSMain";

		Diligent::ShaderDesc vertex_desc;
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		vertex_desc.Name = "ltjs_ssgi_vs";
		shader_info.Desc = vertex_desc;
		shader_info.Source = diligent_get_optimized_2d_vertex_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssgi_resources.vertex_shader);
	}

	if (!g_ssgi_resources.ssgi_pixel_shader)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "PSMain";

		Diligent::ShaderDesc pixel_desc;
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		pixel_desc.Name = "ltjs_ssgi_ps";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_ssgi_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssgi_resources.ssgi_pixel_shader);
	}

	if (!g_ssgi_resources.temporal_pixel_shader)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "PSMain";

		Diligent::ShaderDesc pixel_desc;
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		pixel_desc.Name = "ltjs_ssgi_temporal_ps";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_ssgi_temporal_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssgi_resources.temporal_pixel_shader);
	}

	if (!g_ssgi_resources.apply_pixel_shader)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "PSMain";

		Diligent::ShaderDesc pixel_desc;
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		pixel_desc.Name = "ltjs_ssgi_apply_ps";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_ssgi_apply_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssgi_resources.apply_pixel_shader);
	}

	if (!g_ssgi_resources.ssgi_constants)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_ssgi_constants";
		desc.Size = sizeof(DiligentSsgiConstants);
		desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_ssgi_resources.ssgi_constants);
	}

	if (!g_ssgi_resources.temporal_constants)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_ssgi_temporal_constants";
		desc.Size = sizeof(DiligentSsgiTemporalConstants);
		desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_ssgi_resources.temporal_constants);
	}

	return g_ssgi_resources.vertex_shader && g_ssgi_resources.ssgi_pixel_shader &&
		g_ssgi_resources.temporal_pixel_shader && g_ssgi_resources.apply_pixel_shader &&
		g_ssgi_resources.ssgi_constants && g_ssgi_resources.temporal_constants;
}

DiligentSsgiPipeline* diligent_ssgi_get_pipeline(
	DiligentSsgiPipeline& pipeline,
	Diligent::IShader* pixel_shader,
	Diligent::IBuffer* constants,
	Diligent::TEXTURE_FORMAT rtv_format,
	bool additive_blend)
{
	if (pipeline.pipeline_state && pipeline.rtv_format == rtv_format)
	{
		return &pipeline;
	}

	if (!g_ssgi_resources.vertex_shader || !pixel_shader)
	{
		return nullptr;
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_ssgi_pipeline";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = rtv_format;
	pipeline_info.GraphicsPipeline.DSVFormat = Diligent::TEX_FORMAT_UNKNOWN;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pipeline_info.GraphicsPipeline.SmplDesc.Count = 1;

	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32, false, 0, static_cast<uint32>(sizeof(DiligentOptimized2DVertex))},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false, static_cast<uint32>(offsetof(DiligentOptimized2DVertex, color)), static_cast<uint32>(sizeof(DiligentOptimized2DVertex))},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false, static_cast<uint32>(offsetof(DiligentOptimized2DVertex, uv)), static_cast<uint32>(sizeof(DiligentOptimized2DVertex))}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	pipeline_info.GraphicsPipeline.RasterizerDesc.FillMode = Diligent::FILL_MODE_SOLID;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

	if (additive_blend)
	{
		auto& rt = pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0];
		rt.BlendEnable = true;
		rt.SrcBlend = Diligent::BLEND_FACTOR_ONE;
		rt.DestBlend = Diligent::BLEND_FACTOR_ONE;
		rt.BlendOp = Diligent::BLEND_OPERATION_ADD;
		rt.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
		rt.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
		rt.BlendOpAlpha = Diligent::BLEND_OPERATION_ADD;
	}

	Diligent::ShaderResourceVariableDesc vars[] =
	{
		{Diligent::SHADER_TYPE_PIXEL, "g_ColorTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_NormalTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_DepthTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_RoughnessTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_CurrGITex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_HistoryTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_MotionTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_GITex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	Diligent::ImmutableSamplerDesc sampler;
	sampler.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.AddressU = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler.Desc.AddressV = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler.Desc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler.SamplerOrTextureName = "g_Sampler";

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
	pipeline_info.PSODesc.ResourceLayout.Variables = vars;
	pipeline_info.PSODesc.ResourceLayout.NumVariables = static_cast<uint32>(std::size(vars));
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

	pipeline_info.pVS = g_ssgi_resources.vertex_shader;
	pipeline_info.pPS = pixel_shader;

	pipeline.pipeline_state.Release();
	pipeline.srb.Release();
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	if (constants)
	{
		auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SsgiConstants");
		if (ps_constants)
		{
			ps_constants->Set(constants);
		}
		auto* temporal_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SsgiTemporalConstants");
		if (temporal_constants)
		{
			temporal_constants->Set(constants);
		}
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	if (!pipeline.srb)
	{
		return nullptr;
	}

	pipeline.rtv_format = rtv_format;
	return &pipeline;
}

bool diligent_ssgi_update_constants(const DiligentSsgiConstants& constants)
{
	if (!g_diligent_state.immediate_context || !g_ssgi_resources.ssgi_constants)
	{
		return false;
	}

	Diligent::MapHelper<DiligentSsgiConstants> map(
		g_diligent_state.immediate_context,
		g_ssgi_resources.ssgi_constants,
		Diligent::MAP_WRITE,
		Diligent::MAP_FLAG_DISCARD);
	*map = constants;
	return true;
}

bool diligent_ssgi_update_temporal_constants(const DiligentSsgiTemporalConstants& constants)
{
	if (!g_diligent_state.immediate_context || !g_ssgi_resources.temporal_constants)
	{
		return false;
	}

	Diligent::MapHelper<DiligentSsgiTemporalConstants> map(
		g_diligent_state.immediate_context,
		g_ssgi_resources.temporal_constants,
		Diligent::MAP_WRITE,
		Diligent::MAP_FLAG_DISCARD);
	*map = constants;
	return true;
}

bool diligent_ssgi_draw_fullscreen(
	DiligentSsgiPipeline* pipeline,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target)
{
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb || !render_target)
	{
		return false;
	}

	const uint32 kVertexCount = 4;
	DiligentOptimized2DVertex vertices[kVertexCount];
	vertices[0] = {{-1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}};
	vertices[1] = {{1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}};
	vertices[2] = {{-1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}};
	vertices[3] = {{1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};

	Diligent::IBuffer* vertex_buffer = nullptr;
	if (!diligent_upload_optimized_2d_vertices(vertices, kVertexCount, vertex_buffer))
	{
		return false;
	}

	g_diligent_state.immediate_context->SetRenderTargets(
		1,
		&render_target,
		depth_target,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
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
	draw_attribs.NumVertices = kVertexCount;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->Draw(draw_attribs);
	return true;
}

} // namespace

bool diligent_ssgi_is_enabled()
{
	return g_CV_SSGIEnable.m_Val != 0;
}

bool diligent_apply_ssgi(Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target)
{
	if (!diligent_ssgi_is_enabled())
	{
		return true;
	}

	if (!render_target || !depth_target || !g_diligent_state.immediate_context)
	{
		return false;
	}

	auto* normal_srv = diligent_get_postfx_normal_srv();
	auto* motion_srv = diligent_get_postfx_motion_srv();
	auto* depth_srv = diligent_get_postfx_depth_srv();
	auto* roughness_srv = diligent_get_postfx_roughness_srv();
	if (!normal_srv || !motion_srv || !depth_srv || !roughness_srv)
	{
		return false;
	}

	const auto* render_texture = render_target->GetTexture();
	if (!render_texture)
	{
		return false;
	}

	const uint32 width = render_texture->GetDesc().Width;
	const uint32 height = render_texture->GetDesc().Height;
	const Diligent::TEXTURE_FORMAT format = render_texture->GetDesc().Format;
	if (!diligent_ssgi_ensure_targets(width, height, format))
	{
		return false;
	}

	if (!diligent_ssgi_ensure_resources())
	{
		return false;
	}

	if (!diligent_postfx_copy_scene_color(render_target))
	{
		return false;
	}
	auto* color_srv = diligent_get_postfx_scene_color_srv();
	if (!color_srv)
	{
		return false;
	}

	DiligentSsgiConstants constants{};
	constants.inv_target_size[0] = 1.0f / std::max(1u, width);
	constants.inv_target_size[1] = 1.0f / std::max(1u, height);
	constants.radius = std::max(0.1f, g_CV_SSGIRadius.m_Val);
	constants.intensity = std::max(0.0f, g_CV_SSGIIntensity.m_Val);
	constants.depth_reject = std::max(1e-4f, g_CV_SSGIDepthReject.m_Val);
	constants.normal_reject = std::clamp(g_CV_SSGINormalReject.m_Val, 0.0f, 1.0f);
	constants.sample_count = std::max(1, g_CV_SSGISampleCount.m_Val);
	if (!diligent_ssgi_update_constants(constants))
	{
		return false;
	}

	auto* ssgi_pipeline = diligent_ssgi_get_pipeline(
		g_ssgi_resources.ssgi_pipeline,
		g_ssgi_resources.ssgi_pixel_shader,
		g_ssgi_resources.ssgi_constants,
		g_ssgi_targets.format,
		false);
	if (!ssgi_pipeline)
	{
		return false;
	}

	auto* color_var = ssgi_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_ColorTex");
	if (color_var)
	{
		color_var->Set(color_srv);
	}
	auto* normal_var = ssgi_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_NormalTex");
	if (normal_var)
	{
		normal_var->Set(normal_srv);
	}
	auto* depth_var = ssgi_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_DepthTex");
	if (depth_var)
	{
		depth_var->Set(depth_srv);
	}
	auto* roughness_var = ssgi_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_RoughnessTex");
	if (roughness_var)
	{
		roughness_var->Set(roughness_srv);
	}

	if (!diligent_ssgi_draw_fullscreen(ssgi_pipeline, g_ssgi_targets.current_rtv, nullptr))
	{
		return false;
	}

	Diligent::ITextureView* gi_output_srv = g_ssgi_targets.current_srv;
	if (g_CV_SSGITemporalEnable.m_Val != 0)
	{
		if (!g_ssgi_targets.history_valid)
		{
			Diligent::CopyTextureAttribs copy_attribs;
			copy_attribs.pSrcTexture = g_ssgi_targets.current;
			copy_attribs.pDstTexture = g_ssgi_targets.history;
			copy_attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
			copy_attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
			g_diligent_state.immediate_context->CopyTexture(copy_attribs);
			g_ssgi_targets.history_valid = true;
			gi_output_srv = g_ssgi_targets.history_srv;
		}

		if (gi_output_srv != g_ssgi_targets.history_srv)
		{
		DiligentSsgiTemporalConstants temporal_constants{};
		temporal_constants.inv_target_size[0] = constants.inv_target_size[0];
		temporal_constants.inv_target_size[1] = constants.inv_target_size[1];
		temporal_constants.blend_factor = std::clamp(g_CV_SSGITemporalBlend.m_Val, 0.0f, 0.999f);
		temporal_constants.depth_reject = std::max(1e-4f, g_CV_SSGITemporalDepthReject.m_Val);
		temporal_constants.normal_reject = std::clamp(g_CV_SSGITemporalNormalReject.m_Val, 0.0f, 1.0f);
		if (!diligent_ssgi_update_temporal_constants(temporal_constants))
		{
			return false;
		}

		auto* temporal_pipeline = diligent_ssgi_get_pipeline(
			g_ssgi_resources.temporal_pipeline,
			g_ssgi_resources.temporal_pixel_shader,
			g_ssgi_resources.temporal_constants,
			g_ssgi_targets.format,
			false);
		if (!temporal_pipeline)
		{
			return false;
		}

		auto* curr_var = temporal_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_CurrGITex");
		if (curr_var)
		{
			curr_var->Set(g_ssgi_targets.current_srv);
		}
		auto* history_var = temporal_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_HistoryTex");
		if (history_var)
		{
			history_var->Set(g_ssgi_targets.history_srv);
		}
		auto* depth_var_temporal = temporal_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_DepthTex");
		if (depth_var_temporal)
		{
			depth_var_temporal->Set(depth_srv);
		}
		auto* normal_var_temporal = temporal_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_NormalTex");
		if (normal_var_temporal)
		{
			normal_var_temporal->Set(normal_srv);
		}
		auto* motion_var = temporal_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_MotionTex");
		if (motion_var)
		{
			motion_var->Set(motion_srv);
		}

		if (!diligent_ssgi_draw_fullscreen(temporal_pipeline, g_ssgi_targets.history_rtv, nullptr))
		{
			return false;
		}

		g_ssgi_targets.history_valid = true;
		gi_output_srv = g_ssgi_targets.history_srv;
		}
	}
	else
	{
		g_ssgi_targets.history_valid = false;
	}

	auto* apply_pipeline = diligent_ssgi_get_pipeline(
		g_ssgi_resources.apply_pipeline,
		g_ssgi_resources.apply_pixel_shader,
		nullptr,
		render_texture->GetDesc().Format,
		true);
	if (!apply_pipeline)
	{
		return false;
	}

	auto* gi_var = apply_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_GITex");
	if (gi_var)
	{
		gi_var->Set(gi_output_srv);
	}

	return diligent_ssgi_draw_fullscreen(apply_pipeline, render_target, depth_target);
}

bool diligent_apply_ssgi_resolved(const DiligentAaContext& ctx)
{
	if (!ctx.active)
	{
		return true;
	}

	return diligent_apply_ssgi(ctx.final_render_target, ctx.final_depth_target);
}

void diligent_ssgi_term()
{
	g_ssgi_targets = {};
	g_ssgi_resources = {};
}
