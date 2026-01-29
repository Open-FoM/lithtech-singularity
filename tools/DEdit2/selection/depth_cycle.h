#pragma once

#include "imgui.h"

#include <string>
#include <vector>

struct NodeProperties;
struct ScenePanelState;
struct SelectionFilter;
struct TreeNode;
struct ViewportPanelState;
struct PickRay;

/// A candidate node for depth cycling, with distance for sorting.
struct DepthCycleCandidate
{
  int node_id = -1;
  float distance = 0.0f;
};

/// State for depth-based selection cycling.
struct DepthCycleState
{
  bool active = false;
  std::vector<DepthCycleCandidate> candidates;
  int current_index = 0;
  ImVec2 last_click_pos{0.0f, 0.0f};
  float last_click_time = 0.0f;

  /// Reset the cycling state.
  void Reset()
  {
    active = false;
    candidates.clear();
    current_index = 0;
    last_click_pos = ImVec2(0.0f, 0.0f);
    last_click_time = 0.0f;
  }
};

/// Radius in pixels for detecting repeated clicks at same position.
inline constexpr float kDepthCycleClickRadius = 8.0f;

/// Time window in seconds for depth cycling to remain active.
inline constexpr float kDepthCycleTimeout = 1.5f;

/// Raycast all nodes along the pick ray and return sorted candidates.
/// Unlike single-hit raycast, this returns ALL intersecting nodes sorted by distance.
[[nodiscard]] std::vector<DepthCycleCandidate> RaycastAllNodes(
  const PickRay& ray,
  const ViewportPanelState& viewport_panel,
  const ScenePanelState& scene_panel,
  const SelectionFilter& selection_filter,
  const std::vector<TreeNode>& scene_nodes,
  const std::vector<NodeProperties>& scene_props);

/// Process a click for depth cycling.
/// Returns the node ID to select, or -1 if no node was clicked.
/// Updates the depth cycle state for subsequent repeated clicks.
[[nodiscard]] int ProcessDepthCycleClick(
  DepthCycleState& state,
  const ImVec2& click_pos,
  float current_time,
  const PickRay& ray,
  const ViewportPanelState& viewport_panel,
  const ScenePanelState& scene_panel,
  const SelectionFilter& selection_filter,
  const std::vector<TreeNode>& scene_nodes,
  const std::vector<NodeProperties>& scene_props);

/// Get the current depth cycle status string (e.g., "2 of 5").
/// Returns empty string if depth cycling is not active.
[[nodiscard]] inline std::string GetDepthCycleStatus(const DepthCycleState& state)
{
  if (!state.active || state.candidates.size() <= 1)
  {
    return "";
  }
  return std::to_string(state.current_index + 1) + " of " + std::to_string(state.candidates.size());
}
