#pragma once

#include "DiligentCore/Common/interface/BasicMath.hpp"

#include "viewport/overlays.h"

#include <vector>

struct DiligentContext;
struct MultiViewportState;
struct NodeProperties;
struct ScenePanelState;
struct TreeNode;
struct ViewportPanelState;

enum class SelectionTarget;

struct ViewportPanelResult
{
  ViewportOverlayState overlays;
  int hovered_scene_id = -1;
  bool hovered_hit_valid = false;
  Diligent::float3 hovered_hit_pos{};
  int clicked_scene_id = -1;
};

/// Draw the multi-viewport panel with layout support.
ViewportPanelResult DrawViewportPanel(
  DiligentContext& diligent,
  MultiViewportState& multi_viewport,
  ScenePanelState& scene_panel,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  SelectionTarget active_target);
