#include "app/editor_loop.h"
#include "app/editor_paths.h"
#include "app/editor_session.h"
#include "multi_viewport.h"
#include "app/project_utils.h"
#include "app/recent_projects.h"
#include "app/scene_loader.h"
#include "ui_scene.h"
#include "viewport/diligent_viewport.h"

#include "dedit2_concommand.h"
#include "editor_transfer.h"
#include "platform_macos.h"

#include "imgui.h"

#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"

#include <SDL3/SDL.h>

#include <cstdio>

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
  {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  RegisterEditorTransferFormat();
  DEdit2_InitConsoleCommands();

  const float display_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_WindowFlags window_flags =
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
#if defined(__APPLE__)
  window_flags |= SDL_WINDOW_METAL;
#else
  window_flags |= SDL_WINDOW_VULKAN;
#endif
  SDL_Window* window = SDL_CreateWindow(
    "DEdit2 (SDL/ImGui)",
    static_cast<int>(1280 * display_scale),
    static_cast<int>(800 * display_scale),
    window_flags);
  if (window == nullptr)
  {
    std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(display_scale);
  style.FontScaleDpi = display_scale;

  EditorPaths paths = ParseEditorPaths(argc, argv);
  EditorSession session;
  InitMultiViewport(session.multi_viewport);
  session.project_root = ResolveProjectRoot(paths.project_root);
  session.world_file = paths.world_file;

  LoadRecentProjects(session.recent_projects);
  PruneRecentProjects(session.recent_projects);

  DiligentContext diligent{};
  if (!InitDiligent(window, diligent))
  {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  diligent.grid_ready = InitViewportGridRenderer(
    diligent.engine.device,
    diligent.engine.swapchain ? diligent.engine.swapchain->GetDesc().ColorBufferFormat
      : Diligent::TEX_FORMAT_RGBA8_UNORM,
    Diligent::TEX_FORMAT_D32_FLOAT,
    diligent.grid_renderer);
  diligent.marker_ready = InitMarkerRenderer(
    diligent.engine.device,
    diligent.engine.swapchain ? diligent.engine.swapchain->GetDesc().ColorBufferFormat
      : Diligent::TEX_FORMAT_RGBA8_UNORM,
    Diligent::TEX_FORMAT_D32_FLOAT,
    diligent.marker_renderer);
  InitBrushRenderer(
    diligent.engine.device,
    diligent.engine.swapchain ? diligent.engine.swapchain->GetDesc().ColorBufferFormat
      : Diligent::TEX_FORMAT_RGBA8_UNORM,
    Diligent::TEX_FORMAT_D32_FLOAT,
    diligent.brush_renderer);
  SetEngineProjectRoot(diligent.engine, session.project_root);

  if (!session.project_root.empty())
  {
    session.project_nodes = BuildProjectTree(session.project_root, session.project_props, session.project_error);
  }
  session.project_panel.error = session.project_error;
  session.project_panel.selected_id = DefaultProjectSelection(session.project_nodes);
  session.scene_panel.error = session.scene_error;
  SelectNode(session.scene_panel, session.scene_nodes.empty() ? -1 : 0);
  session.active_target = SelectionTarget::Project;

  DEdit2_SetConsoleBindings({
    &session.project_nodes,
    &session.project_props,
    &session.scene_nodes,
    &session.scene_props});

  if (!session.world_file.empty())
  {
    LoadSceneWorld(
      session.world_file,
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

  RunEditorLoop(window, diligent, session);

  diligent.imgui.reset();
  ShutdownEngineRenderer(diligent.engine);
  if (diligent.metal_view)
  {
    DestroyMetalView(diligent.metal_view);
    diligent.metal_view = nullptr;
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
  DEdit2_TermConsoleCommands();

  return 0;
}
