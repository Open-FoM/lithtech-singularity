#pragma once

#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"

#include "ImGuiImplSDL3.hpp"

#include "de_objects.h"
#include "engine_render.h"
#include "viewport_render.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct DynamicLight;
struct NodeProperties;
struct ViewportPanelState;
struct ViewportOverlayState;
struct SDL_Window;

struct ViewportRenderResources
{
  Diligent::RefCntAutoPtr<Diligent::ITexture> color;
  Diligent::RefCntAutoPtr<Diligent::ITextureView> color_rtv;
  Diligent::RefCntAutoPtr<Diligent::ITextureView> color_srv;
  Diligent::RefCntAutoPtr<Diligent::ITexture> depth;
  Diligent::RefCntAutoPtr<Diligent::ITextureView> depth_dsv;
  uint32_t width = 0;
  uint32_t height = 0;
};

struct DiligentContext
{
	EngineRenderContext engine;
	std::unique_ptr<Diligent::ImGuiImplSDL3> imgui;
	void* metal_view = nullptr;
	ViewportRenderResources viewport;
	bool viewport_visible = false;
	ViewportGridRenderer grid_renderer;
	bool grid_ready = false;
	std::vector<std::unique_ptr<WorldModelInstance>> sky_world_models;
	std::vector<LTObject*> sky_objects;
	std::vector<std::string> sky_world_model_names;

	~DiligentContext();
};

bool InitDiligent(SDL_Window* window, DiligentContext& ctx);
bool CreateViewportTargets(DiligentContext& ctx, uint32_t width, uint32_t height);
void RenderViewport(
  DiligentContext& ctx,
  const ViewportPanelState& viewport_state,
  const NodeProperties* world_props,
  const ViewportOverlayState& overlay_state,
  std::vector<DynamicLight>& dynamic_lights);
