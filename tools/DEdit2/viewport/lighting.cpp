#include "viewport/lighting.h"

#include "editor_state.h"
#include "engine_render.h"
#include "rendererconsolevars.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport/scene_filters.h"
#include "viewport_picking.h"

#include "de_objects.h"
#include "ltbasetypes.h"
#include "ltbasedefs.h"
#include "ltrotation.h"
#include "ltvector.h"
#include "renderstruct.h"

#include <algorithm>
#include <cmath>

bool IsLightIconOccluded(
  const ViewportPanelState& viewport_state,
  const ScenePanelState& scene_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  int light_id,
  const Diligent::float3& cam_pos,
  const float light_pos[3])
{
  const Diligent::float3 light_world(light_pos[0], light_pos[1], light_pos[2]);
  const Diligent::float3 to_light = Diligent::float3(
    light_world.x - cam_pos.x,
    light_world.y - cam_pos.y,
    light_world.z - cam_pos.z);
  const float dist_sq = to_light.x * to_light.x + to_light.y * to_light.y + to_light.z * to_light.z;
  if (dist_sq <= 1.0e-4f)
  {
    return false;
  }
  const float dist = std::sqrt(dist_sq);
  const float inv_dist = 1.0f / dist;
  const Diligent::float3 dir(to_light.x * inv_dist, to_light.y * inv_dist, to_light.z * inv_dist);
  const PickRay ray{cam_pos, dir};
  const float occlusion_epsilon = 0.1f;
  const size_t count = std::min(nodes.size(), props.size());

  for (size_t i = 0; i < count; ++i)
  {
    if (static_cast<int>(i) == light_id)
    {
      continue;
    }
    const TreeNode& node = nodes[i];
    const NodeProperties& node_props = props[i];
    if (node.deleted || node.is_folder || !node_props.visible)
    {
      continue;
    }
    if (IsLightNode(node_props))
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

    float hit_t = 0.0f;
    if (RaycastNode(node_props, ray, hit_t) && hit_t > 0.0f && hit_t < dist - occlusion_epsilon)
    {
      return true;
    }
  }

  return false;
}

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
