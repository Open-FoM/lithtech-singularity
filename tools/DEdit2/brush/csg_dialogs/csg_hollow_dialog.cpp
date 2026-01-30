#include "csg_hollow_dialog.h"

#include "brush/csg/csg_hollow.h"

#include "imgui.h"

void DrawHollowDialog(HollowDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                      std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                      bool& document_dirty) {
  if (!state.open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Hollow Brush", &state.open)) {
    // Get selected brushes
    auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

    if (brush_ids.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No valid brush selected");
      ImGui::End();
      return;
    }

    if (brush_ids.size() > 1) {
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "Warning: Only the first brush will be hollowed");
    }

    ImGui::Text("Wall Thickness:");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##thickness", &state.wall_thickness, 1.0f, 1.0f, 1000.0f, "%.1f units");

    if (state.wall_thickness < 1.0f) {
      state.wall_thickness = 1.0f;
    }

    ImGui::Separator();
    ImGui::Checkbox("Delete original brush", &state.delete_original);

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

      // Call CSG hollow operation
      auto hollow_result = csg::HollowBrush(verts, indices, state.wall_thickness);

      if (hollow_result.success) {
        // Record undo for the original brush deletion (if deleting)
        if (state.delete_original && undo_stack != nullptr) {
          undo_stack->PushDelete(UndoTarget::Scene, brush_id, nodes[brush_id].deleted);
        }

        // Create new wall brushes
        int wall_num = 1;
        for (const auto& wall_brush : hollow_result.results) {
          std::vector<float> wall_verts;
          std::vector<uint32_t> wall_indices;
          wall_brush.ToTriangleMesh(wall_verts, wall_indices);

          if (wall_verts.empty()) {
            continue;
          }

          char name[64];
          snprintf(name, sizeof(name), "Wall_%d", wall_num++);
          int new_id = CreateBrushFromCSGResult(nodes, props, wall_verts, wall_indices, name);

          if (new_id >= 0 && undo_stack != nullptr) {
            undo_stack->PushCreate(UndoTarget::Scene, new_id);
          }
        }

        // Delete original brush if requested
        if (state.delete_original) {
          nodes[brush_id].deleted = true;
        }

        // Clear selection and select new walls
        ClearSelection(scene_panel);

        document_dirty = true;
        state.open = false;
      } else {
        error_state.show = true;
        error_state.message = hollow_result.error_message;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();
}
