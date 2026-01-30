#pragma once

/// @file geometry_dialog_state.h
/// @brief State structs for geometry operation dialogs (EPIC-09).

/// State for the vertex weld dialog.
struct VertexWeldDialogState {
  bool open = false;
  float threshold = 0.001f; ///< Distance threshold for welding nearby vertices
};

/// State for the face extrusion dialog.
struct FaceExtrudeDialogState {
  bool open = false;
  float distance = 16.0f;        ///< Extrusion distance
  bool along_normal = true;      ///< Extrude along face normal vs global direction
  bool create_side_walls = true; ///< Create connecting geometry
};
