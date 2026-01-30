#include "uv_fit_dialog.h"

#include "imgui.h"

UVFitDialogResult DrawUVFitDialog(UVFitDialogState& state) {
  UVFitDialogResult result;

  if (!state.open) {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Fit Texture", &state.open)) {
    ImGui::Text("Scale");
    ImGui::DragFloat("Scale U", &state.scale_u, 0.01f, 0.001f, 100.0f, "%.4f");
    ImGui::DragFloat("Scale V", &state.scale_v, 0.01f, 0.001f, 100.0f, "%.4f");

    ImGui::Separator();

    ImGui::Checkbox("Maintain Aspect Ratio", &state.maintain_aspect);
    if (state.maintain_aspect) {
      // Lock scale_v to scale_u when aspect is maintained
      state.scale_v = state.scale_u;
    }
    ImGui::Checkbox("Center", &state.center);

    ImGui::Separator();

    ImGui::Text("Padding");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##padding", &state.padding, 0.0f, 0.5f, "%.2f");

    ImGui::Separator();

    if (ImGui::Button("Apply", ImVec2(130, 0))) {
      result.apply = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(130, 0))) {
      state.open = false;
    }
  }
  ImGui::End();

  return result;
}
