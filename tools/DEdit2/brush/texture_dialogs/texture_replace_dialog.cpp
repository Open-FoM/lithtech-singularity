#include "texture_replace_dialog.h"

#include "imgui.h"

TextureReplaceDialogResult DrawTextureReplaceDialog(TextureReplaceDialogState& state) {
  TextureReplaceDialogResult result;

  if (!state.open) {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Replace Texture", &state.open)) {
    ImGui::Text("Find:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##find", state.old_texture, sizeof(state.old_texture));

    ImGui::Text("Replace with:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##replace", state.new_texture, sizeof(state.new_texture));

    ImGui::Separator();

    ImGui::Checkbox("Selection only", &state.selection_only);
    ImGui::Checkbox("Use wildcard pattern (*)", &state.use_pattern);

    if (state.use_pattern) {
      ImGui::TextDisabled("Use * to match any characters, e.g. wall_*");
    }

    ImGui::Separator();

    if (state.show_result) {
      if (state.last_replaced > 0) {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Replaced %zu face(s)", state.last_replaced);
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "No matching textures found");
      }
      ImGui::Separator();
    }

    if (ImGui::Button("Find Next", ImVec2(120, 0))) {
      result.find_next = true;
      state.show_result = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Replace All", ImVec2(120, 0))) {
      result.replace = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(120, 0))) {
      state.open = false;
      state.show_result = false;
    }
  }
  ImGui::End();

  return result;
}
