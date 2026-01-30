#pragma once

#include "csg_dialog_helpers.h"

/// State for the Hollow brush dialog.
struct HollowDialogState {
  bool open = false;
  float wall_thickness = 16.0f;  ///< Wall thickness in units
  bool delete_original = true;   ///< Delete original brush after hollowing
};

/// Draw the Hollow brush dialog and execute operation if applied.
/// @param state Dialog state.
/// @param scene_panel Scene panel state (for selection).
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @param undo_stack Undo stack for recording changes.
/// @param error_state Error popup state (for showing errors).
/// @param document_dirty Set to true if document was modified.
void DrawHollowDialog(HollowDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                      std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                      bool& document_dirty);
