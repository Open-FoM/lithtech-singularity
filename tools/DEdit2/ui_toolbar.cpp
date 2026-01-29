#include "ui_toolbar.h"

#include "imgui.h"

#include <algorithm>
#include <cstdio>

namespace {

/// Draw a simple tool icon for the toolbar.
void DrawToolbarIcon(ImDrawList* draw_list, ImVec2 min, ImVec2 max, EditorTool tool, bool selected, bool hovered)
{
  const ImU32 bg_color = selected ? IM_COL32(70, 110, 180, 255)
    : (hovered ? IM_COL32(60, 60, 70, 255) : IM_COL32(45, 45, 50, 255));
  const ImU32 fg_color = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 190, 255);

  const float cx = (min.x + max.x) * 0.5f;
  const float cy = (min.y + max.y) * 0.5f;
  const float w = max.x - min.x;
  const float h = max.y - min.y;
  const float s = std::min(w, h) * 0.30f;

  draw_list->AddRectFilled(min, max, bg_color, 3.0f);

  switch (tool)
  {
  case EditorTool::Select:
    {
      const ImVec2 p1(cx - s * 0.4f, cy - s * 0.7f);
      const ImVec2 p2(cx - s * 0.4f, cy + s * 0.3f);
      const ImVec2 p3(cx + s * 0.4f, cy + s * 0.3f);
      draw_list->AddTriangleFilled(p1, p2, p3, fg_color);
    }
    break;

  case EditorTool::Move:
    {
      const float arrow_len = s * 0.6f;
      const float arrow_head = s * 0.2f;
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx, cy - arrow_len), fg_color, 1.5f);
      draw_list->AddTriangleFilled(
        ImVec2(cx, cy - arrow_len - arrow_head),
        ImVec2(cx - arrow_head * 0.5f, cy - arrow_len),
        ImVec2(cx + arrow_head * 0.5f, cy - arrow_len),
        fg_color);
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx, cy + arrow_len), fg_color, 1.5f);
      draw_list->AddTriangleFilled(
        ImVec2(cx, cy + arrow_len + arrow_head),
        ImVec2(cx - arrow_head * 0.5f, cy + arrow_len),
        ImVec2(cx + arrow_head * 0.5f, cy + arrow_len),
        fg_color);
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx - arrow_len, cy), fg_color, 1.5f);
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx + arrow_len, cy), fg_color, 1.5f);
    }
    break;

  case EditorTool::Rotate:
    {
      const float r = s * 0.5f;
      draw_list->AddCircle(ImVec2(cx, cy), r, fg_color, 16, 1.5f);
      draw_list->AddTriangleFilled(
        ImVec2(cx + r * 0.3f, cy - r - s * 0.12f),
        ImVec2(cx - s * 0.1f, cy - r),
        ImVec2(cx + s * 0.1f, cy - r + s * 0.1f),
        fg_color);
    }
    break;

  case EditorTool::Scale:
    {
      const float box_s = s * 0.4f;
      const float handle_s = s * 0.15f;
      draw_list->AddRect(
        ImVec2(cx - box_s, cy - box_s),
        ImVec2(cx + box_s, cy + box_s),
        fg_color, 0.0f, 0, 1.2f);
      draw_list->AddRectFilled(
        ImVec2(cx + box_s - handle_s, cy - box_s - handle_s),
        ImVec2(cx + box_s + handle_s, cy - box_s + handle_s),
        fg_color);
      draw_list->AddRectFilled(
        ImVec2(cx + box_s - handle_s, cy + box_s - handle_s),
        ImVec2(cx + box_s + handle_s, cy + box_s + handle_s),
        fg_color);
    }
    break;

  case EditorTool::Box:
    {
      const float bs = s * 0.5f;
      draw_list->AddRectFilled(
        ImVec2(cx - bs, cy - bs * 0.3f),
        ImVec2(cx + bs, cy + bs * 0.7f),
        fg_color);
    }
    break;

  case EditorTool::Cylinder:
    {
      const float cyl_w = s * 0.5f;
      const float cyl_h = s * 0.7f;
      draw_list->AddRectFilled(
        ImVec2(cx - cyl_w * 0.5f, cy - cyl_h * 0.4f),
        ImVec2(cx + cyl_w * 0.5f, cy + cyl_h * 0.4f),
        fg_color);
      draw_list->AddEllipseFilled(
        ImVec2(cx, cy - cyl_h * 0.4f),
        ImVec2(cyl_w * 0.5f, cyl_w * 0.2f),
        IM_COL32(
          (fg_color & 0xFF) * 200 / 255,
          ((fg_color >> 8) & 0xFF) * 200 / 255,
          ((fg_color >> 16) & 0xFF) * 200 / 255,
          255));
    }
    break;

  case EditorTool::Pyramid:
    {
      const float pyr_w = s * 0.7f;
      const float pyr_h = s * 0.7f;
      draw_list->AddTriangleFilled(
        ImVec2(cx, cy - pyr_h * 0.5f),
        ImVec2(cx - pyr_w * 0.5f, cy + pyr_h * 0.5f),
        ImVec2(cx + pyr_w * 0.5f, cy + pyr_h * 0.5f),
        fg_color);
    }
    break;

  case EditorTool::Sphere:
    {
      const float sph_r = s * 0.5f;
      draw_list->AddCircleFilled(ImVec2(cx, cy), sph_r, fg_color, 16);
    }
    break;

  case EditorTool::Dome:
    {
      const float dome_r = s * 0.5f;
      draw_list->PathArcTo(ImVec2(cx, cy + dome_r * 0.3f), dome_r, 3.14159f, 0.0f, 10);
      draw_list->PathFillConvex(fg_color);
    }
    break;

  case EditorTool::Plane:
    {
      const float plane_w = s * 0.7f;
      const float plane_h = s * 0.4f;
      const float skew = s * 0.2f;
      const ImVec2 pts[4] = {
        ImVec2(cx - plane_w * 0.5f + skew, cy - plane_h * 0.5f),
        ImVec2(cx + plane_w * 0.5f + skew, cy - plane_h * 0.5f),
        ImVec2(cx + plane_w * 0.5f - skew, cy + plane_h * 0.5f),
        ImVec2(cx - plane_w * 0.5f - skew, cy + plane_h * 0.5f)
      };
      draw_list->AddConvexPolyFilled(pts, 4, fg_color);
    }
    break;

  default:
    break;
  }
}

/// Draw an undo/redo icon.
void DrawUndoRedoIcon(ImDrawList* draw_list, ImVec2 min, ImVec2 max, bool is_redo, bool enabled, bool hovered)
{
  const ImU32 bg_color = hovered && enabled ? IM_COL32(60, 60, 70, 255) : IM_COL32(45, 45, 50, 255);
  const ImU32 fg_color = enabled ? IM_COL32(180, 180, 190, 255) : IM_COL32(80, 80, 90, 255);

  const float cx = (min.x + max.x) * 0.5f;
  const float cy = (min.y + max.y) * 0.5f;
  const float w = max.x - min.x;
  const float h = max.y - min.y;
  const float r = std::min(w, h) * 0.25f;

  draw_list->AddRectFilled(min, max, bg_color, 3.0f);

  // Curved arrow
  const float dir = is_redo ? 1.0f : -1.0f;
  const float start_angle = is_redo ? 2.5f : 0.6f;
  const float end_angle = is_redo ? 0.6f : 2.5f;
  draw_list->PathArcTo(ImVec2(cx, cy), r, start_angle, end_angle, 8);
  draw_list->PathStroke(fg_color, ImDrawFlags_None, 1.5f);

  // Arrow head
  const float head_x = cx + r * dir * 0.7f;
  const float head_y = cy - r * 0.3f;
  const float head_size = r * 0.4f;
  if (is_redo)
  {
    draw_list->AddTriangleFilled(
      ImVec2(head_x + head_size, head_y),
      ImVec2(head_x - head_size * 0.3f, head_y - head_size * 0.5f),
      ImVec2(head_x - head_size * 0.3f, head_y + head_size * 0.5f),
      fg_color);
  }
  else
  {
    draw_list->AddTriangleFilled(
      ImVec2(head_x - head_size, head_y),
      ImVec2(head_x + head_size * 0.3f, head_y - head_size * 0.5f),
      ImVec2(head_x + head_size * 0.3f, head_y + head_size * 0.5f),
      fg_color);
  }
}

} // namespace

float GetToolbarHeight()
{
  return 32.0f;
}

namespace {

/// Draw a snap toggle icon.
void DrawSnapIcon(ImDrawList* draw_list, ImVec2 min, ImVec2 max, bool enabled, bool hovered)
{
  const ImU32 bg_color = enabled ? IM_COL32(70, 130, 80, 255)
    : (hovered ? IM_COL32(60, 60, 70, 255) : IM_COL32(45, 45, 50, 255));
  const ImU32 fg_color = enabled ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 190, 255);

  const float cx = (min.x + max.x) * 0.5f;
  const float cy = (min.y + max.y) * 0.5f;
  const float w = max.x - min.x;
  const float h = max.y - min.y;
  const float s = std::min(w, h) * 0.30f;

  draw_list->AddRectFilled(min, max, bg_color, 3.0f);

  // Draw a magnet-like snap icon (grid with snap lines)
  const float grid_size = s * 0.4f;
  draw_list->AddRect(
    ImVec2(cx - grid_size, cy - grid_size),
    ImVec2(cx + grid_size, cy + grid_size),
    fg_color, 0.0f, 0, 1.2f);
  draw_list->AddLine(ImVec2(cx, cy - grid_size), ImVec2(cx, cy + grid_size), fg_color, 1.0f);
  draw_list->AddLine(ImVec2(cx - grid_size, cy), ImVec2(cx + grid_size, cy), fg_color, 1.0f);

  // Add corner dots to suggest snap points
  const float dot_r = s * 0.1f;
  draw_list->AddCircleFilled(ImVec2(cx - grid_size, cy - grid_size), dot_r, fg_color);
  draw_list->AddCircleFilled(ImVec2(cx + grid_size, cy - grid_size), dot_r, fg_color);
  draw_list->AddCircleFilled(ImVec2(cx - grid_size, cy + grid_size), dot_r, fg_color);
  draw_list->AddCircleFilled(ImVec2(cx + grid_size, cy + grid_size), dot_r, fg_color);
}

} // namespace

ToolbarResult DrawToolbar(ToolsPanelState& state, bool can_undo, bool can_redo, bool snap_enabled)
{
  ToolbarResult result{};

  ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float toolbar_height = GetToolbarHeight();
  const float menu_bar_height = ImGui::GetFrameHeight();

  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menu_bar_height));
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbar_height));

  ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoNav |
    ImGuiWindowFlags_NoFocusOnAppearing;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));

  if (ImGui::Begin("##Toolbar", nullptr, flags))
  {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float button_size = toolbar_height - 6.0f;

    // Transform tools group
    for (EditorTool tool : {EditorTool::Select, EditorTool::Move, EditorTool::Rotate, EditorTool::Scale})
    {
      const bool is_selected = (tool == state.selected_tool);
      char id[32];
      snprintf(id, sizeof(id), "##tb_%d", static_cast<int>(tool));

      const ImVec2 pos = ImGui::GetCursorScreenPos();
      const bool clicked = ImGui::InvisibleButton(id, ImVec2(button_size, button_size));
      const bool hovered = ImGui::IsItemHovered();

      DrawToolbarIcon(draw_list, pos, ImVec2(pos.x + button_size, pos.y + button_size), tool, is_selected, hovered);

      if (clicked)
      {
        state.selected_tool = tool;
        result.tool_changed = true;
        result.new_tool = tool;
      }

      if (hovered)
      {
        ImGui::BeginTooltip();
        ImGui::Text("%s (%s)", ToolName(tool), ToolShortcut(tool));
        ImGui::EndTooltip();
      }

      ImGui::SameLine();
    }

    // Separator
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0.0f, 8.0f);

    // Primitive tools group
    for (EditorTool tool : {EditorTool::Box, EditorTool::Cylinder, EditorTool::Pyramid,
                            EditorTool::Sphere, EditorTool::Dome, EditorTool::Plane})
    {
      char id[32];
      snprintf(id, sizeof(id), "##tb_%d", static_cast<int>(tool));

      const ImVec2 pos = ImGui::GetCursorScreenPos();
      const bool clicked = ImGui::InvisibleButton(id, ImVec2(button_size, button_size));
      const bool hovered = ImGui::IsItemHovered();

      DrawToolbarIcon(draw_list, pos, ImVec2(pos.x + button_size, pos.y + button_size), tool, false, hovered);

      if (clicked)
      {
        result.create_primitive = ToolToPrimitiveType(tool);
      }

      if (hovered)
      {
        ImGui::BeginTooltip();
        ImGui::Text("%s", ToolName(tool));
        ImGui::EndTooltip();
      }

      ImGui::SameLine();
    }

    // Separator
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0.0f, 8.0f);

    // Undo button
    {
      const ImVec2 pos = ImGui::GetCursorScreenPos();
      const bool clicked = ImGui::InvisibleButton("##tb_undo", ImVec2(button_size, button_size));
      const bool hovered = ImGui::IsItemHovered();

      DrawUndoRedoIcon(draw_list, pos, ImVec2(pos.x + button_size, pos.y + button_size), false, can_undo, hovered);

      if (clicked && can_undo)
      {
        result.undo_requested = true;
      }

      if (hovered)
      {
        ImGui::BeginTooltip();
        ImGui::Text("Undo (Cmd+Z)");
        ImGui::EndTooltip();
      }

      ImGui::SameLine();
    }

    // Redo button
    {
      const ImVec2 pos = ImGui::GetCursorScreenPos();
      const bool clicked = ImGui::InvisibleButton("##tb_redo", ImVec2(button_size, button_size));
      const bool hovered = ImGui::IsItemHovered();

      DrawUndoRedoIcon(draw_list, pos, ImVec2(pos.x + button_size, pos.y + button_size), true, can_redo, hovered);

      if (clicked && can_redo)
      {
        result.redo_requested = true;
      }

      if (hovered)
      {
        ImGui::BeginTooltip();
        ImGui::Text("Redo (Cmd+Shift+Z)");
        ImGui::EndTooltip();
      }

      ImGui::SameLine();
    }

    // Separator
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0.0f, 8.0f);

    // Snap toggle button
    {
      const ImVec2 pos = ImGui::GetCursorScreenPos();
      const bool clicked = ImGui::InvisibleButton("##tb_snap", ImVec2(button_size, button_size));
      const bool hovered = ImGui::IsItemHovered();

      DrawSnapIcon(draw_list, pos, ImVec2(pos.x + button_size, pos.y + button_size), snap_enabled, hovered);

      if (clicked)
      {
        result.snap_toggled = true;
      }

      if (hovered)
      {
        ImGui::BeginTooltip();
        ImGui::Text("Toggle Snap (%s)", snap_enabled ? "ON" : "OFF");
        ImGui::Text("Hold Ctrl while dragging to temporarily %s snap", snap_enabled ? "disable" : "enable");
        ImGui::EndTooltip();
      }
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(3);

  return result;
}
