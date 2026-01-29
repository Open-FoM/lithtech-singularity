#include "selection/selection_filter.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace {

/// Convert string to lowercase for case-insensitive comparison.
std::string ToLower(std::string_view str) {
  std::string result;
  result.reserve(str.size());
  for (char c : str) {
    result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return result;
}

} // namespace

bool SelectionFilter::PassesFilter(std::string_view type_str, std::string_view class_name) const {
  if (!filter_active) {
    return true;
  }

  // Determine the SelectableType from the node's type string and class name
  SelectableType node_type = TypeFromString(type_str);

  // Special handling for lights - they may be classified as "Object" type but
  // have light-related class names
  if (node_type == SelectableType::Object && IsLightClass(class_name)) {
    node_type = SelectableType::Light;
  }

  return enabled_types.find(node_type) != enabled_types.end();
}

void SelectionFilter::SetTypeEnabled(SelectableType type, bool enabled) {
  if (enabled) {
    enabled_types.insert(type);
  } else {
    enabled_types.erase(type);
  }
}

bool SelectionFilter::IsTypeEnabled(SelectableType type) const {
  if (!filter_active) {
    return true;
  }
  return enabled_types.find(type) != enabled_types.end();
}

void SelectionFilter::EnableAll() {
  filter_active = false;
  enabled_types.clear();
}

void SelectionFilter::DisableAll() {
  filter_active = true;
  enabled_types.clear();
}

const char* SelectionFilter::TypeName(SelectableType type) {
  switch (type) {
  case SelectableType::Brush:
    return "Brushes";
  case SelectableType::WorldModel:
    return "World Models";
  case SelectableType::Light:
    return "Lights";
  case SelectableType::Object:
    return "Objects";
  case SelectableType::Volume:
    return "Volumes";
  case SelectableType::Prefab:
    return "Prefabs";
  case SelectableType::Container:
    return "Containers";
  }
  return "Unknown";
}

SelectableType SelectionFilter::TypeFromString(std::string_view type_str) {
  std::string lower = ToLower(type_str);

  if (lower == "brush" || lower == "brushlist" || lower == "hull" || lower == "polyhedron") {
    return SelectableType::Brush;
  }
  if (lower == "worldmodel" || lower == "worldmodelbrush" || lower == "door") {
    return SelectableType::WorldModel;
  }
  if (lower == "light" || lower == "pointlight" || lower == "dirlight" || lower == "spotlight" ||
      lower == "directionallight" || lower == "ambientlight" || lower == "staticsunlight") {
    return SelectableType::Light;
  }
  if (lower == "volume" || lower == "physicsvolume" || lower == "aivolume" || lower == "blockingvolume" ||
      lower == "airegion" || lower == "ainode") {
    return SelectableType::Volume;
  }
  if (lower == "prefab" || lower == "prefabref" || lower == "prefabreference") {
    return SelectableType::Prefab;
  }
  if (lower == "container" || lower == "folder" || lower == "group" || lower == "null") {
    return SelectableType::Container;
  }

  // Default to Object for anything else (game objects, triggers, spawners, etc.)
  return SelectableType::Object;
}

bool SelectionFilter::IsLightClass(std::string_view class_name) {
  std::string lower = ToLower(class_name);
  return lower.find("light") != std::string::npos || lower.find("sun") != std::string::npos ||
         lower.find("ambient") != std::string::npos;
}

bool DrawSelectionFilterDialog(SelectionFilterDialogState& state, SelectionFilter& filter) {
  bool modified = false;

  if (!state.open) {
    return false;
  }

  ImGui::SetNextWindowSize(ImVec2(280, 320), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Selection Filter", &state.open)) {
    ImGui::End();
    return false;
  }

  ImGui::TextDisabled("Toggle which object types can be selected.");
  ImGui::Separator();
  ImGui::Spacing();

  // Filter active toggle
  if (ImGui::Checkbox("Enable Filter", &filter.filter_active)) {
    modified = true;
    if (filter.filter_active && filter.enabled_types.empty()) {
      // When enabling filter with nothing enabled, enable all by default
      filter.enabled_types.insert(SelectableType::Brush);
      filter.enabled_types.insert(SelectableType::WorldModel);
      filter.enabled_types.insert(SelectableType::Light);
      filter.enabled_types.insert(SelectableType::Object);
      filter.enabled_types.insert(SelectableType::Volume);
      filter.enabled_types.insert(SelectableType::Prefab);
      filter.enabled_types.insert(SelectableType::Container);
    }
  }

  ImGui::Spacing();
  ImGui::BeginDisabled(!filter.filter_active);

  // Type toggles
  static const SelectableType kAllTypes[] = {
      SelectableType::Brush,  SelectableType::WorldModel, SelectableType::Light,  SelectableType::Object,
      SelectableType::Volume, SelectableType::Prefab,     SelectableType::Container};

  for (SelectableType type : kAllTypes) {
    bool enabled = filter.enabled_types.find(type) != filter.enabled_types.end();
    if (ImGui::Checkbox(SelectionFilter::TypeName(type), &enabled)) {
      filter.SetTypeEnabled(type, enabled);
      modified = true;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Quick actions
  if (ImGui::Button("Enable All")) {
    for (SelectableType type : kAllTypes) {
      filter.enabled_types.insert(type);
    }
    modified = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Disable All")) {
    filter.enabled_types.clear();
    modified = true;
  }

  ImGui::EndDisabled();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Show current filter status
  if (filter.filter_active) {
    int enabled_count = static_cast<int>(filter.enabled_types.size());
    if (enabled_count == 0) {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "All types blocked");
    } else if (enabled_count == 7) {
      ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "All types enabled");
    } else {
      ImGui::Text("%d of 7 types enabled", enabled_count);
    }
  } else {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Filter disabled - all types selectable");
  }

  ImGui::End();
  return modified;
}
