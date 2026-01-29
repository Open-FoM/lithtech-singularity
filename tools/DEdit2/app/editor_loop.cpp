#include "app/editor_loop.h"

#include "app/editor_paths.h"
#include "app/editor_session.h"
#include "app/project_utils.h"
#include "app/recent_projects.h"
#include "app/scene_loader.h"
#include "app/world.h"
#include "grouping/node_grouping.h"
#include "ui/viewport_panel.h"
#include "viewport/diligent_viewport.h"
#include "viewport/lighting.h"
#include "viewport/render_settings.h"
#include "viewport/world_settings.h"

#include "transform/mirror.h"
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

    switch (dialog.type)
    {
    case PrimitiveType::Box:
      ImGui::DragFloat3("Center", dialog.box_params.center, 1.0f);
      ImGui::DragFloat3("Size", dialog.box_params.size, 1.0f, 1.0f, 10000.0f);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Cylinder:
      ImGui::DragFloat3("Center", dialog.cylinder_params.center, 1.0f);
      ImGui::DragFloat("Height", &dialog.cylinder_params.height, 1.0f, 1.0f, 10000.0f);
      ImGui::DragFloat("Radius", &dialog.cylinder_params.radius, 1.0f, 1.0f, 10000.0f);
      ImGui::SliderInt("Sides", &dialog.cylinder_params.sides, 3, 32);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Pyramid:
      ImGui::DragFloat3("Center", dialog.pyramid_params.center, 1.0f);
      ImGui::DragFloat("Height", &dialog.pyramid_params.height, 1.0f, 1.0f, 10000.0f);
      ImGui::DragFloat("Base Radius", &dialog.pyramid_params.radius, 1.0f, 1.0f, 10000.0f);
      ImGui::SliderInt("Sides", &dialog.pyramid_params.sides, 3, 32);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Sphere:
    case PrimitiveType::Dome:
      dialog.sphere_params.dome = (dialog.type == PrimitiveType::Dome);
      ImGui::DragFloat3("Center", dialog.sphere_params.center, 1.0f);
      ImGui::DragFloat("Radius", &dialog.sphere_params.radius, 1.0f, 1.0f, 10000.0f);
      ImGui::SliderInt("H Subdivisions", &dialog.sphere_params.horizontal_subdivisions, 4, 32);
      ImGui::SliderInt("V Subdivisions", &dialog.sphere_params.vertical_subdivisions, 2, 16);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

    case PrimitiveType::Plane:
      ImGui::DragFloat3("Center", dialog.plane_params.center, 1.0f);
      ImGui::DragFloat("Width", &dialog.plane_params.width, 1.0f, 1.0f, 10000.0f);
      ImGui::DragFloat("Height", &dialog.plane_params.height, 1.0f, 1.0f, 10000.0f);
      ImGui::DragFloat3("Normal", dialog.plane_params.normal, 0.01f, -1.0f, 1.0f);
      if (ImGui::Button("Create"))
      {
        create = true;
      }
      break;

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
        }
      }

      dialog.open = false;
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
    ToolbarResult toolbar_result = DrawToolbar(
      session.tools_panel,
      session.undo_stack.CanUndo(),
      session.undo_stack.CanRedo());
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
      session.primitive_dialog.open = true;
      session.primitive_dialog.type = toolbar_result.create_primitive;
    }

    ImGuiIO& io = ImGui::GetIO();
    bool trigger_undo = menu_actions.undo;
    bool trigger_redo = menu_actions.redo;
    if (!io.WantCaptureKeyboard)
    {
      // File operations: Cmd+N (New), Cmd+O (Open), Cmd+S (Save), Cmd+Shift+S (Save As)
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
      {
        menu_actions.new_world = true;
      }
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
      {
        menu_actions.open_world = true;
      }
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
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

      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
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
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
      {
        trigger_redo = true;
      }
      // Selection commands (only when Scene is active)
      if (session.active_target == SelectionTarget::Scene)
      {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
        {
          SelectAll(session.scene_panel, session.scene_nodes, session.scene_props);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
        {
          SelectNone(session.scene_panel);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_I, false))
        {
          SelectInverse(session.scene_panel, session.scene_nodes, session.scene_props);
        }
        // Visibility commands: H = Hide Selected, Shift+H = Unhide Selected,
        // Ctrl+H = Unhide All, Alt+H = Hide Unselected
        if (ImGui::IsKeyPressed(ImGuiKey_H, false))
        {
          if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt)
          {
            UnhideAllWithUndo(session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (io.KeyShift && !io.KeyCtrl && !io.KeyAlt)
          {
            UnhideSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (io.KeyAlt && !io.KeyCtrl && !io.KeyShift)
          {
            HideUnselectedWithUndo(session.scene_panel, session.scene_nodes,
                                   session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (!io.KeyCtrl && !io.KeyShift && !io.KeyAlt && HasSelection(session.scene_panel))
          {
            HideSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            ClearSelection(session.scene_panel);
            session.document_state.MarkDirty();
          }
        }
        // Freeze commands: F = Freeze Selected, Shift+F = Unfreeze Selected,
        // Ctrl+F = Unfreeze All
        if (ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
          if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt)
          {
            UnfreezeAllWithUndo(session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (io.KeyShift && !io.KeyCtrl && !io.KeyAlt)
          {
            UnfreezeSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            session.document_state.MarkDirty();
          }
          else if (!io.KeyCtrl && !io.KeyShift && !io.KeyAlt && HasSelection(session.scene_panel))
          {
            FreezeSelectedWithUndo(session.scene_panel, session.scene_props, &session.undo_stack);
            ClearSelection(session.scene_panel);
            session.document_state.MarkDirty();
          }
        }
        // Mirror commands: Ctrl+Shift+X/Y/Z
        if (io.KeyCtrl && io.KeyShift && HasSelection(session.scene_panel))
        {
          if (ImGui::IsKeyPressed(ImGuiKey_X, false))
          {
            MirrorSelection(session.scene_panel, session.scene_nodes, session.scene_props, MirrorAxis::X);
          }
          if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
          {
            MirrorSelection(session.scene_panel, session.scene_nodes, session.scene_props, MirrorAxis::Y);
          }
          // Note: Ctrl+Shift+Z is also Redo shortcut on some systems, so mirror Z may conflict
          // Users can still use the menu for Mirror Z
        }
        // Group commands: Ctrl+G = Group, Ctrl+Shift+G = Ungroup
        if (ImGui::IsKeyPressed(ImGuiKey_G, false))
        {
          if (io.KeyCtrl && io.KeyShift && !io.KeyAlt)
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
          else if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt && HasSelection(session.scene_panel))
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
      // Numpad 7: Top view (Ctrl+7: Bottom)
      if (ImGui::IsKeyPressed(ImGuiKey_Keypad7, false))
      {
        if (io.KeyCtrl)
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Bottom);
        }
        else
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Top);
        }
      }
      // Numpad 1: Front view (Ctrl+1: Back)
      if (ImGui::IsKeyPressed(ImGuiKey_Keypad1, false))
      {
        if (io.KeyCtrl)
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Back);
        }
        else
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Front);
        }
      }
      // Numpad 3: Right view (Ctrl+3: Left)
      if (ImGui::IsKeyPressed(ImGuiKey_Keypad3, false))
      {
        if (io.KeyCtrl)
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Left);
        }
        else
        {
          SetViewMode(session.viewport_panel(), ViewportPanelState::ViewMode::Right);
        }
      }
      // Tab cycles active viewport (multi-viewport layout)
      if (ImGui::IsKeyPressed(ImGuiKey_Tab, false) && !io.KeyCtrl && !io.KeyShift)
      {
        CycleActiveViewport(session.multi_viewport);
      }
      // F3: Open Go To Node dialog
      if (ImGui::IsKeyPressed(ImGuiKey_F3, false) && !session.scene_nodes.empty())
      {
        OpenGoToDialog(session.goto_dialog);
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
      session.primitive_dialog.open = true;
      session.primitive_dialog.type = menu_actions.create_primitive;
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
    if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A, false) && !io.KeyCtrl)
    {
      ImGui::OpenPopup("AddPrimitive");
    }
    PrimitiveType popup_primitive = PrimitiveType::None;
    DrawPrimitivePopup(popup_primitive);
    if (popup_primitive != PrimitiveType::None)
    {
      session.primitive_dialog.open = true;
      session.primitive_dialog.type = popup_primitive;
    }

    // Draw modal dialogs
    DrawPrimitiveDialog(session);
    DrawSavePromptDialog(session.save_prompt_dialog);
    DrawSelectionFilterDialog(session.selection_filter_dialog, session.selection_filter);
    DrawAdvancedSelectionDialog(
      session.advanced_selection_dialog,
      session.scene_panel,
      session.scene_nodes,
      session.scene_props);

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
      session.active_target);

    overlay_state = viewport_result.overlays;
    if (viewport_result.clicked_scene_id >= 0)
    {
      // Determine selection mode based on modifier keys
      SelectionMode sel_mode = SelectionMode::Replace;
      if (io.KeyShift && io.KeyCtrl)
      {
        sel_mode = SelectionMode::Remove;
      }
      else if (io.KeyShift)
      {
        sel_mode = SelectionMode::Add;
      }
      else if (io.KeyCtrl)
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
      DrawStatusBar(status_info);
    }

    diligent.imgui->Render(diligent.engine.context);
    diligent.engine.swapchain->Present();
  }
}
