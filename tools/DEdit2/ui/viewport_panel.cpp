#include "ui/viewport_panel.h"

#include "dedit2_concommand.h"
#include "multi_viewport.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport/diligent_viewport.h"
#include "viewport/interaction.h"
#include "viewport/lighting.h"
#include "viewport/scene_filters.h"
#include "viewport/sky_models.h"
#include "viewport/world_bounds.h"
#include "viewport_picking.h"
#include "viewport_render.h"
#include "viewport_gizmo.h"

#include "app/string_utils.h"
#include "imgui.h"
#include "rendererconsolevars.h"

#include <algorithm>
#include <cstring>
#include <cmath>

namespace
{
/// Draw the toolbar for a single viewport.
void DrawViewportToolbar(ViewportPanelState& viewport_panel, DiligentContext& diligent,
  std::vector<TreeNode>& scene_nodes, std::vector<NodeProperties>& scene_props);

/// Draw the render settings popup.
void DrawRenderSettingsPopup(ViewportPanelState& viewport_panel, DiligentContext& diligent,
  std::vector<TreeNode>& scene_nodes, std::vector<NodeProperties>& scene_props);

/// Sync render settings from source viewport to target viewport.
/// Copies render flags and settings but NOT camera position, view mode, or gizmo state.
void SyncViewportRenderSettings(ViewportPanelState& target, const ViewportPanelState& source)
{
  // Copy draw flags
  target.render_draw_world = source.render_draw_world;
  target.render_draw_world_models = source.render_draw_world_models;
  target.render_draw_models = source.render_draw_models;
  target.render_draw_sprites = source.render_draw_sprites;
  target.render_draw_polygrids = source.render_draw_polygrids;
  target.render_draw_particles = source.render_draw_particles;
  target.render_draw_volume_effects = source.render_draw_volume_effects;
  target.render_draw_line_systems = source.render_draw_line_systems;
  target.render_draw_canvases = source.render_draw_canvases;
  target.render_draw_sky = source.render_draw_sky;
  target.sky_world_model = source.sky_world_model;

  // Copy render modes
  target.render_wireframe_world = source.render_wireframe_world;
  target.render_wireframe_models = source.render_wireframe_models;
  target.render_world_shading_mode = source.render_world_shading_mode;
  target.render_wireframe_overlay = source.render_wireframe_overlay;
  target.render_lightmap_mode = source.render_lightmap_mode;
  target.render_lightmaps_available = source.render_lightmaps_available;
  target.render_lightmap_intensity = source.render_lightmap_intensity;
  target.render_fullbright = source.render_fullbright;
  target.render_draw_sorted = source.render_draw_sorted;

  // Copy model rendering settings
  target.render_draw_solid_models = source.render_draw_solid_models;
  target.render_draw_translucent_models = source.render_draw_translucent_models;
  target.render_texture_models = source.render_texture_models;
  target.render_light_models = source.render_light_models;
  target.render_model_apply_ambient = source.render_model_apply_ambient;
  target.render_model_apply_sun = source.render_model_apply_sun;
  target.render_model_saturation = source.render_model_saturation;
  target.render_model_lod_offset = source.render_model_lod_offset;

  // Copy lighting settings
  target.render_dynamic_lights = source.render_dynamic_lights;
  target.render_dynamic_lights_world = source.render_dynamic_lights_world;
  target.render_polygrid_env_map = source.render_polygrid_env_map;
  target.render_polygrid_bump_map = source.render_polygrid_bump_map;

  // Copy debug rendering settings
  target.render_model_debug_boxes = source.render_model_debug_boxes;
  target.render_model_debug_touching_lights = source.render_model_debug_touching_lights;
  target.render_model_debug_skeleton = source.render_model_debug_skeleton;
  target.render_model_debug_obbs = source.render_model_debug_obbs;
  target.render_model_debug_vertex_normals = source.render_model_debug_vertex_normals;
  target.render_world_debug_nodes = source.render_world_debug_nodes;
  target.render_world_debug_leaves = source.render_world_debug_leaves;
  target.render_world_debug_portals = source.render_world_debug_portals;

  // Copy post-processing settings
  target.render_screen_glow_enable = source.render_screen_glow_enable;
  target.render_screen_glow_show_texture = source.render_screen_glow_show_texture;
  target.render_screen_glow_show_filter = source.render_screen_glow_show_filter;
  target.render_screen_glow_show_texture_scale = source.render_screen_glow_show_texture_scale;
  target.render_screen_glow_show_filter_scale = source.render_screen_glow_show_filter_scale;
  target.render_screen_glow_uv_scale = source.render_screen_glow_uv_scale;
  target.render_screen_glow_texture_size = source.render_screen_glow_texture_size;
  target.render_screen_glow_filter_size = source.render_screen_glow_filter_size;
  target.render_ssao_enable = source.render_ssao_enable;
  target.render_ssao_backend = source.render_ssao_backend;
  target.render_ssao_intensity = source.render_ssao_intensity;
  target.render_ssao_fx_effect_radius = source.render_ssao_fx_effect_radius;
  target.render_ssao_fx_effect_falloff_range = source.render_ssao_fx_effect_falloff_range;
  target.render_ssao_fx_radius_multiplier = source.render_ssao_fx_radius_multiplier;
  target.render_ssao_fx_depth_mip_offset = source.render_ssao_fx_depth_mip_offset;
  target.render_ssao_fx_temporal_stability = source.render_ssao_fx_temporal_stability;
  target.render_ssao_fx_spatial_reconstruction_radius = source.render_ssao_fx_spatial_reconstruction_radius;
  target.render_ssao_fx_alpha_interpolation = source.render_ssao_fx_alpha_interpolation;
  target.render_ssao_fx_half_resolution = source.render_ssao_fx_half_resolution;
  target.render_ssao_fx_half_precision_depth = source.render_ssao_fx_half_precision_depth;
  target.render_ssao_fx_uniform_weighting = source.render_ssao_fx_uniform_weighting;
  target.render_ssao_fx_reset_accumulation = source.render_ssao_fx_reset_accumulation;

  // Copy UI overlay settings
  target.show_light_icons = source.show_light_icons;
  target.light_icon_occlusion = source.light_icon_occlusion;
  target.show_grid = source.show_grid;
  target.show_axes = source.show_axes;
}
} // namespace

ViewportPanelResult DrawViewportPanel(
  DiligentContext& diligent,
  MultiViewportState& multi_viewport,
  ScenePanelState& scene_panel,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  SelectionTarget active_target)
{
  // Get the active viewport for convenience
  ViewportPanelState& viewport_panel = multi_viewport.ActiveViewport();
  ViewportPanelResult result{};
  // Clear all viewport visibility flags
  for (int i = 0; i < kMaxViewportRenderSlots; ++i)
  {
    diligent.viewport_visible[i] = false;
  }
  if (!ImGui::Begin("Viewport"))
  {
    ImGui::End();
    return result;
  }

  if (ImGui::Checkbox("Grid", &viewport_panel.show_grid))
  {
    // toggle only
  }
  ImGui::SameLine();
  if (ImGui::Checkbox("Axes", &viewport_panel.show_axes))
  {
    // toggle only
  }
  ImGui::SameLine();
  if (ImGui::Button("Center"))
  {
    float bounds_min[3] = {0.0f, 0.0f, 0.0f};
    float bounds_max[3] = {0.0f, 0.0f, 0.0f};
    if (TryGetWorldBounds(scene_props, bounds_min, bounds_max))
    {
      const float center[3] = {
        (bounds_min[0] + bounds_max[0]) * 0.5f,
        (bounds_min[1] + bounds_max[1]) * 0.5f,
        (bounds_min[2] + bounds_max[2]) * 0.5f};
      viewport_panel.grid_origin[0] = center[0];
      viewport_panel.grid_origin[1] = center[1];
      viewport_panel.grid_origin[2] = center[2];
      viewport_panel.target[0] = center[0];
      viewport_panel.target[1] = center[1];
      viewport_panel.target[2] = center[2];

      const float dx = bounds_max[0] - bounds_min[0];
      const float dy = bounds_max[1] - bounds_min[1];
      const float dz = bounds_max[2] - bounds_min[2];
      const float radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
      const float desired = std::max(100.0f, radius * 2.6f);
      viewport_panel.orbit_distance = std::min(desired, 20000.0f);
      if (viewport_panel.fly_mode)
      {
        SyncFlyFromOrbit(viewport_panel);
      }
    }
    else
    {
      DEdit2_Log("World bounds unavailable; cannot center view.");
    }
  }
  ImGui::SameLine();
  bool previous_fly_mode = viewport_panel.fly_mode;
  ImGui::Checkbox("Fly", &viewport_panel.fly_mode);
  if (viewport_panel.fly_mode != previous_fly_mode)
  {
    if (viewport_panel.fly_mode)
    {
      SyncFlyFromOrbit(viewport_panel);
    }
    else
    {
      SyncOrbitFromFly(viewport_panel);
    }
  }
  ImGui::SameLine();
  bool gizmo_move = viewport_panel.gizmo_mode == ViewportPanelState::GizmoMode::Translate;
  bool gizmo_rotate = viewport_panel.gizmo_mode == ViewportPanelState::GizmoMode::Rotate;
  bool gizmo_scale = viewport_panel.gizmo_mode == ViewportPanelState::GizmoMode::Scale;
  if (ImGui::RadioButton("Move", gizmo_move))
  {
    viewport_panel.gizmo_mode = ViewportPanelState::GizmoMode::Translate;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", gizmo_rotate))
  {
    viewport_panel.gizmo_mode = ViewportPanelState::GizmoMode::Rotate;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", gizmo_scale))
  {
    viewport_panel.gizmo_mode = ViewportPanelState::GizmoMode::Scale;
  }
  ImGui::SameLine();
  if (ImGui::Button("Gizmo Settings"))
  {
    ImGui::OpenPopup("GizmoSettings");
  }
  ImGui::SameLine();
  if (ImGui::Button("Render"))
  {
    ImGui::OpenPopup("RenderSettings");
  }
  ImGui::SameLine();
  const float eye_button_size = ImGui::GetFrameHeight();
  if (ImGui::Button("##LightIconSettings", ImVec2(eye_button_size, eye_button_size)))
  {
    ImGui::OpenPopup("LightIconSettings");
  }
  {
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
    const ImVec2 radius(
      (max.x - min.x) * 0.38f,
      (max.y - min.y) * 0.24f);
    const ImU32 eye_color = ImGui::GetColorU32(
      viewport_panel.show_light_icons ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddEllipse(center, radius, eye_color, 0.0f, 0, 1.2f);
    draw_list->AddCircleFilled(center, radius.y * 0.55f, eye_color);
  }
  if (ImGui::BeginPopup("GizmoSettings"))
  {
    ImGui::TextUnformatted("Space");
    ImGui::Separator();
    ImGui::TextUnformatted("Gizmo");
    ImGui::SameLine();
    const bool gizmo_world = viewport_panel.gizmo_space == ViewportPanelState::GizmoSpace::World;
    const bool gizmo_local = viewport_panel.gizmo_space == ViewportPanelState::GizmoSpace::Local;
    if (ImGui::RadioButton("World##GizmoSpace", gizmo_world))
    {
      viewport_panel.gizmo_space = ViewportPanelState::GizmoSpace::World;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Local##GizmoSpace", gizmo_local))
    {
      viewport_panel.gizmo_space = ViewportPanelState::GizmoSpace::Local;
    }
    ImGui::TextUnformatted("Rotation");
    ImGui::SameLine();
    const bool rot_world = viewport_panel.rotation_space == ViewportPanelState::GizmoSpace::World;
    const bool rot_local = viewport_panel.rotation_space == ViewportPanelState::GizmoSpace::Local;
    if (ImGui::RadioButton("World##RotSpace", rot_world))
    {
      viewport_panel.rotation_space = ViewportPanelState::GizmoSpace::World;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Local##RotSpace", rot_local))
    {
      viewport_panel.rotation_space = ViewportPanelState::GizmoSpace::Local;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Snapping");
    ImGui::Separator();
    ImGui::Checkbox("Snap Move", &viewport_panel.snap_translate);
    ImGui::SameLine();
    ImGui::DragFloat("Move Step", &viewport_panel.snap_translate_step, 0.1f, 0.01f, 1000.0f);
    ImGui::TextUnformatted("Move Axes");
    ImGui::SameLine();
    ImGui::Checkbox("X##MoveAxis", &viewport_panel.snap_translate_axis[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Y##MoveAxis", &viewport_panel.snap_translate_axis[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Z##MoveAxis", &viewport_panel.snap_translate_axis[2]);

    ImGui::Separator();
    ImGui::Checkbox("Snap Rotate", &viewport_panel.snap_rotate);
    ImGui::SameLine();
    ImGui::DragFloat("Rotate Step", &viewport_panel.snap_rotate_step, 1.0f, 1.0f, 180.0f);
    ImGui::TextUnformatted("Rotate Axes");
    ImGui::SameLine();
    ImGui::Checkbox("X##RotAxis", &viewport_panel.snap_rotate_axis[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Y##RotAxis", &viewport_panel.snap_rotate_axis[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Z##RotAxis", &viewport_panel.snap_rotate_axis[2]);

    ImGui::Separator();
    ImGui::Checkbox("Snap Scale", &viewport_panel.snap_scale);
    ImGui::SameLine();
    ImGui::DragFloat("Scale Step", &viewport_panel.snap_scale_step, 0.01f, 0.01f, 10.0f);
    ImGui::TextUnformatted("Scale Axes");
    ImGui::SameLine();
    ImGui::Checkbox("X##ScaleAxis", &viewport_panel.snap_scale_axis[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Y##ScaleAxis", &viewport_panel.snap_scale_axis[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Z##ScaleAxis", &viewport_panel.snap_scale_axis[2]);

    ImGui::Separator();
    ImGui::TextUnformatted("Sizing");
    ImGui::Separator();
    ImGui::SliderFloat("Overall", &viewport_panel.gizmo_size, 0.25f, 3.0f, "%.2f");
    ImGui::DragFloat3(
      "Axis Scale",
      viewport_panel.gizmo_axis_scale,
      0.05f,
      0.1f,
      5.0f,
      "%.2f");
    ImGui::EndPopup();
  }
  if (ImGui::BeginPopup("LightIconSettings"))
  {
    ImGui::TextUnformatted("Lightbulbs");
    ImGui::Separator();
    ImGui::Checkbox("Show Lightbulb Icons", &viewport_panel.show_light_icons);
    if (!viewport_panel.show_light_icons)
    {
      ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Occlude Behind Geometry", &viewport_panel.light_icon_occlusion);
    if (!viewport_panel.show_light_icons)
    {
      ImGui::EndDisabled();
    }
    ImGui::EndPopup();
  }
  if (ImGui::BeginPopup("RenderSettings"))
  {
    if (ImGui::BeginTabBar("RenderSettingsTabs"))
    {
      if (ImGui::BeginTabItem("Draw"))
      {
        if (ImGui::Button("All On"))
        {
          viewport_panel.render_draw_world = true;
          viewport_panel.render_draw_world_models = true;
          viewport_panel.render_draw_models = true;
          viewport_panel.render_draw_sprites = true;
          viewport_panel.render_draw_polygrids = true;
          viewport_panel.render_draw_particles = true;
          viewport_panel.render_draw_volume_effects = true;
          viewport_panel.render_draw_line_systems = true;
          viewport_panel.render_draw_canvases = true;
          viewport_panel.render_draw_sky = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("All Off"))
        {
          viewport_panel.render_draw_world = false;
          viewport_panel.render_draw_world_models = false;
          viewport_panel.render_draw_models = false;
          viewport_panel.render_draw_sprites = false;
          viewport_panel.render_draw_polygrids = false;
          viewport_panel.render_draw_particles = false;
          viewport_panel.render_draw_volume_effects = false;
          viewport_panel.render_draw_line_systems = false;
          viewport_panel.render_draw_canvases = false;
          viewport_panel.render_draw_sky = false;
        }
        ImGui::Separator();
        ImGui::Checkbox("World", &viewport_panel.render_draw_world);
        ImGui::Checkbox("World Models", &viewport_panel.render_draw_world_models);
        ImGui::Checkbox("Models", &viewport_panel.render_draw_models);
        ImGui::Checkbox("Sprites", &viewport_panel.render_draw_sprites);
        ImGui::Checkbox("PolyGrids", &viewport_panel.render_draw_polygrids);
        ImGui::Checkbox("Particles", &viewport_panel.render_draw_particles);
        ImGui::Checkbox("Volume Effects", &viewport_panel.render_draw_volume_effects);
        ImGui::Checkbox("Line Systems", &viewport_panel.render_draw_line_systems);
        ImGui::Checkbox("Canvases", &viewport_panel.render_draw_canvases);
        ImGui::Checkbox("Sky", &viewport_panel.render_draw_sky);
        if (!viewport_panel.render_draw_sky || diligent.sky_world_model_names.empty())
        {
          ImGui::BeginDisabled();
        }
        const char* current_sky = viewport_panel.sky_world_model.empty()
          ? "Auto"
          : viewport_panel.sky_world_model.c_str();
        if (ImGui::BeginCombo("Sky Model", current_sky))
        {
          const bool auto_selected = viewport_panel.sky_world_model.empty();
          if (ImGui::Selectable("Auto", auto_selected))
          {
            viewport_panel.sky_world_model.clear();
            BuildSkyWorldModels(diligent, scene_nodes, scene_props, viewport_panel.sky_world_model);
          }
          if (auto_selected)
          {
            ImGui::SetItemDefaultFocus();
          }
          for (const auto& name : diligent.sky_world_model_names)
          {
            const bool selected = (viewport_panel.sky_world_model == name);
            if (ImGui::Selectable(name.c_str(), selected))
            {
              viewport_panel.sky_world_model = name;
              BuildSkyWorldModels(diligent, scene_nodes, scene_props, viewport_panel.sky_world_model);
            }
            if (selected)
            {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        if (!viewport_panel.render_draw_sky || diligent.sky_world_model_names.empty())
        {
          ImGui::EndDisabled();
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Modes"))
      {
        ImGui::Checkbox("Wireframe World", &viewport_panel.render_wireframe_world);
        ImGui::Checkbox("Wireframe Models", &viewport_panel.render_wireframe_models);
        const char* shading_modes[] = {"Textured", "Flat", "Normals"};
        ImGui::Combo("World Shading Mode", &viewport_panel.render_world_shading_mode, shading_modes, IM_ARRAYSIZE(shading_modes));
        ImGui::Checkbox("Wireframe Overlay", &viewport_panel.render_wireframe_overlay);
        ImGui::Checkbox("Fullbright", &viewport_panel.render_fullbright);
        ImGui::Separator();
        ImGui::TextUnformatted("Texture Filtering");
        const char* anisotropy_levels[] = {"Off", "2x", "4x", "8x", "16x"};
        int anisotropy_index = 0;
        switch (g_CV_Anisotropic.m_Val)
        {
          case 16: anisotropy_index = 4; break;
          case 8: anisotropy_index = 3; break;
          case 4: anisotropy_index = 2; break;
          case 2: anisotropy_index = 1; break;
          default: anisotropy_index = 0; break;
        }
        if (ImGui::Combo("Anisotropic", &anisotropy_index, anisotropy_levels, IM_ARRAYSIZE(anisotropy_levels)))
        {
          static const int kLevels[] = {0, 2, 4, 8, 16};
          g_CV_Anisotropic.m_Val = kLevels[anisotropy_index];
        }
        ImGui::Separator();
        ImGui::TextUnformatted("Antialiasing");
        const char* msaa_levels[] = {"Off", "2x", "4x", "8x"};
        int msaa_index = 0;
        switch (g_CV_MSAA.m_Val)
        {
          case 8: msaa_index = 3; break;
          case 4: msaa_index = 2; break;
          case 2: msaa_index = 1; break;
          default: msaa_index = 0; break;
        }
        if (ImGui::Combo("MSAA", &msaa_index, msaa_levels, IM_ARRAYSIZE(msaa_levels)))
        {
          static const int kMsaaLevels[] = {0, 2, 4, 8};
          g_CV_MSAA.m_Val = kMsaaLevels[msaa_index];
        }
        const char* lightmap_modes[] = {"Dynamic", "Baked (if available)", "Lightmap Only"};
        int& lightmap_mode = viewport_panel.render_lightmap_mode;
        if (!viewport_panel.render_lightmaps_available && lightmap_mode != 0)
        {
          lightmap_mode = 0;
        }
        const char* current_lightmap_mode = lightmap_modes[lightmap_mode];
        if (ImGui::BeginCombo("World Lighting", current_lightmap_mode))
        {
          if (ImGui::Selectable(lightmap_modes[0], lightmap_mode == 0))
          {
            lightmap_mode = 0;
          }
          if (!viewport_panel.render_lightmaps_available)
          {
            ImGui::BeginDisabled();
          }
          if (ImGui::Selectable(lightmap_modes[1], lightmap_mode == 1))
          {
            lightmap_mode = 1;
          }
          if (ImGui::Selectable(lightmap_modes[2], lightmap_mode == 2))
          {
            lightmap_mode = 2;
          }
          if (!viewport_panel.render_lightmaps_available)
          {
            ImGui::EndDisabled();
          }
          ImGui::EndCombo();
        }
        ImGui::DragFloat("Lightmap Intensity", &viewport_panel.render_lightmap_intensity, 0.05f, 0.0f, 4.0f, "%.2f");
        ImGui::Checkbox("Draw Sorted", &viewport_panel.render_draw_sorted);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Models"))
      {
        ImGui::Checkbox("Solid Models", &viewport_panel.render_draw_solid_models);
        ImGui::Checkbox("Translucent Models", &viewport_panel.render_draw_translucent_models);
        ImGui::Checkbox("Texture Models", &viewport_panel.render_texture_models);
        ImGui::DragInt("Model LOD Offset", &viewport_panel.render_model_lod_offset, 1.0f, -5, 5);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Lighting"))
      {
        ImGui::Checkbox("Light Models", &viewport_panel.render_light_models);
        ImGui::Checkbox("Apply Ambient", &viewport_panel.render_model_apply_ambient);
        ImGui::Checkbox("Apply Sun", &viewport_panel.render_model_apply_sun);
        ImGui::SliderFloat("Saturation", &viewport_panel.render_model_saturation, 0.0f, 2.0f, "%.2f");
        ImGui::Separator();
        ImGui::TextUnformatted("World");
        ImGui::Checkbox("Dynamic Lights", &viewport_panel.render_dynamic_lights);
        ImGui::Checkbox("World Dynamic Lights", &viewport_panel.render_dynamic_lights_world);
        ImGui::Checkbox("PolyGrid Env Map", &viewport_panel.render_polygrid_env_map);
        ImGui::Checkbox("PolyGrid Bump Map", &viewport_panel.render_polygrid_bump_map);
        ImGui::Separator();
        ImGui::TextUnformatted("Editor Light Scaling");
        ImGui::DragFloat("Intensity Scale", &g_CV_DEdit2LightIntensityScale.m_Val, 0.05f, 0.0f, 10.0f);
        ImGui::DragFloat("Radius Scale", &g_CV_DEdit2LightRadiusScale.m_Val, 0.05f, 0.0f, 10.0f);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Debug"))
      {
        ImGui::Checkbox("Draw Boxes", &viewport_panel.render_model_debug_boxes);
        ImGui::Checkbox("Draw Touching Lights", &viewport_panel.render_model_debug_touching_lights);
        ImGui::Checkbox("Draw Skeleton", &viewport_panel.render_model_debug_skeleton);
        ImGui::Checkbox("Draw OBBs", &viewport_panel.render_model_debug_obbs);
        ImGui::Checkbox("Draw Vertex Normals", &viewport_panel.render_model_debug_vertex_normals);
        ImGui::Separator();
        ImGui::TextUnformatted("World");
        ImGui::Checkbox("World Nodes", &viewport_panel.render_world_debug_nodes);
        ImGui::Checkbox("World Leaves", &viewport_panel.render_world_debug_leaves);
        ImGui::Checkbox("World Portals", &viewport_panel.render_world_debug_portals);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Post FX"))
      {
        ImGui::Checkbox("Screen Glow", &viewport_panel.render_screen_glow_enable);
        ImGui::Checkbox("Show Glow Texture", &viewport_panel.render_screen_glow_show_texture);
        ImGui::SameLine();
        ImGui::DragFloat("Scale##GlowTex", &viewport_panel.render_screen_glow_show_texture_scale, 0.01f, 0.05f, 1.0f);
        ImGui::Checkbox("Show Glow Filter", &viewport_panel.render_screen_glow_show_filter);
        ImGui::SameLine();
        ImGui::DragFloat("Scale##GlowFilter", &viewport_panel.render_screen_glow_show_filter_scale, 0.01f, 0.05f, 1.0f);
        ImGui::DragFloat("Glow UV Scale", &viewport_panel.render_screen_glow_uv_scale, 0.01f, 0.1f, 2.0f);
        ImGui::DragInt("Glow Texture Size", &viewport_panel.render_screen_glow_texture_size, 1.0f, 64, 2048);
        ImGui::DragInt("Glow Filter Size", &viewport_panel.render_screen_glow_filter_size, 1.0f, 4, 128);
        ImGui::Separator();
        ImGui::TextUnformatted("SSAO");
        ImGui::Checkbox("Enable##SSAO", &viewport_panel.render_ssao_enable);
        static const char* ssao_backend_labels[] = {"Legacy", "DiligentFX"};
        ImGui::Combo("Backend##SSAO", &viewport_panel.render_ssao_backend, ssao_backend_labels, IM_ARRAYSIZE(ssao_backend_labels));
        const ImGuiSliderFlags ssao_clamp_flags = ImGuiSliderFlags_AlwaysClamp;
        ImGui::SliderFloat("Intensity##SSAO", &viewport_panel.render_ssao_intensity, 0.0f, 4.0f, "%.2f", ssao_clamp_flags);
        ImGui::SliderFloat("Effect Radius##SSAO", &viewport_panel.render_ssao_fx_effect_radius, 1.0f, 300.0f, "%.2f", ssao_clamp_flags);
        ImGui::SliderFloat("Falloff Range##SSAO", &viewport_panel.render_ssao_fx_effect_falloff_range, 0.1f, 10.0f, "%.2f", ssao_clamp_flags);
        ImGui::SliderFloat("Radius Multiplier##SSAO", &viewport_panel.render_ssao_fx_radius_multiplier, 0.1f, 4.0f, "%.2f", ssao_clamp_flags);
        ImGui::SliderFloat("Depth MIP Offset##SSAO", &viewport_panel.render_ssao_fx_depth_mip_offset, 0.0f, 8.0f, "%.2f", ssao_clamp_flags);
        ImGui::SliderFloat("Temporal Stability##SSAO", &viewport_panel.render_ssao_fx_temporal_stability, 0.0f, 0.999f, "%.3f", ssao_clamp_flags);
        ImGui::SliderFloat("Spatial Reconstruction##SSAO", &viewport_panel.render_ssao_fx_spatial_reconstruction_radius, 0.1f, 16.0f, "%.2f", ssao_clamp_flags);
        ImGui::SliderFloat("Alpha Interp##SSAO", &viewport_panel.render_ssao_fx_alpha_interpolation, 0.0f, 1.0f, "%.2f", ssao_clamp_flags);
        ImGui::Checkbox("Half Resolution##SSAO", &viewport_panel.render_ssao_fx_half_resolution);
        ImGui::Checkbox("Half Precision Depth##SSAO", &viewport_panel.render_ssao_fx_half_precision_depth);
        ImGui::Checkbox("Uniform Weighting##SSAO", &viewport_panel.render_ssao_fx_uniform_weighting);
        ImGui::Checkbox("Reset Accumulation##SSAO", &viewport_panel.render_ssao_fx_reset_accumulation);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Advanced"))
      {
        static char filter_text[128] = "";
        ImGui::InputText("Filter", filter_text, sizeof(filter_text));
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
          filter_text[0] = '\0';
        }

        std::vector<BaseConVar*> vars;
        for (BaseConVar* var = g_pConVars; var; var = var->m_pNext)
        {
          vars.push_back(var);
        }
        std::sort(vars.begin(), vars.end(), [](const BaseConVar* a, const BaseConVar* b)
        {
          return std::strcmp(a->m_pName, b->m_pName) < 0;
        });

        const std::string filter_lower = LowerCopy(filter_text);
        constexpr ImGuiTableFlags kTableFlags =
          ImGuiTableFlags_BordersInnerV |
          ImGuiTableFlags_RowBg |
          ImGuiTableFlags_ScrollY |
          ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable("RenderCvars", 3, kTableFlags, ImVec2(0.0f, 240.0f)))
        {
          ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 220.0f);
          ImGui::TableSetupColumn("Value");
          ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed, 80.0f);
          ImGui::TableHeadersRow();

          for (BaseConVar* var : vars)
          {
            if (!var || !var->m_pName)
            {
              continue;
            }
            if (!filter_lower.empty())
            {
              const std::string name_lower = LowerCopy(var->m_pName);
              if (name_lower.find(filter_lower) == std::string::npos)
              {
                continue;
              }
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(var->m_pName);

            ImGui::TableSetColumnIndex(1);
            const float current = var->GetFloat();
            const std::string var_name = var->m_pName;
            const bool is_bool =
              (current == 0.0f || current == 1.0f) &&
              (var->m_DefaultVal == 0.0f || var->m_DefaultVal == 1.0f);
            if (is_bool)
            {
              bool enabled = (current != 0.0f);
              if (ImGui::Checkbox(("##val_" + var_name).c_str(), &enabled))
              {
                var->SetFloat(enabled ? 1.0f : 0.0f);
              }
            }
            else
            {
              float value = current;
              if (ImGui::DragFloat(("##val_" + var_name).c_str(), &value, 0.1f))
              {
                var->SetFloat(value);
              }
            }

            ImGui::TableSetColumnIndex(2);
            if (ImGui::SmallButton(("Reset##" + var_name).c_str()))
            {
              var->SetFloat(var->m_DefaultVal);
            }
          }

          ImGui::EndTable();
        }

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
    ImGui::EndPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset"))
  {
    viewport_panel.initialized = false;
  }

  // Get total content area for multi-viewport layout
  const ImVec2 content_origin = ImGui::GetCursorScreenPos();
  const ImVec2 content_avail = ImGui::GetContentRegionAvail();
  const int visible_count = multi_viewport.VisibleViewportCount();

  // Sync orthographic viewports to perspective camera position
  SyncOrthoViewportsToPerspective(multi_viewport);

  // Track which viewport slot will receive 3D rendering this frame
  int rendered_slot = -1;
  ImVec2 rendered_pos{};
  ImVec2 rendered_size{};
  bool drew_image = false;

  // Render each visible viewport slot
  for (int slot = 0; slot < visible_count; ++slot)
  {
    float slot_x, slot_y, slot_w, slot_h;
    GetViewportSlotRect(
      multi_viewport.layout,
      content_origin.x, content_origin.y,
      content_avail.x, content_avail.y,
      slot,
      slot_x, slot_y, slot_w, slot_h);

    const bool is_active = (slot == multi_viewport.active_viewport);
    ViewportPanelState& slot_state = multi_viewport.viewports[slot].state;
    const char* view_mode_name = ViewModeName(slot_state.view_mode);

    // Position cursor for this slot
    ImGui::SetCursorScreenPos(ImVec2(slot_x, slot_y));

    // Create unique child window name for this slot
    char child_name[32];
    snprintf(child_name, sizeof(child_name), "##ViewportSlot%d", slot);

    // Child window flags: no scrolling, handle input ourselves
    const ImGuiWindowFlags child_flags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::BeginChild(child_name, ImVec2(slot_w, slot_h), ImGuiChildFlags_Borders, child_flags))
    {
      const ImVec2 child_pos = ImGui::GetCursorScreenPos();
      const ImVec2 child_avail = ImGui::GetContentRegionAvail();

      // ALL viewports render 3D content (not just the active one)
      const uint32_t target_width = static_cast<uint32_t>(child_avail.x > 0.0f ? child_avail.x : 0.0f);
      const uint32_t target_height = static_cast<uint32_t>(child_avail.y > 0.0f ? child_avail.y : 0.0f);

      // Sync render settings from active viewport to this slot
      SyncViewportRenderSettings(slot_state, viewport_panel);

      diligent.viewport_visible[slot] = (target_width > 0 && target_height > 0);
      bool slot_drew_image = false;
      if (diligent.viewport_visible[slot] && CreateViewportTargets(diligent, slot, target_width, target_height))
      {
        ImTextureID view_id = reinterpret_cast<ImTextureID>(diligent.viewports[slot].color_srv.RawPtr());
        ImGui::Image(view_id, child_avail);
        slot_drew_image = true;

        // Track active viewport for overlays/interaction
        if (is_active)
        {
          drew_image = true;
          rendered_slot = slot;
          rendered_pos = child_pos;
          rendered_size = child_avail;
        }

        // Handle input - active viewport gets full control, others just click-to-activate
        const bool hovered = ImGui::IsItemHovered();
        if (is_active)
        {
          UpdateViewportControls(slot_state, child_pos, child_avail, hovered);

          ViewportInteractionResult interaction = UpdateViewportInteraction(
            slot_state,
            scene_panel,
            active_target,
            scene_nodes,
            scene_props,
            child_pos,
            child_avail,
            slot_drew_image,
            hovered);

          result.overlays = interaction.overlays;
          result.hovered_scene_id = interaction.hovered_scene_id;
          result.hovered_hit_valid = interaction.hovered_hit_valid;
          result.hovered_hit_pos = interaction.hovered_hit_pos;
          result.clicked_scene_id = interaction.clicked_scene_id;
        }
        else
        {
          // Click on inactive viewport to activate it
          if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
          {
            SetActiveViewport(multi_viewport, slot);
          }
        }
      }
      else
      {
        ImGui::TextUnformatted("Viewport inactive.");
      }

      // Texture view overlay (only on active viewport)
      if (is_active && slot_drew_image)
      {
        const DEdit2TexViewState& texview = DEdit2_GetTexViewState();
        if (texview.enabled && texview.view != nullptr && texview.width > 0 && texview.height > 0)
        {
          const float max_size = std::min(256.0f, child_avail.x * 0.35f);
          const float aspect = static_cast<float>(texview.height) / static_cast<float>(texview.width);
          ImVec2 size(max_size, max_size * aspect);
          const float max_height = child_avail.y * 0.35f;
          if (size.y > max_height && max_height > 0.0f)
          {
            size.y = max_height;
            size.x = size.y / aspect;
          }

          const ImVec2 pos(
            child_pos.x + child_avail.x - size.x - 8.0f,
            child_pos.y + 8.0f);
          ImDrawList* draw_list = ImGui::GetWindowDrawList();
          draw_list->AddImage(
            reinterpret_cast<ImTextureID>(texview.view),
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y));
          draw_list->AddRect(
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(255, 255, 255, 200));
          draw_list->AddText(
            ImVec2(pos.x, pos.y + size.y + 4.0f),
            IM_COL32(255, 255, 255, 200),
            texview.name.c_str());
        }
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // Draw active viewport border highlight
    if (is_active && visible_count > 1)
    {
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      draw_list->AddRect(
        ImVec2(slot_x, slot_y),
        ImVec2(slot_x + slot_w, slot_y + slot_h),
        IM_COL32(100, 180, 255, 255),
        0.0f, 0, 2.0f);
    }

    // Draw view mode label in corner for all viewports
    {
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      const ImVec2 label_pos(slot_x + 6.0f, slot_y + 4.0f);
      draw_list->AddText(label_pos, IM_COL32(200, 200, 210, 200), view_mode_name);
    }
  }

  // Draw light icons and overlays for the active viewport (if rendered)
  if (drew_image && rendered_slot >= 0)
  {
    const ImVec2 viewport_pos = rendered_pos;
    const ImVec2 avail = rendered_size;

    // Draw light icons and overlays
    const float aspect = avail.y > 0.0f ? (avail.x / avail.y) : 1.0f;
    const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);
    Diligent::float3 cam_pos;
    Diligent::float3 cam_forward;
    Diligent::float3 cam_right;
    Diligent::float3 cam_up;
    ComputeCameraBasis(viewport_panel, cam_pos, cam_forward, cam_right, cam_up);
    (void)cam_forward;
    (void)cam_right;
    (void)cam_up;
    const size_t count = std::min(scene_nodes.size(), scene_props.size());
    const int selected_id = scene_panel.primary_selection;

    if (viewport_panel.show_light_icons)
    {
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      for (size_t i = 0; i < count; ++i)
      {
        const TreeNode& node = scene_nodes[i];
        const NodeProperties& props = scene_props[i];
        if (node.deleted || node.is_folder || !props.visible)
        {
          continue;
        }
        if (!IsLightNode(props))
        {
          continue;
        }
        if (!SceneNodePassesFilters(
          scene_panel,
          static_cast<int>(i),
          scene_nodes,
          scene_props))
        {
          continue;
        }
        if (!NodePickableByRender(viewport_panel, props))
        {
          continue;
        }

        float pick_pos[3] = {props.position[0], props.position[1], props.position[2]};
        TryGetNodePickPosition(props, pick_pos);
        ImVec2 screen_pos;
        if (!ProjectWorldToScreen(view_proj, pick_pos, avail, screen_pos))
        {
          continue;
        }
        if (viewport_panel.light_icon_occlusion &&
          IsLightIconOccluded(
            viewport_panel,
            scene_panel,
            scene_nodes,
            scene_props,
            static_cast<int>(i),
            cam_pos,
            pick_pos))
        {
          continue;
        }

        const ImVec2 center(
          viewport_pos.x + screen_pos.x,
          viewport_pos.y + screen_pos.y);
        const bool is_selected = static_cast<int>(i) == selected_id;
        const bool is_hovered = static_cast<int>(i) == result.hovered_scene_id;
        const float radius = is_selected ? 7.0f : (is_hovered ? 6.0f : 5.0f);
        const ImU32 bulb_color = is_selected ? IM_COL32(255, 210, 120, 240)
          : (is_hovered ? IM_COL32(255, 235, 160, 230) : IM_COL32(220, 200, 120, 210));
        const ImU32 base_color = is_selected ? IM_COL32(120, 110, 90, 230)
          : IM_COL32(90, 85, 70, 200);

        draw_list->AddCircleFilled(center, radius, bulb_color, 20);
        draw_list->AddCircle(center, radius, IM_COL32(30, 30, 30, 160), 20, 1.0f);
        draw_list->AddRectFilled(
          ImVec2(center.x - radius * 0.6f, center.y + radius * 0.4f),
          ImVec2(center.x + radius * 0.6f, center.y + radius * 1.2f),
          base_color,
          2.0f);
      }
    }

    if (result.hovered_scene_id >= 0)
    {
      const TreeNode& node = scene_nodes[result.hovered_scene_id];
      const NodeProperties& props = scene_props[result.hovered_scene_id];
      float pick_pos[3] = {props.position[0], props.position[1], props.position[2]};
      TryGetNodePickPosition(props, pick_pos);
      ImGui::BeginTooltip();
      ImGui::Text("%s", node.name.c_str());
      if (!props.type.empty())
      {
        ImGui::Text("Type: %s", props.type.c_str());
      }
      if (!props.class_name.empty())
      {
        ImGui::Text("Class: %s", props.class_name.c_str());
      }
      if (result.hovered_hit_valid)
      {
        ImGui::Text("Hit: %.1f %.1f %.1f", result.hovered_hit_pos.x, result.hovered_hit_pos.y, result.hovered_hit_pos.z);
      }
      ImGui::Text("Center: %.1f %.1f %.1f", pick_pos[0], pick_pos[1], pick_pos[2]);
      ImGui::EndTooltip();
    }
    DrawViewportOverlay(viewport_panel, ImGui::GetWindowDrawList(), viewport_pos, avail);
  }

  ImGui::End();
  return result;
}
