#include "diligent_postfx.h"

#include "diligent_2d_draw.h"
#include "diligent_device.h"
#include "diligent_glow_blur.h"
#include "diligent_internal.h"
#include "diligent_model_draw.h"
#include "diligent_object_draw.h"
#include "diligent_scene_collect.h"
#include "diligent_utils.h"
#include "diligent_world_data.h"
#include "diligent_world_draw.h"

#include "bdefs.h"
#include "de_objects.h"
#include "de_world.h"
#include "ltmatrix.h"
#include "ltvector.h"
#include "model.h"
#include "renderstruct.h"
#include "rendertarget.h"
#include "viewparams.h"
#include "world_client_bsp.h"
#include "world_tree.h"

#include "../renderstylemap.h"
#include "../rendererconsolevars.h"

#include "diligent_shaders_generated.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

struct DiligentGlowBlurPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

namespace
{
constexpr uint32 kDiligentGlowMinTextureSize = 16;
constexpr uint32 kDiligentGlowMaxTextureSize = 512;
constexpr uint32 kDiligentGlowMaxFilterSize = 64;
constexpr float kDiligentGlowMinElementWeight = 0.01f;
static_assert(kDiligentGlowBlurBatchSize == 8, "Update glow blur shader tap count.");

struct DiligentGlowElement
{
	float tex_offset = 0.0f;
	float weight = 0.0f;
};

struct DiligentGlowState
{
	CRenderStyleMap render_style_map;
	std::unique_ptr<CRenderTarget> glow_target;
	std::unique_ptr<CRenderTarget> blur_target;
	uint32 texture_width = 0;
	uint32 texture_height = 0;
	std::array<DiligentGlowElement, kDiligentGlowMaxFilterSize> filter{};
	uint32 filter_size = 0;
};

DiligentGlowState g_diligent_glow_state;

struct DiligentGlowBlurResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	DiligentGlowBlurPipeline pipeline_solid;
	DiligentGlowBlurPipeline pipeline_add;
};

DiligentGlowBlurResources g_diligent_glow_blur_resources;
Diligent::RefCntAutoPtr<Diligent::ITexture> g_debug_white_texture;
Diligent::RefCntAutoPtr<Diligent::ITextureView> g_debug_white_texture_srv;

struct DiligentSceneDescScope
{
	SceneDesc*& target;
	SceneDesc* previous;

	DiligentSceneDescScope(SceneDesc*& target, SceneDesc* value)
		: target(target), previous(target)
	{
		target = value;
	}

	~DiligentSceneDescScope()
	{
		target = previous;
	}
};

Diligent::ITextureView* diligent_get_debug_white_texture_view()
{
	if (g_debug_white_texture_srv)
	{
		return g_debug_white_texture_srv;
	}

	if (!g_diligent_state.render_device)
	{
		return nullptr;
	}

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_debug_white";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
	desc.Usage = Diligent::USAGE_IMMUTABLE;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	const uint32 white_pixel = 0xFFFFFFFFu;
	Diligent::TextureSubResData subresource;
	subresource.pData = &white_pixel;
	subresource.Stride = sizeof(white_pixel);
	subresource.DepthStride = 0;

	Diligent::TextureData init_data{&subresource, 1};
	g_diligent_state.render_device->CreateTexture(desc, &init_data, &g_debug_white_texture);
	if (!g_debug_white_texture)
	{
		return nullptr;
	}

	g_debug_white_texture_srv = g_debug_white_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	return g_debug_white_texture_srv;
}

uint32 diligent_glow_truncate_to_power_of_two(uint32 value)
{
	uint32 nearest = 0x80000000u;
	while (nearest > value)
	{
		nearest >>= 1;
	}

	return nearest;
}

bool diligent_ensure_glow_blur_resources()
{
	if (g_diligent_glow_blur_resources.vertex_shader && g_diligent_glow_blur_resources.pixel_shader &&
		g_diligent_glow_blur_resources.constant_buffer)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	if (!g_diligent_glow_blur_resources.vertex_shader)
	{
		Diligent::ShaderDesc vertex_desc;
		vertex_desc.Name = "ltjs_glow_blur_vs";
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		shader_info.Desc = vertex_desc;
		shader_info.EntryPoint = "VSMain";
		shader_info.Source = diligent_get_glow_blur_vertex_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_diligent_glow_blur_resources.vertex_shader);
	}

	if (!g_diligent_glow_blur_resources.pixel_shader)
	{
		Diligent::ShaderDesc pixel_desc;
		pixel_desc.Name = "ltjs_glow_blur_ps";
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		shader_info.Desc = pixel_desc;
		shader_info.EntryPoint = "PSMain";
		shader_info.Source = diligent_get_glow_blur_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_diligent_glow_blur_resources.pixel_shader);
	}

	if (!g_diligent_glow_blur_resources.constant_buffer)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_glow_blur_constants";
		desc.Size = sizeof(DiligentGlowBlurConstants);
		desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_diligent_glow_blur_resources.constant_buffer);
	}

	return g_diligent_glow_blur_resources.vertex_shader && g_diligent_glow_blur_resources.pixel_shader &&
		g_diligent_glow_blur_resources.constant_buffer;
}

} // namespace

float diligent_get_tonemap_enabled()
{
	return g_CV_ToneMapEnable.m_Val != 0 ? 1.0f : 0.0f;
}

DiligentGlowBlurPipeline* diligent_get_glow_blur_pipeline(LTSurfaceBlend blend)
{
	if (!diligent_ensure_glow_blur_resources())
	{
		return nullptr;
	}

	if (!g_diligent_state.render_device || !g_diligent_state.swap_chain)
	{
		return nullptr;
	}

	DiligentGlowBlurPipeline* pipeline = (blend == LTSURFACEBLEND_SOLID)
		? &g_diligent_glow_blur_resources.pipeline_solid
		: &g_diligent_glow_blur_resources.pipeline_add;

	if (pipeline->pipeline_state && pipeline->srb)
	{
		return pipeline;
	}

	Diligent::TEXTURE_FORMAT color_format = g_diligent_state.swap_chain->GetDesc().ColorBufferFormat;
	Diligent::TEXTURE_FORMAT depth_format = g_diligent_state.swap_chain->GetDesc().DepthBufferFormat;
	if (g_diligent_glow_state.glow_target)
	{
		auto* rtv = g_diligent_glow_state.glow_target->GetRenderTargetView();
		if (rtv && rtv->GetTexture())
		{
			color_format = rtv->GetTexture()->GetDesc().Format;
		}
		auto* dsv = g_diligent_glow_state.glow_target->GetDepthStencilView();
		if (dsv && dsv->GetTexture())
		{
			depth_format = dsv->GetTexture()->GetDesc().Format;
		}
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_glow_blur";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = color_format;
	pipeline_info.GraphicsPipeline.DSVFormat = depth_format;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;

	diligent_fill_optimized_2d_blend_desc(blend, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);

	pipeline_info.pVS = g_diligent_glow_blur_resources.vertex_shader;
	pipeline_info.pPS = g_diligent_glow_blur_resources.pixel_shader;

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
	sampler_desc.Desc.AddressU = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler_desc.Desc.AddressV = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler_desc.Desc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler_desc.SamplerOrTextureName = "g_Texture";
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler_desc;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline->pipeline_state);
	if (!pipeline->pipeline_state)
	{
		return nullptr;
	}

	auto* ps_constants = pipeline->pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "GlowBlurConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_diligent_glow_blur_resources.constant_buffer);
	}

	pipeline->pipeline_state->CreateShaderResourceBinding(&pipeline->srb, true);
	return pipeline->pipeline_state && pipeline->srb ? pipeline : nullptr;
}

bool diligent_update_glow_blur_constants(const DiligentGlowBlurConstants& constants)
{
	if (!g_diligent_state.immediate_context || !g_diligent_glow_blur_resources.constant_buffer)
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_diligent_glow_blur_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_diligent_glow_blur_resources.constant_buffer, Diligent::MAP_WRITE);
	return true;
}

bool diligent_draw_glow_blur_quad(
	DiligentGlowBlurPipeline* pipeline,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target)
{
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb || !texture_view || !render_target)
	{
		return false;
	}

	constexpr uint32 kVertexCount = 4;
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
	draw_attribs.NumVertices = kVertexCount;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->Draw(draw_attribs);
	return true;
}

void diligent_process_model_attachments_glow(LTObject* object, uint32 depth, const CRenderStyleMap& render_style_map)
{
	if (!object || depth >= 32 || !g_diligent_state.render_struct || !g_diligent_state.render_struct->ProcessAttachment)
	{
		return;
	}

	Attachment* current = object->m_Attachments;
	while (current)
	{
		LTObject* attached = g_diligent_state.render_struct->ProcessAttachment(object, current);
		if (attached && attached->m_WTFrameCode != g_diligent_state.object_frame_code)
		{
			if (attached->m_Attachments)
			{
				diligent_process_model_attachments_glow(attached, depth + 1, render_style_map);
			}

			if (diligent_should_process_model(attached))
			{
				attached->m_WTFrameCode = g_diligent_state.object_frame_code;
				diligent_draw_model_instance_with_render_style_map(attached->ToModel(), &render_style_map);
			}
		}

		current = current->m_pNext;
	}
}

void diligent_process_model_object_glow(LTObject* object, const CRenderStyleMap& render_style_map)
{
	if (!diligent_should_process_model(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;
	diligent_draw_model_instance_with_render_style_map(object->ToModel(), &render_style_map);
	if (object->m_Attachments)
	{
		diligent_process_model_attachments_glow(object, 0, render_style_map);
	}
}

void diligent_filter_world_node_for_models_glow(const ViewParams& params, WorldTreeNode* node, const CRenderStyleMap& render_style_map)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_model_object_glow(object, render_style_map);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_models_glow(params, child, render_style_map);
	}
}

bool diligent_draw_models_glow(SceneDesc* desc, const CRenderStyleMap& render_style_map)
{
	if (!desc)
	{
		return true;
	}
	if (!g_CV_DrawModels.m_Val)
	{
		return true;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return true;
		}

		if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
		{
			g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
		}
		else
		{
			++g_diligent_state.object_frame_code;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_model_object_glow(object, render_style_map);
		}

		return true;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	auto* world_bsp_client = diligent_get_world_bsp_client();
	if (!world_bsp_client)
	{
		return true;
	}

	WorldTree* tree = world_bsp_client->ClientTree();
	if (!tree)
	{
		return true;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	diligent_filter_world_node_for_models_glow(g_diligent_state.view_params, tree->GetRootNode(), render_style_map);

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_model_object_glow(object, render_style_map);
	}

	return true;
}

bool diligent_glow_update_filter()
{
	const uint32 requested_size = LTCLAMP(g_CV_ScreenGlowFilterSize.m_Val, 0, static_cast<int>(kDiligentGlowMaxFilterSize));
	g_diligent_glow_state.filter_size = 0;
	if (requested_size == 0)
	{
		return true;
	}

	const float pixel_shift = g_CV_ScreenGlowPixelShift.m_Val;
	const float gauss_radius[2] = {g_CV_ScreenGlowGaussRadius0, g_CV_ScreenGlowGaussRadius1};
	const float gauss_amplitude[2] = {g_CV_ScreenGlowGaussAmp0, g_CV_ScreenGlowGaussAmp1};

	const float filter_center = requested_size > 1 ? (static_cast<float>(requested_size) - 1.0f) / 2.0f : 0.0f;
	const float inv_center_sqr = filter_center > 0.0f ? 1.0f / (filter_center * filter_center) : 0.0f;

	for (uint32 element_index = 0; element_index < requested_size && g_diligent_glow_state.filter_size < kDiligentGlowMaxFilterSize; ++element_index)
	{
		DiligentGlowElement element;
		element.tex_offset = static_cast<float>(element_index) - filter_center + pixel_shift;
		const float radius = element.tex_offset * element.tex_offset * inv_center_sqr;
		element.weight = 0.0f;
		for (uint32 gaussian_index = 0; gaussian_index < 2; ++gaussian_index)
		{
			element.weight += gauss_amplitude[gaussian_index] / std::exp(radius * gauss_radius[gaussian_index]);
		}

		if (element.weight > kDiligentGlowMinElementWeight)
		{
			g_diligent_glow_state.filter[g_diligent_glow_state.filter_size++] = element;
		}
	}

	while (g_diligent_glow_state.filter_size % 4 != 0 && g_diligent_glow_state.filter_size < kDiligentGlowMaxFilterSize)
	{
		g_diligent_glow_state.filter[g_diligent_glow_state.filter_size++] = {};
	}

	float weight_sum = 0.0f;
	for (uint32 index = 0; index < g_diligent_glow_state.filter_size; ++index)
	{
		weight_sum += g_diligent_glow_state.filter[index].weight;
	}
	if (weight_sum > 0.0f)
	{
		const float inv_weight_sum = 1.0f / weight_sum;
		for (uint32 index = 0; index < g_diligent_glow_state.filter_size; ++index)
		{
			g_diligent_glow_state.filter[index].weight *= inv_weight_sum;
		}
	}

	return true;
}

bool diligent_glow_ensure_targets(uint32 width, uint32 height)
{
	width = diligent_glow_truncate_to_power_of_two(width);
	height = diligent_glow_truncate_to_power_of_two(height);
	if (width == 0 || height == 0)
	{
		return false;
	}

	width = LTCLAMP(width, kDiligentGlowMinTextureSize, kDiligentGlowMaxTextureSize);
	height = LTCLAMP(height, kDiligentGlowMinTextureSize, kDiligentGlowMaxTextureSize);

	if (g_diligent_glow_state.glow_target && g_diligent_glow_state.blur_target &&
		g_diligent_glow_state.texture_width == width && g_diligent_glow_state.texture_height == height)
	{
		return true;
	}

	g_diligent_glow_state.glow_target = std::make_unique<CRenderTarget>();
	g_diligent_glow_state.blur_target = std::make_unique<CRenderTarget>();
	if (!g_diligent_glow_state.glow_target || !g_diligent_glow_state.blur_target)
	{
		return false;
	}

	if (g_diligent_glow_state.glow_target->Init(static_cast<int>(width), static_cast<int>(height), RTFMT_X8R8G8B8, STFMT_D24S8) != LT_OK)
	{
		g_diligent_glow_state.glow_target.reset();
		g_diligent_glow_state.blur_target.reset();
		return false;
	}

	if (g_diligent_glow_state.blur_target->Init(static_cast<int>(width), static_cast<int>(height), RTFMT_X8R8G8B8, STFMT_D24S8) != LT_OK)
	{
		g_diligent_glow_state.glow_target.reset();
		g_diligent_glow_state.blur_target.reset();
		return false;
	}

	g_diligent_glow_state.texture_width = width;
	g_diligent_glow_state.texture_height = height;
	return true;
}

void diligent_glow_free_targets()
{
	g_diligent_glow_state.glow_target.reset();
	g_diligent_glow_state.blur_target.reset();
	g_diligent_glow_state.texture_width = 0;
	g_diligent_glow_state.texture_height = 0;
	g_diligent_glow_state.filter_size = 0;
}

bool diligent_glow_draw_fullscreen_quad(
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target,
	float u0,
	float v0,
	float u1,
	float v1,
	float weight,
	LTSurfaceBlend blend)
{
	const float color = weight;
	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{-1.0f, 1.0f}, {color, color, color, color}, {u0, v0}};
	vertices[1] = {{1.0f, 1.0f}, {color, color, color, color}, {u1, v0}};
	vertices[2] = {{-1.0f, -1.0f}, {color, color, color, color}, {u0, v1}};
	vertices[3] = {{1.0f, -1.0f}, {color, color, color, color}, {u1, v1}};
	return diligent_draw_optimized_2d_vertices_to_target(vertices, 4, blend, texture_view, render_target, depth_target, true);
}

bool diligent_glow_draw_debug_quad(
	float left,
	float top,
	float right,
	float bottom,
	float u0,
	float v0,
	float u1,
	float v1,
	float red,
	float green,
	float blue,
	float alpha,
	Diligent::ITextureView* texture_view)
{
	if (!texture_view)
	{
		return false;
	}

	float left_top_x = 0.0f;
	float left_top_y = 0.0f;
	float right_top_x = 0.0f;
	float right_top_y = 0.0f;
	float left_bottom_x = 0.0f;
	float left_bottom_y = 0.0f;
	float right_bottom_x = 0.0f;
	float right_bottom_y = 0.0f;
	if (!diligent_to_view_clip_space(left, top, left_top_x, left_top_y) ||
		!diligent_to_view_clip_space(right, top, right_top_x, right_top_y) ||
		!diligent_to_view_clip_space(left, bottom, left_bottom_x, left_bottom_y) ||
		!diligent_to_view_clip_space(right, bottom, right_bottom_x, right_bottom_y))
	{
		return false;
	}

	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{left_top_x, left_top_y}, {red, green, blue, alpha}, {u0, v0}};
	vertices[1] = {{right_top_x, right_top_y}, {red, green, blue, alpha}, {u1, v0}};
	vertices[2] = {{left_bottom_x, left_bottom_y}, {red, green, blue, alpha}, {u0, v1}};
	vertices[3] = {{right_bottom_x, right_bottom_y}, {red, green, blue, alpha}, {u1, v1}};
	return diligent_draw_optimized_2d_vertices(vertices, 4, LTSURFACEBLEND_SOLID, texture_view);
}

bool diligent_glow_draw_debug_texture(Diligent::ITextureView* texture_view)
{
	if (!texture_view)
	{
		return false;
	}

	const float size_scale = LTCLAMP(g_CV_ScreenGlowShowTextureScale.m_Val, 0.01f, 1.0f);
	const float width = g_diligent_state.view_params.m_fScreenWidth * size_scale;
	const float height = g_diligent_state.view_params.m_fScreenHeight * size_scale;
	if (width <= 0.0f || height <= 0.0f)
	{
		return false;
	}

	return diligent_glow_draw_debug_quad(0.0f, 0.0f, width, height, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, texture_view);
}

bool diligent_glow_draw_debug_filter()
{
	if (g_diligent_glow_state.filter_size == 0)
	{
		return false;
	}

	auto* texture_view = diligent_get_debug_white_texture_view();
	if (!texture_view)
	{
		return false;
	}

	const float size_scale = LTCLAMP(g_CV_ScreenGlowShowFilterScale.m_Val, 0.01f, 1.0f);
	const float screen_width = g_diligent_state.view_params.m_fScreenWidth;
	const float screen_height = g_diligent_state.view_params.m_fScreenHeight;
	float left = 0.0f;
	float top = 0.0f;
	float right = screen_width * size_scale;
	float bottom = screen_height * size_scale;

	if (g_CV_ScreenGlowShowTexture.m_Val)
	{
		const float texture_scale = LTCLAMP(g_CV_ScreenGlowShowTextureScale.m_Val, 0.01f, 1.0f);
		const float texture_width = texture_scale * screen_width;
		left = LTMIN(left + texture_width, screen_width);
		right = LTMIN(right + texture_width, screen_width);
		if (left >= right)
		{
			return false;
		}
	}

	const float gutter = 1.0f;
	const uint32 filter_size = g_diligent_glow_state.filter_size;
	const uint32 num_gutters = filter_size > 0 ? filter_size - 1 : 0;
	if ((right - left) < (static_cast<float>(filter_size) + static_cast<float>(num_gutters) * gutter))
	{
		return false;
	}

	const float element_width = ((right - left) - static_cast<float>(num_gutters) * gutter) / static_cast<float>(filter_size);
	constexpr float background_color = 0x22 / 255.0f;
	constexpr float element_green = 0xCC / 255.0f;
	const float max_element = g_CV_ScreenGlowShowFilterRange.m_Val;

	if (!diligent_glow_draw_debug_quad(left, top, right, bottom, 0.0f, 0.0f, 1.0f, 1.0f, background_color, background_color, background_color, background_color, texture_view))
	{
		return false;
	}

	float current_x = left;
	for (uint32 index = 0; index < filter_size; ++index)
	{
		const float weight = g_diligent_glow_state.filter[index].weight;
		float height = LTCLAMP(weight / max_element, 0.0f, 1.0f);
		float element_top = top * height + bottom * (1.0f - height);
		diligent_glow_draw_debug_quad(
			current_x,
			element_top,
			current_x + element_width,
			bottom,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			0.0f,
			element_green,
			0.0f,
			1.0f,
			texture_view);
		current_x += element_width + gutter;
	}

	return true;
}

bool diligent_glow_render_blur_pass(
	CRenderTarget& dest_target,
	Diligent::ITextureView* source_view,
	float u_weight,
	float v_weight)
{
	if (!source_view)
	{
		return false;
	}

	const float uv_scale = g_CV_ScreenGlowUVScale.m_Val;
	const float width = static_cast<float>(g_diligent_glow_state.texture_width);
	const float height = static_cast<float>(g_diligent_glow_state.texture_height);

	if (!g_CV_ScreenGlowBlurMultiTap.m_Val)
	{
		bool first_pass = true;
		for (uint32 index = 0; index < g_diligent_glow_state.filter_size; ++index)
		{
			const auto& element = g_diligent_glow_state.filter[index];
			if (element.weight <= 0.0f)
			{
				continue;
			}

			const float u_offset = uv_scale * element.tex_offset * u_weight / width;
			const float v_offset = uv_scale * element.tex_offset * v_weight / height;
			const float u0 = 0.0f + u_offset;
			const float u1 = 1.0f + u_offset;
			const float v0 = 0.0f + v_offset;
			const float v1 = 1.0f + v_offset;
			if (!diligent_glow_draw_fullscreen_quad(
				source_view,
				dest_target.GetRenderTargetView(),
				dest_target.GetDepthStencilView(),
				u0,
				v0,
				u1,
				v1,
				element.weight,
				first_pass ? LTSURFACEBLEND_SOLID : LTSURFACEBLEND_ADD))
			{
				return false;
			}

			first_pass = false;
		}

		return true;
	}

	auto* pipeline = diligent_get_glow_blur_pipeline(LTSURFACEBLEND_SOLID);
	if (!pipeline)
	{
		return false;
	}

	bool first_pass = true;
	uint32 index = 0;
	while (index < g_diligent_glow_state.filter_size)
	{
		DiligentGlowBlurConstants constants{};
		uint32 tap_count = 0;
		while (index < g_diligent_glow_state.filter_size && tap_count < kDiligentGlowBlurBatchSize)
		{
			const auto& element = g_diligent_glow_state.filter[index++];
			if (element.weight <= 0.0f)
			{
				continue;
			}

			const float u_offset = uv_scale * element.tex_offset * u_weight / width;
			const float v_offset = uv_scale * element.tex_offset * v_weight / height;
			const float weight = element.weight;
			constants.taps[tap_count] = {u_offset, v_offset, weight, 0.0f};
			++tap_count;
		}

		if (tap_count == 0)
		{
			continue;
		}

		constants.params[0] = static_cast<float>(tap_count);
		if (!diligent_update_glow_blur_constants(constants))
		{
			return false;
		}

		const LTSurfaceBlend blend = first_pass ? LTSURFACEBLEND_SOLID : LTSURFACEBLEND_ADD;
		pipeline = diligent_get_glow_blur_pipeline(blend);
		if (!pipeline)
		{
			return false;
		}

		if (!diligent_draw_glow_blur_quad(
			pipeline,
			source_view,
			dest_target.GetRenderTargetView(),
			dest_target.GetDepthStencilView()))
		{
			return false;
		}

		first_pass = false;
	}

	return true;
}

bool diligent_render_screen_glow(SceneDesc* desc)
{
	if (!desc || !g_CV_ScreenGlowEnable.m_Val)
	{
		return true;
	}

	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	DiligentSceneDescScope scene_desc_scope(g_diligent_state.scene_desc, desc);

	uint32 texture_size = static_cast<uint32>(g_CV_ScreenGlowTextureSize.m_Val);
	if (!diligent_glow_ensure_targets(texture_size, texture_size))
	{
		return false;
	}

	if (!diligent_glow_update_filter())
	{
		return false;
	}

	auto* glow_target = g_diligent_glow_state.glow_target.get();
	auto* blur_target = g_diligent_glow_state.blur_target.get();
	if (!glow_target || !blur_target)
	{
		return false;
	}

	auto* glow_srv = glow_target->GetShaderResourceView();
	auto* blur_srv = blur_target->GetShaderResourceView();
	if (!glow_srv || !blur_srv)
	{
		return false;
	}

	ViewParams saved_view = g_diligent_state.view_params;
	std::vector<DiligentRenderBlock*> saved_blocks = g_visible_render_blocks;

	constexpr float kGlowNearZ = 7.0f;
	float glow_far_z = g_CV_ScreenGlowFogEnable.m_Val ? g_CV_ScreenGlowFogFarZ.m_Val : g_CV_FarZ.m_Val;
	if (glow_far_z > g_CV_FarZ.m_Val)
	{
		glow_far_z = g_CV_FarZ.m_Val;
	}

	ViewParams glow_params;
	if (!lt_InitViewFrustum(
			&glow_params,
			desc->m_xFov,
			desc->m_yFov,
			kGlowNearZ,
			glow_far_z,
			0.0f,
			0.0f,
			static_cast<float>(g_diligent_glow_state.texture_width),
			static_cast<float>(g_diligent_glow_state.texture_height),
			&desc->m_Pos,
			&desc->m_Rotation,
			ViewParams::eRenderMode_Glow))
	{
		g_diligent_state.view_params = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}

	g_diligent_state.view_params = glow_params;
	g_visible_render_blocks.clear();
	if (g_render_world)
	{
		g_visible_render_blocks.reserve(g_render_world->render_blocks.size());
		for (const auto& block : g_render_world->render_blocks)
		{
			if (!block)
			{
				continue;
			}

			if (g_diligent_state.view_params.ViewAABBIntersect(block->bounds_min, block->bounds_max))
			{
				g_visible_render_blocks.push_back(block.get());
			}
		}
	}

	DiligentWorldConstants glow_constants;
	LTMatrix glow_world;
	glow_world.Identity();
	LTMatrix glow_mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * glow_world;
	diligent_store_matrix_from_lt(glow_mvp, glow_constants.mvp);
	diligent_store_matrix_from_lt(g_diligent_state.view_params.m_mView, glow_constants.view);
	diligent_store_matrix_from_lt(glow_world, glow_constants.world);
	glow_constants.camera_pos[0] = g_diligent_state.view_params.m_Pos.x;
	glow_constants.camera_pos[1] = g_diligent_state.view_params.m_Pos.y;
	glow_constants.camera_pos[2] = g_diligent_state.view_params.m_Pos.z;
	glow_constants.camera_pos[3] = g_CV_ScreenGlowFogEnable.m_Val ? 1.0f : 0.0f;
	glow_constants.fog_color[0] = 0.0f;
	glow_constants.fog_color[1] = 0.0f;
	glow_constants.fog_color[2] = 0.0f;
	glow_constants.fog_color[3] = g_CV_ScreenGlowFogNearZ.m_Val;
	glow_constants.fog_params[0] = g_CV_ScreenGlowFogFarZ.m_Val;
	glow_constants.fog_params[1] = diligent_get_tonemap_enabled();
	glow_constants.fog_params[2] = 0.0f;
	glow_constants.fog_params[3] = 0.0f;

	glow_target->InstallOnDevice();
	diligent_set_viewport(static_cast<float>(g_diligent_glow_state.texture_width), static_cast<float>(g_diligent_glow_state.texture_height));
	if (!diligent_draw_world_glow(glow_constants, g_visible_render_blocks))
	{
		g_diligent_state.view_params = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}
	if (!diligent_draw_world_models_glow(g_diligent_solid_world_models, glow_constants))
	{
		g_diligent_state.view_params = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}

	g_diligent_state.glow_mode = true;
	const bool models_ok = diligent_draw_models_glow(desc, g_diligent_glow_state.render_style_map);
	g_diligent_state.glow_mode = false;
	if (!models_ok)
	{
		g_diligent_state.view_params = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}
	if (!diligent_draw_solid_canvases_glow(desc))
	{
		g_diligent_state.view_params = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}

	if (g_diligent_glow_state.filter_size > 0)
	{
		blur_target->InstallOnDevice();
		diligent_set_viewport(static_cast<float>(g_diligent_glow_state.texture_width), static_cast<float>(g_diligent_glow_state.texture_height));
		if (!diligent_glow_render_blur_pass(*blur_target, glow_srv, 1.0f, 0.0f))
		{
			g_diligent_state.view_params = saved_view;
			g_visible_render_blocks = saved_blocks;
			return false;
		}

		glow_target->InstallOnDevice();
		diligent_set_viewport(static_cast<float>(g_diligent_glow_state.texture_width), static_cast<float>(g_diligent_glow_state.texture_height));
		if (!diligent_glow_render_blur_pass(*glow_target, blur_srv, 0.0f, 1.0f))
		{
			g_diligent_state.view_params = saved_view;
			g_visible_render_blocks = saved_blocks;
			return false;
		}
	}

	const float left = static_cast<float>(desc->m_Rect.left);
	const float top = static_cast<float>(desc->m_Rect.top);
	const float width = static_cast<float>(desc->m_Rect.right - desc->m_Rect.left);
	const float height = static_cast<float>(desc->m_Rect.bottom - desc->m_Rect.top);

	auto* backbuffer_rtv = diligent_get_active_render_target();
	auto* backbuffer_dsv = diligent_get_active_depth_target();
	if (backbuffer_rtv)
	{
		const float u0 = 1.0f / static_cast<float>(g_diligent_glow_state.texture_width);
		const float v0 = 1.0f / static_cast<float>(g_diligent_glow_state.texture_height);
		const float u1 = 1.0f;
		const float v1 = 1.0f;
		diligent_set_viewport_rect(left, top, width, height);
		diligent_glow_draw_fullscreen_quad(glow_srv, backbuffer_rtv, backbuffer_dsv, u0, v0, u1, v1, 1.0f, LTSURFACEBLEND_ADD);
	}

	g_diligent_state.view_params = saved_view;
	g_visible_render_blocks = saved_blocks;

	if (g_CV_ScreenGlowShowTexture.m_Val || g_CV_ScreenGlowShowFilter.m_Val)
	{
		diligent_set_viewport_rect(left, top, width, height);
		if (g_CV_ScreenGlowShowTexture.m_Val)
		{
			diligent_glow_draw_debug_texture(glow_srv);
		}
		if (g_CV_ScreenGlowShowFilter.m_Val)
		{
			diligent_glow_draw_debug_filter();
		}
	}

	return true;
}

bool diligent_AddGlowRenderStyleMapping(const char* source, const char* map_to)
{
	if (!source || !map_to)
	{
		return false;
	}

	return g_diligent_glow_state.render_style_map.AddRenderStyle(source, map_to);
}

bool diligent_SetGlowDefaultRenderStyle(const char* filename)
{
	if (!filename)
	{
		return false;
	}

	return g_diligent_glow_state.render_style_map.SetDefaultRenderStyle(filename);
}

bool diligent_SetNoGlowRenderStyle(const char* filename)
{
	if (!filename)
	{
		return false;
	}

	return g_diligent_glow_state.render_style_map.SetNoGlowRenderStyle(filename);
}

void diligent_postfx_term()
{
	diligent_glow_free_targets();
	g_diligent_glow_state.render_style_map.FreeList();
	g_debug_white_texture_srv.Release();
	g_debug_white_texture.Release();
	g_diligent_glow_blur_resources.vertex_shader.Release();
	g_diligent_glow_blur_resources.pixel_shader.Release();
	g_diligent_glow_blur_resources.constant_buffer.Release();
	g_diligent_glow_blur_resources.pipeline_solid.srb.Release();
	g_diligent_glow_blur_resources.pipeline_solid.pipeline_state.Release();
	g_diligent_glow_blur_resources.pipeline_add.srb.Release();
	g_diligent_glow_blur_resources.pipeline_add.pipeline_state.Release();
}
