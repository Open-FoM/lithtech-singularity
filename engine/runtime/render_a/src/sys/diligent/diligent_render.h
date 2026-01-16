#ifndef __DILIGENT_RENDER_H__
#define __DILIGENT_RENDER_H__

class SharedTexture;

namespace Diligent
{
	class ITextureView;
}

Diligent::ITextureView* diligent_get_drawprim_texture_view(SharedTexture* shared_texture, bool texture_changed);

#endif
