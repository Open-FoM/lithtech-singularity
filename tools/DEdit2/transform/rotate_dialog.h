#pragma once

#include <vector>

class UndoStack;
struct NodeProperties;
struct ScenePanelState;
struct TreeNode;

/// Pivot mode for rotation dialog.
enum class RotatePivotMode
{
  SelectionCenter,  ///< Rotate around the center of selected objects
  WorldOrigin,      ///< Rotate around world origin (0, 0, 0)
  Custom            ///< Rotate around a custom point
};

/// State for the rotate selection dialog.
struct RotateDialogState
{
  bool open = false;
  float rotation[3] = {0.0f, 0.0f, 0.0f};  ///< Rotation angles in degrees
  bool use_local_space = false;             ///< Use local space instead of world space
  RotatePivotMode pivot_mode = RotatePivotMode::SelectionCenter;
  float custom_pivot[3] = {0.0f, 0.0f, 0.0f};  ///< Custom pivot point
  bool preview = false;                         ///< Preview rotation in real-time
};

/// Result from the rotate dialog.
struct RotateDialogResult
{
  bool apply_rotation = false;
  bool cancelled = false;
};

/// Draw the rotate selection dialog.
/// @param dialog_state The current dialog state.
/// @param scene_panel The scene panel state (for selection).
/// @param nodes The scene nodes.
/// @param props The scene node properties.
/// @param undo_stack The undo stack for recording the operation.
/// @return Result indicating whether to apply or cancel.
RotateDialogResult DrawRotateDialog(
  RotateDialogState& dialog_state,
  const ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  UndoStack* undo_stack);

/// Apply rotation to the current selection.
/// @param scene_panel The scene panel state (for selection).
/// @param nodes The scene nodes.
/// @param props The scene node properties.
/// @param rotation Rotation angles in degrees (X, Y, Z).
/// @param use_local_space Whether to use local space.
/// @param pivot_mode The pivot mode.
/// @param custom_pivot Custom pivot point (used when pivot_mode is Custom).
/// @param undo_stack The undo stack for recording the operation.
void ApplyRotationToSelection(
  const ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  const float rotation[3],
  bool use_local_space,
  RotatePivotMode pivot_mode,
  const float custom_pivot[3],
  UndoStack* undo_stack);
