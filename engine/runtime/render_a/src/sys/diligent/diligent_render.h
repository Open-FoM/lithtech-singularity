#ifndef __DILIGENT_RENDER_H__
#define __DILIGENT_RENDER_H__

class SharedTexture;

namespace Diligent
{
	class ITextureView;
}

// Overrides the render target used by the renderer. Pass nullptrs to restore swapchain output.
void diligent_SetOutputTargets(Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target);

Diligent::ITextureView* diligent_get_drawprim_texture_view(SharedTexture* shared_texture, bool texture_changed);

#endif
