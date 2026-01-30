#pragma once

/// Primary editing mode for the editor.
/// Object mode operates on whole nodes/brushes.
/// Geometry modes operate on sub-objects (vertices, edges, faces) of selected brushes.
enum class EditMode {
  Object,  ///< Select and transform whole objects/brushes (default)
  Vertex,  ///< Select and transform individual vertices
  Edge,    ///< Select and transform edges (pairs of vertices)
  Face     ///< Select and transform polygonal faces
};

/// Returns true if the mode is a geometry editing mode (not Object mode).
[[nodiscard]] inline constexpr bool IsGeometryMode(EditMode mode) {
  return mode != EditMode::Object;
}

/// Returns the display name for an edit mode.
[[nodiscard]] inline constexpr const char* GetEditModeName(EditMode mode) {
  switch (mode) {
    case EditMode::Object: return "Object";
    case EditMode::Vertex: return "Vertex";
    case EditMode::Edge: return "Edge";
    case EditMode::Face: return "Face";
  }
  return "Unknown";
}
