#include "viewport/lighting.h"

#include "editor_state.h"
#include "engine_render.h"
#include "rendererconsolevars.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport/scene_filters.h"
#include "viewport_gizmo.h"
#include "viewport_picking.h"

#include "de_objects.h"
#include "ltbasetypes.h"
#include "ltbasedefs.h"
#include "ltrotation.h"
#include "ltvector.h"
#include "renderstruct.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

void BuildViewportDynamicLights(
  const ViewportPanelState& viewport_state,
  const ScenePanelState& scene_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  std::vector<DynamicLight>& out_lights)
{
  out_lights.clear();
  const size_t count = std::min(nodes.size(), props.size());
  out_lights.reserve(count);

  const float intensity_scale = std::max(0.0f, g_CV_DEdit2LightIntensityScale.m_Val);
  const float radius_scale = std::max(0.0f, g_CV_DEdit2LightRadiusScale.m_Val);
  constexpr float kIntensityReference = 5.0f;

  auto to_u8 = [](float value) -> uint8
  {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<uint8>(clamped * 255.0f + 0.5f);
  };

  for (size_t i = 0; i < count; ++i)
  {
    const TreeNode& node = nodes[i];
    const NodeProperties& node_props = props[i];
    if (node.deleted || node.is_folder || !node_props.visible)
    {
      continue;
    }
    if (!IsLightNode(node_props))
    {
      continue;
    }
    if (IsDirectionalLightNode(node_props))
    {
      continue;
    }
    if (!SceneNodePassesFilters(scene_state, static_cast<int>(i), nodes, props))
    {
      continue;
    }
    if (!NodePickableByRender(viewport_state, node_props))
    {
      continue;
    }

    const float radius = std::max(0.0f, node_props.range * radius_scale);
    const float intensity = std::max(0.0f, node_props.intensity * intensity_scale);
    const float intensity_multiplier = (kIntensityReference > 0.0f) ? (intensity / kIntensityReference) : 0.0f;
    if (radius <= 0.0f || intensity_multiplier <= 0.0f)
    {
      continue;
    }

    float pos[3] = {node_props.position[0], node_props.position[1], node_props.position[2]};
    TryGetNodePickPosition(node_props, pos);

    DynamicLight light;
    light.SetPos(LTVector(pos[0], pos[1], pos[2]));
    light.m_LightRadius = radius;
    light.SetDims(LTVector(radius, radius, radius));
    light.m_Flags = FLAG_VISIBLE;
    light.m_Flags2 |= FLAG2_FORCEDYNAMICLIGHTWORLD;
    light.m_ColorR = to_u8(node_props.color[0]);
    light.m_ColorG = to_u8(node_props.color[1]);
    light.m_ColorB = to_u8(node_props.color[2]);
    light.m_ColorA = 255;
    light.m_Intensity = intensity_multiplier;

    out_lights.push_back(light);
  }
}

void ApplyViewportDirectionalLight(
  EngineRenderContext& ctx,
  const ViewportPanelState& viewport_state,
  const ScenePanelState& scene_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props)
{
  if (!ctx.render_struct)
  {
    return;
  }

  if (!viewport_state.render_dynamic_lights_world)
  {
    ctx.render_struct->m_GlobalLightColor.Init(0.0f, 0.0f, 0.0f);
    ctx.render_struct->m_GlobalLightConvertToAmbient = 0.0f;
    return;
  }

  const size_t count = std::min(nodes.size(), props.size());
  const float intensity_scale = std::max(0.0f, g_CV_DEdit2LightIntensityScale.m_Val);
  constexpr float kIntensityReference = 5.0f;

  bool found = false;
  float best_luminance = 0.0f;
  LTVector best_dir(0.0f, -1.0f, 0.0f);
  LTVector best_color(0.0f, 0.0f, 0.0f);

  for (size_t i = 0; i < count; ++i)
  {
    const TreeNode& node = nodes[i];
    const NodeProperties& node_props = props[i];
    if (node.deleted || node.is_folder || !node_props.visible)
    {
      continue;
    }
    if (!IsDirectionalLightNode(node_props))
    {
      continue;
    }
    if (!SceneNodePassesFilters(scene_state, static_cast<int>(i), nodes, props))
    {
      continue;
    }
    if (!NodePickableByRender(viewport_state, node_props))
    {
      continue;
    }

    const float intensity = std::max(0.0f, node_props.intensity * intensity_scale);
    const float intensity_multiplier = (kIntensityReference > 0.0f) ? (intensity / kIntensityReference) : 0.0f;
    if (intensity_multiplier <= 0.0f)
    {
      continue;
    }

    LTRotation rotation(node_props.rotation[0], node_props.rotation[1], node_props.rotation[2]);
    LTVector forward = rotation.Forward();
    if (forward.MagSqr() <= 1.0e-6f)
    {
      continue;
    }
    forward.Normalize();

    LTVector color(
      node_props.color[0] * intensity_multiplier,
      node_props.color[1] * intensity_multiplier,
      node_props.color[2] * intensity_multiplier);
    const float luminance = color.x + color.y + color.z;
    if (!found || luminance > best_luminance)
    {
      found = true;
      best_luminance = luminance;
      best_dir = forward;
      best_color = color;
    }
  }

  ctx.render_struct->m_GlobalLightDir = best_dir;
  ctx.render_struct->m_GlobalLightColor = best_color;
  ctx.render_struct->m_GlobalLightConvertToAmbient = 0.0f;
}

void DrawLightIcons(
  const ViewportPanelState& viewport_state,
  const ScenePanelState& scene_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  const Diligent::float4x4& view_proj,
  const Diligent::float3& cam_pos,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  int hovered_scene_id,
  ImDrawList* draw_list)
{
  if (!viewport_state.show_light_icons || !draw_list)
  {
    return;
  }

  const size_t count = std::min(nodes.size(), props.size());
  const int selected_id = scene_state.primary_selection;

  for (size_t i = 0; i < count; ++i)
  {
    const TreeNode& node = nodes[i];
    const NodeProperties& node_props = props[i];

    // Skip deleted, folder, and invisible nodes
    if (node.deleted || node.is_folder || !node_props.visible)
    {
      continue;
    }
    // Only draw lights
    if (!IsLightNode(node_props))
    {
      continue;
    }
    // Skip frozen nodes
    if (node_props.frozen)
    {
      continue;
    }
    // Check visibility filters
    if (!SceneNodePassesFilters(scene_state, static_cast<int>(i), nodes, props))
    {
      continue;
    }
    // Check render pickability
    if (!NodePickableByRender(viewport_state, node_props))
    {
      continue;
    }

    // Get pick position
    float pick_pos[3] = {node_props.position[0], node_props.position[1], node_props.position[2]};
    TryGetNodePickPosition(node_props, pick_pos);

    // Project to screen
    ImVec2 screen_pos;
    if (!ProjectWorldToScreen(view_proj, pick_pos, viewport_size, screen_pos))
    {
      continue;
    }

    // Compute final screen position (viewport_pos + local screen position)
    const ImVec2 center(viewport_pos.x + screen_pos.x, viewport_pos.y + screen_pos.y);

    // Determine visual state
    const bool is_selected = static_cast<int>(i) == selected_id ||
      scene_state.selected_ids.find(static_cast<int>(i)) != scene_state.selected_ids.end();
    const bool is_hovered = static_cast<int>(i) == hovered_scene_id;
    const float radius = is_selected ? 7.0f : (is_hovered ? 6.0f : 5.0f);

    // Colors
    const ImU32 bulb_color = is_selected ? IM_COL32(255, 210, 120, 240)
      : (is_hovered ? IM_COL32(255, 235, 160, 230) : IM_COL32(220, 200, 120, 210));
    const ImU32 base_color = is_selected ? IM_COL32(120, 110, 90, 230)
      : IM_COL32(90, 85, 70, 200);

    // Draw light bulb icon
    draw_list->AddCircleFilled(center, radius, bulb_color, 20);
    draw_list->AddCircle(center, radius, IM_COL32(30, 30, 30, 160), 20, 1.0f);
    draw_list->AddRectFilled(
      ImVec2(center.x - radius * 0.6f, center.y + radius * 0.4f),
      ImVec2(center.x + radius * 0.6f, center.y + radius * 1.2f),
      base_color,
      2.0f);
  }
}
