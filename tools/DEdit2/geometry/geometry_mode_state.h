#pragma once

#include "edit_mode.h"
#include "subobject_selection.h"

/// State for geometry editing mode management.
/// Tracks the current edit mode and sub-object selection state.
struct GeometryModeState {
  /// Current editing mode (Object, Vertex, Edge, or Face).
  EditMode current_mode = EditMode::Object;

  /// Previous geometry mode, used for Tab toggle functionality.
  /// When switching from a geometry mode to Object mode, this remembers
  /// which geometry mode was active so Tab can toggle back to it.
  EditMode previous_geometry_mode = EditMode::Face;

  /// Sub-object selection state.
  SubObjectSelection selection;

  /// Whether to constrain face movement along face normal.
  bool constrain_to_normal = false;

  /// Whether to preserve UV coordinates during vertex/edge manipulation.
  bool preserve_uvs = true;

  /// Set the current edit mode.
  /// @param new_mode The mode to switch to.
  void SetMode(EditMode new_mode);

  /// Toggle between Object mode and the previous geometry mode.
  /// If in Object mode, switches to previous_geometry_mode.
  /// If in a geometry mode, switches to Object mode.
  void ToggleGeometryMode();

  /// Check if currently in a geometry editing mode.
  [[nodiscard]] bool IsInGeometryMode() const;

  /// Reset to default state (Object mode, clear selection).
  void Reset();
};
