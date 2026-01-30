#include "ui_tools_dock.h"

#include "imgui.h"

#include <cstdio>

const char* ToolName(EditorTool tool)
{
  switch (tool)
  {
  case EditorTool::Select:
    return "Select";
  case EditorTool::Move:
    return "Move";
  case EditorTool::Rotate:
    return "Rotate";
  case EditorTool::Scale:
    return "Scale";
  case EditorTool::Box:
    return "Box";
  case EditorTool::Cylinder:
    return "Cylinder";
  case EditorTool::Pyramid:
    return "Pyramid";
  case EditorTool::Sphere:
    return "Sphere";
  case EditorTool::Dome:
    return "Dome";
  case EditorTool::Plane:
    return "Plane";
  default:
    return "Unknown";
  }
}

const char* ToolShortcut(EditorTool tool)
{
  switch (tool)
  {
  case EditorTool::Select:
    return "Q";
  case EditorTool::Move:
    return "W";
  case EditorTool::Rotate:
    return "E";
  case EditorTool::Scale:
    return "R";
  case EditorTool::Box:
    return "Shift+A, B";
  case EditorTool::Cylinder:
    return "Shift+A, C";
  case EditorTool::Pyramid:
    return "Shift+A, Y";
  case EditorTool::Sphere:
    return "Shift+A, S";
  case EditorTool::Dome:
    return "Shift+A, D";
  case EditorTool::Plane:
    return "Shift+A, P";
  default:
    return "";
  }
}

PrimitiveType ToolToPrimitiveType(EditorTool tool)
{
  switch (tool)
  {
  case EditorTool::Box:
    return PrimitiveType::Box;
  case EditorTool::Cylinder:
    return PrimitiveType::Cylinder;
  case EditorTool::Pyramid:
    return PrimitiveType::Pyramid;
  case EditorTool::Sphere:
    return PrimitiveType::Sphere;
  case EditorTool::Dome:
    return PrimitiveType::Dome;
  case EditorTool::Plane:
    return PrimitiveType::Plane;
  default:
    return PrimitiveType::None;
  }
}

namespace
{

/// CSG operation types for icon drawing.
enum class CSGOperation
{
  Hollow,
  Carve,
  Split,
  Join,
  Triangulate
};

/// Returns the display name for a CSG operation.
const char* CSGOperationName(CSGOperation op)
{
  switch (op)
  {
  case CSGOperation::Hollow:
    return "Hollow";
  case CSGOperation::Carve:
    return "Carve";
  case CSGOperation::Split:
    return "Split";
  case CSGOperation::Join:
    return "Join";
  case CSGOperation::Triangulate:
    return "Triangulate";
  default:
    return "Unknown";
  }
}

/// Returns the tooltip description for a CSG operation.
const char* CSGOperationTooltip(CSGOperation op)
{
  switch (op)
  {
  case CSGOperation::Hollow:
    return "Create hollow brush with wall thickness";
  case CSGOperation::Carve:
    return "Subtract cutter brush from targets";
  case CSGOperation::Split:
    return "Split brush along a plane";
  case CSGOperation::Join:
    return "Merge brushes into convex hull";
  case CSGOperation::Triangulate:
    return "Convert brush faces to triangles";
  default:
    return "";
  }
}

/// Draw an icon for a CSG operation.
void DrawCSGIcon(ImDrawList* draw_list, ImVec2 min, ImVec2 max, CSGOperation op, bool hovered)
{
  const ImU32 bg_color = hovered ? IM_COL32(60, 60, 70, 255) : IM_COL32(50, 50, 55, 255);
  const ImU32 fg_color = IM_COL32(180, 180, 190, 255);
  const ImU32 accent_color = IM_COL32(100, 180, 255, 255);
  const ImU32 outline_color = IM_COL32(70, 70, 80, 255);

  const float cx = (min.x + max.x) * 0.5f;
  const float cy = (min.y + max.y) * 0.5f;
  const float w = max.x - min.x;
  const float h = max.y - min.y;
  const float s = std::min(w, h) * 0.35f;

  // Background
  draw_list->AddRectFilled(min, max, bg_color, 4.0f);
  draw_list->AddRect(min, max, outline_color, 4.0f);

  switch (op)
  {
  case CSGOperation::Hollow:
    // Nested rectangles (solid outer, empty inner)
    {
      const float outer = s * 0.8f;
      const float inner = s * 0.4f;
      // Outer box (filled)
      draw_list->AddRectFilled(
        ImVec2(cx - outer, cy - outer),
        ImVec2(cx + outer, cy + outer),
        fg_color, 2.0f);
      // Inner box (cut out - use background color)
      draw_list->AddRectFilled(
        ImVec2(cx - inner, cy - inner),
        ImVec2(cx + inner, cy + inner),
        bg_color, 1.0f);
      draw_list->AddRect(
        ImVec2(cx - inner, cy - inner),
        ImVec2(cx + inner, cy + inner),
        accent_color, 1.0f, 0, 1.5f);
    }
    break;

  case CSGOperation::Carve:
    // Overlapping shapes with subtraction
    {
      const float box_s = s * 0.6f;
      // Target box
      draw_list->AddRectFilled(
        ImVec2(cx - box_s - s * 0.2f, cy - box_s),
        ImVec2(cx + box_s - s * 0.2f, cy + box_s),
        fg_color, 2.0f);
      // Cutter (overlapping, different color)
      draw_list->AddCircleFilled(
        ImVec2(cx + s * 0.3f, cy),
        s * 0.5f,
        accent_color, 16);
      // Minus sign
      draw_list->AddLine(
        ImVec2(cx + s * 0.1f, cy),
        ImVec2(cx + s * 0.5f, cy),
        IM_COL32(255, 255, 255, 255), 2.0f);
    }
    break;

  case CSGOperation::Split:
    // Box with dividing line
    {
      const float box_s = s * 0.7f;
      // Left half
      draw_list->AddRectFilled(
        ImVec2(cx - box_s, cy - box_s),
        ImVec2(cx - s * 0.05f, cy + box_s),
        fg_color, 2.0f);
      // Right half (slightly offset/different shade)
      draw_list->AddRectFilled(
        ImVec2(cx + s * 0.05f, cy - box_s),
        ImVec2(cx + box_s, cy + box_s),
        IM_COL32(140, 140, 150, 255), 2.0f);
      // Dividing line
      draw_list->AddLine(
        ImVec2(cx, cy - box_s - s * 0.2f),
        ImVec2(cx, cy + box_s + s * 0.2f),
        accent_color, 2.0f);
    }
    break;

  case CSGOperation::Join:
    // Multiple shapes merging into one
    {
      const float box_s = s * 0.4f;
      // Two boxes merging
      draw_list->AddRectFilled(
        ImVec2(cx - box_s - s * 0.3f, cy - box_s),
        ImVec2(cx + box_s - s * 0.3f, cy + box_s),
        fg_color, 2.0f);
      draw_list->AddRectFilled(
        ImVec2(cx - box_s + s * 0.3f, cy - box_s),
        ImVec2(cx + box_s + s * 0.3f, cy + box_s),
        fg_color, 2.0f);
      // Plus sign in center
      draw_list->AddLine(
        ImVec2(cx - s * 0.2f, cy),
        ImVec2(cx + s * 0.2f, cy),
        accent_color, 2.0f);
      draw_list->AddLine(
        ImVec2(cx, cy - s * 0.2f),
        ImVec2(cx, cy + s * 0.2f),
        accent_color, 2.0f);
    }
    break;

  case CSGOperation::Triangulate:
    // Shape with triangle subdivisions
    {
      const float box_s = s * 0.7f;
      // Outer shape
      draw_list->AddRect(
        ImVec2(cx - box_s, cy - box_s),
        ImVec2(cx + box_s, cy + box_s),
        fg_color, 0.0f, 0, 1.5f);
      // Triangle subdivisions
      draw_list->AddLine(
        ImVec2(cx - box_s, cy - box_s),
        ImVec2(cx + box_s, cy + box_s),
        accent_color, 1.5f);
      draw_list->AddLine(
        ImVec2(cx + box_s, cy - box_s),
        ImVec2(cx - box_s, cy + box_s),
        accent_color, 1.5f);
      draw_list->AddLine(
        ImVec2(cx, cy - box_s),
        ImVec2(cx, cy + box_s),
        fg_color, 1.0f);
      draw_list->AddLine(
        ImVec2(cx - box_s, cy),
        ImVec2(cx + box_s, cy),
        fg_color, 1.0f);
    }
    break;
  }
}

/// Draw a CSG operation button and return true if clicked.
bool DrawCSGButton(CSGOperation op, float size)
{
  const char* name = CSGOperationName(op);
  const char* tooltip = CSGOperationTooltip(op);

  // Create unique ID
  char id[64];
  snprintf(id, sizeof(id), "##csg_%d", static_cast<int>(op));

  // Get current position
  const ImVec2 pos = ImGui::GetCursorScreenPos();
  const ImVec2 button_size(size, size);

  // Invisible button for interaction
  const bool clicked = ImGui::InvisibleButton(id, button_size);
  const bool hovered = ImGui::IsItemHovered();

  // Draw the icon
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const ImVec2 min_pos = pos;
  const ImVec2 max_pos(pos.x + size, pos.y + size);

  DrawCSGIcon(draw_list, min_pos, max_pos, op, hovered);

  // Tooltip on hover
  if (hovered)
  {
    ImGui::BeginTooltip();
    ImGui::Text("%s", name);
    ImGui::TextDisabled("%s", tooltip);
    ImGui::EndTooltip();
  }

  return clicked;
}

/// Geometry operation types for icon drawing (EPIC-09).
enum class GeometryOperation
{
  Flip,    // Flip normals
  Weld,    // Weld vertices
  Extrude  // Extrude faces
};

/// Returns the display name for a geometry operation.
const char* GeometryOperationName(GeometryOperation op)
{
  switch (op)
  {
  case GeometryOperation::Flip:
    return "Flip";
  case GeometryOperation::Weld:
    return "Weld";
  case GeometryOperation::Extrude:
    return "Extrude";
  default:
    return "Unknown";
  }
}

/// Returns the tooltip description for a geometry operation.
const char* GeometryOperationTooltip(GeometryOperation op)
{
  switch (op)
  {
  case GeometryOperation::Flip:
    return "Flip face normals (N)";
  case GeometryOperation::Weld:
    return "Weld nearby vertices";
  case GeometryOperation::Extrude:
    return "Extrude faces outward";
  default:
    return "";
  }
}

/// Draw an icon for a geometry operation.
void DrawGeometryIcon(ImDrawList* draw_list, ImVec2 min, ImVec2 max, GeometryOperation op, bool hovered)
{
  const ImU32 bg_color = hovered ? IM_COL32(60, 60, 70, 255) : IM_COL32(50, 50, 55, 255);
  const ImU32 fg_color = IM_COL32(180, 180, 190, 255);
  const ImU32 accent_color = IM_COL32(100, 200, 150, 255); // Green accent for geometry ops
  const ImU32 outline_color = IM_COL32(70, 70, 80, 255);

  const float cx = (min.x + max.x) * 0.5f;
  const float cy = (min.y + max.y) * 0.5f;
  const float w = max.x - min.x;
  const float h = max.y - min.y;
  const float s = std::min(w, h) * 0.35f;

  // Background
  draw_list->AddRectFilled(min, max, bg_color, 4.0f);
  draw_list->AddRect(min, max, outline_color, 4.0f);

  switch (op)
  {
  case GeometryOperation::Flip:
    // Two arrows pointing opposite directions
    {
      const float arrow_len = s * 0.6f;
      const float arrow_head = s * 0.25f;
      const float offset = s * 0.25f;

      // Up arrow (left side)
      draw_list->AddLine(
        ImVec2(cx - offset, cy + arrow_len * 0.5f),
        ImVec2(cx - offset, cy - arrow_len * 0.5f),
        fg_color, 2.0f);
      draw_list->AddTriangleFilled(
        ImVec2(cx - offset, cy - arrow_len * 0.5f - arrow_head),
        ImVec2(cx - offset - arrow_head * 0.5f, cy - arrow_len * 0.5f),
        ImVec2(cx - offset + arrow_head * 0.5f, cy - arrow_len * 0.5f),
        fg_color);

      // Down arrow (right side)
      draw_list->AddLine(
        ImVec2(cx + offset, cy - arrow_len * 0.5f),
        ImVec2(cx + offset, cy + arrow_len * 0.5f),
        accent_color, 2.0f);
      draw_list->AddTriangleFilled(
        ImVec2(cx + offset, cy + arrow_len * 0.5f + arrow_head),
        ImVec2(cx + offset - arrow_head * 0.5f, cy + arrow_len * 0.5f),
        ImVec2(cx + offset + arrow_head * 0.5f, cy + arrow_len * 0.5f),
        accent_color);
    }
    break;

  case GeometryOperation::Weld:
    // Two dots merging to one
    {
      const float dot_r = s * 0.15f;
      const float dist = s * 0.5f;

      // Left dot
      draw_list->AddCircleFilled(ImVec2(cx - dist, cy), dot_r, fg_color, 12);
      // Right dot
      draw_list->AddCircleFilled(ImVec2(cx + dist, cy), dot_r, fg_color, 12);
      // Lines connecting to center
      draw_list->AddLine(
        ImVec2(cx - dist + dot_r, cy),
        ImVec2(cx - dot_r * 0.5f, cy),
        accent_color, 1.5f);
      draw_list->AddLine(
        ImVec2(cx + dist - dot_r, cy),
        ImVec2(cx + dot_r * 0.5f, cy),
        accent_color, 1.5f);
      // Center dot (result)
      draw_list->AddCircleFilled(ImVec2(cx, cy), dot_r * 1.2f, accent_color, 12);
    }
    break;

  case GeometryOperation::Extrude:
    // Face with arrow pointing outward
    {
      const float box_s = s * 0.5f;
      const float arrow_len = s * 0.5f;
      const float arrow_head = s * 0.2f;

      // Base face (square)
      draw_list->AddRectFilled(
        ImVec2(cx - box_s, cy - box_s + s * 0.2f),
        ImVec2(cx + box_s, cy + box_s + s * 0.2f),
        fg_color, 2.0f);

      // Arrow pointing up (extrusion direction)
      draw_list->AddLine(
        ImVec2(cx, cy + s * 0.2f),
        ImVec2(cx, cy - arrow_len),
        accent_color, 2.0f);
      draw_list->AddTriangleFilled(
        ImVec2(cx, cy - arrow_len - arrow_head),
        ImVec2(cx - arrow_head * 0.6f, cy - arrow_len),
        ImVec2(cx + arrow_head * 0.6f, cy - arrow_len),
        accent_color);
    }
    break;
  }
}

/// Draw a geometry operation button and return true if clicked.
bool DrawGeometryButton(GeometryOperation op, float size)
{
  const char* name = GeometryOperationName(op);
  const char* tooltip = GeometryOperationTooltip(op);

  // Create unique ID
  char id[64];
  snprintf(id, sizeof(id), "##geo_%d", static_cast<int>(op));

  // Get current position
  const ImVec2 pos = ImGui::GetCursorScreenPos();
  const ImVec2 button_size(size, size);

  // Invisible button for interaction
  const bool clicked = ImGui::InvisibleButton(id, button_size);
  const bool hovered = ImGui::IsItemHovered();

  // Draw the icon
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const ImVec2 min_pos = pos;
  const ImVec2 max_pos(pos.x + size, pos.y + size);

  DrawGeometryIcon(draw_list, min_pos, max_pos, op, hovered);

  // Tooltip on hover
  if (hovered)
  {
    ImGui::BeginTooltip();
    ImGui::Text("%s", name);
    ImGui::TextDisabled("%s", tooltip);
    ImGui::EndTooltip();
  }

  return clicked;
}

/// Draw a simple icon representing a tool using ImGui draw primitives.
void DrawToolIcon(ImDrawList* draw_list, ImVec2 min, ImVec2 max, EditorTool tool, bool selected)
{
  const ImU32 bg_color = selected ? IM_COL32(70, 110, 180, 255) : IM_COL32(50, 50, 55, 255);
  const ImU32 fg_color = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 190, 255);
  const ImU32 outline_color = selected ? IM_COL32(100, 150, 220, 255) : IM_COL32(70, 70, 80, 255);

  const float cx = (min.x + max.x) * 0.5f;
  const float cy = (min.y + max.y) * 0.5f;
  const float w = max.x - min.x;
  const float h = max.y - min.y;
  const float s = std::min(w, h) * 0.35f; // Icon scale

  // Background
  draw_list->AddRectFilled(min, max, bg_color, 4.0f);
  draw_list->AddRect(min, max, outline_color, 4.0f);

  switch (tool)
  {
  case EditorTool::Select:
    // Arrow cursor
    {
      const ImVec2 p1(cx - s * 0.4f, cy - s * 0.8f);
      const ImVec2 p2(cx - s * 0.4f, cy + s * 0.4f);
      const ImVec2 p3(cx - s * 0.1f, cy + s * 0.1f);
      const ImVec2 p4(cx + s * 0.3f, cy + s * 0.7f);
      const ImVec2 p5(cx + s * 0.1f, cy + s * 0.3f);
      const ImVec2 p6(cx + s * 0.5f, cy + s * 0.3f);
      draw_list->AddTriangleFilled(p1, p2, p6, fg_color);
      draw_list->AddLine(p3, p4, fg_color, 2.0f);
    }
    break;

  case EditorTool::Move:
    // Four arrows
    {
      const float arrow_len = s * 0.7f;
      const float arrow_head = s * 0.25f;
      // Up
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx, cy - arrow_len), fg_color, 2.0f);
      draw_list->AddTriangleFilled(
        ImVec2(cx, cy - arrow_len - arrow_head),
        ImVec2(cx - arrow_head * 0.5f, cy - arrow_len),
        ImVec2(cx + arrow_head * 0.5f, cy - arrow_len),
        fg_color);
      // Down
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx, cy + arrow_len), fg_color, 2.0f);
      draw_list->AddTriangleFilled(
        ImVec2(cx, cy + arrow_len + arrow_head),
        ImVec2(cx - arrow_head * 0.5f, cy + arrow_len),
        ImVec2(cx + arrow_head * 0.5f, cy + arrow_len),
        fg_color);
      // Left
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx - arrow_len, cy), fg_color, 2.0f);
      draw_list->AddTriangleFilled(
        ImVec2(cx - arrow_len - arrow_head, cy),
        ImVec2(cx - arrow_len, cy - arrow_head * 0.5f),
        ImVec2(cx - arrow_len, cy + arrow_head * 0.5f),
        fg_color);
      // Right
      draw_list->AddLine(ImVec2(cx, cy), ImVec2(cx + arrow_len, cy), fg_color, 2.0f);
      draw_list->AddTriangleFilled(
        ImVec2(cx + arrow_len + arrow_head, cy),
        ImVec2(cx + arrow_len, cy - arrow_head * 0.5f),
        ImVec2(cx + arrow_len, cy + arrow_head * 0.5f),
        fg_color);
    }
    break;

  case EditorTool::Rotate:
    // Circular arrow
    {
      const float r = s * 0.6f;
      draw_list->AddCircle(ImVec2(cx, cy), r, fg_color, 24, 2.0f);
      // Arrow head at top
      draw_list->AddTriangleFilled(
        ImVec2(cx + r * 0.3f, cy - r - s * 0.15f),
        ImVec2(cx - s * 0.15f, cy - r),
        ImVec2(cx + s * 0.15f, cy - r + s * 0.15f),
        fg_color);
    }
    break;

  case EditorTool::Scale:
    // Corner scale handles
    {
      const float box_s = s * 0.5f;
      const float handle_s = s * 0.2f;
      draw_list->AddRect(
        ImVec2(cx - box_s, cy - box_s),
        ImVec2(cx + box_s, cy + box_s),
        fg_color, 0.0f, 0, 1.5f);
      // Corner handles
      draw_list->AddRectFilled(
        ImVec2(cx - box_s - handle_s * 0.5f, cy - box_s - handle_s * 0.5f),
        ImVec2(cx - box_s + handle_s * 0.5f, cy - box_s + handle_s * 0.5f),
        fg_color);
      draw_list->AddRectFilled(
        ImVec2(cx + box_s - handle_s * 0.5f, cy - box_s - handle_s * 0.5f),
        ImVec2(cx + box_s + handle_s * 0.5f, cy - box_s + handle_s * 0.5f),
        fg_color);
      draw_list->AddRectFilled(
        ImVec2(cx - box_s - handle_s * 0.5f, cy + box_s - handle_s * 0.5f),
        ImVec2(cx - box_s + handle_s * 0.5f, cy + box_s + handle_s * 0.5f),
        fg_color);
      draw_list->AddRectFilled(
        ImVec2(cx + box_s - handle_s * 0.5f, cy + box_s - handle_s * 0.5f),
        ImVec2(cx + box_s + handle_s * 0.5f, cy + box_s + handle_s * 0.5f),
        fg_color);
    }
    break;

  case EditorTool::Box:
    // 3D box (isometric-ish)
    {
      const float bs = s * 0.6f;
      const float d = bs * 0.4f; // depth offset
      // Front face
      draw_list->AddRectFilled(
        ImVec2(cx - bs * 0.5f, cy - bs * 0.3f),
        ImVec2(cx + bs * 0.5f, cy + bs * 0.7f),
        fg_color);
      // Top face (parallelogram)
      const ImVec2 top_pts[4] = {
        ImVec2(cx - bs * 0.5f, cy - bs * 0.3f),
        ImVec2(cx - bs * 0.5f + d, cy - bs * 0.3f - d),
        ImVec2(cx + bs * 0.5f + d, cy - bs * 0.3f - d),
        ImVec2(cx + bs * 0.5f, cy - bs * 0.3f)};
      draw_list->AddConvexPolyFilled(top_pts, 4, IM_COL32(
        (fg_color & 0xFF) * 200 / 255,
        ((fg_color >> 8) & 0xFF) * 200 / 255,
        ((fg_color >> 16) & 0xFF) * 200 / 255,
        255));
      // Right face (parallelogram)
      const ImVec2 right_pts[4] = {
        ImVec2(cx + bs * 0.5f, cy - bs * 0.3f),
        ImVec2(cx + bs * 0.5f + d, cy - bs * 0.3f - d),
        ImVec2(cx + bs * 0.5f + d, cy + bs * 0.7f - d),
        ImVec2(cx + bs * 0.5f, cy + bs * 0.7f)};
      draw_list->AddConvexPolyFilled(right_pts, 4, IM_COL32(
        (fg_color & 0xFF) * 150 / 255,
        ((fg_color >> 8) & 0xFF) * 150 / 255,
        ((fg_color >> 16) & 0xFF) * 150 / 255,
        255));
    }
    break;

  case EditorTool::Cylinder:
    // Simple cylinder
    {
      const float cyl_w = s * 0.6f;
      const float cyl_h = s * 0.8f;
      const float ellipse_h = s * 0.2f;
      // Body
      draw_list->AddRectFilled(
        ImVec2(cx - cyl_w * 0.5f, cy - cyl_h * 0.5f + ellipse_h * 0.5f),
        ImVec2(cx + cyl_w * 0.5f, cy + cyl_h * 0.5f - ellipse_h * 0.5f),
        fg_color);
      // Top ellipse
      draw_list->AddEllipseFilled(
        ImVec2(cx, cy - cyl_h * 0.5f + ellipse_h * 0.5f),
        ImVec2(cyl_w * 0.5f, ellipse_h * 0.5f),
        IM_COL32(
          (fg_color & 0xFF) * 200 / 255,
          ((fg_color >> 8) & 0xFF) * 200 / 255,
          ((fg_color >> 16) & 0xFF) * 200 / 255,
          255));
      // Bottom ellipse (lower half only visible)
      draw_list->AddEllipse(
        ImVec2(cx, cy + cyl_h * 0.5f - ellipse_h * 0.5f),
        ImVec2(cyl_w * 0.5f, ellipse_h * 0.5f),
        fg_color);
    }
    break;

  case EditorTool::Pyramid:
    // Triangle with base
    {
      const float pyr_w = s * 0.8f;
      const float pyr_h = s * 0.9f;
      draw_list->AddTriangleFilled(
        ImVec2(cx, cy - pyr_h * 0.5f),
        ImVec2(cx - pyr_w * 0.5f, cy + pyr_h * 0.5f),
        ImVec2(cx + pyr_w * 0.5f, cy + pyr_h * 0.5f),
        fg_color);
      draw_list->AddLine(
        ImVec2(cx, cy - pyr_h * 0.5f),
        ImVec2(cx + pyr_w * 0.3f, cy + pyr_h * 0.3f),
        IM_COL32(80, 80, 90, 255), 1.5f);
    }
    break;

  case EditorTool::Sphere:
    // Circle with highlight
    {
      const float sph_r = s * 0.6f;
      draw_list->AddCircleFilled(ImVec2(cx, cy), sph_r, fg_color, 24);
      // Highlight
      draw_list->AddCircleFilled(
        ImVec2(cx - sph_r * 0.3f, cy - sph_r * 0.3f),
        sph_r * 0.25f,
        IM_COL32(255, 255, 255, 100), 12);
    }
    break;

  case EditorTool::Dome:
    // Half sphere
    {
      const float dome_r = s * 0.6f;
      // Draw arc for the dome
      draw_list->PathArcTo(ImVec2(cx, cy + dome_r * 0.3f), dome_r, 3.14159f, 0.0f, 12);
      draw_list->PathFillConvex(fg_color);
      // Base line
      draw_list->AddLine(
        ImVec2(cx - dome_r, cy + dome_r * 0.3f),
        ImVec2(cx + dome_r, cy + dome_r * 0.3f),
        fg_color, 2.0f);
    }
    break;

  case EditorTool::Plane:
    // Flat parallelogram
    {
      const float plane_w = s * 0.9f;
      const float plane_h = s * 0.6f;
      const float skew = s * 0.3f;
      const ImVec2 pts[4] = {
        ImVec2(cx - plane_w * 0.5f + skew, cy - plane_h * 0.5f),
        ImVec2(cx + plane_w * 0.5f + skew, cy - plane_h * 0.5f),
        ImVec2(cx + plane_w * 0.5f - skew, cy + plane_h * 0.5f),
        ImVec2(cx - plane_w * 0.5f - skew, cy + plane_h * 0.5f)};
      draw_list->AddConvexPolyFilled(pts, 4, fg_color);
      draw_list->AddPolyline(pts, 4, outline_color, ImDrawFlags_Closed, 1.0f);
      // Grid lines
      draw_list->AddLine(
        ImVec2(cx + skew * 0.5f, cy - plane_h * 0.5f),
        ImVec2(cx - skew * 0.5f, cy + plane_h * 0.5f),
        outline_color, 1.0f);
      draw_list->AddLine(
        ImVec2(cx - plane_w * 0.25f, cy),
        ImVec2(cx + plane_w * 0.25f, cy),
        outline_color, 1.0f);
    }
    break;

  default:
    // Unknown tool - just show a question mark
    {
      ImGui::PushFont(ImGui::GetFont());
      const char* text = "?";
      const ImVec2 text_size = ImGui::CalcTextSize(text);
      draw_list->AddText(
        ImVec2(cx - text_size.x * 0.5f, cy - text_size.y * 0.5f),
        fg_color, text);
      ImGui::PopFont();
    }
    break;
  }
}

/// Draw a tool button and return true if clicked.
bool DrawToolButton(EditorTool tool, EditorTool selected, float size)
{
  const bool is_selected = (tool == selected);
  const char* name = ToolName(tool);
  const char* shortcut = ToolShortcut(tool);

  // Create unique ID
  char id[64];
  snprintf(id, sizeof(id), "##tool_%d", static_cast<int>(tool));

  // Get current position
  const ImVec2 pos = ImGui::GetCursorScreenPos();
  const ImVec2 button_size(size, size);

  // Invisible button for interaction
  const bool clicked = ImGui::InvisibleButton(id, button_size);
  const bool hovered = ImGui::IsItemHovered();

  // Draw the icon
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const ImVec2 min = pos;
  const ImVec2 max(pos.x + size, pos.y + size);

  // Hover highlight
  if (hovered && !is_selected)
  {
    draw_list->AddRectFilled(min, max, IM_COL32(60, 60, 70, 255), 4.0f);
  }

  DrawToolIcon(draw_list, min, max, tool, is_selected);

  // Tooltip on hover
  if (hovered)
  {
    ImGui::BeginTooltip();
    ImGui::Text("%s", name);
    if (shortcut[0] != '\0')
    {
      ImGui::SameLine();
      ImGui::TextDisabled("(%s)", shortcut);
    }
    ImGui::EndTooltip();
  }

  return clicked;
}

} // namespace

ToolsPanelResult DrawToolsPanel(ToolsPanelState& state)
{
  ToolsPanelResult result{};

  if (!state.visible)
  {
    return result;
  }

  ImGui::SetNextWindowSize(ImVec2(72, 400), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Tools", &state.visible))
  {
    ImGui::End();
    return result;
  }

  const float button_size = 32.0f;
  const float spacing = 4.0f;
  const float avail_width = ImGui::GetContentRegionAvail().x;
  const int buttons_per_row = std::max(1, static_cast<int>((avail_width + spacing) / (button_size + spacing)));

  // Transform tools section
  ImGui::TextDisabled("Transform");
  ImGui::Separator();

  int col = 0;
  for (EditorTool tool : {EditorTool::Select, EditorTool::Move, EditorTool::Rotate, EditorTool::Scale})
  {
    if (col > 0)
    {
      ImGui::SameLine(0.0f, spacing);
    }

    if (DrawToolButton(tool, state.selected_tool, button_size))
    {
      state.selected_tool = tool;
      result.tool_changed = true;
      result.new_tool = tool;
    }

    col++;
    if (col >= buttons_per_row)
    {
      col = 0;
    }
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // Primitives section
  ImGui::TextDisabled("Primitives");
  ImGui::Separator();

  col = 0;
  for (EditorTool tool : {EditorTool::Box, EditorTool::Cylinder, EditorTool::Pyramid,
                          EditorTool::Sphere, EditorTool::Dome, EditorTool::Plane})
  {
    if (col > 0)
    {
      ImGui::SameLine(0.0f, spacing);
    }

    if (DrawToolButton(tool, state.selected_tool, button_size))
    {
      // Primitive tools trigger the creation dialog immediately
      result.create_primitive = ToolToPrimitiveType(tool);
    }

    col++;
    if (col >= buttons_per_row)
    {
      col = 0;
    }
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // Operations section (CSG tools)
  ImGui::TextDisabled("Operations");
  ImGui::Separator();

  col = 0;
  for (CSGOperation op : {CSGOperation::Hollow, CSGOperation::Carve, CSGOperation::Split,
                          CSGOperation::Join, CSGOperation::Triangulate})
  {
    if (col > 0)
    {
      ImGui::SameLine(0.0f, spacing);
    }

    if (DrawCSGButton(op, button_size))
    {
      switch (op)
      {
      case CSGOperation::Hollow:
        result.csg_hollow = true;
        break;
      case CSGOperation::Carve:
        result.csg_carve = true;
        break;
      case CSGOperation::Split:
        result.csg_split = true;
        break;
      case CSGOperation::Join:
        result.csg_join = true;
        break;
      case CSGOperation::Triangulate:
        result.csg_triangulate = true;
        break;
      }
    }

    col++;
    if (col >= buttons_per_row)
    {
      col = 0;
    }
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // Geometry section (EPIC-09)
  ImGui::TextDisabled("Geometry");
  ImGui::Separator();

  col = 0;
  for (GeometryOperation op : {GeometryOperation::Flip, GeometryOperation::Weld, GeometryOperation::Extrude})
  {
    if (col > 0)
    {
      ImGui::SameLine(0.0f, spacing);
    }

    if (DrawGeometryButton(op, button_size))
    {
      switch (op)
      {
      case GeometryOperation::Flip:
        result.geometry_flip = true;
        break;
      case GeometryOperation::Weld:
        result.geometry_weld = true;
        break;
      case GeometryOperation::Extrude:
        result.geometry_extrude = true;
        break;
      }
    }

    col++;
    if (col >= buttons_per_row)
    {
      col = 0;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::TextDisabled("Shift+A: Add primitive");

  ImGui::End();
  return result;
}

bool DrawPrimitivePopup(PrimitiveType& result)
{
  result = PrimitiveType::None;

  if (!ImGui::BeginPopup("AddPrimitive"))
  {
    return false;
  }

  ImGui::TextDisabled("Add Primitive");
  ImGui::Separator();

  if (ImGui::MenuItem("Box", "B"))
  {
    result = PrimitiveType::Box;
  }
  if (ImGui::MenuItem("Cylinder", "C"))
  {
    result = PrimitiveType::Cylinder;
  }
  if (ImGui::MenuItem("Pyramid", "Y"))
  {
    result = PrimitiveType::Pyramid;
  }
  if (ImGui::MenuItem("Sphere", "S"))
  {
    result = PrimitiveType::Sphere;
  }
  if (ImGui::MenuItem("Dome", "D"))
  {
    result = PrimitiveType::Dome;
  }
  if (ImGui::MenuItem("Plane", "P"))
  {
    result = PrimitiveType::Plane;
  }

  ImGui::EndPopup();
  return true;
}
