#include "csg_carve_dialog.h"

#include "brush/csg/csg_carve.h"

#include "imgui.h"

void DrawCarveDialog(CarveDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                     std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                     bool& document_dirty) {
  if (!state.open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Carve Brush", &state.open)) {
    // Get selected brushes
    auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

    if (brush_ids.size() < 2) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Carve requires at least 2 brushes selected.");
      ImGui::TextWrapped("The last selected brush will be the cutter.");
      ImGui::End();
      return;
    }

    // Identify cutter and targets
    int cutter_id = scene_panel.primary_selection;
    std::vector<int> target_ids;
    for (int id : brush_ids) {
      if (id != cutter_id) {
        target_ids.push_back(id);
      }
    }

    // If primary_selection is not in brush_ids, use the last one
    if (std::find(brush_ids.begin(), brush_ids.end(), cutter_id) == brush_ids.end()) {
      cutter_id = brush_ids.back();
      target_ids.clear();
      for (size_t i = 0; i < brush_ids.size() - 1; ++i) {
        target_ids.push_back(brush_ids[i]);
      }
    }

    ImGui::Text("Cutter: %s", nodes[cutter_id].name.c_str());
    ImGui::Text("Targets: %zu brush(es)", target_ids.size());

    ImGui::Separator();
    ImGui::Checkbox("Delete cutter after carve", &state.delete_cutter);
    ImGui::Checkbox("Delete original targets", &state.delete_carved_targets);

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(100, 0))) {
      // Extract cutter geometry
      std::vector<float> cutter_verts;
      std::vector<uint32_t> cutter_indices;
      if (!ExtractBrushGeometry(props[cutter_id], cutter_verts, cutter_indices)) {
        error_state.show = true;
        error_state.message = "Failed to extract cutter geometry.";
        ImGui::End();
        return;
      }

      bool any_carved = false;
      std::vector<int> nodes_to_delete;

      // Carve each target
      for (int target_id : target_ids) {
        std::vector<float> target_verts;
        std::vector<uint32_t> target_indices;
        if (!ExtractBrushGeometry(props[target_id], target_verts, target_indices)) {
          continue;
        }

        auto carve_result = csg::CarveBrush(target_verts, target_indices, cutter_verts, cutter_indices);

        if (carve_result.success && !carve_result.results.empty()) {
          // Create new brushes from carved fragments
          int frag_num = 1;
          for (const auto& frag_brush : carve_result.results) {
            std::vector<float> frag_verts;
            std::vector<uint32_t> frag_indices;
            frag_brush.ToTriangleMesh(frag_verts, frag_indices);

            if (frag_verts.empty()) {
              continue;
            }

            char name[64];
            snprintf(name, sizeof(name), "%s_carved_%d", nodes[target_id].name.c_str(), frag_num++);
            int new_id = CreateBrushFromCSGResult(nodes, props, frag_verts, frag_indices, name);

            if (new_id >= 0 && undo_stack != nullptr) {
              undo_stack->PushCreate(UndoTarget::Scene, new_id);
            }
          }

          if (state.delete_carved_targets) {
            nodes_to_delete.push_back(target_id);
          }
          any_carved = true;
        }
      }

      // Delete cutter if requested
      if (state.delete_cutter) {
        nodes_to_delete.push_back(cutter_id);
      }

      // Mark nodes for deletion with undo support
      for (int id : nodes_to_delete) {
        if (undo_stack != nullptr) {
          undo_stack->PushDelete(UndoTarget::Scene, id, nodes[id].deleted);
        }
        nodes[id].deleted = true;
      }

      if (any_carved) {
        ClearSelection(scene_panel);
        document_dirty = true;
      } else {
        error_state.show = true;
        error_state.message = "No brushes were carved. Brushes may not be intersecting.";
      }

      state.open = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();
}
