#include "transform/rotate_dialog.h"

#include "ui_scene.h"
#include "undo_stack.h"

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "imgui.h"

#include <cmath>

namespace {

constexpr float kPi = 3.1415926f;

Diligent::float3x3 EulerToMatrixXYZ(const float rot_deg[3])
{
  const float rx = rot_deg[0] * kPi / 180.0f;
  const float ry = rot_deg[1] * kPi / 180.0f;
  const float rz = rot_deg[2] * kPi / 180.0f;

  const Diligent::float3x3 rxm = Diligent::float3x3::RotationX(rx);
  const Diligent::float3x3 rym = Diligent::float3x3::RotationY(ry);
  const Diligent::float3x3 rzm = Diligent::float3x3::RotationZ(rz);
  return Diligent::float3x3::Mul(Diligent::float3x3::Mul(rxm, rym), rzm);
}

void MatrixToEulerXYZ(const Diligent::float3x3& m, float out_deg[3])
{
  float sy = -m[0][2];
  sy = std::max(-1.0f, std::min(1.0f, sy));
  const float y = std::asin(sy);
  const float cy = std::cos(y);

  float x = 0.0f;
  float z = 0.0f;
  if (std::fabs(cy) > 1e-5f)
  {
    x = std::atan2(m[1][2], m[2][2]);
    z = std::atan2(m[0][1], m[0][0]);
  }
  else
  {
    x = std::atan2(m[1][0], m[1][1]);
    z = 0.0f;
  }

  out_deg[0] = x * 180.0f / kPi;
  out_deg[1] = y * 180.0f / kPi;
  out_deg[2] = z * 180.0f / kPi;
}

} // namespace

RotateDialogResult DrawRotateDialog(
  RotateDialogState& dialog_state,
  const ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  UndoStack* undo_stack)
{
  RotateDialogResult result{};

  if (!dialog_state.open)
  {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Rotate Selection", &dialog_state.open))
  {
    ImGui::Text("Rotation Angles (degrees)");
    ImGui::DragFloat("X", &dialog_state.rotation[0], 1.0f, -360.0f, 360.0f, "%.1f");
    ImGui::DragFloat("Y", &dialog_state.rotation[1], 1.0f, -360.0f, 360.0f, "%.1f");
    ImGui::DragFloat("Z", &dialog_state.rotation[2], 1.0f, -360.0f, 360.0f, "%.1f");

    ImGui::Separator();

    ImGui::Text("Coordinate Space");
    if (ImGui::RadioButton("World", !dialog_state.use_local_space))
    {
      dialog_state.use_local_space = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", dialog_state.use_local_space))
    {
      dialog_state.use_local_space = true;
    }

    ImGui::Separator();

    ImGui::Text("Pivot");
    if (ImGui::RadioButton("Selection Center", dialog_state.pivot_mode == RotatePivotMode::SelectionCenter))
    {
      dialog_state.pivot_mode = RotatePivotMode::SelectionCenter;
    }
    if (ImGui::RadioButton("World Origin", dialog_state.pivot_mode == RotatePivotMode::WorldOrigin))
    {
      dialog_state.pivot_mode = RotatePivotMode::WorldOrigin;
    }
    if (ImGui::RadioButton("Custom", dialog_state.pivot_mode == RotatePivotMode::Custom))
    {
      dialog_state.pivot_mode = RotatePivotMode::Custom;
    }

    if (dialog_state.pivot_mode == RotatePivotMode::Custom)
    {
      ImGui::Indent();
      ImGui::DragFloat3("Pivot Point", dialog_state.custom_pivot, 1.0f);
      ImGui::Unindent();
    }

    ImGui::Separator();

    ImGui::Checkbox("Preview", &dialog_state.preview);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Preview rotation in real-time");
      ImGui::EndTooltip();
    }

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(100, 0)))
    {
      ApplyRotationToSelection(scene_panel, nodes, props,
        dialog_state.rotation, dialog_state.use_local_space,
        dialog_state.pivot_mode, dialog_state.custom_pivot, undo_stack);
      result.apply_rotation = true;
      dialog_state.open = false;
      // Reset rotation for next use
      dialog_state.rotation[0] = dialog_state.rotation[1] = dialog_state.rotation[2] = 0.0f;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(100, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      result.cancelled = true;
      dialog_state.open = false;
      // Reset rotation for next use
      dialog_state.rotation[0] = dialog_state.rotation[1] = dialog_state.rotation[2] = 0.0f;
    }
  }
  ImGui::End();

  return result;
}

void ApplyRotationToSelection(
  const ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  const float rotation[3],
  bool use_local_space,
  RotatePivotMode pivot_mode,
  const float custom_pivot[3],
  UndoStack* undo_stack)
{
  if (!HasSelection(scene_panel))
  {
    return;
  }

  // Build rotation matrix
  const Diligent::float3x3 rot_matrix = EulerToMatrixXYZ(rotation);

  // Determine pivot point
  Diligent::float3 pivot(0.0f, 0.0f, 0.0f);
  switch (pivot_mode)
  {
    case RotatePivotMode::SelectionCenter:
    {
      const std::array<float, 3> center = ComputeSelectionCenter(scene_panel, props);
      pivot = Diligent::float3(center[0], center[1], center[2]);
      break;
    }
    case RotatePivotMode::WorldOrigin:
      pivot = Diligent::float3(0.0f, 0.0f, 0.0f);
      break;
    case RotatePivotMode::Custom:
      pivot = Diligent::float3(custom_pivot[0], custom_pivot[1], custom_pivot[2]);
      break;
  }

  // Build transform changes
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

    // Rotate position around pivot
    Diligent::float3 offset(
      props[id].position[0] - pivot.x,
      props[id].position[1] - pivot.y,
      props[id].position[2] - pivot.z);
    Diligent::float3 rotated = rot_matrix * offset;
    props[id].position[0] = pivot.x + rotated.x;
    props[id].position[1] = pivot.y + rotated.y;
    props[id].position[2] = pivot.z + rotated.z;

    // Compose rotation with node's existing rotation
    const Diligent::float3x3 base = EulerToMatrixXYZ(props[id].rotation);
    Diligent::float3x3 combined;
    if (use_local_space)
    {
      combined = Diligent::float3x3::Mul(base, rot_matrix);
    }
    else
    {
      combined = Diligent::float3x3::Mul(rot_matrix, base);
    }
    MatrixToEulerXYZ(combined, props[id].rotation);

    // Rotate brush vertices if present
    if (!props[id].brush_vertices.empty())
    {
      change.before_vertices = props[id].brush_vertices;
      for (size_t v = 0; v < props[id].brush_vertices.size(); v += 3)
      {
        Diligent::float3 vert_offset(
          props[id].brush_vertices[v] - pivot.x,
          props[id].brush_vertices[v + 1] - pivot.y,
          props[id].brush_vertices[v + 2] - pivot.z);
        Diligent::float3 vert_rotated = rot_matrix * vert_offset;
        props[id].brush_vertices[v] = pivot.x + vert_rotated.x;
        props[id].brush_vertices[v + 1] = pivot.y + vert_rotated.y;
        props[id].brush_vertices[v + 2] = pivot.z + vert_rotated.z;
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
