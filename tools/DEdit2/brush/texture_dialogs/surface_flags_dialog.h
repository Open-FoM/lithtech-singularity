#pragma once

/// @file surface_flags_dialog.h
/// @brief Surface Flags Dialog for EPIC-10 Texturing System UI.

/// State for the Surface Flags dialog.
struct SurfaceFlagsDialogState {
  bool open = false;
  bool solid = true;
  bool invisible = false;
  bool fullbright = false;
  bool sky = false;
  bool portal = false;
  bool mirror = false;
  bool transparent = false;
  int alpha_ref = 0;
};

/// Result from the Surface Flags dialog.
struct SurfaceFlagsDialogResult {
  bool apply = false;
};

/// Draw the Surface Flags dialog.
/// @param state Dialog state (modified in place).
/// @return Result indicating user action.
SurfaceFlagsDialogResult DrawSurfaceFlagsDialog(SurfaceFlagsDialogState& state);
