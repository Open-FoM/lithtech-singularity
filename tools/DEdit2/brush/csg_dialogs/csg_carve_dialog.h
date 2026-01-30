#pragma once

#include "csg_dialog_helpers.h"

/// State for the Carve brush dialog.
struct CarveDialogState {
  bool open = false;
  bool delete_cutter = true;       ///< Delete cutting brush after operation
  bool delete_carved_targets = true; ///< Delete original target brushes
};

/// Draw the Carve brush dialog and execute operation if applied.
/// The last selected brush (primary_selection) is the cutter.
/// All other selected brushes are targets.
/// @param state Dialog state.
/// @param scene_panel Scene panel state (for selection).
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @param undo_stack Undo stack for recording changes.
/// @param error_state Error popup state (for showing errors).
/// @param document_dirty Set to true if document was modified.
void DrawCarveDialog(CarveDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                     std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                     bool& document_dirty);
