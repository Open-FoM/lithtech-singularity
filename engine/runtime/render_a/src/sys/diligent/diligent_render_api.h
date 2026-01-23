/**
 * diligent_render_api.h
 *
 * This header defines the Render API portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_RENDER_API_H
#define LTJS_DILIGENT_RENDER_API_H

#include "ltbasedefs.h"
#include "renderstruct.h"

class SharedTexture;

namespace Diligent
{
	class IDeviceContext;
	class IRenderDevice;
	class ISwapChain;
	class ITextureView;
}

/// \brief Clears the active render target and (optionally) depth buffer.
/// \details This is the primary entry point for clearing the current output targets.
///          It respects renderer overrides set via diligent_SetOutputTargets and
///          also seeds post effects (e.g., SSAO clear color) with the same RGBA.
/// \param rect Optional rectangle to clear; nullptr clears the full target.
/// \param flags Bitmask of CLEARSCREEN_ flags (see de_codes.h).
/// \param clear_color RGBA color in 0-255 per channel.
/// \code
/// LTRGBColor clear{};
/// clear.rgb.r = 16;
/// clear.rgb.g = 32;
/// clear.rgb.b = 48;
/// clear.rgb.a = 255;
/// diligent_Clear(nullptr, CLEARSCREEN_SCREEN, clear);
/// \endcode
void diligent_Clear(LTRect* rect, uint32 flags, LTRGBColor& clear_color);
/// \brief Marks the beginning of a 3D render section.
/// \details Increments the renderer's in-3D counter and toggles internal state
///          used by DrawPrim and post effects.
/// \code
/// if (diligent_Start3D()) {
///   // draw scene
///   diligent_End3D();
/// }
/// \endcode
bool diligent_Start3D();
/// \brief Marks the end of a 3D render section.
/// \details Decrements the in-3D counter and restores non-3D state when it
///          reaches zero.
bool diligent_End3D();
/// \brief Returns true if currently inside a 3D render section.
bool diligent_IsIn3D();

/// \brief Executes a renderer-specific command line.
/// \details This is intended for legacy command routing. Most renderer settings
///          should be driven via console variables instead.
void diligent_RenderCommand(int argc, const char** argv);
/// \brief Presents the swap chain.
/// \details This forwards to the Diligent swap chain present call.
void diligent_SwapBuffers(uint32 flags);
/// \brief Locks a portion of the back buffer for CPU access.
/// \details Not supported in the Diligent backend; returns false.
bool diligent_LockScreen(int left, int top, int right, int bottom, void** out_buffer, long* out_pitch);
/// \brief Unlocks a previously locked screen region.
void diligent_UnlockScreen();
/// \brief Requests a screenshot capture to the given filename.
/// \note Screenshot capture is currently a stub in the Diligent backend.
void diligent_MakeScreenShot(const char* filename);
/// \brief Generates a cubic environment map from the given scene.
/// \note This is currently a stub in the Diligent backend.
void diligent_MakeCubicEnvMap(const char* filename, uint32 size, const SceneDesc& scene);
/// \brief Re-reads renderer console variables (CVar-backed).
void diligent_ReadConsoleVariables();
/// \brief Fills renderer capability/feature info.
void diligent_GetRenderInfo(RenderInfoStruct* info);

/// \brief Creates a renderer context (currently a lightweight handle).
/// \details The Diligent backend does not allocate heavy per-context state.
HRENDERCONTEXT diligent_CreateContext();
/// \brief Destroys a previously created renderer context.
void diligent_DeleteContext(HRENDERCONTEXT context);

/// \brief Binds a shared texture for immediate-mode rendering.
/// \details Used by legacy DrawPrim-style rendering paths.
void diligent_BindTexture(SharedTexture* texture, bool texture_changed);
/// \brief Unbinds a shared texture previously bound.
void diligent_UnbindTexture(SharedTexture* texture);
/// \brief Returns a legacy texture format descriptor for the given input format.
uint32 diligent_GetTextureFormat1(BPPIdent format, uint32 flags);
/// \brief Returns true if the given format is supported by the renderer.
bool diligent_QueryDDSupport(PFormat* format);
/// \brief Resolves a renderable format for the given input parameters.
bool diligent_GetTextureFormat2(BPPIdent format, uint32 flags, PFormat* out_format);
/// \brief Converts texture data between formats for D3D-style uploads.
/// \details This is primarily used by the legacy texture manager to prepare
///          data for GPU upload.
bool diligent_ConvertTexDataToDD(
	uint8* src_data,
	PFormat* src_format,
	uint32 src_width,
	uint32 src_height,
	uint8* dest_data,
	PFormat* dest_format,
	BPPIdent dest_format_id,
	uint32 dest_width,
	uint32 dest_height,
	uint32 flags);

/// \brief Sets the texture for DrawPrim-style rendering.
void diligent_DrawPrimSetTexture(SharedTexture* texture);
/// \brief Disables textures for DrawPrim-style rendering.
void diligent_DrawPrimDisableTextures();

/// \brief Creates a renderer-owned render object of the given type.
/// \code
/// auto* obj = diligent_CreateRenderObject(CRenderObject::RO_SPRITE);
/// // ... configure obj ...
/// diligent_DestroyRenderObject(obj);
/// \endcode
CRenderObject* diligent_CreateRenderObject(CRenderObject::RENDER_OBJECT_TYPES object_type);
/// \brief Destroys a renderer-owned render object.
bool diligent_DestroyRenderObject(CRenderObject* object);

/// \brief Returns the legacy D3D device pointer (non-Diligent builds only).
#if !defined(LTJS_USE_DILIGENT_RENDER)
void* diligent_GetD3DDevice();
#endif
/// \brief Returns the Diligent render device.
Diligent::IRenderDevice* diligent_GetRenderDevice();
/// \brief Returns the immediate Diligent device context.
Diligent::IDeviceContext* diligent_GetImmediateContext();
/// \brief Returns the swap chain used for presentation.
Diligent::ISwapChain* diligent_GetSwapChain();

/// \brief Enumerates supported display modes.
/// \details The returned list must be freed via rdll_FreeModeList.
RMode* rdll_GetSupportedModes();
/// \brief Frees the mode list returned by rdll_GetSupportedModes.
void rdll_FreeModeList(RMode* modes);

/// \brief Tears down renderer API state and releases shared resources.
void diligent_render_api_term();

#endif
