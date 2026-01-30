#pragma once

/// @file geometry_ops_ui.h
/// @brief UI helper functions for geometry operations (EPIC-09).

#include "brush/csg_dialogs/csg_dialog_helpers.h"

/// Apply flip normals operation to all selected brushes.
///
/// This is a direct action that doesn't require a dialog.
/// It inverts all face normals of the selected brush(es).
///
/// @param scene_panel Scene panel state (for selection).
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @param undo_stack Undo stack for recording changes.
/// @param error_state Error popup state (for showing errors).
/// @param document_dirty Set to true if document was modified.
void ApplyFlipNormals(ScenePanelState &scene_panel, std::vector<TreeNode> &nodes, std::vector<NodeProperties> &props,
                      UndoStack *undo_stack, CSGErrorPopupState &error_state, bool &document_dirty);
