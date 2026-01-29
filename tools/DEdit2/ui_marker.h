#pragma once

struct ViewportPanelState;

/// State for the marker position dialog.
struct MarkerDialogState
{
  bool open = false;
  float position[3] = {0.0f, 0.0f, 0.0f};  ///< Temporary position for editing
};

/// Result from the marker dialog.
struct MarkerDialogResult
{
  bool applied = false;
  bool reset = false;
  bool cancelled = false;
};

/// Draw the marker position dialog.
/// @param dialog_state The current dialog state.
/// @param viewport The viewport panel state containing the marker position.
/// @return Result indicating the action taken.
MarkerDialogResult DrawMarkerDialog(MarkerDialogState& dialog_state, ViewportPanelState& viewport);
