#pragma once

#include <vector>

#include "DiligentCore/Common/interface/BasicMath.hpp"

struct DynamicLight;
struct EngineRenderContext;
struct NodeProperties;
struct ScenePanelState;
struct TreeNode;
struct ViewportPanelState;

bool IsLightIconOccluded(
  const ViewportPanelState& viewport_state,
  const ScenePanelState& scene_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  int light_id,
  const Diligent::float3& cam_pos,
  const float light_pos[3]);

void BuildViewportDynamicLights(
  const ViewportPanelState& viewport_state,
  const ScenePanelState& scene_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  std::vector<DynamicLight>& out_lights);

void ApplyViewportDirectionalLight(
  EngineRenderContext& ctx,
  const ViewportPanelState& viewport_state,
  const ScenePanelState& scene_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);
