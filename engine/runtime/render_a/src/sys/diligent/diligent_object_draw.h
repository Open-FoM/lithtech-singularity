/**
 * diligent_object_draw.h
 *
 * This header defines the Object Draw portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_OBJECT_DRAW_H
#define LTJS_DILIGENT_OBJECT_DRAW_H

#include "diligent_world_draw.h"

#include <vector>

class Canvas;
class LineSystem;
class LTParticleSystem;
class LTPolyGrid;
class LTVolumeEffect;
class SpriteInstance;
class ViewParams;
struct SceneDesc;

/// Translucent sprite instances for the current frame.
extern std::vector<SpriteInstance*> g_diligent_translucent_sprites;
/// No-Z sprite instances for the current frame.
extern std::vector<SpriteInstance*> g_diligent_noz_sprites;
/// Line systems to render this frame.
extern std::vector<LineSystem*> g_diligent_line_systems;
/// Particle systems to render this frame.
extern std::vector<LTParticleSystem*> g_diligent_particle_systems;
/// Opaque canvases to render this frame.
extern std::vector<Canvas*> g_diligent_solid_canvases;
/// Translucent canvases to render this frame.
extern std::vector<Canvas*> g_diligent_translucent_canvases;
/// Opaque volume effects to render this frame.
extern std::vector<LTVolumeEffect*> g_diligent_solid_volume_effects;
/// Translucent volume effects to render this frame.
extern std::vector<LTVolumeEffect*> g_diligent_translucent_volume_effects;
/// Opaque polygrids to render this frame.
extern std::vector<LTPolyGrid*> g_diligent_solid_polygrids;
/// Early translucent polygrids to render this frame.
extern std::vector<LTPolyGrid*> g_diligent_early_translucent_polygrids;
/// Translucent polygrids to render this frame.
extern std::vector<LTPolyGrid*> g_diligent_translucent_polygrids;

/// \brief Draws the sky for the current scene.
/// \details Uses sky portal geometry and sky textures from the world data.
bool diligent_draw_sky(SceneDesc* desc);

/// \brief Draws a list of sprite instances with the given view/fog parameters.
/// \code
/// diligent_draw_sprite_list(g_diligent_translucent_sprites, params, fog_near, fog_far, kWorldDepthEnabled);
/// \endcode
bool diligent_draw_sprite_list(
	const std::vector<SpriteInstance*>& sprites,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode);

/// \brief Draws a list of particle systems with the given view/fog parameters.
bool diligent_draw_particle_system_list(
	const std::vector<LTParticleSystem*>& systems,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode);

/// \brief Draws a list of line systems with the given view/fog parameters.
bool diligent_draw_line_system_list(
	const std::vector<LineSystem*>& systems,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode);

/// \brief Draws a list of volume effects with the given view/fog parameters.
bool diligent_draw_volume_effect_list(
	const std::vector<LTVolumeEffect*>& effects,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode,
	bool sort);

/// \brief Draws a list of polygrids with the given view/fog parameters.
bool diligent_draw_polygrid_list(
	const std::vector<LTPolyGrid*>& grids,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode,
	bool sort);

/// \brief Draws glow pass for solid canvases.
bool diligent_draw_solid_canvases_glow(SceneDesc* desc);

/// \brief Draws a list of canvases, optionally sorted back-to-front.
bool diligent_draw_canvas_list(const std::vector<Canvas*>& canvases, bool sort, const ViewParams& params);

/// \brief Collects sprites from the scene into per-frame lists.
void diligent_collect_sprites(SceneDesc* desc);
/// \brief Collects line systems from the scene into per-frame lists.
void diligent_collect_line_systems(SceneDesc* desc);
/// \brief Collects particle systems from the scene into per-frame lists.
void diligent_collect_particle_systems(SceneDesc* desc);
/// \brief Collects volume effects from the scene into per-frame lists.
void diligent_collect_volume_effects(SceneDesc* desc);
/// \brief Collects polygrids from the scene into per-frame lists.
void diligent_collect_polygrids(SceneDesc* desc);
/// \brief Collects canvases from the scene into per-frame lists.
void diligent_collect_canvases(SceneDesc* desc);

#endif
