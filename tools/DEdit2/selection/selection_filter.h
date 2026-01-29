#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_set>

/// Types of nodes that can be filtered during selection.
enum class SelectableType {
  Brush,      ///< World geometry brushes
  WorldModel, ///< Moving/dynamic brushes
  Light,      ///< All light types (Point, Directional, etc.)
  Object,     ///< Game objects (triggers, spawners, etc.)
  Volume,     ///< Volume brushes (physics, AI, etc.)
  Prefab,     ///< Prefab instances
  Container   ///< Empty group/folder nodes
};

/// Selection filter state controlling which node types can be selected.
///
/// When a type is disabled in the filter, nodes of that type cannot be
/// selected via click or marquee selection. They remain visible in the
/// viewport but are effectively "click-through".
struct SelectionFilter {
  /// Set of enabled (selectable) types. If empty, all types are selectable.
  std::unordered_set<SelectableType> enabled_types;

  /// Whether type filtering is active. When false, all types are selectable.
  bool filter_active = false;

  /// Check if a node with the given type string passes the filter.
  /// @param type_str The node type string (e.g., "Brush", "Light", "Object")
  /// @param class_name The node class name (e.g., "PointLight", "Door")
  /// @return true if the node is selectable, false if filtered out.
  [[nodiscard]] bool PassesFilter(std::string_view type_str, std::string_view class_name) const;

  /// Enable or disable a specific type.
  void SetTypeEnabled(SelectableType type, bool enabled);

  /// Check if a specific type is enabled.
  [[nodiscard]] bool IsTypeEnabled(SelectableType type) const;

  /// Enable all types (disable filtering).
  void EnableAll();

  /// Disable all types (block all selection).
  void DisableAll();

  /// Get a human-readable name for a type.
  [[nodiscard]] static const char* TypeName(SelectableType type);

  /// Convert a node type string to SelectableType.
  /// @return The corresponding SelectableType, or std::nullopt if not recognized.
  [[nodiscard]] static SelectableType TypeFromString(std::string_view type_str);

  /// Check if a class name indicates a light type.
  [[nodiscard]] static bool IsLightClass(std::string_view class_name);
};

/// State for the selection filter dialog.
struct SelectionFilterDialogState {
  bool open = false;
};

/// Draw the selection filter dialog.
/// @param state Dialog state.
/// @param filter The selection filter to modify.
/// @return true if the filter was modified.
bool DrawSelectionFilterDialog(SelectionFilterDialogState& state, SelectionFilter& filter);
