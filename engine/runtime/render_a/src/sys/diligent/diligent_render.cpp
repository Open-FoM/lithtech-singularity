#include "bdefs.h"
#include "renderstruct.h"
#include "ltbasedefs.h"
#include "ltbasetypes.h"
#include "pixelformat.h"

#ifdef LTJS_SDL_BACKEND
#include "ltjs_main_window_descriptor.h"
#endif

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"

#ifndef __WINDOWS_H__
#include <windows.h>
#define __WINDOWS_H__
#endif

namespace
{
struct DiligentSurface
{
	uint32 width;
	uint32 height;
	uint32 pitch;
	uint32 data[1];
};

Diligent::RefCntAutoPtr<Diligent::IRenderDevice> g_render_device;
Diligent::RefCntAutoPtr<Diligent::IDeviceContext> g_immediate_context;
Diligent::RefCntAutoPtr<Diligent::ISwapChain> g_swap_chain;
RenderStruct* g_render_struct = nullptr;
uint32 g_screen_width = 0;
uint32 g_screen_height = 0;
bool g_is_in_3d = false;
bool g_is_in_optimized_2d = false;
Diligent::IEngineFactoryVk* g_engine_factory = nullptr;
HWND g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
SDL_Window* g_sdl_window = nullptr;
#endif

void diligent_UpdateWindowHandles(const RenderStructInit* init)
{
	g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_sdl_window = nullptr;
	if (!init)
	{
		return;
	}

	const auto* window_desc = static_cast<const ltjs::MainWindowDescriptor*>(init->m_hWnd);
	if (!window_desc)
	{
		return;
	}

	g_sdl_window = window_desc->sdl_window;
	g_native_window_handle = static_cast<HWND>(window_desc->native_handle);
#else
	if (init)
	{
		g_native_window_handle = static_cast<HWND>(init->m_hWnd);
	}
#endif
}

bool diligent_QueryWindowSize(uint32& width, uint32& height)
{
#ifdef LTJS_SDL_BACKEND
	if (!g_sdl_window)
	{
		return false;
	}

	int window_width = 0;
	int window_height = 0;
	SDL_GetWindowSize(g_sdl_window, &window_width, &window_height);
	if (window_width <= 0 || window_height <= 0)
	{
		return false;
	}

	width = static_cast<uint32>(window_width);
	height = static_cast<uint32>(window_height);
	return true;
#else
	if (!g_native_window_handle)
	{
		return false;
	}

	RECT rect{};
	if (!GetClientRect(g_native_window_handle, &rect))
	{
		return false;
	}

	const int window_width = rect.right - rect.left;
	const int window_height = rect.bottom - rect.top;
	if (window_width <= 0 || window_height <= 0)
	{
		return false;
	}

	width = static_cast<uint32>(window_width);
	height = static_cast<uint32>(window_height);
	return true;
#endif
}

void diligent_UpdateScreenSize(uint32 width, uint32 height)
{
	g_screen_width = width;
	g_screen_height = height;

	if (g_render_struct)
	{
		g_render_struct->m_Width = width;
		g_render_struct->m_Height = height;
	}
}

bool diligent_EnsureDevice()
{
	if (g_render_device && g_immediate_context)
	{
		return true;
	}

	if (!g_engine_factory)
	{
		g_engine_factory = Diligent::LoadAndGetEngineFactoryVk();
	}

	if (!g_engine_factory)
	{
		return false;
	}

	Diligent::EngineVkCreateInfo engine_create_info;
	g_engine_factory->CreateDeviceAndContextsVk(engine_create_info, &g_render_device, &g_immediate_context);
	return g_render_device && g_immediate_context;
}

bool diligent_CreateSwapChain(uint32 width, uint32 height)
{
	if (!g_engine_factory || !g_render_device || !g_immediate_context || !g_native_window_handle)
	{
		return false;
	}

	Diligent::SwapChainDesc swap_chain_desc;
	swap_chain_desc.Width = width;
	swap_chain_desc.Height = height;

	Diligent::Win32NativeWindow window{g_native_window_handle};
	g_engine_factory->CreateSwapChainVk(g_render_device, g_immediate_context, swap_chain_desc, window, &g_swap_chain);
	return static_cast<bool>(g_swap_chain);
}

bool diligent_EnsureSwapChain()
{
	if (!diligent_EnsureDevice())
	{
		return false;
	}

	uint32 width = g_screen_width;
	uint32 height = g_screen_height;
	if (!diligent_QueryWindowSize(width, height))
	{
		if (width == 0 || height == 0)
		{
			return false;
		}
	}

	if (!g_swap_chain)
	{
		if (!diligent_CreateSwapChain(width, height))
		{
			return false;
		}

		diligent_UpdateScreenSize(width, height);
		return true;
	}

	if (width != g_screen_width || height != g_screen_height)
	{
		g_swap_chain->Resize(width, height, Diligent::SURFACE_TRANSFORM_OPTIMAL);
		diligent_UpdateScreenSize(width, height);
	}

	return true;
}

HLTBUFFER diligent_CreateSurface(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return LTNULL;
	}

	const auto pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
	const auto surface_size = sizeof(DiligentSurface) + (pixel_count - 1) * sizeof(uint32);
	DiligentSurface* surface = nullptr;
	LT_MEM_TRACK_ALLOC(surface = static_cast<DiligentSurface*>(LTMemAlloc(surface_size)), LT_MEM_TYPE_RENDERER);
	if (!surface)
	{
		return LTNULL;
	}

	surface->width = static_cast<uint32>(width);
	surface->height = static_cast<uint32>(height);
	surface->pitch = static_cast<uint32>(width) * sizeof(uint32);
	memset(surface->data, 0, pixel_count * sizeof(uint32));
	return static_cast<HLTBUFFER>(surface);
}

void diligent_DeleteSurface(HLTBUFFER surface_handle)
{
	if (!surface_handle)
	{
		return;
	}

	LTMemFree(surface_handle);
}

void diligent_GetSurfaceInfo(HLTBUFFER surface_handle, uint32* width, uint32* height)
{
	const auto* surface = static_cast<const DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return;
	}

	if (width)
	{
		*width = surface->width;
	}

	if (height)
	{
		*height = surface->height;
	}
}

void* diligent_LockSurface(HLTBUFFER surface_handle, uint32& pitch)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		pitch = 0;
		return nullptr;
	}

	pitch = surface->pitch;
	return surface->data;
}

void diligent_UnlockSurface(HLTBUFFER)
{
}

bool diligent_OptimizeSurface(HLTBUFFER, uint32)
{
	return false;
}

void diligent_UnoptimizeSurface(HLTBUFFER)
{
}

bool diligent_GetScreenFormat(PFormat* format)
{
	if (!format)
	{
		return false;
	}

	format->Init(BPP_32, 0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu);
	return true;
}

void diligent_Clear(LTRect*, uint32, LTRGBColor& clear_color)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	const float clear_rgba[4] = {
		static_cast<float>(clear_color.rgb.r) / 255.0f,
		static_cast<float>(clear_color.rgb.g) / 255.0f,
		static_cast<float>(clear_color.rgb.b) / 255.0f,
		static_cast<float>(clear_color.rgb.a) / 255.0f};

	auto* render_target = g_swap_chain->GetCurrentBackBufferRTV();
	auto* depth_target = g_swap_chain->GetDepthBufferDSV();

	if (!render_target)
	{
		return;
	}

	g_immediate_context->ClearRenderTarget(
		render_target,
		clear_rgba,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (depth_target)
	{
		g_immediate_context->ClearDepthStencil(
			depth_target,
			Diligent::CLEAR_DEPTH_FLAG,
			1.0f,
			0,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}
}

bool diligent_Start3D()
{
	g_is_in_3d = true;
	if (g_render_struct)
	{
		++g_render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_End3D()
{
	g_is_in_3d = false;
	if (g_render_struct && g_render_struct->m_nIn3D > 0)
	{
		--g_render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_IsIn3D()
{
	return g_is_in_3d;
}

bool diligent_StartOptimized2D()
{
	g_is_in_optimized_2d = true;
	if (g_render_struct)
	{
		++g_render_struct->m_nInOptimized2D;
	}
	return true;
}

void diligent_EndOptimized2D()
{
	g_is_in_optimized_2d = false;
	if (g_render_struct && g_render_struct->m_nInOptimized2D > 0)
	{
		--g_render_struct->m_nInOptimized2D;
	}
}

bool diligent_IsInOptimized2D()
{
	return g_is_in_optimized_2d;
}

bool diligent_SetOptimized2DBlend(LTSurfaceBlend)
{
	return false;
}

bool diligent_GetOptimized2DBlend(LTSurfaceBlend&)
{
	return false;
}

bool diligent_SetOptimized2DColor(HLTCOLOR)
{
	return false;
}

bool diligent_GetOptimized2DColor(HLTCOLOR&)
{
	return false;
}

int diligent_RenderScene(SceneDesc*)
{
	return RENDER_OK;
}

void diligent_RenderCommand(int, const char**)
{
}

void diligent_SwapBuffers(uint32)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	g_swap_chain->Present(1);
}

bool diligent_LockScreen(int, int, int, int, void**, long*)
{
	return false;
}

void diligent_UnlockScreen()
{
}

void diligent_BlitToScreen(BlitRequest*)
{
}

void diligent_BlitFromScreen(BlitRequest*)
{
}

bool diligent_WarpToScreen(BlitRequest*)
{
	return false;
}

void diligent_MakeScreenShot(const char*)
{
}

void diligent_MakeCubicEnvMap(const char*, uint32, const SceneDesc&)
{
}

void diligent_ReadConsoleVariables()
{
}

void diligent_GetRenderInfo(RenderInfoStruct*)
{
}

HRENDERCONTEXT diligent_CreateContext()
{
	void* context = nullptr;
	LT_MEM_TRACK_ALLOC(context = LTMemAlloc(1), LT_MEM_TYPE_RENDERER);
	return static_cast<HRENDERCONTEXT>(context);
}

void diligent_DeleteContext(HRENDERCONTEXT context)
{
	if (context)
	{
		LTMemFree(context);
	}
}

void diligent_BindTexture(SharedTexture*, bool)
{
}

void diligent_UnbindTexture(SharedTexture*)
{
}

uint32 diligent_GetTextureFormat1(BPPIdent, uint32)
{
	return 0;
}

bool diligent_QueryDDSupport(PFormat*)
{
	return false;
}

bool diligent_GetTextureFormat2(BPPIdent, uint32, PFormat*)
{
	return false;
}

bool diligent_ConvertTexDataToDD(uint8*, PFormat*, uint32, uint32, uint8*, PFormat*, BPPIdent, uint32, uint32, uint32)
{
	return false;
}

void diligent_DrawPrimSetTexture(SharedTexture*)
{
}

void diligent_DrawPrimDisableTextures()
{
}

CRenderObject* diligent_CreateRenderObject(CRenderObject::RENDER_OBJECT_TYPES)
{
	return nullptr;
}

bool diligent_DestroyRenderObject(CRenderObject*)
{
	return false;
}

bool diligent_LoadWorldData(ILTStream*)
{
	return false;
}

bool diligent_SetLightGroupColor(uint32, const LTVector&)
{
	return false;
}

LTRESULT diligent_SetOccluderEnabled(uint32, bool)
{
	return LT_NOTFOUND;
}

LTRESULT diligent_GetOccluderEnabled(uint32, bool*)
{
	return LT_NOTFOUND;
}

uint32 diligent_GetTextureEffectVarID(const char*, uint32)
{
	return 0;
}

bool diligent_SetTextureEffectVar(uint32, uint32, float)
{
	return false;
}

bool diligent_IsObjectGroupEnabled(uint32)
{
	return false;
}

void diligent_SetObjectGroupEnabled(uint32, bool)
{
}

void diligent_SetAllObjectGroupEnabled()
{
}

bool diligent_AddGlowRenderStyleMapping(const char*, const char*)
{
	return false;
}

bool diligent_SetGlowDefaultRenderStyle(const char*)
{
	return false;
}

bool diligent_SetNoGlowRenderStyle(const char*)
{
	return false;
}

int diligent_Init(RenderStructInit* init)
{
	if (!init)
	{
		return RENDER_ERROR;
	}

	init->m_RendererVersion = LTRENDER_VERSION;
	g_screen_width = init->m_Mode.m_Width;
	g_screen_height = init->m_Mode.m_Height;

	diligent_UpdateWindowHandles(init);
	if (!g_native_window_handle)
	{
		return RENDER_ERROR;
	}

	if (!diligent_EnsureDevice())
	{
		return RENDER_ERROR;
	}

	diligent_UpdateScreenSize(g_screen_width, g_screen_height);
	if (!diligent_EnsureSwapChain())
	{
		return RENDER_ERROR;
	}

	return RENDER_OK;
}

void diligent_Term(bool)
{
	g_swap_chain.Release();
	g_immediate_context.Release();
	g_render_device.Release();
	g_engine_factory = nullptr;
	g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_sdl_window = nullptr;
#endif
	g_screen_width = 0;
	g_screen_height = 0;
	g_is_in_3d = false;
	g_is_in_optimized_2d = false;
}

#if !defined(LTJS_USE_DILIGENT_RENDER)
void* diligent_GetD3DDevice()
{
	return nullptr;
}
#endif

Diligent::IRenderDevice* diligent_GetRenderDevice()
{
	return g_render_device.RawPtr();
}

Diligent::IDeviceContext* diligent_GetImmediateContext()
{
	return g_immediate_context.RawPtr();
}

Diligent::ISwapChain* diligent_GetSwapChain()
{
	return g_swap_chain.RawPtr();
}

RMode* rdll_GetSupportedModes()
{
	RMode* mode = nullptr;
	LT_MEM_TRACK_ALLOC(mode = new RMode, LT_MEM_TYPE_RENDERER);
	if (!mode)
	{
		return nullptr;
	}

	LTStrCpy(mode->m_Description, "Diligent (Vulkan)", sizeof(mode->m_Description));
	LTStrCpy(mode->m_InternalName, "DiligentVulkan", sizeof(mode->m_InternalName));
	mode->m_Width = 1280;
	mode->m_Height = 720;
	mode->m_BitDepth = 32;
	mode->m_bHWTnL = true;
	mode->m_pNext = LTNULL;
	return mode;
}

void rdll_FreeModeList(RMode* modes)
{
	auto* current = modes;
	while (current)
	{
		auto* next = current->m_pNext;
		LTMemFree(current);
		current = next;
	}
}
}

void rdll_RenderDLLSetup(RenderStruct* render_struct)
{
	g_render_struct = render_struct;

	render_struct->Init = diligent_Init;
	render_struct->Term = diligent_Term;
#if !defined(LTJS_USE_DILIGENT_RENDER)
	render_struct->GetD3DDevice = diligent_GetD3DDevice;
#endif
	render_struct->GetRenderDevice = diligent_GetRenderDevice;
	render_struct->GetImmediateContext = diligent_GetImmediateContext;
	render_struct->GetSwapChain = diligent_GetSwapChain;
	render_struct->BindTexture = diligent_BindTexture;
	render_struct->UnbindTexture = diligent_UnbindTexture;
	render_struct->GetTextureDDFormat1 = diligent_GetTextureFormat1;
	render_struct->QueryDDSupport = diligent_QueryDDSupport;
	render_struct->GetTextureDDFormat2 = diligent_GetTextureFormat2;
	render_struct->ConvertTexDataToDD = diligent_ConvertTexDataToDD;
	render_struct->DrawPrimSetTexture = diligent_DrawPrimSetTexture;
	render_struct->DrawPrimDisableTextures = diligent_DrawPrimDisableTextures;
	render_struct->CreateContext = diligent_CreateContext;
	render_struct->DeleteContext = diligent_DeleteContext;
	render_struct->Clear = diligent_Clear;
	render_struct->Start3D = diligent_Start3D;
	render_struct->End3D = diligent_End3D;
	render_struct->IsIn3D = diligent_IsIn3D;
	render_struct->StartOptimized2D = diligent_StartOptimized2D;
	render_struct->EndOptimized2D = diligent_EndOptimized2D;
	render_struct->IsInOptimized2D = diligent_IsInOptimized2D;
	render_struct->SetOptimized2DBlend = diligent_SetOptimized2DBlend;
	render_struct->GetOptimized2DBlend = diligent_GetOptimized2DBlend;
	render_struct->SetOptimized2DColor = diligent_SetOptimized2DColor;
	render_struct->GetOptimized2DColor = diligent_GetOptimized2DColor;
	render_struct->RenderScene = diligent_RenderScene;
	render_struct->RenderCommand = diligent_RenderCommand;
	render_struct->SwapBuffers = diligent_SwapBuffers;
	render_struct->GetScreenFormat = diligent_GetScreenFormat;
	render_struct->CreateSurface = diligent_CreateSurface;
	render_struct->DeleteSurface = diligent_DeleteSurface;
	render_struct->GetSurfaceInfo = diligent_GetSurfaceInfo;
	render_struct->LockSurface = diligent_LockSurface;
	render_struct->UnlockSurface = diligent_UnlockSurface;
	render_struct->OptimizeSurface = diligent_OptimizeSurface;
	render_struct->UnoptimizeSurface = diligent_UnoptimizeSurface;
	render_struct->LockScreen = diligent_LockScreen;
	render_struct->UnlockScreen = diligent_UnlockScreen;
	render_struct->BlitToScreen = diligent_BlitToScreen;
	render_struct->BlitFromScreen = diligent_BlitFromScreen;
	render_struct->WarpToScreen = diligent_WarpToScreen;
	render_struct->MakeScreenShot = diligent_MakeScreenShot;
	render_struct->MakeCubicEnvMap = diligent_MakeCubicEnvMap;
	render_struct->ReadConsoleVariables = diligent_ReadConsoleVariables;
	render_struct->GetRenderInfo = diligent_GetRenderInfo;
	render_struct->CreateRenderObject = diligent_CreateRenderObject;
	render_struct->DestroyRenderObject = diligent_DestroyRenderObject;
	render_struct->LoadWorldData = diligent_LoadWorldData;
	render_struct->SetLightGroupColor = diligent_SetLightGroupColor;
	render_struct->SetOccluderEnabled = diligent_SetOccluderEnabled;
	render_struct->GetOccluderEnabled = diligent_GetOccluderEnabled;
	render_struct->GetTextureEffectVarID = diligent_GetTextureEffectVarID;
	render_struct->SetTextureEffectVar = diligent_SetTextureEffectVar;
	render_struct->IsObjectGroupEnabled = diligent_IsObjectGroupEnabled;
	render_struct->SetObjectGroupEnabled = diligent_SetObjectGroupEnabled;
	render_struct->SetAllObjectGroupEnabled = diligent_SetAllObjectGroupEnabled;
	render_struct->AddGlowRenderStyleMapping = diligent_AddGlowRenderStyleMapping;
	render_struct->SetGlowDefaultRenderStyle = diligent_SetGlowDefaultRenderStyle;
	render_struct->SetNoGlowRenderStyle = diligent_SetNoGlowRenderStyle;
}
