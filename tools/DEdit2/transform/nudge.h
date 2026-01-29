#pragma once

#include <vector>

class UndoStack;
struct NodeProperties;
struct ScenePanelState;
struct TreeNode;
struct ViewportPanelState;

/// Direction for nudge operations.
enum class NudgeDirection
{
  Left,
  Right,
  Up,
  Down,
  Forward,
  Back
};

/// Increment size for nudge operations.
enum class NudgeIncrement
{
  Small,   ///< 1 unit (Ctrl modifier)
  Normal,  ///< grid_spacing
  Large    ///< 10x grid_spacing (Shift modifier)
};

/// Nudge the current selection in the specified direction.
/// Direction is interpreted relative to the current viewport view mode.
/// @param viewport The current viewport panel state.
/// @param scene_panel The scene panel state (for selection).
/// @param nodes The scene nodes.
/// @param props The scene node properties.
/// @param direction The nudge direction.
/// @param increment The nudge increment size.
/// @param undo_stack The undo stack for recording the operation.
void NudgeSelection(
  const ViewportPanelState& viewport,
  const ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props,
  NudgeDirection direction,
  NudgeIncrement increment,
  UndoStack* undo_stack);
