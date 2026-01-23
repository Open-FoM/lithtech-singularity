/**
 * diligent_render_debug.h
 *
 * This header defines the Render Debug portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_RENDER_DEBUG_H
#define LTJS_DILIGENT_RENDER_DEBUG_H

#include <cstdint>

class SharedTexture;

/// \brief Texture inspection data for editor/debug UI.
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

/// \brief Retrieves debug data for a SharedTexture.
/// \details The returned info includes dimensions, format, and a small CPU-side
///          sample if available to aid debugging.
/// \code
/// DiligentTextureDebugInfo info{};
/// if (diligent_GetTextureDebugInfo(texture, info)) { /* inspect info */ }
/// \endcode
bool diligent_GetTextureDebugInfo(SharedTexture* texture, DiligentTextureDebugInfo& out_info);

/// \brief Aggregate statistics about world vertex colors.
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

/// \brief Gathers color statistics for currently loaded world geometry.
bool diligent_GetWorldColorStats(DiligentWorldColorStats& out_stats);

/// \brief Aggregate statistics about world texture usage and binding status.
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

/// \brief Gathers texture binding stats for currently loaded world geometry.
bool diligent_GetWorldTextureStats(DiligentWorldTextureStats& out_stats);

/// \brief Marks world geometry as dirty so it will be rebuilt on next render.
void diligent_InvalidateWorldGeometry();
/// \brief Dumps up to \p limit world texture bindings to the log.
void diligent_DumpWorldTextureBindings(uint32_t limit);
/// \brief Writes debug information for up to \p limit world sections to a file.
void diligent_DumpWorldSurfaceDebug(uint32_t limit);

/// \brief Aggregate statistics about world UV coordinates (ranges and NaNs).
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

/// \brief Gathers UV statistics for currently loaded world geometry.
bool diligent_GetWorldUvStats(DiligentWorldUvStats& out_stats);

/// \brief Pipeline usage counters for world rendering.
struct DiligentWorldPipelineStats
{
	uint64_t total_sections = 0;
	uint64_t sections_lightmap_data = 0;
	uint64_t sections_lightmap_view = 0;
	uint64_t sections_lightmap_black = 0;
	uint64_t sections_light_anim = 0;
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
	uint64_t mode_flat = 0;
	uint64_t mode_normals = 0;
};

/// \brief Resets pipeline usage counters.
void diligent_ResetWorldPipelineStats();
/// \brief Retrieves current pipeline usage counters.
bool diligent_GetWorldPipelineStats(DiligentWorldPipelineStats& out_stats);

/// \brief Drops cached world shader pipelines so they are rebuilt next frame.
void diligent_ResetWorldShaders();
#endif
