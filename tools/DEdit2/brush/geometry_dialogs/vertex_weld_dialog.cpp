#include "vertex_weld_dialog.h"

#include "brush/csg/csg_types.h"
#include "brush/geometry_ops/vertex_weld.h"

#include "imgui.h"

void DrawVertexWeldDialog(VertexWeldDialogState &state, ScenePanelState &scene_panel, std::vector<TreeNode> &nodes,
                          std::vector<NodeProperties> &props, UndoStack *undo_stack, CSGErrorPopupState &error_state,
                          bool &document_dirty) {
  if (!state.open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Weld Vertices", &state.open)) {
    // Get selected brushes
    auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

    if (brush_ids.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No valid brush selected");
      ImGui::End();
      return;
    }

    ImGui::Text("Weld Distance Threshold:");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##threshold", &state.threshold, 0.001f, 0.0001f, 10.0f, "%.4f units");

    if (state.threshold < 0.0001f) {
      state.threshold = 0.0001f;
    }

    ImGui::TextWrapped("Vertices within this distance of each other will be merged to their centroid.");

    ImGui::Separator();

    // Preview: show how many vertices would be welded
    int total_weldable = 0;
    for (int brush_id : brush_ids) {
      std::vector<float> verts;
      std::vector<uint32_t> indices;
      if (ExtractBrushGeometry(props[brush_id], verts, indices)) {
        csg::CSGBrush brush = csg::CSGBrush::FromTriangleMesh(verts, indices);
        auto weld_groups = geometry_ops::FindWeldableVertices(brush, state.threshold);
        for (const auto &group : weld_groups) {
          if (group.size() > 1) {
            total_weldable += static_cast<int>(group.size()) - 1;
          }
        }
      }
    }

    if (total_weldable > 0) {
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%d vertices can be welded", total_weldable);
    } else {
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "No vertices found within threshold");
    }

    ImGui::Separator();

    bool apply_enabled = total_weldable > 0;
    if (!apply_enabled) {
      ImGui::BeginDisabled();
    }

    if (ImGui::Button("Apply", ImVec2(100, 0))) {
      int brushes_modified = 0;

      for (int brush_id : brush_ids) {
        std::vector<float> verts;
        std::vector<uint32_t> indices;

        if (!ExtractBrushGeometry(props[brush_id], verts, indices)) {
          continue;
        }

        csg::CSGBrush brush = csg::CSGBrush::FromTriangleMesh(verts, indices);

        // Find all weldable vertex groups and weld them
        auto weld_groups = geometry_ops::FindWeldableVertices(brush, state.threshold);

        bool brush_modified = false;
        for (const auto &group : weld_groups) {
          if (group.size() > 1) {
            geometry_ops::VertexWeldOptions options;
            options.weld_threshold = state.threshold;
            auto result = geometry_ops::WeldVertices(brush, group, options);
            if (result.success) {
              brush = std::move(result.result_brush);
              brush_modified = true;
            }
          }
        }

        if (brush_modified) {
          // Update the brush geometry
          std::vector<float> new_verts;
          std::vector<uint32_t> new_indices;
          brush.ToTriangleMesh(new_verts, new_indices);

          // Record undo for the original brush
          if (undo_stack != nullptr) {
            TransformChange change;
            change.node_id = brush_id;
            std::copy(std::begin(props[brush_id].position), std::end(props[brush_id].position),
                      std::begin(change.before.position));
            std::copy(std::begin(props[brush_id].rotation), std::end(props[brush_id].rotation),
                      std::begin(change.before.rotation));
            std::copy(std::begin(props[brush_id].scale), std::end(props[brush_id].scale),
                      std::begin(change.before.scale));
            change.after = change.before;
            change.before_vertices = verts;
            change.after_vertices = new_verts;
            change.before_indices = indices;
            change.after_indices = new_indices;
            undo_stack->PushTransform(UndoTarget::Scene, {change});
          }

          props[brush_id].brush_vertices = std::move(new_verts);
          props[brush_id].brush_indices = std::move(new_indices);

          brushes_modified++;
        }
      }

      if (brushes_modified > 0) {
        document_dirty = true;
        state.open = false;
      } else {
        error_state.show = true;
        error_state.message = "No vertices were welded.";
      }
    }

    if (!apply_enabled) {
      ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();
}
