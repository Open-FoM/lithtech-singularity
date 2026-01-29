#pragma once

#include "editor_state.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/// World file format type.
enum class WorldFormat {
  Unknown,
  LTA,     ///< Text format (.lta)
  LTC,     ///< Compressed text format (.ltc)
  Binary   ///< Binary compiled format (.dat) - read-only
};

/// World-level properties (ambient, fog, gravity, etc.).
struct WorldProperties {
  std::string name;
  std::string author;
  std::string description;

  float ambient[3] = {0.2f, 0.2f, 0.2f};
  float background_color[3] = {0.0f, 0.0f, 0.0f};

  bool fog_enabled = false;
  float fog_color[3] = {0.5f, 0.5f, 0.5f};
  float fog_near = 0.0f;
  float fog_far = 2000.0f;
  float fog_density = 0.01f;

  float far_z = 10000.0f;
  float gravity = -9.8f;

  std::string sky_reference;
};

/// World data structure containing all editable world content.
struct World {
  /// Source file path (if loaded from file).
  fs::path source_path;

  /// Path to compiled world file (.dat), if available.
  fs::path compiled_path;

  /// Scene hierarchy nodes.
  std::vector<TreeNode> nodes;

  /// Properties for each node (parallel to nodes vector).
  std::vector<NodeProperties> properties;

  /// World-level properties (extracted from root WorldProperties node).
  WorldProperties world_props;

  /// Check if the world has any content.
  [[nodiscard]] bool IsEmpty() const { return nodes.empty(); }

  /// Get display name (from world_props or filename).
  [[nodiscard]] std::string GetDisplayName() const;

  /// Clear all world data.
  void Clear();

  /// Find the root world node (usually index 0).
  [[nodiscard]] int FindWorldRootNode() const;

  /// Extract WorldProperties from the scene hierarchy.
  void ExtractWorldProperties();

  /// Apply WorldProperties back to the scene hierarchy.
  void ApplyWorldProperties();
};

/// Detect the format of a world file by extension.
[[nodiscard]] WorldFormat DetectWorldFormat(const fs::path& path);

/// Load world from file.
/// @param path Path to the world file (.lta, .ltc, or .dat)
/// @param project_root Project root for finding compiled world
/// @param error Output error message on failure
/// @return Loaded world, or std::nullopt on failure
[[nodiscard]] std::optional<World> LoadWorld(
    const fs::path& path,
    const fs::path& project_root,
    std::string& error);

/// Save world to file.
/// @param world World to save
/// @param path Output file path
/// @param format Output format (LTA or LTC)
/// @param error Output error message on failure
/// @return True on success
[[nodiscard]] bool SaveWorld(
    const World& world,
    const fs::path& path,
    WorldFormat format,
    std::string& error);

/// Create a new empty world.
/// @param name Display name for the world
/// @return New world with minimal structure
[[nodiscard]] World CreateEmptyWorld(std::string_view name);
