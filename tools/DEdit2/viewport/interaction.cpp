#include "viewport/interaction.h"

#include "editor_state.h"
#include "selection/depth_cycle.h"
#include "selection/marquee_selection.h"
#include "selection/selection_filter.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "undo_stack.h"
#include "viewport/scene_filters.h"
#include "viewport/lighting.h"
#include "viewport_gizmo.h"
#include "viewport_picking.h"
#include "viewport_render.h"

#include "imgui.h"

#include <algorithm>

namespace {
/// Persistent marquee state across frames.
MarqueeState g_marquee_state;

/// Persistent gizmo state for multi-selection tracking.
GizmoDrawState g_gizmo_state;
} // namespace

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
  UndoStack* undo_stack)
{
  ViewportInteractionResult result{};
  if (!drew_image)
  {
    return result;
  }

  if (hovered && active_target == SelectionTarget::Scene && HasSelection(scene_panel))
  {
    const float aspect = viewport_size.y > 0.0f ? (viewport_size.x / viewport_size.y) : 1.0f;
    const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);

    Diligent::float3 cam_pos;
    Diligent::float3 cam_forward;
    Diligent::float3 cam_right;
    Diligent::float3 cam_up;
    ComputeCameraBasis(viewport_panel, cam_pos, cam_forward, cam_right, cam_up);

    // Only rebuild gizmo geometry when not dragging (preserve start transforms during drag)
    if (!viewport_panel.gizmo_dragging)
    {
      BuildGizmoDrawStateMultiSelect(
        viewport_panel, scene_panel, scene_nodes, scene_props,
        cam_pos, view_proj, viewport_size, g_gizmo_state);
    }

    if (g_gizmo_state.origin_visible)
    {
      DrawGizmo(viewport_panel, g_gizmo_state, viewport_pos, ImGui::GetWindowDrawList());
      const ImVec2 mouse_local(
        ImGui::GetIO().MousePos.x - viewport_pos.x,
        ImGui::GetIO().MousePos.y - viewport_pos.y);
      const bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
      const bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
      if (UpdateGizmoInteractionMultiSelect(
        viewport_panel, g_gizmo_state, scene_panel, scene_nodes, scene_props,
        mouse_local, mouse_down, mouse_clicked, undo_stack))
      {
        result.gizmo_consumed_click = true;
      }
    }
  }

  if (hovered && !scene_nodes.empty() && !scene_props.empty())
  {
    const float aspect = viewport_size.y > 0.0f ? (viewport_size.x / viewport_size.y) : 1.0f;
    const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const ImVec2 local(mouse.x - viewport_pos.x, mouse.y - viewport_pos.y);
    const PickRay pick_ray = BuildPickRay(viewport_panel, viewport_size, local);
    float best_t = 1.0e30f;
    int best_id = -1;
    const float light_pick_radius = 9.0f;
    const float light_pick_radius2 = light_pick_radius * light_pick_radius;
    float best_light_dist2 = light_pick_radius2;
    int best_light_id = -1;

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
      // Skip frozen nodes - they can't be selected
      if (scene_props[i].frozen)
      {
        continue;
      }
      // Apply selection filter
      if (!selection_filter.PassesFilter(scene_props[i].type, scene_props[i].class_name))
      {
        continue;
      }
      if (IsLightNode(scene_props[i]))
      {
        float pick_pos[3] = {scene_props[i].position[0], scene_props[i].position[1], scene_props[i].position[2]};
        TryGetNodePickPosition(scene_props[i], pick_pos);
        ImVec2 screen_pos;
        if (!ProjectWorldToScreen(view_proj, pick_pos, viewport_size, screen_pos))
        {
          continue;
        }
        const float dx = screen_pos.x - local.x;
        const float dy = screen_pos.y - local.y;
        const float dist2 = dx * dx + dy * dy;
        if (dist2 <= best_light_dist2)
        {
          best_light_dist2 = dist2;
          best_light_id = static_cast<int>(i);
        }
        continue;
      }
      float hit_t = 0.0f;
      if (!RaycastNode(scene_props[i], pick_ray, hit_t))
      {
        continue;
      }
      if (hit_t < best_t)
      {
        best_t = hit_t;
        best_id = static_cast<int>(i);
      }
    }

    if (best_light_id >= 0)
    {
      result.hovered_scene_id = best_light_id;
      const NodeProperties& props = scene_props[best_light_id];
      result.hovered_hit_pos = Diligent::float3(
        props.position[0],
        props.position[1],
        props.position[2]);
      result.hovered_hit_valid = true;
    }
    else if (best_id >= 0)
    {
      result.hovered_scene_id = best_id;
      result.hovered_hit_pos = Diligent::float3(
        pick_ray.origin.x + pick_ray.dir.x * best_t,
        pick_ray.origin.y + pick_ray.dir.y * best_t,
        pick_ray.origin.z + pick_ray.dir.z * best_t);
      result.hovered_hit_valid = true;
    }
  }

  // Marquee selection handling
  const ImVec2 mouse = ImGui::GetIO().MousePos;
  const ImVec2 mouse_local(mouse.x - viewport_pos.x, mouse.y - viewport_pos.y);

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
  {
    if (!scene_nodes.empty() && !scene_props.empty() && !result.gizmo_consumed_click)
    {
      // Check if we are hovering over a light (lights use screen-space picking, not depth cycling)
      const bool hovered_is_light = result.hovered_scene_id >= 0 &&
        static_cast<size_t>(result.hovered_scene_id) < scene_props.size() &&
        IsLightNode(scene_props[result.hovered_scene_id]);

      if (hovered_is_light)
      {
        result.clicked_scene_id = result.hovered_scene_id;
        // Reset depth cycle since we clicked on a light
        depth_cycle.Reset();
      }
      else
      {
        // Use depth cycling for non-light objects
        const PickRay click_ray = BuildPickRay(viewport_panel, viewport_size, mouse_local);
        const float current_time = static_cast<float>(ImGui::GetTime());
        const int cycle_result = ProcessDepthCycleClick(
          depth_cycle, mouse_local, current_time,
          click_ray, viewport_panel, scene_panel, selection_filter,
          scene_nodes, scene_props);

        if (cycle_result >= 0)
        {
          result.clicked_scene_id = cycle_result;
        }
        else
        {
          // Clicked on empty space - start marquee
          BeginMarquee(g_marquee_state, mouse_local,
                       ImGui::GetIO().KeyShift, ImGui::GetIO().KeyAlt);
        }
      }
    }
  }

  // Set depth cycle status for status bar
  result.depth_cycle_status = GetDepthCycleStatus(depth_cycle);

  // Update marquee if active
  if (g_marquee_state.active)
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
      UpdateMarquee(g_marquee_state, mouse_local);
      DrawMarqueeRect(g_marquee_state, viewport_pos, ImGui::GetWindowDrawList());
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
      // End marquee and collect selected nodes
      result.marquee_selected_ids = EndMarquee(
          g_marquee_state, viewport_panel, scene_panel,
          scene_nodes, scene_props, selection_filter, viewport_size);
      result.marquee_additive = g_marquee_state.additive;
      result.marquee_subtractive = g_marquee_state.subtractive;
    }

    // Cancel on Escape
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      CancelMarquee(g_marquee_state);
    }
  }

  const size_t count = std::min(scene_nodes.size(), scene_props.size());
  const int selected_id = scene_panel.primary_selection;
  result.overlays.count = 0;

  // Add overlays for all selected nodes (primary selection gets brighter color)
  for (int sel_id : scene_panel.selected_ids)
  {
    if (sel_id < 0 || static_cast<size_t>(sel_id) >= count)
    {
      continue;
    }
    const TreeNode& node = scene_nodes[sel_id];
    const NodeProperties& props = scene_props[sel_id];
    if (!node.deleted && !node.is_folder &&
      SceneNodePassesFilters(scene_panel, sel_id, scene_nodes, scene_props) &&
      NodePickableByRender(viewport_panel, props))
    {
      if (result.overlays.count < ViewportOverlayState::kMaxOverlays)
      {
        // Primary selection gets brighter highlight
        const bool is_primary = (sel_id == selected_id);
        result.overlays.items[result.overlays.count++] = ViewportOverlayItem{
          &props, MakeOverlayColor(255, is_primary ? 200 : 160, 0, is_primary ? 255 : 200)};
      }
    }
  }

  if (result.hovered_scene_id >= 0 &&
    scene_panel.selected_ids.find(result.hovered_scene_id) == scene_panel.selected_ids.end())
  {
    const TreeNode& node = scene_nodes[result.hovered_scene_id];
    const NodeProperties& props = scene_props[result.hovered_scene_id];
    if (!node.deleted && !node.is_folder &&
      SceneNodePassesFilters(scene_panel, result.hovered_scene_id, scene_nodes, scene_props) &&
      NodePickableByRender(viewport_panel, props))
    {
      if (result.overlays.count < ViewportOverlayState::kMaxOverlays)
      {
        result.overlays.items[result.overlays.count++] =
          ViewportOverlayItem{&props, MakeOverlayColor(255, 255, 255, 180)};
      }
    }
  }

  // Draw light icons using the child window's draw list (after hover detection so we can highlight)
  if (viewport_panel.show_light_icons && !scene_nodes.empty() && !scene_props.empty())
  {
    const float aspect = viewport_size.y > 0.0f ? (viewport_size.x / viewport_size.y) : 1.0f;
    const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);

    Diligent::float3 cam_pos;
    Diligent::float3 cam_forward;
    Diligent::float3 cam_right;
    Diligent::float3 cam_up;
    ComputeCameraBasis(viewport_panel, cam_pos, cam_forward, cam_right, cam_up);
    (void)cam_forward;
    (void)cam_right;
    (void)cam_up;

    DrawLightIcons(
      viewport_panel, scene_panel, scene_nodes, scene_props,
      view_proj, cam_pos, viewport_pos, viewport_size,
      result.hovered_scene_id, ImGui::GetWindowDrawList());
  }

  return result;
}
