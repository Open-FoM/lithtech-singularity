#pragma once

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "imgui.h"

#include "viewport/overlays.h"

#include <string>
#include <vector>

struct DepthCycleState;
struct DiligentContext;
struct MultiViewportState;
struct NodeProperties;
struct ScenePanelState;
struct SelectionFilter;
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

  /// Marquee selection results (filled when marquee ends).
  std::vector<int> marquee_selected_ids;
  bool marquee_additive = false;   ///< True if marquee was Shift+drag
  bool marquee_subtractive = false; ///< True if marquee was Alt+drag

  /// Depth cycle status (e.g., "2 of 5" when cycling through overlapping objects).
  std::string depth_cycle_status;

  /// Active viewport position and size (for ray building).
  ImVec2 active_viewport_pos{};
  ImVec2 active_viewport_size{};
  bool active_viewport_hovered = false;
};

class UndoStack;

/// Draw the multi-viewport panel with layout support.
ViewportPanelResult DrawViewportPanel(
  DiligentContext& diligent,
  MultiViewportState& multi_viewport,
  ScenePanelState& scene_panel,
  const SelectionFilter& selection_filter,
  DepthCycleState& depth_cycle,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  SelectionTarget active_target,
  UndoStack* undo_stack);
