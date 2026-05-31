#pragma once

/// @file ui_face_properties.h
/// @brief Face Properties Panel for editing texture/UV properties of selected faces.
///
/// Provides an always-visible dockable panel that shows and edits the texture properties
/// of selected brush faces. Supports live UV updates, multi-selection handling, and
/// texture eyedropper functionality.

#include "brush/texture_ops/uv_types.h"
#include "geometry/subobject_selection.h"
#include "undo_stack.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct NodeProperties;
struct TextureBrowserState;

/// State for the texture eyedropper tool.
/// When active, Alt+Click on a face picks its texture and mapping parameters.
struct PickedTextureState {
  bool has_pick = false;               ///< Whether a texture has been picked
  std::string texture_name;            ///< Picked texture path/name
  texture_ops::TextureMapping mapping; ///< Picked UV mapping parameters
  uint32_t surface_flags = 0;          ///< Picked surface flags
  uint8_t alpha_ref = 0;               ///< Picked alpha reference value
};

/// State for the Face Properties Panel.
/// Caches values from selected faces and tracks UI state.
struct FacePropertiesPanel {
  bool visible = true; ///< Panel visibility

  // Cached values from selection (updated each frame)
  std::string texture_name;            ///< Current texture name (or "---" if mixed)
  texture_ops::TextureMapping mapping; ///< Current UV mapping (or identity if mixed)
  uint32_t surface_flags = 0;          ///< Current surface flags
  uint8_t alpha_ref = 0;               ///< Current alpha reference value

  // Multi-selection state
  bool mixed_texture = false;  ///< Multiple different textures selected
  bool mixed_offset = false;   ///< Multiple different UV offsets selected
  bool mixed_scale = false;    ///< Multiple different UV scales selected
  bool mixed_rotation = false; ///< Multiple different rotations selected
  bool mixed_flags = false;    ///< Multiple different surface flags selected
  bool mixed_alpha = false;    ///< Multiple different alpha refs selected

  // Texture dimensions (for Fit operation)
  uint32_t texture_width = 128;
  uint32_t texture_height = 128;

  // UI state for incremental editing
  float edit_offset_u = 0.0f;
  float edit_offset_v = 0.0f;
  float edit_scale_u = 1.0f;
  float edit_scale_v = 1.0f;
  float edit_rotation = 0.0f;
  int edit_alpha = 0;
};

/// Actions returned from the Face Properties Panel.
struct FacePropertiesAction {
  bool offset_changed = false;   ///< UV offset was modified
  bool scale_changed = false;    ///< UV scale was modified
  bool rotation_changed = false; ///< UV rotation was modified
  bool fit_requested = false;    ///< Fit texture to face requested
  bool reset_requested = false;  ///< Reset UV to defaults requested
  bool flip_u_requested = false; ///< Flip U coordinate requested
  bool flip_v_requested = false; ///< Flip V coordinate requested
  bool browse_texture = false;   ///< Open texture browser requested
  bool flags_changed = false;    ///< Surface flags were modified
  bool alpha_changed = false;    ///< Alpha reference was modified
  bool apply_picked = false;     ///< Apply the eyedropper-picked texture to selected faces
  bool committed = false;        ///< The current edit gesture finished this frame (commit point for undo)

  // New values when changed
  float new_offset_u = 0.0f;
  float new_offset_v = 0.0f;
  float new_scale_u = 1.0f;
  float new_scale_v = 1.0f;
  float new_rotation = 0.0f;
  uint32_t new_flags = 0;
  uint8_t new_alpha = 0;
};

/// Update the Face Properties Panel state from the current face selection.
/// Call this each frame before drawing the panel.
/// @param panel Panel state to update.
/// @param selection Current sub-object selection.
/// @param props Node properties array.
void UpdateFacePropertiesFromSelection(FacePropertiesPanel &panel, const SubObjectSelection &selection,
                                       const std::vector<NodeProperties> &props);

/// Draw the Face Properties Panel (as a separate ImGui window).
/// @param panel Panel state (modified in place).
/// @param selection Current sub-object selection (for determining if faces are selected).
/// @param props Node properties array (for reading face data).
/// @param picked Eyedropper-picked texture state (for the "Apply Picked" button).
/// @param action Output actions from user interaction.
void DrawFacePropertiesPanel(FacePropertiesPanel &panel, const SubObjectSelection &selection,
                             const std::vector<NodeProperties> &props, const PickedTextureState &picked,
                             FacePropertiesAction &action);

/// Draw face properties content inline (without creating a window).
/// Use this to embed face properties in another panel.
/// @param panel Panel state (modified in place).
/// @param selection Current sub-object selection.
/// @param props Node properties array (for reading face data).
/// @param picked Eyedropper-picked texture state (for the "Apply Picked" button).
/// @param action Output actions from user interaction.
void DrawFacePropertiesContent(FacePropertiesPanel &panel, const SubObjectSelection &selection,
                               const std::vector<NodeProperties> &props, const PickedTextureState &picked,
                               FacePropertiesAction &action);

/// Apply face properties action to selected faces.
/// @param action The action to apply.
/// @param selection Current face selection.
/// @param props Node properties array (modified in place).
/// @return true if any faces were modified.
bool ApplyFacePropertiesAction(const FacePropertiesAction &action, const SubObjectSelection &selection,
                               std::vector<NodeProperties> &props);

/// Pick texture and mapping from a face.
/// @param face The face to pick from.
/// @param props Node properties array.
/// @param picked Output picked texture state.
/// @return true if pick was successful.
bool PickTextureFromFace(const FaceRef &face, const std::vector<NodeProperties> &props, PickedTextureState &picked);

/// Apply picked texture to selected faces.
/// @param picked The picked texture state.
/// @param selection Current face selection.
/// @param props Node properties array (modified in place).
/// @param apply_mapping Whether to apply UV mapping (true) or just texture name (false).
/// @param apply_flags Whether to apply surface flags.
/// @return Number of faces modified.
size_t ApplyPickedTextureToFaces(const PickedTextureState &picked, const SubObjectSelection &selection,
                                 std::vector<NodeProperties> &props, bool apply_mapping = true,
                                 bool apply_flags = false);

/// Compute a UV mapping that makes the texture span the triangle's in-plane bounding box exactly once.
/// Uses the triangle's own plane: builds a 2D basis (u along edge v0->v1, v = normal x u), projects the
/// three vertices, and sets scale = 1/range and offset = -min/range per axis (rotation 0). "Fit both" semantics.
/// @return identity mapping if the triangle is degenerate (zero area / zero extent).
texture_ops::TextureMapping ComputeFaceFitMapping(const std::array<float, 3> &v0, const std::array<float, 3> &v1,
                                                  const std::array<float, 3> &v2);

/// Compute the fit mapping for a specific triangle of a brush, reading geometry from props.
/// @return std::nullopt if the brush geometry or triangle index is invalid.
std::optional<texture_ops::TextureMapping> ComputeFaceFitMappingForFace(const NodeProperties &props,
                                                                        uint32_t triangle_index);

/// Capture the current texture data of all currently-selected faces (fills `before`; `after` left default).
std::vector<FaceTextureChange> CaptureFaceTextureStates(const SubObjectSelection &selection,
                                                        const std::vector<NodeProperties> &props);

/// Fill each change's `after` from the current face data, and drop entries that did not actually change.
void FinalizeFaceTextureStates(std::vector<FaceTextureChange> &changes, const std::vector<NodeProperties> &props);
