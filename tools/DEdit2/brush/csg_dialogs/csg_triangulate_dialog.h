#pragma once

#include "csg_dialog_helpers.h"

/// State for the Triangulate brush dialog.
struct TriangulateDialogState {
  bool open = false;
};

/// Draw the Triangulate brush dialog and execute operation if applied.
/// Triangulates all selected brushes (converts n-gon faces to triangles).
/// @param state Dialog state.
/// @param scene_panel Scene panel state (for selection).
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @param undo_stack Undo stack for recording changes.
/// @param error_state Error popup state (for showing errors).
/// @param document_dirty Set to true if document was modified.
void DrawTriangulateDialog(TriangulateDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                           std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                           bool& document_dirty);
