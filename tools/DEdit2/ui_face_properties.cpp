#include "ui_face_properties.h"

#include "editor_state.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <unordered_set>

namespace {

/// Helper to display a value with "---" if mixed.
const char *MixedOrValue(bool mixed, const char *value) { return mixed ? "---" : value; }

/// Helper to check if a float is approximately equal to another.
bool FloatNearlyEqual(float a, float b, float epsilon = 1e-4f) { return std::abs(a - b) < epsilon; }

/// Format surface flags for display.
std::string FormatSurfaceFlags(uint32_t flags) {
  using namespace texture_ops;
  std::string result;
  auto sf = static_cast<SurfaceFlags>(flags);

  if (HasFlag(sf, SurfaceFlags::Solid))
    result += "Solid ";
  if (HasFlag(sf, SurfaceFlags::Invisible))
    result += "Invisible ";
  if (HasFlag(sf, SurfaceFlags::Transparent))
    result += "Transparent ";
  if (HasFlag(sf, SurfaceFlags::Sky))
    result += "Sky ";
  if (HasFlag(sf, SurfaceFlags::Fullbright))
    result += "Fullbright ";
  if (HasFlag(sf, SurfaceFlags::FlatShade))
    result += "FlatShade ";
  if (HasFlag(sf, SurfaceFlags::Lightmap))
    result += "Lightmap ";
  if (HasFlag(sf, SurfaceFlags::Portal))
    result += "Portal ";
  if (HasFlag(sf, SurfaceFlags::Mirror))
    result += "Mirror ";

  if (result.empty())
    result = "None";
  return result;
}

/// Cross product of two 3D vectors.
std::array<float, 3> Cross3(const std::array<float, 3> &a, const std::array<float, 3> &b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

/// Dot product of two 3D vectors.
float Dot3(const std::array<float, 3> &a, const std::array<float, 3> &b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/// Subtract two 3D vectors (a - b).
std::array<float, 3> Sub3(const std::array<float, 3> &a, const std::array<float, 3> &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

/// Normalize a 3D vector; returns the zero vector if the length is below epsilon.
std::array<float, 3> Normalize3(const std::array<float, 3> &v) {
  float len = std::sqrt(Dot3(v, v));
  if (len < 1e-6f) {
    return {0.0f, 0.0f, 0.0f};
  }
  float inv = 1.0f / len;
  return {v[0] * inv, v[1] * inv, v[2] * inv};
}

/// Compare two face texture data records for "no change" detection.
bool SameFaceTex(const BrushFaceTextureData &a, const BrushFaceTextureData &b) {
  return a.texture_name == b.texture_name && a.mapping.offset_u == b.mapping.offset_u &&
         a.mapping.offset_v == b.mapping.offset_v && a.mapping.scale_u == b.mapping.scale_u &&
         a.mapping.scale_v == b.mapping.scale_v && a.mapping.rotation == b.mapping.rotation &&
         a.surface_flags == b.surface_flags && a.alpha_ref == b.alpha_ref;
}

} // namespace

void UpdateFacePropertiesFromSelection(FacePropertiesPanel &panel, const SubObjectSelection &selection,
                                       const std::vector<NodeProperties> &props) {
  // Reset mixed flags
  panel.mixed_texture = false;
  panel.mixed_offset = false;
  panel.mixed_scale = false;
  panel.mixed_rotation = false;
  panel.mixed_flags = false;
  panel.mixed_alpha = false;

  if (!selection.HasFaceSelection()) {
    panel.texture_name.clear();
    panel.mapping = texture_ops::TextureMapping{};
    panel.surface_flags = 0;
    panel.alpha_ref = 0;
    return;
  }

  bool first = true;
  std::string first_texture;
  texture_ops::TextureMapping first_mapping;
  uint32_t first_flags = 0;
  uint8_t first_alpha = 0;

  for (const auto &face : selection.selected_faces) {
    if (face.node_id < 0 || static_cast<size_t>(face.node_id) >= props.size()) {
      continue;
    }

    const auto &node_props = props[face.node_id];
    if (face.triangle_index >= node_props.brush_face_textures.size()) {
      continue;
    }

    const auto &face_tex = node_props.brush_face_textures[face.triangle_index];

    if (first) {
      first_texture = face_tex.texture_name;
      first_mapping = face_tex.mapping;
      first_flags = face_tex.surface_flags;
      first_alpha = face_tex.alpha_ref;
      first = false;
    } else {
      // Compare with first face's values
      if (first_texture != face_tex.texture_name) {
        panel.mixed_texture = true;
      }
      if (!FloatNearlyEqual(first_mapping.offset_u, face_tex.mapping.offset_u) ||
          !FloatNearlyEqual(first_mapping.offset_v, face_tex.mapping.offset_v)) {
        panel.mixed_offset = true;
      }
      if (!FloatNearlyEqual(first_mapping.scale_u, face_tex.mapping.scale_u) ||
          !FloatNearlyEqual(first_mapping.scale_v, face_tex.mapping.scale_v)) {
        panel.mixed_scale = true;
      }
      if (!FloatNearlyEqual(first_mapping.rotation, face_tex.mapping.rotation)) {
        panel.mixed_rotation = true;
      }
      if (first_flags != face_tex.surface_flags) {
        panel.mixed_flags = true;
      }
      if (first_alpha != face_tex.alpha_ref) {
        panel.mixed_alpha = true;
      }
    }
  }

  // Store first face's values (or leave as defaults if no valid faces)
  if (!first) {
    panel.texture_name = first_texture;
    panel.mapping = first_mapping;
    panel.surface_flags = first_flags;
    panel.alpha_ref = first_alpha;

    // Update edit values
    if (!panel.mixed_offset) {
      panel.edit_offset_u = first_mapping.offset_u;
      panel.edit_offset_v = first_mapping.offset_v;
    }
    if (!panel.mixed_scale) {
      panel.edit_scale_u = first_mapping.scale_u;
      panel.edit_scale_v = first_mapping.scale_v;
    }
    if (!panel.mixed_rotation) {
      panel.edit_rotation = first_mapping.rotation;
    }
    if (!panel.mixed_alpha) {
      panel.edit_alpha = static_cast<int>(first_alpha);
    }
  }
}

void DrawFacePropertiesContent(FacePropertiesPanel &panel, const SubObjectSelection &selection,
                               const std::vector<NodeProperties> &props, const PickedTextureState &picked,
                               FacePropertiesAction &action) {
  action = FacePropertiesAction{};

  // Texture section
  ImGui::Text("Texture");
  ImGui::Indent();

  // Texture name with preview placeholder
  const char *tex_display = panel.mixed_texture ? "(multiple)" : panel.texture_name.c_str();
  if (panel.texture_name.empty() && !panel.mixed_texture) {
    tex_display = "(none)";
  }

  // Texture preview button (placeholder for thumbnail)
  ImVec2 preview_size(64, 64);
  ImGui::Button(panel.mixed_texture ? "???" : "TEX", preview_size);
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::Text("%s", tex_display);
    ImGui::EndTooltip();
  }

  ImGui::SameLine();
  ImGui::BeginGroup();
  ImGui::Text("%s", tex_display);
  if (!panel.mixed_texture && panel.texture_width > 0 && panel.texture_height > 0) {
    ImGui::TextDisabled("%u x %u", panel.texture_width, panel.texture_height);
  }
  if (ImGui::Button("Browse...")) {
    action.browse_texture = true;
  }
  if (picked.has_pick) {
    if (ImGui::Button("Apply Picked")) {
      action.apply_picked = true;
      action.committed = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Apply eyedropper texture '%s' to selected faces", picked.texture_name.c_str());
    }
  }
  ImGui::EndGroup();

  ImGui::Unindent();
  ImGui::Separator();

  // UV Transform section
  ImGui::Text("UV Transform");
  ImGui::Indent();

  // Offset
  ImGui::Text("Offset");
  ImGui::SameLine(80);
  ImGui::SetNextItemWidth(80);
  if (panel.mixed_offset) {
    ImGui::TextDisabled("U: ---");
  } else {
    if (ImGui::DragFloat("##offset_u", &panel.edit_offset_u, 0.01f, -1000.0f, 1000.0f, "U: %.2f")) {
      action.offset_changed = true;
      action.new_offset_u = panel.edit_offset_u;
      action.new_offset_v = panel.edit_offset_v;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      action.committed = true;
    }
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80);
  if (panel.mixed_offset) {
    ImGui::TextDisabled("V: ---");
  } else {
    if (ImGui::DragFloat("##offset_v", &panel.edit_offset_v, 0.01f, -1000.0f, 1000.0f, "V: %.2f")) {
      action.offset_changed = true;
      action.new_offset_u = panel.edit_offset_u;
      action.new_offset_v = panel.edit_offset_v;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      action.committed = true;
    }
  }

  // Scale
  ImGui::Text("Scale");
  ImGui::SameLine(80);
  ImGui::SetNextItemWidth(80);
  if (panel.mixed_scale) {
    ImGui::TextDisabled("U: ---");
  } else {
    if (ImGui::DragFloat("##scale_u", &panel.edit_scale_u, 0.01f, 0.001f, 100.0f, "U: %.2f")) {
      action.scale_changed = true;
      action.new_scale_u = panel.edit_scale_u;
      action.new_scale_v = panel.edit_scale_v;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      action.committed = true;
    }
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80);
  if (panel.mixed_scale) {
    ImGui::TextDisabled("V: ---");
  } else {
    if (ImGui::DragFloat("##scale_v", &panel.edit_scale_v, 0.01f, 0.001f, 100.0f, "V: %.2f")) {
      action.scale_changed = true;
      action.new_scale_u = panel.edit_scale_u;
      action.new_scale_v = panel.edit_scale_v;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      action.committed = true;
    }
  }

  // Rotation
  ImGui::Text("Rotation");
  ImGui::SameLine(80);
  ImGui::SetNextItemWidth(164);
  if (panel.mixed_rotation) {
    ImGui::TextDisabled("---");
  } else {
    if (ImGui::DragFloat("##rotation", &panel.edit_rotation, 1.0f, -360.0f, 360.0f, "%.1f deg")) {
      action.rotation_changed = true;
      action.new_rotation = panel.edit_rotation;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      action.committed = true;
    }
  }

  ImGui::Unindent();

  // Action buttons
  ImGui::Spacing();
  if (ImGui::Button("Fit")) {
    action.fit_requested = true;
    action.committed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    action.reset_requested = true;
    action.committed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Flip U")) {
    action.flip_u_requested = true;
    action.committed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Flip V")) {
    action.flip_v_requested = true;
    action.committed = true;
  }

  ImGui::Separator();

  // Surface Flags section
  ImGui::Text("Surface Flags");
  ImGui::Indent();

  using namespace texture_ops;
  auto sf = static_cast<SurfaceFlags>(panel.surface_flags);

  auto DrawFlagCheckbox = [&](const char *label, SurfaceFlags flag) {
    bool has_flag = HasFlag(sf, flag);
    bool changed = false;
    if (panel.mixed_flags) {
      ImGui::BeginDisabled();
      ImGui::Checkbox(label, &has_flag);
      ImGui::EndDisabled();
    } else {
      changed = ImGui::Checkbox(label, &has_flag);
    }
    if (changed) {
      if (has_flag) {
        sf |= flag;
      } else {
        sf &= ~flag;
      }
      action.flags_changed = true;
      action.committed = true;
      action.new_flags = static_cast<uint32_t>(sf);
    }
  };

  DrawFlagCheckbox("Solid", SurfaceFlags::Solid);
  ImGui::SameLine(100);
  DrawFlagCheckbox("Sky", SurfaceFlags::Sky);
  ImGui::SameLine(200);
  DrawFlagCheckbox("Transparent", SurfaceFlags::Transparent);

  DrawFlagCheckbox("Portal", SurfaceFlags::Portal);
  ImGui::SameLine(100);
  DrawFlagCheckbox("Mirror", SurfaceFlags::Mirror);
  ImGui::SameLine(200);
  DrawFlagCheckbox("Fullbright", SurfaceFlags::Fullbright);

  DrawFlagCheckbox("Invisible", SurfaceFlags::Invisible);
  ImGui::SameLine(100);
  DrawFlagCheckbox("Lightmap", SurfaceFlags::Lightmap);
  ImGui::SameLine(200);
  DrawFlagCheckbox("FlatShade", SurfaceFlags::FlatShade);

  ImGui::Unindent();

  // Alpha reference
  ImGui::Spacing();
  ImGui::Text("Alpha:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80);
  if (panel.mixed_alpha) {
    ImGui::TextDisabled("---");
  } else {
    if (ImGui::DragInt("##alpha", &panel.edit_alpha, 1, 0, 255)) {
      action.alpha_changed = true;
      action.new_alpha = static_cast<uint8_t>(panel.edit_alpha);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      action.committed = true;
    }
  }
}

void DrawFacePropertiesPanel(FacePropertiesPanel &panel, const SubObjectSelection &selection,
                             const std::vector<NodeProperties> &props, const PickedTextureState &picked,
                             FacePropertiesAction &action) {
  action = FacePropertiesAction{};

  if (!panel.visible) {
    return;
  }

  ImGui::Begin("Face Properties", &panel.visible);

  const bool has_selection = selection.HasFaceSelection();
  const size_t face_count = selection.selected_faces.size();

  if (!has_selection) {
    ImGui::TextDisabled("No faces selected");
    ImGui::TextDisabled("Enter Face mode (Tab) and select faces");
    ImGui::End();
    return;
  }

  // Selection info
  ImGui::Text("%zu face%s selected", face_count, face_count == 1 ? "" : "s");
  ImGui::Separator();

  DrawFacePropertiesContent(panel, selection, props, picked, action);

  ImGui::End();
}

bool ApplyFacePropertiesAction(const FacePropertiesAction &action, const SubObjectSelection &selection,
                               std::vector<NodeProperties> &props) {
  if (!selection.HasFaceSelection()) {
    return false;
  }

  bool modified = false;

  for (const auto &face : selection.selected_faces) {
    if (face.node_id < 0 || static_cast<size_t>(face.node_id) >= props.size()) {
      continue;
    }

    auto &node_props = props[face.node_id];
    if (face.triangle_index >= node_props.brush_face_textures.size()) {
      continue;
    }

    auto &face_tex = node_props.brush_face_textures[face.triangle_index];

    if (action.offset_changed) {
      face_tex.mapping.offset_u = action.new_offset_u;
      face_tex.mapping.offset_v = action.new_offset_v;
      modified = true;
    }

    if (action.scale_changed) {
      face_tex.mapping.scale_u = action.new_scale_u;
      face_tex.mapping.scale_v = action.new_scale_v;
      modified = true;
    }

    if (action.rotation_changed) {
      face_tex.mapping.rotation = action.new_rotation;
      modified = true;
    }

    if (action.fit_requested) {
      auto fit = ComputeFaceFitMappingForFace(node_props, face.triangle_index);
      if (fit) {
        face_tex.mapping = *fit;
        modified = true;
      }
    }

    if (action.reset_requested) {
      face_tex.mapping = texture_ops::TextureMapping{};
      modified = true;
    }

    if (action.flip_u_requested) {
      face_tex.mapping.scale_u = -face_tex.mapping.scale_u;
      modified = true;
    }

    if (action.flip_v_requested) {
      face_tex.mapping.scale_v = -face_tex.mapping.scale_v;
      modified = true;
    }

    if (action.flags_changed) {
      face_tex.surface_flags = action.new_flags;
      modified = true;
    }

    if (action.alpha_changed) {
      face_tex.alpha_ref = action.new_alpha;
      modified = true;
    }
  }

  return modified;
}

bool PickTextureFromFace(const FaceRef &face, const std::vector<NodeProperties> &props, PickedTextureState &picked) {
  if (!face.IsValid()) {
    return false;
  }

  if (face.node_id < 0 || static_cast<size_t>(face.node_id) >= props.size()) {
    return false;
  }

  const auto &node_props = props[face.node_id];
  if (face.triangle_index >= node_props.brush_face_textures.size()) {
    return false;
  }

  const auto &face_tex = node_props.brush_face_textures[face.triangle_index];

  picked.has_pick = true;
  picked.texture_name = face_tex.texture_name;
  picked.mapping = face_tex.mapping;
  picked.surface_flags = face_tex.surface_flags;
  picked.alpha_ref = face_tex.alpha_ref;

  return true;
}

size_t ApplyPickedTextureToFaces(const PickedTextureState &picked, const SubObjectSelection &selection,
                                 std::vector<NodeProperties> &props, bool apply_mapping, bool apply_flags) {
  if (!picked.has_pick || !selection.HasFaceSelection()) {
    return 0;
  }

  size_t count = 0;

  for (const auto &face : selection.selected_faces) {
    if (face.node_id < 0 || static_cast<size_t>(face.node_id) >= props.size()) {
      continue;
    }

    auto &node_props = props[face.node_id];
    if (face.triangle_index >= node_props.brush_face_textures.size()) {
      continue;
    }

    auto &face_tex = node_props.brush_face_textures[face.triangle_index];

    // Always apply texture name
    face_tex.texture_name = picked.texture_name;

    if (apply_mapping) {
      face_tex.mapping = picked.mapping;
    }

    if (apply_flags) {
      face_tex.surface_flags = picked.surface_flags;
      face_tex.alpha_ref = picked.alpha_ref;
    }

    ++count;
  }

  return count;
}

texture_ops::TextureMapping ComputeFaceFitMapping(const std::array<float, 3> &v0, const std::array<float, 3> &v1,
                                                  const std::array<float, 3> &v2) {
  const std::array<float, 3> e1 = Sub3(v1, v0);
  const std::array<float, 3> e2 = Sub3(v2, v0);
  const std::array<float, 3> normal = Cross3(e1, e2);

  if (std::sqrt(Dot3(normal, normal)) < 1e-6f) {
    return texture_ops::TextureMapping{};
  }

  const std::array<float, 3> u_axis = Normalize3(e1);
  const std::array<float, 3> n = Normalize3(normal);
  const std::array<float, 3> v_axis = Cross3(n, u_axis);

  float min_u = 0.0f, max_u = 0.0f, min_v = 0.0f, max_v = 0.0f;
  bool first = true;
  for (const std::array<float, 3> &p : {v0, v1, v2}) {
    const std::array<float, 3> d = Sub3(p, v0);
    const float pu = Dot3(d, u_axis);
    const float pv = Dot3(d, v_axis);
    if (first) {
      min_u = max_u = pu;
      min_v = max_v = pv;
      first = false;
    } else {
      min_u = std::min(min_u, pu);
      max_u = std::max(max_u, pu);
      min_v = std::min(min_v, pv);
      max_v = std::max(max_v, pv);
    }
  }

  const float range_u = max_u - min_u;
  const float range_v = max_v - min_v;
  if (range_u < 1e-6f || range_v < 1e-6f) {
    return texture_ops::TextureMapping{};
  }

  texture_ops::TextureMapping m;
  m.scale_u = 1.0f / range_u;
  m.scale_v = 1.0f / range_v;
  m.offset_u = -min_u / range_u;
  m.offset_v = -min_v / range_v;
  m.rotation = 0.0f;
  return m;
}

std::optional<texture_ops::TextureMapping> ComputeFaceFitMappingForFace(const NodeProperties &props,
                                                                        uint32_t triangle_index) {
  if (props.brush_vertices.empty() || props.brush_indices.empty()) {
    return std::nullopt;
  }

  const size_t idx_base = static_cast<size_t>(triangle_index) * 3;
  if (idx_base + 2 >= props.brush_indices.size()) {
    return std::nullopt;
  }

  const uint32_t i0 = props.brush_indices[idx_base];
  const uint32_t i1 = props.brush_indices[idx_base + 1];
  const uint32_t i2 = props.brush_indices[idx_base + 2];

  if (static_cast<size_t>(i0) * 3 + 2 >= props.brush_vertices.size() ||
      static_cast<size_t>(i1) * 3 + 2 >= props.brush_vertices.size() ||
      static_cast<size_t>(i2) * 3 + 2 >= props.brush_vertices.size()) {
    return std::nullopt;
  }

  const std::array<float, 3> v0 = {props.brush_vertices[static_cast<size_t>(i0) * 3 + 0],
                                   props.brush_vertices[static_cast<size_t>(i0) * 3 + 1],
                                   props.brush_vertices[static_cast<size_t>(i0) * 3 + 2]};
  const std::array<float, 3> v1 = {props.brush_vertices[static_cast<size_t>(i1) * 3 + 0],
                                   props.brush_vertices[static_cast<size_t>(i1) * 3 + 1],
                                   props.brush_vertices[static_cast<size_t>(i1) * 3 + 2]};
  const std::array<float, 3> v2 = {props.brush_vertices[static_cast<size_t>(i2) * 3 + 0],
                                   props.brush_vertices[static_cast<size_t>(i2) * 3 + 1],
                                   props.brush_vertices[static_cast<size_t>(i2) * 3 + 2]};

  return ComputeFaceFitMapping(v0, v1, v2);
}

std::vector<FaceTextureChange> CaptureFaceTextureStates(const SubObjectSelection &selection,
                                                        const std::vector<NodeProperties> &props) {
  std::vector<FaceTextureChange> changes;
  for (const auto &face : selection.selected_faces) {
    if (face.node_id < 0 || static_cast<size_t>(face.node_id) >= props.size()) {
      continue;
    }
    const auto &node_props = props[face.node_id];
    if (face.triangle_index >= node_props.brush_face_textures.size()) {
      continue;
    }
    FaceTextureChange change;
    change.node_id = face.node_id;
    change.triangle_index = face.triangle_index;
    change.before = node_props.brush_face_textures[face.triangle_index];
    change.after = BrushFaceTextureData{};
    changes.push_back(std::move(change));
  }
  return changes;
}

void FinalizeFaceTextureStates(std::vector<FaceTextureChange> &changes, const std::vector<NodeProperties> &props) {
  for (auto &change : changes) {
    if (change.node_id < 0 || static_cast<size_t>(change.node_id) >= props.size()) {
      continue;
    }
    const auto &node_props = props[change.node_id];
    if (change.triangle_index >= node_props.brush_face_textures.size()) {
      continue;
    }
    change.after = node_props.brush_face_textures[change.triangle_index];
  }

  changes.erase(std::remove_if(changes.begin(), changes.end(),
                               [](const FaceTextureChange &c) { return SameFaceTex(c.before, c.after); }),
                changes.end());
}
