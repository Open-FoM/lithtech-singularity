#include "diligent_object_draw.h"

#include "diligent_debug_draw.h"
#include "diligent_internal.h"
#include "diligent_device.h"
#include "diligent_texture_cache.h"
#include "diligent_utils.h"
#include "diligent_world_data.h"
#include "diligent_world_draw.h"

#include "bdefs.h"
#include "de_objects.h"
#include "de_world.h"
#include "ltanimtracker.h"
#include "ltb.h"
#include "iltclient.h"
#include "iltdrawprim.h"
#include "ltvector.h"
#include "renderstruct.h"
#include "viewparams.h"
#include "world_client_bsp.h"
#include "world_shared_bsp.h"
#include "world_tree.h"

#include "../rendererconsolevars.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <vector>

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"

#ifndef NEARZ
#define NEARZ SCREEN_NEAR_Z
#endif

std::vector<SpriteInstance*> g_diligent_translucent_sprites;
std::vector<SpriteInstance*> g_diligent_noz_sprites;
std::vector<LineSystem*> g_diligent_line_systems;
std::vector<LTParticleSystem*> g_diligent_particle_systems;
std::vector<Canvas*> g_diligent_solid_canvases;
std::vector<Canvas*> g_diligent_translucent_canvases;
std::vector<LTVolumeEffect*> g_diligent_solid_volume_effects;
std::vector<LTVolumeEffect*> g_diligent_translucent_volume_effects;
std::vector<LTPolyGrid*> g_diligent_solid_polygrids;
std::vector<LTPolyGrid*> g_diligent_early_translucent_polygrids;
std::vector<LTPolyGrid*> g_diligent_translucent_polygrids;

bool diligent_draw_sprite_list(
	const std::vector<SpriteInstance*>& sprites,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode);

bool diligent_draw_polygrid_list(
	const std::vector<LTPolyGrid*>& grids,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode,
	bool sort);

static uint32 diligent_pack_line_color(const LSLinePoint& point, float alpha_scale)
{
	const uint8 r = static_cast<uint8>(LTCLAMP(point.r * 255.0f, 0.0f, 255.0f));
	const uint8 g = static_cast<uint8>(LTCLAMP(point.g * 255.0f, 0.0f, 255.0f));
	const uint8 b = static_cast<uint8>(LTCLAMP(point.b * 255.0f, 0.0f, 255.0f));
	const uint8 a = static_cast<uint8>(LTCLAMP(point.a * alpha_scale, 0.0f, 255.0f));
	return (static_cast<uint32>(a) << 24) |
		(static_cast<uint32>(r) << 16) |
		(static_cast<uint32>(g) << 8) |
		static_cast<uint32>(b);
}

static float diligent_calc_draw_distance(const LTObject* object, const ViewParams& params)
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

template <typename TObject>
static void diligent_sort_translucent_list(std::vector<TObject*>& objects, const ViewParams& params)
{
	if (!g_CV_DrawSorted.m_Val || objects.size() < 2)
	{
		return;
	}

	std::sort(objects.begin(), objects.end(), [&params](const TObject* a, const TObject* b)
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

		const float dist_a = diligent_calc_draw_distance(obj_a, params);
		const float dist_b = diligent_calc_draw_distance(obj_b, params);
		return dist_a > dist_b;
	});
}

DiligentWorldBlendMode diligent_get_object_blend_mode(const LTObject* object)
{
	if (!object)
	{
		return kWorldBlendSolid;
	}

	if (object->m_Flags2 & FLAG2_ADDITIVE)
	{
		return kWorldBlendAdditiveOne;
	}

	if (object->m_Flags2 & FLAG2_MULTIPLY)
	{
		return kWorldBlendMultiply;
	}

	return object->IsTranslucent() ? kWorldBlendAlpha : kWorldBlendSolid;
}


struct DiligentSpriteVertex
{
	LTVector m_Vec;
	uint32 color;
	float u;
	float v;

	void SetupVert(const LTVector& pos, uint32 rgba, float tu, float tv)
	{
		m_Vec = pos;
		color = rgba;
		u = tu;
		v = tv;
	}

	static void ClipExtra(DiligentSpriteVertex* prev, DiligentSpriteVertex* cur, DiligentSpriteVertex* out, float t)
	{
		out->u = prev->u + t * (cur->u - prev->u);
		out->v = prev->v + t * (cur->v - prev->v);
		out->color = cur->color;
	}
};

template<class T>
bool diligent_clip_sprite(SpriteInstance* instance, HPOLY poly, T** points, uint32* point_count, T* out_points)
{
	if (!diligent_get_world_bsp_client() || !instance || poly == INVALID_HPOLY)
	{
		return false;
	}

	WorldPoly* world_poly = diligent_get_world_bsp_client()->GetPolyFromHPoly(poly);
	if (!world_poly)
	{
		return false;
	}

	float d1 = 0.0f;
	float d2 = 0.0f;
	const float dot = world_poly->GetPlane()->DistTo(g_diligent_state.view_params.m_Pos);
	if (dot <= 0.01f)
	{
		return false;
	}

	T* pVerts = *points;
	uint32 nVerts = *point_count;
	T* pOut = out_points;

	LTPlane plane;
	SPolyVertex* end_point = &world_poly->GetVertices()[world_poly->GetNumVertices()];
	SPolyVertex* prev_point = end_point - 1;
	for (SPolyVertex* cur_point = world_poly->GetVertices(); cur_point != end_point; )
	{
		LTVector vec_to;
		VEC_SUB(vec_to, cur_point->m_Vertex->m_Vec, prev_point->m_Vertex->m_Vec);
		VEC_CROSS(plane.m_Normal, vec_to, world_poly->GetPlane()->m_Normal);
		VEC_NORM(plane.m_Normal);
		plane.m_Dist = VEC_DOT(plane.m_Normal, cur_point->m_Vertex->m_Vec);

		#define PLANETEST(pt) (plane.DistTo(pt) > 0.0f)
		#define DOPLANECLIP(pt1, pt2) \
			d1 = plane.DistTo(pt1); \
			d2 = plane.DistTo(pt2); \
			t = -d1 / (d2 - d1); \
			pOut->m_Vec.x = pt1.x + ((pt2.x - pt1.x) * t); \
			pOut->m_Vec.y = pt1.y + ((pt2.y - pt1.y) * t); \
			pOut->m_Vec.z = pt1.z + ((pt2.z - pt1.z) * t);
		#define CLIPTEST PLANETEST
		#define DOCLIP DOPLANECLIP
		#include "../polyclip.h"
		#undef CLIPTEST
		#undef DOCLIP
		#undef PLANETEST
		#undef DOPLANECLIP

		prev_point = cur_point;
		++cur_point;
	}

	*points = pVerts;
	*point_count = nVerts;
	return true;
}




bool diligent_collect_sky_extents(const ViewParams& params, float& min_x, float& min_y, float& max_x, float& max_y)
{
	if (g_visible_render_blocks.empty())
	{
		return false;
	}

	min_x = FLT_MAX;
	min_y = FLT_MAX;
	max_x = -FLT_MAX;
	max_y = -FLT_MAX;

	for (auto* block : g_visible_render_blocks)
	{
		if (!block)
		{
			continue;
		}

		block->ExtendSkyBounds(params, min_x, min_y, max_x, max_y);
	}

	return min_x != FLT_MAX && min_y != FLT_MAX && max_x != -FLT_MAX && max_y != -FLT_MAX;
}

bool diligent_draw_sky_objects(SceneDesc* desc, const ViewParams& sky_params, bool& out_drew)
{
	out_drew = false;
	if (!desc || !desc->m_SkyObjects || desc->m_nSkyObjects <= 0)
	{
		return true;
	}

	std::vector<WorldModelInstance*> solid_world_models;
	std::vector<WorldModelInstance*> translucent_world_models;
	std::vector<SpriteInstance*> translucent_sprites;
	std::vector<LTPolyGrid*> polygrids;

	for (int i = 0; i < desc->m_nSkyObjects; ++i)
	{
		auto* sky_object = desc->m_SkyObjects[i];
		if (!sky_object || (sky_object->m_Flags & FLAG_VISIBLE) == 0)
		{
			continue;
		}

		if (sky_object->HasWorldModel() && g_CV_DrawWorldModels.m_Val)
		{
			auto* instance = sky_object->ToWorldModel();
			if (!instance)
			{
				continue;
			}

			if (sky_object->IsTranslucent())
			{
				translucent_world_models.push_back(instance);
			}
			else
			{
				solid_world_models.push_back(instance);
			}

			continue;
		}

		if (sky_object->m_ObjectType == OT_SPRITE && g_CV_DrawSprites.m_Val)
		{
			auto* sprite = sky_object->ToSprite();
			if (sprite)
			{
				translucent_sprites.push_back(sprite);
			}
			continue;
		}

		if (sky_object->m_ObjectType == OT_POLYGRID && g_CV_DrawPolyGrids.m_Val)
		{
			auto* grid = sky_object->ToPolyGrid();
			if (grid)
			{
				polygrids.push_back(grid);
			}
		}
	}

	if (!solid_world_models.empty())
	{
		out_drew = true;
		if (!diligent_draw_world_model_list_with_view(
				solid_world_models,
				sky_params,
				g_CV_SkyFogNearZ.m_Val,
				g_CV_SkyFogFarZ.m_Val,
				kWorldBlendSolid,
				kWorldDepthDisabled))
		{
			return false;
		}
	}

	if (!translucent_world_models.empty())
	{
		out_drew = true;
		if (!diligent_draw_world_model_list_with_view(
				translucent_world_models,
				sky_params,
				g_CV_SkyFogNearZ.m_Val,
				g_CV_SkyFogFarZ.m_Val,
				kWorldBlendAlpha,
				kWorldDepthDisabled))
		{
			return false;
		}
	}

	if (!polygrids.empty())
	{
		out_drew = true;
		if (!diligent_draw_polygrid_list(polygrids, sky_params, g_CV_SkyFogNearZ.m_Val, g_CV_SkyFogFarZ.m_Val, kWorldDepthDisabled, false))
		{
			return false;
		}
	}

	if (!translucent_sprites.empty())
	{
		out_drew = true;
		if (!diligent_draw_sprite_list(translucent_sprites, sky_params, g_CV_SkyFogNearZ.m_Val, g_CV_SkyFogFarZ.m_Val, kWorldDepthDisabled))
		{
			return false;
		}
	}

	return true;
}

bool diligent_draw_sky(SceneDesc* desc)
{
	if (!desc || desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	if (!g_CV_DrawSky.m_Val || !desc->m_SkyObjects || desc->m_nSkyObjects <= 0)
	{
		return true;
	}

	float min_x = 0.0f;
	float min_y = 0.0f;
	float max_x = 0.0f;
	float max_y = 0.0f;
	if (!diligent_collect_sky_extents(g_diligent_state.view_params, min_x, min_y, max_x, max_y))
	{
		return true;
	}

	if ((max_x - min_x) <= 0.9f || (max_y - min_y) <= 0.9f)
	{
		return true;
	}

	ViewBoxDef view_box;
lt_InitViewBoxFromParams(&view_box, 0.01f, g_CV_SkyFarZ.m_Val, g_diligent_state.view_params, min_x, min_y, max_x, max_y);

	LTMatrix mat = g_diligent_state.view_params.m_mInvView;
	mat.SetTranslation(g_diligent_state.view_params.m_SkyViewPos);

	ViewParams sky_params;
	if (!lt_InitFrustrum(
			&sky_params,
			&view_box,
			min_x,
			min_y,
			max_x,
			max_y,
			&mat,
			LTVector(g_CV_SkyScale.m_Val, g_CV_SkyScale.m_Val, g_CV_SkyScale.m_Val),
			g_diligent_state.view_params.m_eRenderMode))
	{
		return false;
	}

	ViewParams saved_view = g_diligent_state.view_params;
	g_diligent_state.view_params = sky_params;

	bool drew = false;
	const bool ok = diligent_draw_sky_objects(desc, sky_params, drew);
	g_diligent_state.view_params = saved_view;

	if (!ok)
	{
		return false;
	}

	if (drew && g_diligent_state.immediate_context)
	{
		auto* depth_target = diligent_get_active_depth_target();
		if (depth_target)
		{
			g_diligent_state.immediate_context->ClearDepthStencil(
				depth_target,
				Diligent::CLEAR_DEPTH_FLAG,
				1.0f,
				0,
				Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}
	}

	return true;
}

bool diligent_draw_sprite_instance(
	const ViewParams& params,
	SpriteInstance* instance,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	if (!instance)
	{
		return true;
	}

	auto* frame = instance->m_SpriteTracker.m_pCurFrame;
	if (!frame || !frame->m_pTex)
	{
		return true;
	}

	auto* render_texture = diligent_get_render_texture(frame->m_pTex, false);
	if (!render_texture || !render_texture->srv)
	{
		return true;
	}

	const float tex_width = static_cast<float>(render_texture->width);
	const float tex_height = static_cast<float>(render_texture->height);

	const uint32 color =
		(static_cast<uint32>(instance->m_ColorA) << 24) |
		(static_cast<uint32>(instance->m_ColorR) << 16) |
		(static_cast<uint32>(instance->m_ColorG) << 8) |
		static_cast<uint32>(instance->m_ColorB);

	DiligentSpriteVertex sprite_verts[4];
	const bool is_rotatable = (instance->m_Flags & FLAG_ROTATABLESPRITE) != 0;
	const bool really_close = (instance->m_Flags & FLAG_REALLYCLOSE) != 0;

	LTVector vPos = instance->GetPos();
	LTVector vRight;
	LTVector vUp;
	LTVector vForward;
	LTVector vBasisPos;
	LTVector basis_right;
	LTVector basis_up;
	LTVector basis_forward;
	float fZ = 0.0f;

	constexpr float kSpritePositionBias = -20.0f;
	constexpr float kSpriteMinFactorDist = 10.0f;
	constexpr float kSpriteMaxFactorDist = 500.0f;
	constexpr float kSpriteMinFactor = 0.1f;
	constexpr float kSpriteMaxFactor = 2.0f;

	if (is_rotatable)
	{
		LTMatrix rotation_matrix = diligent_build_transform(vPos, instance->m_Rotation, instance->m_Scale);
		rotation_matrix.GetBasisVectors(&vRight, &vUp, &vForward);
		static_cast<void>(vForward);
		vRight *= tex_width;
		vUp *= tex_height;
		vBasisPos = params.m_Pos;
	}
	else
	{
		if (really_close)
		{
			basis_right.Init(1.0f, 0.0f, 0.0f);
			basis_up.Init(0.0f, 1.0f, 0.0f);
			basis_forward.Init(0.0f, 0.0f, 1.0f);
			vBasisPos.Init(0.0f, 0.0f, 0.0f);
		}
		else
		{
			basis_right = params.m_Right;
			basis_up = params.m_Up;
			basis_forward = params.m_Forward;
			vBasisPos = params.m_Pos;
		}

		fZ = (vPos - vBasisPos).Dot(basis_forward);
		if (!really_close && fZ < NEARZ)
		{
			return true;
		}

		float size_x = tex_width * instance->m_Scale.x;
		float size_y = tex_height * instance->m_Scale.y;

		if (instance->m_Flags & FLAG_GLOWSPRITE)
		{
			float factor = (fZ - kSpriteMinFactorDist) / (kSpriteMaxFactorDist - kSpriteMinFactorDist);
			factor = LTCLAMP(factor, 0.0f, 1.0f);
			factor = kSpriteMinFactor + ((kSpriteMaxFactor - kSpriteMinFactor) * factor);
			size_x *= factor;
			size_y *= factor;
		}

		vRight = basis_right * size_x;
		vUp = basis_up * size_y;
	}

	sprite_verts[0].SetupVert(vPos + vUp - vRight, color, 0.0f, 0.0f);
	sprite_verts[1].SetupVert(vPos + vUp + vRight, color, 1.0f, 0.0f);
	sprite_verts[2].SetupVert(vPos + vRight - vUp, color, 1.0f, 1.0f);
	sprite_verts[3].SetupVert(vPos - vRight - vUp, color, 0.0f, 1.0f);

	DiligentSpriteVertex* points = sprite_verts;
	uint32 point_count = 4;
	DiligentSpriteVertex clipped_verts[45];

	if (instance->m_ClipperPoly != INVALID_HPOLY)
	{
		if (!diligent_clip_sprite(instance, instance->m_ClipperPoly, &points, &point_count, clipped_verts))
		{
			return true;
		}
	}

	if (point_count < 3)
	{
		return true;
	}

	if ((instance->m_Flags & FLAG_SPRITEBIAS) && !really_close)
	{
		if (is_rotatable)
		{
			for (uint32 i = 0; i < point_count; ++i)
			{
				LTVector& vPt = points[i].m_Vec;
				LTVector vPtRelCamera = vPt - params.m_Pos;
				float local_z = vPtRelCamera.Dot(params.m_Forward);

				if (local_z <= NEARZ)
				{
					continue;
				}

				float bias_dist = kSpritePositionBias;
				if ((local_z + bias_dist) < NEARZ)
				{
					bias_dist = NEARZ - local_z;
				}

				float scale = 1.0f + bias_dist / local_z;
				vPt = params.m_Right * vPtRelCamera.Dot(params.m_Right) * scale +
					params.m_Up * vPtRelCamera.Dot(params.m_Up) * scale +
					(local_z + bias_dist) * params.m_Forward + params.m_Pos;
			}
		}
		else if (fZ > 0.0f)
		{
			float bias_dist = kSpritePositionBias;
			if ((fZ + bias_dist) < NEARZ)
			{
				bias_dist = NEARZ - fZ;
			}

			float scale = 1.0f + bias_dist / fZ;
			for (uint32 i = 0; i < point_count; ++i)
			{
				LTVector& vPt = points[i].m_Vec;
				vPt = basis_right * (vPt - vBasisPos).Dot(basis_right) * scale +
					basis_up * (vPt - vBasisPos).Dot(basis_up) * scale +
					(fZ + bias_dist) * basis_forward + vBasisPos;
			}
		}
	}

	std::vector<DiligentWorldVertex> vertices;
	vertices.resize(point_count);
	for (uint32 i = 0; i < point_count; ++i)
	{
		auto& dst = vertices[i];
		dst.position[0] = points[i].m_Vec.x;
		dst.position[1] = points[i].m_Vec.y;
		dst.position[2] = points[i].m_Vec.z;
		diligent_set_world_vertex_color(dst, points[i].color);
		dst.uv0[0] = points[i].u;
		dst.uv0[1] = points[i].v;
		dst.uv1[0] = 0.0f;
		dst.uv1[1] = 0.0f;
		diligent_set_world_vertex_normal(dst, LTVector(0.0f, 0.0f, 1.0f));
	}

	std::vector<uint16> indices;
	indices.reserve((point_count - 2) * 3);
	for (uint32 i = 1; i + 1 < point_count; ++i)
	{
		indices.push_back(0);
		indices.push_back(static_cast<uint16>(i));
		indices.push_back(static_cast<uint16>(i + 1));
	}

	DiligentWorldConstants constants;
	LTMatrix world_matrix;
	world_matrix.Identity();
	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((instance->m_Flags & FLAG_FOGDISABLE) == 0);
	diligent_fill_world_constants(params, world_matrix, fog_enabled, fog_near, fog_far, constants);

	const DiligentWorldBlendMode blend_mode = diligent_get_object_blend_mode(instance);
	return diligent_draw_world_immediate(
		constants,
		blend_mode,
		depth_mode,
		render_texture->srv,
		nullptr,
		false,
		vertices.data(),
		static_cast<uint32>(vertices.size()),
		indices.data(),
		static_cast<uint32>(indices.size()));
}

bool diligent_draw_sprite_list(
	const std::vector<SpriteInstance*>& sprites,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	const std::vector<SpriteInstance*>* draw_list = &sprites;
	std::vector<SpriteInstance*> sorted;
	if (depth_mode == kWorldDepthEnabled && g_CV_DrawSorted.m_Val && sprites.size() > 1)
	{
		sorted = sprites;
		diligent_sort_translucent_list(sorted, params);
		draw_list = &sorted;
	}

	for (auto* sprite : *draw_list)
	{
		if (!diligent_draw_sprite_instance(params, sprite, fog_near, fog_far, depth_mode))
		{
			return false;
		}
	}

	return true;
}

uint32 diligent_pack_particle_color(const PSParticle* particle, const LTParticleSystem* system)
{
	if (!particle || !system)
	{
		return 0;
	}

	const float color_scale = 1.0f / 255.0f;
	const float scale_r = static_cast<float>(system->m_ColorR) * color_scale;
	const float scale_g = static_cast<float>(system->m_ColorG) * color_scale;
	const float scale_b = static_cast<float>(system->m_ColorB) * color_scale;
	const float alpha_scale = static_cast<float>(system->m_ColorA);

	const uint8 r = static_cast<uint8>(LTCLAMP(particle->m_Color.x * scale_r, 0.0f, 255.0f));
	const uint8 g = static_cast<uint8>(LTCLAMP(particle->m_Color.y * scale_g, 0.0f, 255.0f));
	const uint8 b = static_cast<uint8>(LTCLAMP(particle->m_Color.z * scale_b, 0.0f, 255.0f));
	const uint8 a = static_cast<uint8>(LTCLAMP(particle->m_Alpha * alpha_scale, 0.0f, 255.0f));

	return (static_cast<uint32>(a) << 24) |
		(static_cast<uint32>(r) << 16) |
		(static_cast<uint32>(g) << 8) |
		static_cast<uint32>(b);
}

const uint16* diligent_get_particle_indices()
{
	constexpr uint32 kParticleBatchSize = 64;
	constexpr uint32 kIndexCount = kParticleBatchSize * 6;
	static uint16 indices[kIndexCount];
	static bool initialized = false;

	if (!initialized)
	{
		initialized = true;
		uint16 cur_index = 0;
		uint16* out = indices;
		for (uint32 i = 0; i < kParticleBatchSize; ++i)
		{
			*out++ = cur_index++;
			*out++ = cur_index++;
			*out++ = cur_index++;
			*out++ = cur_index - 3;
			*out++ = cur_index - 1;
			*out++ = cur_index;
			++cur_index;
		}
	}

	return indices;
}

bool diligent_draw_particle_system_instance(
	const ViewParams& params,
	LTParticleSystem* system,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	if (!system || system->m_nParticles <= 0)
	{
		return true;
	}

	SharedTexture* texture = system->m_pCurTexture;
	Diligent::ITextureView* texture_view = texture ? diligent_get_texture_view(texture, false) : nullptr;
	if (!texture_view)
	{
		return true;
	}

	const bool object_space = (system->m_psFlags & PS_WORLDSPACE) == 0;
	const bool use_rotation = (system->m_psFlags & PS_USEROTATION) != 0;

	LTVector up;
	LTVector right;
	LTVector normal;
	if (system->m_Flags & FLAG_REALLYCLOSE)
	{
		up.Init(0.0f, 1.0f, 0.0f);
		right.Init(1.0f, 0.0f, 0.0f);
		normal.Init(0.0f, 0.0f, -1.0f);
	}
	else
	{
		up = params.m_Up;
		right = params.m_Right;
		normal = -params.m_Forward;
	}

	LTMatrix world_matrix;
	if (object_space)
	{
		world_matrix = diligent_build_transform(system->GetPos(), system->m_Rotation, system->m_Scale);
		LTMatrix inverse = world_matrix;
		inverse.Inverse();
		inverse.Apply3x3(up);
		inverse.Apply3x3(right);
		inverse.Apply3x3(normal);
	}
	else
	{
		world_matrix.Identity();
	}

	up *= 2.0f;
	right *= 2.0f;

	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((system->m_Flags & FLAG_FOGDISABLE) == 0);
	DiligentWorldConstants constants;
	diligent_fill_world_constants(params, world_matrix, fog_enabled, fog_near, fog_far, constants);

	const DiligentWorldBlendMode blend_mode = diligent_get_object_blend_mode(system);
	constexpr uint32 kParticleBatchSize = 64;
	const uint16* indices = diligent_get_particle_indices();

	PSParticle* particle = system->m_ParticleHead.m_pNext;
	int remaining = system->m_nParticles;
	while (remaining > 0 && particle != &system->m_ParticleHead)
	{
		const int batch_count = LTMIN(remaining, static_cast<int>(kParticleBatchSize));
		std::array<DiligentWorldVertex, kParticleBatchSize * 4> vertices{};

		for (int i = 0; i < batch_count; ++i)
		{
			const uint32 color = diligent_pack_particle_color(particle, system);

			LTVector basis_up = up;
			LTVector basis_right = right;
			if (use_rotation)
			{
				const float angle = particle->m_fAngle;
				basis_up = std::cos(angle) * up + std::sin(angle) * right;
				basis_right = basis_up.Cross(normal);
			}

			const LTVector offset_ul = basis_up - basis_right;
			const LTVector offset_ur = basis_up + basis_right;
			const LTVector offset_bl = -basis_up - basis_right;
			const LTVector offset_br = -basis_up + basis_right;

			const float size = particle->m_Size;
			const LTVector& pos = particle->m_Pos;

			auto& v0 = vertices[i * 4 + 0];
			v0.position[0] = pos.x + offset_ul.x * size;
			v0.position[1] = pos.y + offset_ul.y * size;
			v0.position[2] = pos.z + offset_ul.z * size;
			diligent_set_world_vertex_color(v0, color);
			v0.uv0[0] = 0.0f;
			v0.uv0[1] = 0.0f;
			diligent_set_world_vertex_normal(v0, normal);

			auto& v1 = vertices[i * 4 + 1];
			v1.position[0] = pos.x + offset_ur.x * size;
			v1.position[1] = pos.y + offset_ur.y * size;
			v1.position[2] = pos.z + offset_ur.z * size;
			diligent_set_world_vertex_color(v1, color);
			v1.uv0[0] = 1.0f;
			v1.uv0[1] = 0.0f;
			diligent_set_world_vertex_normal(v1, normal);

			auto& v2 = vertices[i * 4 + 2];
			v2.position[0] = pos.x + offset_br.x * size;
			v2.position[1] = pos.y + offset_br.y * size;
			v2.position[2] = pos.z + offset_br.z * size;
			diligent_set_world_vertex_color(v2, color);
			v2.uv0[0] = 1.0f;
			v2.uv0[1] = 1.0f;
			diligent_set_world_vertex_normal(v2, normal);

			auto& v3 = vertices[i * 4 + 3];
			v3.position[0] = pos.x + offset_bl.x * size;
			v3.position[1] = pos.y + offset_bl.y * size;
			v3.position[2] = pos.z + offset_bl.z * size;
			diligent_set_world_vertex_color(v3, color);
			v3.uv0[0] = 0.0f;
			v3.uv0[1] = 1.0f;
			diligent_set_world_vertex_normal(v3, normal);

			particle = particle->m_pNext;
			--remaining;
			if (particle == &system->m_ParticleHead)
			{
				break;
			}
		}

		const uint32 vertex_count = static_cast<uint32>(batch_count * 4);
		const uint32 index_count = static_cast<uint32>(batch_count * 6);
		if (!diligent_draw_world_immediate(
				constants,
				blend_mode,
				depth_mode,
				texture_view,
				nullptr,
				false,
				vertices.data(),
				vertex_count,
				indices,
				index_count))
		{
			return false;
		}
	}

	return true;
}

bool diligent_draw_particle_system_list(
	const std::vector<LTParticleSystem*>& systems,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	const std::vector<LTParticleSystem*>* draw_list = &systems;
	std::vector<LTParticleSystem*> sorted_systems;
	if (g_CV_DrawSorted.m_Val && systems.size() > 1)
	{
		sorted_systems = systems;
		diligent_sort_translucent_list(sorted_systems, params);
		draw_list = &sorted_systems;
	}

	for (auto* system : *draw_list)
	{
		if (!diligent_draw_particle_system_instance(params, system, fog_near, fog_far, depth_mode))
		{
			return false;
		}
	}

	return true;
}

struct DiligentDynamicParticleVertex
{
	float x;
	float y;
	float z;
	uint32 color;
	float u;
	float v;
};

struct DiligentDynamicParticleLightingData
{
	LTVector pos;
	uint32 alpha;
	LTVector acc;
};


struct DiligentVolumeEffectStaticLight
{
	LTVector pos;
	LTVector dir;
	LTVector color;
	LTVector att;
	float radius = 0.0f;
	float radius_sqr = 0.0f;
	float cos_fov = -1.0f;
	ELightAttenuationType attenuation = eAttenuation_Quartic;
	bool is_spot = false;
};

struct DiligentVolumeEffectStaticLightEntry
{
	DiligentVolumeEffectStaticLight light;
	float score = 0.0f;
};

struct DiligentVolumeEffectStaticLightCollector
{
	std::vector<DiligentVolumeEffectStaticLightEntry> entries;
	LTVector object_pos;
	uint32 max_lights = 0;
};

constexpr uint32 kMaxVolumeEffectStaticLights = 8;

LTVector diligent_calc_volume_effect_point_quartic_sample(
	const DiligentVolumeEffectStaticLight& light,
	const LTVector& pos)
{
	if (light.radius_sqr <= 0.0f)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	const LTVector delta = pos - light.pos;
	const float dist_sqr = delta.MagSqr();
	if (dist_sqr >= light.radius_sqr)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	const float dist_percent = 1.0f - (dist_sqr / light.radius_sqr);
	return light.color * (dist_percent * dist_percent);
}

LTVector diligent_calc_volume_effect_point_legacy_sample(
	const DiligentVolumeEffectStaticLight& light,
	const LTVector& pos)
{
	if (light.radius_sqr <= 0.0f)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	const LTVector delta = pos - light.pos;
	const float dist_sqr = delta.MagSqr();
	if (dist_sqr >= light.radius_sqr)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	const float dist = std::sqrt(dist_sqr);
	const float denom = light.att.x + light.att.y * dist + light.att.z * dist_sqr;
	if (denom <= 0.0f)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	return light.color / denom;
}

LTVector diligent_calc_volume_effect_spot_sample(
	const DiligentVolumeEffectStaticLight& light,
	const LTVector& pos)
{
	if (light.radius_sqr <= 0.0f)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	const LTVector delta = pos - light.pos;
	const float dist_sqr = delta.MagSqr();
	if (dist_sqr >= light.radius_sqr)
	{
		return LTVector(0.0f, 0.0f, 0.0f);
	}

	const float dist = std::sqrt(dist_sqr);
	if (dist <= 0.0f)
	{
		return light.color;
	}

	const float sample_dot = delta.Dot(light.dir) / dist;
	if (sample_dot > light.cos_fov)
	{
		const float denom = 1.0f - light.cos_fov;
		if (denom <= 0.0f)
		{
			return light.color;
		}

		return light.color * ((sample_dot - light.cos_fov) / denom);
	}

	return LTVector(0.0f, 0.0f, 0.0f);
}

LTVector diligent_calc_volume_effect_static_light_sample(
	const DiligentVolumeEffectStaticLight& light,
	const LTVector& pos)
{
	if (light.is_spot)
	{
		return diligent_calc_volume_effect_spot_sample(light, pos);
	}

	if (light.attenuation == eAttenuation_Quartic)
	{
		return diligent_calc_volume_effect_point_quartic_sample(light, pos);
	}

	return diligent_calc_volume_effect_point_legacy_sample(light, pos);
}

float diligent_calc_volume_effect_static_light_score(
	const DiligentVolumeEffectStaticLight& light,
	const LTVector& pos)
{
	const LTVector sample = diligent_calc_volume_effect_static_light_sample(light, pos);
	return LTMAX(sample.x, LTMAX(sample.y, sample.z));
}

void diligent_insert_volume_effect_static_light(
	DiligentVolumeEffectStaticLightCollector& collector,
	const DiligentVolumeEffectStaticLight& light,
	float score)
{
	if (collector.max_lights == 0 || score <= 0.0f)
	{
		return;
	}

	collector.entries.push_back({light, score});
	std::sort(collector.entries.begin(), collector.entries.end(),
		[](const DiligentVolumeEffectStaticLightEntry& a, const DiligentVolumeEffectStaticLightEntry& b)
		{
			return a.score > b.score;
		});

	if (collector.entries.size() > collector.max_lights)
	{
		collector.entries.resize(collector.max_lights);
	}
}

void diligent_collect_volume_effect_static_light_callback(WorldTreeObj* obj, void* user)
{
	if (!obj || obj->GetObjType() != WTObj_Light || !user)
	{
		return;
	}

	auto* collector = static_cast<DiligentVolumeEffectStaticLightCollector*>(user);
	const auto* light = static_cast<const StaticLight*>(obj);

	LTVector color = light->m_Color;
	if (light->m_pLightGroupColor)
	{
		color.x *= light->m_pLightGroupColor->x;
		color.y *= light->m_pLightGroupColor->y;
		color.z *= light->m_pLightGroupColor->z;
	}

	if (color.x < 1.0f && color.y < 1.0f && color.z < 1.0f)
	{
		return;
	}

	DiligentVolumeEffectStaticLight entry;
	entry.pos = light->m_Pos;
	entry.dir = light->m_Dir;
	entry.color = color * (1.0f - light->m_fConvertToAmbient);
	entry.att = light->m_AttCoefs;
	entry.radius = light->m_Radius;
	entry.radius_sqr = light->m_Radius * light->m_Radius;
	entry.cos_fov = light->m_FOV;
	entry.attenuation = light->m_eAttenuation;
	entry.is_spot = (light->m_FOV > -0.99f);

	const float score = diligent_calc_volume_effect_static_light_score(entry, collector->object_pos);
	diligent_insert_volume_effect_static_light(*collector, entry, score);
}

void diligent_collect_volume_effect_static_lights(
	const LTVector& object_pos,
	const LTVector& object_dims,
	std::vector<DiligentVolumeEffectStaticLight>& out)
{
	out.clear();

	if (!diligent_get_world_bsp_client() || !diligent_get_world_bsp_client()->ClientTree())
	{
		return;
	}

	DiligentVolumeEffectStaticLightCollector collector;
	collector.object_pos = object_pos;
	collector.max_lights = kMaxVolumeEffectStaticLights;

	FindObjInfo find_info;
	find_info.m_iObjArray = NOA_Lights;
	find_info.m_Min = object_pos - object_dims;
	find_info.m_Max = object_pos + object_dims;
	find_info.m_CB = &diligent_collect_volume_effect_static_light_callback;
	find_info.m_pCBUser = &collector;

	diligent_get_world_bsp_client()->ClientTree()->FindObjectsInBox2(&find_info);

	out.reserve(collector.entries.size());
	for (const auto& entry : collector.entries)
	{
		out.push_back(entry.light);
	}
}

const uint16* diligent_get_volume_effect_quad_indices()
{
	constexpr uint32 kVertCapacity = 6000;
	constexpr uint32 kQuadCount = kVertCapacity / 4;
	constexpr uint32 kIndexCount = kQuadCount * 6;
	static uint16 indices[kIndexCount];
	static bool initialized = false;

	if (!initialized)
	{
		initialized = true;
		uint16* out = indices;
		for (uint32 i = 0; i < kQuadCount; ++i)
		{
			const uint16 base = static_cast<uint16>(i * 4);
			*out++ = base;
			*out++ = base + 1;
			*out++ = base + 2;
			*out++ = base + 2;
			*out++ = base + 1;
			*out++ = base + 3;
		}
	}

	return indices;
}

bool diligent_draw_volume_effect_instance(
	const ViewParams& params,
	LTVolumeEffect* effect,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	if (!effect || effect->m_EffectType != VolumeEffectInfo::kDynamicParticles)
	{
		return true;
	}

	if (!effect->m_DPUpdateFn)
	{
		return true;
	}

	SharedTexture* texture = effect->m_DPTexture;
	Diligent::ITextureView* texture_view = texture ? diligent_get_texture_view(texture, false) : nullptr;
	if (!texture_view)
	{
		return true;
	}

	const bool render_quads = (effect->m_DPPrimitive == VolumeEffectInfo::kQuadlist);
	const bool saturate = effect->m_DPSaturate;
	constexpr uint32 kVertCapacity = 6000;
	std::array<DiligentDynamicParticleVertex, kVertCapacity> vb{};
	std::array<DiligentDynamicParticleLightingData, kVertCapacity / 3> lighting{};
	const bool use_lighting = (effect->m_DPLighting != VolumeEffectInfo::kNone);
	std::vector<DiligentVolumeEffectStaticLight> static_lights;

	uint32 total_filled = 0;
	bool done = false;

	LTMatrix world_matrix;
	world_matrix.Identity();
	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((effect->m_Flags & FLAG_FOGDISABLE) == 0);
	DiligentWorldConstants constants;
	diligent_fill_world_constants(params, world_matrix, fog_enabled, fog_near, fog_far, constants);

	const DiligentWorldBlendMode blend_mode = diligent_get_object_blend_mode(effect);
	const uint16* quad_indices = diligent_get_volume_effect_quad_indices();

	if (use_lighting && effect->m_DPLighting == VolumeEffectInfo::kSinglePointNonDirectional)
	{
		diligent_collect_volume_effect_static_lights(effect->GetPos(), effect->GetDims(), static_lights);
	}

	while (!done)
	{
		uint32 cur_filled = 0;
		void* lighting_ptr = use_lighting ? lighting.data() : nullptr;
		done = effect->m_DPUpdateFn(effect->m_DPUserData, vb.data(), lighting_ptr, kVertCapacity, total_filled, cur_filled);
		if (cur_filled == 0)
		{
			break;
		}

		std::vector<DiligentWorldVertex> vertices;
		vertices.resize(cur_filled);
		for (uint32 i = 0; i < cur_filled; ++i)
		{
			const auto& src = vb[i];
			auto& dst = vertices[i];
			dst.position[0] = src.x;
			dst.position[1] = src.y;
			dst.position[2] = src.z;
			diligent_set_world_vertex_color(dst, src.color);
			dst.uv0[0] = src.u;
			dst.uv0[1] = src.v;
			dst.uv1[0] = 0.0f;
			dst.uv1[1] = 0.0f;
			diligent_set_world_vertex_normal(dst, LTVector(0.0f, 0.0f, 1.0f));
		}

		if (use_lighting && effect->m_DPLighting == VolumeEffectInfo::kSinglePointNonDirectional)
		{
			const uint32 sample_count = render_quads ? (cur_filled / 4) : (cur_filled / 3);
			for (uint32 sample = 0; sample < sample_count; ++sample)
			{
				auto& light = lighting[sample];
				if (auto* world_bsp_shared = diligent_get_world_bsp_shared())
				{
					LTRGBColor ambient{};
					w_DoLightLookup(world_bsp_shared->LightTable(), &light.pos, &ambient);
					light.acc.x += ambient.rgb.r;
					light.acc.y += ambient.rgb.g;
					light.acc.z += ambient.rgb.b;
				}

				for (uint32 light_index = 0; light_index < g_diligent_num_world_dynamic_lights; ++light_index)
				{
					const DynamicLight* dynamic = g_diligent_world_dynamic_lights[light_index];
					if (!dynamic || (dynamic->m_Flags & FLAG_ONLYLIGHTWORLD))
					{
						continue;
					}

					LTVector delta = light.pos - dynamic->m_Pos;
					const float dist = delta.Mag();
					const float radius = dynamic->m_LightRadius;
					if (radius <= 0.0f || dist >= radius)
					{
						continue;
					}

					const float atten = LTCLAMP(1.0f - (dist / radius), 0.0f, 1.0f);
					const float intensity = LTMAX(dynamic->m_Intensity, 0.0f);
					const float scale = atten * intensity;
					light.acc.x += static_cast<float>(dynamic->m_ColorR) * scale;
					light.acc.y += static_cast<float>(dynamic->m_ColorG) * scale;
					light.acc.z += static_cast<float>(dynamic->m_ColorB) * scale;
				}

				for (const auto& static_light : static_lights)
				{
					light.acc += diligent_calc_volume_effect_static_light_sample(static_light, light.pos);
				}
			}

			for (uint32 sample = 0; sample < sample_count; ++sample)
			{
				const auto& light = lighting[sample];
				uint32 argb = light.alpha;
				uint8 r = static_cast<uint8>(LTCLAMP(light.acc.x, 0.0f, 255.0f));
				uint8 g = static_cast<uint8>(LTCLAMP(light.acc.y, 0.0f, 255.0f));
				uint8 b = static_cast<uint8>(LTCLAMP(light.acc.z, 0.0f, 255.0f));
				argb |= (static_cast<uint32>(r) << 16) | (static_cast<uint32>(g) << 8) | static_cast<uint32>(b);

				const uint32 base = render_quads ? (sample * 4) : (sample * 3);
				const uint32 count = render_quads ? 4u : 3u;
				for (uint32 i = 0; i < count; ++i)
				{
					diligent_set_world_vertex_color(vertices[base + i], argb);
				}
			}
		}

		if (saturate)
		{
			for (uint32 i = 0; i < cur_filled; ++i)
			{
				auto& color = vertices[i].color;
				color[0] = LTMIN(color[0] * 2.0f, 1.0f);
				color[1] = LTMIN(color[1] * 2.0f, 1.0f);
				color[2] = LTMIN(color[2] * 2.0f, 1.0f);
			}
		}

		if (!render_quads)
		{
			const uint32 tri_count = cur_filled / 3;
			const uint32 index_count = tri_count * 3;
			std::vector<uint16> indices;
			indices.resize(index_count);
			for (uint32 i = 0; i < index_count; ++i)
			{
				indices[i] = static_cast<uint16>(i);
			}

			if (!diligent_draw_world_immediate_mode(
					constants,
					blend_mode,
					depth_mode,
					kWorldPipelineVolumeEffect,
					texture_view,
					vertices.data(),
					cur_filled,
					indices.data(),
					index_count))
			{
				return false;
			}
		}
		else
		{
			const uint32 quad_count = cur_filled / 4;
			const uint32 index_count = quad_count * 6;
			if (!diligent_draw_world_immediate_mode(
					constants,
					blend_mode,
					depth_mode,
					kWorldPipelineVolumeEffect,
					texture_view,
					vertices.data(),
					cur_filled,
					quad_indices,
					index_count))
			{
				return false;
			}
		}

		total_filled += cur_filled;
	}

	return true;
}

bool diligent_draw_volume_effect_list(
	const std::vector<LTVolumeEffect*>& effects,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode,
	bool sort)
{
	const std::vector<LTVolumeEffect*>* draw_list = &effects;
	std::vector<LTVolumeEffect*> sorted;
	if (sort && g_CV_DrawSorted.m_Val && effects.size() > 1)
	{
		sorted = effects;
		diligent_sort_translucent_list(sorted, params);
		draw_list = &sorted;
	}

	for (auto* effect : *draw_list)
	{
		if (!diligent_draw_volume_effect_instance(params, effect, fog_near, fog_far, depth_mode))
		{
			return false;
		}
	}

	return true;
}

bool diligent_draw_line_system_instance(
	const ViewParams& params,
	LineSystem* system,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	if (!system)
	{
		return true;
	}

	LSLine* line = system->m_LineHead.m_pNext;
	if (line == &system->m_LineHead)
	{
		return true;
	}

	const float alpha_scale = static_cast<float>(system->m_ColorA);
	std::vector<DiligentWorldVertex> vertices;
	while (line != &system->m_LineHead)
	{
		for (uint32 i = 0; i < 2; ++i)
		{
			DiligentWorldVertex vertex{};
			vertex.position[0] = line->m_Points[i].m_Pos.x;
			vertex.position[1] = line->m_Points[i].m_Pos.y;
			vertex.position[2] = line->m_Points[i].m_Pos.z;
			diligent_set_world_vertex_color(vertex, diligent_pack_line_color(line->m_Points[i], alpha_scale));
			vertex.uv0[0] = 0.0f;
			vertex.uv0[1] = 0.0f;
			vertex.uv1[0] = 0.0f;
			vertex.uv1[1] = 0.0f;
			diligent_set_world_vertex_normal(vertex, LTVector(0.0f, 0.0f, 1.0f));
			vertices.push_back(vertex);
		}

		line = line->m_pNext;
	}

	if (vertices.empty())
	{
		return true;
	}

	LTMatrix world_matrix = diligent_build_transform(system->GetPos(), system->m_Rotation, system->m_Scale);
	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((system->m_Flags & FLAG_FOGDISABLE) == 0);
	DiligentWorldConstants constants;
	diligent_fill_world_constants(params, world_matrix, fog_enabled, fog_near, fog_far, constants);

	const DiligentWorldBlendMode blend_mode = diligent_get_object_blend_mode(system);
	return diligent_draw_line_vertices(
		constants,
		blend_mode,
		depth_mode,
		vertices.data(),
		static_cast<uint32>(vertices.size()));
}

bool diligent_draw_line_system_list(
	const std::vector<LineSystem*>& systems,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	const std::vector<LineSystem*>* draw_list = &systems;
	std::vector<LineSystem*> sorted_systems;
	if (g_CV_DrawSorted.m_Val && systems.size() > 1)
	{
		sorted_systems = systems;
		diligent_sort_translucent_list(sorted_systems, params);
		draw_list = &sorted_systems;
	}

	for (auto* system : *draw_list)
	{
		if (!diligent_draw_line_system_instance(params, system, fog_near, fog_far, depth_mode))
		{
			return false;
		}
	}

	return true;
}

static LTVector diligent_compute_polygrid_normal(
	const LTPolyGrid* grid,
	int32 x_index,
	int32 y_index,
	float spacing_x,
	float spacing_z,
	float y_scale)
{
	const int32 width = static_cast<int32>(grid->m_Width);
	const int32 height = static_cast<int32>(grid->m_Height);
	const int8* data = reinterpret_cast<const int8*>(grid->m_Data);

	const int32 x0 = x_index > 0 ? x_index - 1 : x_index;
	const int32 x1 = x_index + 1 < width ? x_index + 1 : x_index;
	const int32 y0 = y_index > 0 ? y_index - 1 : y_index;
	const int32 y1 = y_index + 1 < height ? y_index + 1 : y_index;

	const float left = static_cast<float>(data[y_index * width + x0]);
	const float right = static_cast<float>(data[y_index * width + x1]);
	const float up = static_cast<float>(data[y0 * width + x_index]);
	const float down = static_cast<float>(data[y1 * width + x_index]);

	LTVector normal(
		(left - right) * y_scale * spacing_z,
		spacing_x * spacing_z,
		(up - down) * y_scale * spacing_x);
	normal.Normalize();
	return normal;
}

static void diligent_compute_envmap_uv(
	const ViewParams& params,
	const LTMatrix& world_matrix,
	const LTVector& local_pos,
	const LTVector& local_normal,
	float& u_out,
	float& v_out)
{
	LTVector world_pos = local_pos;
	world_matrix.Apply(world_pos);

	LTVector world_normal = local_normal;
	world_matrix.Apply3x3(world_normal);
	world_normal.Normalize();

	LTVector view_dir = params.m_Pos - world_pos;
	const float view_mag = view_dir.Mag();
	if (view_mag > 0.0001f)
	{
		view_dir /= view_mag;
	}

	LTVector incident = -view_dir;
	const float dot_val = incident.Dot(world_normal);
	LTVector reflection = incident - (2.0f * dot_val) * world_normal;
	params.m_mView.Apply3x3(reflection);
	const float refl_mag = reflection.Mag();
	if (refl_mag > 0.0001f)
	{
		reflection /= refl_mag;
	}

	u_out = 0.5f + 0.5f * reflection.x;
	v_out = 0.5f - 0.5f * reflection.y;
}

bool diligent_draw_polygrid_instance(
	const ViewParams& params,
	LTPolyGrid* grid,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	if (!grid || !grid->m_Data || !grid->m_Indices || grid->m_nIndices == 0)
	{
		return true;
	}

	if (grid->m_Width < 2 || grid->m_Height < 2)
	{
		return true;
	}

	const bool wants_env_map = (grid->m_pEnvMap && g_CV_EnvMapPolyGrids.m_Val != 0);
	const bool wants_bump_map = wants_env_map && (grid->m_nPGFlags & PG_NORMALMAPSPRITE) && (g_CV_BumpMapPolyGrids.m_Val != 0);

	SharedTexture* texture = nullptr;
	if (grid->m_pSprite && grid->m_SpriteTracker.m_pCurFrame)
	{
		texture = grid->m_SpriteTracker.m_pCurFrame->m_pTex;
	}

	Diligent::ITextureView* texture_view = texture ? diligent_get_texture_view(texture, false) : nullptr;
	Diligent::ITextureView* env_map_view = wants_env_map ? diligent_get_texture_view(grid->m_pEnvMap, false) : nullptr;
	Diligent::ITextureView* base_view = texture_view;
	const bool use_env_map = (base_view && env_map_view);
	const bool use_bump = wants_bump_map && texture_view && env_map_view;

	const float half_width = (static_cast<float>(grid->m_Width) - 1.0f) * 0.5f;
	const float half_height = (static_cast<float>(grid->m_Height) - 1.0f) * 0.5f;

	const float x_inc = grid->GetDims().x * 2.0f / (grid->m_Width - 1);
	const float z_inc = grid->GetDims().z * 2.0f / (grid->m_Height - 1);
	const float y_scale = grid->GetDims().y / 127.0f;
	const float spacing_x = x_inc * 2.0f;
	const float spacing_z = z_inc * 2.0f;

	const float x_start = -half_width * x_inc;
	float curr_x = x_start;
	float curr_z = -half_height * z_inc;

	int8* data_pos = reinterpret_cast<int8*>(grid->m_Data);
	int8* data_end = data_pos + grid->m_Width * grid->m_Height;
	int8* line_end = data_pos + grid->m_Width;

	const float scaled_r = static_cast<float>(grid->m_ColorR) / 255.0f;
	const float scaled_g = static_cast<float>(grid->m_ColorG) / 255.0f;
	const float scaled_b = static_cast<float>(grid->m_ColorB) / 255.0f;
	const float alpha = static_cast<float>(grid->m_ColorA);

	uint32 color_table[256];
	for (uint32 i = 0; i < 256; ++i)
	{
		const auto& base_color = grid->m_ColorTable[i];
		const uint8 r = static_cast<uint8>(LTCLAMP(base_color.x * scaled_r, 0.0f, 255.0f));
		const uint8 g = static_cast<uint8>(LTCLAMP(base_color.y * scaled_g, 0.0f, 255.0f));
		const uint8 b = static_cast<uint8>(LTCLAMP(base_color.z * scaled_b, 0.0f, 255.0f));
		const uint8 a = static_cast<uint8>(LTCLAMP(alpha, 0.0f, 255.0f));
		color_table[i] = (static_cast<uint32>(a) << 24) |
			(static_cast<uint32>(r) << 16) |
			(static_cast<uint32>(g) << 8) |
			static_cast<uint32>(b);
	}

	uint32* color_lookup = color_table + 128;

	const float x_scale = grid->m_xScale / ((grid->m_Width - 1) * x_inc);
	const float z_scale = grid->m_yScale / ((grid->m_Height - 1) * z_inc);

	float uv_scale = 1.0f;
	if (use_bump)
	{
		auto* render_texture = diligent_get_render_texture(texture, false);
		if (render_texture && render_texture->texture_data)
		{
			uv_scale = render_texture->texture_data->m_Header.GetDetailTextureScale();
		}
	}

	const float start_u = static_cast<float>(fmod(grid->m_xPan * uv_scale, 1.0f));
	const float start_v = static_cast<float>(fmod(grid->m_yPan * uv_scale, 1.0f));

	float curr_u = start_u;
	float curr_v = start_v;
	const float u_inc = x_inc * x_scale * uv_scale;
	const float v_inc = z_inc * z_scale * uv_scale;

	LTMatrix world_matrix = diligent_build_transform(grid->GetPos(), grid->m_Rotation, LTVector(1.0f, 1.0f, 1.0f));
	std::vector<DiligentWorldVertex> vertices;
	vertices.reserve(static_cast<size_t>(grid->m_Width) * grid->m_Height);

	int32 x_index = 0;
	int32 y_index = 0;

	if (grid->m_pValidMask)
	{
		uint32* curr_mask = grid->m_pValidMask;
		const uint32 mask_line_adjust = (grid->m_Width % 32) ? 1 : 0;
		uint32 shift = 1;

		while (data_pos < data_end)
		{
			shift = 1;
			while (data_pos < line_end)
			{
				if (*curr_mask & shift)
				{
					DiligentWorldVertex vertex{};
					vertex.position[0] = curr_x;
					const float height_value = static_cast<float>(*data_pos) * y_scale;
					vertex.position[1] = height_value;
					vertex.position[2] = curr_z;
					diligent_set_world_vertex_color(vertex, color_lookup[*data_pos]);
					vertex.uv0[0] = curr_u;
					vertex.uv0[1] = curr_v;
					const LTVector local_pos(curr_x, height_value, curr_z);
					const LTVector local_normal = diligent_compute_polygrid_normal(grid, x_index, y_index, spacing_x, spacing_z, y_scale);
					diligent_set_world_vertex_normal(vertex, local_normal);
					if (use_env_map)
					{
						diligent_compute_envmap_uv(params, world_matrix, local_pos, local_normal, vertex.uv1[0], vertex.uv1[1]);
					}
					else
					{
						vertex.uv1[0] = 0.0f;
						vertex.uv1[1] = 0.0f;
					}
					vertices.push_back(vertex);
				}

				++data_pos;
				curr_x += x_inc;
				curr_u += u_inc;
				++x_index;

				if (shift == 0x80000000)
				{
					++curr_mask;
					shift = 1;
				}
				else
				{
					shift <<= 1;
				}
			}

			curr_x = x_start;
			line_end += grid->m_Width;
			curr_mask += mask_line_adjust;
			curr_z += z_inc;
			curr_u = start_u;
			curr_v += v_inc;
			x_index = 0;
			++y_index;
		}
	}
	else
	{
		while (data_pos < data_end)
		{
			while (data_pos < line_end)
			{
				DiligentWorldVertex vertex{};
				vertex.position[0] = curr_x;
				const float height_value = static_cast<float>(*data_pos) * y_scale;
				vertex.position[1] = height_value;
				vertex.position[2] = curr_z;
				diligent_set_world_vertex_color(vertex, color_lookup[*data_pos]);
				vertex.uv0[0] = curr_u;
				vertex.uv0[1] = curr_v;
				const LTVector local_pos(curr_x, height_value, curr_z);
				const LTVector local_normal = diligent_compute_polygrid_normal(grid, x_index, y_index, spacing_x, spacing_z, y_scale);
				diligent_set_world_vertex_normal(vertex, local_normal);
				if (use_env_map)
				{
					diligent_compute_envmap_uv(params, world_matrix, local_pos, local_normal, vertex.uv1[0], vertex.uv1[1]);
				}
				else
				{
					vertex.uv1[0] = 0.0f;
					vertex.uv1[1] = 0.0f;
				}
				vertices.push_back(vertex);

				++data_pos;
				curr_x += x_inc;
				curr_u += u_inc;
				++x_index;
			}

			curr_x = x_start;
			line_end += grid->m_Width;
			curr_z += z_inc;
			curr_u = start_u;
			curr_v += v_inc;
			x_index = 0;
			++y_index;
		}
	}

	if (vertices.empty())
	{
		return true;
	}

	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((grid->m_Flags & FLAG_FOGDISABLE) == 0);
	DiligentWorldConstants constants;
	diligent_fill_world_constants(params, world_matrix, fog_enabled, fog_near, fog_far, constants);
	if (use_bump)
	{
		constants.dynamic_light_pos[0] = g_CV_EnvBumpMapScale.m_Val;
	}

	const DiligentWorldBlendMode blend_mode = diligent_get_object_blend_mode(grid);
	return diligent_draw_world_immediate(
		constants,
		blend_mode,
		depth_mode,
		base_view,
		use_env_map ? env_map_view : nullptr,
		use_bump,
		vertices.data(),
		static_cast<uint32>(vertices.size()),
		grid->m_Indices,
		grid->m_nIndices);
}

bool diligent_draw_polygrid_list(
	const std::vector<LTPolyGrid*>& grids,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode,
	bool sort)
{
	const std::vector<LTPolyGrid*>* draw_list = &grids;
	std::vector<LTPolyGrid*> sorted_grids;
	if (sort)
	{
		sorted_grids = grids;
		diligent_sort_translucent_list(sorted_grids, params);
		draw_list = &sorted_grids;
	}

	for (auto* grid : *draw_list)
	{
		if (!diligent_draw_polygrid_instance(params, grid, fog_near, fog_far, depth_mode))
		{
			return false;
		}
	}

	return true;
}

bool diligent_should_process_sprite(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_SPRITE)
	{
		return false;
	}

	if (!g_CV_DrawSprites.m_Val)
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

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_sprite_object(LTObject* object)
{
	if (!diligent_should_process_sprite(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	auto* instance = object->ToSprite();
	if (!instance)
	{
		return;
	}

	if (object->m_Flags & FLAG_SPRITE_NOZ)
	{
		g_diligent_noz_sprites.push_back(instance);
	}
	else
	{
		g_diligent_translucent_sprites.push_back(instance);
	}
}

bool diligent_should_process_line_system(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_LINESYSTEM)
	{
		return false;
	}

	if (!g_CV_DrawLineSystems.m_Val)
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

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_line_system_object(LTObject* object)
{
	if (!diligent_should_process_line_system(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	auto* system = object->ToLineSystem();
	if (!system)
	{
		return;
	}

	g_diligent_line_systems.push_back(system);
}

bool diligent_should_process_particle_system(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_PARTICLESYSTEM)
	{
		return false;
	}

	if (!g_CV_DrawParticles.m_Val)
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

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_particle_system_object(LTObject* object)
{
	if (!diligent_should_process_particle_system(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	auto* system = object->ToParticleSystem();
	if (!system)
	{
		return;
	}

	g_diligent_particle_systems.push_back(system);
}

bool diligent_should_process_volume_effect(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_VOLUMEEFFECT)
	{
		return false;
	}

	if (!g_CV_DrawVolumeEffects.m_Val)
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

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_volume_effect_object(LTObject* object)
{
	if (!diligent_should_process_volume_effect(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	auto* effect = object->ToVolumeEffect();
	if (!effect)
	{
		return;
	}

	if (object->m_Flags2 & FLAG2_FORCETRANSLUCENT)
	{
		g_diligent_translucent_volume_effects.push_back(effect);
	}
	else
	{
		g_diligent_solid_volume_effects.push_back(effect);
	}
}

bool diligent_should_process_polygrid(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_POLYGRID)
	{
		return false;
	}

	if (!g_CV_DrawPolyGrids.m_Val)
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

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_polygrid_object(LTObject* object)
{
	if (!diligent_should_process_polygrid(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	auto* grid = object->ToPolyGrid();
	if (!grid)
	{
		return;
	}

	if (object->IsTranslucent())
	{
		if (grid->m_nPGFlags & PG_RENDERBEFORETRANSLUCENTS)
		{
			g_diligent_early_translucent_polygrids.push_back(grid);
		}
		else
		{
			g_diligent_translucent_polygrids.push_back(grid);
		}
	}
	else
	{
		g_diligent_solid_polygrids.push_back(grid);
	}
}

bool diligent_should_process_canvas(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_CANVAS)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if ((object->cd.m_ClientFlags & CF_SOLIDCANVAS) == 0)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

bool diligent_should_process_canvas_main(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_CANVAS)
	{
		return false;
	}

	if (!g_CV_DrawCanvases.m_Val)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_state.object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_canvas_object(LTObject* object)
{
	if (!diligent_should_process_canvas(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	Canvas* canvas = object->ToCanvas();
	if (canvas && canvas->m_Fn)
	{
		canvas->m_Fn(canvas, canvas->m_pFnUserData);
	}
}

void diligent_process_canvas_object_main(LTObject* object)
{
	if (!diligent_should_process_canvas_main(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_state.object_frame_code;

	auto* canvas = object->ToCanvas();
	if (!canvas)
	{
		return;
	}

	if (canvas->cd.m_ClientFlags & CF_SOLIDCANVAS)
	{
		g_diligent_solid_canvases.push_back(canvas);
	}
	else
	{
		g_diligent_translucent_canvases.push_back(canvas);
	}
}

void diligent_filter_world_node_for_sprites(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_SPRITE)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_sprite_object(object);
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

		diligent_filter_world_node_for_sprites(params, child);
	}
}

void diligent_filter_world_node_for_line_systems(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_LINESYSTEM)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_line_system_object(object);
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

		diligent_filter_world_node_for_line_systems(params, child);
	}
}

void diligent_filter_world_node_for_particle_systems(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_PARTICLESYSTEM)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_particle_system_object(object);
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

		diligent_filter_world_node_for_particle_systems(params, child);
	}
}

void diligent_filter_world_node_for_volume_effects(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_VOLUMEEFFECT)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_volume_effect_object(object);
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

		diligent_filter_world_node_for_volume_effects(params, child);
	}
}

void diligent_filter_world_node_for_polygrids(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_POLYGRID)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_polygrid_object(object);
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

		diligent_filter_world_node_for_polygrids(params, child);
	}
}

void diligent_filter_world_node_for_canvases(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_CANVAS)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_canvas_object(object);
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

		diligent_filter_world_node_for_canvases(params, child);
	}
}

void diligent_filter_world_node_for_canvases_main(const ViewParams& params, WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_CANVAS)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_canvas_object_main(object);
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

		diligent_filter_world_node_for_canvases_main(params, child);
	}
}

void diligent_collect_sprites(SceneDesc* desc)
{
	g_diligent_translucent_sprites.clear();
	g_diligent_noz_sprites.clear();

	if (!desc || !g_CV_DrawSprites.m_Val)
	{
		return;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_sprite_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !diligent_get_world_bsp_client())
	{
		return;
	}

	WorldTree* tree = diligent_get_world_bsp_client()->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_sprites(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_sprite_object(object);
	}
}

void diligent_collect_line_systems(SceneDesc* desc)
{
	g_diligent_line_systems.clear();

	if (!desc || !g_CV_DrawLineSystems.m_Val)
	{
		return;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_line_system_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !diligent_get_world_bsp_client())
	{
		return;
	}

	WorldTree* tree = diligent_get_world_bsp_client()->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_line_systems(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_line_system_object(object);
	}
}

void diligent_collect_particle_systems(SceneDesc* desc)
{
	g_diligent_particle_systems.clear();

	if (!desc || !g_CV_DrawParticles.m_Val)
	{
		return;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_particle_system_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !diligent_get_world_bsp_client())
	{
		return;
	}

	WorldTree* tree = diligent_get_world_bsp_client()->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_particle_systems(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_particle_system_object(object);
	}
}

void diligent_collect_volume_effects(SceneDesc* desc)
{
	g_diligent_solid_volume_effects.clear();
	g_diligent_translucent_volume_effects.clear();

	if (!desc || !g_CV_DrawVolumeEffects.m_Val)
	{
		return;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_volume_effect_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !diligent_get_world_bsp_client())
	{
		return;
	}

	WorldTree* tree = diligent_get_world_bsp_client()->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_volume_effects(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_volume_effect_object(object);
	}
}

void diligent_collect_polygrids(SceneDesc* desc)
{
	g_diligent_solid_polygrids.clear();
	g_diligent_early_translucent_polygrids.clear();
	g_diligent_translucent_polygrids.clear();

	if (!desc || !g_CV_DrawPolyGrids.m_Val)
	{
		return;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_polygrid_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !diligent_get_world_bsp_client())
	{
		return;
	}

	WorldTree* tree = diligent_get_world_bsp_client()->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_polygrids(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_polygrid_object(object);
	}
}

bool diligent_draw_solid_canvases_glow(SceneDesc* desc)
{
	if (!desc)
	{
		return true;
	}

	if (!g_CV_DrawCanvases.m_Val || !g_CV_ScreenGlow_DrawCanvases.m_Val)
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

			diligent_process_canvas_object(object);
		}

		return true;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	if (!diligent_get_world_bsp_client())
	{
		return true;
	}

	WorldTree* tree = diligent_get_world_bsp_client()->ClientTree();
	if (!tree)
	{
		return true;
	}

	diligent_filter_world_node_for_canvases(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_canvas_object(object);
	}

	return true;
}

bool diligent_draw_canvas_list(const std::vector<Canvas*>& canvases, bool sort, const ViewParams& params)
{
	const std::vector<Canvas*>* draw_list = &canvases;
	std::vector<Canvas*> sorted_canvases;
	if (sort && g_CV_DrawSorted.m_Val && canvases.size() > 1)
	{
		sorted_canvases = canvases;
		diligent_sort_translucent_list(sorted_canvases, params);
		draw_list = &sorted_canvases;
	}

	for (auto* canvas : *draw_list)
	{
		if (canvas && canvas->m_Fn)
		{
			canvas->m_Fn(canvas, canvas->m_pFnUserData);
		}
	}

	return true;
}

void diligent_collect_canvases(SceneDesc* desc)
{
	g_diligent_solid_canvases.clear();
	g_diligent_translucent_canvases.clear();

	if (!desc || !g_CV_DrawCanvases.m_Val)
	{
		return;
	}

	if (g_diligent_state.render_struct && g_diligent_state.render_struct->IncObjectFrameCode)
	{
		g_diligent_state.object_frame_code = g_diligent_state.render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_state.object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_canvas_object_main(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !diligent_get_world_bsp_client())
	{
		return;
	}

	WorldTree* tree = diligent_get_world_bsp_client()->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_canvases_main(g_diligent_state.view_params, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_canvas_object_main(object);
	}
}
