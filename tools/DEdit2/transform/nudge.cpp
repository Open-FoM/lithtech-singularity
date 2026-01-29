#include "transform/nudge.h"

#include "ui_scene.h"
#include "ui_viewport.h"
#include "undo_stack.h"

#include "DiligentCore/Common/interface/BasicMath.hpp"

namespace {

/// Get the world-space nudge vector based on direction and viewport mode.
Diligent::float3 GetNudgeVector(const ViewportPanelState& viewport, NudgeDirection direction, float amount)
{
  // Default world axes for perspective view
  Diligent::float3 right(1.0f, 0.0f, 0.0f);
  Diligent::float3 up(0.0f, 1.0f, 0.0f);
  Diligent::float3 forward(0.0f, 0.0f, 1.0f);

  // Adjust axes based on view mode
  switch (viewport.view_mode)
  {
    case ViewportPanelState::ViewMode::Perspective:
      // Use camera-relative directions
      {
        const float yaw_rad = viewport.orbit_yaw * 3.14159f / 180.0f;
        right = Diligent::float3(std::cos(yaw_rad), 0.0f, -std::sin(yaw_rad));
        forward = Diligent::float3(std::sin(yaw_rad), 0.0f, std::cos(yaw_rad));
      }
      break;
    case ViewportPanelState::ViewMode::Top:
      // XZ plane, Y is depth
      right = Diligent::float3(1.0f, 0.0f, 0.0f);
      up = Diligent::float3(0.0f, 0.0f, -1.0f);
      forward = Diligent::float3(0.0f, 1.0f, 0.0f);
      break;
    case ViewportPanelState::ViewMode::Bottom:
      right = Diligent::float3(1.0f, 0.0f, 0.0f);
      up = Diligent::float3(0.0f, 0.0f, 1.0f);
      forward = Diligent::float3(0.0f, -1.0f, 0.0f);
      break;
    case ViewportPanelState::ViewMode::Front:
      // XY plane, Z is depth
      right = Diligent::float3(1.0f, 0.0f, 0.0f);
      up = Diligent::float3(0.0f, 1.0f, 0.0f);
      forward = Diligent::float3(0.0f, 0.0f, -1.0f);
      break;
    case ViewportPanelState::ViewMode::Back:
      right = Diligent::float3(-1.0f, 0.0f, 0.0f);
      up = Diligent::float3(0.0f, 1.0f, 0.0f);
      forward = Diligent::float3(0.0f, 0.0f, 1.0f);
      break;
    case ViewportPanelState::ViewMode::Left:
      // ZY plane, X is depth
      right = Diligent::float3(0.0f, 0.0f, -1.0f);
      up = Diligent::float3(0.0f, 1.0f, 0.0f);
      forward = Diligent::float3(-1.0f, 0.0f, 0.0f);
      break;
    case ViewportPanelState::ViewMode::Right:
      right = Diligent::float3(0.0f, 0.0f, 1.0f);
      up = Diligent::float3(0.0f, 1.0f, 0.0f);
      forward = Diligent::float3(1.0f, 0.0f, 0.0f);
      break;
  }

  switch (direction)
  {
    case NudgeDirection::Left:
      return Diligent::float3(-right.x * amount, -right.y * amount, -right.z * amount);
    case NudgeDirection::Right:
      return Diligent::float3(right.x * amount, right.y * amount, right.z * amount);
    case NudgeDirection::Up:
      return Diligent::float3(up.x * amount, up.y * amount, up.z * amount);
    case NudgeDirection::Down:
      return Diligent::float3(-up.x * amount, -up.y * amount, -up.z * amount);
    case NudgeDirection::Forward:
      return Diligent::float3(forward.x * amount, forward.y * amount, forward.z * amount);
    case NudgeDirection::Back:
      return Diligent::float3(-forward.x * amount, -forward.y * amount, -forward.z * amount);
  }
  return Diligent::float3(0.0f, 0.0f, 0.0f);
}

} // namespace

void NudgeSelection(
  const ViewportPanelState& viewport,
  const ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  NudgeDirection direction,
  NudgeIncrement increment,
  UndoStack* undo_stack)
{
  if (!HasSelection(scene_panel))
  {
    return;
  }

  // Calculate nudge amount based on increment
  float amount = viewport.grid_spacing;
  switch (increment)
  {
    case NudgeIncrement::Small:
      amount = 1.0f;
      break;
    case NudgeIncrement::Normal:
      amount = viewport.grid_spacing;
      break;
    case NudgeIncrement::Large:
      amount = viewport.grid_spacing * 10.0f;
      break;
  }

  const Diligent::float3 delta = GetNudgeVector(viewport, direction, amount);
  if (delta.x == 0.0f && delta.y == 0.0f && delta.z == 0.0f)
  {
    return;
  }

  // Build list of transform changes
  std::vector<TransformChange> changes;
  const size_t count = std::min(nodes.size(), props.size());

  for (int id : scene_panel.selected_ids)
  {
    if (id < 0 || static_cast<size_t>(id) >= count)
    {
      continue;
    }
    if (nodes[id].deleted || nodes[id].is_folder)
    {
      continue;
    }
    if (props[id].frozen)
    {
      continue;
    }

    TransformChange change;
    change.node_id = id;
    change.before.position[0] = props[id].position[0];
    change.before.position[1] = props[id].position[1];
    change.before.position[2] = props[id].position[2];
    change.before.rotation[0] = props[id].rotation[0];
    change.before.rotation[1] = props[id].rotation[1];
    change.before.rotation[2] = props[id].rotation[2];
    change.before.scale[0] = props[id].scale[0];
    change.before.scale[1] = props[id].scale[1];
    change.before.scale[2] = props[id].scale[2];

    // Apply nudge
    props[id].position[0] += delta.x;
    props[id].position[1] += delta.y;
    props[id].position[2] += delta.z;

    // Also nudge brush vertices if present
    if (!props[id].brush_vertices.empty())
    {
      change.before_vertices = props[id].brush_vertices;
      for (size_t v = 0; v < props[id].brush_vertices.size(); v += 3)
      {
        props[id].brush_vertices[v] += delta.x;
        props[id].brush_vertices[v + 1] += delta.y;
        props[id].brush_vertices[v + 2] += delta.z;
      }
      change.after_vertices = props[id].brush_vertices;
    }

    change.after.position[0] = props[id].position[0];
    change.after.position[1] = props[id].position[1];
    change.after.position[2] = props[id].position[2];
    change.after.rotation[0] = props[id].rotation[0];
    change.after.rotation[1] = props[id].rotation[1];
    change.after.rotation[2] = props[id].rotation[2];
    change.after.scale[0] = props[id].scale[0];
    change.after.scale[1] = props[id].scale[1];
    change.after.scale[2] = props[id].scale[2];

    changes.push_back(std::move(change));
  }

  // Push undo action
  if (undo_stack != nullptr && !changes.empty())
  {
    undo_stack->PushTransform(UndoTarget::Scene, std::move(changes));
  }
}
