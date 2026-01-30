#include "csg_split_dialog.h"

#include "brush/csg/csg_split.h"

#include "imgui.h"

namespace {

void GetPlaneNormalForAxis(SplitAxis axis, float out_normal[3]) {
  out_normal[0] = 0.0f;
  out_normal[1] = 0.0f;
  out_normal[2] = 0.0f;

  switch (axis) {
  case SplitAxis::X:
    out_normal[0] = 1.0f;
    break;
  case SplitAxis::Y:
    out_normal[1] = 1.0f;
    break;
  case SplitAxis::Z:
    out_normal[2] = 1.0f;
    break;
  case SplitAxis::Custom:
    // Custom normal is set separately
    break;
  }
}

} // namespace

void DrawSplitDialog(SplitDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                     std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                     bool& document_dirty) {
  // Reset preview state when dialog is closed
  if (!state.open) {
    state.preview_valid = false;
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Split Brush", &state.open)) {
    // Get selected brushes
    auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

    if (brush_ids.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No valid brush selected");
      ImGui::End();
      return;
    }

    if (brush_ids.size() > 1) {
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "Warning: Only the first brush will be split");
    }

    // Axis selection
    ImGui::Text("Split Axis:");
    const char* axis_names[] = {"X (YZ plane)", "Y (XZ plane)", "Z (XY plane)", "Custom"};
    int current_axis = static_cast<int>(state.axis);
    if (ImGui::Combo("##axis", &current_axis, axis_names, 4)) {
      state.axis = static_cast<SplitAxis>(current_axis);
    }

    // Show custom normal input if custom axis selected
    if (state.axis == SplitAxis::Custom) {
      ImGui::Text("Custom Normal:");
      ImGui::DragFloat3("##normal", state.custom_normal, 0.01f, -1.0f, 1.0f);
      // Normalize
      float len = std::sqrt(state.custom_normal[0] * state.custom_normal[0] +
                            state.custom_normal[1] * state.custom_normal[1] +
                            state.custom_normal[2] * state.custom_normal[2]);
      if (len > 0.001f) {
        state.custom_normal[0] /= len;
        state.custom_normal[1] /= len;
        state.custom_normal[2] /= len;
      }
    }

    ImGui::Text("Plane Distance:");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##distance", &state.distance, 1.0f, -10000.0f, 10000.0f, "%.1f units");

    ImGui::Checkbox("Relative to brush center", &state.use_brush_center);
    ImGui::Checkbox("Delete original brush", &state.delete_original);

    // Compute preview state for live visualization
    state.preview_valid = false;
    if (!brush_ids.empty()) {
      int brush_id = brush_ids[0];
      const auto& brush_props = props[brush_id];

      // Compute plane normal
      if (state.axis == SplitAxis::Custom) {
        state.preview_normal[0] = state.custom_normal[0];
        state.preview_normal[1] = state.custom_normal[1];
        state.preview_normal[2] = state.custom_normal[2];
      } else {
        GetPlaneNormalForAxis(state.axis, state.preview_normal);
      }

      // Compute plane point (a point on the plane)
      float plane_distance = state.distance;
      float center[3] = {brush_props.position[0], brush_props.position[1], brush_props.position[2]};

      if (state.use_brush_center) {
        // Plane passes through brush center + offset along normal
        state.preview_point[0] = center[0] + state.preview_normal[0] * state.distance;
        state.preview_point[1] = center[1] + state.preview_normal[1] * state.distance;
        state.preview_point[2] = center[2] + state.preview_normal[2] * state.distance;
      } else {
        // Plane at world distance along normal
        state.preview_point[0] = state.preview_normal[0] * plane_distance;
        state.preview_point[1] = state.preview_normal[1] * plane_distance;
        state.preview_point[2] = state.preview_normal[2] * plane_distance;
      }

      // Compute preview extent based on brush size
      float brush_extent = 64.0f;  // Default
      if (!brush_props.brush_vertices.empty()) {
        float min_bound[3] = {1e9f, 1e9f, 1e9f};
        float max_bound[3] = {-1e9f, -1e9f, -1e9f};
        for (size_t i = 0; i + 2 < brush_props.brush_vertices.size(); i += 3) {
          min_bound[0] = std::min(min_bound[0], brush_props.brush_vertices[i]);
          min_bound[1] = std::min(min_bound[1], brush_props.brush_vertices[i + 1]);
          min_bound[2] = std::min(min_bound[2], brush_props.brush_vertices[i + 2]);
          max_bound[0] = std::max(max_bound[0], brush_props.brush_vertices[i]);
          max_bound[1] = std::max(max_bound[1], brush_props.brush_vertices[i + 1]);
          max_bound[2] = std::max(max_bound[2], brush_props.brush_vertices[i + 2]);
        }
        float dx = max_bound[0] - min_bound[0];
        float dy = max_bound[1] - min_bound[1];
        float dz = max_bound[2] - min_bound[2];
        brush_extent = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.6f;
      }
      state.preview_extent = std::max(32.0f, brush_extent);
      state.preview_valid = true;
    }

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(100, 0))) {
      int brush_id = brush_ids[0];
      std::vector<float> verts;
      std::vector<uint32_t> indices;

      if (!ExtractBrushGeometry(props[brush_id], verts, indices)) {
        error_state.show = true;
        error_state.message = "Failed to extract brush geometry.";
        ImGui::End();
        return;
      }

      // Compute plane normal
      float normal[3];
      if (state.axis == SplitAxis::Custom) {
        normal[0] = state.custom_normal[0];
        normal[1] = state.custom_normal[1];
        normal[2] = state.custom_normal[2];
      } else {
        GetPlaneNormalForAxis(state.axis, normal);
      }

      // Compute plane distance
      float plane_distance = state.distance;
      if (state.use_brush_center) {
        // Add brush center offset along the plane normal
        plane_distance += props[brush_id].position[0] * normal[0] + props[brush_id].position[1] * normal[1] +
                          props[brush_id].position[2] * normal[2];
      }

      // Call CSG split operation
      auto split_result = csg::SplitBrush(verts, indices, normal, plane_distance);

      if (split_result.success && split_result.results.size() >= 2) {
        // Record undo for the original brush deletion (if deleting)
        if (state.delete_original && undo_stack != nullptr) {
          undo_stack->PushDelete(UndoTarget::Scene, brush_id, nodes[brush_id].deleted);
        }

        // Create new brushes for front and back
        const char* suffixes[] = {"_front", "_back"};
        for (size_t i = 0; i < split_result.results.size() && i < 2; ++i) {
          std::vector<float> part_verts;
          std::vector<uint32_t> part_indices;
          split_result.results[i].ToTriangleMesh(part_verts, part_indices);

          if (part_verts.empty()) {
            continue;
          }

          char name[64];
          snprintf(name, sizeof(name), "%s%s", nodes[brush_id].name.c_str(), suffixes[i]);
          int new_id = CreateBrushFromCSGResult(nodes, props, part_verts, part_indices, name);

          if (new_id >= 0 && undo_stack != nullptr) {
            undo_stack->PushCreate(UndoTarget::Scene, new_id);
          }
        }

        // Delete original brush if requested
        if (state.delete_original) {
          nodes[brush_id].deleted = true;
        }

        ClearSelection(scene_panel);
        document_dirty = true;
        state.open = false;
      } else if (split_result.success && split_result.results.size() == 1) {
        error_state.show = true;
        error_state.message = "Plane does not split the brush. It may be entirely on one side.";
      } else {
        error_state.show = true;
        error_state.message = split_result.error_message.empty() ? "Split operation failed." : split_result.error_message;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();
}
