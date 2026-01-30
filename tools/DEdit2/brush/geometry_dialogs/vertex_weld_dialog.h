#pragma once

/// @file vertex_weld_dialog.h
/// @brief Dialog for the vertex weld operation (EPIC-09).

#include "brush/csg_dialogs/csg_dialog_helpers.h"
#include "geometry_dialog_state.h"

/// Draw the vertex weld dialog and execute operation if applied.
/// @param state Dialog state.
/// @param scene_panel Scene panel state (for selection).
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @param undo_stack Undo stack for recording changes.
/// @param error_state Error popup state (for showing errors).
/// @param document_dirty Set to true if document was modified.
void DrawVertexWeldDialog(VertexWeldDialogState &state, ScenePanelState &scene_panel, std::vector<TreeNode> &nodes,
                          std::vector<NodeProperties> &props, UndoStack *undo_stack, CSGErrorPopupState &error_state,
                          bool &document_dirty);
