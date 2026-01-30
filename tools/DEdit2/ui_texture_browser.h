#pragma once

/// @file ui_texture_browser.h
/// @brief Texture Browser Panel for browsing and selecting textures (US-10-01).
///
/// Provides a searchable, sortable grid/list view of textures in the project.
/// Supports lazy thumbnail loading and texture application to brush faces.

#include "imgui.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

/// View mode for the texture browser panel.
enum class TextureBrowserViewMode {
  Grid,  ///< Grid of thumbnails
  List   ///< Vertical list with details
};

/// Sort mode for texture entries.
enum class TextureBrowserSortMode {
  Name,        ///< Sort alphabetically by name
  Size,        ///< Sort by file size
  Dimensions   ///< Sort by texture dimensions (width * height)
};

/// Entry representing a single texture in the browser.
struct TextureBrowserEntry {
  std::string name;              ///< Display name (filename without path)
  std::string path;              ///< Full filesystem path
  std::string relative_path;     ///< Path relative to project root
  uint32_t width = 0;            ///< Texture width in pixels
  uint32_t height = 0;           ///< Texture height in pixels
  uint32_t file_size = 0;        ///< File size in bytes
  std::string format_label;      ///< Format string (e.g., "DTX", "TGA")
  std::string dimensions_label;  ///< Dimensions string (e.g., "256x256")
  bool info_loaded = false;      ///< Whether texture info has been loaded
};

/// Cached thumbnail for a texture.
struct TextureThumbnail {
  void* texture_handle = nullptr;  ///< GPU texture handle (ImTextureID)
  bool load_attempted = false;     ///< Whether loading was attempted
  bool valid = false;              ///< Whether thumbnail is valid
};

/// State for the texture browser panel.
struct TextureBrowserState {
  TextureBrowserViewMode view_mode = TextureBrowserViewMode::Grid;
  TextureBrowserSortMode sort_mode = TextureBrowserSortMode::Name;
  bool sort_ascending = true;      ///< Sort direction
  ImGuiTextFilter filter;          ///< Search filter

  std::vector<TextureBrowserEntry> entries;  ///< All discovered texture entries
  std::unordered_map<std::string, TextureThumbnail> thumbnail_cache;

  int selected_index = -1;         ///< Currently selected entry index
  std::string current_texture;     ///< Currently selected texture path
  int thumbnail_size = 64;         ///< Thumbnail size in pixels
  bool needs_refresh = true;       ///< Whether to re-scan directories

  /// Number of thumbnails to load per frame (for lazy loading).
  static constexpr int kMaxThumbnailLoadsPerFrame = 4;
};

/// Actions returned from the texture browser panel.
struct TextureBrowserAction {
  bool texture_selected = false;   ///< A texture was double-clicked
  bool apply_to_faces = false;     ///< Apply texture to selected faces
  std::string selected_texture;    ///< The selected texture path
};

/// Draw the texture browser panel.
/// @param state Panel state (modified in place).
/// @param project_root Root directory of the project.
/// @param action Output actions from user interaction.
void DrawTextureBrowserPanel(TextureBrowserState& state,
                             const std::string& project_root,
                             TextureBrowserAction& action);

/// Scan project directories for texture files.
/// @param project_root Root directory of the project.
/// @return Vector of discovered texture entries.
[[nodiscard]] std::vector<TextureBrowserEntry> ScanTextureDirectories(
    const std::string& project_root);

/// Sort texture entries according to the given mode.
/// @param entries Entries to sort (modified in place).
/// @param mode Sort mode.
/// @param ascending Sort direction.
void SortTextureEntries(std::vector<TextureBrowserEntry>& entries,
                        TextureBrowserSortMode mode,
                        bool ascending);

/// Format a file size in bytes to a human-readable string.
/// @param bytes File size in bytes.
/// @return Formatted string (e.g., "1.5 KB", "2.3 MB").
[[nodiscard]] std::string FormatFileSize(uint32_t bytes);

/// Get a format label for bits per pixel.
/// @param bpp Bits per pixel.
/// @return Format label (e.g., "DXT1", "RGBA32").
[[nodiscard]] std::string FormatBPP(int bpp);
