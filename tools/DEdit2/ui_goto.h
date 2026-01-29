#pragma once

#include "editor_state.h"

#include <string>
#include <vector>

/// Result of the Go To dialog.
struct GoToResult
{
  bool accepted = false;     ///< True if user confirmed a selection.
  int selected_id = -1;      ///< Node ID to navigate to, or -1 if none.
};

/// State for the Go To Node dialog.
struct GoToDialogState
{
  bool open = false;
  char search_buffer[256] = {};
  std::vector<int> search_results;
  int selected_index = 0;          ///< Currently highlighted result index.
  bool focus_text_input = false;   ///< Set to focus the text input on next frame.
};

/// Draw the Go To Node dialog.
/// @param state Dialog state (modified on each frame).
/// @param nodes The scene tree nodes.
/// @param props The node properties.
/// @return GoToResult indicating if a selection was made.
[[nodiscard]] GoToResult DrawGoToDialog(
    GoToDialogState& state,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props);

/// Open the Go To dialog (sets open=true and resets state).
void OpenGoToDialog(GoToDialogState& state);
