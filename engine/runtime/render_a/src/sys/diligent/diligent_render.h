/**
 * diligent_render.h
 *
 * This header defines the Render portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef __DILIGENT_RENDER_H__
#define __DILIGENT_RENDER_H__

#include "ltvector.h"

namespace Diligent
{
	class ITextureView;
}

/// \brief Overrides the active render/depth targets for subsequent renderer output.
/// \details This is used by the editor and post-processing passes to redirect
///          rendering into offscreen targets. Pass nullptrs to restore the
///          swap chain back buffer and depth buffer.
/// \code
/// diligent_SetOutputTargets(offscreen_rtv, offscreen_dsv);
/// // ... render offscreen ...
/// diligent_SetOutputTargets(nullptr, nullptr);
/// \endcode
void diligent_SetOutputTargets(Diligent::ITextureView *render_target, Diligent::ITextureView *depth_target);
/// \brief Sets the world-space offset applied to world geometry during rendering.
/// \details Used by editor preview and world-model rendering to shift geometry
///          without modifying source data.
void diligent_SetWorldOffset(const LTVector& offset);
/// \brief Returns the current world-space offset applied during rendering.
const LTVector& diligent_GetWorldOffset();

#endif
