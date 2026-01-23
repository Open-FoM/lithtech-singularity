/**
 * diligent_drawprim_api.h
 *
 * This header defines the Drawprim API portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#pragma once

class SharedTexture;

namespace Diligent
{
	class ITextureView;
}

/// \brief Returns the shader-resource view for a DrawPrim texture.
/// \details If \p texture_changed is true, the texture cache is refreshed before
///          returning the SRV. This is used by immediate-mode rendering paths.
/// \code
/// auto* view = diligent_get_drawprim_texture_view(shared_tex, true);
/// if (view) { /* bind SRV */ }
/// \endcode
Diligent::ITextureView* diligent_get_drawprim_texture_view(SharedTexture* shared_texture, bool texture_changed);
