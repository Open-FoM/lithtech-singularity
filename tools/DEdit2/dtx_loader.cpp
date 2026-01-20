#include "dtx_loader.h"

#include "lt_stream.h"

#include <algorithm>
#include <cstring>

namespace
{
uint32 CalcDtxImageSize(BPPIdent bpp, uint32 width, uint32 height)
{
	switch (bpp)
	{
		case BPP_8P:
		case BPP_8:
			return width * height;
		case BPP_16:
			return width * height * 2;
		case BPP_24:
			return width * height * 3;
		case BPP_32:
		case BPP_32P:
			return width * height * 4;
		case BPP_S3TC_DXT1:
		{
			const uint32 blocks_w = (width + 3) / 4;
			const uint32 blocks_h = (height + 3) / 4;
			return blocks_w * blocks_h * 8;
		}
		case BPP_S3TC_DXT3:
		case BPP_S3TC_DXT5:
		{
			const uint32 blocks_w = (width + 3) / 4;
			const uint32 blocks_h = (height + 3) / 4;
			return blocks_w * blocks_h * 16;
		}
		default:
			return width * height * 4;
	}
}

int32 CalcDtxPitch(BPPIdent bpp, uint32 width, uint32 height)
{
	switch (bpp)
	{
		case BPP_8P:
		case BPP_8:
			return static_cast<int32>(width);
		case BPP_16:
			return static_cast<int32>(width * 2);
		case BPP_24:
			return static_cast<int32>(width * 3);
		case BPP_32:
		case BPP_32P:
			return static_cast<int32>(width * 4);
		case BPP_S3TC_DXT1:
		{
			const uint32 blocks_w = (width + 3) / 4;
			return static_cast<int32>(blocks_w * 8);
		}
		case BPP_S3TC_DXT3:
		case BPP_S3TC_DXT5:
		{
			const uint32 blocks_w = (width + 3) / 4;
			return static_cast<int32>(blocks_w * 16);
		}
		default:
			return static_cast<int32>(width * 4);
	}
}

LTRESULT SkipStreamBytes(ILTStream* stream, uint32 size)
{
	if (!stream || size == 0)
	{
		return LT_OK;
	}

	uint32 pos = 0;
	if (stream->GetPos(&pos) != LT_OK)
	{
		return LT_ERROR;
	}

	return stream->SeekTo(pos + size);
}
} // namespace

TextureMipData::TextureMipData()
{
	m_Width = 0;
	m_Height = 0;
	m_DataHeader = nullptr;
	m_Data = nullptr;
	m_dataSize = 0;
	m_Pitch = 0;
}

TextureData::TextureData()
{
	m_pSharedTexture = nullptr;
	m_pDataBuffer = nullptr;
	m_bufSize = 0;
	m_Link.m_pData = this;
}

TextureData::~TextureData()
{
	delete[] m_pDataBuffer;
	m_pDataBuffer = nullptr;
	m_bufSize = 0;
}

void dtx_Destroy(TextureData* texture)
{
	delete texture;
}

void dtx_SetupDTXFormat2(BPPIdent bpp, PFormat* format)
{
	if (!format)
	{
		return;
	}

	format->m_eType = bpp;
	switch (bpp)
	{
		case BPP_8P:
		case BPP_8:
			format->m_nBPP = 8;
			break;
		case BPP_16:
			format->m_nBPP = 16;
			break;
		case BPP_24:
			format->m_nBPP = 24;
			break;
		case BPP_32:
		case BPP_32P:
			format->m_nBPP = 32;
			break;
		case BPP_S3TC_DXT1:
			format->m_nBPP = 4;
			break;
		case BPP_S3TC_DXT3:
		case BPP_S3TC_DXT5:
			format->m_nBPP = 8;
			break;
		default:
			format->m_nBPP = 32;
			break;
	}

	for (uint32 i = 0; i < NUM_COLORPLANES; ++i)
	{
		format->m_Masks[i] = 0;
		format->m_nBits[i] = 0;
		format->m_FirstBits[i] = 0;
	}
}

TextureData* CreateSolidColorTexture(uint32 rgba)
{
	auto* texture = new TextureData();
	if (!texture)
	{
		return nullptr;
	}

	texture->m_ResHeader.m_Type = LT_RESTYPE_DTX;
	texture->m_Header.m_ResType = LT_RESTYPE_DTX;
	texture->m_Header.m_Version = CURRENT_DTX_VERSION;
	texture->m_Header.m_BaseWidth = 1;
	texture->m_Header.m_BaseHeight = 1;
	texture->m_Header.m_nMipmaps = 1;
	texture->m_Header.m_IFlags = 0;
	texture->m_Header.m_UserFlags = 0;
	texture->m_Header.SetBPPIdent(BPP_32);
	texture->m_Header.m_nSections = 0;
	std::memset(texture->m_Header.m_CommandString, 0, sizeof(texture->m_Header.m_CommandString));
	texture->m_Header.m_ExtraLong[0] = 0;
	texture->m_Header.m_ExtraLong[1] = 0;
	texture->m_Header.m_ExtraLong[2] = 0;
	dtx_SetupDTXFormat2(BPP_32, &texture->m_PFormat);

	texture->m_bufSize = sizeof(uint32);
	texture->m_pDataBuffer = new uint8[texture->m_bufSize];
	if (!texture->m_pDataBuffer)
	{
		delete texture;
		return nullptr;
	}

	std::memcpy(texture->m_pDataBuffer, &rgba, sizeof(uint32));
	texture->m_Mips[0].m_Width = 1;
	texture->m_Mips[0].m_Height = 1;
	texture->m_Mips[0].m_Pitch = sizeof(uint32);
	texture->m_Mips[0].m_dataSize = sizeof(uint32);
	texture->m_Mips[0].m_DataHeader = texture->m_pDataBuffer;
	texture->m_Mips[0].m_Data = texture->m_pDataBuffer;

	return texture;
}

TextureData* LoadDtxTexture(const std::string& path, std::string* error)
{
	ILTStream* stream = OpenFileStream(path);
	if (!stream)
	{
		if (error)
		{
			*error = "Failed to open texture file.";
		}
		return nullptr;
	}

	DtxHeader header{};
	stream->Read(&header, sizeof(header));
	if (header.m_ResType != LT_RESTYPE_DTX)
	{
		if (error)
		{
			*error = "Invalid DTX resource type.";
		}
		stream->Release();
		return nullptr;
	}

	if (header.m_Version != CURRENT_DTX_VERSION)
	{
		if (error)
		{
			*error = "Unsupported DTX version.";
		}
		stream->Release();
		return nullptr;
	}

	if (header.m_nMipmaps == 0 || header.m_nMipmaps > MAX_DTX_MIPMAPS)
	{
		if (error)
		{
			*error = "DTX mipmap count is invalid.";
		}
		stream->Release();
		return nullptr;
	}

	const uint32 mip_offset = 0;
	const uint32 base_width = std::max<uint32>(1, header.m_BaseWidth >> mip_offset);
	const uint32 base_height = std::max<uint32>(1, header.m_BaseHeight >> mip_offset);
	const uint32 mip_count = header.m_nMipmaps - mip_offset;
	const BPPIdent bpp = header.GetBPPIdent();
	const uint32 faces = (header.m_IFlags & DTX_CUBEMAP) ? 6u : 1u;

	auto* texture = new TextureData();
	if (!texture)
	{
		if (error)
		{
			*error = "Failed to allocate texture data.";
		}
		stream->Release();
		return nullptr;
	}

	texture->m_ResHeader.m_Type = LT_RESTYPE_DTX;
	texture->m_Header = header;
	texture->m_Header.m_BaseWidth = static_cast<uint16>(base_width);
	texture->m_Header.m_BaseHeight = static_cast<uint16>(base_height);
	texture->m_Header.m_nMipmaps = static_cast<uint16>(mip_count);
	texture->m_Header.m_nSections = 0;
	dtx_SetupDTXFormat2(bpp, &texture->m_PFormat);

	uint32 total_size = 0;
	uint32 width = base_width;
	uint32 height = base_height;
	for (uint32 mip_index = 0; mip_index < mip_count; ++mip_index)
	{
		total_size += CalcDtxImageSize(bpp, width, height);
		width = std::max<uint32>(1, width / 2);
		height = std::max<uint32>(1, height / 2);
	}

	texture->m_bufSize = total_size;
	texture->m_pDataBuffer = new uint8[total_size];
	if (!texture->m_pDataBuffer)
	{
		if (error)
		{
			*error = "Failed to allocate texture buffer.";
		}
		stream->Release();
		delete texture;
		return nullptr;
	}

	uint8* cursor = texture->m_pDataBuffer;
	width = base_width;
	height = base_height;
	for (uint32 mip_index = 0; mip_index < mip_count; ++mip_index)
	{
		const uint32 mip_size = CalcDtxImageSize(bpp, width, height);
		auto& mip = texture->m_Mips[mip_index];
		mip.m_Width = width;
		mip.m_Height = height;
		mip.m_Pitch = CalcDtxPitch(bpp, width, height);
		mip.m_dataSize = static_cast<int>(mip_size);
		mip.m_DataHeader = cursor;
		mip.m_Data = cursor;
		cursor += mip_size;

		width = std::max<uint32>(1, width / 2);
		height = std::max<uint32>(1, height / 2);
	}

	width = header.m_BaseWidth;
	height = header.m_BaseHeight;
	for (uint32 mip_index = 0; mip_index < header.m_nMipmaps; ++mip_index)
	{
		const uint32 mip_size = CalcDtxImageSize(bpp, width, height);
		for (uint32 face = 0; face < faces; ++face)
		{
			if (mip_index < mip_offset || face > 0)
			{
				if (SkipStreamBytes(stream, mip_size) != LT_OK)
				{
					if (error)
					{
						*error = "Failed to skip DTX mip data.";
					}
					stream->Release();
					delete texture;
					return nullptr;
				}
				continue;
			}

			if (stream->Read(texture->m_Mips[mip_index - mip_offset].m_Data, mip_size) != LT_OK)
			{
				if (error)
				{
					*error = "Failed to read DTX mip data.";
				}
				stream->Release();
				delete texture;
				return nullptr;
			}
		}

		width = std::max<uint32>(1, width / 2);
		height = std::max<uint32>(1, height / 2);
	}

	const LTRESULT status = stream->ErrorStatus();
	stream->Release();
	if (status != LT_OK)
	{
		if (error)
		{
			*error = "DTX stream error.";
		}
		delete texture;
		return nullptr;
	}

	return texture;
}
