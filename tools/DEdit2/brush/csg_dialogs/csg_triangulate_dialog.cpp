#include "csg_triangulate_dialog.h"

#include "brush/csg/csg_triangulate.h"
#include "brush/csg/csg_types.h"

#include "imgui.h"

void DrawTriangulateDialog(TriangulateDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                           std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                           bool& document_dirty) {
  if (!state.open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Triangulate Brush", &state.open)) {
    // Get selected brushes
    auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

    if (brush_ids.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No valid brush selected");
      ImGui::End();
      return;
    }

    ImGui::Text("Triangulating %zu brush(es)", brush_ids.size());
    ImGui::TextWrapped("This ensures all brush faces are triangles. Brushes stored as triangle meshes are already "
                       "triangulated, so this is mainly for validation.");

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(100, 0))) {
      int processed = 0;

      for (int brush_id : brush_ids) {
        std::vector<float> verts;
        std::vector<uint32_t> indices;

        if (!ExtractBrushGeometry(props[brush_id], verts, indices)) {
          continue;
        }

        // Convert to CSG representation
        csg::CSGBrush brush = csg::CSGBrush::FromTriangleMesh(verts, indices);
        if (!brush.IsValid()) {
          continue;
        }

        // Triangulate (should be a no-op for already-triangulated brushes)
        csg::CSGBrush triangulated = csg::TriangulateBrush(brush);

        // Convert back to triangle mesh
        std::vector<float> new_verts;
        std::vector<uint32_t> new_indices;
        triangulated.ToTriangleMesh(new_verts, new_indices);

        // Only update if geometry changed
        if (new_verts.size() != verts.size() || new_indices.size() != indices.size()) {
          // Record undo for transform (stores before/after brush data)
          if (undo_stack != nullptr) {
            TransformChange change;
            change.node_id = brush_id;
            change.before_vertices = props[brush_id].brush_vertices;
            change.before_indices = props[brush_id].brush_indices;
            change.after_vertices = new_verts;
            change.after_indices = new_indices;
            // Copy transform state
            std::copy(std::begin(props[brush_id].position), std::end(props[brush_id].position),
                      std::begin(change.before.position));
            std::copy(std::begin(props[brush_id].rotation), std::end(props[brush_id].rotation),
                      std::begin(change.before.rotation));
            std::copy(std::begin(props[brush_id].scale), std::end(props[brush_id].scale),
                      std::begin(change.before.scale));
            change.after = change.before;
            undo_stack->PushTransform(UndoTarget::Scene, {change});
          }

          props[brush_id].brush_vertices = std::move(new_verts);
          props[brush_id].brush_indices = std::move(new_indices);
          document_dirty = true;
        }

        ++processed;
      }

      if (processed > 0) {
        state.open = false;
      } else {
        error_state.show = true;
        error_state.message = "No brushes could be processed.";
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();
}
