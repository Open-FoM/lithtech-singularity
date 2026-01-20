#pragma once

#include <string>

#include "texture_cache.h"

#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h"

struct RenderStruct;
struct RMode;
struct SDL_Window;

struct EngineRenderContext
{
	RenderStruct* render_struct = nullptr;
	Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
	Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapchain;
	TextureCache textures;
	std::string project_root;
	bool world_loaded = false;
};

bool InitEngineRenderer(SDL_Window* window, void* native_handle, EngineRenderContext& ctx, std::string& error);
void ShutdownEngineRenderer(EngineRenderContext& ctx);
void SetEngineProjectRoot(EngineRenderContext& ctx, const std::string& root);
bool LoadRenderWorld(EngineRenderContext& ctx, const std::string& world_path, std::string& error);
TextureCache* GetEngineTextureCache();
