#pragma once

#include "ui_viewport.h"

#include <array>
#include <cstdint>

/// Maximum number of viewports supported.
constexpr int kMaxViewports = 4;

/// Viewport layout configuration.
enum class ViewportLayout
{
  Single,       ///< Single viewport fills entire area
  TwoVertical,  ///< Two viewports side by side (left/right)
  TwoHorizontal,///< Two viewports stacked (top/bottom)
  ThreeLeft,    ///< Large left viewport, two small on right
  ThreeTop,     ///< Large top viewport, two small on bottom
  Quad          ///< 2x2 grid of viewports
};

/// Individual viewport instance within the multi-viewport system.
struct ViewportInstance
{
  ViewportPanelState state;
  bool visible = true;   ///< Whether this viewport is part of current layout
  int layout_slot = 0;   ///< Slot index in current layout (0-3)
};

/// Multi-viewport system state.
struct MultiViewportState
{
  std::array<ViewportInstance, kMaxViewports> viewports;
  int active_viewport = 0;   ///< Index of the currently active viewport
  ViewportLayout layout = ViewportLayout::Single;

  /// Last known perspective camera target position.
  /// Used to sync orthographic viewports when no perspective view is active.
  float last_perspective_target[3] = {0.0f, 0.0f, 0.0f};

  /// Returns the currently active viewport state.
  [[nodiscard]] ViewportPanelState& ActiveViewport()
  {
    return viewports[active_viewport].state;
  }

  [[nodiscard]] const ViewportPanelState& ActiveViewport() const
  {
    return viewports[active_viewport].state;
  }

  /// Returns number of visible viewports for current layout.
  [[nodiscard]] int VisibleViewportCount() const;
};

/// Returns display name for layout.
[[nodiscard]] const char* LayoutName(ViewportLayout layout);

/// Initialize multi-viewport state with default configuration.
/// Sets up classic 4-viewport layout: Perspective, Top, Front, Right.
void InitMultiViewport(MultiViewportState& state);

/// Set the viewport layout and update visibility flags.
void SetViewportLayout(MultiViewportState& state, ViewportLayout layout);

/// Activate a viewport by index (clamped to valid range).
void SetActiveViewport(MultiViewportState& state, int index);

/// Cycle to the next visible viewport.
void CycleActiveViewport(MultiViewportState& state);

/// Synchronize orthographic viewports to the perspective camera position.
/// Finds the first perspective viewport and centers all ortho viewports on its target.
/// If no perspective viewport exists, uses the last known perspective target.
void SyncOrthoViewportsToPerspective(MultiViewportState& state);

/// Returns the viewport rectangle for a given slot in the current layout.
/// @param area Total available area for all viewports.
/// @param slot Slot index (0-3).
/// @param out_x Output X position.
/// @param out_y Output Y position.
/// @param out_width Output width.
/// @param out_height Output height.
void GetViewportSlotRect(
    ViewportLayout layout,
    float area_x, float area_y, float area_width, float area_height,
    int slot,
    float& out_x, float& out_y, float& out_width, float& out_height);
