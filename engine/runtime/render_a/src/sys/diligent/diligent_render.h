#ifndef __DILIGENT_RENDER_H__
#define __DILIGENT_RENDER_H__

#include "ltvector.h"
#include <cstdint>

class SharedTexture;
class TextureData;

namespace Diligent
{
	class ITextureView;
}

// Overrides the render target used by the renderer. Pass nullptrs to restore swapchain output.
void diligent_SetOutputTargets(Diligent::ITextureView *render_target, Diligent::ITextureView *depth_target);
void diligent_SetShadersEnabled(int enabled);
int diligent_GetShadersEnabled();
void diligent_SetWorldOffset(const LTVector& offset);
const LTVector& diligent_GetWorldOffset();

Diligent::ITextureView *diligent_get_drawprim_texture_view(SharedTexture *shared_texture, bool texture_changed);

struct DiligentTextureDebugInfo
{
	bool has_render_texture = false;
	bool used_conversion = false;
	bool has_cpu_pixel = false;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t format = 0;
	uint32_t first_pixel = 0;
};

bool diligent_GetTextureDebugInfo(SharedTexture* texture, DiligentTextureDebugInfo& out_info);

struct DiligentWorldColorStats
{
	uint64_t vertex_count = 0;
	uint64_t zero_color_count = 0;
	uint32_t min_r = 255;
	uint32_t min_g = 255;
	uint32_t min_b = 255;
	uint32_t min_a = 255;
	uint32_t max_r = 0;
	uint32_t max_g = 0;
	uint32_t max_b = 0;
	uint32_t max_a = 0;
};

bool diligent_GetWorldColorStats(DiligentWorldColorStats& out_stats);

struct DiligentWorldTextureStats
{
	uint64_t section_count = 0;
	uint64_t texture0_present = 0;
	uint64_t texture1_present = 0;
	uint64_t texture0_view = 0;
	uint64_t texture1_view = 0;
	uint64_t lightmap_present = 0;
	uint64_t lightmap_view = 0;
};

bool diligent_GetWorldTextureStats(DiligentWorldTextureStats& out_stats);

struct DiligentFogDebugInfo
{
	int enabled = 0;
	float near_z = 0.0f;
	float far_z = 0.0f;
	int color_r = 0;
	int color_g = 0;
	int color_b = 0;
};

bool diligent_GetFogDebugInfo(DiligentFogDebugInfo& out_info);
void diligent_SetFogEnabled(int enabled);
void diligent_SetFogRange(float near_z, float far_z);
void diligent_InvalidateWorldGeometry();
void diligent_SetForceTexturedWorld(int enabled);
int diligent_GetForceTexturedWorld();
void diligent_DumpWorldTextureBindings(uint32_t limit);

struct DiligentWorldUvStats
{
	uint64_t vertex_count = 0;
	uint64_t nan_uv0 = 0;
	uint64_t nan_uv1 = 0;
	float min_u0 = 0.0f;
	float min_v0 = 0.0f;
	float max_u0 = 0.0f;
	float max_v0 = 0.0f;
	float min_u1 = 0.0f;
	float min_v1 = 0.0f;
	float max_u1 = 0.0f;
	float max_v1 = 0.0f;
	bool has_range = false;
};

bool diligent_GetWorldUvStats(DiligentWorldUvStats& out_stats);
void diligent_SetWorldUvDebug(int enabled);
int diligent_GetWorldUvDebug();

struct DiligentWorldPipelineStats
{
	uint64_t total_sections = 0;
	uint64_t mode_skip = 0;
	uint64_t mode_solid = 0;
	uint64_t mode_textured = 0;
	uint64_t mode_lightmap = 0;
	uint64_t mode_lightmap_only = 0;
	uint64_t mode_dual = 0;
	uint64_t mode_lightmap_dual = 0;
	uint64_t mode_bump = 0;
	uint64_t mode_glow = 0;
	uint64_t mode_dynamic_light = 0;
	uint64_t mode_volume = 0;
	uint64_t mode_shadow_project = 0;
};

void diligent_ResetWorldPipelineStats();
bool diligent_GetWorldPipelineStats(DiligentWorldPipelineStats& out_stats);

void diligent_SetWorldPsDebug(int enabled);
int diligent_GetWorldPsDebug();
void diligent_ResetWorldShaders();
void diligent_SetWorldTexDebugMode(int mode);
int diligent_GetWorldTexDebugMode();
void diligent_SetWorldTexelUV(int enabled);
int diligent_GetWorldTexelUV();
void diligent_SetWorldUseBaseVertex(int enabled);
int diligent_GetWorldUseBaseVertex();

bool diligent_TestConvertTextureToBgra32(const TextureData* source, uint32 mip_count, TextureData*& out_converted);

#endif
