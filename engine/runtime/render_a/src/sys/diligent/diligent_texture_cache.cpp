#include "diligent_texture_cache.h"
#include "diligent_state.h"

#include "diligent_render_debug.h"
#include "diligent_utils.h"

#include "renderstruct.h"
#include "de_world.h"
#include "dtxmgr.h"
#include "pixelformat.h"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"

#include <cstring>
#include <vector>

FormatMgr g_FormatMgr;

static Diligent::TEXTURE_FORMAT diligent_map_texture_format(BPPIdent format, const PFormat* pformat)
{
	switch (format)
	{
		case BPP_32:
			return Diligent::TEX_FORMAT_BGRA8_UNORM;
		case BPP_24:
			return Diligent::TEX_FORMAT_UNKNOWN;
		case BPP_16:
			if (pformat && pformat->m_Masks[CP_ALPHA] != 0)
			{
				return Diligent::TEX_FORMAT_UNKNOWN;
			}
			return Diligent::TEX_FORMAT_B5G6R5_UNORM;
		case BPP_S3TC_DXT1:
			return Diligent::TEX_FORMAT_BC1_UNORM;
		case BPP_S3TC_DXT3:
			return Diligent::TEX_FORMAT_BC2_UNORM;
		case BPP_S3TC_DXT5:
			return Diligent::TEX_FORMAT_BC3_UNORM;
		default:
			return Diligent::TEX_FORMAT_UNKNOWN;
	}
}

static inline BPPIdent diligent_get_bpp_ident_const(const DtxHeader& header)
{
	return header.m_Extra[2] == 0 ? BPP_32 : static_cast<BPPIdent>(header.m_Extra[2]);
}

static bool diligent_is_texture_format_supported(Diligent::TEXTURE_FORMAT format)
{
	return g_diligent_state.render_device && g_diligent_state.render_device->GetTextureFormatInfo(format).Supported;
}

static uint32 diligent_calc_compressed_stride(BPPIdent format, uint32 width)
{
	const uint32 blocks_w = (width + 3) / 4;
	switch (format)
	{
		case BPP_S3TC_DXT1:
			return blocks_w * 8;
		case BPP_S3TC_DXT3:
		case BPP_S3TC_DXT5:
			return blocks_w * 16;
		default:
			return 0;
	}
}

static std::unique_ptr<TextureData, DiligentRenderTexture::DtxTextureDeleter>
diligent_convert_texture_to_bgra32(const TextureData* source, uint32 mip_count, uint32 mip_offset)
{
	if (!source || mip_count == 0)
	{
		return {};
	}

	if (mip_offset >= mip_count)
	{
		mip_offset = 0;
	}
	mip_count -= mip_offset;
	if (mip_count == 0)
	{
		return {};
	}

	const auto& base_mip = source->m_Mips[mip_offset];
	if (!base_mip.m_Data || base_mip.m_Width == 0 || base_mip.m_Height == 0)
	{
		return {};
	}

	auto* dest = dtx_Alloc(
		BPP_32,
		base_mip.m_Width,
		base_mip.m_Height,
		mip_count,
		nullptr,
		nullptr,
		source->m_Header.m_IFlags);

	if (!dest)
	{
		return {};
	}

	dtx_SetupDTXFormat2(BPP_32, &dest->m_PFormat);
	dest->m_Header.m_IFlags = source->m_Header.m_IFlags;
	dest->m_Header.m_UserFlags = source->m_Header.m_UserFlags;
	dest->m_Header.m_nSections = source->m_Header.m_nSections;
	std::memcpy(dest->m_Header.m_CommandString,
		source->m_Header.m_CommandString,
		sizeof(dest->m_Header.m_CommandString));

	PFormat src_format = source->m_PFormat;

	for (uint32 level = 0; level < mip_count; ++level)
	{
		const auto& src_mip = source->m_Mips[level + mip_offset];
		auto& dst_mip = dest->m_Mips[level];
		if (!src_mip.m_Data || !dst_mip.m_Data)
		{
			break;
		}

		FMConvertRequest request;
		request.m_pSrcFormat = &src_format;
		request.m_pSrc = const_cast<uint8*>(src_mip.m_Data);
		request.m_SrcPitch = 0;
		request.m_pSrcPalette = nullptr;
		request.m_pDestFormat = &g_FormatMgr.m_32BitFormat;
		request.m_pDest = dst_mip.m_Data;
		request.m_DestPitch = dst_mip.m_Pitch;
		request.m_Width = src_mip.m_Width;
		request.m_Height = src_mip.m_Height;

		if (g_FormatMgr.ConvertPixels(&request, nullptr) != LT_OK)
		{
			dtx_Destroy(dest);
			return {};
		}
	}

	return std::unique_ptr<TextureData, DiligentRenderTexture::DtxTextureDeleter>(dest);
}

static std::unique_ptr<TextureData, DiligentRenderTexture::DtxTextureDeleter>
diligent_clone_texture_data(const TextureData* source, uint32 mip_count)
{
	if (!source || mip_count == 0)
	{
		return {};
	}

	const auto& base_mip = source->m_Mips[0];
	if (!base_mip.m_Data || base_mip.m_Width == 0 || base_mip.m_Height == 0)
	{
		return {};
	}

	const auto source_bpp = diligent_get_bpp_ident_const(source->m_Header);
	auto* dest = dtx_Alloc(
		source_bpp,
		base_mip.m_Width,
		base_mip.m_Height,
		mip_count,
		nullptr,
		nullptr,
		source->m_Header.m_IFlags);

	if (!dest)
	{
		return {};
	}

	dtx_SetupDTXFormat2(source_bpp, &dest->m_PFormat);
	dest->m_Header = source->m_Header;
	dest->m_Header.m_nMipmaps = static_cast<uint16>(mip_count);
	dest->m_Header.m_BaseWidth = static_cast<uint16>(base_mip.m_Width);
	dest->m_Header.m_BaseHeight = static_cast<uint16>(base_mip.m_Height);

	for (uint32 level = 0; level < mip_count; ++level)
	{
		const auto& src_mip = source->m_Mips[level];
		auto& dst_mip = dest->m_Mips[level];
		if (!src_mip.m_Data || !dst_mip.m_Data)
		{
			break;
		}
		const uint32 copy_size = LTMIN(static_cast<uint32>(src_mip.m_dataSize),
			static_cast<uint32>(dst_mip.m_dataSize));
		std::memcpy(dst_mip.m_Data, src_mip.m_Data, copy_size);
	}

	return std::unique_ptr<TextureData, DiligentRenderTexture::DtxTextureDeleter>(dest);
}

static bool diligent_texture_has_visible_alpha(const TextureData* texture_data)
{
	if (!texture_data || diligent_get_bpp_ident_const(texture_data->m_Header) != BPP_32)
	{
		return false;
	}

	const auto& mip = texture_data->m_Mips[0];
	if (!mip.m_Data || mip.m_Pitch <= 0 || mip.m_Height == 0)
	{
		return false;
	}

	const uint32 row_stride = static_cast<uint32>(mip.m_Pitch);
	if (row_stride < mip.m_Width * sizeof(uint32))
	{
		return false;
	}

	const uint8* row = mip.m_Data;
	const uint32 sample_stride = LTMAX<uint32>(1, mip.m_Width / 16);
	for (uint32 y = 0; y < mip.m_Height; ++y)
	{
		const uint32* pixels = reinterpret_cast<const uint32*>(row);
		for (uint32 x = 0; x < mip.m_Width; x += sample_stride)
		{
			if ((pixels[x] & 0xFF000000u) != 0)
			{
				return true;
			}
		}
		row += row_stride;
	}

	return false;
}

static void diligent_force_texture_alpha_opaque(TextureData* texture_data)
{
	if (!texture_data || diligent_get_bpp_ident_const(texture_data->m_Header) != BPP_32)
	{
		return;
	}

	for (uint32 level = 0; level < texture_data->m_Header.m_nMipmaps; ++level)
	{
		auto& mip = texture_data->m_Mips[level];
		if (!mip.m_Data || mip.m_Pitch <= 0 || mip.m_Height == 0)
		{
			continue;
		}

		const uint32 row_stride = static_cast<uint32>(mip.m_Pitch);
		if (row_stride < mip.m_Width * sizeof(uint32))
		{
			continue;
		}

		uint8* row = mip.m_Data;
		for (uint32 y = 0; y < mip.m_Height; ++y)
		{
			auto* pixels = reinterpret_cast<uint32*>(row);
			for (uint32 x = 0; x < mip.m_Width; ++x)
			{
				pixels[x] |= 0xFF000000u;
			}
			row += row_stride;
		}
	}
}

bool diligent_TestConvertTextureToBgra32_Internal(const TextureData* source, uint32 mip_count, TextureData*& out_converted)
{
	out_converted = nullptr;
	auto converted = diligent_convert_texture_to_bgra32(source, mip_count, 0);
	if (!converted)
	{
		return false;
	}

	out_converted = converted.release();
	return true;
}

static uint32 diligent_get_mip_count(const TextureData* texture_data)
{
	if (!texture_data)
	{
		return 0;
	}

	const uint32 mip_count = texture_data->m_Header.m_nMipmaps;
	return mip_count == 0 ? 1u : mip_count;
}

void diligent_release_render_texture(SharedTexture* shared_texture)
{
	if (!shared_texture)
	{
		return;
	}

	auto* render_texture = static_cast<DiligentRenderTexture*>(shared_texture->m_pRenderData);
	if (!render_texture)
	{
		return;
	}

	shared_texture->m_pRenderData = nullptr;
	delete render_texture;
}

DiligentRenderTexture* diligent_get_render_texture(SharedTexture* shared_texture, bool texture_changed)
{
	if (!shared_texture)
	{
		return nullptr;
	}

	auto* render_texture = static_cast<DiligentRenderTexture*>(shared_texture->m_pRenderData);
	if (render_texture && !texture_changed)
	{
		return render_texture;
	}

	diligent_release_render_texture(shared_texture);
	if (!g_diligent_state.render_struct)
	{
		return nullptr;
	}

	TextureData* texture_data = g_diligent_state.render_struct->GetTexture(shared_texture);
	if (!texture_data)
	{
		return nullptr;
	}

	const uint32 mip_count = diligent_get_mip_count(texture_data);
	auto format = diligent_map_texture_format(texture_data->m_Header.GetBPPIdent(), &texture_data->m_PFormat);
	auto converted = diligent_clone_texture_data(texture_data, mip_count);
	if (!converted)
	{
		return nullptr;
	}

	const BPPIdent source_bpp = diligent_get_bpp_ident_const(texture_data->m_Header);
	const bool is_compressed = (source_bpp == BPP_S3TC_DXT1 || source_bpp == BPP_S3TC_DXT3 || source_bpp == BPP_S3TC_DXT5);
	if (format == Diligent::TEX_FORMAT_UNKNOWN ||
		!diligent_is_texture_format_supported(format)
		|| (is_compressed && !converted))
	{
		converted = diligent_convert_texture_to_bgra32(texture_data, mip_count, 0);
		if (!converted)
		{
			return nullptr;
		}
		format = Diligent::TEX_FORMAT_BGRA8_UNORM;
	}

	if (converted && format == Diligent::TEX_FORMAT_BGRA8_UNORM && !diligent_texture_has_visible_alpha(converted.get()))
	{
		diligent_force_texture_alpha_opaque(converted.get());
	}

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_texture";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = converted->m_Mips[0].m_Width;
	desc.Height = converted->m_Mips[0].m_Height;
	desc.MipLevels = mip_count;
	desc.Format = format;
	desc.Usage = Diligent::USAGE_DEFAULT;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	std::vector<Diligent::TextureSubResData> subresources;
	subresources.resize(mip_count);
	for (uint32 level = 0; level < mip_count; ++level)
	{
		const auto& mip = converted->m_Mips[level];
		uint32 stride = mip.m_Pitch;
		if (is_compressed)
		{
			stride = diligent_calc_compressed_stride(converted->m_Header.GetBPPIdent(), mip.m_Width);
		}
		subresources[level].pData = mip.m_Data;
		subresources[level].Stride = stride;
		subresources[level].DepthStride = 0;
	}

	Diligent::TextureData init_data{subresources.data(), static_cast<Diligent::Uint32>(subresources.size())};
	auto* new_render_texture = new DiligentRenderTexture();
	new_render_texture->texture_data = texture_data;
	new_render_texture->format = texture_data->m_Header.GetBPPIdent();
	new_render_texture->converted_texture_data = std::move(converted);
	g_diligent_state.render_device->CreateTexture(desc, &init_data, &new_render_texture->texture);
	if (!new_render_texture->texture)
	{
		delete new_render_texture;
		return nullptr;
	}

	new_render_texture->srv = new_render_texture->texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	new_render_texture->width = desc.Width;
	new_render_texture->height = desc.Height;
	shared_texture->m_pRenderData = new_render_texture;
	return new_render_texture;
}

Diligent::ITextureView* diligent_get_texture_view(SharedTexture* shared_texture, bool texture_changed)
{
	auto* render_texture = diligent_get_render_texture(shared_texture, texture_changed);
	return render_texture ? render_texture->srv.RawPtr() : nullptr;
}

bool diligent_GetTextureDebugInfo_Internal(SharedTexture* texture, DiligentTextureDebugInfo& out_info)
{
	out_info = {};
	if (!texture)
	{
		return false;
	}

	auto* render_texture = diligent_get_render_texture(texture, false);
	if (!render_texture)
	{
		return false;
	}

	out_info.has_render_texture = true;
	out_info.used_conversion = render_texture->converted_texture_data != nullptr;
	out_info.width = render_texture->width;
	out_info.height = render_texture->height;
	if (render_texture->texture)
	{
		out_info.format = static_cast<uint32_t>(render_texture->texture->GetDesc().Format);
	}
	const TextureData* cpu_texture = render_texture->converted_texture_data
		? render_texture->converted_texture_data.get()
		: render_texture->texture_data;
	if (cpu_texture)
	{
		const auto bpp = cpu_texture->m_Header.m_Extra[2] == 0
			? BPP_32
			: static_cast<BPPIdent>(cpu_texture->m_Header.m_Extra[2]);
		if (bpp == BPP_32 && cpu_texture->m_Mips[0].m_Data)
		{
			out_info.has_cpu_pixel = true;
			std::memcpy(&out_info.first_pixel, cpu_texture->m_Mips[0].m_Data, sizeof(uint32));
		}
	}
	return true;
}

bool diligent_GetTextureDebugInfo(SharedTexture* texture, DiligentTextureDebugInfo& out_info)
{
	return diligent_GetTextureDebugInfo_Internal(texture, out_info);
}

bool diligent_TestConvertTextureToBgra32(const TextureData* source, uint32 mip_count, TextureData*& out_converted)
{
	return diligent_TestConvertTextureToBgra32_Internal(source, mip_count, out_converted);
}
