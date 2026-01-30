#include "geometry_mode_state.h"

void GeometryModeState::SetMode(EditMode new_mode) {
  if (current_mode == new_mode) {
    return;
  }

  // Remember the current geometry mode before switching
  if (IsGeometryMode(current_mode)) {
    previous_geometry_mode = current_mode;
  }

  // Clear sub-object selection when switching between geometry modes
  // or when returning to object mode
  if (new_mode == EditMode::Object) {
    selection.ClearAll();
  } else if (current_mode != EditMode::Object) {
    // Switching between geometry modes - clear the selection for the old mode
    switch (current_mode) {
      case EditMode::Vertex: selection.ClearVertices(); break;
      case EditMode::Edge: selection.ClearEdges(); break;
      case EditMode::Face: selection.ClearFaces(); break;
      default: break;
    }
  }

  current_mode = new_mode;
}

void GeometryModeState::ToggleGeometryMode() {
  if (current_mode == EditMode::Object) {
    // Switch to the previous geometry mode
    SetMode(previous_geometry_mode);
  } else {
    // Switch back to object mode
    SetMode(EditMode::Object);
  }
}

bool GeometryModeState::IsInGeometryMode() const { return IsGeometryMode(current_mode); }

void GeometryModeState::Reset() {
  current_mode = EditMode::Object;
  previous_geometry_mode = EditMode::Face;
  selection.ClearAll();
  constrain_to_normal = false;
  preserve_uvs = true;
}
