#include "uv_projection_dialog.h"

#include "imgui.h"

UVProjectionDialogResult DrawUVProjectionDialog(UVProjectionDialogState& state) {
  UVProjectionDialogResult result;

  if (!state.open) {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("UV Projection", &state.open)) {
    static const char* projection_types[] = {"Planar X (YZ)", "Planar Y (XZ)", "Planar Z (XY)", "Planar Auto",
                                             "Cylindrical",   "Spherical",     "Box"};
    ImGui::Text("Projection Type");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##projection", &state.projection_type, projection_types, 7);

    ImGui::Separator();

    ImGui::Text("Scale");
    ImGui::SetNextItemWidth(150);
    ImGui::DragFloat("U##scale", &state.scale_u, 0.001f, 0.001f, 1.0f, "%.4f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::DragFloat("V##scale", &state.scale_v, 0.001f, 0.001f, 1.0f, "%.4f");

    // Clamp scale values
    if (state.scale_u < 0.001f) {
      state.scale_u = 0.001f;
    }
    if (state.scale_v < 0.001f) {
      state.scale_v = 0.001f;
    }

    // Show additional parameters for cylindrical/spherical projections
    if (state.projection_type == 4 || state.projection_type == 5) {
      ImGui::Separator();

      ImGui::Text("Axis");
      ImGui::SetNextItemWidth(-1);
      ImGui::DragFloat3("##axis", state.axis, 0.01f, -1.0f, 1.0f, "%.2f");

      ImGui::Text("Center");
      ImGui::SetNextItemWidth(-1);
      ImGui::DragFloat3("##center", state.center, 1.0f);

      if (state.projection_type == 4) {
        ImGui::Text("Radius");
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat("##radius", &state.radius, 1.0f, 1.0f, 1000.0f, "%.1f");
        if (state.radius < 1.0f) {
          state.radius = 1.0f;
        }
      }
    }

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(150, 0))) {
      result.apply = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(150, 0))) {
      state.open = false;
    }
  }
  ImGui::End();

  return result;
}
