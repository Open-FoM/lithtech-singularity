#pragma once

/// @file uv_transform_dialog.h
/// @brief UV Transform Dialog for EPIC-10 Texturing System UI.

/// State for the UV Transform dialog.
struct UVTransformDialogState {
  bool open = false;
  float offset_u = 0.0f;
  float offset_v = 0.0f;
  float scale_u = 1.0f;
  float scale_v = 1.0f;
  float rotation = 0.0f;
  bool preview = false;
};

/// Result from the UV Transform dialog.
struct UVTransformDialogResult {
  bool apply = false;
  bool reset = false;
};

/// Draw the UV Transform dialog.
/// @param state Dialog state (modified in place).
/// @return Result indicating user action.
UVTransformDialogResult DrawUVTransformDialog(UVTransformDialogState& state);
