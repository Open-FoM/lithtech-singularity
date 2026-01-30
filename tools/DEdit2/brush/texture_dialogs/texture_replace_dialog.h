#pragma once

/// @file texture_replace_dialog.h
/// @brief Texture Replace Dialog for EPIC-10 Texturing System UI.

#include <cstddef>

/// State for the Texture Replace dialog.
struct TextureReplaceDialogState {
  bool open = false;
  char old_texture[256] = "";
  char new_texture[256] = "";
  bool selection_only = false;
  bool use_pattern = false;
  size_t last_replaced = 0;
  bool show_result = false;
};

/// Result from the Texture Replace dialog.
struct TextureReplaceDialogResult {
  bool replace = false;
  bool find_next = false;
};

/// Draw the Texture Replace dialog.
/// @param state Dialog state (modified in place).
/// @return Result indicating user action.
TextureReplaceDialogResult DrawTextureReplaceDialog(TextureReplaceDialogState& state);
