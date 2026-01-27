#pragma once

#include <vector>

struct NodeProperties;

struct WorldRenderSettingsCache
{
  bool valid = false;
  bool fog_enabled = false;
  float fog_near = 0.0f;
  float fog_far = 0.0f;
  float far_z = 0.0f;
  float fog_color[3] = {0.0f, 0.0f, 0.0f};
};

const NodeProperties* FindWorldProperties(const std::vector<NodeProperties>& props);
void ApplyWorldSettingsToRenderer(const NodeProperties& world_props);
bool WorldSettingsDifferent(const NodeProperties& props, const WorldRenderSettingsCache& cache);
void UpdateWorldSettingsCache(const NodeProperties& props, WorldRenderSettingsCache& cache);
