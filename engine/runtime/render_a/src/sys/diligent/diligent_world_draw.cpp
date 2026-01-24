#include "diligent_world_draw.h"

#include "diligent_buffers.h"
#include "diligent_device.h"
#include "diligent_state.h"
#include "diligent_postfx.h"
#include "diligent_render.h"
#include "diligent_render_debug.h"
#include "diligent_scene_collect.h"
#include "diligent_texture_cache.h"
#include "diligent_utils.h"
#include "diligent_internal.h"
#include "world_client_bsp.h"

#include "bdefs.h"
#include "de_objects.h"
#include "de_world.h"
#include "renderstruct.h"
#include "texturescriptinstance.h"
#include "texturescriptmgr.h"
#include "texturescriptvarmgr.h"
#include "viewparams.h"
#include "dtxmgr.h"

#include <algorithm>

#include "diligent_shaders_generated.h"
#include "ltrenderstyle.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#include "../rendererconsolevars.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace
{
constexpr uint8 kWorldPipelineSkip = 0xFFu;

struct DiligentTextureEffectStage
{
	ETextureScriptChannel channel = TSChannel_Null;
	bool use_uv1 = false;
};

const WorldBsp* diligent_pick_external_bsp(const DiligentRenderBlock& block, uint32 poly_index, uint32& out_world_index)
{
	out_world_index = 0;
	if (!g_diligent_state.external_world_bsp_models || g_diligent_state.external_world_bsp_model_count == 0)
	{
		return nullptr;
	}

	const char* desired_name = nullptr;
	if (block.world && !block.world->world_name.empty())
	{
		desired_name = block.world->world_name.c_str();
	}

	if (desired_name && desired_name[0] != '\0')
	{
		for (uint32 i = 0; i < g_diligent_state.external_world_bsp_model_count; ++i)
		{
			const WorldBsp* candidate = g_diligent_state.external_world_bsp_models[i];
			if (!candidate)
			{
				continue;
			}
			if (stricmp(candidate->m_WorldName, desired_name) == 0)
			{
				out_world_index = i;
				return candidate;
			}
		}
	}

	for (uint32 i = 0; i < g_diligent_state.external_world_bsp_model_count; ++i)
	{
		const WorldBsp* candidate = g_diligent_state.external_world_bsp_models[i];
		if (!candidate)
		{
			continue;
		}
		if (poly_index < candidate->m_nPolies || poly_index < candidate->m_nSurfaces)
		{
			out_world_index = i;
			return candidate;
		}
	}

	return nullptr;
}

static DiligentWorldPipelineStats g_diligent_pipeline_stats{};
static LTVector g_diligent_world_offset(0.0f, 0.0f, 0.0f);

bool diligent_is_placeholder_texture(SharedTexture* texture)
{
	if (!texture || !g_diligent_state.render_struct)
	{
		return false;
	}

	TextureData* texture_data = g_diligent_state.render_struct->GetTexture(texture);
	if (!texture_data)
	{
		return false;
	}

	return texture_data->m_Header.m_BaseWidth <= 1 && texture_data->m_Header.m_BaseHeight <= 1;
}

SharedTexture* diligent_get_effect_linked_texture(SharedTexture* texture)
{
	if (!texture)
	{
		return nullptr;
	}

	const std::array<ELinkedTex, 4> linked_order = {
		eLinkedTex_EffectTexture1,
		eLinkedTex_EffectTexture2,
		eLinkedTex_EffectTexture3,
		eLinkedTex_EffectTexture4};

	SharedTexture* fallback = nullptr;
	for (const auto linked_type : linked_order)
	{
		if (auto* linked = texture->GetLinkedTexture(linked_type))
		{
			if (!fallback)
			{
				fallback = linked;
			}
			if (!diligent_is_placeholder_texture(linked))
			{
				return linked;
			}
		}
	}

	return fallback;
}

SharedTexture* diligent_get_section_surface_texture(const DiligentRenderBlock& block, const DiligentRBSection& section)
{
	if (section.poly_index == 0xFFFFFFFFu)
	{
		return nullptr;
	}

	auto* bsp_client = diligent_get_world_bsp_client();
	if (bsp_client && bsp_client->NumWorldModels() == 0)
	{
		bsp_client = nullptr;
	}
	const WorldBsp* external_bsp = nullptr;
	if (!bsp_client)
	{
		uint32 external_index = 0;
		external_bsp = diligent_pick_external_bsp(block, section.poly_index, external_index);
	}

	const WorldBsp* bsp = external_bsp;
	if (!bsp_client && !bsp)
	{
		return nullptr;
	}

	if (!bsp)
	{
		const uint32 model_count = bsp_client->NumWorldModels();
		const WorldData* world_model = nullptr;
		uint32 world_index = 0;
		if (block.world)
		{
			world_index = block.world->ResolveWorldIndex();
		}
		if (world_index < model_count)
		{
			world_model = bsp_client->GetWorldModel(world_index);
		}

		bsp = world_model ? world_model->OriginalBSP() : nullptr;
		if (!bsp || section.poly_index >= bsp->m_nPolies)
		{
			for (uint32 i = 0; i < model_count; ++i)
			{
				const WorldData* candidate = bsp_client->GetWorldModel(i);
				if (!candidate || !candidate->OriginalBSP())
				{
					continue;
				}
				const WorldBsp* candidate_bsp = candidate->OriginalBSP();
				if (section.poly_index < candidate_bsp->m_nPolies ||
					section.poly_index < candidate_bsp->m_nSurfaces)
				{
					bsp = candidate_bsp;
					break;
				}
			}
		}
		if (!bsp)
		{
			return nullptr;
		}
	}

	Surface* surface = nullptr;
	if (section.poly_index < bsp->m_nPolies && bsp->m_Polies)
	{
		WorldPoly* poly = bsp->m_Polies[section.poly_index];
		if (poly)
		{
			surface = poly->GetSurface();
		}
	}

	if (!surface && section.poly_index < bsp->m_nSurfaces && bsp->m_Surfaces)
	{
		surface = &bsp->m_Surfaces[section.poly_index];
	}

	if (!surface)
	{
		return nullptr;
	}

	if (auto* texture = surface->GetSharedTexture())
	{
		return texture;
	}

	if (!bsp->m_TextureNames || !g_diligent_state.render_struct)
	{
		return nullptr;
	}

	const char* texture_name = bsp->m_TextureNames[surface->m_iTexture];
	if (!texture_name || texture_name[0] == '\0')
	{
		return nullptr;
	}

	return g_diligent_state.render_struct->GetSharedTexture(texture_name);
}

SharedTexture* diligent_resolve_effect_texture(SharedTexture* texture)
{
	if (!texture)
	{
		return nullptr;
	}

	if (texture->m_eTexType == eSharedTexType_Effect)
	{
		if (auto* linked = diligent_get_effect_linked_texture(texture))
		{
			return linked;
		}
	}

	if (diligent_is_placeholder_texture(texture))
	{
		if (auto* linked = diligent_get_effect_linked_texture(texture))
		{
			return linked;
		}
	}

	return texture;
}

float diligent_calc_world_model_draw_distance(const LTObject* object, const ViewParams& params)
{
	if (!object)
	{
		return 0.0f;
	}

	if ((object->m_Flags & FLAG_REALLYCLOSE) == 0)
	{
		return (object->GetPos() - params.m_Pos).MagSqr();
	}

	return object->GetPos().MagSqr();
}

void diligent_sort_translucent_world_models(std::vector<WorldModelInstance*>& objects, const ViewParams& params)
{
	if (!g_CV_DrawSorted.m_Val || objects.size() < 2)
	{
		return;
	}

	std::sort(objects.begin(), objects.end(), [&params](const WorldModelInstance* a, const WorldModelInstance* b)
	{
		if (!a || !b)
		{
			return a != nullptr;
		}

		const auto* obj_a = static_cast<const LTObject*>(a);
		const auto* obj_b = static_cast<const LTObject*>(b);
		const bool a_close = (obj_a->m_Flags & FLAG_REALLYCLOSE) != 0;
		const bool b_close = (obj_b->m_Flags & FLAG_REALLYCLOSE) != 0;
		if (a_close != b_close)
		{
			return !a_close && b_close;
		}

		const float dist_a = diligent_calc_world_model_draw_distance(obj_a, params);
		const float dist_b = diligent_calc_world_model_draw_distance(obj_b, params);
		return dist_a > dist_b;
	});
}
} // namespace

DiligentWorldResources g_world_resources;
DiligentWorldPipelines g_world_pipelines;
std::vector<DiligentRenderBlock*> g_visible_render_blocks;
std::vector<WorldModelInstance*> g_diligent_solid_world_models;
std::vector<WorldModelInstance*> g_diligent_translucent_world_models;
DynamicLight* g_diligent_world_dynamic_lights[kDiligentMaxDynamicLights] = {};
uint32 g_diligent_num_world_dynamic_lights = 0;
bool g_diligent_shadow_mode = false;

namespace
{
int diligent_get_world_tex_debug_mode()
{
	return 0;
}

int diligent_get_world_ps_debug()
{
	static int last_value = -1;
	const int value = g_CV_WorldPsDebug.m_Val != 0 ? 1 : 0;
	if (value != last_value)
	{
		g_world_pipelines.pipelines.clear();
		last_value = value;
	}
	return value;
}

bool diligent_get_world_uv_debug()
{
	return g_CV_WorldUvDebug.m_Val != 0;
}

bool diligent_get_world_use_base_vertex()
{
	return g_CV_WorldUseBaseVertex.m_Val != 0;
}

bool diligent_get_force_textured_world()
{
	return g_CV_WorldForceTexture.m_Val != 0;
}
} // namespace

int diligent_get_world_shading_mode()
{
	int mode = g_CV_WorldShadingMode.m_Val;
	if (mode < kWorldShadingTextured || mode > kWorldShadingNormals)
	{
		mode = kWorldShadingTextured;
	}
	if (mode == kWorldShadingTextured && g_CV_DrawFlat.m_Val != 0)
	{
		mode = kWorldShadingFlat;
	}
	return mode;
}
uint32 diligent_get_texture_effect_stages(uint8 mode, std::array<DiligentTextureEffectStage, 4>& stages)
{
	stages = {};
	const bool swap_lightmap_uv = (g_CV_LightmapSwapUV.m_Val != 0);
	const bool has_uv1 = g_render_world && g_render_world->has_uv1;
	const bool base_use_uv1 = swap_lightmap_uv;
	const bool lightmap_use_uv1 = swap_lightmap_uv ? false : has_uv1;
	switch (mode)
	{
		case kWorldPipelineTextured:
		case kWorldPipelineGlowTextured:
			stages[0] = {TSChannel_Base, base_use_uv1};
			return 1;
		case kWorldPipelineLightmap:
			stages[0] = {TSChannel_Base, base_use_uv1};
			stages[1] = {TSChannel_LightMap, lightmap_use_uv1};
			return 2;
		case kWorldPipelineLightmapOnly:
			stages[0] = {TSChannel_LightMap, lightmap_use_uv1};
			return 1;
		case kWorldPipelineDualTexture:
			stages[0] = {TSChannel_Base, base_use_uv1};
			stages[1] = {TSChannel_DualTexture, true};
			return 2;
		case kWorldPipelineLightmapDual:
			stages[0] = {TSChannel_Base, base_use_uv1};
			stages[1] = {TSChannel_LightMap, lightmap_use_uv1};
			stages[2] = {TSChannel_DualTexture, true};
			return 3;
		default:
			break;
	}

	return 0;
}
void diligent_fill_world_blend_desc(DiligentWorldBlendMode blend, Diligent::RenderTargetBlendDesc& blend_desc)
{
	blend_desc.RenderTargetWriteMask = Diligent::COLOR_MASK_ALL;
	if (blend == kWorldBlendSolid)
	{
		blend_desc.BlendEnable = false;
		return;
	}

	blend_desc.BlendEnable = true;
	blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
	blend_desc.BlendOpAlpha = blend_desc.BlendOp;

	if (blend == kWorldBlendAdditive)
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
	}
	else if (blend == kWorldBlendAdditiveOne)
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
	}
	else if (blend == kWorldBlendMultiply)
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ZERO;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_SRC_COLOR;
	}
	else
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
	}

	blend_desc.SrcBlendAlpha = blend_desc.SrcBlend;
	blend_desc.DestBlendAlpha = blend_desc.DestBlend;
}
bool diligent_ensure_world_constant_buffer()
{
	if (g_world_resources.constant_buffer)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_world_constants";
	desc.Size = sizeof(DiligentWorldConstants);
	desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_world_resources.constant_buffer);
	return g_world_resources.constant_buffer != nullptr;
}
bool diligent_ensure_shadow_project_constants()
{
	if (g_world_resources.shadow_project_constants)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_shadow_project_constants";
	desc.Size = sizeof(DiligentShadowProjectConstants);
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_world_resources.shadow_project_constants);
	return g_world_resources.shadow_project_constants != nullptr;
}
bool diligent_ensure_world_shaders()
{
	if (g_world_resources.vertex_shader && g_world_resources.pixel_shader_textured && g_world_resources.pixel_shader_glow && g_world_resources.pixel_shader_lightmap && g_world_resources.pixel_shader_lightmap_only && g_world_resources.pixel_shader_dual && g_world_resources.pixel_shader_lightmap_dual && g_world_resources.pixel_shader_solid && g_world_resources.pixel_shader_flat && g_world_resources.pixel_shader_normals && g_world_resources.pixel_shader_dynamic_light && g_world_resources.pixel_shader_bump && g_world_resources.pixel_shader_volume_effect && g_world_resources.pixel_shader_shadow_project && g_world_resources.pixel_shader_debug)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	if (!g_world_resources.vertex_shader)
	{
		Diligent::ShaderDesc vertex_desc;
		vertex_desc.Name = "ltjs_world_vs";
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		shader_info.Desc = vertex_desc;
		shader_info.EntryPoint = "VSMain";
		shader_info.Source = diligent_get_world_vertex_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.vertex_shader);
		if (!g_world_resources.vertex_shader)
		{
			return false;
		}
	}

	Diligent::ShaderDesc pixel_desc;
	pixel_desc.Name = "ltjs_world_ps_textured";
	pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	shader_info.Desc = pixel_desc;
	shader_info.EntryPoint = "PSMain";

	if (!g_world_resources.pixel_shader_textured)
	{
		shader_info.Source = diligent_get_world_pixel_shader_textured_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_textured);
	}

	if (!g_world_resources.pixel_shader_glow)
	{
		shader_info.Source = diligent_get_world_pixel_shader_glow_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_glow);
	}

	if (!g_world_resources.pixel_shader_lightmap)
	{
		shader_info.Source = diligent_get_world_pixel_shader_lightmap_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_lightmap);
	}

	if (!g_world_resources.pixel_shader_lightmap_only)
	{
		shader_info.Source = diligent_get_world_pixel_shader_lightmap_only_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_lightmap_only);
	}

	if (!g_world_resources.pixel_shader_dual)
	{
		shader_info.Source = diligent_get_world_pixel_shader_dual_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_dual);
	}

	if (!g_world_resources.pixel_shader_bump)
	{
		shader_info.Source = diligent_get_world_pixel_shader_bump_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_bump);
	}

	if (!g_world_resources.pixel_shader_volume_effect)
	{
		shader_info.Source = diligent_get_world_pixel_shader_volume_effect_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_volume_effect);
	}

	if (!g_world_resources.pixel_shader_lightmap_dual)
	{
		shader_info.Source = diligent_get_world_pixel_shader_lightmap_dual_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_lightmap_dual);
	}

	if (!g_world_resources.pixel_shader_solid)
	{
		shader_info.Source = diligent_get_world_pixel_shader_solid_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_solid);
	}
	if (!g_world_resources.pixel_shader_flat)
	{
		shader_info.Source = diligent_get_world_pixel_shader_flat_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_flat);
	}
	if (!g_world_resources.pixel_shader_normals)
	{
		shader_info.Source = diligent_get_world_pixel_shader_normals_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_normals);
	}

	if (!g_world_resources.pixel_shader_dynamic_light)
	{
		shader_info.Source = diligent_get_world_pixel_shader_dynamic_light_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_dynamic_light);
	}

	if (!g_world_resources.pixel_shader_shadow_project)
	{
		shader_info.Source = diligent_get_world_pixel_shader_shadow_project_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_shadow_project);
	}

	if (!g_world_resources.pixel_shader_debug)
	{
		shader_info.Source = diligent_get_world_pixel_shader_debug_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_debug);
	}

	return g_world_resources.pixel_shader_textured && g_world_resources.pixel_shader_glow && g_world_resources.pixel_shader_lightmap && g_world_resources.pixel_shader_lightmap_only && g_world_resources.pixel_shader_dual && g_world_resources.pixel_shader_lightmap_dual && g_world_resources.pixel_shader_solid && g_world_resources.pixel_shader_flat && g_world_resources.pixel_shader_normals && g_world_resources.pixel_shader_dynamic_light && g_world_resources.pixel_shader_bump && g_world_resources.pixel_shader_volume_effect && g_world_resources.pixel_shader_shadow_project && g_world_resources.pixel_shader_debug;
}
DiligentWorldPipeline* diligent_get_world_pipeline(
	uint8 mode,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	bool wireframe)
{
	const uint8 sample_count = static_cast<uint8>(diligent_get_active_sample_count());
	DiligentWorldPipelineKey key{
		mode,
		blend_mode,
		depth_mode,
		static_cast<uint8>(wireframe ? 1 : 0),
		sample_count
	};
	auto it = g_world_pipelines.pipelines.find(key);
	if (it != g_world_pipelines.pipelines.end())
	{
		return &it->second;
	}

	if (!diligent_ensure_world_shaders())
	{
		return nullptr;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return nullptr;
	}

	if (mode == kWorldPipelineShadowProject && !diligent_ensure_shadow_project_constants())
	{
		return nullptr;
	}

	if (!g_diligent_state.render_device || !g_diligent_state.swap_chain)
	{
		return nullptr;
	}

	const auto swap_desc = g_diligent_state.swap_chain->GetDesc();

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	if (mode == kWorldPipelineLightmap)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_lightmap";
	}
	else if (mode == kWorldPipelineTextured)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_textured";
	}
	else if (mode == kWorldPipelineGlowTextured)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_glow";
	}
	else if (mode == kWorldPipelineLightmapOnly)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_lightmap_only";
	}
	else if (mode == kWorldPipelineDualTexture)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_dual";
	}
	else if (mode == kWorldPipelineLightmapDual)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_lightmap_dual";
	}
	else if (mode == kWorldPipelineDynamicLight)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_dynamic_light";
	}
	else if (mode == kWorldPipelineBump)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_bump";
	}
	else if (mode == kWorldPipelineVolumeEffect)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_volume_effect";
	}
	else if (mode == kWorldPipelineShadowProject)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_shadow_project";
	}
	else if (mode == kWorldPipelineFlat)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_flat";
	}
	else if (mode == kWorldPipelineNormals)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_normals";
	}
	else
	{
		pipeline_info.PSODesc.Name = "ltjs_world_unlit";
	}
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = swap_desc.ColorBufferFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = swap_desc.DepthBufferFormat;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline_info.GraphicsPipeline.SmplDesc.Count = sample_count;

	constexpr auto kWorldVertexStride = static_cast<uint32>(sizeof(DiligentWorldVertex));
	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false, static_cast<uint32>(offsetof(DiligentWorldVertex, position)), kWorldVertexStride},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false, static_cast<uint32>(offsetof(DiligentWorldVertex, color)), kWorldVertexStride},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false, static_cast<uint32>(offsetof(DiligentWorldVertex, uv0)), kWorldVertexStride},
		Diligent::LayoutElement{3, 0, 2, Diligent::VT_FLOAT32, false, static_cast<uint32>(offsetof(DiligentWorldVertex, uv1)), kWorldVertexStride},
		Diligent::LayoutElement{4, 0, 3, Diligent::VT_FLOAT32, false, static_cast<uint32>(offsetof(DiligentWorldVertex, normal)), kWorldVertexStride}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	pipeline_info.GraphicsPipeline.RasterizerDesc.FillMode =
		wireframe ? Diligent::FILL_MODE_WIREFRAME : Diligent::FILL_MODE_SOLID;
	if (mode == kWorldPipelineShadowProject && g_CV_ModelShadow_Proj_BackFaceCull.m_Val)
	{
		pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
		pipeline_info.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = true;
	}
	const bool depth_enabled = (depth_mode != kWorldDepthDisabled);
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = depth_enabled;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = depth_enabled;
	if (mode == kWorldPipelineDynamicLight)
	{
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_EQUAL;
		auto& blend_desc = pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0];
		blend_desc.BlendEnable = true;
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
		blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
		blend_desc.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
		blend_desc.BlendOpAlpha = Diligent::BLEND_OPERATION_ADD;
	}
	else if (mode == kWorldPipelineShadowProject)
	{
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_EQUAL;
		diligent_fill_world_blend_desc(kWorldBlendMultiply, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);
	}
	else
	{
		if (!depth_enabled || blend_mode != kWorldBlendSolid)
		{
			pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
		}
		if (!depth_enabled)
		{
			pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_ALWAYS;
		}
		else if (blend_mode == kWorldBlendMultiply)
		{
			pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_EQUAL;
		}
		diligent_fill_world_blend_desc(blend_mode, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);
	}

	pipeline_info.pVS = g_world_resources.vertex_shader;
	if (diligent_get_world_ps_debug() != 0)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_debug;
	}
	else if (mode == kWorldPipelineLightmap)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_lightmap;
	}
	else if (mode == kWorldPipelineLightmapOnly)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_lightmap_only;
	}
	else if (mode == kWorldPipelineDualTexture)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_dual;
	}
	else if (mode == kWorldPipelineLightmapDual)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_lightmap_dual;
	}
	else if (mode == kWorldPipelineTextured)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_textured;
	}
	else if (mode == kWorldPipelineGlowTextured)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_glow;
	}
	else if (mode == kWorldPipelineDynamicLight)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_dynamic_light;
	}
	else if (mode == kWorldPipelineBump)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_bump;
	}
	else if (mode == kWorldPipelineVolumeEffect)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_volume_effect;
	}
	else if (mode == kWorldPipelineShadowProject)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_shadow_project;
	}
	else if (mode == kWorldPipelineFlat)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_flat;
	}
	else if (mode == kWorldPipelineNormals)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_normals;
	}
	else
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_solid;
	}

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::ShaderResourceVariableDesc variables[] = {
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture0", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture1", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture2", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_ShadowTexture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	Diligent::ImmutableSamplerDesc sampler_desc[3];
	sampler_desc[0].ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc[0].Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[0].Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[0].Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[0].Desc.MipLODBias = g_CV_MipMapBias.m_Val;
	sampler_desc[0].Desc.AddressU = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[0].Desc.AddressV = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[0].Desc.AddressW = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[0].SamplerOrTextureName = "g_Texture0_sampler";
	diligent_apply_anisotropy(sampler_desc[0].Desc);
	sampler_desc[1].ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc[1].Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[1].Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[1].Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[1].Desc.MipLODBias = g_CV_MipMapBias.m_Val;
	sampler_desc[1].Desc.AddressU = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[1].Desc.AddressV = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[1].Desc.AddressW = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[1].SamplerOrTextureName = "g_Texture1_sampler";
	diligent_apply_anisotropy(sampler_desc[1].Desc);
	sampler_desc[2].ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc[2].Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[2].Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[2].Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[2].Desc.MipLODBias = g_CV_MipMapBias.m_Val;
	sampler_desc[2].Desc.AddressU = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[2].Desc.AddressV = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[2].Desc.AddressW = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc[2].SamplerOrTextureName = "g_Texture2_sampler";
	diligent_apply_anisotropy(sampler_desc[2].Desc);

	if (mode == kWorldPipelineVolumeEffect)
	{
		sampler_desc[0].Desc.AddressU = Diligent::TEXTURE_ADDRESS_CLAMP;
		sampler_desc[0].Desc.AddressV = Diligent::TEXTURE_ADDRESS_CLAMP;
		sampler_desc[0].Desc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
	}
	else if (mode == kWorldPipelineLightmap ||
		mode == kWorldPipelineLightmapOnly ||
		mode == kWorldPipelineLightmapDual)
	{
		sampler_desc[1].Desc.AddressU = Diligent::TEXTURE_ADDRESS_CLAMP;
		sampler_desc[1].Desc.AddressV = Diligent::TEXTURE_ADDRESS_CLAMP;
		sampler_desc[1].Desc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
	}

	if (mode == kWorldPipelineTextured || mode == kWorldPipelineGlowTextured || mode == kWorldPipelineVolumeEffect)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 1u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1u;
	}
	else if (mode == kWorldPipelineLightmap)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 2u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 2u;
	}
	else if (mode == kWorldPipelineLightmapOnly)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables + 1;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 1u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc + 1;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1u;
	}
	else if (mode == kWorldPipelineDualTexture)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 2u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 2u;
	}
	else if (mode == kWorldPipelineBump)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 2u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 2u;
	}
	else if (mode == kWorldPipelineLightmapDual)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 3u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 3u;
	}
	else if (mode == kWorldPipelineShadowProject)
	{
		sampler_desc[0].Desc.AddressU = Diligent::TEXTURE_ADDRESS_CLAMP;
		sampler_desc[0].Desc.AddressV = Diligent::TEXTURE_ADDRESS_CLAMP;
		sampler_desc[0].Desc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
			sampler_desc[0].SamplerOrTextureName = "g_ShadowTexture_sampler";
		pipeline_info.PSODesc.ResourceLayout.Variables = variables + 3;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 1u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1u;
	}
	else
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = nullptr;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 0u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = nullptr;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 0u;
	}

	DiligentWorldPipeline pipeline;
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* vs_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "WorldConstants");
	if (vs_constants)
	{
		vs_constants->Set(g_world_resources.constant_buffer);
	}

	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "WorldConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_world_resources.constant_buffer);
	}

	if (mode == kWorldPipelineShadowProject)
	{
		auto* shadow_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "ShadowProjectConstants");
		if (shadow_constants)
		{
			shadow_constants->Set(g_world_resources.shadow_project_constants);
		}
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	auto result = g_world_pipelines.pipelines.emplace(key, std::move(pipeline));
	return result.second ? &result.first->second : nullptr;
}
bool diligent_ensure_world_vertex_buffer(uint32 size)
{
	if (g_world_resources.vertex_buffer && g_world_resources.vertex_buffer_size >= size)
	{
		return true;
	}

	if (!g_diligent_state.render_device || size == 0)
	{
		return false;
	}

	g_world_resources.vertex_buffer.Release();
	g_world_resources.vertex_buffer_size = 0;

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_world_vertices";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = sizeof(DiligentWorldVertex);

	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_world_resources.vertex_buffer);
	if (!g_world_resources.vertex_buffer)
	{
		return false;
	}

	g_world_resources.vertex_buffer_size = size;
	return true;
}
bool diligent_ensure_world_index_buffer(uint32 size)
{
	if (g_world_resources.index_buffer && g_world_resources.index_buffer_size >= size)
	{
		return true;
	}

	if (!g_diligent_state.render_device || size == 0)
	{
		return false;
	}

	g_world_resources.index_buffer.Release();
	g_world_resources.index_buffer_size = 0;

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_world_indices";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_INDEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = sizeof(uint16);

	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_world_resources.index_buffer);
	if (!g_world_resources.index_buffer)
	{
		return false;
	}

	g_world_resources.index_buffer_size = size;
	return true;
}
void diligent_set_world_vertex_color(DiligentWorldVertex& vertex, uint32 color)
{
	vertex.color[0] = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
	vertex.color[1] = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
	vertex.color[2] = static_cast<float>(color & 0xFF) / 255.0f;
	vertex.color[3] = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
}
void diligent_set_world_vertex_normal(DiligentWorldVertex& vertex, const LTVector& normal)
{
	vertex.normal[0] = normal.x;
	vertex.normal[1] = normal.y;
	vertex.normal[2] = normal.z;
}
void diligent_init_texture_effect_constants(DiligentWorldConstants& constants)
{
	for (auto& matrix : constants.tex_effect_matrices)
	{
		diligent_store_identity_matrix(matrix);
	}
	for (auto& params : constants.tex_effect_params)
	{
		params = {0, 3, 2, 0};
	}
	for (auto& uv : constants.tex_effect_uv)
	{
		uv = {0, 0, 0, 0};
	}
}
bool diligent_pipeline_allows_texel_uv(uint8 pipeline_mode)
{
	switch (pipeline_mode)
	{
		case kWorldPipelineLightmap:
		case kWorldPipelineLightmapOnly:
		case kWorldPipelineLightmapDual:
			return false;
		default:
			return true;
	}
}
bool diligent_section_supports_texel_uv(const DiligentRBSection& section)
{
	const auto shader_type = static_cast<DiligentPCShaderType>(section.shader_code);
	return shader_type == kPcShaderLightmap ||
		shader_type == kPcShaderLightmapTexture ||
		shader_type == kPcShaderLightmapDualTexture;
}
bool diligent_section_uses_texel_uv(
	const DiligentRenderBlock& block,
	const DiligentRBSection& section,
	uint32 width,
	uint32 height)
{
	if (width == 0 || height == 0)
	{
		return false;
	}

	const uint32 start = section.start_vertex;
	if (start >= block.vertices.size() || section.vertex_count == 0)
	{
		return false;
	}

	const uint32 end = std::min(
		start + section.vertex_count,
		static_cast<uint32>(block.vertices.size()));
	const uint32 count = end - start;
	if (count == 0)
	{
		return false;
	}

	constexpr uint32 kSampleTarget = 64;
	const uint32 step = std::max(1u, count / kSampleTarget);
	float min_u = std::numeric_limits<float>::max();
	float min_v = std::numeric_limits<float>::max();
	float max_u = std::numeric_limits<float>::lowest();
	float max_v = std::numeric_limits<float>::lowest();
	uint32 samples = 0;
	uint32 integer_samples = 0;
	uint32 uv1_zero_samples = 0;
	constexpr float kIntegerEpsilon = 0.01f;
	constexpr float kUv1ZeroEps = 0.0001f;

	for (uint32 i = start; i < end; i += step)
	{
		const auto& vert = block.vertices[i];
		const float u = vert.u0;
		const float v = vert.v0;
		if (!std::isfinite(u) || !std::isfinite(v))
		{
			continue;
		}
		min_u = std::min(min_u, u);
		max_u = std::max(max_u, u);
		min_v = std::min(min_v, v);
		max_v = std::max(max_v, v);

		const float u_rounded = std::round(u);
		const float v_rounded = std::round(v);
		if (std::abs(u - u_rounded) <= kIntegerEpsilon && std::abs(v - v_rounded) <= kIntegerEpsilon)
		{
			++integer_samples;
		}
		if (std::abs(vert.u1) <= kUv1ZeroEps && std::abs(vert.v1) <= kUv1ZeroEps)
		{
			++uv1_zero_samples;
		}
		++samples;
	}

	if (samples == 0)
	{
		return false;
	}

	const float range_u = max_u - min_u;
	const float range_v = max_v - min_v;
	if (range_u <= 0.0f && range_v <= 0.0f)
	{
		return false;
	}

	const float repeats_u = range_u / static_cast<float>(width);
	const float repeats_v = range_v / static_cast<float>(height);
	const float extent = std::max(range_u, range_v);
	const float uv1_zero_ratio = static_cast<float>(uv1_zero_samples) / static_cast<float>(samples);
	if (uv1_zero_ratio >= 0.9f && extent > 2.0f)
	{
		return true;
	}

	constexpr float kRepeatTolerance = 0.25f;
	constexpr float kMinRepeats = 0.05f;
	constexpr float kMaxRepeats = 64.0f;
	const auto repeats_match = [](float repeats) -> bool {
		if (!std::isfinite(repeats))
		{
			return false;
		}
		if (repeats < kMinRepeats || repeats > kMaxRepeats)
		{
			return false;
		}
		return std::abs(repeats - std::round(repeats)) <= kRepeatTolerance;
	};
	const bool repeat_ok = repeats_match(repeats_u) || repeats_match(repeats_v);
	const float integer_ratio = static_cast<float>(integer_samples) / static_cast<float>(samples);
	constexpr float kMinIntegerRatio = 0.6f;

	return repeat_ok && integer_ratio >= kMinIntegerRatio;
}
bool diligent_get_texel_uv_scale(
	const DiligentRenderBlock& block,
	const DiligentRBSection& section,
	const DiligentTextureEffectStage& stage,
	uint8 pipeline_mode,
	float& out_scale_u,
	float& out_scale_v)
{
	if (!diligent_pipeline_allows_texel_uv(pipeline_mode))
	{
		return false;
	}
	if (!diligent_section_supports_texel_uv(section))
	{
		return false;
	}
	if (stage.use_uv1 || stage.channel == TSChannel_LightMap)
	{
		return false;
	}

	SharedTexture* stage_texture = nullptr;
	if (stage.channel == TSChannel_Base)
	{
		stage_texture = diligent_resolve_effect_texture(section.textures[0]);
		if (diligent_is_placeholder_texture(stage_texture))
		{
			if (SharedTexture* surface_texture = diligent_get_section_surface_texture(block, section))
			{
				stage_texture = diligent_resolve_effect_texture(surface_texture);
			}
		}
	}
	else if (stage.channel == TSChannel_DualTexture)
	{
		stage_texture = diligent_resolve_effect_texture(section.textures[1]);
	}

	if (!stage_texture || !g_diligent_state.render_struct)
	{
		return false;
	}

	TextureData* texture_data = g_diligent_state.render_struct->GetTexture(stage_texture);
	if (!texture_data || texture_data->m_Header.m_BaseWidth == 0 || texture_data->m_Header.m_BaseHeight == 0)
	{
		return false;
	}

	const uint32 width = texture_data->m_Header.m_BaseWidth;
	const uint32 height = texture_data->m_Header.m_BaseHeight;
	if (!diligent_section_uses_texel_uv(block, section, width, height))
	{
		return false;
	}

	out_scale_u = 1.0f / static_cast<float>(width);
	out_scale_v = 1.0f / static_cast<float>(height);
	return true;
}
bool diligent_section_applies_texel_uv(
	const DiligentRenderBlock& block,
	const DiligentRBSection& section,
	uint8 pipeline_mode)
{
	std::array<DiligentTextureEffectStage, 4> stages{};
	const uint32 stage_count = diligent_get_texture_effect_stages(pipeline_mode, stages);
	if (stage_count == 0)
	{
		return false;
	}

	for (uint32 stage_index = 0; stage_index < stage_count; ++stage_index)
	{
		float scale_u = 1.0f;
		float scale_v = 1.0f;
		if (diligent_get_texel_uv_scale(block, section, stages[stage_index], pipeline_mode, scale_u, scale_v))
		{
			return true;
		}
	}

	return false;
}
void diligent_apply_texture_effect_constants(
	const DiligentRenderBlock& block,
	const DiligentRBSection& section,
	uint8 pipeline_mode,
	DiligentWorldConstants& constants)
{
	diligent_init_texture_effect_constants(constants);

	std::array<DiligentTextureEffectStage, 4> stages{};
	const uint32 stage_count = diligent_get_texture_effect_stages(pipeline_mode, stages);
	if (stage_count == 0)
	{
		return;
	}

	for (uint32 stage_index = 0; stage_index < stage_count; ++stage_index)
	{
		const auto& stage = stages[stage_index];
		const int32 use_uv1 = stage.use_uv1 ? 1 : 0;
		constants.tex_effect_uv[stage_index] = {use_uv1, 0, 0, 0};

		bool apply_scale = false;
		float scale_u = 1.0f;
		float scale_v = 1.0f;
		apply_scale = diligent_get_texel_uv_scale(block, section, stage, pipeline_mode, scale_u, scale_v);

		TextureScriptStageData stage_data;
		if (!section.texture_effect || !section.texture_effect->GetStageData(stage.channel, stage_data))
		{
			const int32 enable = apply_scale ? 1 : 0;
			constants.tex_effect_params[stage_index] = {
				enable,
				static_cast<int32>(ITextureScriptEvaluator::INPUT_UV),
				2,
				0};
			if (apply_scale)
			{
				LTMatrix scale_matrix;
				scale_matrix.Identity();
				scale_matrix.m[0][0] = scale_u;
				scale_matrix.m[1][1] = scale_v;
				diligent_store_matrix_from_lt(scale_matrix, constants.tex_effect_matrices[stage_index]);
			}
			continue;
		}

		int32 coord_count = 2;
		if (stage_data.flags & ITextureScriptEvaluator::FLAG_COORD1)
		{
			coord_count = 1;
		}
		else if (stage_data.flags & ITextureScriptEvaluator::FLAG_COORD2)
		{
			coord_count = 2;
		}
		else if (stage_data.flags & ITextureScriptEvaluator::FLAG_COORD3)
		{
			coord_count = 3;
		}
		else if (stage_data.flags & ITextureScriptEvaluator::FLAG_COORD4)
		{
			coord_count = 4;
		}

		const int32 projected = (stage_data.flags & ITextureScriptEvaluator::FLAG_PROJECTED) ? 1 : 0;
		constants.tex_effect_params[stage_index] = {1, static_cast<int32>(stage_data.input_type), coord_count, projected};
		LTMatrix final_matrix = stage_data.transform;
		if (apply_scale)
		{
			LTMatrix scale_matrix;
			scale_matrix.Identity();
			scale_matrix.m[0][0] = scale_u;
			scale_matrix.m[1][1] = scale_v;
			final_matrix = scale_matrix * final_matrix;
		}
		diligent_store_matrix_from_lt(final_matrix, constants.tex_effect_matrices[stage_index]);
	}
}
bool diligent_draw_world_immediate(
	const DiligentWorldConstants& constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* texture1_view,
	bool use_bump,
	const DiligentWorldVertex* vertices,
	uint32 vertex_count,
	const uint16* indices,
	uint32 index_count)
{
	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	if (!vertices || vertex_count == 0 || !indices || index_count == 0)
	{
		return true;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

	const uint32 vertex_bytes = vertex_count * sizeof(DiligentWorldVertex);
	const uint32 index_bytes = index_count * sizeof(uint16);
	if (!diligent_ensure_world_vertex_buffer(vertex_bytes) || !diligent_ensure_world_index_buffer(index_bytes))
	{
		return false;
	}

	void* mapped_vertices = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	if (!mapped_vertices)
	{
		return false;
	}
	std::memcpy(mapped_vertices, vertices, vertex_bytes);
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE);

	void* mapped_indices = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.index_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_indices);
	if (!mapped_indices)
	{
		return false;
	}
	std::memcpy(mapped_indices, indices, index_bytes);
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.index_buffer, Diligent::MAP_WRITE);

	auto* render_target = diligent_get_active_render_target();
	auto* depth_target = diligent_get_active_depth_target();
	g_diligent_state.immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::IBuffer* buffers[] = {g_world_resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
	g_diligent_state.immediate_context->SetIndexBuffer(g_world_resources.index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	uint8 mode = kWorldPipelineSolid;
	if (use_bump && texture_view && texture1_view)
	{
		mode = kWorldPipelineBump;
	}
	else if (texture_view && texture1_view)
	{
		mode = kWorldPipelineDualTexture;
	}
	else if (texture_view)
	{
		mode = kWorldPipelineTextured;
	}
	const int shading_mode = diligent_get_world_shading_mode();
	if (shading_mode == kWorldShadingFlat)
	{
		mode = kWorldPipelineFlat;
	}
	else if (shading_mode == kWorldShadingNormals)
	{
		mode = kWorldPipelineNormals;
	}
	auto* pipeline = diligent_get_world_pipeline(mode, blend_mode, depth_mode, g_CV_Wireframe.m_Val != 0);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	if (texture_view)
	{
		auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
		if (texture_var)
		{
			texture_var->Set(texture_view);
		}
	}

	if (texture1_view)
	{
		auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
		if (texture_var)
		{
			texture_var->Set(texture1_view);
		}
	}

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs draw_attribs;
	draw_attribs.NumIndices = index_count;
	draw_attribs.IndexType = Diligent::VT_UINT16;
	draw_attribs.FirstIndexLocation = 0;
	draw_attribs.BaseVertex = 0;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->DrawIndexed(draw_attribs);

	return true;
}
bool diligent_draw_world_immediate_mode(
	const DiligentWorldConstants& constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	uint8 mode,
	Diligent::ITextureView* texture_view,
	const DiligentWorldVertex* vertices,
	uint32 vertex_count,
	const uint16* indices,
	uint32 index_count)
{
	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	if (!vertices || vertex_count == 0 || !indices || index_count == 0)
	{
		return true;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

	const uint32 vertex_bytes = vertex_count * sizeof(DiligentWorldVertex);
	const uint32 index_bytes = index_count * sizeof(uint16);
	if (!diligent_ensure_world_vertex_buffer(vertex_bytes) || !diligent_ensure_world_index_buffer(index_bytes))
	{
		return false;
	}

	void* mapped_vertices = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	if (!mapped_vertices)
	{
		return false;
	}
	std::memcpy(mapped_vertices, vertices, vertex_bytes);
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE);

	void* mapped_indices = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.index_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_indices);
	if (!mapped_indices)
	{
		return false;
	}
	std::memcpy(mapped_indices, indices, index_bytes);
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.index_buffer, Diligent::MAP_WRITE);

	auto* render_target = diligent_get_active_render_target();
	auto* depth_target = diligent_get_active_depth_target();
	g_diligent_state.immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::IBuffer* buffers[] = {g_world_resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
	g_diligent_state.immediate_context->SetIndexBuffer(g_world_resources.index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	const int shading_mode = diligent_get_world_shading_mode();
	if (shading_mode == kWorldShadingFlat)
	{
		mode = kWorldPipelineFlat;
	}
	else if (shading_mode == kWorldShadingNormals)
	{
		mode = kWorldPipelineNormals;
	}
	auto* pipeline = diligent_get_world_pipeline(mode, blend_mode, depth_mode, g_CV_Wireframe.m_Val != 0);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	if (texture_view)
	{
		auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
		if (texture_var)
		{
			texture_var->Set(texture_view);
		}
	}

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs draw_attribs;
	draw_attribs.NumIndices = index_count;
	draw_attribs.IndexType = Diligent::VT_UINT16;
	draw_attribs.FirstIndexLocation = 0;
	draw_attribs.BaseVertex = 0;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->DrawIndexed(draw_attribs);

	return true;
}
void diligent_fill_world_constants(
	const ViewParams& view_params,
	const LTMatrix& world_matrix,
	bool fog_enabled,
	float fog_near,
	float fog_far,
	DiligentWorldConstants& constants)
{
	LTMatrix mvp = view_params.m_mProjection * view_params.m_mView * world_matrix;
	diligent_store_matrix_from_lt(mvp, constants.mvp);
	diligent_store_matrix_from_lt(view_params.m_mView, constants.view);
	diligent_store_matrix_from_lt(world_matrix, constants.world);
	constants.camera_pos[0] = view_params.m_Pos.x;
	constants.camera_pos[1] = view_params.m_Pos.y;
	constants.camera_pos[2] = view_params.m_Pos.z;
	constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
	constants.fog_color[0] = static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f;
	constants.fog_color[1] = static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f;
	constants.fog_color[2] = static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f;
	constants.fog_color[3] = fog_near;
	constants.fog_params[0] = fog_far;
	constants.fog_params[1] = diligent_get_tonemap_enabled();
	constants.fog_params[2] = diligent_get_swapchain_output_is_srgb();
	if (g_diligent_state.render_struct)
	{
		const LTVector& dir = g_diligent_state.render_struct->m_GlobalLightDir;
		const LTVector& color = g_diligent_state.render_struct->m_GlobalLightColor;
		constants.sun_dir[0] = dir.x;
		constants.sun_dir[1] = dir.y;
		constants.sun_dir[2] = dir.z;
		constants.sun_dir[3] = 0.0f;
		constants.sun_color[0] = std::max(0.0f, color.x);
		constants.sun_color[1] = std::max(0.0f, color.y);
		constants.sun_color[2] = std::max(0.0f, color.z);
		constants.sun_color[3] = 1.0f;
	}
	else
	{
		constants.sun_dir[0] = 0.0f;
		constants.sun_dir[1] = 0.0f;
		constants.sun_dir[2] = 1.0f;
		constants.sun_dir[3] = 0.0f;
		constants.sun_color[0] = 0.0f;
		constants.sun_color[1] = 0.0f;
		constants.sun_color[2] = 0.0f;
		constants.sun_color[3] = 0.0f;
	}
	const int tex_debug_mode = diligent_get_world_tex_debug_mode();
	if (tex_debug_mode != 0)
	{
		constants.fog_params[3] = static_cast<float>(tex_debug_mode);
	}
	else
	{
		constants.fog_params[3] = diligent_get_world_uv_debug() ? 1.0f : 0.0f;
	}
	diligent_init_texture_effect_constants(constants);
}
bool diligent_draw_render_blocks_with_constants(
	const std::vector<DiligentRenderBlock*>& blocks,
	const DiligentWorldConstants& base_constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	DiligentWorldSectionFilter section_filter)
{
	if (blocks.empty())
	{
		return true;
	}

	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	const int shading_mode = diligent_get_world_shading_mode();
	const bool wireframe_overlay = (g_CV_WireframeOverlay.m_Val != 0) && (g_CV_Wireframe.m_Val == 0);

	auto* render_target = diligent_get_active_render_target();
	auto* depth_target = diligent_get_active_depth_target();
	g_diligent_state.immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	for (auto* block : blocks)
	{
		if (!block || block->vertices.empty() || block->indices.empty())
		{
			continue;
		}

		if (!block->EnsureGpuBuffers())
		{
			continue;
		}

		block->UpdateLightmaps();

		Diligent::IBuffer* buffers[] = {block->vertex_buffer};
		Diligent::Uint64 offsets[] = {0};
		g_diligent_state.immediate_context->SetVertexBuffers(
			0,
			1,
			buffers,
			offsets,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
		g_diligent_state.immediate_context->SetIndexBuffer(block->index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		for (const auto& section_ptr : block->sections)
		{
			if (!section_ptr)
			{
				continue;
			}

			auto& section = *section_ptr;
			const bool placeholder_light_anim =
				section.light_anim && section.textures[0] && diligent_is_placeholder_texture(section.textures[0]);
			if (section_filter == DiligentWorldSectionFilter::Normal)
			{
				if (section.light_anim && !placeholder_light_anim)
				{
					continue;
				}
			}
			else if (section_filter == DiligentWorldSectionFilter::LightAnim)
			{
				if (!section.light_anim || placeholder_light_anim)
				{
					continue;
				}
			}

			Diligent::ITextureView* texture_view = nullptr;
			Diligent::ITextureView* dual_texture_view = nullptr;
			if (section.textures[0])
			{
				SharedTexture* base_texture = diligent_resolve_effect_texture(section.textures[0]);
				if (diligent_is_placeholder_texture(base_texture))
				{
					SharedTexture* surface_texture = diligent_get_section_surface_texture(*block, section);
					if (surface_texture)
					{
						base_texture = diligent_resolve_effect_texture(surface_texture);
						if (base_texture && base_texture != section.textures[0])
						{
							if (section.textures[0])
							{
								section.textures[0]->SetRefCount(section.textures[0]->GetRefCount() - 1);
							}
							base_texture->SetRefCount(base_texture->GetRefCount() + 1);
							section.textures[0] = base_texture;
							if (g_diligent_state.render_struct)
							{
								TextureData* texture_data = g_diligent_state.render_struct->GetTexture(base_texture);
								section.fullbright =
									texture_data && (texture_data->m_Header.m_IFlags & DTX_FULLBRITE) != 0;
							}
						}
					}
				}
				texture_view = diligent_get_texture_view(base_texture, false);
			}
			if (section.textures[1])
			{
				dual_texture_view = diligent_get_texture_view(diligent_resolve_effect_texture(section.textures[1]), false);
			}
			const bool had_lightmap_data =
				(section.lightmap_size != 0 && section.lightmap_width != 0 && section.lightmap_height != 0);
			if (had_lightmap_data)
			{
				++g_diligent_pipeline_stats.sections_lightmap_data;
			}
			if (section.light_anim)
			{
				++g_diligent_pipeline_stats.sections_light_anim;
			}

			Diligent::ITextureView* lightmap_view = diligent_get_lightmap_view(section);
			if (lightmap_view)
			{
				++g_diligent_pipeline_stats.sections_lightmap_view;
			}
			else if (had_lightmap_data && section.lightmap_size == 0 && section.lightmap_data.empty())
			{
				++g_diligent_pipeline_stats.sections_lightmap_black;
			}
			if (g_CV_LightMap.m_Val == 0)
			{
				lightmap_view = nullptr;
			}

			const auto shader_type = static_cast<DiligentPCShaderType>(section.shader_code);
			const bool can_combine_lightmap = g_render_world && g_render_world->has_uv1;
			const bool is_lightmap_texture_section =
				(!section.light_anim &&
					(shader_type == kPcShaderLightmapTexture || shader_type == kPcShaderLightmapDualTexture));

			uint8 mode = kWorldPipelineSkip;
			if (section.light_anim)
			{
				mode = texture_view ? kWorldPipelineTextured : kWorldPipelineSolid;
			}
			else
			{
				switch (shader_type)
				{
					case kPcShaderGouraud:
						mode = texture_view ? kWorldPipelineTextured : kWorldPipelineSolid;
						break;
					case kPcShaderLightmap:
						if (lightmap_view)
						{
							mode = kWorldPipelineLightmapOnly;
						}
						else if (texture_view)
						{
							mode = kWorldPipelineTextured;
						}
						else
						{
							mode = kWorldPipelineSolid;
						}
						break;
					case kPcShaderLightmapTexture:
						if (texture_view && lightmap_view && can_combine_lightmap)
						{
							mode = kWorldPipelineLightmap;
						}
						else if (texture_view)
						{
							mode = kWorldPipelineTextured;
						}
						else if (lightmap_view)
						{
							// Skip: kPcShaderLightmapTexture expects a texture, but none is available.
							mode = kWorldPipelineSkip;
						}
						else
						{
							mode = kWorldPipelineSolid;
						}
						break;
					case kPcShaderDualTexture:
						if (texture_view && dual_texture_view)
						{
							mode = kWorldPipelineDualTexture;
						}
						else if (texture_view)
						{
							mode = kWorldPipelineTextured;
						}
						else
						{
							mode = kWorldPipelineSolid;
						}
						break;
					case kPcShaderLightmapDualTexture:
						if (texture_view && lightmap_view && dual_texture_view && can_combine_lightmap)
						{
							mode = kWorldPipelineLightmapDual;
						}
						else if (texture_view && dual_texture_view)
						{
							mode = kWorldPipelineDualTexture;
						}
						else if (texture_view && lightmap_view)
						{
							mode = can_combine_lightmap ? kWorldPipelineLightmap : kWorldPipelineTextured;
						}
						else if (texture_view)
						{
							mode = kWorldPipelineTextured;
						}
						else if (lightmap_view)
						{
							// Skip: kPcShaderLightmapDualTexture expects textures, but none available.
							mode = kWorldPipelineSkip;
						}
						else
						{
							mode = kWorldPipelineSolid;
						}
						break;
					case kPcShaderSkypan:
					case kPcShaderSkyPortal:
					case kPcShaderOccluder:
					case kPcShaderNone:
					case kPcShaderUnknown:
					default:
						mode = kWorldPipelineSkip;
						break;
				}
			}

			if (diligent_get_force_textured_world())
			{
				mode = texture_view ? kWorldPipelineTextured : kWorldPipelineSolid;
			}

			if (g_CV_ShadersEnabled.m_Val == 0)
			{
				mode = kWorldPipelineSolid;
			}

			if (!section.light_anim && g_CV_LightmapsOnly.m_Val != 0)
			{
				mode = lightmap_view ? kWorldPipelineLightmapOnly : kWorldPipelineSolid;
			}

			if (mode != kWorldPipelineSkip)
			{
				if (shading_mode == kWorldShadingFlat)
				{
					mode = kWorldPipelineFlat;
				}
				else if (shading_mode == kWorldShadingNormals)
				{
					mode = kWorldPipelineNormals;
				}
			}

			if (mode == kWorldPipelineSkip)
			{
				++g_diligent_pipeline_stats.mode_skip;
				continue;
			}

			++g_diligent_pipeline_stats.total_sections;
			switch (mode)
			{
				case kWorldPipelineSolid:
					++g_diligent_pipeline_stats.mode_solid;
					break;
				case kWorldPipelineTextured:
					++g_diligent_pipeline_stats.mode_textured;
					break;
				case kWorldPipelineLightmap:
					++g_diligent_pipeline_stats.mode_lightmap;
					break;
				case kWorldPipelineLightmapOnly:
					++g_diligent_pipeline_stats.mode_lightmap_only;
					break;
				case kWorldPipelineDualTexture:
					++g_diligent_pipeline_stats.mode_dual;
					break;
				case kWorldPipelineLightmapDual:
					++g_diligent_pipeline_stats.mode_lightmap_dual;
					break;
				case kWorldPipelineBump:
					++g_diligent_pipeline_stats.mode_bump;
					break;
				case kWorldPipelineGlowTextured:
					++g_diligent_pipeline_stats.mode_glow;
					break;
				case kWorldPipelineDynamicLight:
					++g_diligent_pipeline_stats.mode_dynamic_light;
					break;
				case kWorldPipelineVolumeEffect:
					++g_diligent_pipeline_stats.mode_volume;
					break;
				case kWorldPipelineShadowProject:
					++g_diligent_pipeline_stats.mode_shadow_project;
					break;
				case kWorldPipelineFlat:
					++g_diligent_pipeline_stats.mode_flat;
					break;
				case kWorldPipelineNormals:
					++g_diligent_pipeline_stats.mode_normals;
					break;
				default:
					break;
			}

			DiligentWorldBlendMode section_blend_mode = blend_mode;
			if (is_lightmap_texture_section &&
				(mode == kWorldPipelineTextured || mode == kWorldPipelineDualTexture) &&
				g_CV_LightmapsOnly.m_Val == 0)
			{
				section_blend_mode = kWorldBlendMultiply;
			}

			auto* pipeline = diligent_get_world_pipeline(mode, section_blend_mode, depth_mode, g_CV_Wireframe.m_Val != 0);
			if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
			{
				continue;
			}

			DiligentWorldConstants constants = base_constants;
			float vertex_color_scale = 1.0f;
			if (section.light_anim)
			{
				vertex_color_scale = 0.0f;
			}
			else
			{
				if (mode == kWorldPipelineLightmap ||
					mode == kWorldPipelineLightmapOnly ||
					mode == kWorldPipelineLightmapDual)
				{
					vertex_color_scale = 0.0f;
				}
			}
			if (is_lightmap_texture_section && (mode == kWorldPipelineTextured || mode == kWorldPipelineDualTexture))
			{
				vertex_color_scale = 0.0f;
			}
			constants.world_params[0] = vertex_color_scale;
			constants.world_params[1] = g_CV_LightmapIntensity.m_Val;
			const bool has_uv1 = g_render_world && g_render_world->has_uv1;
			const bool use_tex_effect = section.texture_effect != nullptr;
			const bool allow_texel_uv = (g_CV_LightMap.m_Val == 0 && g_CV_LightmapsOnly.m_Val == 0);
			const bool use_texel_uv = allow_texel_uv && diligent_section_applies_texel_uv(*block, section, mode);
			constants.world_params[2] = (use_tex_effect || has_uv1 || use_texel_uv) ? 1.0f : 0.0f;
			diligent_apply_texture_effect_constants(*block, section, mode, constants);
			void* mapped_constants = nullptr;
			g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
			if (!mapped_constants)
			{
				continue;
			}
			std::memcpy(mapped_constants, &constants, sizeof(constants));
			g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

			if (mode == kWorldPipelineTextured || mode == kWorldPipelineGlowTextured)
			{
				auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (texture_var)
				{
					texture_var->Set(texture_view);
				}
			}
			else if (mode == kWorldPipelineLightmap)
			{
				auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (base_var)
				{
					base_var->Set(texture_view);
				}
				auto* lightmap_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (lightmap_var)
				{
					lightmap_var->Set(lightmap_view);
				}
			}
			else if (mode == kWorldPipelineLightmapOnly)
			{
				auto* lightmap_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (lightmap_var)
				{
					lightmap_var->Set(lightmap_view);
				}
			}
			else if (mode == kWorldPipelineDualTexture)
			{
				auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (base_var)
				{
					base_var->Set(texture_view);
				}
				auto* dual_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (dual_var)
				{
					dual_var->Set(dual_texture_view);
				}
			}
			else if (mode == kWorldPipelineLightmapDual)
			{
				auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (base_var)
				{
					base_var->Set(texture_view);
				}
				auto* lightmap_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (lightmap_var)
				{
					lightmap_var->Set(lightmap_view);
				}
				auto* dual_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture2");
				if (dual_var)
				{
					dual_var->Set(dual_texture_view);
				}
			}

			g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
			g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			Diligent::DrawIndexedAttribs draw_attribs;
			draw_attribs.NumIndices = section.tri_count * 3;
			draw_attribs.IndexType = Diligent::VT_UINT16;
			draw_attribs.FirstIndexLocation = section.start_index;
			const bool use_base_vertex = diligent_get_world_use_base_vertex() && block->use_base_vertex;
			draw_attribs.BaseVertex = use_base_vertex
				? static_cast<int32>(section.start_vertex)
				: 0;
			draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
			g_diligent_state.immediate_context->DrawIndexed(draw_attribs);
		}
	}

	if (section_filter == DiligentWorldSectionFilter::Normal &&
		shading_mode == kWorldShadingTextured && g_diligent_num_world_dynamic_lights > 0)
	{
		auto* light_pipeline = diligent_get_world_pipeline(kWorldPipelineDynamicLight, kWorldBlendSolid, kWorldDepthEnabled, g_CV_Wireframe.m_Val != 0);
		if (light_pipeline && light_pipeline->pipeline_state && light_pipeline->srb)
		{
			for (uint32 light_index = 0; light_index < g_diligent_num_world_dynamic_lights; ++light_index)
			{
				const DynamicLight* light = g_diligent_world_dynamic_lights[light_index];
				if (!light || light->m_LightRadius <= 0.0f)
				{
					continue;
				}

				DiligentWorldConstants light_constants = base_constants;
				light_constants.dynamic_light_pos[0] = light->m_Pos.x;
				light_constants.dynamic_light_pos[1] = light->m_Pos.y;
				light_constants.dynamic_light_pos[2] = light->m_Pos.z;
				light_constants.dynamic_light_pos[3] = light->m_LightRadius;
				light_constants.dynamic_light_color[0] = static_cast<float>(light->m_ColorR) / 255.0f;
				light_constants.dynamic_light_color[1] = static_cast<float>(light->m_ColorG) / 255.0f;
				light_constants.dynamic_light_color[2] = static_cast<float>(light->m_ColorB) / 255.0f;
				light_constants.dynamic_light_color[3] = light->m_Intensity;

				void* light_mapped_constants = nullptr;
				g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, light_mapped_constants);
				if (!light_mapped_constants)
				{
					continue;
				}
				std::memcpy(light_mapped_constants, &light_constants, sizeof(light_constants));
				g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

				for (auto* block : blocks)
				{
					if (!block || block->vertices.empty() || block->indices.empty())
					{
						continue;
					}

					if (!block->EnsureGpuBuffers())
					{
						continue;
					}

					Diligent::IBuffer* buffers[] = {block->vertex_buffer};
					Diligent::Uint64 offsets[] = {0};
					g_diligent_state.immediate_context->SetVertexBuffers(
						0,
						1,
						buffers,
						offsets,
						Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
						Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
					g_diligent_state.immediate_context->SetIndexBuffer(block->index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

					g_diligent_state.immediate_context->SetPipelineState(light_pipeline->pipeline_state);
					g_diligent_state.immediate_context->CommitShaderResources(light_pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

					for (const auto& section_ptr : block->sections)
					{
						if (!section_ptr)
						{
							continue;
						}

						auto& section = *section_ptr;
						if (section.fullbright)
						{
							continue;
						}

						bool can_light = false;
						switch (static_cast<DiligentPCShaderType>(section.shader_code))
						{
							case kPcShaderGouraud:
							case kPcShaderLightmap:
							case kPcShaderLightmapTexture:
							case kPcShaderDualTexture:
							case kPcShaderLightmapDualTexture:
								can_light = true;
								break;
							case kPcShaderSkypan:
							case kPcShaderSkyPortal:
							case kPcShaderOccluder:
							case kPcShaderNone:
							case kPcShaderUnknown:
							default:
								can_light = false;
								break;
						}

						if (!can_light)
						{
							continue;
						}

						Diligent::DrawIndexedAttribs draw_attribs;
						draw_attribs.NumIndices = section.tri_count * 3;
						draw_attribs.IndexType = Diligent::VT_UINT16;
						draw_attribs.FirstIndexLocation = section.start_index;
						const bool use_base_vertex = diligent_get_world_use_base_vertex() && block->use_base_vertex;
						draw_attribs.BaseVertex = use_base_vertex
							? static_cast<int32>(section.start_vertex)
							: 0;
						draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
						g_diligent_state.immediate_context->DrawIndexed(draw_attribs);
					}
				}
			}
		}
	}

	if (wireframe_overlay)
	{
		void* overlay_constants = nullptr;
		g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, overlay_constants);
		if (overlay_constants)
		{
			std::memcpy(overlay_constants, &base_constants, sizeof(base_constants));
			g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);
		}

		for (auto* block : blocks)
		{
			if (!block || block->vertices.empty() || block->indices.empty())
			{
				continue;
			}

			if (!block->EnsureGpuBuffers())
			{
				continue;
			}

			Diligent::IBuffer* buffers[] = {block->vertex_buffer};
			Diligent::Uint64 offsets[] = {0};
			g_diligent_state.immediate_context->SetVertexBuffers(
				0,
				1,
				buffers,
				offsets,
				Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
				Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
			g_diligent_state.immediate_context->SetIndexBuffer(block->index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			auto* pipeline = diligent_get_world_pipeline(kWorldPipelineSolid, kWorldBlendAlpha, kWorldDepthEnabled, true);
			if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
			{
				continue;
			}

			g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
			g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			for (const auto& section_ptr : block->sections)
			{
				if (!section_ptr)
				{
					continue;
				}

				auto& section = *section_ptr;
				uint8 mode = kWorldPipelineSkip;
				switch (static_cast<DiligentPCShaderType>(section.shader_code))
				{
					case kPcShaderGouraud:
					case kPcShaderLightmap:
					case kPcShaderLightmapTexture:
					case kPcShaderDualTexture:
					case kPcShaderLightmapDualTexture:
						mode = kWorldPipelineSolid;
						break;
					case kPcShaderSkypan:
					case kPcShaderSkyPortal:
					case kPcShaderOccluder:
					case kPcShaderNone:
					case kPcShaderUnknown:
					default:
						mode = kWorldPipelineSkip;
						break;
				}

				if (mode == kWorldPipelineSkip)
				{
					continue;
				}

				Diligent::DrawIndexedAttribs draw_attribs;
				draw_attribs.NumIndices = section.tri_count * 3;
				draw_attribs.IndexType = Diligent::VT_UINT16;
				draw_attribs.FirstIndexLocation = section.start_index;
				const bool use_base_vertex = diligent_get_world_use_base_vertex() && block->use_base_vertex;
				draw_attribs.BaseVertex = use_base_vertex
					? static_cast<int32>(section.start_vertex)
					: 0;
				draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
				g_diligent_state.immediate_context->DrawIndexed(draw_attribs);
			}
		}
	}

	return true;
}
bool diligent_draw_world_blocks()
{
	if (!g_CV_DrawWorld.m_Val)
	{
		return true;
	}

	if (g_visible_render_blocks.empty())
	{
		return true;
	}

	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0);
	DiligentWorldConstants constants;
	LTMatrix world_matrix;
	world_matrix.Identity();
	world_matrix.m[0][3] = g_diligent_world_offset.x;
	world_matrix.m[1][3] = g_diligent_world_offset.y;
	world_matrix.m[2][3] = g_diligent_world_offset.z;
	LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * world_matrix;
	diligent_store_matrix_from_lt(mvp, constants.mvp);
	diligent_store_matrix_from_lt(g_diligent_state.view_params.m_mView, constants.view);
	diligent_store_matrix_from_lt(world_matrix, constants.world);
	constants.camera_pos[0] = g_diligent_state.view_params.m_Pos.x;
	constants.camera_pos[1] = g_diligent_state.view_params.m_Pos.y;
	constants.camera_pos[2] = g_diligent_state.view_params.m_Pos.z;
	constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
	constants.fog_color[0] = static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f;
	constants.fog_color[1] = static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f;
	constants.fog_color[2] = static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f;
	constants.fog_color[3] = g_CV_FogNearZ.m_Val;
	constants.fog_params[0] = g_CV_FogFarZ.m_Val;
	constants.fog_params[1] = diligent_get_tonemap_enabled();
	constants.fog_params[2] = diligent_get_swapchain_output_is_srgb();
	const int tex_debug_mode = diligent_get_world_tex_debug_mode();
	if (tex_debug_mode != 0)
	{
		constants.fog_params[3] = static_cast<float>(tex_debug_mode);
	}
	else
	{
		constants.fog_params[3] = diligent_get_world_uv_debug() ? 1.0f : 0.0f;
	}
	if (g_diligent_num_world_dynamic_lights > 0)
	{
		const DynamicLight* light = g_diligent_world_dynamic_lights[0];
		constants.dynamic_light_pos[0] = light->m_Pos.x;
		constants.dynamic_light_pos[1] = light->m_Pos.y;
		constants.dynamic_light_pos[2] = light->m_Pos.z;
		constants.dynamic_light_pos[3] = light->m_LightRadius;
		constants.dynamic_light_color[0] = static_cast<float>(light->m_ColorR) / 255.0f;
		constants.dynamic_light_color[1] = static_cast<float>(light->m_ColorG) / 255.0f;
		constants.dynamic_light_color[2] = static_cast<float>(light->m_ColorB) / 255.0f;
		constants.dynamic_light_color[3] = light->m_Intensity;
	}
	constants.world_params[0] = 1.0f;
	constants.world_params[1] = g_CV_LightmapIntensity.m_Val;
	constants.world_params[2] = 0.0f;
	constants.world_params[3] = 0.0f;

	if (!diligent_draw_render_blocks_with_constants(
			g_visible_render_blocks,
			constants,
			kWorldBlendSolid,
			kWorldDepthEnabled,
			DiligentWorldSectionFilter::Normal))
	{
		return false;
	}

	if (g_CV_LightMap.m_Val != 0 && diligent_get_world_shading_mode() == kWorldShadingTextured)
	{
		const DiligentWorldBlendMode light_anim_blend =
			(g_CV_LightmapsOnly.m_Val != 0) ? kWorldBlendSolid : kWorldBlendMultiply;
		diligent_draw_render_blocks_with_constants(
			g_visible_render_blocks,
			constants,
			light_anim_blend,
			kWorldDepthEnabled,
			DiligentWorldSectionFilter::LightAnim);
	}

	return true;
}
bool diligent_draw_world_model_list_with_view(
	const std::vector<WorldModelInstance*>& models,
	const ViewParams& view_params,
	float fog_near_z,
	float fog_far_z,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode)
{
	if (models.empty())
	{
		return true;
	}

	const bool fog_enabled_base = (g_CV_FogEnable.m_Val != 0);

	for (auto* instance : models)
	{
		if (!instance)
		{
			continue;
		}

		auto* render_world = diligent_find_world_model(instance);
		if (!render_world)
		{
			continue;
		}

		std::vector<DiligentRenderBlock*> blocks;
		blocks.reserve(render_world->render_blocks.size());
		for (const auto& block : render_world->render_blocks)
		{
			if (block)
			{
				blocks.push_back(block.get());
			}
		}

		const bool fog_enabled = fog_enabled_base && ((instance->m_Flags & FLAG_FOGDISABLE) == 0);
		const DiligentWorldBlendMode instance_blend =
			blend_mode == kWorldBlendSolid
				? kWorldBlendSolid
				: ((instance->m_Flags2 & FLAG2_ADDITIVE) != 0 ? kWorldBlendAdditive : kWorldBlendAlpha);

		DiligentWorldConstants constants;
		LTMatrix world_matrix = instance->m_Transform;
		world_matrix.m[0][3] += g_diligent_world_offset.x;
		world_matrix.m[1][3] += g_diligent_world_offset.y;
		world_matrix.m[2][3] += g_diligent_world_offset.z;
		LTMatrix mvp = view_params.m_mProjection * view_params.m_mView * world_matrix;
		diligent_store_matrix_from_lt(mvp, constants.mvp);
		diligent_store_matrix_from_lt(view_params.m_mView, constants.view);
		diligent_store_matrix_from_lt(world_matrix, constants.world);
		constants.camera_pos[0] = view_params.m_Pos.x;
		constants.camera_pos[1] = view_params.m_Pos.y;
		constants.camera_pos[2] = view_params.m_Pos.z;
		constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
		constants.fog_color[0] = static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f;
		constants.fog_color[1] = static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f;
		constants.fog_color[2] = static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f;
		constants.fog_color[3] = fog_near_z;
		constants.fog_params[0] = fog_far_z;
		constants.fog_params[1] = diligent_get_tonemap_enabled();
		constants.fog_params[2] = diligent_get_swapchain_output_is_srgb();
		const int tex_debug_mode = diligent_get_world_tex_debug_mode();
		if (tex_debug_mode != 0)
		{
			constants.fog_params[3] = static_cast<float>(tex_debug_mode);
		}
		else
		{
			constants.fog_params[3] = diligent_get_world_uv_debug() ? 1.0f : 0.0f;
		}
		if (g_diligent_num_world_dynamic_lights > 0)
		{
			const DynamicLight* light = g_diligent_world_dynamic_lights[0];
			constants.dynamic_light_pos[0] = light->m_Pos.x;
			constants.dynamic_light_pos[1] = light->m_Pos.y;
			constants.dynamic_light_pos[2] = light->m_Pos.z;
			constants.dynamic_light_pos[3] = light->m_LightRadius;
			constants.dynamic_light_color[0] = static_cast<float>(light->m_ColorR) / 255.0f;
			constants.dynamic_light_color[1] = static_cast<float>(light->m_ColorG) / 255.0f;
			constants.dynamic_light_color[2] = static_cast<float>(light->m_ColorB) / 255.0f;
			constants.dynamic_light_color[3] = light->m_Intensity;
		}
		constants.world_params[0] = 1.0f;
		constants.world_params[1] = g_CV_LightmapIntensity.m_Val;
		constants.world_params[2] = 0.0f;
		constants.world_params[3] = 0.0f;

		if (!diligent_draw_render_blocks_with_constants(
				blocks,
				constants,
				instance_blend,
				depth_mode,
				DiligentWorldSectionFilter::Normal))
		{
			return false;
		}
	}

	return true;
}
bool diligent_draw_world_model_list(const std::vector<WorldModelInstance*>& models, DiligentWorldBlendMode blend_mode)
{
	const std::vector<WorldModelInstance*>* draw_list = &models;
	std::vector<WorldModelInstance*> sorted_models;
	if (blend_mode == kWorldBlendAlpha && g_CV_DrawSorted.m_Val && models.size() > 1)
	{
		sorted_models = models;
		diligent_sort_translucent_world_models(sorted_models, g_diligent_state.view_params);
		draw_list = &sorted_models;
	}

	return diligent_draw_world_model_list_with_view(
		*draw_list,
		g_diligent_state.view_params,
		g_CV_FogNearZ.m_Val,
		g_CV_FogFarZ.m_Val,
		blend_mode,
		kWorldDepthEnabled);
}

bool diligent_draw_world_glow(const DiligentWorldConstants& constants, const std::vector<DiligentRenderBlock*>& blocks)
{
	if (blocks.empty())
	{
		return true;
	}

	if (!g_diligent_state.immediate_context)
	{
		return false;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	for (auto* block : blocks)
	{
		if (!block || block->vertices.empty() || block->indices.empty())
		{
			continue;
		}

		if (!block->EnsureGpuBuffers())
		{
			continue;
		}

		Diligent::IBuffer* buffers[] = {block->vertex_buffer};
		Diligent::Uint64 offsets[] = {0};
		g_diligent_state.immediate_context->SetVertexBuffers(
			0,
			1,
			buffers,
			offsets,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
		g_diligent_state.immediate_context->SetIndexBuffer(block->index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		for (const auto& section_ptr : block->sections)
		{
			if (!section_ptr)
			{
				continue;
			}

			auto& section = *section_ptr;
			if (!section.fullbright)
			{
				continue;
			}

			Diligent::ITextureView* texture_view = nullptr;
			if (section.textures[0])
			{
				texture_view = diligent_get_texture_view(section.textures[0], false);
			}

			if (!texture_view)
			{
				continue;
			}

			const uint8 mode = kWorldPipelineGlowTextured;
			auto* pipeline = diligent_get_world_pipeline(mode, kWorldBlendSolid, kWorldDepthEnabled, g_CV_Wireframe.m_Val != 0);
			if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
			{
				continue;
			}

			if (mode == kWorldPipelineGlowTextured)
			{
				auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (texture_var)
				{
					texture_var->Set(texture_view);
				}
			}

			DiligentWorldConstants section_constants = constants;
			const bool allow_texel_uv = (g_CV_LightMap.m_Val == 0 && g_CV_LightmapsOnly.m_Val == 0);
			const bool use_texel_uv = allow_texel_uv && diligent_section_applies_texel_uv(*block, section, mode);
			section_constants.world_params[2] = use_texel_uv ? 1.0f : 0.0f;
			diligent_apply_texture_effect_constants(*block, section, mode, section_constants);
			void* mapped_constants = nullptr;
			g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
			if (!mapped_constants)
			{
				continue;
			}
			std::memcpy(mapped_constants, &section_constants, sizeof(section_constants));
			g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

			g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
			g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			Diligent::DrawIndexedAttribs draw_attribs;
			draw_attribs.NumIndices = section.tri_count * 3;
			draw_attribs.IndexType = Diligent::VT_UINT16;
			draw_attribs.FirstIndexLocation = section.start_index;
			const bool use_base_vertex = diligent_get_world_use_base_vertex() && block->use_base_vertex;
			draw_attribs.BaseVertex = use_base_vertex
				? static_cast<int32>(section.start_vertex)
				: 0;
			draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
			g_diligent_state.immediate_context->DrawIndexed(draw_attribs);
		}
	}

	return true;
}

bool diligent_draw_world_models_glow(const std::vector<WorldModelInstance*>& models, const DiligentWorldConstants& base_constants)
{
	if (models.empty())
	{
		return true;
	}

	for (auto* instance : models)
	{
		if (!instance)
		{
			continue;
		}

		auto* render_world = diligent_find_world_model(instance);
		if (!render_world)
		{
			continue;
		}

		std::vector<DiligentRenderBlock*> blocks;
		blocks.reserve(render_world->render_blocks.size());
		for (const auto& block : render_world->render_blocks)
		{
			if (block)
			{
				blocks.push_back(block.get());
			}
		}

		DiligentWorldConstants constants = base_constants;
		LTMatrix world_matrix = instance->m_Transform;
		world_matrix.m[0][3] += g_diligent_world_offset.x;
		world_matrix.m[1][3] += g_diligent_world_offset.y;
		world_matrix.m[2][3] += g_diligent_world_offset.z;
		LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * world_matrix;
		diligent_store_matrix_from_lt(mvp, constants.mvp);
		diligent_store_matrix_from_lt(world_matrix, constants.world);

		if (!diligent_draw_world_glow(constants, blocks))
		{
			return false;
		}
	}

	return true;
}
bool diligent_update_world_constants_buffer(const DiligentWorldConstants& constants)
{
	if (!g_diligent_state.immediate_context || !g_world_resources.constant_buffer)
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);
	return true;
}
bool diligent_GetWorldColorStats(DiligentWorldColorStats& out_stats)
{
	out_stats = {};
	out_stats.min_r = 255;
	out_stats.min_g = 255;
	out_stats.min_b = 255;
	out_stats.min_a = 255;

	if (!g_render_world)
	{
		return false;
	}

	uint64_t total = 0;
	uint64_t zero = 0;

	for (const auto& block_ptr : g_render_world->render_blocks)
	{
		if (!block_ptr)
		{
			continue;
		}
		const auto& verts = block_ptr->vertices;
		for (const auto& vert : verts)
		{
			++total;
			const uint32 color = vert.color;
			if (color == 0)
			{
				++zero;
			}
			const uint32 a = (color >> 24) & 0xFF;
			const uint32 r = (color >> 16) & 0xFF;
			const uint32 g = (color >> 8) & 0xFF;
			const uint32 b = color & 0xFF;
			out_stats.min_r = LTMIN(out_stats.min_r, r);
			out_stats.min_g = LTMIN(out_stats.min_g, g);
			out_stats.min_b = LTMIN(out_stats.min_b, b);
			out_stats.min_a = LTMIN(out_stats.min_a, a);
			out_stats.max_r = LTMAX(out_stats.max_r, r);
			out_stats.max_g = LTMAX(out_stats.max_g, g);
			out_stats.max_b = LTMAX(out_stats.max_b, b);
			out_stats.max_a = LTMAX(out_stats.max_a, a);
		}
	}

	out_stats.vertex_count = total;
	out_stats.zero_color_count = zero;
	return total > 0;
}
bool diligent_GetWorldTextureStats(DiligentWorldTextureStats& out_stats)
{
	out_stats = {};

	if (!g_render_world)
	{
		return false;
	}

	uint64_t sections = 0;
	for (const auto& block_ptr : g_render_world->render_blocks)
	{
		if (!block_ptr)
		{
			continue;
		}

		for (const auto& section_ptr : block_ptr->sections)
		{
			if (!section_ptr)
			{
				continue;
			}

			++sections;
			const auto& section = *section_ptr;
			if (section.textures[0])
			{
				++out_stats.texture0_present;
				if (diligent_get_texture_view(section.textures[0], false))
				{
					++out_stats.texture0_view;
				}
			}
			if (section.textures[1])
			{
				++out_stats.texture1_present;
				if (diligent_get_texture_view(section.textures[1], false))
				{
					++out_stats.texture1_view;
				}
			}
			if (section.lightmap_size != 0 && section.lightmap_width != 0 && section.lightmap_height != 0)
			{
				++out_stats.lightmap_present;
				if (diligent_get_lightmap_view(const_cast<DiligentRBSection&>(section)))
				{
					++out_stats.lightmap_view;
				}
			}
		}
	}

	out_stats.section_count = sections;
	return sections > 0;
}
void diligent_InvalidateWorldGeometry()
{
	if (!g_render_world)
	{
		return;
	}

	for (const auto& block_ptr : g_render_world->render_blocks)
	{
		if (!block_ptr)
		{
			continue;
		}
		block_ptr->vertex_buffer.Release();
		block_ptr->index_buffer.Release();
	}
}
void diligent_DumpWorldTextureBindings(uint32_t limit)
{
	if (!g_render_world)
	{
		dsi_ConsolePrint("World textures unavailable.");
		return;
	}

	uint32_t dumped = 0;
	for (const auto& block_ptr : g_render_world->render_blocks)
	{
		if (!block_ptr)
		{
			continue;
		}
		for (const auto& section_ptr : block_ptr->sections)
		{
			if (!section_ptr || !section_ptr->textures[0])
			{
				continue;
			}

			DiligentTextureDebugInfo info;
			if (!diligent_GetTextureDebugInfo(section_ptr->textures[0], info))
			{
				continue;
			}

			const char* name = nullptr;
			if (g_diligent_state.render_struct && g_diligent_state.render_struct->GetTextureName)
			{
				name = g_diligent_state.render_struct->GetTextureName(section_ptr->textures[0]);
			}
			const char* bsp_name = nullptr;
			const char* bsp_texname = nullptr;
			uint32 bsp_world_index = 0;
			uint32 bsp_poly_count = 0;
			uint32 bsp_surface_count = 0;
			uint32 bsp_surface_index = 0xFFFFFFFFu;
			uint16 bsp_tex_index = 0xFFFFu;
			auto* bsp_client = diligent_get_world_bsp_client();
			if (bsp_client && bsp_client->NumWorldModels() == 0)
			{
				bsp_client = nullptr;
			}
			if (bsp_client)
			{
				const uint32 model_count = bsp_client->NumWorldModels();
				const WorldData* world_model = nullptr;
				if (block_ptr->world)
				{
					bsp_world_index = block_ptr->world->ResolveWorldIndex();
				}
				if (bsp_world_index < model_count)
				{
					world_model = bsp_client->GetWorldModel(bsp_world_index);
				}
				const WorldBsp* bsp = world_model ? world_model->OriginalBSP() : nullptr;
				if (!bsp && model_count > 0)
				{
					world_model = bsp_client->GetWorldModel(0);
					bsp = world_model ? world_model->OriginalBSP() : nullptr;
					bsp_world_index = 0;
				}

				if (bsp)
				{
					bsp_poly_count = bsp->m_nPolies;
					bsp_surface_count = bsp->m_nSurfaces;
					if (section_ptr->poly_index < bsp->m_nPolies && bsp->m_Polies)
					{
						WorldPoly* poly = bsp->m_Polies[section_ptr->poly_index];
						if (poly && poly->GetSurface())
						{
							Surface* surface = poly->GetSurface();
							bsp_surface_index = static_cast<uint32>(surface - bsp->m_Surfaces);
							bsp_tex_index = surface->m_iTexture;
							if (g_diligent_state.render_struct)
							{
								bsp_name = g_diligent_state.render_struct->GetTextureName(surface->GetSharedTexture());
							}
						}
					}
					else if (section_ptr->poly_index < bsp->m_nSurfaces && bsp->m_Surfaces)
					{
						Surface* surface = &bsp->m_Surfaces[section_ptr->poly_index];
						bsp_surface_index = section_ptr->poly_index;
						bsp_tex_index = surface->m_iTexture;
						if (g_diligent_state.render_struct)
						{
							bsp_name = g_diligent_state.render_struct->GetTextureName(surface->GetSharedTexture());
						}
					}

					if (bsp->m_TextureNames && bsp_tex_index != 0xFFFFu)
					{
						bsp_texname = bsp->m_TextureNames[bsp_tex_index];
					}
				}
			}
			else
			{
				const WorldBsp* bsp = diligent_pick_external_bsp(*block_ptr, section_ptr->poly_index, bsp_world_index);
				if (bsp)
				{
					bsp_poly_count = bsp->m_nPolies;
					bsp_surface_count = bsp->m_nSurfaces;
					if (section_ptr->poly_index < bsp->m_nPolies && bsp->m_Polies)
					{
						WorldPoly* poly = bsp->m_Polies[section_ptr->poly_index];
						if (poly && poly->GetSurface())
						{
							Surface* surface = poly->GetSurface();
							bsp_surface_index = static_cast<uint32>(surface - bsp->m_Surfaces);
							bsp_tex_index = surface->m_iTexture;
							if (g_diligent_state.render_struct)
							{
								bsp_name = g_diligent_state.render_struct->GetTextureName(surface->GetSharedTexture());
							}
						}
					}
					else if (section_ptr->poly_index < bsp->m_nSurfaces && bsp->m_Surfaces)
					{
						Surface* surface = &bsp->m_Surfaces[section_ptr->poly_index];
						bsp_surface_index = section_ptr->poly_index;
						bsp_tex_index = surface->m_iTexture;
						if (g_diligent_state.render_struct)
						{
							bsp_name = g_diligent_state.render_struct->GetTextureName(surface->GetSharedTexture());
						}
					}

					if (bsp->m_TextureNames && bsp_tex_index != 0xFFFFu)
					{
						bsp_texname = bsp->m_TextureNames[bsp_tex_index];
					}
				}
			}

			dsi_ConsolePrint(
				"World tex[%u]: %s size=%ux%u gpu=%d bsp=%s tex=%s wi=%u poly=%u "
				"surfaces=%u polies=%u surf=%u texi=%u",
				dumped,
				name ? name : "(unknown)",
				info.width,
				info.height,
				info.has_render_texture ? 1 : 0,
				bsp_name ? bsp_name : "(none)",
				bsp_texname ? bsp_texname : "(none)",
				bsp_world_index,
				section_ptr->poly_index,
				bsp_surface_count,
				bsp_poly_count,
				bsp_surface_index,
				bsp_tex_index);

			++dumped;
			if (limit != 0 && dumped >= limit)
			{
				return;
			}
		}
	}

	if (dumped == 0)
	{
		dsi_ConsolePrint("World textures: none bound.");
	}
}
void diligent_DumpWorldSurfaceDebug(uint32_t limit)
{
	if (!g_render_world)
	{
		dsi_ConsolePrint("World debug dump unavailable.");
		return;
	}

	const char* home_dir = std::getenv("HOME");
	std::string output_path = home_dir ? (std::string(home_dir) + "/world_surface_debug.txt") : "world_surface_debug.txt";
	std::ofstream output(output_path, std::ios::trunc);
	if (!output.is_open())
	{
		dsi_ConsolePrint("World debug dump: failed to open %s", output_path.c_str());
		return;
	}

	output << "World surface debug dump\n";
	uint32_t dumped = 0;
	uint32_t block_index = 0;
	for (const auto& block_ptr : g_render_world->render_blocks)
	{
		if (!block_ptr)
		{
			++block_index;
			continue;
		}

		const auto& block = *block_ptr;
		uint32_t section_index = 0;
		for (const auto& section_ptr : block.sections)
		{
			if (!section_ptr)
			{
				++section_index;
				continue;
			}

			if (limit != 0 && dumped >= limit)
			{
				break;
			}

			const auto& section = *section_ptr;
			output << "\n[section " << dumped << "] block=" << block_index << " section=" << section_index << "\n";
			output << " poly=" << section.poly_index
				   << " start_vertex=" << section.start_vertex
				   << " vertex_count=" << section.vertex_count
				   << " start_index=" << section.start_index
				   << " tri_count=" << section.tri_count << "\n";

			const char* base_name = nullptr;
			if (section.textures[0] && g_diligent_state.render_struct && g_diligent_state.render_struct->GetTextureName)
			{
				base_name = g_diligent_state.render_struct->GetTextureName(section.textures[0]);
			}
			const char* dual_name = nullptr;
			if (section.textures[1] && g_diligent_state.render_struct && g_diligent_state.render_struct->GetTextureName)
			{
				dual_name = g_diligent_state.render_struct->GetTextureName(section.textures[1]);
			}

			DiligentTextureDebugInfo base_info{};
			const bool base_info_valid = section.textures[0] && diligent_GetTextureDebugInfo(section.textures[0], base_info);

			output << " base_texture=" << (base_name ? base_name : "(none)")
				   << " size=" << (base_info_valid ? base_info.width : 0) << "x"
				   << (base_info_valid ? base_info.height : 0)
				   << " gpu=" << (base_info_valid && base_info.has_render_texture ? 1 : 0) << "\n";
			if (section.textures[1])
			{
				DiligentTextureDebugInfo dual_info{};
				const bool dual_valid = diligent_GetTextureDebugInfo(section.textures[1], dual_info);
				output << " dual_texture=" << (dual_name ? dual_name : "(none)")
					   << " size=" << (dual_valid ? dual_info.width : 0) << "x"
					   << (dual_valid ? dual_info.height : 0)
					   << " gpu=" << (dual_valid && dual_info.has_render_texture ? 1 : 0) << "\n";
			}

			bool lightmap_view = section.lightmap_srv != nullptr;
			if (!lightmap_view && section.lightmap_size != 0 && section.lightmap_width != 0 && section.lightmap_height != 0)
			{
				lightmap_view = diligent_get_lightmap_view(const_cast<DiligentRBSection&>(section)) != nullptr;
			}
			output << " lightmap size=" << section.lightmap_width << "x" << section.lightmap_height
				   << " bytes=" << section.lightmap_size
				   << " srv=" << (lightmap_view ? 1 : 0) << "\n";
			output << " light_anim=" << (section.light_anim ? 1 : 0) << " fullbright=" << (section.fullbright ? 1 : 0)
				   << " effect=" << (section.texture_effect ? 1 : 0) << "\n";
			if (section.texture_effect)
			{
				TextureScriptStageData stage_data{};
				if (section.texture_effect->GetStageData(TSChannel_Base, stage_data))
				{
					output << " effect base input=" << static_cast<int>(stage_data.input_type)
						   << " flags=0x" << std::hex << stage_data.flags << std::dec << "\n";
				}
				if (section.texture_effect->GetStageData(TSChannel_LightMap, stage_data))
				{
					output << " effect lightmap input=" << static_cast<int>(stage_data.input_type)
						   << " flags=0x" << std::hex << stage_data.flags << std::dec << "\n";
				}
			}

			const WorldBsp* bsp = nullptr;
			uint32 bsp_world_index = 0;
			const char* bsp_texname = nullptr;
			auto* bsp_client = diligent_get_world_bsp_client();
			if (bsp_client && bsp_client->NumWorldModels() == 0)
			{
				bsp_client = nullptr;
			}
			if (bsp_client)
			{
				const uint32 model_count = bsp_client->NumWorldModels();
				const WorldData* world_model = nullptr;
				if (block.world)
				{
					bsp_world_index = block.world->ResolveWorldIndex();
				}
				if (bsp_world_index < model_count)
				{
					world_model = bsp_client->GetWorldModel(bsp_world_index);
				}
				bsp = world_model ? world_model->OriginalBSP() : nullptr;
				if (!bsp && model_count > 0)
				{
					world_model = bsp_client->GetWorldModel(0);
					bsp = world_model ? world_model->OriginalBSP() : nullptr;
					bsp_world_index = 0;
				}
			}
			else
			{
				bsp = diligent_pick_external_bsp(block, section.poly_index, bsp_world_index);
			}

			if (bsp && bsp->m_TextureNames && section.poly_index < bsp->m_nPolies && bsp->m_Polies)
			{
				WorldPoly* poly = bsp->m_Polies[section.poly_index];
				if (poly && poly->GetSurface())
				{
					Surface* surface = poly->GetSurface();
					if (surface && surface->m_iTexture != 0xFFFFu)
					{
						bsp_texname = bsp->m_TextureNames[surface->m_iTexture];
					}
				}
			}
			output << " bsp_world=" << bsp_world_index << " bsp_tex=" << (bsp_texname ? bsp_texname : "(none)") << "\n";

			const size_t total_vertices = block.vertices.size();
			const uint32 start = section.start_vertex;
			const uint32 count = section.vertex_count;
			if (start + count <= total_vertices && count > 0)
			{
				bool has_range = false;
				float min_u0 = 0.0f;
				float min_v0 = 0.0f;
				float max_u0 = 0.0f;
				float max_v0 = 0.0f;
				float min_u1 = 0.0f;
				float min_v1 = 0.0f;
				float max_u1 = 0.0f;
				float max_v1 = 0.0f;
				uint64_t uv1_nonzero = 0;
				uint32 min_r = 255;
				uint32 min_g = 255;
				uint32 min_b = 255;
				uint32 min_a = 255;
				uint32 max_r = 0;
				uint32 max_g = 0;
				uint32 max_b = 0;
				uint32 max_a = 0;

				for (uint32 i = 0; i < count; ++i)
				{
					const auto& vert = block.vertices[start + i];
					if (!has_range)
					{
						min_u0 = max_u0 = vert.u0;
						min_v0 = max_v0 = vert.v0;
						min_u1 = max_u1 = vert.u1;
						min_v1 = max_v1 = vert.v1;
						has_range = true;
					}
					else
					{
						min_u0 = std::min(min_u0, vert.u0);
						min_v0 = std::min(min_v0, vert.v0);
						max_u0 = std::max(max_u0, vert.u0);
						max_v0 = std::max(max_v0, vert.v0);
						min_u1 = std::min(min_u1, vert.u1);
						min_v1 = std::min(min_v1, vert.v1);
						max_u1 = std::max(max_u1, vert.u1);
						max_v1 = std::max(max_v1, vert.v1);
					}
					if (vert.u1 != 0.0f || vert.v1 != 0.0f)
					{
						++uv1_nonzero;
					}
					const uint32 color = vert.color;
					const uint32 a = (color >> 24) & 0xFF;
					const uint32 r = (color >> 16) & 0xFF;
					const uint32 g = (color >> 8) & 0xFF;
					const uint32 b = color & 0xFF;
					min_r = LTMIN(min_r, r);
					min_g = LTMIN(min_g, g);
					min_b = LTMIN(min_b, b);
					min_a = LTMIN(min_a, a);
					max_r = LTMAX(max_r, r);
					max_g = LTMAX(max_g, g);
					max_b = LTMAX(max_b, b);
					max_a = LTMAX(max_a, a);
				}

				output << " uv0 min=(" << min_u0 << "," << min_v0 << ") max=(" << max_u0 << "," << max_v0 << ")\n";
				output << " uv1 min=(" << min_u1 << "," << min_v1 << ") max=(" << max_u1 << "," << max_v1
					   << ") nonzero=" << uv1_nonzero << "/" << count << "\n";
				output << " color min=(" << min_r << "," << min_g << "," << min_b << "," << min_a << ") max=("
					   << max_r << "," << max_g << "," << max_b << "," << max_a << ")\n";

				const auto& vert0 = block.vertices[start];
				output << " sample v0 uv0=(" << vert0.u0 << "," << vert0.v0 << ") uv1=(" << vert0.u1 << "," << vert0.v1
					   << ") color=0x" << std::hex << vert0.color << std::dec << "\n";
			}
			else
			{
				output << " vertex_range invalid (start=" << start << " count=" << count << " total=" << total_vertices
					   << ")\n";
			}

			++dumped;
			++section_index;
		}

		if (limit != 0 && dumped >= limit)
		{
			break;
		}

		++block_index;
	}

	output.close();
	dsi_ConsolePrint("World surface dump: %u section(s) -> %s", dumped, output_path.c_str());
}
void diligent_ResetWorldShaders()
{
	g_world_pipelines.pipelines.clear();
	g_world_resources.vertex_shader.Release();
	g_world_resources.pixel_shader_textured.Release();
	g_world_resources.pixel_shader_glow.Release();
	g_world_resources.pixel_shader_lightmap.Release();
	g_world_resources.pixel_shader_lightmap_only.Release();
	g_world_resources.pixel_shader_dual.Release();
	g_world_resources.pixel_shader_lightmap_dual.Release();
	g_world_resources.pixel_shader_solid.Release();
	g_world_resources.pixel_shader_flat.Release();
	g_world_resources.pixel_shader_normals.Release();
	g_world_resources.pixel_shader_dynamic_light.Release();
	g_world_resources.pixel_shader_bump.Release();
	g_world_resources.pixel_shader_volume_effect.Release();
	g_world_resources.pixel_shader_shadow_project.Release();
	g_world_resources.pixel_shader_debug.Release();
}
bool diligent_GetWorldUvStats(DiligentWorldUvStats& out_stats)
{
	out_stats = {};

	if (!g_render_world)
	{
		return false;
	}

	bool has_range = false;
	float min_u0 = 0.0f;
	float min_v0 = 0.0f;
	float max_u0 = 0.0f;
	float max_v0 = 0.0f;
	float min_u1 = 0.0f;
	float min_v1 = 0.0f;
	float max_u1 = 0.0f;
	float max_v1 = 0.0f;

	uint64_t count = 0;
	uint64_t nan0 = 0;
	uint64_t nan1 = 0;

	for (const auto& block_ptr : g_render_world->render_blocks)
	{
		if (!block_ptr)
		{
			continue;
		}
		const auto& verts = block_ptr->vertices;
		for (const auto& vert : verts)
		{
			++count;
			const float u0 = vert.u0;
			const float v0 = vert.v0;
			const float u1 = vert.u1;
			const float v1 = vert.v1;

			if (std::isnan(u0) || std::isnan(v0))
			{
				++nan0;
			}
			if (std::isnan(u1) || std::isnan(v1))
			{
				++nan1;
			}

			if (!has_range)
			{
				min_u0 = max_u0 = u0;
				min_v0 = max_v0 = v0;
				min_u1 = max_u1 = u1;
				min_v1 = max_v1 = v1;
				has_range = true;
			}
			else
			{
				min_u0 = LTMIN(min_u0, u0);
				min_v0 = LTMIN(min_v0, v0);
				max_u0 = LTMAX(max_u0, u0);
				max_v0 = LTMAX(max_v0, v0);
				min_u1 = LTMIN(min_u1, u1);
				min_v1 = LTMIN(min_v1, v1);
				max_u1 = LTMAX(max_u1, u1);
				max_v1 = LTMAX(max_v1, v1);
			}
		}
	}

	if (!has_range)
	{
		return false;
	}

	out_stats.vertex_count = count;
	out_stats.nan_uv0 = nan0;
	out_stats.nan_uv1 = nan1;
	out_stats.min_u0 = min_u0;
	out_stats.min_v0 = min_v0;
	out_stats.max_u0 = max_u0;
	out_stats.max_v0 = max_v0;
	out_stats.min_u1 = min_u1;
	out_stats.min_v1 = min_v1;
	out_stats.max_u1 = max_u1;
	out_stats.max_v1 = max_v1;
	out_stats.has_range = true;
	return true;
}
void diligent_ResetWorldPipelineStats()
{
	g_diligent_pipeline_stats = {};
}
bool diligent_GetWorldPipelineStats(DiligentWorldPipelineStats& out_stats)
{
	out_stats = g_diligent_pipeline_stats;
	return true;
}
void diligent_SetWorldOffset(const LTVector& offset)
{
	g_diligent_world_offset = offset;
}
const LTVector& diligent_GetWorldOffset()
{
	return g_diligent_world_offset;
}
