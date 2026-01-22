#include "diligent_renderer_lifecycle.h"

#include "diligent_2d_draw.h"
#include "diligent_debug_draw.h"
#include "diligent_device.h"
#include "diligent_state.h"
#include "diligent_model_draw.h"
#include "diligent_pipeline_cache.h"
#include "diligent_postfx.h"
#include "diligent_render_api.h"
#include "diligent_shadow_draw.h"
#include "diligent_texture_cache.h"
#include "diligent_world_data.h"
#include "diligent_world_draw.h"
#include "renderstruct.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

int diligent_Init(RenderStructInit* init)
{
	if (!init)
	{
		return RENDER_ERROR;
	}

	init->m_RendererVersion = LTRENDER_VERSION;
	g_diligent_state.screen_width = init->m_Mode.m_Width;
	g_diligent_state.screen_height = init->m_Mode.m_Height;

	diligent_UpdateWindowHandles(init);
	if (!g_diligent_state.native_window_handle)
	{
		return RENDER_ERROR;
	}

	if (!diligent_EnsureDevice())
	{
		return RENDER_ERROR;
	}

	diligent_UpdateScreenSize(g_diligent_state.screen_width, g_diligent_state.screen_height);
	if (!diligent_EnsureSwapChain())
	{
		return RENDER_ERROR;
	}

	return RENDER_OK;
}

void diligent_Term(bool)
{
	g_diligent_state.swap_chain.Release();
	g_diligent_state.immediate_context.Release();
	g_diligent_state.render_device.Release();
	g_pso_cache.Reset();
	g_srb_cache.Reset();
	g_render_world.reset();
	g_visible_render_blocks.clear();
	diligent_release_shadow_textures();
	diligent_postfx_term();
	diligent_render_api_term();
	g_world_pipelines.pipelines.clear();
	diligent_debug_term();
	diligent_release_model_resources();
	g_world_resources.vertex_shader.Release();
	g_world_resources.pixel_shader_textured.Release();
	g_world_resources.pixel_shader_glow.Release();
	g_world_resources.pixel_shader_lightmap.Release();
	g_world_resources.pixel_shader_lightmap_only.Release();
	g_world_resources.pixel_shader_solid.Release();
	g_world_resources.pixel_shader_flat.Release();
	g_world_resources.pixel_shader_normals.Release();
	g_world_resources.pixel_shader_dynamic_light.Release();
	g_world_resources.pixel_shader_bump.Release();
	g_world_resources.pixel_shader_volume_effect.Release();
	g_world_resources.pixel_shader_shadow_project.Release();
	g_world_resources.pixel_shader_debug.Release();
	g_world_resources.constant_buffer.Release();
	g_world_resources.shadow_project_constants.Release();
	g_world_resources.vertex_buffer.Release();
	g_world_resources.index_buffer.Release();
	g_world_resources.vertex_buffer_size = 0;
	g_world_resources.index_buffer_size = 0;
	diligent_release_optimized_2d_resources();
	g_diligent_state.engine_factory = nullptr;
	g_diligent_state.native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_diligent_state.sdl_window = nullptr;
#endif
	g_diligent_state.screen_width = 0;
	g_diligent_state.screen_height = 0;
	g_diligent_state.is_in_3d = false;
	g_diligent_state.is_in_optimized_2d = false;
}
