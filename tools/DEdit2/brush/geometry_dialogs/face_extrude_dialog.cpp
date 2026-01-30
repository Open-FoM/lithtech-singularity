#include "face_extrude_dialog.h"

#include "brush/csg/csg_types.h"
#include "brush/geometry_ops/face_extrude.h"

#include "imgui.h"

void DrawFaceExtrudeDialog(FaceExtrudeDialogState &state, ScenePanelState &scene_panel, std::vector<TreeNode> &nodes,
                           std::vector<NodeProperties> &props, UndoStack *undo_stack, CSGErrorPopupState &error_state,
                           bool &document_dirty) {
  if (!state.open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Extrude Faces", &state.open)) {
    // Get selected brushes
    auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

    if (brush_ids.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No valid brush selected");
      ImGui::End();
      return;
    }

    ImGui::Text("Extrusion Distance:");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##distance", &state.distance, 1.0f, -1000.0f, 1000.0f, "%.1f units");

    ImGui::Checkbox("Extrude along face normal", &state.along_normal);
    ImGui::Checkbox("Create side walls", &state.create_side_walls);

    ImGui::Separator();

    ImGui::TextWrapped("Note: Currently extrudes ALL faces of the brush. "
                       "Sub-object face selection will be available in a future update.");

    ImGui::Separator();

    // Count total faces that will be extruded
    int total_faces = 0;
    for (int brush_id : brush_ids) {
      std::vector<float> verts;
      std::vector<uint32_t> indices;
      if (ExtractBrushGeometry(props[brush_id], verts, indices)) {
        csg::CSGBrush brush = csg::CSGBrush::FromTriangleMesh(verts, indices);
        total_faces += static_cast<int>(brush.PolygonCount());
      }
    }

    ImGui::Text("%d face(s) will be extruded", total_faces);

    ImGui::Separator();

    bool can_apply = total_faces > 0 && std::abs(state.distance) > 0.001f;
    if (!can_apply) {
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

        // Build list of all face indices
        std::vector<uint32_t> face_indices;
        face_indices.reserve(brush.PolygonCount());
        for (uint32_t i = 0; i < static_cast<uint32_t>(brush.PolygonCount()); ++i) {
          face_indices.push_back(i);
        }

        geometry_ops::FaceExtrudeOptions options;
        options.distance = state.distance;
        options.along_normal = state.along_normal;
        options.create_side_walls = state.create_side_walls;

        auto result = geometry_ops::ExtrudeFaces(brush, face_indices, options);

        if (result.success) {
          // Update the brush geometry
          std::vector<float> new_verts;
          std::vector<uint32_t> new_indices;
          result.result_brush.ToTriangleMesh(new_verts, new_indices);

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
        } else {
          error_state.show = true;
          error_state.message = result.error_message;
          break;
        }
      }

      if (brushes_modified > 0) {
        document_dirty = true;
        state.open = false;
      }
    }

    if (!can_apply) {
      ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();
}
