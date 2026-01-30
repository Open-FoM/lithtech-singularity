#include "surface_flags_dialog.h"

#include "imgui.h"

SurfaceFlagsDialogResult DrawSurfaceFlagsDialog(SurfaceFlagsDialogState& state) {
  SurfaceFlagsDialogResult result;

  if (!state.open) {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Surface Flags", &state.open)) {
    ImGui::Text("Render Flags");
    ImGui::Indent();
    ImGui::Checkbox("Solid", &state.solid);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Surface collides with objects");
    }
    ImGui::Checkbox("Invisible", &state.invisible);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Don't render this surface (nodraw)");
    }
    ImGui::Checkbox("Fullbright", &state.fullbright);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Ignore lighting, render at full intensity");
    }
    ImGui::Checkbox("Transparent", &state.transparent);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Enable alpha blending");
    }
    ImGui::Unindent();

    ImGui::Separator();

    ImGui::Text("Special Flags");
    ImGui::Indent();
    ImGui::Checkbox("Sky", &state.sky);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Sky portal - renders skybox through this face");
    }
    ImGui::Checkbox("Portal", &state.portal);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Visibility portal (can be opened/closed)");
    }
    ImGui::Checkbox("Mirror", &state.mirror);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Reflective surface");
    }
    ImGui::Unindent();

    ImGui::Separator();

    ImGui::Text("Alpha Reference");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##alpha_ref", &state.alpha_ref, 0, 255);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Alpha test threshold (0 = disabled)");
    }

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
