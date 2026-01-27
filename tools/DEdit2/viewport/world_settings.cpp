#include "viewport/world_settings.h"

#include "editor_state.h"
#include "rendererconsolevars.h"

#include <algorithm>
#include <vector>

const NodeProperties* FindWorldProperties(const std::vector<NodeProperties>& props)
{
  for (const auto& prop : props)
  {
    if (prop.type == "World")
    {
      return &prop;
    }
  }

  return nullptr;
}

void ApplyWorldSettingsToRenderer(const NodeProperties& world_props)
{
  g_CV_FogEnable = world_props.fog_enabled ? 1 : 0;
  g_CV_FogNearZ = world_props.fog_near;
  g_CV_FogFarZ = world_props.fog_far;
  g_CV_FarZ = world_props.far_z;

  g_CV_FogColorR = static_cast<int>(std::clamp(world_props.color[0], 0.0f, 1.0f) * 255.0f);
  g_CV_FogColorG = static_cast<int>(std::clamp(world_props.color[1], 0.0f, 1.0f) * 255.0f);
  g_CV_FogColorB = static_cast<int>(std::clamp(world_props.color[2], 0.0f, 1.0f) * 255.0f);
}

bool WorldSettingsDifferent(const NodeProperties& props, const WorldRenderSettingsCache& cache)
{
  return !cache.valid ||
    cache.fog_enabled != props.fog_enabled ||
    cache.fog_near != props.fog_near ||
    cache.fog_far != props.fog_far ||
    cache.far_z != props.far_z ||
    cache.fog_color[0] != props.color[0] ||
    cache.fog_color[1] != props.color[1] ||
    cache.fog_color[2] != props.color[2];
}

void UpdateWorldSettingsCache(const NodeProperties& props, WorldRenderSettingsCache& cache)
{
  cache.valid = true;
  cache.fog_enabled = props.fog_enabled;
  cache.fog_near = props.fog_near;
  cache.fog_far = props.fog_far;
  cache.far_z = props.far_z;
  cache.fog_color[0] = props.color[0];
  cache.fog_color[1] = props.color[1];
  cache.fog_color[2] = props.color[2];
}
