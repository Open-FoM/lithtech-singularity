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

extern std::vector<SpriteInstance*> g_diligent_translucent_sprites;
extern std::vector<SpriteInstance*> g_diligent_noz_sprites;
extern std::vector<LineSystem*> g_diligent_line_systems;
extern std::vector<LTParticleSystem*> g_diligent_particle_systems;
extern std::vector<Canvas*> g_diligent_solid_canvases;
extern std::vector<Canvas*> g_diligent_translucent_canvases;
extern std::vector<LTVolumeEffect*> g_diligent_solid_volume_effects;
extern std::vector<LTVolumeEffect*> g_diligent_translucent_volume_effects;
extern std::vector<LTPolyGrid*> g_diligent_solid_polygrids;
extern std::vector<LTPolyGrid*> g_diligent_early_translucent_polygrids;
extern std::vector<LTPolyGrid*> g_diligent_translucent_polygrids;

bool diligent_draw_sky(SceneDesc* desc);

bool diligent_draw_sprite_list(
	const std::vector<SpriteInstance*>& sprites,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode);

bool diligent_draw_particle_system_list(
	const std::vector<LTParticleSystem*>& systems,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode);

bool diligent_draw_line_system_list(
	const std::vector<LineSystem*>& systems,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode);

bool diligent_draw_volume_effect_list(
	const std::vector<LTVolumeEffect*>& effects,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode,
	bool sort);

bool diligent_draw_polygrid_list(
	const std::vector<LTPolyGrid*>& grids,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode,
	bool sort);

bool diligent_draw_solid_canvases_glow(SceneDesc* desc);

bool diligent_draw_canvas_list(const std::vector<Canvas*>& canvases, bool sort, const ViewParams& params);

void diligent_collect_sprites(SceneDesc* desc);
void diligent_collect_line_systems(SceneDesc* desc);
void diligent_collect_particle_systems(SceneDesc* desc);
void diligent_collect_volume_effects(SceneDesc* desc);
void diligent_collect_polygrids(SceneDesc* desc);
void diligent_collect_canvases(SceneDesc* desc);

#endif
