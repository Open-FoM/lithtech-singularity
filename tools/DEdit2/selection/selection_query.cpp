#include "selection/selection_query.h"

#include "editor_state.h"
#include "ui_scene.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>

namespace {

/// Case-insensitive string comparison.
bool StrEqualsCI(std::string_view a, std::string_view b)
{
  if (a.size() != b.size())
  {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i)
  {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i])))
    {
      return false;
    }
  }
  return true;
}

/// Case-insensitive substring search.
bool StrContainsCI(std::string_view haystack, std::string_view needle)
{
  if (needle.empty())
  {
    return true;
  }
  if (haystack.size() < needle.size())
  {
    return false;
  }
  for (size_t i = 0; i <= haystack.size() - needle.size(); ++i)
  {
    bool match = true;
    for (size_t j = 0; j < needle.size(); ++j)
    {
      if (std::tolower(static_cast<unsigned char>(haystack[i + j])) !=
          std::tolower(static_cast<unsigned char>(needle[j])))
      {
        match = false;
        break;
      }
    }
    if (match)
    {
      return true;
    }
  }
  return false;
}

/// Case-insensitive prefix match.
bool StrStartsWithCI(std::string_view str, std::string_view prefix)
{
  if (str.size() < prefix.size())
  {
    return false;
  }
  for (size_t i = 0; i < prefix.size(); ++i)
  {
    if (std::tolower(static_cast<unsigned char>(str[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i])))
    {
      return false;
    }
  }
  return true;
}

/// Case-insensitive suffix match.
bool StrEndsWithCI(std::string_view str, std::string_view suffix)
{
  if (str.size() < suffix.size())
  {
    return false;
  }
  const size_t offset = str.size() - suffix.size();
  for (size_t i = 0; i < suffix.size(); ++i)
  {
    if (std::tolower(static_cast<unsigned char>(str[offset + i])) !=
        std::tolower(static_cast<unsigned char>(suffix[i])))
    {
      return false;
    }
  }
  return true;
}

/// Wildcard pattern match (* and ?).
/// * matches any number of characters (including zero)
/// ? matches exactly one character
bool WildcardMatchCI(std::string_view str, std::string_view pattern)
{
  size_t s = 0;
  size_t p = 0;
  size_t star_idx = std::string_view::npos;
  size_t match_idx = 0;

  while (s < str.size())
  {
    if (p < pattern.size() &&
        (std::tolower(static_cast<unsigned char>(pattern[p])) ==
           std::tolower(static_cast<unsigned char>(str[s])) ||
         pattern[p] == '?'))
    {
      ++s;
      ++p;
    }
    else if (p < pattern.size() && pattern[p] == '*')
    {
      star_idx = p;
      match_idx = s;
      ++p;
    }
    else if (star_idx != std::string_view::npos)
    {
      p = star_idx + 1;
      ++match_idx;
      s = match_idx;
    }
    else
    {
      return false;
    }
  }

  while (p < pattern.size() && pattern[p] == '*')
  {
    ++p;
  }

  return p == pattern.size();
}

/// Get the field value from a node.
std::string_view GetFieldValue(
  SelectionCriterionField field,
  const TreeNode& node,
  const NodeProperties& props)
{
  switch (field)
  {
    case SelectionCriterionField::Name:
      return node.name;
    case SelectionCriterionField::Type:
      return props.type;
    case SelectionCriterionField::ClassName:
      return props.class_name;
    case SelectionCriterionField::Property:
      return ""; // Not yet implemented
  }
  return "";
}

} // namespace

bool EvaluateCriterion(
  const SelectionCriterion& criterion,
  const TreeNode& node,
  const NodeProperties& props)
{
  const std::string_view field_value = GetFieldValue(criterion.field, node, props);

  switch (criterion.op)
  {
    case SelectionCriterionOp::Equals:
      return StrEqualsCI(field_value, criterion.value);
    case SelectionCriterionOp::NotEquals:
      return !StrEqualsCI(field_value, criterion.value);
    case SelectionCriterionOp::Contains:
      return StrContainsCI(field_value, criterion.value);
    case SelectionCriterionOp::StartsWith:
      return StrStartsWithCI(field_value, criterion.value);
    case SelectionCriterionOp::EndsWith:
      return StrEndsWithCI(field_value, criterion.value);
    case SelectionCriterionOp::Matches:
      return WildcardMatchCI(field_value, criterion.value);
  }
  return false;
}

bool EvaluateQuery(
  const SelectionQuery& query,
  const TreeNode& node,
  const NodeProperties& props)
{
  if (query.criteria.empty())
  {
    return false;
  }

  if (query.combiner == SelectionQueryCombiner::All)
  {
    // AND - all criteria must match
    for (const auto& criterion : query.criteria)
    {
      if (!EvaluateCriterion(criterion, node, props))
      {
        return false;
      }
    }
    return true;
  }
  else
  {
    // OR - at least one criterion must match
    for (const auto& criterion : query.criteria)
    {
      if (EvaluateCriterion(criterion, node, props))
      {
        return true;
      }
    }
    return false;
  }
}

int CountQueryMatches(
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props)
{
  int count = 0;
  const size_t n = std::min(nodes.size(), props.size());
  for (size_t i = 0; i < n; ++i)
  {
    const TreeNode& node = nodes[i];
    if (node.deleted || node.is_folder)
    {
      continue;
    }
    const NodeProperties& prop = props[i];
    if (!prop.visible || prop.frozen)
    {
      continue;
    }
    if (EvaluateQuery(query, node, prop))
    {
      ++count;
    }
  }
  return count;
}

void SelectByQuery(
  ScenePanelState& scene_panel,
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props)
{
  scene_panel.selected_ids.clear();
  const size_t n = std::min(nodes.size(), props.size());
  for (size_t i = 0; i < n; ++i)
  {
    const TreeNode& node = nodes[i];
    if (node.deleted || node.is_folder)
    {
      continue;
    }
    const NodeProperties& prop = props[i];
    if (!prop.visible || prop.frozen)
    {
      continue;
    }
    if (EvaluateQuery(query, node, prop))
    {
      scene_panel.selected_ids.insert(static_cast<int>(i));
    }
  }
  if (!scene_panel.selected_ids.empty())
  {
    scene_panel.primary_selection = *scene_panel.selected_ids.begin();
  }
  else
  {
    scene_panel.primary_selection = -1;
  }
}

void AddToSelectionByQuery(
  ScenePanelState& scene_panel,
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props)
{
  const size_t n = std::min(nodes.size(), props.size());
  for (size_t i = 0; i < n; ++i)
  {
    const TreeNode& node = nodes[i];
    if (node.deleted || node.is_folder)
    {
      continue;
    }
    const NodeProperties& prop = props[i];
    if (!prop.visible || prop.frozen)
    {
      continue;
    }
    if (EvaluateQuery(query, node, prop))
    {
      scene_panel.selected_ids.insert(static_cast<int>(i));
    }
  }
  if (scene_panel.primary_selection < 0 && !scene_panel.selected_ids.empty())
  {
    scene_panel.primary_selection = *scene_panel.selected_ids.begin();
  }
}

void RemoveFromSelectionByQuery(
  ScenePanelState& scene_panel,
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props)
{
  const size_t n = std::min(nodes.size(), props.size());
  for (size_t i = 0; i < n; ++i)
  {
    const TreeNode& node = nodes[i];
    if (node.deleted || node.is_folder)
    {
      continue;
    }
    const NodeProperties& prop = props[i];
    if (EvaluateQuery(query, node, prop))
    {
      scene_panel.selected_ids.erase(static_cast<int>(i));
    }
  }
  // Update primary selection if it was removed
  if (scene_panel.selected_ids.find(scene_panel.primary_selection) ==
      scene_panel.selected_ids.end())
  {
    if (!scene_panel.selected_ids.empty())
    {
      scene_panel.primary_selection = *scene_panel.selected_ids.begin();
    }
    else
    {
      scene_panel.primary_selection = -1;
    }
  }
}

const char* GetFieldLabel(SelectionCriterionField field)
{
  switch (field)
  {
    case SelectionCriterionField::Name:
      return "Name";
    case SelectionCriterionField::Type:
      return "Type";
    case SelectionCriterionField::ClassName:
      return "Class";
    case SelectionCriterionField::Property:
      return "Property";
  }
  return "Unknown";
}

const char* GetOperatorLabel(SelectionCriterionOp op)
{
  switch (op)
  {
    case SelectionCriterionOp::Equals:
      return "Equals";
    case SelectionCriterionOp::NotEquals:
      return "Not Equals";
    case SelectionCriterionOp::Contains:
      return "Contains";
    case SelectionCriterionOp::StartsWith:
      return "Starts With";
    case SelectionCriterionOp::EndsWith:
      return "Ends With";
    case SelectionCriterionOp::Matches:
      return "Matches";
  }
  return "Unknown";
}

const char* GetCombinerLabel(SelectionQueryCombiner combiner)
{
  switch (combiner)
  {
    case SelectionQueryCombiner::All:
      return "All (AND)";
    case SelectionQueryCombiner::Any:
      return "Any (OR)";
  }
  return "Unknown";
}

void DrawAdvancedSelectionDialog(
  AdvancedSelectionDialogState& state,
  ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props)
{
  if (!state.open)
  {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Advanced Selection", &state.open))
  {
    ImGui::End();
    return;
  }

  // Criteria section
  ImGui::TextUnformatted("Criteria:");
  ImGui::Separator();

  int remove_idx = -1;
  for (size_t i = 0; i < state.current_query.criteria.size(); ++i)
  {
    SelectionCriterion& criterion = state.current_query.criteria[i];
    ImGui::PushID(static_cast<int>(i));

    // Field combo
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::BeginCombo("##Field", GetFieldLabel(criterion.field)))
    {
      for (int f = 0; f <= static_cast<int>(SelectionCriterionField::ClassName); ++f)
      {
        const auto field = static_cast<SelectionCriterionField>(f);
        if (ImGui::Selectable(GetFieldLabel(field), criterion.field == field))
        {
          criterion.field = field;
        }
      }
      ImGui::EndCombo();
    }

    ImGui::SameLine();

    // Operator combo
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::BeginCombo("##Op", GetOperatorLabel(criterion.op)))
    {
      for (int o = 0; o <= static_cast<int>(SelectionCriterionOp::Matches); ++o)
      {
        const auto op = static_cast<SelectionCriterionOp>(o);
        if (ImGui::Selectable(GetOperatorLabel(op), criterion.op == op))
        {
          criterion.op = op;
        }
      }
      ImGui::EndCombo();
    }

    ImGui::SameLine();

    // Value input
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30.0f);
    char value_buf[256];
    strncpy(value_buf, criterion.value.c_str(), sizeof(value_buf) - 1);
    value_buf[sizeof(value_buf) - 1] = '\0';
    if (ImGui::InputText("##Value", value_buf, sizeof(value_buf)))
    {
      criterion.value = value_buf;
    }

    ImGui::SameLine();

    // Remove button
    if (ImGui::Button("X"))
    {
      remove_idx = static_cast<int>(i);
    }

    ImGui::PopID();
  }

  // Remove criterion if marked
  if (remove_idx >= 0)
  {
    state.current_query.criteria.erase(
      state.current_query.criteria.begin() + remove_idx);
  }

  // Add criterion button
  if (ImGui::Button("+ Add Criterion"))
  {
    state.current_query.criteria.push_back(SelectionCriterion{});
  }

  ImGui::Separator();

  // Combiner selection
  ImGui::TextUnformatted("Combine:");
  ImGui::SameLine();
  if (ImGui::RadioButton("All (AND)",
      state.current_query.combiner == SelectionQueryCombiner::All))
  {
    state.current_query.combiner = SelectionQueryCombiner::All;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Any (OR)",
      state.current_query.combiner == SelectionQueryCombiner::Any))
  {
    state.current_query.combiner = SelectionQueryCombiner::Any;
  }

  ImGui::Separator();

  // Match count preview
  const int match_count = CountQueryMatches(state.current_query, nodes, props);
  ImGui::Text("Matches: %d nodes", match_count);

  ImGui::Separator();

  // Action buttons
  const bool has_matches = match_count > 0;
  if (ImGui::Button("Select") && has_matches)
  {
    SelectByQuery(scene_panel, state.current_query, nodes, props);
  }
  ImGui::SameLine();
  if (ImGui::Button("Add to Selection") && has_matches)
  {
    AddToSelectionByQuery(scene_panel, state.current_query, nodes, props);
  }
  ImGui::SameLine();
  if (ImGui::Button("Remove from Selection") && has_matches)
  {
    RemoveFromSelectionByQuery(scene_panel, state.current_query, nodes, props);
  }

  ImGui::Separator();

  // Presets section
  ImGui::TextUnformatted("Presets:");
  for (size_t i = 0; i < state.presets.size(); ++i)
  {
    const auto& preset = state.presets[i];
    ImGui::PushID(static_cast<int>(i));
    if (ImGui::Button(preset.name.c_str()))
    {
      state.current_query = preset.query;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("X"))
    {
      state.presets.erase(state.presets.begin() + static_cast<ptrdiff_t>(i));
      ImGui::PopID();
      break;
    }
    ImGui::PopID();
  }

  // Save preset
  char preset_name_buf[64];
  strncpy(preset_name_buf, state.new_preset_name.c_str(), sizeof(preset_name_buf) - 1);
  preset_name_buf[sizeof(preset_name_buf) - 1] = '\0';
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::InputText("##PresetName", preset_name_buf, sizeof(preset_name_buf)))
  {
    state.new_preset_name = preset_name_buf;
  }
  ImGui::SameLine();
  if (ImGui::Button("Save Preset") && !state.new_preset_name.empty() &&
      !state.current_query.IsEmpty())
  {
    SelectionQueryPreset preset;
    preset.name = state.new_preset_name;
    preset.query = state.current_query;
    state.presets.push_back(std::move(preset));
    state.new_preset_name.clear();
  }

  ImGui::End();
}
