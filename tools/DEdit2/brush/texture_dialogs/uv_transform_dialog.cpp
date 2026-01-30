#include "uv_transform_dialog.h"

#include "imgui.h"

UVTransformDialogResult DrawUVTransformDialog(UVTransformDialogState& state) {
  UVTransformDialogResult result;

  if (!state.open) {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("UV Transform", &state.open)) {
    ImGui::Text("Offset");
    ImGui::SetNextItemWidth(150);
    ImGui::DragFloat("U##offset", &state.offset_u, 0.01f, -1.0f, 1.0f, "%.3f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::DragFloat("V##offset", &state.offset_v, 0.01f, -1.0f, 1.0f, "%.3f");

    ImGui::Separator();

    ImGui::Text("Scale");
    ImGui::SetNextItemWidth(150);
    ImGui::DragFloat("U##scale", &state.scale_u, 0.01f, 0.1f, 4.0f, "%.3f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::DragFloat("V##scale", &state.scale_v, 0.01f, 0.1f, 4.0f, "%.3f");

    // Clamp scale values to prevent degenerate UVs
    if (state.scale_u < 0.01f) {
      state.scale_u = 0.01f;
    }
    if (state.scale_v < 0.01f) {
      state.scale_v = 0.01f;
    }

    ImGui::Separator();

    ImGui::Text("Rotation");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##rotation", &state.rotation, 0.0f, 360.0f, "%.1f deg");

    ImGui::Separator();

    ImGui::Checkbox("Preview", &state.preview);

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(100, 0))) {
      result.apply = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(100, 0))) {
      result.reset = true;
      state.offset_u = 0.0f;
      state.offset_v = 0.0f;
      state.scale_u = 1.0f;
      state.scale_v = 1.0f;
      state.rotation = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(100, 0))) {
      state.open = false;
    }
  }
  ImGui::End();

  return result;
}
