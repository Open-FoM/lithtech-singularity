#include "selection/depth_cycle.h"

#include "selection/selection_filter.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport/scene_filters.h"
#include "viewport_picking.h"
#include "viewport_render.h"

#include <algorithm>

std::vector<DepthCycleCandidate> RaycastAllNodes(
  const PickRay& ray,
  const ViewportPanelState& viewport_panel,
  const ScenePanelState& scene_panel,
  const SelectionFilter& selection_filter,
  const std::vector<TreeNode>& scene_nodes,
  const std::vector<NodeProperties>& scene_props)
{
  std::vector<DepthCycleCandidate> candidates;
  const size_t count = std::min(scene_nodes.size(), scene_props.size());

  for (size_t i = 0; i < count; ++i)
  {
    const TreeNode& node = scene_nodes[i];
    if (node.deleted || node.is_folder)
    {
      continue;
    }
    if (!SceneNodePassesFilters(scene_panel, static_cast<int>(i), scene_nodes, scene_props))
    {
      continue;
    }
    if (!NodePickableByRender(viewport_panel, scene_props[i]))
    {
      continue;
    }
    // Skip frozen nodes
    if (scene_props[i].frozen)
    {
      continue;
    }
    // Apply selection filter
    if (!selection_filter.PassesFilter(scene_props[i].type, scene_props[i].class_name))
    {
      continue;
    }

    // Handle light nodes specially - they use screen-space picking
    // and are not included in depth cycling (they're point entities)
    if (IsLightNode(scene_props[i]))
    {
      continue;
    }

    float hit_t = 0.0f;
    if (RaycastNode(scene_props[i], ray, hit_t))
    {
      candidates.push_back(DepthCycleCandidate{static_cast<int>(i), hit_t});
    }
  }

  // Sort by distance (closest first)
  std::sort(candidates.begin(), candidates.end(),
    [](const DepthCycleCandidate& a, const DepthCycleCandidate& b) {
      return a.distance < b.distance;
    });

  return candidates;
}

int ProcessDepthCycleClick(
  DepthCycleState& state,
  const ImVec2& click_pos,
  float current_time,
  const PickRay& ray,
  const ViewportPanelState& viewport_panel,
  const ScenePanelState& scene_panel,
  const SelectionFilter& selection_filter,
  const std::vector<TreeNode>& scene_nodes,
  const std::vector<NodeProperties>& scene_props)
{
  // Check if this is a repeated click at the same position within timeout
  const float dx = click_pos.x - state.last_click_pos.x;
  const float dy = click_pos.y - state.last_click_pos.y;
  const float dist2 = dx * dx + dy * dy;
  const float elapsed = current_time - state.last_click_time;

  const bool is_repeat_click = state.active &&
    dist2 < kDepthCycleClickRadius * kDepthCycleClickRadius &&
    elapsed < kDepthCycleTimeout &&
    !state.candidates.empty();

  if (is_repeat_click)
  {
    // Cycle to next candidate
    state.current_index = (state.current_index + 1) % static_cast<int>(state.candidates.size());
    state.last_click_time = current_time;
    return state.candidates[state.current_index].node_id;
  }

  // Fresh click - rebuild candidates
  state.candidates = RaycastAllNodes(
    ray, viewport_panel, scene_panel, selection_filter, scene_nodes, scene_props);
  state.current_index = 0;
  state.last_click_pos = click_pos;
  state.last_click_time = current_time;
  state.active = !state.candidates.empty();

  if (!state.candidates.empty())
  {
    return state.candidates[0].node_id;
  }

  return -1;
}
