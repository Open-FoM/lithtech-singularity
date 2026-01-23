/**
 * diligent_texture_cache.h
 *
 * This header defines the Texture Cache portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
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

/// Cached GPU texture plus associated converted CPU data (if needed).
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

/// Releases cached render texture resources for a SharedTexture.
void diligent_release_render_texture(SharedTexture* shared_texture);
/// Returns the cached render texture, updating it if \p texture_changed is true.
DiligentRenderTexture* diligent_get_render_texture(SharedTexture* shared_texture, bool texture_changed);
/// \brief Returns the shader-resource view for a SharedTexture.
/// \details If the texture data has changed, pass \p texture_changed to refresh.
/// \code
/// auto* srv = diligent_get_texture_view(shared_tex, true);
/// if (srv) { /* bind to shader */ }
/// \endcode
Diligent::ITextureView* diligent_get_texture_view(SharedTexture* shared_texture, bool texture_changed);

/// Internal helper for extracting texture debug info.
bool diligent_GetTextureDebugInfo_Internal(SharedTexture* texture, DiligentTextureDebugInfo& out_info);
/// Internal helper for testing texture conversion to BGRA32.
bool diligent_TestConvertTextureToBgra32_Internal(const TextureData* source, uint32 mip_count, TextureData*& out_converted);

#endif
