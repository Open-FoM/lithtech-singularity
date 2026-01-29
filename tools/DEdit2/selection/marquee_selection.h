#pragma once

#include "imgui.h"

#include <vector>

struct NodeProperties;
struct ScenePanelState;
struct SelectionFilter;
struct TreeNode;
struct ViewportPanelState;

/// Selection rectangle behavior.
enum class MarqueeMode {
  Window,  ///< Object must be fully inside rectangle
  Crossing ///< Object can intersect rectangle (default)
};

/// Marquee selection state machine.
struct MarqueeState {
  bool active = false;
  ImVec2 start_pos{0.0f, 0.0f};   ///< Screen-space start position (viewport-local)
  ImVec2 current_pos{0.0f, 0.0f}; ///< Screen-space current position (viewport-local)
  MarqueeMode mode = MarqueeMode::Crossing;
  bool additive = false;   ///< Shift held - add to selection
  bool subtractive = false; ///< Alt held - remove from selection
};

/// Begin a marquee selection drag.
/// @param state Marquee state to update.
/// @param viewport_local_pos Mouse position relative to viewport origin.
/// @param shift_held True if Shift key is held (additive mode).
/// @param alt_held True if Alt key is held (subtractive mode).
void BeginMarquee(MarqueeState& state, const ImVec2& viewport_local_pos, bool shift_held, bool alt_held);

/// Update marquee during drag.
/// @param state Marquee state to update.
/// @param viewport_local_pos Current mouse position relative to viewport origin.
void UpdateMarquee(MarqueeState& state, const ImVec2& viewport_local_pos);

/// Complete marquee selection and return node IDs within the rectangle.
/// @param state Marquee state.
/// @param viewport Viewport panel state for camera info.
/// @param scene_panel Scene panel state for current selection.
/// @param nodes Scene nodes.
/// @param props Node properties.
/// @param filter Selection filter.
/// @param viewport_size Viewport dimensions.
/// @return Vector of node IDs that fall within the marquee.
[[nodiscard]] std::vector<int> EndMarquee(
    MarqueeState& state,
    const ViewportPanelState& viewport,
    const ScenePanelState& scene_panel,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props,
    const SelectionFilter& filter,
    const ImVec2& viewport_size);

/// Cancel marquee without applying selection.
void CancelMarquee(MarqueeState& state);

/// Draw the marquee rectangle overlay.
/// @param state Marquee state.
/// @param viewport_pos Screen position of viewport origin.
/// @param draw_list ImGui draw list to draw on.
void DrawMarqueeRect(const MarqueeState& state, const ImVec2& viewport_pos, ImDrawList* draw_list);

/// Test if a node is inside the marquee rectangle.
/// @param props Node properties.
/// @param viewport Viewport panel state.
/// @param viewport_size Viewport dimensions.
/// @param marquee_min Marquee rectangle minimum corner (viewport-local).
/// @param marquee_max Marquee rectangle maximum corner (viewport-local).
/// @param mode Selection mode (Window or Crossing).
/// @return True if node is selected by the marquee.
[[nodiscard]] bool NodeInMarquee(
    const NodeProperties& props,
    const ViewportPanelState& viewport,
    const ImVec2& viewport_size,
    const ImVec2& marquee_min,
    const ImVec2& marquee_max,
    MarqueeMode mode);
