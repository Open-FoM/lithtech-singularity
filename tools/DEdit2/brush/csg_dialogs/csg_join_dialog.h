#pragma once

#include "csg_dialog_helpers.h"

/// State for the Join brushes dialog.
struct JoinDialogState {
  bool open = false;
  bool delete_originals = true; ///< Delete original brushes after join
};

/// Draw the Join brushes dialog and execute operation if applied.
/// Joins selected brushes into a single convex hull.
/// @param state Dialog state.
/// @param scene_panel Scene panel state (for selection).
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @param undo_stack Undo stack for recording changes.
/// @param error_state Error popup state (for showing errors).
/// @param document_dirty Set to true if document was modified.
void DrawJoinDialog(JoinDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                    std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                    bool& document_dirty);
