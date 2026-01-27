#include "diligent_postfx.h"

#include "diligent_2d_draw.h"
#include "diligent_device.h"
#include "diligent_internal.h"
#include "diligent_mesh_layout.h"
#include "diligent_model_draw.h"
#include "diligent_render.h"
#include "diligent_utils.h"
#include "diligent_texture_cache.h"
#include "diligent_world_data.h"
#include "diligent_world_draw.h"
#include "diligent_scene_collect.h"
#include "texturescriptinstance.h"
#include "texturescriptvarmgr.h"

#include "bdefs.h"
#include "de_objects.h"
#include "de_world.h"
#include "ltmatrix.h"
#include "ltrenderstyle.h"
#include "ltvector.h"
#include "model.h"
#include "renderstruct.h"
#include "viewparams.h"
#include "world_client_bsp.h"
#include "world_tree.h"

#include "../rendererconsolevars.h"

#include "diligent_shaders_generated.h"

#include "PostProcess/Common/interface/PostFXContext.hpp"
#include "PostProcess/ScreenSpaceAmbientOcclusion/interface/ScreenSpaceAmbientOcclusion.hpp"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Diligent
{
namespace HLSL
{
#include "Shaders/Common/public/ShaderDefinitions.fxh"
#include "Shaders/Common/public/BasicStructures.fxh"
#include "Shaders/PostProcess/ScreenSpaceAmbientOcclusion/public/ScreenSpaceAmbientOcclusionStructures.fxh"
} // namespace HLSL
} // namespace Diligent

namespace
{
constexpr Diligent::TEXTURE_FORMAT kSsaoFxNormalFormat = Diligent::TEX_FORMAT_RGBA16_FLOAT;
constexpr Diligent::TEXTURE_FORMAT kSsaoFxMotionFormat = Diligent::TEX_FORMAT_RG16_FLOAT;
constexpr Diligent::TEXTURE_FORMAT kSsaoFxRoughnessFormat = Diligent::TEX_FORMAT_R8_UNORM;
constexpr Diligent::TEXTURE_FORMAT kSsaoFxDepthFormat = Diligent::TEX_FORMAT_D32_FLOAT;
constexpr uint32 kRoughnessSourceVarIndex = TSVAR_USER;
constexpr uint32 kRoughnessValueVarIndex = TSVAR_USER + 1;
constexpr uint32 kRoughnessOverrideVarIndex = TSVAR_USER + 2;

struct DiligentSsaoPrepassConstants
{
	std::array<float, 16> view_proj{};
	std::array<float, 16> prev_view_proj{};
	std::array<float, 16> world{};
	std::array<float, 16> prev_world{};
	float prepass_params[4] = {0.5f, 0.0f, 0.0f, 0.0f};
};

bool diligent_section_get_roughness_override(
	const DiligentRBSection& section,
	float& out_roughness,
	bool& out_use_alpha)
{
	out_roughness = std::clamp(g_CV_SSRDefaultRoughness.m_Val, 0.0f, 1.0f);
	out_use_alpha = false;

	if (!section.texture_effect)
	{
		return false;
	}

	uint32 var_id = 0;
	if (!section.texture_effect->GetStageVarID(TSChannel_Base, var_id) || var_id == 0)
	{
		return false;
	}

	float* vars = CTextureScriptVarMgr::GetSingleton().GetVars(var_id);
	if (!vars)
	{
		return false;
	}

	if (vars[kRoughnessSourceVarIndex] > 0.5f)
	{
		out_use_alpha = true;
		return true;
	}

	if (vars[kRoughnessOverrideVarIndex] > 0.5f)
	{
		out_roughness = std::clamp(vars[kRoughnessValueVarIndex], 0.0f, 1.0f);
		return true;
	}

	return false;
}

bool diligent_render_style_uses_alpha_roughness(CRenderStyle* render_style)
{
	if (!render_style)
	{
		return false;
	}

	LightingMaterial material{};
	if (!render_style->GetLightingMaterial(&material))
	{
		return false;
	}

	return material.Specular.a > 0.5f;
}

struct DiligentSsaoCompositeConstants
{
	float intensity = 1.0f;
	float pad0 = 0.0f;
	float pad1[2] = {0.0f, 0.0f};
};

struct DiligentSsaoFxTargets
{
	uint32 width = 0;
	uint32 height = 0;
	Diligent::TEXTURE_FORMAT color_format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::RefCntAutoPtr<Diligent::ITexture> scene_color;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> scene_color_srv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> scene_color_rtv;
	Diligent::RefCntAutoPtr<Diligent::ITexture> normal;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> normal_rtv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> normal_srv;
	Diligent::RefCntAutoPtr<Diligent::ITexture> motion;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> motion_rtv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> motion_srv;
	Diligent::RefCntAutoPtr<Diligent::ITexture> roughness;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> roughness_rtv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> roughness_srv;
	Diligent::RefCntAutoPtr<Diligent::ITexture> depth;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> depth_dsv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> depth_srv;
	Diligent::RefCntAutoPtr<Diligent::ITexture> depth_history;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> depth_history_srv;
};

struct DiligentSsaoFxPrepassPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	Diligent::TEXTURE_FORMAT normal_format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::TEXTURE_FORMAT motion_format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::TEXTURE_FORMAT roughness_format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::TEXTURE_FORMAT depth_format = Diligent::TEX_FORMAT_UNKNOWN;
};

struct DiligentSsaoFxCompositePipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	Diligent::TEXTURE_FORMAT rtv_format = Diligent::TEX_FORMAT_UNKNOWN;
};

struct DiligentSsaoFxModelPipelineKey
{
	uint32 layout_hash = 0;
	Diligent::TEXTURE_FORMAT normal_format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::TEXTURE_FORMAT motion_format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::TEXTURE_FORMAT roughness_format = Diligent::TEX_FORMAT_UNKNOWN;
	Diligent::TEXTURE_FORMAT depth_format = Diligent::TEX_FORMAT_UNKNOWN;

	bool operator==(const DiligentSsaoFxModelPipelineKey& other) const
	{
		return layout_hash == other.layout_hash &&
			normal_format == other.normal_format &&
			motion_format == other.motion_format &&
			roughness_format == other.roughness_format &&
			depth_format == other.depth_format;
	}
};

struct DiligentSsaoFxModelPipelineKeyHash
{
	size_t operator()(const DiligentSsaoFxModelPipelineKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, key.layout_hash);
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.normal_format));
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.motion_format));
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.roughness_format));
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.depth_format));
		return static_cast<size_t>(hash);
	}
};

struct DiligentSsaoFxModelPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct DiligentSsaoFxPrevTransform
{
	LTMatrix prev{};
	LTMatrix curr{};
	uint32 last_frame = 0;
	bool valid = false;
};

struct DiligentSsaoFxState
{
	std::unique_ptr<Diligent::PostFXContext> post_fx;
	std::unique_ptr<Diligent::ScreenSpaceAmbientOcclusion> ssao;
	Diligent::RefCntAutoPtr<Diligent::IShader> prepass_ps;
	Diligent::RefCntAutoPtr<Diligent::IShader> world_prepass_vs;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> prepass_constants;
	DiligentSsaoFxPrepassPipeline world_pipeline;
	std::unordered_map<DiligentSsaoFxModelPipelineKey, DiligentSsaoFxModelPipeline, DiligentSsaoFxModelPipelineKeyHash> model_pipelines;
	std::unordered_map<uint32, Diligent::RefCntAutoPtr<Diligent::IShader>> model_vs;
	Diligent::RefCntAutoPtr<Diligent::IShader> composite_vs;
	Diligent::RefCntAutoPtr<Diligent::IShader> composite_ps;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> composite_constants;
	DiligentSsaoFxCompositePipeline composite_pipeline;
	Diligent::RefCntAutoPtr<Diligent::IShader> copy_vs;
	Diligent::RefCntAutoPtr<Diligent::IShader> copy_ps;
	DiligentSsaoFxCompositePipeline copy_pipeline;
	DiligentSsaoFxTargets targets;
	LTMatrix current_view_proj;
	LTMatrix prev_view_proj;
	bool has_prev_view_proj = false;
	Diligent::HLSL::CameraAttribs curr_camera{};
	Diligent::HLSL::CameraAttribs prev_camera{};
	bool has_prev_camera = false;
	bool history_valid = false;
	std::unordered_map<const ModelInstance*, DiligentSsaoFxPrevTransform> model_prev;
	std::unordered_map<const WorldModelInstance*, DiligentSsaoFxPrevTransform> world_prev;
};

DiligentSsaoFxState g_ssao_fx_state;

bool diligent_ssao_fx_get_model_transform_raw(ModelInstance* instance, uint32 bone_index, LTMatrix& transform)
{
	if (!instance)
	{
		transform.Identity();
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model || bone_index >= model->NumNodes())
	{
		transform.Identity();
		return false;
	}

	if (!instance->GetCachedTransform(bone_index, transform))
	{
		transform.Identity();
		return false;
	}

	return true;
}

template <typename T>
LTMatrix diligent_ssao_fx_get_prev_transform(
	const T* key,
	const LTMatrix& current,
	std::unordered_map<const T*, DiligentSsaoFxPrevTransform>& map)
{
	if (!key)
	{
		return current;
	}

	auto& entry = map[key];
	if (!entry.valid)
	{
		entry.prev = current;
		entry.curr = current;
		entry.last_frame = g_diligent_state.frame_counter;
		entry.valid = true;
		return current;
	}

	if (entry.last_frame != g_diligent_state.frame_counter)
	{
		if (entry.last_frame + 1 == g_diligent_state.frame_counter)
		{
			entry.prev = entry.curr;
		}
		else
		{
			entry.prev = current;
		}
		entry.curr = current;
		entry.last_frame = g_diligent_state.frame_counter;
	}
	else
	{
		entry.curr = current;
	}

	return entry.prev;
}

bool diligent_ssao_fx_should_run()
{
	return g_CV_SSAOEnable.m_Val != 0 && g_CV_SSAOBackend.m_Val != 0;
}

void diligent_ssao_fx_release_targets()
{
	g_ssao_fx_state.targets = {};
	g_ssao_fx_state.history_valid = false;
	g_ssao_fx_state.has_prev_view_proj = false;
	g_ssao_fx_state.has_prev_camera = false;
}

bool diligent_ssao_fx_ensure_targets(uint32 width, uint32 height, Diligent::TEXTURE_FORMAT color_format)
{
	auto& targets = g_ssao_fx_state.targets;
	if (targets.width == width && targets.height == height && targets.color_format == color_format &&
		targets.normal && targets.motion && targets.roughness && targets.depth && targets.depth_history &&
		targets.scene_color && targets.scene_color_srv && targets.scene_color_rtv)
	{
		return true;
	}

	diligent_ssao_fx_release_targets();

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	targets.width = width;
	targets.height = height;
	targets.color_format = color_format;

	Diligent::TextureDesc color_desc;
	color_desc.Name = "ltjs_ssao_fx_scene_color";
	color_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	color_desc.Width = width;
	color_desc.Height = height;
	color_desc.MipLevels = 1;
	color_desc.Format = color_format;
	color_desc.Usage = Diligent::USAGE_DEFAULT;
	// Allow copy/resolve into the scene color on Vulkan by enabling RT usage.
	color_desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
	g_diligent_state.render_device->CreateTexture(color_desc, nullptr, &targets.scene_color);
	if (!targets.scene_color)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}
	targets.scene_color_srv = targets.scene_color->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!targets.scene_color_srv)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}
	targets.scene_color_rtv = targets.scene_color->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	if (!targets.scene_color_rtv)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}

	Diligent::TextureDesc normal_desc;
	normal_desc.Name = "ltjs_ssao_fx_normal";
	normal_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	normal_desc.Width = width;
	normal_desc.Height = height;
	normal_desc.MipLevels = 1;
	normal_desc.Format = kSsaoFxNormalFormat;
	normal_desc.Usage = Diligent::USAGE_DEFAULT;
	normal_desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
	g_diligent_state.render_device->CreateTexture(normal_desc, nullptr, &targets.normal);
	if (!targets.normal)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}
	targets.normal_rtv = targets.normal->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	targets.normal_srv = targets.normal->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!targets.normal_rtv || !targets.normal_srv)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}

	Diligent::TextureDesc motion_desc = normal_desc;
	motion_desc.Name = "ltjs_ssao_fx_motion";
	motion_desc.Format = kSsaoFxMotionFormat;
	g_diligent_state.render_device->CreateTexture(motion_desc, nullptr, &targets.motion);
	if (!targets.motion)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}
	targets.motion_rtv = targets.motion->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	targets.motion_srv = targets.motion->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!targets.motion_rtv || !targets.motion_srv)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}

	Diligent::TextureDesc roughness_desc = normal_desc;
	roughness_desc.Name = "ltjs_ssao_fx_roughness";
	roughness_desc.Format = kSsaoFxRoughnessFormat;
	g_diligent_state.render_device->CreateTexture(roughness_desc, nullptr, &targets.roughness);
	if (!targets.roughness)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}
	targets.roughness_rtv = targets.roughness->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	targets.roughness_srv = targets.roughness->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!targets.roughness_rtv || !targets.roughness_srv)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}

	Diligent::TextureDesc depth_desc;
	depth_desc.Name = "ltjs_ssao_fx_depth";
	depth_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	depth_desc.Width = width;
	depth_desc.Height = height;
	depth_desc.MipLevels = 1;
	depth_desc.Format = kSsaoFxDepthFormat;
	depth_desc.Usage = Diligent::USAGE_DEFAULT;
	depth_desc.BindFlags = Diligent::BIND_DEPTH_STENCIL | Diligent::BIND_SHADER_RESOURCE;
	g_diligent_state.render_device->CreateTexture(depth_desc, nullptr, &targets.depth);
	if (!targets.depth)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}
	targets.depth_dsv = targets.depth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
	targets.depth_srv = targets.depth->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!targets.depth_dsv || !targets.depth_srv)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}

	Diligent::TextureDesc depth_history_desc = depth_desc;
	depth_history_desc.Name = "ltjs_ssao_fx_depth_history";
	g_diligent_state.render_device->CreateTexture(depth_history_desc, nullptr, &targets.depth_history);
	if (!targets.depth_history)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}
	targets.depth_history_srv = targets.depth_history->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!targets.depth_history_srv)
	{
		diligent_ssao_fx_release_targets();
		return false;
	}

	g_ssao_fx_state.history_valid = false;
	g_ssao_fx_state.has_prev_view_proj = false;
	g_ssao_fx_state.has_prev_camera = false;
	return true;
}

bool diligent_ssao_fx_ensure_objects()
{
	if (!g_diligent_state.render_device)
	{
		return false;
	}

	if (!g_ssao_fx_state.post_fx)
	{
		Diligent::PostFXContext::CreateInfo create_info{};
		create_info.EnableAsyncCreation = false;
		create_info.PackMatrixRowMajor = true;
		g_ssao_fx_state.post_fx = std::make_unique<Diligent::PostFXContext>(g_diligent_state.render_device, create_info);
	}

	if (!g_ssao_fx_state.ssao)
	{
		Diligent::ScreenSpaceAmbientOcclusion::CreateInfo create_info{};
		create_info.EnableAsyncCreation = false;
		g_ssao_fx_state.ssao = std::make_unique<Diligent::ScreenSpaceAmbientOcclusion>(g_diligent_state.render_device, create_info);
	}

	return g_ssao_fx_state.post_fx && g_ssao_fx_state.ssao;
}

bool diligent_ssao_fx_ensure_prepass_resources()
{
	if (!g_diligent_state.render_device)
	{
		return false;
	}

	if (!g_ssao_fx_state.prepass_ps)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "PSMain";

		Diligent::ShaderDesc pixel_desc;
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		pixel_desc.Name = "ltjs_ssao_fx_prepass_ps";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_ssao_prepass_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssao_fx_state.prepass_ps);
	}

	if (!g_ssao_fx_state.world_prepass_vs)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "VSMain";

		Diligent::ShaderDesc vertex_desc;
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		vertex_desc.Name = "ltjs_ssao_fx_world_prepass_vs";
		shader_info.Desc = vertex_desc;
		shader_info.Source = diligent_get_world_ssao_prepass_vertex_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssao_fx_state.world_prepass_vs);
	}

	if (!g_ssao_fx_state.prepass_constants)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_ssao_fx_prepass_constants";
		desc.Size = sizeof(DiligentSsaoPrepassConstants);
		desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_ssao_fx_state.prepass_constants);
	}

	return g_ssao_fx_state.prepass_ps && g_ssao_fx_state.world_prepass_vs && g_ssao_fx_state.prepass_constants;
}

bool diligent_ssao_fx_ensure_composite_resources()
{
	if (!g_diligent_state.render_device)
	{
		return false;
	}

	if (!g_ssao_fx_state.composite_vs)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "VSMain";

		Diligent::ShaderDesc vertex_desc;
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		vertex_desc.Name = "ltjs_ssao_fx_composite_vs";
		shader_info.Desc = vertex_desc;
		shader_info.Source = diligent_get_optimized_2d_vertex_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssao_fx_state.composite_vs);
	}

	if (!g_ssao_fx_state.composite_ps)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "PSMain";

		Diligent::ShaderDesc pixel_desc;
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		pixel_desc.Name = "ltjs_ssao_fx_composite_ps";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_ssao_composite_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssao_fx_state.composite_ps);
	}

	if (!g_ssao_fx_state.composite_constants)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_ssao_fx_composite_constants";
		desc.Size = sizeof(DiligentSsaoCompositeConstants);
		desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_ssao_fx_state.composite_constants);
	}

	return g_ssao_fx_state.composite_vs && g_ssao_fx_state.composite_ps && g_ssao_fx_state.composite_constants;
}

DiligentSsaoFxCompositePipeline* diligent_ssao_fx_get_copy_pipeline(Diligent::TEXTURE_FORMAT rtv_format)
{
	auto& pipeline = g_ssao_fx_state.copy_pipeline;
	if (pipeline.pipeline_state && pipeline.rtv_format == rtv_format)
	{
		return &pipeline;
	}

	if (!g_diligent_state.render_device)
	{
		return nullptr;
	}

	if (!g_ssao_fx_state.copy_vs)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "VSMain";

		Diligent::ShaderDesc vertex_desc;
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		vertex_desc.Name = "ltjs_postfx_copy_vs";
		shader_info.Desc = vertex_desc;
		shader_info.Source = diligent_get_optimized_2d_vertex_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssao_fx_state.copy_vs);
	}

	if (!g_ssao_fx_state.copy_ps)
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "PSMain";

		Diligent::ShaderDesc pixel_desc;
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		pixel_desc.Name = "ltjs_postfx_copy_ps";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_postfx_copy_pixel_shader_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_ssao_fx_state.copy_ps);
	}

	if (!g_ssao_fx_state.copy_vs || !g_ssao_fx_state.copy_ps)
	{
		return nullptr;
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_postfx_copy";
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
		{Diligent::SHADER_TYPE_PIXEL, "g_ColorTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
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

	pipeline_info.pVS = g_ssao_fx_state.copy_vs;
	pipeline_info.pPS = g_ssao_fx_state.copy_ps;

	pipeline.pipeline_state.Release();
	pipeline.srb.Release();
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	if (!pipeline.srb)
	{
		return nullptr;
	}

	pipeline.rtv_format = rtv_format;
	return &pipeline;
}

bool diligent_ssao_fx_blit_scene_color(Diligent::ITextureView* source_srv, Diligent::ITextureView* dest_rtv)
{
	if (!source_srv || !dest_rtv || !g_diligent_state.immediate_context)
	{
		return false;
	}

	auto* dest_texture = dest_rtv->GetTexture();
	if (!dest_texture)
	{
		return false;
	}

	auto* pipeline = diligent_ssao_fx_get_copy_pipeline(dest_texture->GetDesc().Format);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	auto* color_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_ColorTex");
	if (color_var)
	{
		color_var->Set(source_srv);
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

	const auto dest_desc = dest_texture->GetDesc();
	diligent_set_viewport(static_cast<float>(dest_desc.Width), static_cast<float>(dest_desc.Height));

	g_diligent_state.immediate_context->SetRenderTargets(
		1,
		&dest_rtv,
		nullptr,
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

DiligentSsaoFxPrepassPipeline* diligent_ssao_fx_get_world_pipeline(
	Diligent::TEXTURE_FORMAT normal_format,
	Diligent::TEXTURE_FORMAT motion_format,
	Diligent::TEXTURE_FORMAT depth_format)
{
	auto& pipeline = g_ssao_fx_state.world_pipeline;
	if (pipeline.pipeline_state && pipeline.normal_format == normal_format &&
		pipeline.motion_format == motion_format && pipeline.roughness_format == kSsaoFxRoughnessFormat &&
		pipeline.depth_format == depth_format)
	{
		return &pipeline;
	}

	if (!diligent_ssao_fx_ensure_prepass_resources())
	{
		return nullptr;
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_ssao_fx_world_prepass";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 3;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = normal_format;
	pipeline_info.GraphicsPipeline.RTVFormats[1] = motion_format;
	pipeline_info.GraphicsPipeline.RTVFormats[2] = kSsaoFxRoughnessFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = depth_format;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline_info.GraphicsPipeline.SmplDesc.Count = 1;

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
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
	pipeline_info.GraphicsPipeline.RasterizerDesc.FillMode = Diligent::FILL_MODE_SOLID;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = true;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;

	Diligent::ShaderResourceVariableDesc vars[] =
	{
		{Diligent::SHADER_TYPE_PIXEL, "g_BaseTexture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	Diligent::ImmutableSamplerDesc sampler;
	sampler.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.AddressU = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler.Desc.AddressV = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler.Desc.AddressW = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler.SamplerOrTextureName = "g_BaseSampler";

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
	pipeline_info.PSODesc.ResourceLayout.Variables = vars;
	pipeline_info.PSODesc.ResourceLayout.NumVariables = static_cast<uint32>(std::size(vars));
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

	pipeline_info.pVS = g_ssao_fx_state.world_prepass_vs;
	pipeline_info.pPS = g_ssao_fx_state.prepass_ps;

	pipeline.pipeline_state.Release();
	pipeline.srb.Release();
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* vs_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "SsaoPrepassConstants");
	if (vs_constants)
	{
		vs_constants->Set(g_ssao_fx_state.prepass_constants);
	}
	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SsaoPrepassConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_ssao_fx_state.prepass_constants);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	if (!pipeline.srb)
	{
		return nullptr;
	}

	pipeline.normal_format = normal_format;
	pipeline.motion_format = motion_format;
	pipeline.roughness_format = kSsaoFxRoughnessFormat;
	pipeline.depth_format = depth_format;
	return &pipeline;
}

DiligentSsaoFxModelPipeline* diligent_ssao_fx_get_model_pipeline(
	const DiligentMeshLayout& layout,
	Diligent::TEXTURE_FORMAT normal_format,
	Diligent::TEXTURE_FORMAT motion_format,
	Diligent::TEXTURE_FORMAT depth_format)
{
	DiligentSsaoFxModelPipelineKey key{};
	key.layout_hash = layout.hash;
	key.normal_format = normal_format;
	key.motion_format = motion_format;
	key.roughness_format = kSsaoFxRoughnessFormat;
	key.depth_format = depth_format;

	auto it = g_ssao_fx_state.model_pipelines.find(key);
	if (it != g_ssao_fx_state.model_pipelines.end())
	{
		return &it->second;
	}

	if (!diligent_ssao_fx_ensure_prepass_resources())
	{
		return nullptr;
	}

	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	auto vs_it = g_ssao_fx_state.model_vs.find(layout.hash);
	if (vs_it != g_ssao_fx_state.model_vs.end())
	{
		vertex_shader = vs_it->second;
	}
	else
	{
		Diligent::ShaderCreateInfo shader_info;
		shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
		shader_info.EntryPoint = "VSMain";

		Diligent::ShaderDesc vertex_desc;
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		vertex_desc.Name = "ltjs_ssao_fx_model_prepass_vs";
		shader_info.Desc = vertex_desc;
		const std::string source = diligent_build_model_ssao_prepass_vertex_shader_source(layout);
		shader_info.Source = source.c_str();
		g_diligent_state.render_device->CreateShader(shader_info, &vertex_shader);
		if (vertex_shader)
		{
			g_ssao_fx_state.model_vs.emplace(layout.hash, vertex_shader);
		}
	}

	if (!vertex_shader)
	{
		return nullptr;
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_ssao_fx_model_prepass";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 3;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = normal_format;
	pipeline_info.GraphicsPipeline.RTVFormats[1] = motion_format;
	pipeline_info.GraphicsPipeline.RTVFormats[2] = kSsaoFxRoughnessFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = depth_format;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline_info.GraphicsPipeline.SmplDesc.Count = 1;
	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout.elements.data();
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(layout.elements.size());
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
	pipeline_info.GraphicsPipeline.RasterizerDesc.FillMode = Diligent::FILL_MODE_SOLID;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = true;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;

	Diligent::ShaderResourceVariableDesc vars[] =
	{
		{Diligent::SHADER_TYPE_PIXEL, "g_BaseTexture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	Diligent::ImmutableSamplerDesc sampler;
	sampler.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler.Desc.AddressU = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler.Desc.AddressV = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler.Desc.AddressW = Diligent::TEXTURE_ADDRESS_WRAP;
	sampler.SamplerOrTextureName = "g_BaseSampler";

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
	pipeline_info.PSODesc.ResourceLayout.Variables = vars;
	pipeline_info.PSODesc.ResourceLayout.NumVariables = static_cast<uint32>(std::size(vars));
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

	pipeline_info.pVS = vertex_shader;
	pipeline_info.pPS = g_ssao_fx_state.prepass_ps;

	DiligentSsaoFxModelPipeline pipeline;
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* vs_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "SsaoPrepassConstants");
	if (vs_constants)
	{
		vs_constants->Set(g_ssao_fx_state.prepass_constants);
	}
	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SsaoPrepassConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_ssao_fx_state.prepass_constants);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	if (!pipeline.srb)
	{
		return nullptr;
	}

	auto [insert_it, inserted] = g_ssao_fx_state.model_pipelines.emplace(key, std::move(pipeline));
	return inserted ? &insert_it->second : nullptr;
}

DiligentSsaoFxCompositePipeline* diligent_ssao_fx_get_composite_pipeline(Diligent::TEXTURE_FORMAT rtv_format)
{
	auto& pipeline = g_ssao_fx_state.composite_pipeline;
	if (pipeline.pipeline_state && pipeline.rtv_format == rtv_format)
	{
		return &pipeline;
	}

	if (!diligent_ssao_fx_ensure_composite_resources())
	{
		return nullptr;
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_ssao_fx_composite";
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
		{Diligent::SHADER_TYPE_PIXEL, "g_AOTex", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
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

	pipeline_info.pVS = g_ssao_fx_state.composite_vs;
	pipeline_info.pPS = g_ssao_fx_state.composite_ps;

	pipeline.pipeline_state.Release();
	pipeline.srb.Release();
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "SsaoCompositeConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_ssao_fx_state.composite_constants);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	if (!pipeline.srb)
	{
		return nullptr;
	}

	pipeline.rtv_format = rtv_format;
	return &pipeline;
}

bool diligent_ssao_fx_update_prepass_constants(const DiligentSsaoPrepassConstants& constants)
{
	if (!g_diligent_state.immediate_context || !g_ssao_fx_state.prepass_constants)
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(
		g_ssao_fx_state.prepass_constants,
		Diligent::MAP_WRITE,
		Diligent::MAP_FLAG_DISCARD,
		mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_ssao_fx_state.prepass_constants, Diligent::MAP_WRITE);
	return true;
}

bool diligent_ssao_fx_update_composite_constants(float intensity)
{
	if (!g_diligent_state.immediate_context || !g_ssao_fx_state.composite_constants)
	{
		return false;
	}

	DiligentSsaoCompositeConstants constants{};
	constants.intensity = intensity;

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(
		g_ssao_fx_state.composite_constants,
		Diligent::MAP_WRITE,
		Diligent::MAP_FLAG_DISCARD,
		mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_ssao_fx_state.composite_constants, Diligent::MAP_WRITE);
	return true;
}

void diligent_ssao_fx_store_matrix_column_major(const LTMatrix& matrix, Diligent::float4x4& out_matrix)
{
	// Our prepass shaders use mul(matrix, vector) with default column-major packing.
	// Transpose the LTMatrix to column-major memory layout.
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			out_matrix.m[row][col] = matrix.m[col][row];
		}
	}
}

void diligent_ssao_fx_store_matrix_row_major(const LTMatrix& matrix, Diligent::float4x4& out_matrix)
{
	// DiligentFX uses mul(vector, matrix) with row-major packing.
	// LTMatrix is column-vector based, so transpose to convert.
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			out_matrix.m[row][col] = matrix.m[col][row];
		}
	}
}

float diligent_ssao_fx_calc_handness(const ViewParams& params)
{
	LTVector cross = params.m_Right.Cross(params.m_Up);
	return (cross.Dot(params.m_Forward) < 0.0f) ? -1.0f : 1.0f;
}

void diligent_ssao_fx_build_camera_attribs(
	const ViewParams& params,
	uint32 width,
	uint32 height,
	uint32 frame_index,
	Diligent::HLSL::CameraAttribs& camera)
{
	const float w = static_cast<float>(width);
	const float h = static_cast<float>(height);
	camera.f4ViewportSize = Diligent::float4{w, h, w > 0.0f ? 1.0f / w : 0.0f, h > 0.0f ? 1.0f / h : 0.0f};
	camera.f4Position = Diligent::float4{params.m_Pos.x, params.m_Pos.y, params.m_Pos.z, 1.0f};
	camera.fHandness = diligent_ssao_fx_calc_handness(params);
	camera.uiFrameIndex = frame_index;
	camera.f2Jitter = Diligent::float2{0.0f, 0.0f};
	camera.SetClipPlanes(params.m_NearZ, params.m_FarZ);

	LTMatrix view = params.m_mView;
	LTMatrix proj = params.m_mProjection;
	LTMatrix view_proj = proj * view;
	LTMatrix view_inv = view;
	view_inv.Inverse();
	LTMatrix proj_inv = proj;
	proj_inv.Inverse();
	LTMatrix view_proj_inv = view_proj;
	view_proj_inv.Inverse();

	diligent_ssao_fx_store_matrix_row_major(view, camera.mView);
	diligent_ssao_fx_store_matrix_row_major(proj, camera.mProj);
	diligent_ssao_fx_store_matrix_row_major(view_proj, camera.mViewProj);
	diligent_ssao_fx_store_matrix_row_major(view_inv, camera.mViewInv);
	diligent_ssao_fx_store_matrix_row_major(proj_inv, camera.mProjInv);
	diligent_ssao_fx_store_matrix_row_major(view_proj_inv, camera.mViewProjInv);
}

LTMatrix diligent_ssao_fx_get_prev_model_transform(const ModelInstance* instance, const LTMatrix& current)
{
	return diligent_ssao_fx_get_prev_transform(instance, current, g_ssao_fx_state.model_prev);
}

LTMatrix diligent_ssao_fx_get_prev_world_model_transform(const WorldModelInstance* instance, const LTMatrix& current)
{
	return diligent_ssao_fx_get_prev_transform(instance, current, g_ssao_fx_state.world_prev);
}

bool diligent_ssao_fx_draw_render_blocks(
	const std::vector<DiligentRenderBlock*>& blocks,
	const LTMatrix& world_matrix,
	const LTMatrix& prev_world_matrix)
{
	if (!g_CV_DrawWorld.m_Val)
	{
		return true;
	}

	if (blocks.empty())
	{
		return true;
	}

	if (!g_diligent_state.immediate_context)
	{
		return false;
	}

	auto* pipeline = diligent_ssao_fx_get_world_pipeline(
		kSsaoFxNormalFormat,
		kSsaoFxMotionFormat,
		kSsaoFxDepthFormat);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	DiligentSsaoPrepassConstants constants{};
	diligent_store_matrix_from_lt(g_ssao_fx_state.current_view_proj, constants.view_proj);
	diligent_store_matrix_from_lt(g_ssao_fx_state.prev_view_proj, constants.prev_view_proj);
	diligent_store_matrix_from_lt(world_matrix, constants.world);
	diligent_store_matrix_from_lt(prev_world_matrix, constants.prev_world);
	const float default_roughness = std::clamp(g_CV_SSRDefaultRoughness.m_Val, 0.0f, 1.0f);
	constants.prepass_params[0] = default_roughness;
	constants.prepass_params[1] = 0.0f;
	if (!diligent_ssao_fx_update_prepass_constants(constants))
	{
		return false;
	}

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);

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
			bool draw_section = false;
			switch (static_cast<DiligentPCShaderType>(section.shader_code))
			{
				case kPcShaderGouraud:
				case kPcShaderLightmap:
				case kPcShaderLightmapTexture:
				case kPcShaderDualTexture:
				case kPcShaderLightmapDualTexture:
					draw_section = true;
					break;
				default:
					draw_section = false;
					break;
			}
			if (!draw_section)
			{
				continue;
			}

			float section_roughness = default_roughness;
			bool wants_alpha_roughness = false;
			diligent_section_get_roughness_override(section, section_roughness, wants_alpha_roughness);

			Diligent::ITextureView* texture_view = nullptr;
			if (wants_alpha_roughness && section.textures[0])
			{
				texture_view = diligent_get_texture_view(section.textures[0], false);
			}

			constants.prepass_params[0] = section_roughness;
			constants.prepass_params[1] = texture_view ? 1.0f : 0.0f;
			if (!diligent_ssao_fx_update_prepass_constants(constants))
			{
				return false;
			}

			auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_BaseTexture");
			if (base_var)
			{
				base_var->Set(texture_view);
			}

			g_diligent_state.immediate_context->CommitShaderResources(
				pipeline->srb,
				Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			Diligent::DrawIndexedAttribs draw_attribs;
			draw_attribs.NumIndices = section.tri_count * 3;
			draw_attribs.IndexType = Diligent::VT_UINT16;
			draw_attribs.FirstIndexLocation = section.start_index;
			const bool use_base_vertex = (g_CV_WorldUseBaseVertex.m_Val != 0) && block->use_base_vertex;
			draw_attribs.BaseVertex = use_base_vertex
				? static_cast<int32>(section.start_vertex)
				: 0;
			draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
			g_diligent_state.immediate_context->DrawIndexed(draw_attribs);
		}
	}

	return true;
}

bool diligent_ssao_fx_draw_model_mesh_prepass(
	ModelInstance* instance,
	const DiligentMeshLayout& layout,
	Diligent::IBuffer* const* vertex_buffers,
	Diligent::IBuffer* index_buffer,
	uint32 index_count,
	const LTMatrix& world_matrix,
	const LTMatrix& prev_world_matrix,
	Diligent::ITextureView* base_texture_view,
	bool use_alpha_roughness)
{
	if (!instance || !g_diligent_state.immediate_context || !vertex_buffers || !index_buffer)
	{
		return false;
	}

	if (index_count == 0)
	{
		return false;
	}

	if (layout.position_attrib < 0)
	{
		return false;
	}

	auto* pipeline = diligent_ssao_fx_get_model_pipeline(layout, kSsaoFxNormalFormat, kSsaoFxMotionFormat, kSsaoFxDepthFormat);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	DiligentSsaoPrepassConstants constants{};
	diligent_store_matrix_from_lt(g_ssao_fx_state.current_view_proj, constants.view_proj);
	diligent_store_matrix_from_lt(g_ssao_fx_state.prev_view_proj, constants.prev_view_proj);
	diligent_store_matrix_from_lt(world_matrix, constants.world);
	diligent_store_matrix_from_lt(prev_world_matrix, constants.prev_world);
	const float roughness = std::clamp(g_CV_SSRDefaultRoughness.m_Val, 0.0f, 1.0f);
	constants.prepass_params[0] = roughness;
	constants.prepass_params[1] = use_alpha_roughness ? 1.0f : 0.0f;
	if (!diligent_ssao_fx_update_prepass_constants(constants))
	{
		return false;
	}

	Diligent::IBuffer* bound_buffers[4] = {};
	Diligent::Uint64 offsets[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		bound_buffers[stream_index] = vertex_buffers[stream_index];
	}

	auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_BaseTexture");
	if (base_var)
	{
		base_var->Set(use_alpha_roughness ? base_texture_view : nullptr);
	}

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_diligent_state.immediate_context->CommitShaderResources(
		pipeline->srb,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		4,
		bound_buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
	g_diligent_state.immediate_context->SetIndexBuffer(index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs draw_attribs;
	draw_attribs.NumIndices = index_count;
	draw_attribs.IndexType = Diligent::VT_UINT16;
	draw_attribs.FirstIndexLocation = 0;
	draw_attribs.BaseVertex = 0;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->DrawIndexed(draw_attribs);
	return true;
}

bool diligent_ssao_fx_draw_rigid_mesh_prepass(
	ModelInstance* instance,
	DiligentRigidMesh* mesh,
	Diligent::ITextureView* base_texture_view,
	bool use_alpha_roughness)
{
	if (!instance || !mesh)
	{
		return false;
	}

	const auto& layout = mesh->GetLayout();
	LTMatrix model_matrix;
	diligent_ssao_fx_get_model_transform_raw(instance, mesh->GetBoneEffector(), model_matrix);
	const LTMatrix prev_model_matrix = diligent_ssao_fx_get_prev_model_transform(instance, model_matrix);

	Diligent::IBuffer* vertex_buffers[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		vertex_buffers[stream_index] = mesh->GetVertexBuffer(stream_index);
	}

	return diligent_ssao_fx_draw_model_mesh_prepass(
		instance,
		layout,
		vertex_buffers,
		mesh->GetIndexBuffer(),
		mesh->GetIndexCount(),
		model_matrix,
		prev_model_matrix,
		base_texture_view,
		use_alpha_roughness);
}

bool diligent_ssao_fx_draw_skel_mesh_prepass(
	ModelInstance* instance,
	DiligentSkelMesh* mesh,
	Diligent::ITextureView* base_texture_view,
	bool use_alpha_roughness)
{
	if (!instance || !mesh)
	{
		return false;
	}

	if (!mesh->UpdateSkinnedVertices(instance))
	{
		return false;
	}

	const auto& layout = mesh->GetLayout();
	LTMatrix model_matrix;
	model_matrix.Identity();
	LTMatrix prev_model_matrix;
	prev_model_matrix.Identity();

	Diligent::IBuffer* vertex_buffers[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		vertex_buffers[stream_index] = mesh->GetVertexBuffer(stream_index);
	}

	return diligent_ssao_fx_draw_model_mesh_prepass(
		instance,
		layout,
		vertex_buffers,
		mesh->GetIndexBuffer(),
		mesh->GetIndexCount(),
		model_matrix,
		prev_model_matrix,
		base_texture_view,
		use_alpha_roughness);
}

bool diligent_ssao_fx_draw_va_mesh_prepass(
	ModelInstance* instance,
	DiligentVAMesh* mesh,
	Diligent::ITextureView* base_texture_view,
	bool use_alpha_roughness)
{
	if (!instance || !mesh)
	{
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return false;
	}

	if (!mesh->UpdateVA(model, &instance->m_AnimTracker.m_TimeRef))
	{
		return false;
	}

	const auto& layout = mesh->GetLayout();
	LTMatrix model_matrix;
	diligent_ssao_fx_get_model_transform_raw(instance, mesh->GetBoneEffector(), model_matrix);
	const LTMatrix prev_model_matrix = diligent_ssao_fx_get_prev_model_transform(instance, model_matrix);

	Diligent::IBuffer* vertex_buffers[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		vertex_buffers[stream_index] = mesh->GetVertexBuffer(stream_index);
	}

	return diligent_ssao_fx_draw_model_mesh_prepass(
		instance,
		layout,
		vertex_buffers,
		mesh->GetIndexBuffer(),
		mesh->GetIndexCount(),
		model_matrix,
		prev_model_matrix,
		base_texture_view,
		use_alpha_roughness);
}

bool diligent_ssao_fx_draw_model_instance_prepass(ModelInstance* instance)
{
	if (!instance)
	{
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return false;
	}

	const uint32 piece_count = model->NumPieces();
	for (uint32 piece_index = 0; piece_index < piece_count; ++piece_index)
	{
		if (instance->IsPieceHidden(piece_index))
		{
			continue;
		}

		ModelPiece* piece = model->GetPiece(piece_index);
		if (!piece)
		{
			continue;
		}

		const int32 lod_bias = g_CV_ModelLODOffset.m_Val;
		const LTVector to_camera = g_diligent_state.view_params.m_Pos - instance->m_Pos;
		const float dist = to_camera.Mag();
		CDIModelDrawable* lod = piece->GetLODFromDist(lod_bias, dist);
		if (!lod)
		{
			lod = piece->GetLOD(0);
		}
		if (!lod)
		{
			continue;
		}

		DiligentRigidMesh* rigid_mesh = nullptr;
		DiligentSkelMesh* skel_mesh = nullptr;
		DiligentVAMesh* va_mesh = nullptr;

		switch (lod->GetType())
		{
			case CRenderObject::eRigidMesh:
				rigid_mesh = static_cast<DiligentRigidMesh*>(lod);
				break;
			case CRenderObject::eSkelMesh:
				skel_mesh = static_cast<DiligentSkelMesh*>(lod);
				break;
			case CRenderObject::eVAMesh:
				va_mesh = static_cast<DiligentVAMesh*>(lod);
				break;
			default:
				break;
		}

		CRenderStyle* render_style = nullptr;
		instance->GetRenderStyle(piece->m_iRenderStyle, &render_style);
		const bool wants_alpha_roughness = diligent_render_style_uses_alpha_roughness(render_style);

		std::array<SharedTexture*, MAX_PIECE_TEXTURES> piece_textures{};
		diligent_get_model_piece_textures(instance, piece, piece_textures);

		Diligent::ITextureView* base_texture_view = nullptr;
		if (wants_alpha_roughness && piece_textures[0])
		{
			base_texture_view = diligent_get_texture_view(piece_textures[0], false);
		}
		const bool use_alpha_roughness = base_texture_view != nullptr;

		if (rigid_mesh)
		{
			diligent_ssao_fx_draw_rigid_mesh_prepass(
				instance,
				rigid_mesh,
				base_texture_view,
				use_alpha_roughness);
		}
		else if (skel_mesh)
		{
			diligent_ssao_fx_draw_skel_mesh_prepass(
				instance,
				skel_mesh,
				base_texture_view,
				use_alpha_roughness);
		}
		else if (va_mesh)
		{
			diligent_ssao_fx_draw_va_mesh_prepass(
				instance,
				va_mesh,
				base_texture_view,
				use_alpha_roughness);
		}
	}

	return true;
}

bool diligent_ssao_fx_should_process_model(const LTObject* object)
{
	if (!object || !object->IsModel())
	{
		return false;
	}

	if (!g_CV_DrawModels.m_Val)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_Flags2 & FLAG2_SKYOBJECT)
	{
		return false;
	}

	const bool translucent = object->IsTranslucent();
	if (translucent)
	{
		return false;
	}

	if (!g_CV_DrawSolidModels.m_Val)
	{
		return false;
	}

	return true;
}

void diligent_ssao_fx_process_model_attachments(
	LTObject* object,
	int depth,
	std::unordered_set<const LTObject*>& visited)
{
	if (!object || !object->m_Attachments || !g_diligent_state.render_struct)
	{
		return;
	}

	if (depth > 8)
	{
		return;
	}

	Attachment* current = object->m_Attachments;
	while (current)
	{
		LTObject* attached = g_diligent_state.render_struct->ProcessAttachment(object, current);
		if (attached && visited.find(attached) == visited.end())
		{
			if (attached->m_Attachments)
			{
				diligent_ssao_fx_process_model_attachments(attached, depth + 1, visited);
			}

			if (diligent_ssao_fx_should_process_model(attached))
			{
				visited.insert(attached);
				diligent_ssao_fx_draw_model_instance_prepass(attached->ToModel());
			}
		}

		current = current->m_pNext;
	}
}

void diligent_ssao_fx_process_model_object(LTObject* object, std::unordered_set<const LTObject*>& visited)
{
	if (!diligent_ssao_fx_should_process_model(object))
	{
		return;
	}

	if (!visited.insert(object).second)
	{
		return;
	}

	diligent_ssao_fx_draw_model_instance_prepass(object->ToModel());
	if (object->m_Attachments)
	{
		diligent_ssao_fx_process_model_attachments(object, 0, visited);
	}
}

void diligent_ssao_fx_filter_world_node_for_models(
	const ViewParams& params,
	WorldTreeNode* node,
	std::unordered_set<const LTObject*>& visited)
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

		diligent_ssao_fx_process_model_object(object, visited);
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

		diligent_ssao_fx_filter_world_node_for_models(params, child, visited);
	}
}

bool diligent_ssao_fx_draw_models_prepass(SceneDesc* desc)
{
	if (!desc)
	{
		return true;
	}

	if (!g_CV_DrawModels.m_Val)
	{
		return true;
	}

	std::unordered_set<const LTObject*> visited;
	visited.reserve(256);

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return true;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_ssao_fx_process_model_object(object, visited);
		}

		return true;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	auto* bsp_client = diligent_get_world_bsp_client();
	if (!bsp_client)
	{
		return true;
	}

	WorldTree* tree = bsp_client->ClientTree();
	if (!tree)
	{
		return true;
	}

	diligent_ssao_fx_filter_world_node_for_models(g_diligent_state.view_params, tree->GetRootNode(), visited);

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_ssao_fx_process_model_object(object, visited);
	}

	return true;
}

bool diligent_ssao_fx_render_prepass(SceneDesc* desc)
{
	if (!desc || !g_diligent_state.immediate_context)
	{
		return false;
	}

	const uint32 width = desc->m_Rect.right - desc->m_Rect.left;
	const uint32 height = desc->m_Rect.bottom - desc->m_Rect.top;
	if (width == 0 || height == 0)
	{
		return false;
	}

	auto* render_target = diligent_get_active_render_target();
	if (!render_target || !render_target->GetTexture())
	{
		return false;
	}

	const Diligent::TEXTURE_FORMAT color_format = render_target->GetTexture()->GetDesc().Format;
	if (!diligent_ssao_fx_ensure_targets(width, height, color_format))
	{
		return false;
	}

	if (!diligent_ssao_fx_ensure_prepass_resources())
	{
		return false;
	}

	Diligent::ITextureView* prev_rtv = diligent_get_active_render_target();
	Diligent::ITextureView* prev_dsv = diligent_get_active_depth_target();

	Diligent::ITextureView* render_targets[] =
	{
		g_ssao_fx_state.targets.normal_rtv,
		g_ssao_fx_state.targets.motion_rtv,
		g_ssao_fx_state.targets.roughness_rtv
	};
	g_diligent_state.immediate_context->SetRenderTargets(
		static_cast<uint32>(std::size(render_targets)),
		render_targets,
		g_ssao_fx_state.targets.depth_dsv,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	const float normal_clear[4] = {0.0f, 0.0f, 1.0f, 0.0f};
	const float motion_clear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	const float roughness_clear[4] = {std::clamp(g_CV_SSRDefaultRoughness.m_Val, 0.0f, 1.0f), 0.0f, 0.0f, 0.0f};
	g_diligent_state.immediate_context->ClearRenderTarget(
		g_ssao_fx_state.targets.normal_rtv,
		normal_clear,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_diligent_state.immediate_context->ClearRenderTarget(
		g_ssao_fx_state.targets.motion_rtv,
		motion_clear,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_diligent_state.immediate_context->ClearRenderTarget(
		g_ssao_fx_state.targets.roughness_rtv,
		roughness_clear,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_diligent_state.immediate_context->ClearDepthStencil(
		g_ssao_fx_state.targets.depth_dsv,
		Diligent::CLEAR_DEPTH_FLAG,
		1.0f,
		0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	LTMatrix world_matrix;
	world_matrix.Identity();
	const LTVector& world_offset = diligent_GetWorldOffset();
	world_matrix.m[0][3] = world_offset.x;
	world_matrix.m[1][3] = world_offset.y;
	world_matrix.m[2][3] = world_offset.z;

	LTMatrix prev_world_matrix = world_matrix;
	diligent_ssao_fx_draw_render_blocks(g_visible_render_blocks, world_matrix, prev_world_matrix);

	for (auto* instance : g_diligent_solid_world_models)
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

		LTMatrix world_model_matrix = instance->m_Transform;
		world_model_matrix.m[0][3] += world_offset.x;
		world_model_matrix.m[1][3] += world_offset.y;
		world_model_matrix.m[2][3] += world_offset.z;
		const LTMatrix prev_world_model_matrix = diligent_ssao_fx_get_prev_world_model_transform(instance, world_model_matrix);
		diligent_ssao_fx_draw_render_blocks(blocks, world_model_matrix, prev_world_model_matrix);
	}

	diligent_ssao_fx_draw_models_prepass(desc);

	g_diligent_state.immediate_context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
	if (prev_rtv || prev_dsv)
	{
		g_diligent_state.immediate_context->SetRenderTargets(1, &prev_rtv, prev_dsv, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}

	return true;
}

bool diligent_ssao_fx_execute(SceneDesc* desc, bool enable_ssao_fx)
{
	if (!desc || !g_diligent_state.immediate_context)
	{
		return false;
	}

	const uint32 width = desc->m_Rect.right - desc->m_Rect.left;
	const uint32 height = desc->m_Rect.bottom - desc->m_Rect.top;
	if (width == 0 || height == 0)
	{
		return false;
	}

	if (!diligent_ssao_fx_ensure_objects())
	{
		return false;
	}

	const bool reversed_depth = g_diligent_state.view_params.m_NearZ > g_diligent_state.view_params.m_FarZ;
	Diligent::PostFXContext::FEATURE_FLAGS postfx_flags = Diligent::PostFXContext::FEATURE_FLAG_NONE;
	if (reversed_depth)
	{
		postfx_flags = static_cast<Diligent::PostFXContext::FEATURE_FLAGS>(
			postfx_flags | Diligent::PostFXContext::FEATURE_FLAG_REVERSED_DEPTH);
	}
	if (g_CV_SSAOFxHalfPrecisionDepth.m_Val != 0)
	{
		postfx_flags = static_cast<Diligent::PostFXContext::FEATURE_FLAGS>(
			postfx_flags | Diligent::PostFXContext::FEATURE_FLAG_HALF_PRECISION_DEPTH);
	}

	g_ssao_fx_state.post_fx->PrepareResources(
		g_diligent_state.render_device,
		Diligent::PostFXContext::FrameDesc{g_diligent_state.frame_counter, width, height, width, height},
		postfx_flags);

	if (enable_ssao_fx)
	{
		Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAGS ssao_flags = Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAG_NONE;
		if (g_CV_SSAOFxHalfPrecisionDepth.m_Val != 0)
		{
			ssao_flags = static_cast<Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAGS>(
				ssao_flags | Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAG_HALF_PRECISION_DEPTH);
		}
		if (g_CV_SSAOFxHalfResolution.m_Val != 0)
		{
			ssao_flags = static_cast<Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAGS>(
				ssao_flags | Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAG_HALF_RESOLUTION);
		}
		if (g_CV_SSAOFxUniformWeighting.m_Val != 0)
		{
			ssao_flags = static_cast<Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAGS>(
				ssao_flags | Diligent::ScreenSpaceAmbientOcclusion::FEATURE_FLAG_UNIFORM_WEIGHTING);
		}

		g_ssao_fx_state.ssao->PrepareResources(
			g_diligent_state.render_device,
			g_diligent_state.immediate_context,
			g_ssao_fx_state.post_fx.get(),
			ssao_flags);
	}

	diligent_ssao_fx_build_camera_attribs(
		g_diligent_state.view_params,
		width,
		height,
		g_diligent_state.frame_counter,
		g_ssao_fx_state.curr_camera);

	if (!g_ssao_fx_state.has_prev_camera)
	{
		g_ssao_fx_state.prev_camera = g_ssao_fx_state.curr_camera;
	}

	Diligent::PostFXContext::RenderAttributes postfx_attribs;
	postfx_attribs.pDevice = g_diligent_state.render_device;
	postfx_attribs.pDeviceContext = g_diligent_state.immediate_context;
	postfx_attribs.pCurrDepthBufferSRV = g_ssao_fx_state.targets.depth_srv;
	postfx_attribs.pPrevDepthBufferSRV = g_ssao_fx_state.history_valid
		? g_ssao_fx_state.targets.depth_history_srv
		: g_ssao_fx_state.targets.depth_srv;
	postfx_attribs.pMotionVectorsSRV = g_ssao_fx_state.targets.motion_srv;
	postfx_attribs.pCurrCamera = &g_ssao_fx_state.curr_camera;
	postfx_attribs.pPrevCamera = &g_ssao_fx_state.prev_camera;
	g_ssao_fx_state.post_fx->Execute(postfx_attribs);

	if (enable_ssao_fx)
	{
		Diligent::HLSL::ScreenSpaceAmbientOcclusionAttribs ssao_attribs{};
		ssao_attribs.EffectRadius = std::max(0.001f, g_CV_SSAOFxEffectRadius.m_Val);
		ssao_attribs.EffectFalloffRange = std::max(0.001f, g_CV_SSAOFxEffectFalloffRange.m_Val);
		ssao_attribs.RadiusMultiplier = std::max(0.001f, g_CV_SSAOFxRadiusMultiplier.m_Val);
		ssao_attribs.DepthMIPSamplingOffset = std::max(0.0f, g_CV_SSAOFxDepthMipOffset.m_Val);
		ssao_attribs.TemporalStabilityFactor = std::clamp(g_CV_SSAOFxTemporalStability.m_Val, 0.0f, 0.999f);
		ssao_attribs.SpatialReconstructionRadius = std::max(0.1f, g_CV_SSAOFxSpatialReconstructionRadius.m_Val);
		ssao_attribs.AlphaInterpolation = std::clamp(g_CV_SSAOFxAlphaInterpolation.m_Val, 0.0f, 1.0f);
		ssao_attribs.ResetAccumulation = (!g_ssao_fx_state.history_valid || !g_ssao_fx_state.has_prev_camera ||
			g_CV_SSAOFxResetAccumulation.m_Val != 0) ? TRUE : FALSE;

		Diligent::ScreenSpaceAmbientOcclusion::RenderAttributes ssao_render_attribs;
		ssao_render_attribs.pDevice = g_diligent_state.render_device;
		ssao_render_attribs.pDeviceContext = g_diligent_state.immediate_context;
		ssao_render_attribs.pPostFXContext = g_ssao_fx_state.post_fx.get();
		ssao_render_attribs.pDepthBufferSRV = g_ssao_fx_state.targets.depth_srv;
		ssao_render_attribs.pNormalBufferSRV = g_ssao_fx_state.targets.normal_srv;
		ssao_render_attribs.pSSAOAttribs = &ssao_attribs;
		g_ssao_fx_state.ssao->Execute(ssao_render_attribs);
	}

	g_ssao_fx_state.prev_camera = g_ssao_fx_state.curr_camera;
	g_ssao_fx_state.has_prev_camera = true;
	g_ssao_fx_state.history_valid = true;
	g_ssao_fx_state.prev_view_proj = g_ssao_fx_state.current_view_proj;
	g_ssao_fx_state.has_prev_view_proj = true;

	Diligent::CopyTextureAttribs depth_copy_attribs;
	depth_copy_attribs.pSrcTexture = g_ssao_fx_state.targets.depth;
	depth_copy_attribs.pDstTexture = g_ssao_fx_state.targets.depth_history;
	depth_copy_attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	depth_copy_attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	g_diligent_state.immediate_context->CopyTexture(depth_copy_attribs);

	return true;
}

bool diligent_apply_ssao_fx_internal(
	Diligent::ITextureView* source_color,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target)
{
	if (!diligent_ssao_fx_should_run())
	{
		return true;
	}

	if (!source_color || !source_color->GetTexture() ||
		!render_target || !render_target->GetTexture() ||
		!g_diligent_state.immediate_context)
	{
		return false;
	}

	if (!diligent_ssao_fx_ensure_composite_resources())
	{
		return false;
	}

	auto* ao_srv = g_ssao_fx_state.ssao ? g_ssao_fx_state.ssao->GetAmbientOcclusionSRV() : nullptr;
	if (!ao_srv)
	{
		return false;
	}

	if (g_ssao_fx_state.targets.width == 0 || g_ssao_fx_state.targets.height == 0)
	{
		return false;
	}

	const Diligent::TEXTURE_FORMAT color_format = render_target->GetTexture()->GetDesc().Format;
	if (!diligent_ssao_fx_ensure_targets(g_ssao_fx_state.targets.width, g_ssao_fx_state.targets.height, color_format))
	{
		return false;
	}

	Diligent::CopyTextureAttribs color_copy_attribs;
	color_copy_attribs.pSrcTexture = source_color->GetTexture();
	color_copy_attribs.pDstTexture = g_ssao_fx_state.targets.scene_color;
	color_copy_attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	color_copy_attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	g_diligent_state.immediate_context->CopyTexture(color_copy_attribs);

	DiligentSsaoFxCompositePipeline* pipeline = diligent_ssao_fx_get_composite_pipeline(color_format);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	const float intensity = std::max(0.0f, g_CV_SSAOIntensity.m_Val);
	if (!diligent_ssao_fx_update_composite_constants(intensity))
	{
		return false;
	}

	auto* color_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_ColorTex");
	if (color_var)
	{
		color_var->Set(g_ssao_fx_state.targets.scene_color_srv);
	}
	auto* ao_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_AOTex");
	if (ao_var)
	{
		ao_var->Set(ao_srv);
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

bool diligent_ssao_fx_is_enabled()
{
	return diligent_ssao_fx_should_run();
}

bool diligent_postfx_prepass_needed()
{
	return diligent_ssao_fx_should_run() || g_CV_SSGIEnable.m_Val != 0;
}

bool diligent_prepare_ssao_fx(SceneDesc* desc)
{
	if (!diligent_ssao_fx_should_run())
	{
		return false;
	}

	g_ssao_fx_state.current_view_proj = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView;
	if (!g_ssao_fx_state.has_prev_view_proj)
	{
		g_ssao_fx_state.prev_view_proj = g_ssao_fx_state.current_view_proj;
	}

	if (!diligent_ssao_fx_render_prepass(desc))
	{
		return false;
	}

	return diligent_ssao_fx_execute(desc, true);
}

bool diligent_prepare_postfx_prepass(SceneDesc* desc, bool enable_ssao_fx)
{
	if (!desc)
	{
		return false;
	}

	g_ssao_fx_state.current_view_proj = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView;
	if (!g_ssao_fx_state.has_prev_view_proj)
	{
		g_ssao_fx_state.prev_view_proj = g_ssao_fx_state.current_view_proj;
	}

	if (!diligent_ssao_fx_render_prepass(desc))
	{
		return false;
	}

	return diligent_ssao_fx_execute(desc, enable_ssao_fx);
}

bool diligent_apply_ssao_fx(Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target)
{
	return diligent_apply_ssao_fx_internal(render_target, render_target, depth_target);
}

bool diligent_apply_ssao_fx_resolved(const DiligentAaContext& ctx)
{
	if (!ctx.active)
	{
		return true;
	}

	return diligent_apply_ssao_fx_internal(ctx.color_resolve_srv, ctx.final_render_target, ctx.final_depth_target);
}

void diligent_ssao_fx_term()
{
	g_ssao_fx_state = {};
}

Diligent::ITextureView* diligent_get_postfx_normal_srv()
{
	return g_ssao_fx_state.targets.normal_srv;
}

Diligent::ITextureView* diligent_get_postfx_motion_srv()
{
	return g_ssao_fx_state.targets.motion_srv;
}

Diligent::ITextureView* diligent_get_postfx_depth_srv()
{
	return g_ssao_fx_state.targets.depth_srv;
}

Diligent::ITextureView* diligent_get_postfx_depth_history_srv()
{
	return g_ssao_fx_state.targets.depth_history_srv;
}

Diligent::ITextureView* diligent_get_postfx_roughness_srv()
{
	return g_ssao_fx_state.targets.roughness_srv;
}

Diligent::ITextureView* diligent_get_postfx_scene_color_srv()
{
	return g_ssao_fx_state.targets.scene_color_srv;
}

Diligent::PostFXContext* diligent_get_postfx_context()
{
	return g_ssao_fx_state.post_fx.get();
}

bool diligent_postfx_copy_scene_color(Diligent::ITextureView* source_color)
{
	if (!source_color || !source_color->GetTexture() || !g_diligent_state.immediate_context)
	{
		return false;
	}

	auto* src_texture = source_color->GetTexture();
	const auto src_desc = src_texture->GetDesc();
	if (!diligent_ssao_fx_ensure_targets(src_desc.Width, src_desc.Height, src_desc.Format))
	{
		return false;
	}

	if (!g_ssao_fx_state.targets.scene_color)
	{
		return false;
	}

	const auto dst_desc = g_ssao_fx_state.targets.scene_color->GetDesc();
	if (dst_desc.Width != src_desc.Width || dst_desc.Height != src_desc.Height || dst_desc.Format != src_desc.Format)
	{
		return false;
	}

	g_diligent_state.immediate_context->SetRenderTargets(
		0,
		nullptr,
		nullptr,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (src_desc.SampleCount > 1 && dst_desc.SampleCount == 1)
	{
		Diligent::ResolveTextureSubresourceAttribs resolve_attribs;
		resolve_attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
		resolve_attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
		g_diligent_state.immediate_context->ResolveTextureSubresource(
			src_texture,
			g_ssao_fx_state.targets.scene_color,
			resolve_attribs);
		return true;
	}

	Diligent::CopyTextureAttribs copy_attribs;
	copy_attribs.pSrcTexture = src_texture;
	copy_attribs.pDstTexture = g_ssao_fx_state.targets.scene_color;
	copy_attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	copy_attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	g_diligent_state.immediate_context->CopyTexture(copy_attribs);
	return true;
}
