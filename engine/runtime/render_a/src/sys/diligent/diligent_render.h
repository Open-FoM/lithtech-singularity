#ifndef __DILIGENT_RENDER_H__
#define __DILIGENT_RENDER_H__

#include "ltvector.h"

class SharedTexture;

namespace Diligent
{
	class ITextureView;
}

// Overrides the render target used by the renderer. Pass nullptrs to restore swapchain output.
void diligent_SetOutputTargets(Diligent::ITextureView *render_target, Diligent::ITextureView *depth_target);
void diligent_SetWorldOffset(const LTVector& offset);
const LTVector& diligent_GetWorldOffset();

#endif
