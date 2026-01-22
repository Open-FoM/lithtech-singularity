#ifndef LTJS_DILIGENT_TEXTURE_CACHE_H
#define LTJS_DILIGENT_TEXTURE_CACHE_H

#include "ltbasedefs.h"
#include "dtxmgr.h"
#include "pixelformat.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Texture.h"

#include <memory>

class SharedTexture;
class TextureData;
struct DiligentTextureDebugInfo;

struct DiligentRenderTexture
{
	Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> srv;
	TextureData* texture_data = nullptr;
	struct DtxTextureDeleter
	{
		void operator()(TextureData* ptr) const
		{
			dtx_Destroy(ptr);
		}
	};
	std::unique_ptr<TextureData, DtxTextureDeleter> converted_texture_data;
	uint32 width = 0;
	uint32 height = 0;
	BPPIdent format = BPP_32;
};

void diligent_release_render_texture(SharedTexture* shared_texture);
DiligentRenderTexture* diligent_get_render_texture(SharedTexture* shared_texture, bool texture_changed);
Diligent::ITextureView* diligent_get_texture_view(SharedTexture* shared_texture, bool texture_changed);

bool diligent_GetTextureDebugInfo_Internal(SharedTexture* texture, DiligentTextureDebugInfo& out_info);
bool diligent_TestConvertTextureToBgra32_Internal(const TextureData* source, uint32 mip_count, TextureData*& out_converted);

#endif
