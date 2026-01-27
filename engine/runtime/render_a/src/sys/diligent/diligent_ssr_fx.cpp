#include "diligent_ssr_fx.h"

#include "diligent_2d_draw.h"
#include "diligent_device.h"
#include "diligent_internal.h"
#include "diligent_state.h"
#include "diligent_utils.h"

#include "bdefs.h"
#include "renderstruct.h"

#include "../rendererconsolevars.h"

#include "diligent_shaders_generated.h"

#include "PostProcess/Common/interface/PostFXContext.hpp"
#include "PostProcess/ScreenSpaceReflection/interface/ScreenSpaceReflection.hpp"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "MapHelper.hpp"

#include <algorithm>
#include <memory>

namespace Diligent
{
namespace HLSL
{
#include "Shaders/Common/public/ShaderDefinitions.fxh"
#include "Shaders/Common/public/BasicStructures.fxh"
#include "Shaders/PostProcess/ScreenSpaceReflection/public/ScreenSpaceReflectionStructures.fxh"
} // namespace HLSL
} // namespace Diligent

namespace
{
struct DiligentSsrCompositeConstants
{
	float intensity = 1.0f;
	float pad0[3] = {0.0f, 0.0f, 0.0f};
};

struct DiligentSsrPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	Diligent::TEXTURE_FORMAT rtv_format = Diligent::TEX_FORMAT_UNKNOWN;
};

struct DiligentSsrState
{
	std::unique_ptr<Diligent::ScreenSpaceReflection> ssr;
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> composite_pixel_shader;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> composite_constants;
	DiligentSsrPipeline composite_pipeline;
};

DiligentSsrState g_ssr_state;

bool diligent_ssr_ensure_resources()
{
	if (!g_diligent_state.render_device)
	{
		return false;
	}

	if (!g_ssr_state.ssr)
	{
		Diligent::ScreenSpaceReflection::CreateInfo create_info{};
		create_info.EnableAsyncCreation = false;
		g_ssr_state.ssr = std::make_unique<Diligent::ScreenSpaceReflection>(g_diligent_state.render_device, create_info);
	}

	if (!g_ssr_state.vertex_shader)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "VSMain";

		Diligent::ShaderDesc vertex_desc;
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		vertex_desc.Name = "ltjs_ssr_vs";
		shader_info.Desc = vertex_desc;
		shader_info.Source = diligent_get_optimized_2d_vertex_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssr_state.vertex_shader);
	}

	if (!g_ssr_state.composite_pixel_shader)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "PSMain";

		Diligent::ShaderDesc pixel_desc;
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		pixel_desc.Name = "ltjs_ssr_composite_ps";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_ssr_composite_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssr_state.composite_pixel_shader);
	}

	if (!g_ssr_state.composite_constants)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_ssr_composite_constants";
		desc.Size = sizeof(DiligentSsrCompositeConstants);
		desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_ssr_state.composite_constants);
	}

	return g_ssr_state.ssr && g_ssr_state.vertex_shader && g_ssr_state.composite_pixel_shader && g_ssr_state.composite_constants;
}

DiligentSsrPipeline* diligent_ssr_get_composite_pipeline(Diligent::TEXTURE_FORMAT rtv_format)
{
	auto& pipeline = g_ssr_state.composite_pipeline;
	if (pipeline.pipeline_state && pipeline.rtv_format == rtv_format)
	{
		return &pipeline;
	}

	if (!g_ssr_state.vertex_shader || !g_ssr_state.composite_pixel_shader)
	{
		return nullptr;
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_ssr_composite";
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

	Diligent::ShaderResourceVariableDesc vars[] =
	{
		{Diligent::SHADER_TYPE_PIXEL, "g_ColorTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_SSRColorTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
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

	pipeline_info.pVS = g_ssr_state.vertex_shader;
	pipeline_info.pPS = g_ssr_state.composite_pixel_shader;

	pipeline.pipeline_state.Release();
	pipeline.srb.Release();
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SsrCompositeConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_ssr_state.composite_constants);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	if (!pipeline.srb)
	{
		return nullptr;
	}

	pipeline.rtv_format = rtv_format;
	return &pipeline;
}

bool diligent_ssr_update_composite_constants(float intensity)
{
	if (!g_diligent_state.immediate_context || !g_ssr_state.composite_constants)
	{
		return false;
	}

	Diligent::MapHelper<DiligentSsrCompositeConstants> map(
		g_diligent_state.immediate_context,
		g_ssr_state.composite_constants,
		Diligent::MAP_WRITE,
		Diligent::MAP_FLAG_DISCARD);
	map->intensity = intensity;
	return true;
}

bool diligent_ssr_draw_fullscreen(DiligentSsrPipeline* pipeline, Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target)
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

bool diligent_ssr_is_enabled()
{
	return g_CV_SSREnable.m_Val != 0;
}

bool diligent_apply_ssr(Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target)
{
	if (!diligent_ssr_is_enabled())
	{
		return true;
	}

	if (!render_target || !depth_target || !g_diligent_state.immediate_context)
	{
		return false;
	}

	auto* postfx_context = diligent_get_postfx_context();
	auto* normal_srv = diligent_get_postfx_normal_srv();
	auto* motion_srv = diligent_get_postfx_motion_srv();
	auto* depth_srv = diligent_get_postfx_depth_srv();
	auto* roughness_srv = diligent_get_postfx_roughness_srv();
	if (!postfx_context || !normal_srv || !motion_srv || !depth_srv || !roughness_srv)
	{
		return false;
	}

	if (!diligent_ssr_ensure_resources())
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

	Diligent::ScreenSpaceReflection::FEATURE_FLAGS flags = Diligent::ScreenSpaceReflection::FEATURE_FLAG_NONE;
	if (g_CV_SSRHalfResolution.m_Val != 0)
	{
		flags = static_cast<Diligent::ScreenSpaceReflection::FEATURE_FLAGS>(
			flags | Diligent::ScreenSpaceReflection::FEATURE_FLAG_HALF_RESOLUTION);
	}

	g_ssr_state.ssr->PrepareResources(
		g_diligent_state.render_device,
		g_diligent_state.immediate_context,
		postfx_context,
		flags);

	Diligent::HLSL::ScreenSpaceReflectionAttribs ssr_attribs{};
	ssr_attribs.RoughnessThreshold = std::clamp(g_CV_SSRRoughnessThreshold.m_Val, 0.0f, 1.0f);
	ssr_attribs.MaxTraversalIntersections = static_cast<uint32>(std::max(1, g_CV_SSRMaxTraversal.m_Val));
	ssr_attribs.TemporalRadianceStabilityFactor = std::clamp(g_CV_SSRTemporalStability.m_Val, 0.0f, 0.999f);
	ssr_attribs.TemporalVarianceStabilityFactor = std::clamp(g_CV_SSRTemporalStability.m_Val, 0.0f, 0.999f);
	ssr_attribs.IsRoughnessPerceptual = TRUE;
	ssr_attribs.RoughnessChannel = 0;

	Diligent::ScreenSpaceReflection::RenderAttributes ssr_render_attribs{};
	ssr_render_attribs.pDevice = g_diligent_state.render_device;
	ssr_render_attribs.pDeviceContext = g_diligent_state.immediate_context;
	ssr_render_attribs.pPostFXContext = postfx_context;
	ssr_render_attribs.pColorBufferSRV = color_srv;
	ssr_render_attribs.pDepthBufferSRV = depth_srv;
	ssr_render_attribs.pNormalBufferSRV = normal_srv;
	ssr_render_attribs.pMaterialBufferSRV = roughness_srv;
	ssr_render_attribs.pMotionVectorsSRV = motion_srv;
	ssr_render_attribs.pSSRAttribs = &ssr_attribs;
	g_ssr_state.ssr->Execute(ssr_render_attribs);

	auto* ssr_srv = g_ssr_state.ssr->GetSSRRadianceSRV();
	if (!ssr_srv)
	{
		return false;
	}

	const float intensity = std::max(0.0f, g_CV_SSRIntensity.m_Val);
	if (!diligent_ssr_update_composite_constants(intensity))
	{
		return false;
	}

	auto* composite_pipeline = diligent_ssr_get_composite_pipeline(render_target->GetTexture()->GetDesc().Format);
	if (!composite_pipeline || !composite_pipeline->pipeline_state || !composite_pipeline->srb)
	{
		return false;
	}

	auto* color_var = composite_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_ColorTex");
	if (color_var)
	{
		color_var->Set(color_srv);
	}
	auto* ssr_var = composite_pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_SSRColorTex");
	if (ssr_var)
	{
		ssr_var->Set(ssr_srv);
	}

	return diligent_ssr_draw_fullscreen(composite_pipeline, render_target, depth_target);
}

bool diligent_apply_ssr_resolved(const DiligentAaContext& ctx)
{
	if (!ctx.active)
	{
		return true;
	}

	return diligent_apply_ssr(ctx.final_render_target, ctx.final_depth_target);
}

void diligent_ssr_term()
{
	g_ssr_state = {};
}
