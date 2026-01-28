#include "app/editor_loop.h"

#include "app/editor_paths.h"
#include "app/editor_session.h"
#include "app/project_utils.h"
#include "app/recent_projects.h"
#include "app/scene_loader.h"
#include "ui/viewport_panel.h"
#include "viewport/diligent_viewport.h"
#include "viewport/lighting.h"
#include "viewport/render_settings.h"
#include "viewport/world_settings.h"

#include "transform/mirror.h"
#include "ui_console.h"
#include "ui_dock.h"
#include "ui_project.h"
#include "ui_properties.h"
#include "ui_scene.h"
#include "ui_shared.h"
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
    DrawMainMenuBar(
      request_reset_layout,
      menu_actions,
      session.recent_projects,
      session.undo_stack.CanUndo(),
      session.undo_stack.CanRedo(),
      &session.tools_panel.visible);

    ImGuiIO& io = ImGui::GetIO();
    bool trigger_undo = menu_actions.undo;
    bool trigger_redo = menu_actions.redo;
    if (!io.WantCaptureKeyboard)
    {
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
        // Visibility commands: H = Hide Selected, Shift+H = Unhide All, Alt+H = Hide Unselected
        if (ImGui::IsKeyPressed(ImGuiKey_H, false))
        {
          if (io.KeyShift)
          {
            UnhideAll(session.scene_props);
          }
          else if (io.KeyAlt)
          {
            HideUnselected(session.scene_panel, session.scene_nodes, session.scene_props);
          }
          else if (HasSelection(session.scene_panel))
          {
            HideSelected(session.scene_panel, session.scene_props);
            ClearSelection(session.scene_panel);
          }
        }
        // Freeze commands: F = Freeze Selected, Shift+F = Unfreeze All
        if (ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
          if (io.KeyShift)
          {
            UnfreezeAll(session.scene_props);
          }
          else if (HasSelection(session.scene_panel))
          {
            FreezeSelected(session.scene_panel, session.scene_props);
            ClearSelection(session.scene_panel);
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
    }
    if (trigger_undo)
    {
      session.undo_stack.Undo(session.project_nodes, session.scene_nodes);
    }
    if (trigger_redo)
    {
      session.undo_stack.Redo(session.project_nodes, session.scene_nodes);
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

    ProjectContextAction project_action{};
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
    DrawWorldBrowserPanel(session.world_browser, session.project_root, world_action);
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
    DrawScenePanel(session.scene_panel, session.scene_nodes, session.scene_props, session.active_target, &session.undo_stack);
    if (session.scene_panel.error != session.scene_error)
    {
      session.scene_error = session.scene_panel.error;
    }
    DrawPropertiesPanel(
      session.active_target,
      session.project_nodes,
      session.project_props,
      session.project_panel.selected_id,
      session.scene_nodes,
      session.scene_props,
      session.scene_panel.primary_selection,
      session.project_root);

    DrawConsolePanel(session.console_panel);

    // Draw tools panel and handle tool selection
    ToolsPanelResult tools_result = DrawToolsPanel(session.tools_panel);
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

    ViewportPanelResult viewport_result = DrawViewportPanel(
      diligent,
      session.multi_viewport,
      session.scene_panel,
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
    if (back_dsv)
    {
      diligent.engine.context->ClearDepthStencil(
        back_dsv,
        Diligent::CLEAR_DEPTH_FLAG,
        1.0f,
        0,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    diligent.imgui->Render(diligent.engine.context);
    diligent.engine.swapchain->Present();
  }
}
