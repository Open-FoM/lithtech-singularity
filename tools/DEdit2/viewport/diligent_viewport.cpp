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

bool CreateViewportTargets(DiligentContext& ctx, uint32_t width, uint32_t height)
{
  if (width == 0 || height == 0)
  {
    return false;
  }
  if (ctx.viewport.width == width && ctx.viewport.height == height &&
    ctx.viewport.color && ctx.viewport.depth)
  {
    return true;
  }

  ctx.viewport = {};
  ctx.viewport.width = width;
  ctx.viewport.height = height;

  Diligent::TextureDesc color_desc;
  color_desc.Name = "DEdit2 Viewport Color";
  color_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
  color_desc.Width = width;
  color_desc.Height = height;
  color_desc.MipLevels = 1;
  color_desc.ArraySize = 1;
  color_desc.Format = ctx.engine.swapchain
    ? ctx.engine.swapchain->GetDesc().ColorBufferFormat
    : Diligent::TEX_FORMAT_RGBA8_UNORM;
  color_desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
  ctx.engine.device->CreateTexture(color_desc, nullptr, &ctx.viewport.color);
  if (!ctx.viewport.color)
  {
    return false;
  }

  ctx.viewport.color_rtv = ctx.viewport.color->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
  ctx.viewport.color_srv = ctx.viewport.color->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

  Diligent::TextureDesc depth_desc;
  depth_desc.Name = "DEdit2 Viewport Depth";
  depth_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
  depth_desc.Width = width;
  depth_desc.Height = height;
  depth_desc.MipLevels = 1;
  depth_desc.ArraySize = 1;
  depth_desc.Format = Diligent::TEX_FORMAT_D32_FLOAT;
  depth_desc.BindFlags = Diligent::BIND_DEPTH_STENCIL;
  ctx.engine.device->CreateTexture(depth_desc, nullptr, &ctx.viewport.depth);
  if (!ctx.viewport.depth)
  {
    return false;
  }

  ctx.viewport.depth_dsv = ctx.viewport.depth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
  return true;
}

void RenderViewport(
  DiligentContext& ctx,
  const ViewportPanelState& viewport_state,
  const NodeProperties* world_props,
  const ViewportOverlayState& overlay_state,
  std::vector<DynamicLight>& dynamic_lights)
{
  if (!ctx.viewport_visible || !ctx.viewport.color_rtv || !ctx.viewport.depth_dsv || !ctx.engine.context)
  {
    return;
  }

  Diligent::ITextureView* render_target = ctx.viewport.color_rtv;
  Diligent::ITextureView* depth_target = ctx.viewport.depth_dsv;
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
    const float aspect = ctx.viewport.height > 0 ? static_cast<float>(ctx.viewport.width) /
      static_cast<float>(ctx.viewport.height) : 1.0f;
    const float fov_y = Diligent::PI_F / 4.0f;
    const float fov_x = 2.0f * std::atan(std::tan(fov_y * 0.5f) * aspect);

    Diligent::float3 cam_pos{};
    Diligent::float3 cam_forward{};
    Diligent::float3 cam_right{};
    Diligent::float3 cam_up{};
    ComputeCameraBasis(viewport_state, cam_pos, cam_forward, cam_right, cam_up);

    SceneDesc scene{};
    scene.m_FrameTime = ImGui::GetIO().DeltaTime;
    scene.m_fActualFrameTime = scene.m_FrameTime;
    scene.m_GlobalLightScale.Init(1.0f, 1.0f, 1.0f);
    scene.m_GlobalLightAdd.Init(0.0f, 0.0f, 0.0f);
    scene.m_GlobalModelLightAdd.Init(0.0f, 0.0f, 0.0f);
    scene.m_Rect.left = 0;
    scene.m_Rect.top = 0;
    scene.m_Rect.right = static_cast<int>(ctx.viewport.width);
    scene.m_Rect.bottom = static_cast<int>(ctx.viewport.height);
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
    const float aspect = ctx.viewport.height > 0 ? static_cast<float>(ctx.viewport.width) /
      static_cast<float>(ctx.viewport.height) : 1.0f;
    const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_state, aspect);
    const Diligent::float3 grid_origin{
      viewport_state.grid_origin[0],
      viewport_state.grid_origin[1],
      viewport_state.grid_origin[2]};
    UpdateViewportGridConstants(ctx.engine.context, ctx.grid_renderer, view_proj, grid_origin);

    Diligent::Viewport vp;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(ctx.viewport.width);
    vp.Height = static_cast<float>(ctx.viewport.height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx.engine.context->SetViewports(1, &vp, 0, 0);

    DrawViewportGrid(ctx.engine.context, ctx.grid_renderer, viewport_state.show_grid, viewport_state.show_axes);
  }

  if (ctx.engine.render_struct && ctx.engine.render_struct->m_bInitted)
  {
    RenderViewportOverlays(overlay_state);
  }

  diligent_SetOutputTargets(nullptr, nullptr);
  ctx.engine.context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
}
