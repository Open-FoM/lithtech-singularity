#include "ui_texture_browser.h"

#include <algorithm>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

// =============================================================================
// Helper Functions (Unit Testable)
// =============================================================================

std::string FormatFileSize(uint32_t bytes) {
  if (bytes < 1024) {
    return std::to_string(bytes) + " B";
  }
  if (bytes < 1024 * 1024) {
    float kb = static_cast<float>(bytes) / 1024.0f;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f KB", kb);
    return buf;
  }
  float mb = static_cast<float>(bytes) / (1024.0f * 1024.0f);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.1f MB", mb);
  return buf;
}

std::string FormatBPP(int bpp) {
  switch (bpp) {
  case 4:
    return "DXT1";
  case 8:
    return "DXT3/5";
  case 16:
    return "RGB16";
  case 24:
    return "RGB24";
  case 32:
    return "RGBA32";
  default:
    return std::to_string(bpp) + "bpp";
  }
}

void SortTextureEntries(std::vector<TextureBrowserEntry>& entries,
                        TextureBrowserSortMode mode,
                        bool ascending) {
  auto compare = [mode, ascending](const TextureBrowserEntry& a, const TextureBrowserEntry& b) {
    int result = 0;
    switch (mode) {
    case TextureBrowserSortMode::Name:
      result = a.name.compare(b.name);
      break;
    case TextureBrowserSortMode::Size:
      result = (a.file_size < b.file_size) ? -1 : (a.file_size > b.file_size ? 1 : 0);
      break;
    case TextureBrowserSortMode::Dimensions: {
      uint64_t area_a = static_cast<uint64_t>(a.width) * a.height;
      uint64_t area_b = static_cast<uint64_t>(b.width) * b.height;
      result = (area_a < area_b) ? -1 : (area_a > area_b ? 1 : 0);
      break;
    }
    }
    return ascending ? (result < 0) : (result > 0);
  };

  std::sort(entries.begin(), entries.end(), compare);
}

std::vector<TextureBrowserEntry> ScanTextureDirectories(const std::string& project_root) {
  std::vector<TextureBrowserEntry> entries;

  if (project_root.empty() || !fs::exists(project_root)) {
    return entries;
  }

  // Common texture directories in LithTech projects
  const std::vector<std::string> texture_dirs = {
      "Textures",
      "textures",
      "Tex",
      "tex",
      "Materials",
      "materials"
  };

  // Supported texture extensions
  const std::vector<std::string> extensions = {
      ".dtx",
      ".tga",
      ".bmp",
      ".png",
      ".jpg",
      ".jpeg",
      ".dds"
  };

  auto is_texture_file = [&extensions](const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    for (const auto& tex_ext : extensions) {
      if (ext == tex_ext) {
        return true;
      }
    }
    return false;
  };

  // Track seen files to avoid duplicates
  std::unordered_set<std::string> seen_paths;

  // Scan each texture directory
  for (const auto& dir_name : texture_dirs) {
    fs::path tex_path = fs::path(project_root) / dir_name;
    if (!fs::exists(tex_path) || !fs::is_directory(tex_path)) {
      continue;
    }

    try {
      for (const auto& entry : fs::recursive_directory_iterator(tex_path)) {
        if (!entry.is_regular_file()) {
          continue;
        }

        if (!is_texture_file(entry.path())) {
          continue;
        }

        // Avoid duplicates by checking canonical path
        std::error_code ec;
        std::string canonical = fs::canonical(entry.path(), ec).string();
        if (!ec) {
          if (seen_paths.count(canonical) > 0) {
            continue;
          }
          seen_paths.insert(canonical);
        }

        TextureBrowserEntry tex_entry;
        tex_entry.name = entry.path().filename().string();
        tex_entry.path = entry.path().string();

        // Compute relative path
        auto rel = fs::relative(entry.path(), project_root);
        tex_entry.relative_path = rel.string();

        // Get file size
        auto file_size = fs::file_size(entry.path(), ec);
        if (!ec) {
          tex_entry.file_size = static_cast<uint32_t>(file_size);
        }

        // Determine format from extension
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        if (!ext.empty() && ext[0] == '.') {
          ext = ext.substr(1);
        }
        tex_entry.format_label = ext;

        entries.push_back(std::move(tex_entry));
      }
    } catch (const fs::filesystem_error&) {
      // Skip directories we can't access
      continue;
    }
  }

  return entries;
}

// =============================================================================
// UI Drawing
// =============================================================================

void DrawTextureBrowserPanel(TextureBrowserState& state,
                             const std::string& project_root,
                             TextureBrowserAction& action) {
  action = TextureBrowserAction{};

  ImGui::Begin("Texture Browser");

  // Refresh button and view mode toggle
  if (ImGui::Button("Refresh")) {
    state.needs_refresh = true;
  }
  ImGui::SameLine();

  // View mode toggle
  if (ImGui::RadioButton("Grid", state.view_mode == TextureBrowserViewMode::Grid)) {
    state.view_mode = TextureBrowserViewMode::Grid;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("List", state.view_mode == TextureBrowserViewMode::List)) {
    state.view_mode = TextureBrowserViewMode::List;
  }

  // Sort mode
  ImGui::SameLine();
  ImGui::Separator();
  ImGui::SameLine();
  ImGui::Text("Sort:");
  ImGui::SameLine();

  const char* sort_items[] = {"Name", "Size", "Dimensions"};
  int sort_index = static_cast<int>(state.sort_mode);
  ImGui::SetNextItemWidth(100);
  if (ImGui::Combo("##sort", &sort_index, sort_items, 3)) {
    state.sort_mode = static_cast<TextureBrowserSortMode>(sort_index);
    SortTextureEntries(state.entries, state.sort_mode, state.sort_ascending);
  }
  ImGui::SameLine();
  if (ImGui::Button(state.sort_ascending ? "Asc" : "Desc")) {
    state.sort_ascending = !state.sort_ascending;
    SortTextureEntries(state.entries, state.sort_mode, state.sort_ascending);
  }

  // Search filter
  state.filter.Draw("Search", -100);

  // Thumbnail size slider (for grid mode)
  if (state.view_mode == TextureBrowserViewMode::Grid) {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("Size", &state.thumbnail_size, 32, 128);
  }

  ImGui::Separator();

  // Refresh entries if needed
  if (state.needs_refresh) {
    state.entries = ScanTextureDirectories(project_root);
    SortTextureEntries(state.entries, state.sort_mode, state.sort_ascending);
    state.needs_refresh = false;
    state.selected_index = -1;
    state.current_texture.clear();
  }

  // Display texture count
  ImGui::Text("%zu textures", state.entries.size());

  ImGui::Separator();

  // Scrollable region for textures
  ImGui::BeginChild("TextureList", ImVec2(0, 0), true);

  if (state.view_mode == TextureBrowserViewMode::Grid) {
    // Grid view - fixed width items with proper column layout
    const float item_spacing = 8.0f;
    const float item_width = static_cast<float>(state.thumbnail_size) + item_spacing;
    float panel_width = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>(panel_width / item_width));
    int col = 0;

    for (size_t i = 0; i < state.entries.size(); ++i) {
      const auto& entry = state.entries[i];

      if (!state.filter.PassFilter(entry.name.c_str())) {
        continue;
      }

      ImGui::PushID(static_cast<int>(i));

      bool is_selected = (static_cast<int>(i) == state.selected_index);

      ImVec2 thumb_size(static_cast<float>(state.thumbnail_size),
                        static_cast<float>(state.thumbnail_size));

      // Group thumbnail and name together for proper layout
      ImGui::BeginGroup();

      if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
      }

      // Use button as thumbnail placeholder
      // Format label as button text for visual identification
      bool clicked = ImGui::Button(entry.format_label.c_str(), thumb_size);

      if (clicked) {
        state.selected_index = static_cast<int>(i);
        state.current_texture = entry.relative_path;
        action.texture_selected = true;
        action.selected_texture = entry.relative_path;
      }

      if (is_selected) {
        ImGui::PopStyleColor();
      }

      // Double-click to apply
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        action.apply_to_faces = true;
        action.selected_texture = entry.relative_path;
      }

      // Tooltip
      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", entry.name.c_str());
        if (entry.width > 0 && entry.height > 0) {
          ImGui::Text("%ux%u", entry.width, entry.height);
        }
        if (entry.file_size > 0) {
          ImGui::Text("%s", FormatFileSize(entry.file_size).c_str());
        }
        ImGui::Text("%s", entry.relative_path.c_str());
        ImGui::EndTooltip();
      }

      // Context menu
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Apply to Selection")) {
          action.apply_to_faces = true;
          action.selected_texture = entry.relative_path;
        }
        if (ImGui::MenuItem("Copy Path")) {
          ImGui::SetClipboardText(entry.relative_path.c_str());
        }
        ImGui::EndPopup();
      }

      // Draw truncated name below thumbnail (fixed width)
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + static_cast<float>(state.thumbnail_size));
      ImGui::TextWrapped("%s", entry.name.c_str());
      ImGui::PopTextWrapPos();

      ImGui::EndGroup();

      ImGui::PopID();

      col++;
      if (col < columns) {
        ImGui::SameLine(0, item_spacing);
      } else {
        col = 0;
      }
    }
  } else {
    // List view
    if (ImGui::BeginTable("TextureTable", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
      ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 60);
      ImGui::TableSetupColumn("Dimensions", ImGuiTableColumnFlags_WidthFixed, 80);
      ImGui::TableHeadersRow();

      for (size_t i = 0; i < state.entries.size(); ++i) {
        const auto& entry = state.entries[i];

        if (!state.filter.PassFilter(entry.name.c_str())) {
          continue;
        }

        ImGui::TableNextRow();

        bool is_selected = (static_cast<int>(i) == state.selected_index);

        ImGui::TableNextColumn();
        if (ImGui::Selectable(entry.name.c_str(), is_selected,
                              ImGuiSelectableFlags_SpanAllColumns)) {
          state.selected_index = static_cast<int>(i);
          state.current_texture = entry.relative_path;
          action.texture_selected = true;
          action.selected_texture = entry.relative_path;
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
          action.apply_to_faces = true;
          action.selected_texture = entry.relative_path;
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
          if (ImGui::MenuItem("Apply to Selection")) {
            action.apply_to_faces = true;
            action.selected_texture = entry.relative_path;
          }
          if (ImGui::MenuItem("Copy Path")) {
            ImGui::SetClipboardText(entry.relative_path.c_str());
          }
          ImGui::EndPopup();
        }

        ImGui::TableNextColumn();
        ImGui::Text("%s", FormatFileSize(entry.file_size).c_str());

        ImGui::TableNextColumn();
        ImGui::Text("%s", entry.format_label.c_str());

        ImGui::TableNextColumn();
        if (entry.width > 0 && entry.height > 0) {
          ImGui::Text("%ux%u", entry.width, entry.height);
        } else {
          ImGui::TextDisabled("--");
        }
      }

      ImGui::EndTable();
    }
  }

  ImGui::EndChild();
  ImGui::End();
}
