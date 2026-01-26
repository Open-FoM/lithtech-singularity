#include "diligent_scene_render.h"

#include "diligent_debug_draw.h"
#include "diligent_device.h"
#include "diligent_render.h"
#include "diligent_state.h"
#include "diligent_object_draw.h"
#include "diligent_postfx.h"
#include "diligent_scene_collect.h"
#include "diligent_shadow_draw.h"
#include "diligent_world_draw.h"
#include "ltvector.h"
#include "renderstruct.h"
#include "viewparams.h"
#include "../rendererconsolevars.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"

/// \brief Main per-frame renderer entry point for scene rendering.
/// \details Performs view setup, visibility collection, world/model draws,
///          and post effects (e.g., SSAO, glow). Returns a RENDER_* status.
int diligent_RenderScene(SceneDesc* desc)
{
	if (!desc)
	{
		return RENDER_ERROR;
	}

	if (g_diligent_state.is_in_optimized_2d)
	{
		return RENDER_OK;
	}

	++g_diligent_state.frame_counter;

	if (!diligent_EnsureSwapChain())
	{
		return RENDER_ERROR;
	}

	DiligentAaContext aa_ctx{};
	const bool aa_active = diligent_begin_antialiasing(desc, aa_ctx);
	struct DiligentAaScope
	{
		DiligentAaContext& ctx;
		bool active;
		~DiligentAaScope()
		{
			if (active)
			{
				diligent_end_antialiasing(ctx);
			}
		}
		void Dismiss()
		{
			active = false;
		}
	} aa_scope{aa_ctx, aa_active};

	constexpr float kNearZ = 7.0f;
	constexpr float kFarZ = 10000.0f;

	const float left = static_cast<float>(desc->m_Rect.left);
	const float top = static_cast<float>(desc->m_Rect.top);
	const float right = static_cast<float>(desc->m_Rect.right);
	const float bottom = static_cast<float>(desc->m_Rect.bottom);

	if (!lt_InitViewFrustum(
			&g_diligent_state.view_params,
			desc->m_xFov,
			desc->m_yFov,
			kNearZ,
			kFarZ,
			left,
			top,
			right - 0.1f,
			bottom - 0.1f,
			&desc->m_Pos,
			&desc->m_Rotation,
			ViewParams::eRenderMode_Normal))
	{
		return RENDER_ERROR;
	}

	if (g_diligent_state.immediate_context)
	{
		Diligent::Viewport viewport;
		viewport.TopLeftX = left;
		viewport.TopLeftY = top;
		viewport.Width = right - left;
		viewport.Height = bottom - top;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		g_diligent_state.immediate_context->SetViewports(1, &viewport, 0, 0);
	}

	const uint32 msaa_samples = diligent_get_msaa_samples();
	const bool ssao_enabled = g_CV_SSAOEnable.m_Val != 0;
	const bool use_ssao_fx = ssao_enabled && g_CV_SSAOBackend.m_Val != 0;
	const bool ssao_msaa = (msaa_samples > 0 && ssao_enabled);
	DiligentSsaoContext ssao_ctx{};
	const bool ssao_active = (!use_ssao_fx && msaa_samples == 0) ? diligent_begin_ssao(desc, ssao_ctx) : false;
	bool ssao_fx_active = false;
	if (aa_active && ssao_msaa)
	{
		aa_ctx.defer_copy = true;
	}

	diligent_collect_visible_render_blocks(g_diligent_state.view_params);

	const bool debug_lines_enabled = diligent_debug_lines_enabled();
	if (debug_lines_enabled)
	{
		diligent_debug_clear_lines();
		diligent_debug_add_world_lines();
	}

	diligent_collect_scene_dynamic_lights(desc, g_diligent_state.view_params);

	if (g_CV_ModelShadow_Proj_Enable.m_Val)
	{
		diligent_update_shadow_texture_list();
	}

	if (!diligent_draw_sky(desc))
	{
		return RENDER_ERROR;
	}

	diligent_collect_world_models(desc);

	if (use_ssao_fx)
	{
		ssao_fx_active = diligent_prepare_ssao_fx(desc);
	}

	diligent_collect_polygrids(desc);
	diligent_collect_sprites(desc);
	diligent_collect_line_systems(desc);
	diligent_collect_particle_systems(desc);
	diligent_collect_canvases(desc);
	diligent_collect_volume_effects(desc);

	if (!diligent_draw_world_blocks())
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_world_model_list(g_diligent_solid_world_models, kWorldBlendSolid))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_polygrid_list(
			g_diligent_solid_polygrids,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled,
			false))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_volume_effect_list(
			g_diligent_solid_volume_effects,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled,
			false))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_canvas_list(g_diligent_solid_canvases, false, g_diligent_state.view_params))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_models(desc))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_polygrid_list(
			g_diligent_early_translucent_polygrids,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled,
			false))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_world_model_list(g_diligent_translucent_world_models, kWorldBlendAlpha))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_translucent_world_model_shadow_projection(g_diligent_state.view_params))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_polygrid_list(
			g_diligent_translucent_polygrids,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled,
			true))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_particle_system_list(
			g_diligent_particle_systems,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_line_system_list(
			g_diligent_line_systems,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_canvas_list(g_diligent_translucent_canvases, true, g_diligent_state.view_params))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_sprite_list(
			g_diligent_translucent_sprites,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_volume_effect_list(
			g_diligent_translucent_volume_effects,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthEnabled,
			true))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_sprite_list(
			g_diligent_noz_sprites,
			g_diligent_state.view_params,
			g_CV_FogNearZ.m_Val,
			g_CV_FogFarZ.m_Val,
			kWorldDepthDisabled))
	{
		return RENDER_ERROR;
	}

	if (ssao_active)
	{
		if (!diligent_apply_ssao(ssao_ctx))
		{
			diligent_end_ssao(ssao_ctx);
			return RENDER_ERROR;
		}
		diligent_end_ssao(ssao_ctx);
	}

	if (ssao_fx_active && !ssao_msaa)
	{
		if (!diligent_apply_ssao_fx(diligent_get_active_render_target(), diligent_get_active_depth_target()))
		{
			return RENDER_ERROR;
		}
	}

	if (ssao_msaa && aa_active)
	{
		if (!diligent_apply_antialiasing(aa_ctx))
		{
			return RENDER_ERROR;
		}

		bool ssao_resolved_ok = false;
		if (use_ssao_fx)
		{
			if (ssao_fx_active)
			{
				ssao_resolved_ok = diligent_apply_ssao_fx_resolved(aa_ctx);
			}
		}
		else
		{
			ssao_resolved_ok = diligent_apply_ssao_resolved(aa_ctx);
		}

		if (!ssao_resolved_ok)
		{
			DiligentAaContext copy_ctx = aa_ctx;
			copy_ctx.defer_copy = false;
			if (!diligent_apply_antialiasing(copy_ctx))
			{
				return RENDER_ERROR;
			}
		}

		diligent_SetOutputTargets(aa_ctx.final_render_target, aa_ctx.final_depth_target);
	}

	diligent_render_screen_glow(desc);

	if (aa_active)
	{
		if (!ssao_msaa)
		{
			if (!diligent_apply_antialiasing(aa_ctx))
			{
				return RENDER_ERROR;
			}
		}
		diligent_end_antialiasing(aa_ctx);
		aa_scope.Dismiss();
	}

	if (debug_lines_enabled)
	{
		if (!diligent_draw_debug_lines())
		{
			return RENDER_ERROR;
		}
	}

	return RENDER_OK;
}
