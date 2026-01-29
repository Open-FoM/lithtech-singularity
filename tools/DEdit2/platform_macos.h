#pragma once

#include <string>
#include <vector>

struct SDL_Window;

void* CreateMetalView(SDL_Window* window);
void DestroyMetalView(void* view);
void* GetMetalLayer(void* view);

/// Open a folder selection dialog.
/// @param initial_dir Initial directory to show
/// @param out_path Output selected folder path
/// @return True if user selected a folder
bool OpenFolderDialog(const std::string& initial_dir, std::string& out_path);

/// Open a file selection dialog.
/// @param initial_dir Initial directory to show
/// @param filter_extensions List of allowed extensions (e.g., {"lta", "ltc"})
/// @param filter_description Description shown in filter dropdown (e.g., "World Files")
/// @param out_path Output selected file path
/// @return True if user selected a file
bool OpenFileDialog(
    const std::string& initial_dir,
    const std::vector<std::string>& filter_extensions,
    const std::string& filter_description,
    std::string& out_path);

/// Open a save file dialog.
/// @param initial_dir Initial directory to show
/// @param default_name Default filename (without extension)
/// @param filter_extension Primary extension to use (e.g., "lta")
/// @param filter_description Description shown in filter dropdown
/// @param out_path Output selected file path
/// @return True if user confirmed save
bool SaveFileDialog(
    const std::string& initial_dir,
    const std::string& default_name,
    const std::string& filter_extension,
    const std::string& filter_description,
    std::string& out_path);

/// Open a path in the system file manager (Finder on macOS).
/// @param path Path to reveal/open
void OpenPathInFileManager(const std::string& path);
