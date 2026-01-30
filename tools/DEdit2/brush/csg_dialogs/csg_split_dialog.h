#pragma once

#include "csg_dialog_helpers.h"

/// Split plane axis preset.
enum class SplitAxis {
  X,      ///< Split along YZ plane (perpendicular to X)
  Y,      ///< Split along XZ plane (perpendicular to Y)
  Z,      ///< Split along XY plane (perpendicular to Z)
  Custom  ///< Custom plane normal and distance
};

/// State for the Split brush dialog.
struct SplitDialogState {
  bool open = false;
  SplitAxis axis = SplitAxis::Y;
  float distance = 0.0f;              ///< Plane distance from origin (or brush center)
  float custom_normal[3] = {0, 1, 0}; ///< Custom plane normal
  bool use_brush_center = true;       ///< Offset plane distance by brush center
  bool delete_original = true;        ///< Delete original brush after split

  // Preview state (computed each frame when dialog is open)
  bool preview_valid = false;         ///< Whether preview data is valid
  float preview_point[3] = {0, 0, 0}; ///< Point on the split plane
  float preview_normal[3] = {0, 1, 0}; ///< Split plane normal
  float preview_extent = 64.0f;       ///< Half-extent of preview plane quad
};

/// Draw the Split brush dialog and execute operation if applied.
/// @param state Dialog state.
/// @param scene_panel Scene panel state (for selection).
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @param undo_stack Undo stack for recording changes.
/// @param error_state Error popup state (for showing errors).
/// @param document_dirty Set to true if document was modified.
void DrawSplitDialog(SplitDialogState& state, ScenePanelState& scene_panel, std::vector<TreeNode>& nodes,
                     std::vector<NodeProperties>& props, UndoStack* undo_stack, CSGErrorPopupState& error_state,
                     bool& document_dirty);
