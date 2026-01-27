#ifndef LTJS_DILIGENT_SSGI_FX_H
#define LTJS_DILIGENT_SSGI_FX_H

#include "diligent_postfx.h"

/// \brief Returns true when SSGI is enabled.
bool diligent_ssgi_is_enabled();
/// \brief Applies SSGI to the given render target.
bool diligent_apply_ssgi(Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target);
/// \brief Applies SSGI after MSAA resolve.
bool diligent_apply_ssgi_resolved(const DiligentAaContext& ctx);
/// \brief Releases SSGI resources.
void diligent_ssgi_term();

#endif
