#pragma once

#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"

#include "ImGuiImplSDL3.hpp"

#include "de_objects.h"
#include "engine_render.h"
#include "viewport_render.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct DynamicLight;
struct NodeProperties;
struct ViewportPanelState;
struct ViewportOverlayState;
struct SDL_Window;

/// Maximum number of simultaneous viewport render targets.
constexpr int kMaxViewportRenderSlots = 4;

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

	/// Per-viewport render targets (one per visible viewport slot).
	std::array<ViewportRenderResources, kMaxViewportRenderSlots> viewports;
	/// Per-viewport visibility flags.
	std::array<bool, kMaxViewportRenderSlots> viewport_visible{};

	ViewportGridRenderer grid_renderer;
	bool grid_ready = false;
	std::vector<std::unique_ptr<WorldModelInstance>> sky_world_models;
	std::vector<LTObject*> sky_objects;
	std::vector<std::string> sky_world_model_names;

	~DiligentContext();
};

bool InitDiligent(SDL_Window* window, DiligentContext& ctx);

/// Create/resize render targets for a specific viewport slot.
bool CreateViewportTargets(DiligentContext& ctx, int slot, uint32_t width, uint32_t height);

/// Render scene to a specific viewport slot's render target.
void RenderViewport(
  DiligentContext& ctx,
  int slot,
  const ViewportPanelState& viewport_state,
  const NodeProperties* world_props,
  const ViewportOverlayState& overlay_state,
  std::vector<DynamicLight>& dynamic_lights);
