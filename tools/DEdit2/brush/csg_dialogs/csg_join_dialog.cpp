#include "csg_join_dialog.h"

#include "brush/csg/csg_join.h"

#include "imgui.h"

void DrawJoinDialog(JoinDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                    std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                    bool& document_dirty) {
  if (!state.open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Join Brushes", &state.open)) {
    // Get selected brushes
    auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

    if (brush_ids.size() < 2) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Join requires at least 2 brushes selected.");
      ImGui::End();
      return;
    }

    ImGui::Text("Joining %zu brushes into convex hull", brush_ids.size());

    ImGui::Separator();
    ImGui::Checkbox("Delete original brushes", &state.delete_originals);

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(100, 0))) {
      // Collect all brush geometry
      std::vector<std::vector<float>> all_vertices;
      std::vector<std::vector<uint32_t>> all_indices;

      for (int id : brush_ids) {
        std::vector<float> verts;
        std::vector<uint32_t> indices;
        if (ExtractBrushGeometry(props[id], verts, indices)) {
          all_vertices.push_back(std::move(verts));
          all_indices.push_back(std::move(indices));
        }
      }

      if (all_vertices.size() < 2) {
        error_state.show = true;
        error_state.message = "Failed to extract geometry from at least 2 brushes.";
        ImGui::End();
        return;
      }

      // Call CSG join operation
      auto join_result = csg::JoinBrushes(all_vertices, all_indices);

      if (join_result.success && !join_result.results.empty()) {
        // Record undo for deleting originals
        if (state.delete_originals) {
          for (int id : brush_ids) {
            if (undo_stack != nullptr) {
              undo_stack->PushDelete(UndoTarget::Scene, id, nodes[id].deleted);
            }
          }
        }

        // Create joined brush
        std::vector<float> joined_verts;
        std::vector<uint32_t> joined_indices;
        join_result.results[0].ToTriangleMesh(joined_verts, joined_indices);

        if (!joined_verts.empty()) {
          int new_id = CreateBrushFromCSGResult(nodes, props, joined_verts, joined_indices, "Joined");

          if (new_id >= 0 && undo_stack != nullptr) {
            undo_stack->PushCreate(UndoTarget::Scene, new_id);
          }
        }

        // Delete original brushes if requested
        if (state.delete_originals) {
          for (int id : brush_ids) {
            nodes[id].deleted = true;
          }
        }

        ClearSelection(scene_panel);
        document_dirty = true;
        state.open = false;
      } else {
        error_state.show = true;
        error_state.message = join_result.error_message.empty() ? "Join operation failed." : join_result.error_message;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();
}
