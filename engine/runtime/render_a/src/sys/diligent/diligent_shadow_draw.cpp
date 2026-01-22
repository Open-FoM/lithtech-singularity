#include "diligent_shadow_draw.h"

#include "diligent_device.h"
#include "diligent_glow_blur.h"
#include "diligent_state.h"
#include "diligent_model_draw.h"
#include "diligent_scene_collect.h"
#include "diligent_utils.h"
#include "diligent_world_draw.h"

#include "bdefs.h"
#include "de_objects.h"
#include "de_world.h"
#include "ltmatrix.h"
#include "ltvector.h"
#include "model.h"
#include "rendertarget.h"
#include "renderstruct.h"
#include "viewparams.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"

#include "../rendererconsolevars.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
constexpr uint32 kDiligentShadowMinSize = 8;
constexpr uint32 kDiligentShadowMaxSize = 512;
constexpr uint32 kDiligentMaxShadowTextures = 8;
}

struct DiligentShadowRenderParams;

bool diligent_shadow_render_texture(uint32 index, const DiligentShadowRenderParams& params);
bool diligent_shadow_blur_texture(uint32 src_index, uint32 dest_index);

bool diligent_draw_world_shadow_projection(
	const ViewParams& params,
	const DiligentShadowRenderParams& render_params,
	Diligent::ITextureView* shadow_view);

struct DiligentShadowTexture
{
	CRenderTarget target;
	uint32 width = 0;
	uint32 height = 0;

	bool Init(uint32 desired_width, uint32 desired_height)
	{
		if (desired_width == 0 || desired_height == 0)
		{
			return false;
		}

		width = desired_width;
		height = desired_height;
		return target.Init(static_cast<int>(width), static_cast<int>(height), RTFMT_X8R8G8B8, STFMT_UNKNOWN) == LT_OK;
	}

	void Term()
	{
		target.Term();
		width = 0;
		height = 0;
	}
};

struct DiligentShadowListEntry
{
	DiligentShadowTexture* texture = nullptr;
	bool dirty = true;
};

struct DiligentShadowList
{
	std::vector<DiligentShadowListEntry> textures;
	uint32 texture_res = 0;
};

struct DiligentQueuedShadow
{
	ModelInstance* instance = nullptr;
	LTVector model_pos;
	LTVector light_pos;
	LTVector light_color;
	bool ortho = false;
	float score = 0.0f;
};

struct DiligentShadowRenderParams
{
	ModelInstance* instance = nullptr;
	LTVector model_pos;
	LTVector light_pos;
	LTVector light_color;
	bool perspective = true;
	float proj_size_x = 0.0f;
	float proj_size_y = 0.0f;
};

struct DiligentShadowProjectionRequest
{
	DiligentShadowRenderParams params{};
	uint32 texture_index = 0;
};

std::vector<std::unique_ptr<DiligentShadowTexture>> g_diligent_shadow_textures;
static DiligentShadowList g_diligent_shadow_list;
static std::vector<DiligentQueuedShadow> g_diligent_shadow_queue;
static std::vector<DiligentShadowProjectionRequest> g_diligent_shadow_projection_queue;

void diligent_clear_shadow_queue()
{
	g_diligent_shadow_queue.clear();
	g_diligent_shadow_projection_queue.clear();
}

uint32 diligent_clamp_shadow_texture_size(uint32 size)
{
	size = LTCLAMP(size, kDiligentShadowMinSize, kDiligentShadowMaxSize);

	if ((size & (size - 1)) == 0)
	{
		return size;
	}

	uint32 clamped = 1;
	while ((clamped << 1) <= size)
	{
		clamped <<= 1;
	}

	return LTMAX(clamped, kDiligentShadowMinSize);
}

DiligentShadowTexture* diligent_alloc_shadow_texture(uint32 width, uint32 height)
{
	const uint32 clamped_width = diligent_clamp_shadow_texture_size(width);
	const uint32 clamped_height = diligent_clamp_shadow_texture_size(height);

	auto texture = std::make_unique<DiligentShadowTexture>();
	if (!texture->Init(clamped_width, clamped_height))
	{
		return nullptr;
	}

	g_diligent_shadow_textures.push_back(std::move(texture));
	return g_diligent_shadow_textures.back().get();
}

void diligent_free_shadow_texture(DiligentShadowTexture* texture)
{
	if (!texture)
	{
		return;
	}

	auto it = std::find_if(
		g_diligent_shadow_textures.begin(),
		g_diligent_shadow_textures.end(),
		[texture](const std::unique_ptr<DiligentShadowTexture>& entry)
		{
			return entry.get() == texture;
		});

	if (it == g_diligent_shadow_textures.end())
	{
		return;
	}

	(*it)->Term();
	g_diligent_shadow_textures.erase(it);
}

void diligent_release_shadow_textures()
{
	for (auto& texture : g_diligent_shadow_textures)
	{
		if (texture)
		{
			texture->Term();
		}
	}
	g_diligent_shadow_textures.clear();
	g_diligent_shadow_list.textures.clear();
	g_diligent_shadow_list.texture_res = 0;
}

void diligent_invalidate_shadow_textures()
{
	for (auto& entry : g_diligent_shadow_list.textures)
	{
		entry.dirty = true;
	}
}

void diligent_free_shadow_textures()
{
	for (auto& entry : g_diligent_shadow_list.textures)
	{
		if (entry.texture)
		{
			diligent_free_shadow_texture(entry.texture);
			entry.texture = nullptr;
		}
		entry.dirty = true;
	}
}

void diligent_update_shadow_texture_list()
{
	const uint32 desired_count = LTMIN(kDiligentMaxShadowTextures, static_cast<uint32>(g_CV_ModelShadow_Proj_NumTextures.m_Val));
	const uint32 desired_res = diligent_clamp_shadow_texture_size(static_cast<uint32>(g_CV_ModelShadow_Proj_TextureRes.m_Val));

	if (desired_count == 0)
	{
		diligent_free_shadow_textures();
		g_diligent_shadow_list.textures.clear();
		g_diligent_shadow_list.texture_res = 0;
		return;
	}

	if (desired_count != g_diligent_shadow_list.textures.size() || desired_res != g_diligent_shadow_list.texture_res)
	{
		diligent_free_shadow_textures();
		g_diligent_shadow_list.textures.clear();
		g_diligent_shadow_list.textures.resize(desired_count);
		g_diligent_shadow_list.texture_res = desired_res;
		diligent_invalidate_shadow_textures();
	}
}

DiligentShadowTexture* diligent_get_shadow_texture(uint32 index)
{
	diligent_update_shadow_texture_list();
	if (g_diligent_shadow_list.textures.empty() || index >= g_diligent_shadow_list.textures.size())
	{
		return nullptr;
	}

	auto& entry = g_diligent_shadow_list.textures[index];
	if (!entry.texture)
	{
		entry.texture = diligent_alloc_shadow_texture(g_diligent_shadow_list.texture_res, g_diligent_shadow_list.texture_res);
		entry.dirty = true;
	}

	return entry.texture;
}

bool diligent_is_shadow_texture_dirty(uint32 index)
{
	if (index >= g_diligent_shadow_list.textures.size())
	{
		return false;
	}

	return g_diligent_shadow_list.textures[index].dirty;
}

void diligent_mark_shadow_texture_clean(uint32 index)
{
	if (index >= g_diligent_shadow_list.textures.size())
	{
		return;
	}

	g_diligent_shadow_list.textures[index].dirty = false;
}

LTVector diligent_get_model_shadow_pos(ModelInstance* instance)
{
	if (!instance)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	LTVector model_pos = instance->GetPos();
	Model* model = instance->GetModelDB();
	if (model)
	{
		ModelNode* node = model->FindNode("translation");
		if (node)
		{
			LTMatrix transform;
			instance->GetNodeTransform(node->m_NodeIndex, transform, true);
			transform.GetTranslation(model_pos);
		}
	}

	return model_pos;
}

bool diligent_should_draw_model_shadow(ModelInstance* instance)
{
	if (!instance || !g_CV_ModelShadow_Proj_Enable.m_Val)
	{
		return false;
	}

	ModelHookData hook_data{};
	if (diligent_get_model_hook_data(instance, hook_data))
	{
		if ((hook_data.m_ObjectFlags & FLAG_SHADOW) == 0)
		{
			return false;
		}
		if (hook_data.m_ObjectFlags & FLAG_REALLYCLOSE)
		{
			return false;
		}
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return false;
	}

	return g_CV_DrawAllModelShadows.m_Val || model->m_bShadowEnable;
}

void diligent_queue_model_shadows(ModelInstance* instance)
{
	if (!instance || !diligent_should_draw_model_shadow(instance))
	{
		return;
	}

	const uint32 max_shadows = static_cast<uint32>(LTMAX(g_CV_ModelShadow_Proj_MaxShadows.m_Val, 0));
	if (max_shadows == 0)
	{
		return;
	}

	const LTVector model_pos = diligent_get_model_shadow_pos(instance);
	std::vector<DiligentQueuedShadow> candidates;
	candidates.reserve(g_diligent_num_world_dynamic_lights);

	const bool ortho = g_CV_ModelShadow_Proj_Perspective.m_Val == 0;
	for (uint32 i = 0; i < g_diligent_num_world_dynamic_lights; ++i)
	{
		const DynamicLight* light = g_diligent_world_dynamic_lights[i];
		if (!light)
		{
			continue;
		}

		if (!(light->m_Flags & FLAG_CASTSHADOWS) && !g_CV_DrawAllModelShadows.m_Val)
		{
			continue;
		}

		const float radius = light->m_LightRadius;
		if (radius <= 0.0f)
		{
			continue;
		}

		const LTVector delta = model_pos - light->m_Pos;
		const float dist = delta.Mag();
		if (dist >= radius)
		{
			continue;
		}

		const float atten = LTCLAMP(1.0f - (dist / radius), 0.0f, 1.0f);
		LTVector color(
			static_cast<float>(light->m_ColorR) * atten,
			static_cast<float>(light->m_ColorG) * atten,
			static_cast<float>(light->m_ColorB) * atten);
		const float score = LTMAX(color.x, LTMAX(color.y, color.z));

		if (score <= 0.0f)
		{
			continue;
		}

		DiligentQueuedShadow queued{};
		queued.instance = instance;
		queued.model_pos = model_pos;
		queued.light_pos = light->m_Pos;
		queued.light_color = color;
		queued.ortho = ortho;
		queued.score = score;
		candidates.push_back(queued);
	}

	if (candidates.empty())
	{
		return;
	}

	std::sort(candidates.begin(), candidates.end(),
		[](const DiligentQueuedShadow& a, const DiligentQueuedShadow& b)
		{
			return a.score > b.score;
		});

	const uint32 shadow_count = LTMIN(max_shadows, static_cast<uint32>(candidates.size()));
	for (uint32 i = 0; i < shadow_count; ++i)
	{
		g_diligent_shadow_queue.push_back(candidates[i]);
	}
}

float diligent_calc_shadow_score(const ViewParams& params, const DiligentQueuedShadow& shadow)
{
	const float dist_sqr = (params.m_Pos - shadow.model_pos).MagSqr();
	const float far_z = LTMAX(params.m_FarZ, 1.0f);
	const float dist_ratio = dist_sqr / (far_z * far_z);
	const float color_score = LTMAX(shadow.light_color.x, LTMAX(shadow.light_color.y, shadow.light_color.z)) / 255.0f;
	return (1.0f - dist_ratio) * 1.0f + color_score * 0.0f;
}

bool diligent_render_queued_model_shadows(const ViewParams& params)
{
	if (g_diligent_shadow_queue.empty() || !g_CV_ModelShadow_Proj_Enable.m_Val)
	{
		g_diligent_shadow_queue.clear();
		g_diligent_shadow_projection_queue.clear();
		return true;
	}

	diligent_update_shadow_texture_list();
	if (g_diligent_shadow_list.textures.empty())
	{
		g_diligent_shadow_queue.clear();
		g_diligent_shadow_projection_queue.clear();
		return true;
	}

	const bool blur_shadows = g_CV_ModelShadow_Proj_BlurShadows.m_Val != 0;
	const uint32 texture_stride = blur_shadows ? 2u : 1u;
	const uint32 max_by_textures = texture_stride > 0
		? static_cast<uint32>(g_diligent_shadow_list.textures.size() / texture_stride)
		: 0u;
	if (max_by_textures == 0)
	{
		g_diligent_shadow_queue.clear();
		g_diligent_shadow_projection_queue.clear();
		return true;
	}

	uint32 shadows_to_render = static_cast<uint32>(g_diligent_shadow_queue.size());
	if (g_CV_ModelShadow_Proj_MaxShadowsPerFrame.m_Val >= 0)
	{
		shadows_to_render = LTMIN(shadows_to_render, static_cast<uint32>(g_CV_ModelShadow_Proj_MaxShadowsPerFrame.m_Val));
		for (auto& shadow : g_diligent_shadow_queue)
		{
			shadow.score = diligent_calc_shadow_score(params, shadow);
		}
		std::sort(g_diligent_shadow_queue.begin(), g_diligent_shadow_queue.end(),
			[](const DiligentQueuedShadow& a, const DiligentQueuedShadow& b)
			{
				return a.score > b.score;
			});
	}

	shadows_to_render = LTMIN(shadows_to_render, max_by_textures);
	for (uint32 i = 0; i < shadows_to_render; ++i)
	{
		const auto& shadow = g_diligent_shadow_queue[i];
		if (!shadow.instance)
		{
			continue;
		}

		const float radius = shadow.instance->GetDims().Mag();
		const float size = radius * 2.0f * g_CV_ModelShadow_Proj_ProjAreaRadiusScale.m_Val;

		DiligentShadowRenderParams render_params{};
		render_params.instance = shadow.instance;
		render_params.model_pos = shadow.model_pos;
		render_params.light_pos = shadow.light_pos;
		render_params.light_color = shadow.light_color;
		render_params.perspective = !shadow.ortho;
		render_params.proj_size_x = size;
		render_params.proj_size_y = size;

		const uint32 texture_index = i * texture_stride;
		if (!diligent_shadow_render_texture(texture_index, render_params))
		{
			continue;
		}

		if (blur_shadows)
		{
			diligent_shadow_blur_texture(texture_index, texture_index + 1);
		}

		DiligentShadowTexture* shadow_texture = diligent_get_shadow_texture(texture_index);
		if (shadow_texture)
		{
			auto* shadow_view = shadow_texture->target.GetShaderResourceView();
			if (shadow_view)
			{
				diligent_draw_world_shadow_projection(params, render_params, shadow_view);
				DiligentShadowProjectionRequest request{};
				request.params = render_params;
				request.texture_index = texture_index;
				g_diligent_shadow_projection_queue.push_back(request);
			}
		}
	}

	g_diligent_shadow_queue.clear();
	return true;
}

void diligent_build_shadow_render_pass(RenderPassOp& pass)
{
	std::memset(&pass, 0, sizeof(pass));

	pass.BlendMode = RENDERSTYLE_NOBLEND;
	pass.ZBufferMode = RENDERSTYLE_NOZ;
	pass.CullMode = g_CV_ModelShadow_Proj_BackFaceCull.m_Val ? RENDERSTYLE_CULL_CCW : RENDERSTYLE_CULL_NONE;
	pass.ZBufferTestMode = RENDERSTYLE_ALPHATEST_LESSEQUAL;
	pass.AlphaTestMode = RENDERSTYLE_NOALPHATEST;
	pass.FillMode = RENDERSTYLE_FILL;

	for (uint32 i = 0; i < 4; ++i)
	{
		pass.TextureStages[i].TextureParam = RENDERSTYLE_NOTEXTURE;
		pass.TextureStages[i].ColorOp = RENDERSTYLE_COLOROP_SELECTARG1;
		pass.TextureStages[i].ColorArg1 = RENDERSTYLE_COLORARG_DIFFUSE;
		pass.TextureStages[i].ColorArg2 = RENDERSTYLE_COLORARG_DIFFUSE;
		pass.TextureStages[i].AlphaOp = RENDERSTYLE_ALPHAOP_SELECTARG1;
		pass.TextureStages[i].AlphaArg1 = RENDERSTYLE_ALPHAARG_DIFFUSE;
		pass.TextureStages[i].AlphaArg2 = RENDERSTYLE_ALPHAARG_DIFFUSE;
		pass.TextureStages[i].UVSource = RENDERSTYLE_UVFROM_MODELDATA_UVSET1;
		pass.TextureStages[i].UAddress = RENDERSTYLE_UVADDR_CLAMP;
		pass.TextureStages[i].VAddress = RENDERSTYLE_UVADDR_CLAMP;
		pass.TextureStages[i].TexFilter = RENDERSTYLE_TEXFILTER_BILINEAR;
		pass.TextureStages[i].UVTransform_Enable = false;
		pass.TextureStages[i].ProjectTexCoord = false;
		pass.TextureStages[i].TexCoordCount = 2;
	}

	pass.DynamicLight = false;
	pass.TextureFactor = 0;
	pass.AlphaRef = 0;
	pass.bUseBumpEnvMap = false;
	pass.BumpEnvMapStage = 0;
	pass.fBumpEnvMap_Scale = 0.0f;
	pass.fBumpEnvMap_Offset = 0.0f;
}

void diligent_build_shadow_render_shaders(RSRenderPassShaders& shaders)
{
	std::memset(&shaders, 0, sizeof(shaders));
	shaders.bUseVertexShader = false;
	shaders.bUsePixelShader = false;
	shaders.VertexShaderID = 0;
	shaders.PixelShaderID = 0;
}

void diligent_build_shadow_light_frame(const LTVector& light_dir, LTVector& light_up, LTVector& light_right)
{
	LTVector up(0.0f, 1.0f, 0.0f);
	const float dot = static_cast<float>(std::fabs(VEC_DOT(up, light_dir)));
	if (dot > 0.9999f)
	{
		up.Init(1.0f, 0.0f, 0.0f);
	}

	light_right = light_dir.Cross(up);
	light_right.Norm();
	light_up = light_right.Cross(light_dir);
	light_up.Norm();
}

LTMatrix diligent_build_shadow_view_matrix(
	const LTVector& light_pos,
	const LTVector& light_right,
	const LTVector& light_up,
	const LTVector& light_dir,
	LTMatrix& out_inv_view)
{
	LTMatrix light_transform;
	light_transform.Identity();
	Mat_SetBasisVectors(&light_transform, &light_right, &light_up, &light_dir);
	Mat_SetTranslation(light_transform, light_pos);

	LTMatrix view_matrix;
	Mat_InverseTransformation(&light_transform, &view_matrix);
	out_inv_view = light_transform;
	return view_matrix;
}

LTMatrix diligent_build_shadow_projection_matrix(
	float proj_size_x,
	float proj_size_y,
	float near_z,
	float far_z,
	float dist_light_to_model,
	bool perspective)
{
	LTMatrix projection;
	projection.Identity();

	if (perspective)
	{
		const float dist = LTMAX(dist_light_to_model, 0.01f);
		const float w_at_near = near_z * (proj_size_x / dist);
		const float h_at_near = near_z * (proj_size_y / dist);
		const float inv_depth = 1.0f / (far_z - near_z);

		projection.m[0][0] = (w_at_near > 0.0f) ? (2.0f * near_z / w_at_near) : 1.0f;
		projection.m[1][1] = (h_at_near > 0.0f) ? (2.0f * near_z / h_at_near) : 1.0f;
		projection.m[2][2] = far_z * inv_depth;
		projection.m[2][3] = 1.0f;
		projection.m[3][2] = (-far_z * near_z) * inv_depth;
		projection.m[3][3] = 0.0f;
	}
	else
	{
		const float inv_width = (proj_size_x > 0.0f) ? (2.0f / proj_size_x) : 1.0f;
		const float inv_height = (proj_size_y > 0.0f) ? (2.0f / proj_size_y) : 1.0f;
		const float inv_depth = 1.0f / (far_z - near_z);

		projection.m[0][0] = inv_width;
		projection.m[1][1] = inv_height;
		projection.m[2][2] = inv_depth;
		projection.m[3][2] = -near_z * inv_depth;
		projection.m[3][3] = 1.0f;
	}

	return projection;
}

bool diligent_shadow_render_texture(uint32 index, const DiligentShadowRenderParams& params)
{
	if (!g_diligent_state.immediate_context || !params.instance)
	{
		return false;
	}

	DiligentShadowTexture* texture = diligent_get_shadow_texture(index);
	if (!texture)
	{
		return false;
	}

	texture->target.InstallOnDevice();

	auto* rtv = texture->target.GetRenderTargetView();
	if (!rtv)
	{
		return false;
	}

	if (params.proj_size_x <= 0.0f || params.proj_size_y <= 0.0f)
	{
		return false;
	}

	const float clear_color[4] = {
		g_CV_ModelShadow_Proj_TintFill.m_Val ? 0.0f : 1.0f,
		g_CV_ModelShadow_Proj_TintFill.m_Val ? 1.0f : 1.0f,
		g_CV_ModelShadow_Proj_TintFill.m_Val ? 0.0f : 1.0f,
		1.0f
	};
	g_diligent_state.immediate_context->ClearRenderTarget(rtv, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	const float viewport_width = static_cast<float>(texture->width);
	const float viewport_height = static_cast<float>(texture->height);
	const float inset = (viewport_width > 2.0f && viewport_height > 2.0f) ? 1.0f : 0.0f;
	diligent_set_viewport_rect(inset, inset, viewport_width - inset * 2.0f, viewport_height - inset * 2.0f);

	LTVector light_dir = params.model_pos - params.light_pos;
	const float light_dir_len = light_dir.Mag();
	if (light_dir_len <= 0.0001f)
	{
		return false;
	}
	light_dir /= light_dir_len;

	LTVector light_up;
	LTVector light_right;
	diligent_build_shadow_light_frame(light_dir, light_up, light_right);

	const float dist_light_to_model = (params.light_pos - params.model_pos).Mag();
	constexpr float kShadowNearZ = 1.0f;
	constexpr float kShadowFarZ = 20000.0f;

	LTMatrix inv_view;
	LTMatrix view_matrix = diligent_build_shadow_view_matrix(params.light_pos, light_right, light_up, light_dir, inv_view);
	LTMatrix proj_matrix = diligent_build_shadow_projection_matrix(
		params.proj_size_x,
		params.proj_size_y,
		kShadowNearZ,
		kShadowFarZ,
		dist_light_to_model,
		params.perspective);

	const auto* rt_texture = texture->target.GetRenderTargetTexture();
	const auto* ds_texture = texture->target.GetDepthStencilView() ? texture->target.GetDepthStencilView()->GetTexture() : nullptr;
	const auto rt_format = rt_texture ? rt_texture->GetDesc().Format : Diligent::TEX_FORMAT_UNKNOWN;
	const auto ds_format = ds_texture ? ds_texture->GetDesc().Format : Diligent::TEX_FORMAT_UNKNOWN;

	ViewParams saved_params = g_diligent_state.view_params;
	const bool saved_shadow_mode = g_diligent_shadow_mode;
	g_diligent_shadow_mode = true;

	ViewParams shadow_params = g_diligent_state.view_params;
	shadow_params.m_mView = view_matrix;
	shadow_params.m_mProjection = proj_matrix;
	shadow_params.m_mInvView = inv_view;
	shadow_params.m_Pos = params.light_pos;
	shadow_params.m_Right = light_right;
	shadow_params.m_Up = light_up;
	shadow_params.m_Forward = light_dir;
	shadow_params.m_fScreenWidth = viewport_width;
	shadow_params.m_fScreenHeight = viewport_height;
	shadow_params.m_Rect.left = 0;
	shadow_params.m_Rect.top = 0;
	shadow_params.m_Rect.right = static_cast<int32>(texture->width);
	shadow_params.m_Rect.bottom = static_cast<int32>(texture->height);
	g_diligent_state.view_params = shadow_params;

	const uint8 shadow_r = static_cast<uint8>(LTCLAMP(255.0f - params.light_color.x, 0.0f, 255.0f));
	const uint8 shadow_g = static_cast<uint8>(LTCLAMP(255.0f - params.light_color.y, 0.0f, 255.0f));
	const uint8 shadow_b = static_cast<uint8>(LTCLAMP(255.0f - params.light_color.z, 0.0f, 255.0f));

	RenderPassOp pass{};
	RSRenderPassShaders shaders{};
	diligent_build_shadow_render_pass(pass);
	diligent_build_shadow_render_shaders(shaders);

	const bool ok = diligent_draw_model_shadow_with_attachments(
		params.instance,
		pass,
		shaders,
		rt_format,
		ds_format,
		shadow_r,
		shadow_g,
		shadow_b);

	g_diligent_state.view_params = saved_params;
	g_diligent_shadow_mode = saved_shadow_mode;

	if (!ok)
	{
		return false;
	}

	diligent_mark_shadow_texture_clean(index);
	return true;
}

bool diligent_update_shadow_project_constants(
	const LTMatrix& world_to_shadow,
	const LTVector& light_dir,
	const LTVector& proj_center,
	float max_proj_dist,
	bool fade)
{
	if (!g_diligent_state.immediate_context || !g_world_resources.shadow_project_constants)
	{
		return false;
	}

	DiligentShadowProjectConstants constants{};
	diligent_store_matrix_from_lt(world_to_shadow, constants.world_to_shadow);
	constants.light_dir[0] = light_dir.x;
	constants.light_dir[1] = light_dir.y;
	constants.light_dir[2] = light_dir.z;
	constants.light_dir[3] = max_proj_dist;
	constants.proj_center[0] = proj_center.x;
	constants.proj_center[1] = proj_center.y;
	constants.proj_center[2] = proj_center.z;
	constants.proj_center[3] = fade ? 1.0f : 0.0f;

	void* mapped = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.shadow_project_constants, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped);
	if (!mapped)
	{
		return false;
	}

	std::memcpy(mapped, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.shadow_project_constants, Diligent::MAP_WRITE);
	return true;
}

void diligent_draw_shadow_projection_blocks(const std::vector<DiligentRenderBlock*>& blocks)
{
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
			switch (static_cast<DiligentPCShaderType>(section.shader_code))
			{
				case kPcShaderSkypan:
				case kPcShaderSkyPortal:
				case kPcShaderOccluder:
				case kPcShaderNone:
				case kPcShaderUnknown:
					continue;
				default:
					break;
			}

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
}

bool diligent_begin_shadow_projection_pass(
	const ViewParams& params,
	const DiligentShadowRenderParams& render_params,
	Diligent::ITextureView* shadow_view)
{
	if (!shadow_view || !g_diligent_state.swap_chain || !g_diligent_state.immediate_context)
	{
		return false;
	}

	if (!diligent_ensure_shadow_project_constants())
	{
		return false;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	auto* rtv = diligent_get_active_render_target();
	auto* dsv = diligent_get_active_depth_target();
	if (!rtv)
	{
		return false;
	}

	g_diligent_state.immediate_context->SetRenderTargets(1, &rtv, dsv, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	diligent_set_viewport_rect(
		static_cast<float>(params.m_Rect.left),
		static_cast<float>(params.m_Rect.top),
		static_cast<float>(params.m_Rect.right - params.m_Rect.left),
		static_cast<float>(params.m_Rect.bottom - params.m_Rect.top));

	LTVector light_dir = render_params.model_pos - render_params.light_pos;
	const float light_len = light_dir.Mag();
	if (light_len <= 0.0001f)
	{
		return false;
	}
	light_dir /= light_len;

	LTVector light_up;
	LTVector light_right;
	diligent_build_shadow_light_frame(light_dir, light_up, light_right);

	LTMatrix inv_view;
	LTMatrix view_matrix = diligent_build_shadow_view_matrix(render_params.light_pos, light_right, light_up, light_dir, inv_view);
	constexpr float kShadowNearZ = 1.0f;
	constexpr float kShadowFarZ = 20000.0f;
	LTMatrix proj_matrix = diligent_build_shadow_projection_matrix(
		render_params.proj_size_x,
		render_params.proj_size_y,
		kShadowNearZ,
		kShadowFarZ,
		light_len,
		render_params.perspective);

	LTMatrix world_to_shadow = proj_matrix * view_matrix;

	const float fade_offset = render_params.instance
		? render_params.instance->GetDims().Mag() * g_CV_ModelShadow_Proj_DimFadeOffsetScale.m_Val
		: 0.0f;
	const float max_proj_dist = g_CV_ModelShadow_Proj_MaxProjDist.m_Val + fade_offset;
	const bool fade = g_CV_ModelShadow_Proj_Fade.m_Val != 0;

	if (!diligent_update_shadow_project_constants(world_to_shadow, light_dir, render_params.model_pos, max_proj_dist, fade))
	{
		return false;
	}

	auto* pipeline = diligent_get_world_pipeline(kWorldPipelineShadowProject, kWorldBlendMultiply, kWorldDepthEnabled, g_CV_Wireframe.m_Val != 0);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	auto* shadow_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_ShadowTexture");
	if (shadow_var)
	{
		shadow_var->Set(shadow_view);
	}

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	return true;
}

bool diligent_draw_world_shadow_projection(
	const ViewParams& params,
	const DiligentShadowRenderParams& render_params,
	Diligent::ITextureView* shadow_view)
{
	if (!shadow_view || !g_diligent_state.swap_chain)
	{
		return true;
	}

	if (g_visible_render_blocks.empty() && g_diligent_solid_world_models.empty())
	{
		return true;
	}

	if (!diligent_begin_shadow_projection_pass(params, render_params, shadow_view))
	{
		return false;
	}

	if (!g_visible_render_blocks.empty())
	{
		DiligentWorldConstants constants;
		LTMatrix world_matrix;
		world_matrix.Identity();
		diligent_fill_world_constants(params, world_matrix, false, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, constants);

		if (!diligent_update_world_constants_buffer(constants))
		{
			return false;
		}

		diligent_draw_shadow_projection_blocks(g_visible_render_blocks);
	}

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

		if (blocks.empty())
		{
			continue;
		}

		DiligentWorldConstants constants;
		LTMatrix world_matrix = instance->m_Transform;
		diligent_fill_world_constants(params, world_matrix, false, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, constants);

		if (!diligent_update_world_constants_buffer(constants))
		{
			return false;
		}

		diligent_draw_shadow_projection_blocks(blocks);
	}

	return true;
}

bool diligent_draw_world_model_shadow_projection(
	const ViewParams& params,
	const DiligentShadowRenderParams& render_params,
	Diligent::ITextureView* shadow_view,
	const std::vector<WorldModelInstance*>& models)
{
	if (!shadow_view || !g_diligent_state.swap_chain || models.empty())
	{
		return true;
	}

	if (!diligent_begin_shadow_projection_pass(params, render_params, shadow_view))
	{
		return false;
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

		if (blocks.empty())
		{
			continue;
		}

		DiligentWorldConstants constants;
		LTMatrix world_matrix = instance->m_Transform;
		diligent_fill_world_constants(params, world_matrix, false, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, constants);
		if (!diligent_update_world_constants_buffer(constants))
		{
			return false;
		}

		diligent_draw_shadow_projection_blocks(blocks);
	}

	return true;
}

bool diligent_draw_translucent_world_model_shadow_projection(const ViewParams& params)
{
	if (g_diligent_shadow_projection_queue.empty() || g_diligent_translucent_world_models.empty())
	{
		g_diligent_shadow_projection_queue.clear();
		return true;
	}

	for (const auto& request : g_diligent_shadow_projection_queue)
	{
		DiligentShadowTexture* shadow_texture = diligent_get_shadow_texture(request.texture_index);
		if (!shadow_texture)
		{
			continue;
		}

		auto* shadow_view = shadow_texture->target.GetShaderResourceView();
		if (!shadow_view)
		{
			continue;
		}

		if (!diligent_draw_world_model_shadow_projection(
				params,
				request.params,
				shadow_view,
				g_diligent_translucent_world_models))
		{
			g_diligent_shadow_projection_queue.clear();
			return false;
		}
	}

	g_diligent_shadow_projection_queue.clear();
	return true;
}

bool diligent_shadow_blur_pass(
	CRenderTarget& dest_target,
	Diligent::ITextureView* source_view,
	float u_weight,
	float v_weight,
	float pixel_spacing)
{
	if (!source_view)
	{
		return false;
	}

	auto* pipeline = diligent_get_glow_blur_pipeline(LTSURFACEBLEND_SOLID);
	if (!pipeline)
	{
		return false;
	}

	const float width = static_cast<float>(dest_target.GetRenderTargetParams().Width);
	const float height = static_cast<float>(dest_target.GetRenderTargetParams().Height);

	DiligentGlowBlurConstants constants{};
	const float offsets[] = {-1.0f, 0.0f, 1.0f};
	const float weights[] = {0.25f, 0.5f, 0.25f};
	for (uint32 i = 0; i < 3; ++i)
	{
		const float u_offset = (offsets[i] * pixel_spacing * u_weight) / width;
		const float v_offset = (offsets[i] * pixel_spacing * v_weight) / height;
		constants.taps[i] = {u_offset, v_offset, weights[i], 0.0f};
	}
	constants.params[0] = 3.0f;
	if (!diligent_update_glow_blur_constants(constants))
	{
		return false;
	}

	return diligent_draw_glow_blur_quad(
		pipeline,
		source_view,
		dest_target.GetRenderTargetView(),
		dest_target.GetDepthStencilView());
}

bool diligent_shadow_blur_texture(uint32 src_index, uint32 dest_index)
{
	if (!g_CV_ModelShadow_Proj_BlurShadows.m_Val)
	{
		return true;
	}

	DiligentShadowTexture* src_texture = diligent_get_shadow_texture(src_index);
	DiligentShadowTexture* dest_texture = diligent_get_shadow_texture(dest_index);
	if (!src_texture || !dest_texture)
	{
		return false;
	}

	auto* src_view = src_texture->target.GetShaderResourceView();
	if (!src_view)
	{
		return false;
	}

	dest_texture->target.InstallOnDevice();
	diligent_set_viewport(static_cast<float>(dest_texture->width), static_cast<float>(dest_texture->height));

	const float spacing = LTMAX(0.0f, g_CV_ModelShadow_Proj_BlurPixelSpacing.m_Val);
	if (!diligent_shadow_blur_pass(dest_texture->target, src_view, 1.0f, 0.0f, spacing))
	{
		return false;
	}

	auto* blur_view = dest_texture->target.GetShaderResourceView();
	if (!blur_view)
	{
		return false;
	}

	src_texture->target.InstallOnDevice();
	diligent_set_viewport(static_cast<float>(src_texture->width), static_cast<float>(src_texture->height));
	if (!diligent_shadow_blur_pass(src_texture->target, blur_view, 0.0f, 1.0f, spacing))
	{
		return false;
	}

	return true;
}
