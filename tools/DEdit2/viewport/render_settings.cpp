#include "viewport/render_settings.h"

#include "app/string_utils.h"
#include "diligent_render_debug.h"
#include "rendererconsolevars.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport/scene_filters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace
{
enum class RenderCategory
{
  World,
  WorldModel,
  Model,
  Sprite,
  PolyGrid,
  Particles,
  Volume,
  LineSystem,
  Canvas,
  Sky,
  Count
};

struct RenderCategoryState
{
  bool has = false;
  bool any_visible = false;
};
}

void ApplySceneVisibilityToRenderer(
  const ScenePanelState& scene_state,
  const ViewportPanelState& viewport_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props)
{
  std::array<RenderCategoryState, static_cast<size_t>(RenderCategory::Count)> categories{};
  const size_t count = std::min(nodes.size(), props.size());

  auto node_visible_for_render = [&](int node_id, const NodeProperties& node_props) -> bool
  {
    const TreeNode& node = nodes[static_cast<size_t>(node_id)];
    if (node.deleted || node.is_folder || !node_props.visible)
    {
      return false;
    }
    if (!SceneNodePassesFilters(scene_state, node_id, nodes, props))
    {
      return false;
    }
    return true;
  };

  auto mark = [&](RenderCategory cat)
  {
    RenderCategoryState& state = categories[static_cast<size_t>(cat)];
    state.has = true;
    state.any_visible = true;
  };

  for (size_t i = 0; i < count; ++i)
  {
    const NodeProperties& node_props = props[i];
    if (!node_visible_for_render(static_cast<int>(i), node_props))
    {
      continue;
    }

    const std::string type = LowerCopy(node_props.type);
    const std::string cls = LowerCopy(node_props.class_name);
    if (type == "world" || cls == "world")
    {
      mark(RenderCategory::World);
    }
    if (type == "worldmodel" || type == "world_model" || cls == "worldmodel" || cls == "world_model")
    {
      mark(RenderCategory::WorldModel);
    }
    if (type == "model" || cls == "model")
    {
      mark(RenderCategory::Model);
    }
    if (type == "sprite" || cls == "sprite")
    {
      mark(RenderCategory::Sprite);
    }
    if (type == "polygrid" || cls == "polygrid" || type == "terrain" || cls == "terrain")
    {
      mark(RenderCategory::PolyGrid);
    }
    if (type == "particlesystem" || cls == "particlesystem" ||
      type == "particles" || cls == "particles" ||
      type == "particle" || cls == "particle")
    {
      mark(RenderCategory::Particles);
    }
    if (type == "volumeeffect" || cls == "volumeeffect" ||
      type == "volume" || cls == "volume")
    {
      mark(RenderCategory::Volume);
    }
    if (type == "linesystem" || cls == "linesystem" ||
      type == "line" || cls == "line")
    {
      mark(RenderCategory::LineSystem);
    }
    if (type == "canvas" || cls == "canvas")
    {
      mark(RenderCategory::Canvas);
    }
    if (type == "sky" || cls == "sky" ||
      type == "skyobject" || cls == "skyobject")
    {
      mark(RenderCategory::Sky);
    }
  }

  auto apply = [&](RenderCategory cat, auto& cvar)
  {
    const RenderCategoryState& state = categories[static_cast<size_t>(cat)];
    const bool enabled = [&]()
    {
      switch (cat)
      {
        case RenderCategory::World:
          return viewport_state.render_draw_world;
        case RenderCategory::WorldModel:
          return viewport_state.render_draw_world_models;
        case RenderCategory::Model:
          return viewport_state.render_draw_models;
        case RenderCategory::Sprite:
          return viewport_state.render_draw_sprites;
        case RenderCategory::PolyGrid:
          return viewport_state.render_draw_polygrids;
        case RenderCategory::Particles:
          return viewport_state.render_draw_particles;
        case RenderCategory::Volume:
          return viewport_state.render_draw_volume_effects;
        case RenderCategory::LineSystem:
          return viewport_state.render_draw_line_systems;
        case RenderCategory::Canvas:
          return viewport_state.render_draw_canvases;
        case RenderCategory::Sky:
          return viewport_state.render_draw_sky;
        default:
          return true;
      }
    }();
    const bool visible = !state.has || state.any_visible;
    cvar = (enabled && visible) ? 1 : 0;
  };

  apply(RenderCategory::World, g_CV_DrawWorld);
  apply(RenderCategory::WorldModel, g_CV_DrawWorldModels);
  apply(RenderCategory::Model, g_CV_DrawModels);
  apply(RenderCategory::Sprite, g_CV_DrawSprites);
  apply(RenderCategory::PolyGrid, g_CV_DrawPolyGrids);
  apply(RenderCategory::Particles, g_CV_DrawParticles);
  apply(RenderCategory::Volume, g_CV_DrawVolumeEffects);
  apply(RenderCategory::LineSystem, g_CV_DrawLineSystems);
  apply(RenderCategory::Canvas, g_CV_DrawCanvases);
  apply(RenderCategory::Sky, g_CV_DrawSky);

  g_CV_Wireframe = viewport_state.render_wireframe_world ? 1 : 0;
  g_CV_WireframeModels = viewport_state.render_wireframe_models ? 1 : 0;
  g_CV_WorldShadingMode = viewport_state.render_world_shading_mode;
  g_CV_DrawFlat = (viewport_state.render_world_shading_mode == 1) ? 1 : 0;
  g_CV_WireframeOverlay = viewport_state.render_wireframe_overlay ? 1 : 0;
  {
    const int new_fullbright = viewport_state.render_fullbright ? 1 : 0;
    if (new_fullbright != g_CV_Fullbright.m_Val)
    {
      g_CV_Fullbright = new_fullbright;
      diligent_InvalidateWorldGeometry();
    }
  }
  int lightmap_mode = viewport_state.render_lightmap_mode;
  if (!viewport_state.render_lightmaps_available && lightmap_mode != 0)
  {
    lightmap_mode = 0;
  }
  g_CV_LightMap = (lightmap_mode != 0) ? 1 : 0;
  g_CV_LightmapsOnly = (lightmap_mode == 2) ? 1 : 0;
  g_CV_LightmapIntensity = viewport_state.render_lightmap_intensity;
  g_CV_DrawSorted = viewport_state.render_draw_sorted ? 1 : 0;
  g_CV_DrawSolidModels = viewport_state.render_draw_solid_models ? 1 : 0;
  g_CV_DrawTranslucentModels = viewport_state.render_draw_translucent_models ? 1 : 0;
  g_CV_TextureModels = viewport_state.render_texture_models ? 1 : 0;
  g_CV_LightModels = viewport_state.render_light_models ? 1 : 0;
  g_CV_ModelApplyAmbient = viewport_state.render_model_apply_ambient ? 1 : 0;
  g_CV_ModelApplySun = viewport_state.render_model_apply_sun ? 1 : 0;
  g_CV_ModelSaturation = viewport_state.render_model_saturation;
  g_CV_Saturate = (std::fabs(viewport_state.render_model_saturation - 1.0f) > 0.001f) ? 1 : 0;
  g_CV_DynamicLight = viewport_state.render_dynamic_lights ? 1 : 0;
  g_CV_DynamicLightWorld = viewport_state.render_dynamic_lights_world ? 1 : 0;
  g_CV_EnvMapPolyGrids = viewport_state.render_polygrid_env_map ? 1 : 0;
  g_CV_BumpMapPolyGrids = viewport_state.render_polygrid_bump_map ? 1 : 0;
  g_CV_ModelLODOffset = viewport_state.render_model_lod_offset;
  g_CV_ModelDebug_DrawBoxes = viewport_state.render_model_debug_boxes ? 1 : 0;
  g_CV_ModelDebug_DrawTouchingLights = viewport_state.render_model_debug_touching_lights ? 1 : 0;
  g_CV_ModelDebug_DrawSkeleton = viewport_state.render_model_debug_skeleton ? 1 : 0;
  g_CV_ModelDebug_DrawOBBS = viewport_state.render_model_debug_obbs ? 1 : 0;
  g_CV_ModelDebug_DrawVertexNormals = viewport_state.render_model_debug_vertex_normals ? 1 : 0;
  g_CV_DrawWorldTree = viewport_state.render_world_debug_nodes ? 1 : 0;
  g_CV_DrawWorldLeaves = viewport_state.render_world_debug_leaves ? 1 : 0;
  g_CV_DrawWorldPortals = viewport_state.render_world_debug_portals ? 1 : 0;
  g_CV_ScreenGlowEnable = viewport_state.render_screen_glow_enable ? 1 : 0;
  g_CV_ScreenGlowShowTexture = viewport_state.render_screen_glow_show_texture ? 1 : 0;
  g_CV_ScreenGlowShowFilter = viewport_state.render_screen_glow_show_filter ? 1 : 0;
  g_CV_ScreenGlowShowTextureScale = viewport_state.render_screen_glow_show_texture_scale;
  g_CV_ScreenGlowShowFilterScale = viewport_state.render_screen_glow_show_filter_scale;
  g_CV_ScreenGlowUVScale = viewport_state.render_screen_glow_uv_scale;
  g_CV_ScreenGlowTextureSize = viewport_state.render_screen_glow_texture_size;
  g_CV_ScreenGlowFilterSize = viewport_state.render_screen_glow_filter_size;
  g_CV_SSAOEnable = viewport_state.render_ssao_enable ? 1 : 0;
  g_CV_SSAOBackend = viewport_state.render_ssao_backend;
  g_CV_SSAOIntensity = viewport_state.render_ssao_intensity;
  g_CV_SSAOFxEffectRadius = viewport_state.render_ssao_fx_effect_radius;
  g_CV_SSAOFxEffectFalloffRange = viewport_state.render_ssao_fx_effect_falloff_range;
  g_CV_SSAOFxRadiusMultiplier = viewport_state.render_ssao_fx_radius_multiplier;
  g_CV_SSAOFxDepthMipOffset = viewport_state.render_ssao_fx_depth_mip_offset;
  g_CV_SSAOFxTemporalStability = viewport_state.render_ssao_fx_temporal_stability;
  g_CV_SSAOFxSpatialReconstructionRadius = viewport_state.render_ssao_fx_spatial_reconstruction_radius;
  g_CV_SSAOFxAlphaInterpolation = viewport_state.render_ssao_fx_alpha_interpolation;
  g_CV_SSAOFxHalfResolution = viewport_state.render_ssao_fx_half_resolution ? 1 : 0;
  g_CV_SSAOFxHalfPrecisionDepth = viewport_state.render_ssao_fx_half_precision_depth ? 1 : 0;
  g_CV_SSAOFxUniformWeighting = viewport_state.render_ssao_fx_uniform_weighting ? 1 : 0;
  g_CV_SSAOFxResetAccumulation = viewport_state.render_ssao_fx_reset_accumulation ? 1 : 0;
}
