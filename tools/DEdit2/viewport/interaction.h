#pragma once

#include "DiligentCore/Common/interface/BasicMath.hpp"

#include "selection/marquee_selection.h"
#include "viewport/overlays.h"

#include <string>
#include <vector>

class UndoStack;
struct DepthCycleState;
struct ImVec2;
struct NodeProperties;
struct ScenePanelState;
struct SelectionFilter;
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

  /// Marquee selection results (filled when marquee ends).
  std::vector<int> marquee_selected_ids;
  bool marquee_additive = false;   ///< True if marquee was Shift+drag
  bool marquee_subtractive = false; ///< True if marquee was Alt+drag

  /// Depth cycle status (e.g., "2 of 5" when cycling through overlapping objects).
  std::string depth_cycle_status;
};

ViewportInteractionResult UpdateViewportInteraction(
  ViewportPanelState& viewport_panel,
  const ScenePanelState& scene_panel,
  const SelectionFilter& selection_filter,
  DepthCycleState& depth_cycle,
  SelectionTarget active_target,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  bool drew_image,
  bool hovered,
  UndoStack* undo_stack);
