#pragma once

#include <vector>

#include "DiligentCore/Common/interface/BasicMath.hpp"

struct DynamicLight;
struct EngineRenderContext;
struct ImDrawList;
struct ImVec2;
struct NodeProperties;
struct ScenePanelState;
struct TreeNode;
struct ViewportPanelState;

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

/// Draw light icons in viewport.
/// Called from inside the child window context to ensure correct coordinate system.
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
  ImDrawList* draw_list);
