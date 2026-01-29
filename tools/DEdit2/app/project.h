#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/// Validation result for project resource paths.
struct ProjectValidationResult {
  bool valid = true;
  std::vector<std::string> warnings;  ///< Missing optional paths
  std::vector<std::string> errors;    ///< Critical missing paths
};

/// Project configuration loaded from .dep files.
/// Stores paths to resources (textures, models, prefabs, objects, worlds).
struct Project {
  std::string name;          ///< Project display name
  fs::path root_path;        ///< Directory containing .dep file
  fs::path dep_file_path;    ///< Full path to .dep file

  // Resource paths (relative to root_path, or absolute)
  fs::path textures_path;    ///< e.g., "Textures"
  fs::path objects_path;     ///< e.g., "Objects"
  fs::path models_path;      ///< e.g., "Models"
  fs::path prefabs_path;     ///< e.g., "Prefabs"
  fs::path worlds_path;      ///< e.g., "Worlds"
  fs::path sounds_path;      ///< e.g., "Sounds"

  /// Get full path to textures directory.
  [[nodiscard]] fs::path GetTexturesFullPath() const;

  /// Get full path to objects directory.
  [[nodiscard]] fs::path GetObjectsFullPath() const;

  /// Get full path to models directory.
  [[nodiscard]] fs::path GetModelsFullPath() const;

  /// Get full path to prefabs directory.
  [[nodiscard]] fs::path GetPrefabsFullPath() const;

  /// Get full path to worlds directory.
  [[nodiscard]] fs::path GetWorldsFullPath() const;

  /// Get full path to sounds directory.
  [[nodiscard]] fs::path GetSoundsFullPath() const;

  /// Validate that resource paths exist.
  [[nodiscard]] ProjectValidationResult Validate() const;

  /// Check if the project has been loaded/created.
  [[nodiscard]] bool IsValid() const { return !root_path.empty(); }

  /// Clear all project data.
  void Clear();
};

/// Load project from .dep file.
/// @param dep_path Path to the .dep file
/// @param error Output error message on failure
/// @return Loaded project, or std::nullopt on failure
[[nodiscard]] std::optional<Project> LoadProject(const fs::path& dep_path, std::string& error);

/// Save project to .dep file.
/// @param project Project to save
/// @param error Output error message on failure
/// @return True on success
[[nodiscard]] bool SaveProject(const Project& project, std::string& error);

/// Create a new project with default resource paths.
/// @param root_path Directory where project will be created
/// @param name Display name for the project
/// @return New project with default paths
[[nodiscard]] Project CreateDefaultProject(const fs::path& root_path, std::string_view name);
