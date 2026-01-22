#ifndef LTJS_DILIGENT_STATE_H
#define LTJS_DILIGENT_STATE_H

#include "ltbasedefs.h"
#include "viewparams.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

namespace Diligent
{
	class IDeviceContext;
	class IEngineFactoryVk;
	class IRenderDevice;
	class ISwapChain;
	class ITextureView;
}

#if defined(_WIN32)
#ifndef __WINDOWS_H__
#include <windows.h>
#define __WINDOWS_H__
#endif
#else
using HWND = void*;
#endif

struct RenderStruct;
struct SceneDesc;
class ILTCommon;
class IWorldClientBSP;
class IWorldSharedBSP;

#ifdef LTJS_SDL_BACKEND
struct SDL_Window;
#endif

// Render state container; fields will be added incrementally during refactor.
struct DiligentRenderState
{
	RenderStruct* render_struct = nullptr;
	SceneDesc* scene_desc = nullptr;
	uint32 object_frame_code = 0;
	ViewParams view_params{};
	ILTCommon* common_client = nullptr;
	IWorldClientBSP* world_bsp_client = nullptr;
	IWorldSharedBSP* world_bsp_shared = nullptr;

	Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device;
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> immediate_context;
	Diligent::RefCntAutoPtr<Diligent::ISwapChain> swap_chain;

	Diligent::IEngineFactoryVk* engine_factory = nullptr;
	HWND native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	SDL_Window* sdl_window = nullptr;
#endif

	Diligent::ITextureView* output_render_target_override = nullptr;
	Diligent::ITextureView* output_depth_target_override = nullptr;

	uint32 screen_width = 0;
	uint32 screen_height = 0;
	bool is_in_3d = false;
	bool is_in_optimized_2d = false;
	bool glow_mode = false;
};

extern DiligentRenderState g_diligent_state;

#endif
