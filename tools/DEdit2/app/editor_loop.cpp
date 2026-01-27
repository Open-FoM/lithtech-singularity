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
      session.undo_stack.CanRedo());

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
    }
    if (trigger_undo)
    {
      session.undo_stack.Undo(session.project_nodes, session.scene_nodes);
    }
    if (trigger_redo)
    {
      session.undo_stack.Redo(session.project_nodes, session.scene_nodes);
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
        session.viewport_panel,
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
        session.viewport_panel,
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
      session.scene_panel.selected_id,
      session.project_root);

    DrawConsolePanel(session.console_panel);

    ViewportPanelResult viewport_result = DrawViewportPanel(
      diligent,
      session.viewport_panel,
      session.scene_panel,
      session.scene_nodes,
      session.scene_props,
      session.active_target);

    overlay_state = viewport_result.overlays;
    if (viewport_result.clicked_scene_id >= 0)
    {
      session.scene_panel.selected_id = viewport_result.clicked_scene_id;
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
      session.viewport_panel,
      session.scene_panel,
      session.scene_nodes,
      session.scene_props);
    BuildViewportDynamicLights(
      session.viewport_panel,
      session.scene_panel,
      session.scene_nodes,
      session.scene_props,
      viewport_dynamic_lights);
    ApplySceneVisibilityToRenderer(
      session.scene_panel,
      session.viewport_panel,
      session.scene_nodes,
      session.scene_props);
    RenderViewport(diligent, session.viewport_panel, world_props, overlay_state, viewport_dynamic_lights);

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
