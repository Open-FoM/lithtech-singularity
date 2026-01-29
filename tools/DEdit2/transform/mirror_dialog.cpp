#include "transform/mirror_dialog.h"

#include "grouping/node_grouping.h"
#include "ui_scene.h"
#include "undo_stack.h"

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace {

/// Reverses the winding order of a triangle mesh to fix inside-out faces after mirroring.
void ReverseTriangleWinding(std::vector<uint32_t>& indices)
{
  for (size_t i = 0; i + 2 < indices.size(); i += 3)
  {
    std::swap(indices[i + 1], indices[i + 2]);
  }
}

} // namespace

MirrorDialogResult DrawMirrorDialog(
  MirrorDialogState& dialog_state,
  ScenePanelState& scene_panel,
  std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  UndoStack* undo_stack)
{
  MirrorDialogResult result{};

  if (!dialog_state.open)
  {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Mirror Selection", &dialog_state.open))
  {
    ImGui::Text("Mirror Axis");
    if (ImGui::RadioButton("X", dialog_state.axis == MirrorAxis::X))
    {
      dialog_state.axis = MirrorAxis::X;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Y", dialog_state.axis == MirrorAxis::Y))
    {
      dialog_state.axis = MirrorAxis::Y;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Z", dialog_state.axis == MirrorAxis::Z))
    {
      dialog_state.axis = MirrorAxis::Z;
    }

    ImGui::Separator();

    ImGui::Text("Pivot");
    if (ImGui::RadioButton("Selection Center", dialog_state.pivot_mode == MirrorPivotMode::SelectionCenter))
    {
      dialog_state.pivot_mode = MirrorPivotMode::SelectionCenter;
    }
    if (ImGui::RadioButton("World Origin", dialog_state.pivot_mode == MirrorPivotMode::WorldOrigin))
    {
      dialog_state.pivot_mode = MirrorPivotMode::WorldOrigin;
    }
    if (ImGui::RadioButton("Custom", dialog_state.pivot_mode == MirrorPivotMode::Custom))
    {
      dialog_state.pivot_mode = MirrorPivotMode::Custom;
    }

    if (dialog_state.pivot_mode == MirrorPivotMode::Custom)
    {
      ImGui::Indent();
      ImGui::DragFloat3("Pivot Point", dialog_state.custom_pivot, 1.0f);
      ImGui::Unindent();
    }

    ImGui::Separator();

    ImGui::Checkbox("Clone (create mirrored copy)", &dialog_state.clone);
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Creates a copy of the selection and mirrors it,\ninstead of modifying the original.");
      ImGui::EndTooltip();
    }

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(100, 0)))
    {
      MirrorSelectionEx(scene_panel, nodes, props,
        dialog_state.axis, dialog_state.pivot_mode,
        dialog_state.custom_pivot, dialog_state.clone, undo_stack);
      result.apply_mirror = true;
      dialog_state.open = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(100, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      result.cancelled = true;
      dialog_state.open = false;
    }
  }
  ImGui::End();

  return result;
}

void MirrorSelectionEx(
  ScenePanelState& scene_panel,
  std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  MirrorAxis axis,
  MirrorPivotMode pivot_mode,
  const float custom_pivot[3],
  bool clone,
  UndoStack* undo_stack)
{
  if (!HasSelection(scene_panel))
  {
    return;
  }

  // Determine pivot point
  Diligent::float3 pivot(0.0f, 0.0f, 0.0f);
  switch (pivot_mode)
  {
    case MirrorPivotMode::SelectionCenter:
    {
      const std::array<float, 3> center = ComputeSelectionCenter(scene_panel, props);
      pivot = Diligent::float3(center[0], center[1], center[2]);
      break;
    }
    case MirrorPivotMode::WorldOrigin:
      pivot = Diligent::float3(0.0f, 0.0f, 0.0f);
      break;
    case MirrorPivotMode::Custom:
      pivot = Diligent::float3(custom_pivot[0], custom_pivot[1], custom_pivot[2]);
      break;
  }

  const int axis_index = static_cast<int>(axis);
  const size_t count = std::min(nodes.size(), props.size());

  // Build transform changes for undo
  std::vector<TransformChange> changes;
  std::vector<int> new_node_ids;  // For clone mode

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

    int target_id = id;

    if (clone)
    {
      // Create a copy of the node
      TreeNode new_node = nodes[id];
      NodeProperties new_props = props[id];
      new_node.name += "_mirrored";
      new_node.children.clear();  // Cloned node doesn't inherit children
      target_id = static_cast<int>(nodes.size());
      nodes.push_back(new_node);
      props.push_back(new_props);
      new_node_ids.push_back(target_id);

      // Add the new node to the parent's children list so it appears in the hierarchy
      int parent_id = FindParentId(nodes, 0, id);
      if (parent_id >= 0 && parent_id < static_cast<int>(nodes.size()))
      {
        nodes[parent_id].children.push_back(target_id);
      }
    }

    // Only track transform changes for in-place mirrors (not clones)
    // Clones will be deleted entirely on undo via PushCreate
    TransformChange change;
    if (!clone)
    {
      change.node_id = target_id;
      change.before.position[0] = props[target_id].position[0];
      change.before.position[1] = props[target_id].position[1];
      change.before.position[2] = props[target_id].position[2];
      change.before.rotation[0] = props[target_id].rotation[0];
      change.before.rotation[1] = props[target_id].rotation[1];
      change.before.rotation[2] = props[target_id].rotation[2];
      change.before.scale[0] = props[target_id].scale[0];
      change.before.scale[1] = props[target_id].scale[1];
      change.before.scale[2] = props[target_id].scale[2];
      if (!props[target_id].brush_vertices.empty())
      {
        change.before_vertices = props[target_id].brush_vertices;
        change.before_indices = props[target_id].brush_indices;
      }
    }

    // Mirror position
    const float offset = props[target_id].position[axis_index] - pivot[axis_index];
    props[target_id].position[axis_index] = pivot[axis_index] - offset;

    // Mirror brush vertices if present
    if (!props[target_id].brush_vertices.empty())
    {
      for (size_t v = 0; v < props[target_id].brush_vertices.size(); v += 3)
      {
        const float vert_offset = props[target_id].brush_vertices[v + axis_index] - pivot[axis_index];
        props[target_id].brush_vertices[v + axis_index] = pivot[axis_index] - vert_offset;
      }
      // Reverse triangle winding
      ReverseTriangleWinding(props[target_id].brush_indices);
    }

    // Record after state for non-clone transforms
    if (!clone)
    {
      if (!props[target_id].brush_vertices.empty())
      {
        change.after_vertices = props[target_id].brush_vertices;
        change.after_indices = props[target_id].brush_indices;
      }
      change.after.position[0] = props[target_id].position[0];
      change.after.position[1] = props[target_id].position[1];
      change.after.position[2] = props[target_id].position[2];
      change.after.rotation[0] = props[target_id].rotation[0];
      change.after.rotation[1] = props[target_id].rotation[1];
      change.after.rotation[2] = props[target_id].rotation[2];
      change.after.scale[0] = props[target_id].scale[0];
      change.after.scale[1] = props[target_id].scale[1];
      change.after.scale[2] = props[target_id].scale[2];
      changes.push_back(std::move(change));
    }
  }

  // If clone mode, select the new nodes
  if (clone && !new_node_ids.empty())
  {
    SelectNone(scene_panel);
    for (int new_id : new_node_ids)
    {
      ModifySelection(scene_panel, new_id, SelectionMode::Add);
    }
  }

  // Push undo actions
  if (undo_stack != nullptr)
  {
    if (clone)
    {
      // For clones, only push create actions - undo will delete the clones entirely
      for (int new_id : new_node_ids)
      {
        undo_stack->PushCreate(UndoTarget::Scene, new_id);
      }
    }
    else if (!changes.empty())
    {
      // For in-place mirrors, push transform changes
      undo_stack->PushTransform(UndoTarget::Scene, std::move(changes));
    }
  }
}
