#pragma once

#include <vector>

struct NodeProperties;
struct ScenePanelState;
struct TreeNode;
struct ViewportPanelState;

void ApplySceneVisibilityToRenderer(
  const ScenePanelState& scene_state,
  const ViewportPanelState& viewport_state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);
