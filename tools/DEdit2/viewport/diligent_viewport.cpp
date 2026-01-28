#include "viewport/diligent_viewport.h"

#include "viewport/overlays.h"

#include "diligent_render.h"
#include "diligent_render_debug.h"
#include "engine_render.h"
#include "editor_state.h"
#include "platform_macos.h"
#include "rendererconsolevars.h"
#include "ui_viewport.h"
#include "viewport_picking.h"

#include "imgui.h"
#include "ltbasetypes.h"
#include "ltrotation.h"
#include "ltvector.h"
#include "renderstruct.h"

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <dlfcn.h>
#include <string>
#include <vector>

namespace
{
bool EnsureVulkanLoaderAvailable()
{
  void* handle = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
  if (handle == nullptr)
  {
    std::fprintf(
      stderr,
      "Vulkan loader (libvulkan.dylib) not found. Install via Homebrew "
      "(brew install vulkan-loader molten-vk) and ensure /opt/homebrew/lib is "
      "on the loader path or set VULKAN_SDK=/opt/homebrew.\n");
    return false;
  }

  dlclose(handle);
  return true;
}
}

DiligentContext::~DiligentContext() = default;

bool InitDiligent(SDL_Window* window, DiligentContext& ctx)
{
#if defined(__APPLE__)
  if (!EnsureVulkanLoaderAvailable())
  {
    return false;
  }
#endif

  ctx.metal_view = CreateMetalView(window);
  if (ctx.metal_view == nullptr)
  {
    std::fprintf(stderr, "Diligent: failed to create Metal view for SDL window.\n");
    return false;
  }

  std::string error;
  if (!InitEngineRenderer(window, ctx.metal_view, ctx.engine, error))
  {
    std::fprintf(stderr, "Diligent: %s\n", error.c_str());
    DestroyMetalView(ctx.metal_view);
    ctx.metal_view = nullptr;
    return false;
  }

  Diligent::ImGuiDiligentCreateInfo imgui_ci{ctx.engine.device, ctx.engine.swapchain->GetDesc()};
  ctx.imgui = Diligent::ImGuiImplSDL3::Create(imgui_ci, window);
  if (!ctx.imgui)
  {
    std::fprintf(stderr, "Diligent: failed to create ImGui renderer.\n");
    ShutdownEngineRenderer(ctx.engine);
    DestroyMetalView(ctx.metal_view);
    ctx.metal_view = nullptr;
    return false;
  }

  return true;
}

bool CreateViewportTargets(DiligentContext& ctx, int slot, uint32_t width, uint32_t height)
{
  if (slot < 0 || slot >= kMaxViewportRenderSlots)
  {
    return false;
  }
  if (width == 0 || height == 0)
  {
    return false;
  }

  ViewportRenderResources& vp = ctx.viewports[slot];
  if (vp.width == width && vp.height == height && vp.color && vp.depth)
  {
    return true;
  }

  vp = {};
  vp.width = width;
  vp.height = height;

  // Create unique names for each viewport slot's textures
  char color_name[64];
  char depth_name[64];
  snprintf(color_name, sizeof(color_name), "DEdit2 Viewport %d Color", slot);
  snprintf(depth_name, sizeof(depth_name), "DEdit2 Viewport %d Depth", slot);

  Diligent::TextureDesc color_desc;
  color_desc.Name = color_name;
  color_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
  color_desc.Width = width;
  color_desc.Height = height;
  color_desc.MipLevels = 1;
  color_desc.ArraySize = 1;
  color_desc.Format = ctx.engine.swapchain
    ? ctx.engine.swapchain->GetDesc().ColorBufferFormat
    : Diligent::TEX_FORMAT_RGBA8_UNORM;
  color_desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
  ctx.engine.device->CreateTexture(color_desc, nullptr, &vp.color);
  if (!vp.color)
  {
    return false;
  }

  vp.color_rtv = vp.color->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  vp.color_srv = vp.color->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

  Diligent::TextureDesc depth_desc;
  depth_desc.Name = depth_name;
  depth_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
  depth_desc.Width = width;
  depth_desc.Height = height;
  depth_desc.MipLevels = 1;
  depth_desc.ArraySize = 1;
  depth_desc.Format = Diligent::TEX_FORMAT_D32_FLOAT;
  depth_desc.BindFlags = Diligent::BIND_DEPTH_STENCIL;
  ctx.engine.device->CreateTexture(depth_desc, nullptr, &vp.depth);
  if (!vp.depth)
  {
    return false;
  }

  vp.depth_dsv = vp.depth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
  return true;
}

void RenderViewport(
  DiligentContext& ctx,
  int slot,
  const ViewportPanelState& viewport_state,
  const NodeProperties* world_props,
  const ViewportOverlayState& overlay_state,
  std::vector<DynamicLight>& dynamic_lights)
{
  if (slot < 0 || slot >= kMaxViewportRenderSlots)
  {
    return;
  }

  const ViewportRenderResources& vp = ctx.viewports[slot];
  if (!ctx.viewport_visible[slot] || !vp.color_rtv || !vp.depth_dsv || !ctx.engine.context)
  {
    return;
  }

  Diligent::ITextureView* render_target = vp.color_rtv;
  Diligent::ITextureView* depth_target = vp.depth_dsv;
  diligent_SetOutputTargets(render_target, depth_target);

  ctx.engine.context->SetRenderTargets(
    1,
    &render_target,
    depth_target,
    Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  float clear_color[] = {0.08f, 0.10f, 0.14f, 1.0f};
  if (world_props)
  {
    clear_color[0] = world_props->background_color[0];
    clear_color[1] = world_props->background_color[1];
    clear_color[2] = world_props->background_color[2];
  }
  if (ctx.engine.render_struct && ctx.engine.render_struct->Clear)
  {
    const auto to_u8 = [](float value) -> uint8
    {
      const float clamped = std::clamp(value, 0.0f, 1.0f);
      return static_cast<uint8>(clamped * 255.0f + 0.5f);
    };

    LTRGBColor clear_color_ltrgb{};
    clear_color_ltrgb.rgb.r = to_u8(clear_color[0]);
    clear_color_ltrgb.rgb.g = to_u8(clear_color[1]);
    clear_color_ltrgb.rgb.b = to_u8(clear_color[2]);
    clear_color_ltrgb.rgb.a = to_u8(clear_color[3]);
    ctx.engine.render_struct->Clear(nullptr, 0, clear_color_ltrgb);
  }
  else
  {
    ctx.engine.context->ClearRenderTarget(render_target, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    ctx.engine.context->ClearDepthStencil(
      depth_target,
      Diligent::CLEAR_DEPTH_FLAG,
      1.0f,
      0,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }

  if (ctx.engine.world_loaded && ctx.engine.render_struct && ctx.engine.render_struct->RenderScene)
  {
    const float aspect = vp.height > 0 ? static_cast<float>(vp.width) /
      static_cast<float>(vp.height) : 1.0f;

    Diligent::float3 cam_pos{};
    Diligent::float3 cam_forward{};
    Diligent::float3 cam_right{};
    Diligent::float3 cam_up{};
    ComputeCameraBasis(viewport_state, cam_pos, cam_forward, cam_right, cam_up);

    // Calculate FOV - use very small FOV for ortho simulation
    float fov_y = Diligent::PI_F / 4.0f;  // Default perspective FOV (45 degrees)
    if (IsOrthographicView(viewport_state))
    {
      // Simulate orthographic with very small FOV and distant camera.
      // The camera is already positioned far away by ComputeCameraBasis (10000 units).
      // Using a tiny FOV (~0.06 degrees) gives nearly parallel rays.
      // The visible half-height at 10000 units with FOV gives us ortho_zoom scaling.
      const float ortho_half_height = viewport_state.ortho_zoom * 400.0f;
      const float cam_dist = 10000.0f;
      // FOV = 2 * atan(half_height / distance)
      fov_y = 2.0f * std::atan(ortho_half_height / cam_dist);
    }
    const float fov_x = 2.0f * std::atan(std::tan(fov_y * 0.5f) * aspect);

    SceneDesc scene{};
    scene.m_FrameTime = ImGui::GetIO().DeltaTime;
    scene.m_fActualFrameTime = scene.m_FrameTime;
    scene.m_GlobalLightScale.Init(1.0f, 1.0f, 1.0f);
    scene.m_GlobalLightAdd.Init(0.0f, 0.0f, 0.0f);
    scene.m_GlobalModelLightAdd.Init(0.0f, 0.0f, 0.0f);
    scene.m_Rect.left = 0;
    scene.m_Rect.top = 0;
    scene.m_Rect.right = static_cast<int>(vp.width);
    scene.m_Rect.bottom = static_cast<int>(vp.height);
    scene.m_xFov = fov_x;
    scene.m_yFov = fov_y;
    scene.m_Pos.Init(cam_pos.x, cam_pos.y, cam_pos.z);
    scene.m_Rotation = LTRotation(
      LTVector(cam_forward.x, cam_forward.y, cam_forward.z),
      LTVector(cam_up.x, cam_up.y, cam_up.z));
    scene.m_SkyObjects = ctx.sky_objects.empty() ? nullptr : ctx.sky_objects.data();
    scene.m_nSkyObjects = static_cast<int>(ctx.sky_objects.size());

    std::vector<LTObject*> object_list;
    if (!dynamic_lights.empty())
    {
      object_list.reserve(dynamic_lights.size());
      for (DynamicLight& light : dynamic_lights)
      {
        object_list.push_back(static_cast<LTObject*>(&light));
      }
    }

    if (!object_list.empty())
    {
      scene.m_DrawMode = DRAWMODE_OBJECTLIST;
      scene.m_pObjectList = object_list.data();
      scene.m_ObjectListSize = static_cast<int>(object_list.size());
    }
    else
    {
      scene.m_DrawMode = DRAWMODE_NORMAL;
      scene.m_pObjectList = nullptr;
      scene.m_ObjectListSize = 0;
    }

    if (ctx.engine.render_struct->Start3D)
    {
      ctx.engine.render_struct->Start3D();
    }
    ctx.engine.render_struct->RenderScene(&scene);
    if (ctx.engine.render_struct->End3D)
    {
      ctx.engine.render_struct->End3D();
    }
  }

  // Ensure editor overlays render to the viewport targets even if the renderer
  // temporarily bound offscreen targets for post-processing.
  diligent_SetOutputTargets(render_target, depth_target);
  ctx.engine.context->SetRenderTargets(
    1,
    &render_target,
    depth_target,
    Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  if (ctx.grid_ready)
  {
    const float grid_aspect = vp.height > 0 ? static_cast<float>(vp.width) /
      static_cast<float>(vp.height) : 1.0f;
    const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_state, grid_aspect);
    const Diligent::float3 grid_origin{
      viewport_state.grid_origin[0],
      viewport_state.grid_origin[1],
      viewport_state.grid_origin[2]};
    UpdateViewportGridConstants(ctx.engine.context, ctx.grid_renderer, view_proj, grid_origin);

    Diligent::Viewport diligent_vp;
    diligent_vp.TopLeftX = 0.0f;
    diligent_vp.TopLeftY = 0.0f;
    diligent_vp.Width = static_cast<float>(vp.width);
    diligent_vp.Height = static_cast<float>(vp.height);
    diligent_vp.MinDepth = 0.0f;
    diligent_vp.MaxDepth = 1.0f;
    ctx.engine.context->SetViewports(1, &diligent_vp, 0, 0);

    DrawViewportGrid(ctx.engine.context, ctx.grid_renderer, viewport_state.show_grid, viewport_state.show_axes);
  }

  if (ctx.engine.render_struct && ctx.engine.render_struct->m_bInitted)
  {
    RenderViewportOverlays(overlay_state);
  }

  diligent_SetOutputTargets(nullptr, nullptr);
  ctx.engine.context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
}
