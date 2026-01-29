#include "app/editor_loop.h"

#include "app/editor_paths.h"
#include "app/editor_session.h"
#include "app/project_utils.h"
#include "app/recent_projects.h"
#include "app/scene_loader.h"
#include "app/world.h"
#include "grid/grid_settings.h"
#include "grouping/node_grouping.h"
#include "ui/viewport_panel.h"
#include "viewport/diligent_viewport.h"
#include "viewport/lighting.h"
#include "viewport/overlays.h"
#include "viewport/render_settings.h"
#include "viewport/world_settings.h"
#include "viewport_picking.h"
#include "viewport_render.h"

#include "transform/mirror.h"
#include "transform/nudge.h"
#include "ui_console.h"
#include "ui_dock.h"
#include "ui_goto.h"
#include "ui_project.h"
#include "ui_properties.h"
#include "ui_scene.h"
#include "ui_shared.h"
#include "ui_status_bar.h"
#include "ui_toolbar.h"
#include "ui_viewport.h"
#include "ui_worlds.h"
#include "platform_macos.h"

#include "de_objects.h"
#include "imgui.h"
#include "renderstruct.h"

#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"

#include <SDL3/SDL.h>

#include <filesystem>
#include <cstdio>

namespace {

/// Helper to draw a DragFloat that snaps values when grid snap is enabled.
bool SnapDragFloat(const char* label, float* v, float snap_step, float v_speed = 1.0f, float v_min = 0.0f,
                   float v_max = 0.0f) {
  bool changed = ImGui::DragFloat(label, v, v_speed, v_min, v_max);
  if (changed && snap_step > 0.0f) {
    *v = grid::SnapValue(*v, snap_step);
  }
  return changed;
}

/// Helper to draw a DragFloat3 that snaps values when grid snap is enabled.
bool SnapDragFloat3(const char* label, float v[3], float snap_step, float v_speed = 1.0f) {
  bool changed = ImGui::DragFloat3(label, v, v_speed);
  if (changed && snap_step > 0.0f) {
    v[0] = grid::SnapValue(v[0], snap_step);
    v[1] = grid::SnapValue(v[1], snap_step);
    v[2] = grid::SnapValue(v[2], snap_step);
  }
  return changed;
}

bool IsPrimaryShortcutDown(const ImGuiIO& io)
{
#if defined(__APPLE__)
  return io.KeySuper;
#else
  return io.KeyCtrl;
#endif
}

/// Creates a new brush node from primitive result and adds it to the scene.
int CreateBrushFromPrimitive(
  std::vector<TreeNode>& scene_nodes,
  std::vector<NodeProperties>& scene_props,
  const PrimitiveResult& primitive,
  const char* name)
{
  if (!primitive.success || primitive.vertices.empty())
  {
    return -1;
  }

  // Find or create root node
  if (scene_nodes.empty())
  {
    TreeNode root;
    root.name = "World";
    root.is_folder = true;
    scene_nodes.push_back(root);
    scene_props.push_back(MakeProps("World"));
  }

  const int new_id = static_cast<int>(scene_nodes.size());
  TreeNode node;
  node.name = name;
  node.is_folder = false;
  scene_nodes.push_back(node);

  NodeProperties props = MakeProps("Brush");
  props.brush_vertices = primitive.vertices;
  props.brush_indices = primitive.indices;

  // Compute centroid from vertices (consistent with loaded brushes)
  if (primitive.vertices.size() >= 3)
  {
    const size_t vertex_count = primitive.vertices.size() / 3;
    double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
    for (size_t i = 0; i < primitive.vertices.size(); i += 3)
    {
      sum_x += primitive.vertices[i];
      sum_y += primitive.vertices[i + 1];
      sum_z += primitive.vertices[i + 2];
    }
    props.position[0] = static_cast<float>(sum_x / static_cast<double>(vertex_count));
    props.position[1] = static_cast<float>(sum_y / static_cast<double>(vertex_count));
    props.position[2] = static_cast<float>(sum_z / static_cast<double>(vertex_count));
  }

  scene_props.push_back(props);

  // Add to root's children
  scene_nodes[0].children.push_back(new_id);

  return new_id;
}

/// Sets all primitive dialog centers to the marker position.
void SetPrimitiveCentersToMarker(PrimitiveDialogState& dialog, const ViewportPanelState& viewport)
{
  const float* marker = viewport.marker_position;
  dialog.box_params.center[0] = marker[0];
  dialog.box_params.center[1] = marker[1];
  dialog.box_params.center[2] = marker[2];
  dialog.cylinder_params.center[0] = marker[0];
  dialog.cylinder_params.center[1] = marker[1];
  dialog.cylinder_params.center[2] = marker[2];
  dialog.pyramid_params.center[0] = marker[0];
  dialog.pyramid_params.center[1] = marker[1];
  dialog.pyramid_params.center[2] = marker[2];
  dialog.sphere_params.center[0] = marker[0];
  dialog.sphere_params.center[1] = marker[1];
  dialog.sphere_params.center[2] = marker[2];
  dialog.plane_params.center[0] = marker[0];
  dialog.plane_params.center[1] = marker[1];
  dialog.plane_params.center[2] = marker[2];
}

/// Draws the primitive creation dialog and handles creation.
void DrawPrimitiveDialog(EditorSession& session)
{
  PrimitiveDialogState& dialog = session.primitive_dialog;
  if (!dialog.open)
  {
    return;
  }

  const char* title = "Create Primitive";
  switch (dialog.type)
  {
  case PrimitiveType::Box:
    title = "Create Box";
    break;
  case PrimitiveType::Cylinder:
    title = "Create Cylinder";
    break;
  case PrimitiveType::Pyramid:
    title = "Create Pyramid";
    break;
  case PrimitiveType::Sphere:
    title = "Create Sphere";
    break;
  case PrimitiveType::Dome:
    title = "Create Dome";
    break;
  case PrimitiveType::Plane:
    title = "Create Plane";
    break;
  default:
    dialog.open = false;
    return;
  }

  ImGui::OpenPopup(title);
  if (ImGui::BeginPopupModal(title, &dialog.open, ImGuiWindowFlags_AlwaysAutoResize))
  {
    bool create = false;

    // Get snap step if snapping is enabled
    const ViewportPanelState& viewport = session.viewport_panel();
    const float snap_step = viewport.snap_translate ? viewport.snap_translate_step : 0.0f;

    switch (dialog.type)
    {
    case PrimitiveType::Box:
      SnapDragFloat3("Center", dialog.box_params.center, snap_step, 1.0f);
      SnapDragFloat3("Size", dialog.box_params.size, snap_step, 1.0f);
      // Enforce minimum size to prevent degenerate or inverted geometry
      for (int i = 0; i < 3; ++i) {
        if (dialog.box_params.size[i] < 1.0f) {
          dialog.box_params.size[i] = 1.0f;
        }
      }
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Cylinder:
      SnapDragFloat3("Center", dialog.cylinder_params.center, snap_step, 1.0f);
      SnapDragFloat("Height", &dialog.cylinder_params.height, snap_step, 1.0f, 1.0f, 10000.0f);
      SnapDragFloat("Radius", &dialog.cylinder_params.radius, snap_step, 1.0f, 1.0f, 10000.0f);
      ImGui::SliderInt("Sides", &dialog.cylinder_params.sides, 3, 64);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Pyramid:
      SnapDragFloat3("Center", dialog.pyramid_params.center, snap_step, 1.0f);
      SnapDragFloat("Height", &dialog.pyramid_params.height, snap_step, 1.0f, 1.0f, 10000.0f);
      SnapDragFloat("Base Radius", &dialog.pyramid_params.radius, snap_step, 1.0f, 1.0f, 10000.0f);
      SnapDragFloat("Top Radius", &dialog.pyramid_params.top_radius, snap_step, 1.0f, 0.0f,
                    dialog.pyramid_params.radius);
      if (dialog.pyramid_params.top_radius > 0.0f) {
        ImGui::SameLine();
        ImGui::TextDisabled("(Frustum)");
      }
      ImGui::SliderInt("Sides", &dialog.pyramid_params.sides, 3, 32);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Sphere:
    case PrimitiveType::Dome:
      dialog.sphere_params.dome = (dialog.type == PrimitiveType::Dome);
      SnapDragFloat3("Center", dialog.sphere_params.center, snap_step, 1.0f);
      SnapDragFloat("Radius", &dialog.sphere_params.radius, snap_step, 1.0f, 1.0f, 10000.0f);
      ImGui::SliderInt("H Subdivisions", &dialog.sphere_params.horizontal_subdivisions, 4, 32);
      ImGui::SliderInt("V Subdivisions", &dialog.sphere_params.vertical_subdivisions, 2, 16);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Plane: {
      SnapDragFloat3("Center", dialog.plane_params.center, snap_step, 1.0f);
      SnapDragFloat("Width", &dialog.plane_params.width, snap_step, 1.0f, 1.0f, 10000.0f);
      SnapDragFloat("Height", &dialog.plane_params.height, snap_step, 1.0f, 1.0f, 10000.0f);

      // Orientation preset combo
      static const char* orientation_names[] = {"XZ (Floor)", "XY (Wall Z)", "YZ (Wall X)"};
      int orientation_idx = static_cast<int>(dialog.plane_params.orientation);
      if (ImGui::Combo("Orientation", &orientation_idx, orientation_names, 3)) {
        dialog.plane_params.orientation = static_cast<PlaneOrientation>(orientation_idx);
        // Update normal based on orientation
        switch (dialog.plane_params.orientation) {
        case PlaneOrientation::XZ:
          dialog.plane_params.normal[0] = 0.0f;
          dialog.plane_params.normal[1] = 1.0f;
          dialog.plane_params.normal[2] = 0.0f;
          break;
        case PlaneOrientation::XY:
          dialog.plane_params.normal[0] = 0.0f;
          dialog.plane_params.normal[1] = 0.0f;
          dialog.plane_params.normal[2] = 1.0f;
          break;
        case PlaneOrientation::YZ:
          dialog.plane_params.normal[0] = 1.0f;
          dialog.plane_params.normal[1] = 0.0f;
          dialog.plane_params.normal[2] = 0.0f;
          break;
        }
      }

      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;
    }

    default:
      break;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
      dialog.open = false;
    }

    if (create)
    {
      PrimitiveResult result;
      const char* name = "Brush";

      switch (dialog.type)
      {
      case PrimitiveType::Box:
        result = CreatePrimitiveBox(dialog.box_params);
        name = "Box";
        break;
      case PrimitiveType::Cylinder:
        result = CreatePrimitiveCylinder(dialog.cylinder_params);
        name = "Cylinder";
        break;
      case PrimitiveType::Pyramid:
        result = CreatePrimitivePyramid(dialog.pyramid_params);
        name = "Pyramid";
        break;
      case PrimitiveType::Sphere:
      case PrimitiveType::Dome:
        result = CreatePrimitiveSphere(dialog.sphere_params);
        name = dialog.type == PrimitiveType::Dome ? "Dome" : "Sphere";
        break;
      case PrimitiveType::Plane:
        result = CreatePrimitivePlane(dialog.plane_params);
        name = "Plane";
        break;
      default:
        break;
      }

      if (result.success)
      {
        const int new_id = CreateBrushFromPrimitive(
          session.scene_nodes,
          session.scene_props,
          result,
          name);
        if (new_id >= 0)
        {
          SelectNode(session.scene_panel, new_id);
          session.active_target = SelectionTarget::Scene;
          session.undo_stack.PushCreate(UndoTarget::Scene, new_id);
          session.document_state.MarkDirty();
        }
      }

      dialog.open = false;
    }

    ImGui::EndPopup();
  }
}

/// Draws the polygon extrusion dialog and handles brush creation.
void DrawPolygonExtrusionDialog(EditorSession& session)
{
  PolygonDrawState& state = session.polygon_draw;
  if (!state.show_extrusion_dialog)
  {
    return;
  }

  const char* title = "Extrude Polygon";
  ImGui::OpenPopup(title);
  if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    const size_t num_verts = state.vertices.size() / 3;
    ImGui::Text("Polygon has %zu vertices", num_verts);

    // Check convexity
    std::vector<float> vertices_2d;
    vertices_2d.reserve(num_verts * 2);
    for (size_t i = 0; i < num_verts; ++i)
    {
      // Project 3D to 2D based on plane
      switch (state.plane)
      {
      case PolygonDrawPlane::XZ:
        vertices_2d.push_back(state.vertices[i * 3 + 0]);  // X
        vertices_2d.push_back(state.vertices[i * 3 + 2]);  // Z
        break;
      case PolygonDrawPlane::XY:
        vertices_2d.push_back(state.vertices[i * 3 + 0]);  // X
        vertices_2d.push_back(state.vertices[i * 3 + 1]);  // Y
        break;
      case PolygonDrawPlane::YZ:
        vertices_2d.push_back(state.vertices[i * 3 + 1]);  // Y
        vertices_2d.push_back(state.vertices[i * 3 + 2]);  // Z
        break;
      }
    }

    state.is_convex = IsPolygonConvex(vertices_2d);
    if (!state.is_convex)
    {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Warning: Polygon is not convex!");
    }

    // Get snap step if snapping is enabled
    const ViewportPanelState& viewport = session.viewport_panel();
    const float snap_step = viewport.snap_translate ? viewport.snap_translate_step : 0.0f;

    SnapDragFloat("Extrusion Height", &state.extrusion_height, snap_step, 1.0f, -10000.0f, 10000.0f);

    bool create = false;
    if (ImGui::Button("Create Brush"))
    {
      create = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
      state.show_extrusion_dialog = false;
      state.active = false;
      state.vertices.clear();
    }

    if (create)
    {
      // Get extrusion normal and axis-aligned basis based on plane.
      // The basis must match how vertices_2d was constructed:
      // - XZ: u=X, v=Z, normal=Y
      // - XY: u=X, v=Y, normal=Z
      // - YZ: u=Y, v=Z, normal=X
      float normal[3] = {0.0f, 0.0f, 0.0f};
      float tangent[3] = {0.0f, 0.0f, 0.0f};
      float bitangent[3] = {0.0f, 0.0f, 0.0f};
      switch (state.plane)
      {
      case PolygonDrawPlane::XZ:
        normal[1] = 1.0f;
        tangent[0] = 1.0f;    // u -> X
        bitangent[2] = 1.0f;  // v -> Z
        break;
      case PolygonDrawPlane::XY:
        normal[2] = 1.0f;
        tangent[0] = 1.0f;    // u -> X
        bitangent[1] = 1.0f;  // v -> Y
        break;
      case PolygonDrawPlane::YZ:
        normal[0] = 1.0f;
        tangent[1] = 1.0f;    // u -> Y
        bitangent[2] = 1.0f;  // v -> Z
        break;
      }

      PrimitiveResult result =
          ExtrudePolygon(vertices_2d, state.extrusion_height, normal, tangent, bitangent, state.plane_offset);
      if (result.success)
      {
        const int new_id = CreateBrushFromPrimitive(session.scene_nodes, session.scene_props, result, "Polygon");
        if (new_id >= 0)
        {
          SelectNode(session.scene_panel, new_id);
          session.active_target = SelectionTarget::Scene;
          session.undo_stack.PushCreate(UndoTarget::Scene, new_id);
          session.document_state.MarkDirty();
        }
      }

      state.show_extrusion_dialog = false;
      state.active = false;
      state.vertices.clear();
    }

    ImGui::EndPopup();
  }
}

/// Draws the save prompt dialog and updates the result.
void DrawSavePromptDialog(SavePromptDialogState& dialog)
{
  if (!dialog.open)
  {
    return;
  }

  const char* title = "Save Changes?";
  ImGui::OpenPopup(title);

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
  {
    ImGui::Text("You have unsaved changes. Do you want to save them?");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Save", ImVec2(80, 0)))
    {
      dialog.result = SavePromptResult::Save;
      dialog.open = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Don't Save", ImVec2(100, 0)))
    {
      dialog.result = SavePromptResult::DontSave;
      dialog.open = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(80, 0)))
    {
      dialog.result = SavePromptResult::Cancel;
      dialog.open = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

}  // namespace

void RunEditorLoop(SDL_Window* window, DiligentContext& diligent, EditorSession& session)
{
  bool done = false;
  bool request_reset_layout = true;

  while (!done)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      diligent.imgui->HandleSDLEvent(&event);
      if (event.type == SDL_EVENT_QUIT)
      {
        done = true;
      }
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == SDL_GetWindowID(window))
      {
        done = true;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED ||
        event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
      {
        int w = 0;
        int h = 0;
        if (SDL_GetWindowSizeInPixels(window, &w, &h))
        {
          diligent.engine.swapchain->Resize(
            static_cast<Diligent::Uint32>(w),
            static_cast<Diligent::Uint32>(h),
            Diligent::SURFACE_TRANSFORM_OPTIMAL);
        }
      }
    }

    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
    {
      SDL_Delay(10);
      continue;
    }

    ViewportOverlayState overlay_state{};

    const auto& sc_desc = diligent.engine.swapchain->GetDesc();
    diligent.imgui->NewFrame(sc_desc.Width, sc_desc.Height, sc_desc.PreTransform);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::GetID("DEditDockSpace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    EnsureDockLayout(dockspace_id, viewport, request_reset_layout);
    request_reset_layout = false;
    ImGui::DockSpaceOverViewport(dockspace_id, viewport, dockspace_flags);

    MainMenuActions menu_actions{};
    const bool has_selection = (session.active_target == SelectionTarget::Scene) && HasSelection(session.scene_panel);
    PanelVisibilityFlags panel_flags{
      &session.panel_visibility.show_project,
      &session.panel_visibility.show_worlds,
      &session.panel_visibility.show_scene,
      &session.panel_visibility.show_properties,
      &session.panel_visibility.show_console,
      &session.panel_visibility.show_tools
    };
    ViewportDisplayFlags viewport_display_flags{
      &session.viewport_panel().show_fps
    };
    const bool has_world = !session.scene_nodes.empty();
    // Keep document dirty state in sync with undo stack position
    session.document_state.UpdateFromUndoPosition(session.undo_stack.GetPosition());
    const std::vector<std::string> scene_classes = CollectSceneClasses(
      session.scene_nodes, session.scene_props);
    DrawMainMenuBar(
      request_reset_layout,
      menu_actions,
      session.recent_projects,
      scene_classes,
      session.undo_stack.CanUndo(),
      session.undo_stack.CanRedo(),
      has_selection,
      has_world,
      session.document_state.IsDirty(),
      &panel_flags,
      &viewport_display_flags);

    // Draw toolbar
    const bool snap_enabled = session.viewport_panel().snap_translate ||
      session.viewport_panel().snap_rotate || session.viewport_panel().snap_scale;
    ToolbarResult toolbar_result = DrawToolbar(
      session.tools_panel,
      session.undo_stack.CanUndo(),
      session.undo_stack.CanRedo(),
      snap_enabled);
    if (toolbar_result.snap_toggled)
    {
      // Toggle all snap modes together
      const bool new_snap = !snap_enabled;
      session.viewport_panel().snap_translate = new_snap;
      session.viewport_panel().snap_rotate = new_snap;
      session.viewport_panel().snap_scale = new_snap;
      // Link translate snap step to grid spacing
      if (new_snap)
      {
        session.viewport_panel().snap_translate_step = session.viewport_panel().grid_spacing;
      }
    }
    if (toolbar_result.undo_requested)
    {
      menu_actions.undo = true;
    }
    if (toolbar_result.redo_requested)
    {
      menu_actions.redo = true;
    }
    if (toolbar_result.tool_changed)
    {
      // Map tool selection to gizmo mode
      switch (toolbar_result.new_tool)
      {
      case EditorTool::Move:
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Translate;
        break;
      case EditorTool::Rotate:
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Rotate;
        break;
      case EditorTool::Scale:
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Scale;
        break;
      default:
        break;
      }
    }
    if (toolbar_result.create_primitive != PrimitiveType::None)
    {
      SetPrimitiveCentersToMarker(session.primitive_dialog, session.viewport_panel());
      session.primitive_dialog.open = true;
      session.primitive_dialog.type = toolbar_result.create_primitive;
    }

    ImGuiIO& io = ImGui::GetIO();
    const bool allow_shortcuts = !io.WantTextInput && !ImGui::IsAnyItemActive();
    const bool primary_down = IsPrimaryShortcutDown(io);
    bool trigger_undo = menu_actions.undo;
    bool trigger_redo = menu_actions.redo;
    if (allow_shortcuts)
    {
      // File operations: primary+N (New), primary+O (Open), primary+S (Save), primary+Shift+S (Save As)
      if (primary_down && ImGui::IsKeyPressed(ImGuiKey_N, false))
      {
        menu_actions.new_world = true;
      }
      if (primary_down && ImGui::IsKeyPressed(ImGuiKey_O, false))
      {
        menu_actions.open_world = true;
      }
      if (primary_down && ImGui::IsKeyPressed(ImGuiKey_S, false))
      {
        if (io.KeyShift)
        {
          menu_actions.save_world_as = true;
        }
        else if (has_world)
        {
          menu_actions.save_world = true;
        }
      }

      if (primary_down && ImGui::IsKeyPressed(ImGuiKey_Z, false))
      {
        if (io.KeyShift)
        {
          trigger_redo = true;
        }
        else
        {
          trigger_undo = true;
        }
      }
      if (primary_down && ImGui::IsKeyPressed(ImGuiKey_Y, false))
      {
        trigger_redo = true;
      }
      // Selection commands (only when Scene is active)
      if (session.active_target == SelectionTarget::Scene)
      {
        if (primary_down && ImGui::IsKeyPressed(ImGuiKey_A, false))
        {
          SelectAll(session.scene_panel, session.scene_nodes, session.scene_props);
        }
        if (primary_down && ImGui::IsKeyPressed(ImGuiKey_D, false))
        {
          SelectNone(session.scene_panel);
        }
        if (primary_down && ImGui::IsKeyPressed(ImGuiKey_I, false))
        {
          SelectInverse(session.scene_panel, session.scene_nodes, session.scene_props);
        }
        // Visibility commands: H = Hide Selected, Shift+H = Unhide Selected,
        // Primary+H = Unhide All, Alt+H = Hide Unselected
        if (ImGui::IsKeyPressed(ImGuiKey_H, false))
        {
          if (primary_down && !io.KeyShift && !io.KeyAlt)
          {
            UnhideAllWithUndo(session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (io.KeyShift && !primary_down && !io.KeyAlt)
          {
            UnhideSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (io.KeyAlt && !primary_down && !io.KeyShift)
          {
            HideUnselectedWithUndo(session.scene_panel, session.scene_nodes,
                                   session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (!primary_down && !io.KeyShift && !io.KeyAlt && HasSelection(session.scene_panel))
          {
            HideSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            ClearSelection(session.scene_panel);
            session.document_state.MarkDirty();
          }
        }
        // Freeze commands: F = Freeze Selected, Shift+F = Unfreeze Selected,
        // Primary+F = Unfreeze All
        if (ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
          if (primary_down && !io.KeyShift && !io.KeyAlt)
          {
            UnfreezeAllWithUndo(session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (io.KeyShift && !primary_down && !io.KeyAlt)
          {
            UnfreezeSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (!primary_down && !io.KeyShift && !io.KeyAlt && HasSelection(session.scene_panel))
          {
            FreezeSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            ClearSelection(session.scene_panel);
            session.document_state.MarkDirty();
          }
        }
        // Mirror commands: primary+Shift+X/Y/Z
        if (primary_down && io.KeyShift && HasSelection(session.scene_panel))
        {
          if (ImGui::IsKeyPressed(ImGuiKey_X, false))
          {
            MirrorSelection(session.scene_panel, session.scene_nodes, session.scene_props, MirrorAxis::X);
          }
          if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
          {
            MirrorSelection(session.scene_panel, session.scene_nodes, session.scene_props, MirrorAxis::Y);
          }
          // Note: primary+Shift+Z is also Redo shortcut on some systems, so mirror Z may conflict
          // Users can still use the menu for Mirror Z
        }
        // Group commands: primary+G = Group, primary+Shift+G = Ungroup
        if (ImGui::IsKeyPressed(ImGuiKey_G, false))
        {
          if (primary_down && io.KeyShift && !io.KeyAlt)
          {
            // Ungroup: only works when a single container is selected
            if (SelectionCount(session.scene_panel) == 1)
            {
              int container_id = session.scene_panel.primary_selection;
              if (container_id >= 0 && static_cast<size_t>(container_id) < session.scene_nodes.size() &&
                  session.scene_nodes[container_id].is_folder)
              {
                if (UngroupContainer(container_id, session.scene_nodes, session.scene_props,
                                     session.scene_panel, &session.undo_stack))
                {
                  session.document_state.MarkDirty();
                }
              }
            }
          }
          else if (primary_down && !io.KeyShift && !io.KeyAlt && HasSelection(session.scene_panel))
          {
            // Group selected nodes
            GroupResult result = GroupSelectedNodes(
              session.scene_panel, session.scene_nodes, session.scene_props, &session.undo_stack);
            if (result.success)
            {
              session.document_state.MarkDirty();
            }
          }
        }
      }

      // Viewport view mode shortcuts (numpad keys)
      // Numpad 5: Toggle ortho/perspective
      if (ImGui::IsKeyPressed(ImGuiKey_Keypad5, false))
      {
        ToggleOrthoPerspective(session.viewport_panel());
      }
      // Numpad 7: Top view (primary+7: Bottom)
      if (ImGui::IsKeyPressed(ImGuiKey_Keypad7, false))
      {
        if (primary_down)
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Bottom);
        }
        else
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Top);
        }
      }
      // Numpad 1: Front view (primary+1: Back)
      if (ImGui::IsKeyPressed(ImGuiKey_Keypad1, false))
      {
        if (primary_down)
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Back);
        }
        else
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Front);
        }
      }
      // Numpad 3: Right view (primary+3: Left)
      if (ImGui::IsKeyPressed(ImGuiKey_Keypad3, false))
      {
        if (primary_down)
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Left);
        }
        else
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Right);
        }
      }
      // Tab cycles active viewport (multi-viewport layout)
      if (ImGui::IsKeyPressed(ImGuiKey_Tab, false) && !primary_down && !io.KeyShift)
      {
        CycleActiveViewport(session.multi_viewport);
      }
      // F3: Open Go To Node dialog
      if (ImGui::IsKeyPressed(ImGuiKey_F3, false) && !session.scene_nodes.empty())
      {
        OpenGoToDialog(session.goto_dialog);
      }
      // W: Translate gizmo mode
      if (ImGui::IsKeyPressed(ImGuiKey_W, false) && !primary_down && !io.KeyShift && !io.KeyAlt)
      {
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Translate;
      }
      // E: Rotate gizmo mode
      if (ImGui::IsKeyPressed(ImGuiKey_E, false) && !primary_down && !io.KeyShift && !io.KeyAlt)
      {
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Rotate;
      }
      // R: Scale gizmo mode
      if (ImGui::IsKeyPressed(ImGuiKey_R, false) && !primary_down && !io.KeyShift && !io.KeyAlt)
      {
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Scale;
      }

      // Grid controls
      // G: Toggle grid visibility (plain G without modifiers; primary+G is Group)
      if (ImGui::IsKeyPressed(ImGuiKey_G, false) && !primary_down && !io.KeyShift && !io.KeyAlt)
      {
        session.viewport_panel().show_grid = !session.viewport_panel().show_grid;
      }
      // [: Halve grid spacing
      if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false))
      {
        session.viewport_panel().grid_spacing = grid::HalveSpacing(session.viewport_panel().grid_spacing);
        // Keep snap step in sync with grid when snapping is enabled
        if (session.viewport_panel().snap_translate)
        {
          session.viewport_panel().snap_translate_step = session.viewport_panel().grid_spacing;
        }
      }
      // ]: Double grid spacing
      if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false))
      {
        session.viewport_panel().grid_spacing = grid::DoubleSpacing(session.viewport_panel().grid_spacing);
        // Keep snap step in sync with grid when snapping is enabled
        if (session.viewport_panel().snap_translate)
        {
          session.viewport_panel().snap_translate_step = session.viewport_panel().grid_spacing;
        }
      }

      // B: Open Box primitive dialog
      if (ImGui::IsKeyPressed(ImGuiKey_B, false) && !primary_down && !io.KeyShift && !io.KeyAlt)
      {
        SetPrimitiveCentersToMarker(session.primitive_dialog, session.viewport_panel());
        session.primitive_dialog.open = true;
        session.primitive_dialog.type = PrimitiveType::Box;
      }

      // P: Enter polygon drawing mode (only when not already in polygon mode)
      if (ImGui::IsKeyPressed(ImGuiKey_P, false) && !primary_down && !io.KeyShift && !io.KeyAlt &&
          !session.polygon_draw.active)
      {
        session.polygon_draw.active = true;
        session.polygon_draw.vertices.clear();
        session.polygon_draw.is_convex = true;
        session.polygon_draw.show_extrusion_dialog = false;
        // Set plane based on current view
        const auto view_mode = session.viewport_panel().view_mode;
        if (view_mode == ViewportPanelState::ViewMode::Top ||
            view_mode == ViewportPanelState::ViewMode::Bottom)
        {
          session.polygon_draw.plane = PolygonDrawPlane::XZ;
          session.polygon_draw.plane_offset = session.viewport_panel().marker_position[1];
        }
        else if (view_mode == ViewportPanelState::ViewMode::Front ||
                 view_mode == ViewportPanelState::ViewMode::Back)
        {
          session.polygon_draw.plane = PolygonDrawPlane::XY;
          session.polygon_draw.plane_offset = session.viewport_panel().marker_position[2];
        }
        else if (view_mode == ViewportPanelState::ViewMode::Left ||
                 view_mode == ViewportPanelState::ViewMode::Right)
        {
          session.polygon_draw.plane = PolygonDrawPlane::YZ;
          session.polygon_draw.plane_offset = session.viewport_panel().marker_position[0];
        }
        else
        {
          // Perspective: default to XZ
          session.polygon_draw.plane = PolygonDrawPlane::XZ;
          session.polygon_draw.plane_offset = session.viewport_panel().marker_position[1];
        }
      }

      // Polygon drawing mode keyboard controls
      if (session.polygon_draw.active && !session.polygon_draw.show_extrusion_dialog)
      {
        // Escape: Cancel polygon mode
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
          session.polygon_draw.active = false;
          session.polygon_draw.vertices.clear();
        }
        // Backspace: Remove last vertex
        else if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false) && session.polygon_draw.vertices.size() >= 3)
        {
          session.polygon_draw.vertices.pop_back();
          session.polygon_draw.vertices.pop_back();
          session.polygon_draw.vertices.pop_back();
        }
        // Enter: Complete polygon (need at least 3 vertices)
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) && session.polygon_draw.vertices.size() >= 9)
        {
          session.polygon_draw.show_extrusion_dialog = true;
        }
      }

      // Nudge keys (arrow keys for XY/XZ movement, Page Up/Down for depth)
      if (session.active_target == SelectionTarget::Scene && HasSelection(session.scene_panel))
      {
        const NudgeIncrement increment = io.KeyCtrl ? NudgeIncrement::Small :
          (io.KeyShift ? NudgeIncrement::Large : NudgeIncrement::Normal);

        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
        {
          NudgeSelection(session.viewport_panel(), session.scene_panel, session.scene_nodes,
            session.scene_props, NudgeDirection::Left, increment, &session.undo_stack);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
        {
          NudgeSelection(session.viewport_panel(), session.scene_panel, session.scene_nodes,
            session.scene_props, NudgeDirection::Right, increment, &session.undo_stack);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false))
        {
          NudgeSelection(session.viewport_panel(), session.scene_panel, session.scene_nodes,
            session.scene_props, NudgeDirection::Up, increment, &session.undo_stack);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))
        {
          NudgeSelection(session.viewport_panel(), session.scene_panel, session.scene_nodes,
            session.scene_props, NudgeDirection::Down, increment, &session.undo_stack);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp, false))
        {
          NudgeSelection(session.viewport_panel(), session.scene_panel, session.scene_nodes,
            session.scene_props, NudgeDirection::Forward, increment, &session.undo_stack);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown, false))
        {
          NudgeSelection(session.viewport_panel(), session.scene_panel, session.scene_nodes,
            session.scene_props, NudgeDirection::Back, increment, &session.undo_stack);
        }
      }
    }
    if (trigger_undo)
    {
      session.undo_stack.Undo(session.project_nodes, session.scene_nodes,
                              session.project_props, session.scene_props);
      session.document_state.UpdateFromUndoPosition(session.undo_stack.GetPosition());
    }
    if (trigger_redo)
    {
      session.undo_stack.Redo(session.project_nodes, session.scene_nodes,
                              session.project_props, session.scene_props);
      session.document_state.UpdateFromUndoPosition(session.undo_stack.GetPosition());
    }

    // Handle menu-triggered selection/visibility commands
    if (session.active_target == SelectionTarget::Scene)
    {
      if (menu_actions.select_all)
      {
        SelectAll(session.scene_panel, session.scene_nodes, session.scene_props);
      }
      if (menu_actions.select_none)
      {
        SelectNone(session.scene_panel);
      }
      if (menu_actions.select_inverse)
      {
        SelectInverse(session.scene_panel, session.scene_nodes, session.scene_props);
      }
      if (menu_actions.select_brushes)
      {
        SelectByType(session.scene_panel, session.scene_nodes, session.scene_props, "Brush");
      }
      if (menu_actions.select_lights)
      {
        SelectByType(session.scene_panel, session.scene_nodes, session.scene_props, "Light");
      }
      if (menu_actions.select_objects)
      {
        SelectByType(session.scene_panel, session.scene_nodes, session.scene_props, "Object");
      }
      if (menu_actions.select_world_models)
      {
        SelectByType(session.scene_panel, session.scene_nodes, session.scene_props, "WorldModel");
      }
      if (menu_actions.select_by_class)
      {
        SelectByClass(session.scene_panel, session.scene_nodes, session.scene_props,
          menu_actions.select_class_name);
      }
      if (menu_actions.hide_selected && HasSelection(session.scene_panel))
      {
        HideSelected(session.scene_panel, session.scene_props);
        ClearSelection(session.scene_panel);
      }
      if (menu_actions.unhide_all)
      {
        UnhideAll(session.scene_props);
      }
      if (menu_actions.freeze_selected && HasSelection(session.scene_panel))
      {
        FreezeSelected(session.scene_panel, session.scene_props);
        ClearSelection(session.scene_panel);
      }
      if (menu_actions.unfreeze_all)
      {
        UnfreezeAll(session.scene_props);
      }
      // Mirror operations
      if (menu_actions.mirror_x && HasSelection(session.scene_panel))
      {
        MirrorSelection(session.scene_panel, session.scene_nodes, session.scene_props, MirrorAxis::X);
      }
      if (menu_actions.mirror_y && HasSelection(session.scene_panel))
      {
        MirrorSelection(session.scene_panel, session.scene_nodes, session.scene_props, MirrorAxis::Y);
      }
      if (menu_actions.mirror_z && HasSelection(session.scene_panel))
      {
        MirrorSelection(session.scene_panel, session.scene_nodes, session.scene_props, MirrorAxis::Z);
      }
      // Rotate selection dialog
      if (menu_actions.rotate_selection && HasSelection(session.scene_panel))
      {
        session.rotate_dialog.open = true;
      }
      // Mirror selection dialog
      if (menu_actions.mirror_selection_dialog && HasSelection(session.scene_panel))
      {
        session.mirror_dialog.open = true;
      }
    }

    // Handle marker dialog
    if (menu_actions.open_marker_dialog)
    {
      session.marker_dialog.position[0] = session.viewport_panel().marker_position[0];
      session.marker_dialog.position[1] = session.viewport_panel().marker_position[1];
      session.marker_dialog.position[2] = session.viewport_panel().marker_position[2];
      session.marker_dialog.open = true;
    }
    if (menu_actions.reset_marker)
    {
      session.viewport_panel().marker_position[0] = 0.0f;
      session.viewport_panel().marker_position[1] = 0.0f;
      session.viewport_panel().marker_position[2] = 0.0f;
    }

    // Handle selection filter dialog
    if (menu_actions.open_selection_filter)
    {
      session.selection_filter_dialog.open = true;
    }
    if (menu_actions.open_advanced_selection)
    {
      session.advanced_selection_dialog.open = true;
    }

    // Handle view mode changes from menu
    switch (menu_actions.view_mode)
    {
    case ViewModeAction::Perspective:
      session.viewport_panel().view_mode = ViewportPanelState::ViewMode::Perspective;
      break;
    case ViewModeAction::Top:
      SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Top);
      break;
    case ViewModeAction::Bottom:
      SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Bottom);
      break;
    case ViewModeAction::Front:
      SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Front);
      break;
    case ViewModeAction::Back:
      SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Back);
      break;
    case ViewModeAction::Left:
      SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Left);
      break;
    case ViewModeAction::Right:
      SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Right);
      break;
    case ViewModeAction::ToggleOrthoPerspective:
      ToggleOrthoPerspective(session.viewport_panel());
      break;
    case ViewModeAction::None:
    default:
      break;
    }

    // Handle viewport layout changes
    if (menu_actions.layout_change.has_value())
    {
      SetViewportLayout(session.multi_viewport, *menu_actions.layout_change);
    }
    if (menu_actions.cycle_viewport)
    {
      CycleActiveViewport(session.multi_viewport);
    }

    // Handle primitive creation menu action
    if (menu_actions.create_primitive != PrimitiveType::None)
    {
      SetPrimitiveCentersToMarker(session.primitive_dialog, session.viewport_panel());
      session.primitive_dialog.open = true;
      session.primitive_dialog.type = menu_actions.create_primitive;
    }

    // Handle enter polygon drawing mode
    if (menu_actions.enter_polygon_mode)
    {
      session.polygon_draw.active = true;
      session.polygon_draw.vertices.clear();
      session.polygon_draw.is_convex = true;
      session.polygon_draw.show_extrusion_dialog = false;
      // Set plane based on current view
      const auto view_mode = session.viewport_panel().view_mode;
      if (view_mode == ViewportPanelState::ViewMode::Top ||
          view_mode == ViewportPanelState::ViewMode::Bottom)
      {
        session.polygon_draw.plane = PolygonDrawPlane::XZ;
        session.polygon_draw.plane_offset = session.viewport_panel().marker_position[1];
      }
      else if (view_mode == ViewportPanelState::ViewMode::Front ||
               view_mode == ViewportPanelState::ViewMode::Back)
      {
        session.polygon_draw.plane = PolygonDrawPlane::XY;
        session.polygon_draw.plane_offset = session.viewport_panel().marker_position[2];
      }
      else if (view_mode == ViewportPanelState::ViewMode::Left ||
               view_mode == ViewportPanelState::ViewMode::Right)
      {
        session.polygon_draw.plane = PolygonDrawPlane::YZ;
        session.polygon_draw.plane_offset = session.viewport_panel().marker_position[0];
      }
      else
      {
        // Perspective view: default to XZ (horizontal)
        session.polygon_draw.plane = PolygonDrawPlane::XZ;
        session.polygon_draw.plane_offset = session.viewport_panel().marker_position[1];
      }
    }

    // Handle reset splitters
    if (menu_actions.reset_splitters)
    {
      ResetSplitters(session.multi_viewport);
    }

    if (menu_actions.open_project_folder)
    {
      std::string selected_path;
      const std::string initial_path = session.project_root.empty()
        ? std::filesystem::current_path().string()
        : session.project_root;
      if (OpenFolderDialog(initial_path, selected_path))
      {
        session.project_root = ResolveProjectRoot(selected_path);
        session.project_error.clear();
        session.project_nodes = BuildProjectTree(session.project_root, session.project_props, session.project_error);
        session.project_panel.error = session.project_error;
        session.project_panel.selected_id = DefaultProjectSelection(session.project_nodes);
        session.project_panel.tree_ui = {};
        session.world_browser.refresh = true;
        SetEngineProjectRoot(diligent.engine, session.project_root);
        PushRecentProject(session.recent_projects, session.project_root);
      }
    }
    if (menu_actions.open_recent_project)
    {
      session.project_root = ResolveProjectRoot(menu_actions.recent_project_path);
      session.project_error.clear();
      session.project_nodes = BuildProjectTree(session.project_root, session.project_props, session.project_error);
      session.project_panel.error = session.project_error;
      session.project_panel.selected_id = DefaultProjectSelection(session.project_nodes);
      session.project_panel.tree_ui = {};
      session.world_browser.refresh = true;
      SetEngineProjectRoot(diligent.engine, session.project_root);
      PushRecentProject(session.recent_projects, session.project_root);
    }

    // Handle world file operations
    // Lambda to perform New World action
    auto doNewWorld = [&]() {
      World new_world = CreateEmptyWorld("Untitled");
      session.scene_nodes = std::move(new_world.nodes);
      session.scene_props = std::move(new_world.properties);
      session.world_file.clear();
      session.scene_error.clear();
      session.scene_panel = ScenePanelState{};
      session.active_target = SelectionTarget::Scene;
      session.document_state.Reset();
      session.undo_stack.Clear();
    };

    // Lambda to perform Open World action
    auto doOpenWorld = [&](const std::string& path) {
      LoadSceneWorld(
        path,
        diligent,
        session.project_root,
        session.world_file,
        session.viewport_panel(),
        session.scene_nodes,
        session.scene_props,
        session.scene_panel,
        session.scene_error,
        session.active_target,
        session.world_settings_cache);
      session.document_state.Reset();
      session.undo_stack.Clear();
    };

    // Lambda to save the current world
    auto doSaveWorld = [&]() -> bool {
      if (session.world_file.empty())
      {
        // Need Save As - prompt for path
        std::string selected_path;
        const std::string initial_dir = session.project_root.empty()
          ? std::filesystem::current_path().string()
          : session.project_root;
        if (!SaveFileDialog(initial_dir, "Untitled", "lta", "LithTech World", selected_path))
        {
          return false;  // User cancelled
        }
        session.world_file = selected_path;
      }

      World world;
      world.source_path = session.world_file;
      world.nodes = session.scene_nodes;
      world.properties = session.scene_props;
      world.ExtractWorldProperties();

      WorldFormat format = DetectWorldFormat(session.world_file);
      if (format == WorldFormat::Unknown || format == WorldFormat::Binary)
      {
        format = WorldFormat::LTA;
      }

      std::string save_error;
      if (SaveWorld(world, session.world_file, format, save_error))
      {
        session.document_state.MarkSaved(session.undo_stack.GetPosition());
        return true;
      }
      session.scene_error = "Save failed: " + save_error;
      return false;
    };

    if (menu_actions.new_world)
    {
      if (session.document_state.IsDirty() && has_world)
      {
        // Prompt to save
        session.save_prompt_dialog.open = true;
        session.save_prompt_dialog.trigger = SavePromptTrigger::NewWorld;
        session.save_prompt_dialog.result = SavePromptResult::Pending;
        session.save_prompt_dialog.pending_world_path.clear();
      }
      else
      {
        doNewWorld();
      }
    }
    if (menu_actions.open_world)
    {
      std::string selected_path;
      const std::string initial_dir = session.project_root.empty()
        ? std::filesystem::current_path().string()
        : session.project_root;
      if (OpenFileDialog(initial_dir, {"lta", "ltc"}, "World Files", selected_path))
      {
        if (session.document_state.IsDirty() && has_world)
        {
          // Prompt to save
          session.save_prompt_dialog.open = true;
          session.save_prompt_dialog.trigger = SavePromptTrigger::OpenWorld;
          session.save_prompt_dialog.result = SavePromptResult::Pending;
          session.save_prompt_dialog.pending_world_path = selected_path;
        }
        else
        {
          doOpenWorld(selected_path);
        }
      }
    }

    // Handle save prompt dialog results
    if (!session.save_prompt_dialog.open &&
        session.save_prompt_dialog.result != SavePromptResult::Pending)
    {
      SavePromptTrigger trigger = session.save_prompt_dialog.trigger;
      SavePromptResult result = session.save_prompt_dialog.result;
      std::string pending_path = session.save_prompt_dialog.pending_world_path;

      // Reset dialog state
      session.save_prompt_dialog.trigger = SavePromptTrigger::None;
      session.save_prompt_dialog.result = SavePromptResult::Pending;
      session.save_prompt_dialog.pending_world_path.clear();

      if (result == SavePromptResult::Cancel)
      {
        // User cancelled, do nothing
      }
      else if (result == SavePromptResult::Save)
      {
        // Save first, then proceed if successful
        if (doSaveWorld())
        {
          switch (trigger)
          {
          case SavePromptTrigger::NewWorld:
            doNewWorld();
            break;
          case SavePromptTrigger::OpenWorld:
            doOpenWorld(pending_path);
            break;
          default:
            break;
          }
        }
      }
      else if (result == SavePromptResult::DontSave)
      {
        // Proceed without saving
        switch (trigger)
        {
        case SavePromptTrigger::NewWorld:
          doNewWorld();
          break;
        case SavePromptTrigger::OpenWorld:
          doOpenWorld(pending_path);
          break;
        default:
          break;
        }
      }
    }
    if (menu_actions.save_world && has_world)
    {
      if (session.world_file.empty())
      {
        // No existing file, treat as Save As
        menu_actions.save_world_as = true;
      }
      else
      {
        // Save to existing path
        World world;
        world.source_path = session.world_file;
        world.nodes = session.scene_nodes;
        world.properties = session.scene_props;
        world.ExtractWorldProperties();

        WorldFormat format = DetectWorldFormat(session.world_file);
        if (format == WorldFormat::Unknown || format == WorldFormat::Binary)
        {
          format = WorldFormat::LTA;
        }

        std::string save_error;
        if (SaveWorld(world, session.world_file, format, save_error))
        {
          session.document_state.MarkSaved(session.undo_stack.GetPosition());
        }
        else
        {
          session.scene_error = "Save failed: " + save_error;
        }
      }
    }
    if (menu_actions.save_world_as && has_world)
    {
      std::string selected_path;
      const std::string initial_dir = session.world_file.empty()
        ? (session.project_root.empty() ? std::filesystem::current_path().string() : session.project_root)
        : std::filesystem::path(session.world_file).parent_path().string();
      const std::string default_name = session.world_file.empty()
        ? "Untitled"
        : std::filesystem::path(session.world_file).stem().string();

      if (SaveFileDialog(initial_dir, default_name, "lta", "LithTech World", selected_path))
      {
        World world;
        world.source_path = selected_path;
        world.nodes = session.scene_nodes;
        world.properties = session.scene_props;
        world.ExtractWorldProperties();

        WorldFormat format = DetectWorldFormat(selected_path);
        if (format == WorldFormat::Unknown || format == WorldFormat::Binary)
        {
          format = WorldFormat::LTA;
        }

        std::string save_error;
        if (SaveWorld(world, selected_path, format, save_error))
        {
          session.world_file = selected_path;
          session.document_state.MarkSaved(session.undo_stack.GetPosition());
        }
        else
        {
          session.scene_error = "Save failed: " + save_error;
        }
      }
    }

    // Handle reset panel visibility
    if (menu_actions.reset_panel_visibility)
    {
      session.panel_visibility = PanelVisibility{};
    }

    ProjectContextAction project_action{};
    if (session.panel_visibility.show_project)
    {
      DrawProjectPanel(
        session.project_panel,
        session.project_root,
        session.project_nodes,
        session.project_props,
        session.active_target,
        &project_action,
        &session.undo_stack);
      if (session.project_panel.error != session.project_error)
      {
        session.project_error = session.project_panel.error;
      }
    }
    if (project_action.load_world)
    {
      LoadSceneWorld(
        project_action.world_path,
        diligent,
        session.project_root,
        session.world_file,
        session.viewport_panel(),
        session.scene_nodes,
        session.scene_props,
        session.scene_panel,
        session.scene_error,
        session.active_target,
        session.world_settings_cache);
    }
    WorldBrowserAction world_action{};
    if (session.panel_visibility.show_worlds)
    {
      DrawWorldBrowserPanel(session.world_browser, session.project_root, world_action);
    }
    if (world_action.load_world)
    {
      LoadSceneWorld(
        world_action.world_path,
        diligent,
        session.project_root,
        session.world_file,
        session.viewport_panel(),
        session.scene_nodes,
        session.scene_props,
        session.scene_panel,
        session.scene_error,
        session.active_target,
        session.world_settings_cache);
    }
    if (session.panel_visibility.show_scene)
    {
      DrawScenePanel(session.scene_panel, session.scene_nodes, session.scene_props, session.active_target, &session.undo_stack);
      if (session.scene_panel.error != session.scene_error)
      {
        session.scene_error = session.scene_panel.error;
      }

      // Handle double-click focus request from Scene Tree
      const int focus_id = session.scene_panel.tree_ui.focus_request_id;
      if (focus_id >= 0 && static_cast<size_t>(focus_id) < session.scene_props.size())
      {
        const NodeProperties& focus_props = session.scene_props[focus_id];
        float bounds_min[3], bounds_max[3];
        bool has_bounds = ComputeSelectionBounds(
          session.scene_panel, session.scene_nodes, session.scene_props,
          bounds_min, bounds_max);
        FocusViewportOn(
          session.multi_viewport.ActiveViewport(),
          focus_props.position,
          has_bounds ? bounds_min : nullptr,
          has_bounds ? bounds_max : nullptr);
        session.scene_panel.tree_ui.focus_request_id = -1;  // Clear request
      }
    }
    if (session.panel_visibility.show_properties)
    {
      DrawPropertiesPanel(
        session.active_target,
        session.project_nodes,
        session.project_props,
        session.project_panel.selected_id,
        session.scene_nodes,
        session.scene_props,
        session.scene_panel.primary_selection,
        session.project_root);
    }

    if (session.panel_visibility.show_console)
    {
      DrawConsolePanel(session.console_panel);
    }

    // Draw tools panel and handle tool selection
    // Sync panel visibility with tools panel state
    session.tools_panel.visible = session.panel_visibility.show_tools;
    ToolsPanelResult tools_result = DrawToolsPanel(session.tools_panel);
    session.panel_visibility.show_tools = session.tools_panel.visible;
    if (tools_result.create_primitive != PrimitiveType::None)
    {
      SetPrimitiveCentersToMarker(session.primitive_dialog, session.viewport_panel());
      session.primitive_dialog.open = true;
      session.primitive_dialog.type = tools_result.create_primitive;
    }
    if (tools_result.tool_changed)
    {
      // Map tool selection to gizmo mode
      switch (tools_result.new_tool)
      {
      case EditorTool::Move:
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Translate;
        break;
      case EditorTool::Rotate:
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Rotate;
        break;
      case EditorTool::Scale:
        session.viewport_panel().gizmo_mode = ViewportPanelState::GizmoMode::Scale;
        break;
      default:
        break;
      }
    }

    // Handle Shift+A for primitive popup
    if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A, false) && !primary_down)
    {
      ImGui::OpenPopup("AddPrimitive");
    }
    PrimitiveType popup_primitive = PrimitiveType::None;
    DrawPrimitivePopup(popup_primitive);
    if (popup_primitive != PrimitiveType::None)
    {
      SetPrimitiveCentersToMarker(session.primitive_dialog, session.viewport_panel());
      session.primitive_dialog.open = true;
      session.primitive_dialog.type = popup_primitive;
    }

    // Draw modal dialogs
    DrawPrimitiveDialog(session);
    DrawPolygonExtrusionDialog(session);
    DrawSavePromptDialog(session.save_prompt_dialog);
    DrawSelectionFilterDialog(session.selection_filter_dialog, session.selection_filter);
    DrawAdvancedSelectionDialog(
      session.advanced_selection_dialog,
      session.scene_panel,
      session.scene_nodes,
      session.scene_props);
    DrawRotateDialog(
      session.rotate_dialog,
      session.scene_panel,
      session.scene_nodes,
      session.scene_props,
      &session.undo_stack);
    DrawMirrorDialog(
      session.mirror_dialog,
      session.scene_panel,
      session.scene_nodes,
      session.scene_props,
      &session.undo_stack);
    DrawMarkerDialog(session.marker_dialog, session.viewport_panel());

    // Draw Go To Node dialog and handle result
    GoToResult goto_result = DrawGoToDialog(
      session.goto_dialog,
      session.scene_nodes,
      session.scene_props);
    if (goto_result.accepted && goto_result.selected_id >= 0)
    {
      // Select the node
      SelectNode(session.scene_panel, goto_result.selected_id);
      session.active_target = SelectionTarget::Scene;

      // Focus viewport on the node
      if (static_cast<size_t>(goto_result.selected_id) < session.scene_props.size())
      {
        const NodeProperties& focus_props = session.scene_props[goto_result.selected_id];
        FocusViewportOn(session.multi_viewport.ActiveViewport(), focus_props.position, nullptr, nullptr);
      }
    }

    ViewportPanelResult viewport_result = DrawViewportPanel(
      diligent,
      session.multi_viewport,
      session.scene_panel,
      session.selection_filter,
      session.depth_cycle,
      session.scene_nodes,
      session.scene_props,
      session.active_target,
      &session.undo_stack);

    overlay_state = viewport_result.overlays;
    if (viewport_result.clicked_scene_id >= 0)
    {
      // Determine selection mode based on modifier keys
      SelectionMode sel_mode = SelectionMode::Replace;
      if (io.KeyShift && primary_down)
      {
        sel_mode = SelectionMode::Remove;
      }
      else if (io.KeyShift)
      {
        sel_mode = SelectionMode::Add;
      }
      else if (primary_down)
      {
        sel_mode = SelectionMode::Toggle;
      }
      ModifySelection(session.scene_panel, viewport_result.clicked_scene_id, sel_mode);
      session.active_target = SelectionTarget::Scene;
    }

    // Handle marquee selection results
    if (!viewport_result.marquee_selected_ids.empty())
    {
      if (viewport_result.marquee_subtractive)
      {
        // Remove selected nodes
        for (int node_id : viewport_result.marquee_selected_ids)
        {
          ModifySelection(session.scene_panel, node_id, SelectionMode::Remove);
        }
      }
      else if (viewport_result.marquee_additive)
      {
        // Add to selection
        for (int node_id : viewport_result.marquee_selected_ids)
        {
          ModifySelection(session.scene_panel, node_id, SelectionMode::Add);
        }
      }
      else
      {
        // Replace selection
        ClearSelection(session.scene_panel);
        for (int node_id : viewport_result.marquee_selected_ids)
        {
          ModifySelection(session.scene_panel, node_id, SelectionMode::Add);
        }
      }
      session.active_target = SelectionTarget::Scene;
    }

    // Polygon drawing mode: handle left-click to add vertices
    if (session.polygon_draw.active && !session.polygon_draw.show_extrusion_dialog &&
        viewport_result.active_viewport_hovered &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
      // Build a pick ray from the mouse position
      const ImVec2 mouse = ImGui::GetIO().MousePos;
      const ImVec2 local(mouse.x - viewport_result.active_viewport_pos.x,
                         mouse.y - viewport_result.active_viewport_pos.y);
      const PickRay ray = BuildPickRay(session.viewport_panel(), viewport_result.active_viewport_size, local);

      // Intersect with the drawing plane
      float normal[3] = {0.0f, 0.0f, 0.0f};
      switch (session.polygon_draw.plane)
      {
      case PolygonDrawPlane::XZ:
        normal[1] = 1.0f;
        break;
      case PolygonDrawPlane::XY:
        normal[2] = 1.0f;
        break;
      case PolygonDrawPlane::YZ:
        normal[0] = 1.0f;
        break;
      }

      Diligent::float3 hit_pos;
      if (RayPlaneIntersect(ray, normal, session.polygon_draw.plane_offset, hit_pos))
      {
        // Snap to grid if enabled
        float snap_step = 0.0f;
        if (session.viewport_panel().snap_translate)
        {
          snap_step = session.viewport_panel().snap_translate_step;
        }

        float x = hit_pos.x;
        float y = hit_pos.y;
        float z = hit_pos.z;

        if (snap_step > 0.0f)
        {
          x = grid::SnapValue(x, snap_step);
          y = grid::SnapValue(y, snap_step);
          z = grid::SnapValue(z, snap_step);
        }

        // Add the vertex
        session.polygon_draw.vertices.push_back(x);
        session.polygon_draw.vertices.push_back(y);
        session.polygon_draw.vertices.push_back(z);
      }
    }

    // Marker controls (require viewport interaction data)
    if (allow_shortcuts)
    {
      // M: Set marker at cursor position
      if (ImGui::IsKeyPressed(ImGuiKey_M, false) && !primary_down && !io.KeyShift && !io.KeyAlt)
      {
        if (viewport_result.hovered_hit_valid)
        {
          session.viewport_panel().marker_position[0] = viewport_result.hovered_hit_pos.x;
          session.viewport_panel().marker_position[1] = viewport_result.hovered_hit_pos.y;
          session.viewport_panel().marker_position[2] = viewport_result.hovered_hit_pos.z;
        }
      }
      // Primary+M: Reset marker to origin
      if (primary_down && ImGui::IsKeyPressed(ImGuiKey_M, false) && !io.KeyShift && !io.KeyAlt)
      {
        session.viewport_panel().marker_position[0] = 0.0f;
        session.viewport_panel().marker_position[1] = 0.0f;
        session.viewport_panel().marker_position[2] = 0.0f;
      }
      // Shift+M: Open marker position dialog
      if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_M, false) && !primary_down && !io.KeyAlt)
      {
        session.marker_dialog.position[0] = session.viewport_panel().marker_position[0];
        session.marker_dialog.position[1] = session.viewport_panel().marker_position[1];
        session.marker_dialog.position[2] = session.viewport_panel().marker_position[2];
        session.marker_dialog.open = true;
      }
    }

    const NodeProperties* world_props = FindWorldProperties(session.scene_props);
    if (world_props && WorldSettingsDifferent(*world_props, session.world_settings_cache))
    {
      ApplyWorldSettingsToRenderer(*world_props);
      UpdateWorldSettingsCache(*world_props, session.world_settings_cache);
    }
    std::vector<DynamicLight> viewport_dynamic_lights;
    ApplyViewportDirectionalLight(
      diligent.engine,
      session.viewport_panel(),
      session.scene_panel,
      session.scene_nodes,
      session.scene_props);
    BuildViewportDynamicLights(
      session.viewport_panel(),
      session.scene_panel,
      session.scene_nodes,
      session.scene_props,
      viewport_dynamic_lights);
    ApplySceneVisibilityToRenderer(
      session.scene_panel,
      session.viewport_panel(),
      session.scene_nodes,
      session.scene_props);

    // Render all visible viewport slots
    const int visible_count = session.multi_viewport.VisibleViewportCount();
    for (int slot = 0; slot < visible_count; ++slot)
    {
      ViewportPanelState& slot_state = session.multi_viewport.viewports[slot].state;
      RenderViewport(diligent, slot, slot_state, world_props, overlay_state, viewport_dynamic_lights);
    }

    Diligent::ITextureView* back_rtv = diligent.engine.swapchain->GetCurrentBackBufferRTV();
    Diligent::ITextureView* back_dsv = diligent.engine.swapchain->GetDepthBufferDSV();
    diligent.engine.context->SetRenderTargets(
      1,
      &back_rtv,
      back_dsv,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const float clear_color[] = {0.10f, 0.11f, 0.12f, 1.0f};
    diligent.engine.context->ClearRenderTarget(back_rtv, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (back_dsv != nullptr)
    {
      diligent.engine.context->ClearDepthStencil(
        back_dsv,
        Diligent::CLEAR_DEPTH_FLAG,
        1.0f,
        0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    // Draw polygon outline if in polygon draw mode
    if (session.polygon_draw.active && !session.polygon_draw.vertices.empty())
    {
      const size_t vertex_count = session.polygon_draw.vertices.size() / 3;
      if (vertex_count > 0 && viewport_result.active_viewport_size.x > 0 &&
          viewport_result.active_viewport_size.y > 0)
      {
        const float aspect =
            viewport_result.active_viewport_size.x / viewport_result.active_viewport_size.y;
        const Diligent::float4x4 view_proj = ComputeViewportViewProj(session.viewport_panel(), aspect);

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        const ImU32 line_color = IM_COL32(0, 255, 128, 255);      // Green lines
        const ImU32 vertex_color = IM_COL32(255, 200, 50, 255);   // Orange vertices
        constexpr float thickness = 2.0f;

        DrawPolygonOutline(
            session.polygon_draw.vertices.data(),
            vertex_count,
            view_proj,
            viewport_result.active_viewport_pos,
            viewport_result.active_viewport_size,
            draw_list,
            line_color,
            vertex_color,
            thickness);
      }
    }

    // Draw status bar
    {
      StatusBarInfo status_info;
      status_info.grid_spacing = session.viewport_panel().grid_spacing;
      status_info.cursor_valid = viewport_result.hovered_hit_valid;
      if (viewport_result.hovered_hit_valid)
      {
        status_info.cursor_pos[0] = viewport_result.hovered_hit_pos.x;
        status_info.cursor_pos[1] = viewport_result.hovered_hit_pos.y;
        status_info.cursor_pos[2] = viewport_result.hovered_hit_pos.z;
      }
      status_info.selection_count = SelectionCount(session.scene_panel);
      status_info.current_tool = session.tools_panel.selected_tool;
      status_info.show_fps = session.viewport_panel().show_fps;
      status_info.fps = 1.0f / io.DeltaTime;
      status_info.filter_active = session.selection_filter.filter_active;
      if (session.selection_filter.filter_active)
      {
        static char filter_status_buf[32];
        snprintf(filter_status_buf, sizeof(filter_status_buf), "%zu/7",
          session.selection_filter.enabled_types.size());
        status_info.filter_status = filter_status_buf;
      }
      if (!viewport_result.depth_cycle_status.empty())
      {
        status_info.depth_cycle_status = viewport_result.depth_cycle_status.c_str();
      }
      // Show polygon mode hint
      if (session.polygon_draw.active)
      {
        static char polygon_hint_buf[64];
        const size_t num_verts = session.polygon_draw.vertices.size() / 3;
        if (session.polygon_draw.show_extrusion_dialog)
        {
          snprintf(polygon_hint_buf, sizeof(polygon_hint_buf), "POLYGON: Set extrusion height");
        }
        else if (num_verts < 3)
        {
          snprintf(polygon_hint_buf, sizeof(polygon_hint_buf),
                   "POLYGON: Click to add vertices (%zu/3 min) | Esc=Cancel", num_verts);
        }
        else
        {
          snprintf(polygon_hint_buf, sizeof(polygon_hint_buf),
                   "POLYGON: %zu vertices | Enter=Complete | Backspace=Undo | Esc=Cancel", num_verts);
        }
        status_info.hint = polygon_hint_buf;
      }
      DrawStatusBar(status_info);
    }

    diligent.imgui->Render(diligent.engine.context);
    diligent.engine.swapchain->Present();
  }
}
