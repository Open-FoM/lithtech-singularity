/**
 * diligent_postfx.h
 *
 * This header defines the Postfx portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_POSTFX_H
#define LTJS_DILIGENT_POSTFX_H

#include "ltbasedefs.h"

struct SceneDesc;
union LTRGBColor;

namespace Diligent
{
	class ITextureView;
}

/// \brief Per-frame SSAO render state used to restore targets after post-processing.
struct DiligentSsaoContext
{
	bool active = false;
	Diligent::ITextureView* prev_render_target = nullptr;
	Diligent::ITextureView* prev_depth_target = nullptr;
	Diligent::ITextureView* final_render_target = nullptr;
	Diligent::ITextureView* final_depth_target = nullptr;
};

/// \brief Per-frame antialiasing render state used to restore targets after post-processing.
struct DiligentAaContext
{
	bool active = false;
	bool msaa_active = false;
	bool defer_copy = false;
	Diligent::ITextureView* prev_render_target = nullptr;
	Diligent::ITextureView* prev_depth_target = nullptr;
	Diligent::ITextureView* final_render_target = nullptr;
	Diligent::ITextureView* final_depth_target = nullptr;
};

/// \brief Returns 1.0f if tonemapping is enabled, otherwise 0.0f.
float diligent_get_tonemap_enabled();

/// \brief Renders the screen-space glow post-effect for the given scene.
/// \details This uses the glow render-style mapping and blur pipeline to
///          composite glow over the current back buffer.
bool diligent_render_screen_glow(SceneDesc* desc);
/// \brief Returns the effective MSAA sample count (0 disables MSAA).
uint32 diligent_get_msaa_samples();
/// \brief Begins MSAA rendering and redirects output to AA targets if needed.
bool diligent_begin_antialiasing(SceneDesc* desc, DiligentAaContext& ctx);
/// \brief Resolves MSAA into the final target if active.
bool diligent_apply_antialiasing(const DiligentAaContext& ctx);
/// \brief Ends AA rendering and restores prior render targets.
void diligent_end_antialiasing(const DiligentAaContext& ctx);
/// \brief Applies SSAO using MSAA-resolved color/depth inputs.
bool diligent_apply_ssao_resolved(const DiligentAaContext& ctx);
/// \brief Begins SSAO rendering and redirects output to SSAO targets.
/// \details On success, the caller should render the scene into the SSAO
///          targets and then call diligent_apply_ssao/diligent_end_ssao.
bool diligent_begin_ssao(SceneDesc* desc, DiligentSsaoContext& ctx);
/// \brief Executes SSAO passes (AO, blur, composite) if active.
/// \code
/// DiligentSsaoContext ctx{};
/// if (diligent_begin_ssao(desc, ctx)) {
///   // draw scene into SSAO targets
///   diligent_apply_ssao(ctx);
///   diligent_end_ssao(ctx);
/// }
/// \endcode
bool diligent_apply_ssao(const DiligentSsaoContext& ctx);
/// \brief Ends SSAO rendering and restores prior render targets.
void diligent_end_ssao(const DiligentSsaoContext& ctx);

/// \brief Updates the clear color used for SSAO scene capture.
void diligent_ssao_set_clear_color(const LTRGBColor& clear_color);

/// \brief Adds a renderstyle remap for glow processing.
/// \details \p source is replaced by \p map_to for the glow pass.
bool diligent_AddGlowRenderStyleMapping(const char* source, const char* map_to);
/// \brief Sets the default renderstyle file for glow processing.
bool diligent_SetGlowDefaultRenderStyle(const char* filename);
bool diligent_SetNoGlowRenderStyle(const char* filename);

void diligent_postfx_term();

#endif
