#include "ui_marker.h"

#include "ui_viewport.h"

#include "imgui.h"

MarkerDialogResult DrawMarkerDialog(MarkerDialogState& dialog_state, ViewportPanelState& viewport)
{
  MarkerDialogResult result{};

  if (!dialog_state.open)
  {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Marker Position", &dialog_state.open))
  {
    ImGui::Text("Construction Marker Coordinates");
    ImGui::Separator();

    ImGui::DragFloat("X", &dialog_state.position[0], 1.0f, 0.0f, 0.0f, "%.2f");
    ImGui::DragFloat("Y", &dialog_state.position[1], 1.0f, 0.0f, 0.0f, "%.2f");
    ImGui::DragFloat("Z", &dialog_state.position[2], 1.0f, 0.0f, 0.0f, "%.2f");

    ImGui::Separator();

    if (ImGui::Button("Set", ImVec2(80, 0)))
    {
      viewport.marker_position[0] = dialog_state.position[0];
      viewport.marker_position[1] = dialog_state.position[1];
      viewport.marker_position[2] = dialog_state.position[2];
      result.applied = true;
      dialog_state.open = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset", ImVec2(80, 0)))
    {
      dialog_state.position[0] = 0.0f;
      dialog_state.position[1] = 0.0f;
      dialog_state.position[2] = 0.0f;
      viewport.marker_position[0] = 0.0f;
      viewport.marker_position[1] = 0.0f;
      viewport.marker_position[2] = 0.0f;
      result.reset = true;
      dialog_state.open = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(80, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      result.cancelled = true;
      dialog_state.open = false;
    }

    ImGui::Separator();

    ImGui::Checkbox("Show Marker", &viewport.show_marker);
  }
  ImGui::End();

  return result;
}
