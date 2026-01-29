#pragma once

#include "transform/mirror.h"

#include <vector>

class UndoStack;
struct NodeProperties;
struct ScenePanelState;
struct TreeNode;

/// Pivot mode for mirror dialog.
enum class MirrorPivotMode
{
  SelectionCenter,  ///< Mirror around the center of selected objects
  WorldOrigin,      ///< Mirror around world origin (0, 0, 0)
  Custom            ///< Mirror around a custom point
};

/// State for the mirror selection dialog.
struct MirrorDialogState
{
  bool open = false;
  MirrorAxis axis = MirrorAxis::X;            ///< Mirror axis
  MirrorPivotMode pivot_mode = MirrorPivotMode::SelectionCenter;
  float custom_pivot[3] = {0.0f, 0.0f, 0.0f}; ///< Custom pivot point
  bool clone = false;                          ///< Create a mirrored copy instead of modifying
};

/// Result from the mirror dialog.
struct MirrorDialogResult
{
  bool apply_mirror = false;
  bool cancelled = false;
};

/// Draw the mirror selection dialog.
/// @param dialog_state The current dialog state.
/// @param scene_panel The scene panel state (for selection).
/// @param nodes The scene nodes.
/// @param props The scene node properties.
/// @param undo_stack The undo stack for recording the operation.
/// @return Result indicating whether to apply or cancel.
MirrorDialogResult DrawMirrorDialog(
  MirrorDialogState& dialog_state,
  ScenePanelState& scene_panel,
  std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  UndoStack* undo_stack);

/// Apply mirror to the current selection with options.
/// @param scene_panel The scene panel state (for selection).
/// @param nodes The scene nodes.
/// @param props The scene node properties.
/// @param axis The mirror axis.
/// @param pivot_mode The pivot mode.
/// @param custom_pivot Custom pivot point (used when pivot_mode is Custom).
/// @param clone If true, create a mirrored copy instead of modifying in place.
/// @param undo_stack The undo stack for recording the operation.
void MirrorSelectionEx(
  ScenePanelState& scene_panel,
  std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  MirrorAxis axis,
  MirrorPivotMode pivot_mode,
  const float custom_pivot[3],
  bool clone,
  UndoStack* undo_stack);
