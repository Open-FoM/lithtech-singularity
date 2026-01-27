#pragma once

#include "DiligentCore/Common/interface/BasicMath.hpp"

#include "viewport/overlays.h"

#include <vector>

struct ImVec2;
struct NodeProperties;
struct ScenePanelState;
struct TreeNode;
struct ViewportPanelState;

enum class SelectionTarget;

struct ViewportInteractionResult
{
  ViewportOverlayState overlays;
  int hovered_scene_id = -1;
  bool hovered_hit_valid = false;
  Diligent::float3 hovered_hit_pos{};
  bool gizmo_consumed_click = false;
  int clicked_scene_id = -1;
};

ViewportInteractionResult UpdateViewportInteraction(
  ViewportPanelState& viewport_panel,
  const ScenePanelState& scene_panel,
  SelectionTarget active_target,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  bool drew_image,
  bool hovered);
