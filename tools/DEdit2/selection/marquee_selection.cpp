#include "selection/marquee_selection.h"

#include "editor_state.h"
#include "selection/selection_filter.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport_picking.h"
#include "viewport_render.h"
#include "viewport/scene_filters.h"

#include <algorithm>
#include <cmath>

namespace {

/// Project a 3D world position to 2D screen space.
/// Returns false if the point is behind the camera.
bool ProjectToScreen(
    const Diligent::float4x4& view_proj,
    const float pos[3],
    const ImVec2& viewport_size,
    ImVec2& out_screen)
{
  Diligent::float4 clip = Diligent::float4(pos[0], pos[1], pos[2], 1.0f) * view_proj;

  // Behind camera check
  if (clip.w <= 0.0f) {
    return false;
  }

  // Perspective divide
  const float inv_w = 1.0f / clip.w;
  const float ndc_x = clip.x * inv_w;
  const float ndc_y = clip.y * inv_w;

  // NDC to screen (flip Y for ImGui coordinate system)
  out_screen.x = (ndc_x * 0.5f + 0.5f) * viewport_size.x;
  out_screen.y = (1.0f - (ndc_y * 0.5f + 0.5f)) * viewport_size.y;

  return true;
}

/// Test if a 2D rectangle intersects another rectangle.
bool Rect2DIntersects(
    const ImVec2& a_min, const ImVec2& a_max,
    const ImVec2& b_min, const ImVec2& b_max)
{
  return !(a_max.x < b_min.x || a_min.x > b_max.x ||
           a_max.y < b_min.y || a_min.y > b_max.y);
}

/// Test if rectangle A is fully contained within rectangle B.
bool Rect2DContains(
    const ImVec2& outer_min, const ImVec2& outer_max,
    const ImVec2& inner_min, const ImVec2& inner_max)
{
  return inner_min.x >= outer_min.x && inner_max.x <= outer_max.x &&
         inner_min.y >= outer_min.y && inner_max.y <= outer_max.y;
}

} // namespace

void BeginMarquee(MarqueeState& state, const ImVec2& viewport_local_pos, bool shift_held, bool alt_held) {
  state.active = true;
  state.start_pos = viewport_local_pos;
  state.current_pos = viewport_local_pos;
  state.additive = shift_held;
  state.subtractive = alt_held;
}

void UpdateMarquee(MarqueeState& state, const ImVec2& viewport_local_pos) {
  if (!state.active) {
    return;
  }
  state.current_pos = viewport_local_pos;
}

std::vector<int> EndMarquee(
    MarqueeState& state,
    const ViewportPanelState& viewport,
    const ScenePanelState& scene_panel,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props,
    const SelectionFilter& filter,
    const ImVec2& viewport_size)
{
  std::vector<int> result;

  if (!state.active) {
    return result;
  }

  // Compute normalized marquee rectangle
  const ImVec2 marquee_min(
      std::min(state.start_pos.x, state.current_pos.x),
      std::min(state.start_pos.y, state.current_pos.y));
  const ImVec2 marquee_max(
      std::max(state.start_pos.x, state.current_pos.x),
      std::max(state.start_pos.y, state.current_pos.y));

  // Skip if marquee is too small (just a click)
  const float width = marquee_max.x - marquee_min.x;
  const float height = marquee_max.y - marquee_min.y;
  if (width < 4.0f && height < 4.0f) {
    state.active = false;
    return result;
  }

  const size_t count = std::min(nodes.size(), props.size());
  for (size_t i = 0; i < count; ++i) {
    const TreeNode& node = nodes[i];
    const NodeProperties& node_props = props[i];

    // Skip deleted, folder, and frozen nodes
    if (node.deleted || node.is_folder || node_props.frozen) {
      continue;
    }

    // Check visibility filters
    if (!SceneNodePassesFilters(scene_panel, static_cast<int>(i), nodes, props)) {
      continue;
    }

    // Check selection filter
    if (!filter.PassesFilter(node_props.type, node_props.class_name)) {
      continue;
    }

    // Check if node is in marquee
    if (NodeInMarquee(node_props, viewport, viewport_size, marquee_min, marquee_max, state.mode)) {
      result.push_back(static_cast<int>(i));
    }
  }

  state.active = false;
  return result;
}

void CancelMarquee(MarqueeState& state) {
  state.active = false;
}

void DrawMarqueeRect(const MarqueeState& state, const ImVec2& viewport_pos, ImDrawList* draw_list) {
  if (!state.active) {
    return;
  }

  // Convert to screen coordinates
  const ImVec2 screen_start(viewport_pos.x + state.start_pos.x, viewport_pos.y + state.start_pos.y);
  const ImVec2 screen_current(viewport_pos.x + state.current_pos.x, viewport_pos.y + state.current_pos.y);

  const ImVec2 rect_min(std::min(screen_start.x, screen_current.x), std::min(screen_start.y, screen_current.y));
  const ImVec2 rect_max(std::max(screen_start.x, screen_current.x), std::max(screen_start.y, screen_current.y));

  // Draw semi-transparent fill
  ImU32 fill_color;
  if (state.subtractive) {
    fill_color = IM_COL32(255, 100, 100, 30); // Red tint for subtractive
  } else if (state.additive) {
    fill_color = IM_COL32(100, 255, 100, 30); // Green tint for additive
  } else {
    fill_color = IM_COL32(100, 150, 255, 30); // Blue tint for replace
  }
  draw_list->AddRectFilled(rect_min, rect_max, fill_color);

  // Draw outline
  const ImU32 outline_color = IM_COL32(255, 255, 255, 200);
  draw_list->AddRect(rect_min, rect_max, outline_color, 0.0f, 0, 1.0f);

  // Draw dashed corners for visual feedback
  const float corner_len = 8.0f;
  draw_list->AddLine(rect_min, ImVec2(rect_min.x + corner_len, rect_min.y), IM_COL32(255, 255, 255, 255), 2.0f);
  draw_list->AddLine(rect_min, ImVec2(rect_min.x, rect_min.y + corner_len), IM_COL32(255, 255, 255, 255), 2.0f);
  draw_list->AddLine(ImVec2(rect_max.x, rect_min.y), ImVec2(rect_max.x - corner_len, rect_min.y), IM_COL32(255, 255, 255, 255), 2.0f);
  draw_list->AddLine(ImVec2(rect_max.x, rect_min.y), ImVec2(rect_max.x, rect_min.y + corner_len), IM_COL32(255, 255, 255, 255), 2.0f);
  draw_list->AddLine(rect_max, ImVec2(rect_max.x - corner_len, rect_max.y), IM_COL32(255, 255, 255, 255), 2.0f);
  draw_list->AddLine(rect_max, ImVec2(rect_max.x, rect_max.y - corner_len), IM_COL32(255, 255, 255, 255), 2.0f);
  draw_list->AddLine(ImVec2(rect_min.x, rect_max.y), ImVec2(rect_min.x + corner_len, rect_max.y), IM_COL32(255, 255, 255, 255), 2.0f);
  draw_list->AddLine(ImVec2(rect_min.x, rect_max.y), ImVec2(rect_min.x, rect_max.y - corner_len), IM_COL32(255, 255, 255, 255), 2.0f);
}

bool NodeInMarquee(
    const NodeProperties& props,
    const ViewportPanelState& viewport,
    const ImVec2& viewport_size,
    const ImVec2& marquee_min,
    const ImVec2& marquee_max,
    MarqueeMode mode)
{
  // Get node bounds
  float bounds_min[3], bounds_max[3];
  if (!TryGetNodeBounds(props, bounds_min, bounds_max)) {
    // Fall back to position if no bounds
    bounds_min[0] = bounds_max[0] = props.position[0];
    bounds_min[1] = bounds_max[1] = props.position[1];
    bounds_min[2] = bounds_max[2] = props.position[2];
  }

  // Compute view-projection matrix
  const float aspect = viewport_size.y > 0.0f ? (viewport_size.x / viewport_size.y) : 1.0f;
  const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport, aspect);

  // Project all 8 corners of the bounding box to screen space
  const float corners[8][3] = {
      {bounds_min[0], bounds_min[1], bounds_min[2]},
      {bounds_max[0], bounds_min[1], bounds_min[2]},
      {bounds_min[0], bounds_max[1], bounds_min[2]},
      {bounds_max[0], bounds_max[1], bounds_min[2]},
      {bounds_min[0], bounds_min[1], bounds_max[2]},
      {bounds_max[0], bounds_min[1], bounds_max[2]},
      {bounds_min[0], bounds_max[1], bounds_max[2]},
      {bounds_max[0], bounds_max[1], bounds_max[2]}};

  ImVec2 screen_min(1e30f, 1e30f);
  ImVec2 screen_max(-1e30f, -1e30f);
  int visible_count = 0;

  for (int i = 0; i < 8; ++i) {
    ImVec2 screen_pos;
    if (ProjectToScreen(view_proj, corners[i], viewport_size, screen_pos)) {
      screen_min.x = std::min(screen_min.x, screen_pos.x);
      screen_min.y = std::min(screen_min.y, screen_pos.y);
      screen_max.x = std::max(screen_max.x, screen_pos.x);
      screen_max.y = std::max(screen_max.y, screen_pos.y);
      ++visible_count;
    }
  }

  // If no corners are visible, the object is behind the camera
  if (visible_count == 0) {
    return false;
  }

  // Test based on mode
  if (mode == MarqueeMode::Window) {
    // All corners must be inside the marquee
    return Rect2DContains(marquee_min, marquee_max, screen_min, screen_max);
  } else {
    // Crossing mode - any overlap counts
    return Rect2DIntersects(marquee_min, marquee_max, screen_min, screen_max);
  }
}
