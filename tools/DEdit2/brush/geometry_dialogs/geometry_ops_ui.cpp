#include "geometry_ops_ui.h"

#include "brush/csg/csg_types.h"
#include "brush/geometry_ops/flip_normal.h"

void ApplyFlipNormals(ScenePanelState &scene_panel, std::vector<TreeNode> &nodes, std::vector<NodeProperties> &props,
                      UndoStack *undo_stack, CSGErrorPopupState &error_state, bool &document_dirty) {
  auto brush_ids = GetSelectedCSGBrushIds(scene_panel, nodes, props);

  if (brush_ids.empty()) {
    error_state.show = true;
    error_state.message = "No valid brush selected.";
    return;
  }

  int brushes_modified = 0;

  for (int brush_id : brush_ids) {
    std::vector<float> verts;
    std::vector<uint32_t> indices;

    if (!ExtractBrushGeometry(props[brush_id], verts, indices)) {
      continue;
    }

    csg::CSGBrush brush = csg::CSGBrush::FromTriangleMesh(verts, indices);

    auto result = geometry_ops::FlipEntireBrush(brush);

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
        std::copy(std::begin(props[brush_id].scale), std::end(props[brush_id].scale), std::begin(change.before.scale));
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
  }
}
