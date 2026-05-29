/**
 * diligent_imgui.cpp
 *
 * Engine-owned Dear ImGui host for LithTech Singularity.
 *
 * Responsibilities:
 *   - Own the single ImGui context + the Diligent/SDL3 ImGui backend.
 *   - Lazily initialize once the render device, swap chain and OS window exist.
 *   - Drive the per-frame ImGui lifecycle on behalf of game modules (which only
 *     issue ImGui:: widget calls): BeginFrame() -> [game UI] -> EndFrameAndRender().
 *   - Expose all of the above to game modules through the ILTImGui interface,
 *     plus a HTEXTURE -> ImTextureID bridge for ImGui::Image().
 *
 * The host is consumed across the DLL boundary: game modules obtain the context
 * and allocator via ILTImGui and call ImGui::SetCurrentContext /
 * SetAllocatorFunctions so ImGui's per-module globals resolve to this context.
 *
 * SDL input is forwarded from the kernel event pump via ltjs_imgui_handle_sdl_event().
 */

#include "iltimgui.h"

#include "diligent_device.h"       // diligent_get_active_render_target
#include "diligent_drawprim_api.h" // diligent_get_drawprim_texture_view
#include "diligent_state.h"        // g_diligent_state, g_diligent_imgui_term_hook
#include "ltjs_imgui_input.h"

#include "imgui.h"

#include "ImGuiImplSDL3.hpp"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"

#include <memory>

namespace
{

std::unique_ptr<Diligent::ImGuiImplSDL3> g_backend;
bool g_ready = false;        // backend successfully created
bool g_frame_active = false; // BeginFrame() started an ImGui frame this tick

/// Releases the ImGui *device* objects before the render device is destroyed.
/// Invoked from diligent_Term() via g_diligent_imgui_term_hook.
///
/// We deliberately do NOT run ~ImGuiImplSDL3 / ImGui::DestroyContext() here: at
/// engine teardown the SDL subsystem and window may already be gone, and the
/// platform backend's shutdown touches SDL - which crashed on exit. Freeing the
/// Diligent GPU objects (the only thing that must happen before the device dies)
/// and leaking the remaining platform/context state is safe; the OS reclaims it
/// at process exit.
void ImGuiHostShutdown()
{
	if (g_backend)
	{
		g_backend->InvalidateDeviceObjects();
		(void)g_backend.release(); // intentional leak to avoid SDL/context shutdown-order crash
	}
	g_ready = false;
	g_frame_active = false;
}

/// Creates the ImGui context + Diligent/SDL3 backend once the device, swap chain
/// and window are available. Returns true when the host is ready to draw.
bool EnsureHost()
{
	if (g_ready)
	{
		return true;
	}

	if (!g_diligent_state.render_device || !g_diligent_state.swap_chain)
	{
		return false;
	}
#ifdef LTJS_SDL_BACKEND
	if (!g_diligent_state.sdl_window)
	{
		return false;
	}
#endif

	if (ImGui::GetCurrentContext() == nullptr)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();
		io.IniFilename = nullptr; // do not read/write imgui.ini
		ImGui::StyleColorsDark();
	}

	const Diligent::SwapChainDesc &sc_desc = g_diligent_state.swap_chain->GetDesc();
	const Diligent::ImGuiDiligentCreateInfo ci{g_diligent_state.render_device.RawPtr(), sc_desc};

#ifdef LTJS_SDL_BACKEND
	g_backend = Diligent::ImGuiImplSDL3::Create(ci, g_diligent_state.sdl_window);
#endif

	if (!g_backend)
	{
		return false;
	}

	// Ensure device objects are released before the device is torn down.
	g_diligent_imgui_term_hook = &ImGuiHostShutdown;

	g_ready = true;
	return true;
}

} // namespace

bool ltjs_imgui_handle_sdl_event(const void *sdl_event)
{
	if (!g_ready || !g_backend || sdl_event == nullptr)
	{
		return false;
	}
	return g_backend->HandleSDLEvent(static_cast<const SDL_Event *>(sdl_event));
}

namespace
{

/// ILTImGui implementation. Thin facade over the host state above so game
/// modules can drive ImGui through the engine interface database.
class CLTImGui : public ILTImGui
{
public:
	declare_interface(CLTImGui);

	ImGuiContext *GetContext() override
	{
		return ImGui::GetCurrentContext();
	}

	void GetAllocatorFunctions(ImGuiMemAllocFunc *pAllocFunc, ImGuiMemFreeFunc *pFreeFunc, void **ppUserData) override
	{
		ImGui::GetAllocatorFunctions(pAllocFunc, pFreeFunc, ppUserData);
	}

	bool BeginFrame() override
	{
		g_frame_active = false;
		if (!EnsureHost())
		{
			return false;
		}

		const Diligent::SwapChainDesc &sc_desc = g_diligent_state.swap_chain->GetDesc();
		g_backend->NewFrame(sc_desc.Width, sc_desc.Height, sc_desc.PreTransform); // calls ImGui::NewFrame()
		g_frame_active = true;
		return true;
	}

	void EndFrameAndRender() override
	{
		if (!g_frame_active)
		{
			return;
		}
		g_frame_active = false;

		Diligent::ITextureView *rtv = diligent_get_active_render_target();
		Diligent::IDeviceContext *ctx = g_backend ? g_diligent_state.immediate_context.RawPtr() : nullptr;
		if (g_backend && rtv && ctx)
		{
			ctx->SetRenderTargets(1, &rtv, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			g_backend->Render(ctx); // calls ImGui::Render() internally
		}
		else
		{
			// Balance the ImGui::NewFrame() issued by BeginFrame() so the next
			// frame's NewFrame() does not assert.
			ImGui::EndFrame();
		}
	}

	ImTextureID GetTextureId(HTEXTURE hTexture) override
	{
		if (hTexture == nullptr)
		{
			return 0;
		}
		Diligent::ITextureView *view = diligent_get_drawprim_texture_view(hTexture, false);
		return reinterpret_cast<ImTextureID>(view);
	}

	bool WantCaptureMouse() override
	{
		return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
	}

	bool WantCaptureKeyboard() override
	{
		return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;
	}
};

} // namespace

define_interface(CLTImGui, ILTImGui);
