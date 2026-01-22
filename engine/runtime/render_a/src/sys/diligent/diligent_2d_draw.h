#ifndef LTJS_DILIGENT_2D_DRAW_H
#define LTJS_DILIGENT_2D_DRAW_H

#include "ltbasedefs.h"
#include "renderstruct.h"

namespace Diligent
{
	struct RenderTargetBlendDesc;
	class IBuffer;
	class ITextureView;
}

struct DiligentOptimized2DVertex
{
	float position[2];
	float color[4];
	float uv[2];
};

void diligent_fill_optimized_2d_blend_desc(LTSurfaceBlend blend, Diligent::RenderTargetBlendDesc& blend_desc);

bool diligent_upload_optimized_2d_vertices(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	Diligent::IBuffer*& out_buffer);

bool diligent_draw_optimized_2d_vertices(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view);

bool diligent_draw_optimized_2d_vertices_to_target(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target,
	bool clamp_sampler);

HLTBUFFER diligent_CreateSurface(int width, int height);
void diligent_DeleteSurface(HLTBUFFER surface_handle);
void diligent_GetSurfaceInfo(HLTBUFFER surface_handle, uint32* width, uint32* height);
void* diligent_LockSurface(HLTBUFFER surface_handle, uint32& pitch);
void diligent_UnlockSurface(HLTBUFFER surface_handle);
bool diligent_OptimizeSurface(HLTBUFFER surface_handle, uint32 transparent_color);
void diligent_UnoptimizeSurface(HLTBUFFER surface_handle);
bool diligent_GetScreenFormat(PFormat* format);

bool diligent_StartOptimized2D();
void diligent_EndOptimized2D();
bool diligent_IsInOptimized2D();
bool diligent_SetOptimized2DBlend(LTSurfaceBlend blend);
bool diligent_GetOptimized2DBlend(LTSurfaceBlend& blend);
bool diligent_SetOptimized2DColor(HLTCOLOR color);
bool diligent_GetOptimized2DColor(HLTCOLOR& color);

void diligent_BlitToScreen(BlitRequest* request);
void diligent_BlitFromScreen(BlitRequest* request);
bool diligent_WarpToScreen(BlitRequest* request);

void diligent_release_optimized_2d_resources();

#endif
