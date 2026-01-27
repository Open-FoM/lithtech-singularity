#ifndef LTJS_DILIGENT_SSR_FX_H
#define LTJS_DILIGENT_SSR_FX_H

#include "diligent_postfx.h"

/// \brief Returns true when SSR is enabled.
bool diligent_ssr_is_enabled();
/// \brief Applies SSR to the given render target.
bool diligent_apply_ssr(Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target);
/// \brief Applies SSR after MSAA resolve.
bool diligent_apply_ssr_resolved(const DiligentAaContext& ctx);
/// \brief Releases SSR resources.
void diligent_ssr_term();

#endif
