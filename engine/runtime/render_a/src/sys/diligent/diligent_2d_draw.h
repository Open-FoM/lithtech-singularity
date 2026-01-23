/**
 * diligent_2d_draw.h
 *
 * This header defines the 2d Draw portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
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

/// Vertex layout for optimized 2D rendering.
struct DiligentOptimized2DVertex
{
	float position[2];
	float color[4];
	float uv[2];
};

/// Fills a blend descriptor for optimized 2D rendering.
void diligent_fill_optimized_2d_blend_desc(LTSurfaceBlend blend, Diligent::RenderTargetBlendDesc& blend_desc);

/// Uploads optimized 2D vertices to a dynamic GPU buffer.
bool diligent_upload_optimized_2d_vertices(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	Diligent::IBuffer*& out_buffer);

/// \brief Draws optimized 2D vertices to the current render target.
/// \code
/// DiligentOptimized2DVertex verts[4] = {};
/// diligent_draw_optimized_2d_vertices(verts, 4, LTSURFACEBLEND_SOLID, texture_view);
/// \endcode
bool diligent_draw_optimized_2d_vertices(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view);

/// Draws optimized 2D vertices to an explicit render/depth target.
bool diligent_draw_optimized_2d_vertices_to_target(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target,
	bool clamp_sampler);

/// Creates a CPU-backed surface for 2D blits.
HLTBUFFER diligent_CreateSurface(int width, int height);
/// Destroys a surface created by diligent_CreateSurface.
void diligent_DeleteSurface(HLTBUFFER surface_handle);
/// Returns the surface width/height.
void diligent_GetSurfaceInfo(HLTBUFFER surface_handle, uint32* width, uint32* height);
/// Locks the surface for CPU access and returns a pointer to pixel data.
void* diligent_LockSurface(HLTBUFFER surface_handle, uint32& pitch);
/// Unlocks the surface after CPU access.
void diligent_UnlockSurface(HLTBUFFER surface_handle);
/// Converts surface data to an optimized format (e.g., colorkey handling).
bool diligent_OptimizeSurface(HLTBUFFER surface_handle, uint32 transparent_color);
/// Reverts an optimized surface back to its original representation.
void diligent_UnoptimizeSurface(HLTBUFFER surface_handle);
/// Returns the screen format used by the renderer.
bool diligent_GetScreenFormat(PFormat* format);

/// \brief Begins an optimized 2D batch (sets up state).
/// \details Call diligent_EndOptimized2D when done.
bool diligent_StartOptimized2D();
/// \brief Ends the optimized 2D batch and restores state.
void diligent_EndOptimized2D();
/// Returns true if currently in an optimized 2D batch.
bool diligent_IsInOptimized2D();
/// Sets the blend mode for optimized 2D rendering.
bool diligent_SetOptimized2DBlend(LTSurfaceBlend blend);
/// Retrieves the current optimized 2D blend mode.
bool diligent_GetOptimized2DBlend(LTSurfaceBlend& blend);
/// Sets the current optimized 2D vertex color.
bool diligent_SetOptimized2DColor(HLTCOLOR color);
/// Retrieves the current optimized 2D vertex color.
bool diligent_GetOptimized2DColor(HLTCOLOR& color);

/// Blits a surface to the screen using the provided request.
void diligent_BlitToScreen(BlitRequest* request);
/// Blits from the screen to a surface using the provided request.
void diligent_BlitFromScreen(BlitRequest* request);
/// Applies a warp effect to the screen using the provided request.
bool diligent_WarpToScreen(BlitRequest* request);

/// Releases cached optimized 2D GPU resources.
void diligent_release_optimized_2d_resources();

#endif
