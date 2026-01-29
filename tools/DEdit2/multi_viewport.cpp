#include "multi_viewport.h"

#include <algorithm>

int MultiViewportState::VisibleViewportCount() const
{
  switch (layout)
  {
  case ViewportLayout::Single:
    return 1;
  case ViewportLayout::TwoVertical:
  case ViewportLayout::TwoHorizontal:
    return 2;
  case ViewportLayout::ThreeLeft:
  case ViewportLayout::ThreeTop:
    return 3;
  case ViewportLayout::Quad:
    return 4;
  }
  return 1;
}

const char* LayoutName(ViewportLayout layout)
{
  switch (layout)
  {
  case ViewportLayout::Single:
    return "Single";
  case ViewportLayout::TwoVertical:
    return "Two Vertical";
  case ViewportLayout::TwoHorizontal:
    return "Two Horizontal";
  case ViewportLayout::ThreeLeft:
    return "Three (Left)";
  case ViewportLayout::ThreeTop:
    return "Three (Top)";
  case ViewportLayout::Quad:
    return "Quad (2x2)";
  }
  return "Unknown";
}

void InitMultiViewport(MultiViewportState& state)
{
  // Initialize all viewports
  for (int i = 0; i < kMaxViewports; ++i)
  {
    state.viewports[i].state = ViewportPanelState{};
    state.viewports[i].layout_slot = i;
    state.viewports[i].visible = false;
  }

  // Set up classic DEdit 4-viewport configuration:
  // Slot 0: Perspective (main 3D view)
  // Slot 1: Top (looking down Y axis)
  // Slot 2: Front (looking down Z axis)
  // Slot 3: Right (looking down X axis)

  state.viewports[0].state.view_mode = ViewportPanelState::ViewMode::Perspective;
  state.viewports[0].visible = true;

  state.viewports[1].state.view_mode = ViewportPanelState::ViewMode::Top;
  state.viewports[1].state.ortho_zoom = 1.0f;
  state.viewports[1].visible = true;

  state.viewports[2].state.view_mode = ViewportPanelState::ViewMode::Front;
  state.viewports[2].state.ortho_zoom = 1.0f;
  state.viewports[2].visible = true;

  state.viewports[3].state.view_mode = ViewportPanelState::ViewMode::Right;
  state.viewports[3].state.ortho_zoom = 1.0f;
  state.viewports[3].visible = true;

  state.active_viewport = 0;
  state.layout = ViewportLayout::Single; // Start with single viewport
}

void SetViewportLayout(MultiViewportState& state, ViewportLayout layout)
{
  state.layout = layout;

  const int visible_count = state.VisibleViewportCount();

  // Update visibility based on layout
  for (int i = 0; i < kMaxViewports; ++i)
  {
    state.viewports[i].visible = (i < visible_count);
  }

  // Clamp active viewport to visible range
  if (state.active_viewport >= visible_count)
  {
    state.active_viewport = visible_count - 1;
  }
}

void SetActiveViewport(MultiViewportState& state, int index)
{
  const int visible_count = state.VisibleViewportCount();
  state.active_viewport = std::clamp(index, 0, visible_count - 1);
}

void CycleActiveViewport(MultiViewportState& state)
{
  const int visible_count = state.VisibleViewportCount();
  if (visible_count <= 1)
  {
    return;
  }
  state.active_viewport = (state.active_viewport + 1) % visible_count;
}

void SyncOrthoViewportsToPerspective(MultiViewportState& state)
{
  const int visible_count = state.VisibleViewportCount();
  if (visible_count <= 1)
  {
    return;
  }

  // Find the first perspective viewport and get its target position
  float sync_target[3] = {
    state.last_perspective_target[0],
    state.last_perspective_target[1],
    state.last_perspective_target[2]
  };
  bool found_perspective = false;

  for (int i = 0; i < visible_count; ++i)
  {
    const ViewportPanelState& vp = state.viewports[i].state;
    if (vp.view_mode == ViewportPanelState::ViewMode::Perspective)
    {
      // Use target for orbit mode, or fly_pos for fly mode
      if (vp.fly_mode)
      {
        sync_target[0] = vp.fly_pos[0];
        sync_target[1] = vp.fly_pos[1];
        sync_target[2] = vp.fly_pos[2];
      }
      else
      {
        sync_target[0] = vp.target[0];
        sync_target[1] = vp.target[1];
        sync_target[2] = vp.target[2];
      }
      found_perspective = true;
      break;
    }
  }

  // Update last known perspective target if we found one
  if (found_perspective)
  {
    state.last_perspective_target[0] = sync_target[0];
    state.last_perspective_target[1] = sync_target[1];
    state.last_perspective_target[2] = sync_target[2];
  }

  // Sync all orthographic viewports to this position
  // Scale down depth offset to keep ortho views closer to ground level
  // (prevents exceeding draw distance while still allowing zoom adjustment)
  constexpr float kDepthScale = 0.2f;

  for (int i = 0; i < visible_count; ++i)
  {
    ViewportPanelState& vp = state.viewports[i].state;

    // Map 3D target to 2D ortho_center based on view mode
    // ortho_center[0] = horizontal axis, ortho_center[1] = vertical axis
    // ortho_depth = depth into screen (scaled down to stay within draw distance)
    switch (vp.view_mode)
    {
    case ViewportPanelState::ViewMode::Top:
      // Looking down -Y: screen X = world X, screen Y = world -Z
      vp.ortho_center[0] = sync_target[0];
      vp.ortho_center[1] = -sync_target[2];
      vp.ortho_depth = sync_target[1] * kDepthScale;
      break;

    case ViewportPanelState::ViewMode::Bottom:
      // Looking up +Y: screen X = world X, screen Y = world Z
      vp.ortho_center[0] = sync_target[0];
      vp.ortho_center[1] = sync_target[2];
      vp.ortho_depth = sync_target[1] * kDepthScale;
      break;

    case ViewportPanelState::ViewMode::Front:
      // Looking down -Z: screen X = world X, screen Y = world Y
      vp.ortho_center[0] = sync_target[0];
      vp.ortho_center[1] = sync_target[1];
      vp.ortho_depth = sync_target[2] * kDepthScale;
      break;

    case ViewportPanelState::ViewMode::Back:
      // Looking down +Z: screen X = world -X, screen Y = world Y
      vp.ortho_center[0] = -sync_target[0];
      vp.ortho_center[1] = sync_target[1];
      vp.ortho_depth = sync_target[2] * kDepthScale;
      break;

    case ViewportPanelState::ViewMode::Left:
      // Looking down +X: screen X = world Z, screen Y = world Y
      vp.ortho_center[0] = sync_target[2];
      vp.ortho_center[1] = sync_target[1];
      vp.ortho_depth = sync_target[0] * kDepthScale;
      break;

    case ViewportPanelState::ViewMode::Right:
      // Looking down -X: screen X = world -Z, screen Y = world Y
      vp.ortho_center[0] = -sync_target[2];
      vp.ortho_center[1] = sync_target[1];
      vp.ortho_depth = sync_target[0] * kDepthScale;
      break;

    case ViewportPanelState::ViewMode::Perspective:
      // Don't modify perspective viewports
      break;
    }
  }
}

void GetViewportSlotRect(
    ViewportLayout layout,
    float area_x, float area_y, float area_width, float area_height,
    int slot,
    float splitter_h, float splitter_v,
    float& out_x, float& out_y, float& out_width, float& out_height)
{
  // Default: full area
  out_x = area_x;
  out_y = area_y;
  out_width = area_width;
  out_height = area_height;

  // Small gap between viewports
  constexpr float gap = 2.0f;

  // Clamp splitters to valid range
  splitter_h = std::clamp(splitter_h, 0.15f, 0.85f);
  splitter_v = std::clamp(splitter_v, 0.15f, 0.85f);

  switch (layout)
  {
  case ViewportLayout::Single:
    // Slot 0 takes entire area
    break;

  case ViewportLayout::TwoVertical:
    // Two side-by-side viewports
    {
      const float left_width = (area_width - gap) * splitter_h;
      const float right_width = area_width - left_width - gap;
      if (slot == 0)
      {
        // Left viewport
        out_width = left_width;
      }
      else if (slot == 1)
      {
        // Right viewport
        out_x = area_x + left_width + gap;
        out_width = right_width;
      }
    }
    break;

  case ViewportLayout::TwoHorizontal:
    // Two stacked viewports
    {
      const float top_height = (area_height - gap) * splitter_v;
      const float bottom_height = area_height - top_height - gap;
      if (slot == 0)
      {
        // Top viewport
        out_height = top_height;
      }
      else if (slot == 1)
      {
        // Bottom viewport
        out_y = area_y + top_height + gap;
        out_height = bottom_height;
      }
    }
    break;

  case ViewportLayout::ThreeLeft:
    // Large left viewport (slot 0), two small on right (slots 1, 2)
    {
      const float left_width = (area_width - gap) * splitter_h;
      const float right_width = area_width - left_width - gap;
      const float top_height = (area_height - gap) * splitter_v;
      const float bottom_height = area_height - top_height - gap;

      if (slot == 0)
      {
        // Large left viewport
        out_width = left_width;
      }
      else if (slot == 1)
      {
        // Top right viewport
        out_x = area_x + left_width + gap;
        out_width = right_width;
        out_height = top_height;
      }
      else if (slot == 2)
      {
        // Bottom right viewport
        out_x = area_x + left_width + gap;
        out_y = area_y + top_height + gap;
        out_width = right_width;
        out_height = bottom_height;
      }
    }
    break;

  case ViewportLayout::ThreeTop:
    // Large top viewport (slot 0), two small on bottom (slots 1, 2)
    {
      const float top_height = (area_height - gap) * splitter_v;
      const float bottom_height = area_height - top_height - gap;
      const float left_width = (area_width - gap) * splitter_h;
      const float right_width = area_width - left_width - gap;

      if (slot == 0)
      {
        // Large top viewport
        out_height = top_height;
      }
      else if (slot == 1)
      {
        // Bottom left viewport
        out_y = area_y + top_height + gap;
        out_width = left_width;
        out_height = bottom_height;
      }
      else if (slot == 2)
      {
        // Bottom right viewport
        out_x = area_x + left_width + gap;
        out_y = area_y + top_height + gap;
        out_width = right_width;
        out_height = bottom_height;
      }
    }
    break;

  case ViewportLayout::Quad:
    // 2x2 grid with adjustable splitters
    {
      const float left_width = (area_width - gap) * splitter_h;
      const float right_width = area_width - left_width - gap;
      const float top_height = (area_height - gap) * splitter_v;
      const float bottom_height = area_height - top_height - gap;

      switch (slot)
      {
      case 0: // Top left
        out_width = left_width;
        out_height = top_height;
        break;
      case 1: // Top right
        out_x = area_x + left_width + gap;
        out_width = right_width;
        out_height = top_height;
        break;
      case 2: // Bottom left
        out_y = area_y + top_height + gap;
        out_width = left_width;
        out_height = bottom_height;
        break;
      case 3: // Bottom right
        out_x = area_x + left_width + gap;
        out_y = area_y + top_height + gap;
        out_width = right_width;
        out_height = bottom_height;
        break;
      default:
        break;
      }
    }
    break;
  }
}

void ToggleMaximizeViewport(MultiViewportState& state, int slot)
{
  if (state.maximized)
  {
    // Swap the viewport back to its original slot before restoring layout
    if (state.maximized_slot != 0 && state.maximized_slot > 0 && state.maximized_slot < kMaxViewports)
    {
      std::swap(state.viewports[0], state.viewports[state.maximized_slot]);
    }

    // Restore previous layout
    state.layout = state.pre_maximize_layout;
    state.maximized = false;
    state.maximized_slot = -1;

    // Update visibility
    const int visible_count = state.VisibleViewportCount();
    for (int i = 0; i < kMaxViewports; ++i)
    {
      state.viewports[i].visible = (i < visible_count);
    }
  }
  else
  {
    // Save current layout and maximize
    state.pre_maximize_layout = state.layout;
    state.maximized_slot = slot;
    state.maximized = true;

    // Switch to single viewport showing the specified slot
    state.layout = ViewportLayout::Single;
    state.active_viewport = 0;

    // Swap the maximized viewport to slot 0
    if (slot != 0)
    {
      std::swap(state.viewports[0], state.viewports[slot]);
    }

    // Update visibility
    state.viewports[0].visible = true;
    for (int i = 1; i < kMaxViewports; ++i)
    {
      state.viewports[i].visible = false;
    }
  }
}

void ResetSplitters(MultiViewportState& state)
{
  state.splitter_h = 0.5f;
  state.splitter_v = 0.5f;
}
