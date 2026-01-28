#include "diligent_device.h"
#include "diligent_state.h"

#include "bdefs.h"

#include "renderstruct.h"

#ifdef LTJS_SDL_BACKEND
#include "ltjs_main_window_descriptor.h"
#endif

#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "Platforms/interface/NativeWindow.h"

namespace
{

bool diligent_is_srgb_format(Diligent::TEXTURE_FORMAT format)
{
	switch (format)
	{
		case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
		case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
		case Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB:
			return true;
		default:
			return false;
	}
}

} // namespace

Diligent::ITextureView* diligent_get_active_render_target()
{
	if (g_diligent_state.output_render_target_override)
	{
		return g_diligent_state.output_render_target_override;
	}

	return g_diligent_state.swap_chain ? g_diligent_state.swap_chain->GetCurrentBackBufferRTV() : nullptr;
}

Diligent::ITextureView* diligent_get_active_depth_target()
{
	if (g_diligent_state.output_depth_target_override)
	{
		return g_diligent_state.output_depth_target_override;
	}

	return g_diligent_state.swap_chain ? g_diligent_state.swap_chain->GetDepthBufferDSV() : nullptr;
}

uint32 diligent_get_active_sample_count()
{
	auto* render_target = diligent_get_active_render_target();
	if (!render_target)
	{
		return 1;
	}

	auto* texture = render_target->GetTexture();
	if (!texture)
	{
		return 1;
	}

	const uint32 samples = texture->GetDesc().SampleCount;
	return samples > 0 ? samples : 1;
}

float diligent_get_swapchain_output_is_srgb()
{
	if (!g_diligent_state.swap_chain)
	{
		return 0.0f;
	}

	return diligent_is_srgb_format(g_diligent_state.swap_chain->GetDesc().ColorBufferFormat) ? 1.0f : 0.0f;
}

void diligent_UpdateWindowHandles(const RenderStructInit* init)
{
	g_diligent_state.native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_diligent_state.sdl_window = nullptr;
	if (!init)
	{
		return;
	}

	const auto* window_desc = static_cast<const ltjs::MainWindowDescriptor*>(init->m_hWnd);
	if (!window_desc)
	{
		return;
	}

	g_diligent_state.sdl_window = window_desc->sdl_window;
	g_diligent_state.native_window_handle = static_cast<HWND>(window_desc->native_handle);
#else
	if (init)
	{
		g_diligent_state.native_window_handle = static_cast<HWND>(init->m_hWnd);
	}
#endif
}

bool diligent_QueryWindowSize(uint32& width, uint32& height)
{
#ifdef LTJS_SDL_BACKEND
	if (!g_diligent_state.sdl_window)
	{
		return false;
	}

	int window_width = 0;
	int window_height = 0;
	SDL_GetWindowSizeInPixels(g_diligent_state.sdl_window, &window_width, &window_height);
	if (window_width <= 0 || window_height <= 0)
	{
		SDL_GetWindowSize(g_diligent_state.sdl_window, &window_width, &window_height);
	}
	if (window_width <= 0 || window_height <= 0)
	{
		return false;
	}

	width = static_cast<uint32>(window_width);
	height = static_cast<uint32>(window_height);
	return true;
#else
	if (!g_diligent_state.native_window_handle)
	{
		return false;
	}

	RECT rect{};
	if (!GetClientRect(g_diligent_state.native_window_handle, &rect))
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
	g_diligent_state.screen_width = width;
	g_diligent_state.screen_height = height;

	if (g_diligent_state.render_struct)
	{
		g_diligent_state.render_struct->m_Width = width;
		g_diligent_state.render_struct->m_Height = height;
	}
}

bool diligent_EnsureDevice()
{
	if (g_diligent_state.render_device && g_diligent_state.immediate_context)
	{
		return true;
	}

	if (!g_diligent_state.engine_factory)
	{
		g_diligent_state.engine_factory = Diligent::LoadAndGetEngineFactoryVk();
	}

	if (!g_diligent_state.engine_factory)
	{
		dsi_ConsolePrint("Diligent: failed to load Vulkan engine factory.");
		return false;
	}

	Diligent::EngineVkCreateInfo engine_create_info;
	g_diligent_state.engine_factory->CreateDeviceAndContextsVk(engine_create_info, &g_diligent_state.render_device, &g_diligent_state.immediate_context);
	if (!g_diligent_state.render_device || !g_diligent_state.immediate_context)
	{
		dsi_ConsolePrint("Diligent: failed to create device/contexts.");
	}
	return g_diligent_state.render_device && g_diligent_state.immediate_context;
}

bool diligent_CreateSwapChain(uint32 width, uint32 height)
{
	if (!g_diligent_state.engine_factory || !g_diligent_state.render_device || !g_diligent_state.immediate_context || !g_diligent_state.native_window_handle)
	{
		dsi_ConsolePrint("Diligent: missing prerequisites for swap chain creation.");
		return false;
	}

	Diligent::SwapChainDesc swap_chain_desc{};
	swap_chain_desc.Width = width;
	swap_chain_desc.Height = height;
	swap_chain_desc.Usage = static_cast<Diligent::SWAP_CHAIN_USAGE_FLAGS>(
		swap_chain_desc.Usage | Diligent::SWAP_CHAIN_USAGE_COPY_SOURCE);

	Diligent::NativeWindow window{g_diligent_state.native_window_handle};
	g_diligent_state.engine_factory->CreateSwapChainVk(g_diligent_state.render_device, g_diligent_state.immediate_context, swap_chain_desc, window, &g_diligent_state.swap_chain);
	if (!g_diligent_state.swap_chain)
	{
		dsi_ConsolePrint("Diligent: failed to create swap chain.");
	}
	return static_cast<bool>(g_diligent_state.swap_chain);
}

bool diligent_EnsureSwapChain()
{
	if (!diligent_EnsureDevice())
	{
		return false;
	}

	uint32 width = g_diligent_state.screen_width;
	uint32 height = g_diligent_state.screen_height;
	if (!diligent_QueryWindowSize(width, height))
	{
		if (width == 0 || height == 0)
		{
			return false;
		}
	}
	if (!g_diligent_state.swap_chain)
	{
		if (!diligent_CreateSwapChain(width, height))
		{
			return false;
		}

		diligent_UpdateScreenSize(width, height);
		return true;
	}

	if (width != g_diligent_state.screen_width || height != g_diligent_state.screen_height)
	{
		g_diligent_state.swap_chain->Resize(width, height, Diligent::SURFACE_TRANSFORM_OPTIMAL);
		diligent_UpdateScreenSize(width, height);
	}

	return true;
}
