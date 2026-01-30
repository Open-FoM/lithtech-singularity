#pragma once

/// @file uv_projection_dialog.h
/// @brief UV Projection Dialog for EPIC-10 Texturing System UI.

/// State for the UV Projection dialog.
struct UVProjectionDialogState {
  bool open = false;
  int projection_type = 3;          ///< 0=PlanarX, 1=PlanarY, 2=PlanarZ, 3=PlanarAuto, 4=Cylindrical, 5=Spherical, 6=Box
  float scale_u = 0.0625f;
  float scale_v = 0.0625f;
  float axis[3] = {0.0f, 1.0f, 0.0f};
  float center[3] = {0.0f, 0.0f, 0.0f};
  float radius = 64.0f;
};

/// Result from the UV Projection dialog.
struct UVProjectionDialogResult {
  bool apply = false;
};

/// Draw the UV Projection dialog.
/// @param state Dialog state (modified in place).
/// @return Result indicating user action.
UVProjectionDialogResult DrawUVProjectionDialog(UVProjectionDialogState& state);
