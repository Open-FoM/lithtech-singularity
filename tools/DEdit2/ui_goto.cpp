#include "ui_goto.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>

namespace
{

/// Case-insensitive substring search.
bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
{
  if (needle.empty())
  {
    return true;
  }
  if (haystack.size() < needle.size())
  {
    return false;
  }

  auto toLower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };

  auto it = std::search(
      haystack.begin(), haystack.end(),
      needle.begin(), needle.end(),
      [&](char a, char b) { return toLower(a) == toLower(b); });

  return it != haystack.end();
}

/// Collect all node IDs matching the search query.
void CollectMatchingNodes(
    const std::string& query,
    const std::vector<TreeNode>& nodes,
    int node_id,
    std::vector<int>& out_results)
{
  if (node_id < 0 || node_id >= static_cast<int>(nodes.size()))
  {
    return;
  }

  const TreeNode& node = nodes[node_id];
  if (node.deleted)
  {
    return;
  }

  // Check if name matches (case-insensitive contains)
  if (!query.empty() && ContainsIgnoreCase(node.name, query))
  {
    out_results.push_back(node_id);
  }

  // Recurse into children
  for (int child_id : node.children)
  {
    CollectMatchingNodes(query, nodes, child_id, out_results);
  }
}

} // namespace

void OpenGoToDialog(GoToDialogState& state)
{
  state.open = true;
  state.search_buffer[0] = '\0';
  state.search_results.clear();
  state.selected_index = 0;
  state.focus_text_input = true;
}

GoToResult DrawGoToDialog(
    GoToDialogState& state,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props)
{
  GoToResult result;

  if (!state.open)
  {
    return result;
  }

  const char* title = "Go To Node";
  ImGui::OpenPopup(title);

  // Center the dialog
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

  if (ImGui::BeginPopupModal(title, &state.open, ImGuiWindowFlags_None))
  {
    // Search input
    if (state.focus_text_input)
    {
      ImGui::SetKeyboardFocusHere();
      state.focus_text_input = false;
    }

    bool enter_pressed = ImGui::InputText(
        "##search", state.search_buffer, sizeof(state.search_buffer),
        ImGuiInputTextFlags_EnterReturnsTrue);

    // Update search results when text changes
    static char prev_query[256] = {};
    if (std::strcmp(prev_query, state.search_buffer) != 0)
    {
      std::strcpy(prev_query, state.search_buffer);
      state.search_results.clear();
      state.selected_index = 0;

      if (state.search_buffer[0] != '\0' && !nodes.empty())
      {
        CollectMatchingNodes(state.search_buffer, nodes, 0, state.search_results);
      }
    }

    // Handle keyboard navigation
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !state.search_results.empty())
    {
      state.selected_index = std::min(
          state.selected_index + 1,
          static_cast<int>(state.search_results.size()) - 1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !state.search_results.empty())
    {
      state.selected_index = std::max(state.selected_index - 1, 0);
    }

    // Show results
    ImGui::Separator();
    ImGui::Text("Results: %zu", state.search_results.size());

    if (ImGui::BeginChild("##results", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true))
    {
      for (size_t i = 0; i < state.search_results.size(); ++i)
      {
        const int node_id = state.search_results[i];
        if (node_id < 0 || node_id >= static_cast<int>(nodes.size()))
        {
          continue;
        }

        const TreeNode& node = nodes[node_id];
        const bool is_selected = (static_cast<int>(i) == state.selected_index);

        // Build display text with type info
        std::string display_text = node.name;
        if (node_id >= 0 && static_cast<size_t>(node_id) < props.size())
        {
          const std::string& type = props[node_id].type;
          if (!type.empty())
          {
            display_text += " [" + type + "]";
          }
        }

        if (ImGui::Selectable(display_text.c_str(), is_selected,
                               ImGuiSelectableFlags_AllowDoubleClick))
        {
          state.selected_index = static_cast<int>(i);
          if (ImGui::IsMouseDoubleClicked(0))
          {
            result.accepted = true;
            result.selected_id = node_id;
            state.open = false;
          }
        }

        // Auto-scroll to selected item
        if (is_selected)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
    }
    ImGui::EndChild();

    // Buttons
    bool go_pressed = ImGui::Button("Go", ImVec2(80, 0));
    ImGui::SameLine();
    bool cancel_pressed = ImGui::Button("Cancel", ImVec2(80, 0));

    // Handle Enter key or Go button
    if ((enter_pressed || go_pressed) && !state.search_results.empty() &&
        state.selected_index >= 0 &&
        state.selected_index < static_cast<int>(state.search_results.size()))
    {
      result.accepted = true;
      result.selected_id = state.search_results[state.selected_index];
      state.open = false;
      ImGui::CloseCurrentPopup();
    }

    // Handle Escape or Cancel button
    if (cancel_pressed || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      state.open = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  (void)props; // Used for type display

  return result;
}
