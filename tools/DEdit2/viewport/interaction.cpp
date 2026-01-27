#include "viewport/interaction.h"

#include "editor_state.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport/scene_filters.h"
#include "viewport_gizmo.h"
#include "viewport_picking.h"
#include "viewport_render.h"

#include "imgui.h"

#include <algorithm>

ViewportInteractionResult UpdateViewportInteraction(
  ViewportPanelState& viewport_panel,
  const ScenePanelState& scene_panel,
  SelectionTarget active_target,
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  const ImVec2& viewport_pos,
  const ImVec2& viewport_size,
  bool drew_image,
  bool hovered)
{
  ViewportInteractionResult result{};
  if (!drew_image)
  {
    return result;
  }

  if (hovered && active_target == SelectionTarget::Scene)
  {
    const int selected_id = scene_panel.primary_selection;
    const size_t count = std::min(scene_nodes.size(), scene_props.size());
    if (selected_id >= 0 && static_cast<size_t>(selected_id) < count)
    {
      const TreeNode& node = scene_nodes[selected_id];
      NodeProperties& props = scene_props[selected_id];
      if (!node.deleted && !node.is_folder && !props.frozen &&
        SceneNodePassesFilters(scene_panel, selected_id, scene_nodes, scene_props))
      {
        const float aspect = viewport_size.y > 0.0f ? (viewport_size.x / viewport_size.y) : 1.0f;
        const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);

        Diligent::float3 cam_pos;
        Diligent::float3 cam_forward;
        Diligent::float3 cam_right;
        Diligent::float3 cam_up;
        ComputeCameraBasis(viewport_panel, cam_pos, cam_forward, cam_right, cam_up);

        GizmoDrawState gizmo_state{};
        if (BuildGizmoDrawState(viewport_panel, props, cam_pos, view_proj, viewport_size, gizmo_state))
        {
          DrawGizmo(viewport_panel, gizmo_state, viewport_pos, ImGui::GetWindowDrawList());
          const ImVec2 mouse_local(
            ImGui::GetIO().MousePos.x - viewport_pos.x,
            ImGui::GetIO().MousePos.y - viewport_pos.y);
          const bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
          const bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
          if (UpdateGizmoInteraction(viewport_panel, gizmo_state, props, mouse_local, mouse_down, mouse_clicked))
          {
            result.gizmo_consumed_click = true;
          }
        }
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

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
  {
    if (!scene_nodes.empty() && !scene_props.empty())
    {
      if (!result.gizmo_consumed_click && result.hovered_scene_id >= 0)
      {
        result.clicked_scene_id = result.hovered_scene_id;
      }
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

  return result;
}
