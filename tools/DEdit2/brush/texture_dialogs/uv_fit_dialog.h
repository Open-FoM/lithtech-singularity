#pragma once

/// @file uv_fit_dialog.h
/// @brief UV Fit Dialog for EPIC-10 Texturing System UI.

/// State for the UV Fit dialog.
struct UVFitDialogState {
  bool open = false;
  bool maintain_aspect = true;
  bool center = true;
  float padding = 0.0f;
  float scale_u = 1.0f;           ///< Computed fit scale for U axis
  float scale_v = 1.0f;           ///< Computed fit scale for V axis
};

/// Result from the UV Fit dialog.
struct UVFitDialogResult {
  bool apply = false;
};

/// Draw the UV Fit dialog.
/// @param state Dialog state (modified in place).
/// @return Result indicating user action.
UVFitDialogResult DrawUVFitDialog(UVFitDialogState& state);
