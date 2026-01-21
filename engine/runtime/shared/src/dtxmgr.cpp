
#include "bdefs.h"
#include "dtxmgr.h"
#include "render.h"

int g_dtxInMemSize = 0;

//Texture groups, used to control offsetting of Mip Maps when they are loaded, thus allowing
//us to not waste any memory loading mips higher than those that we are going to use. Note that
//this define needs to match the one in the engine vars code. This is ugly, but there is no
//shared header.
#define MAX_TEXTURE_GROUPS 4
extern int32	g_CV_TextureMipMapOffset;
extern int32	g_CV_TextureGroupOffset[MAX_TEXTURE_GROUPS];


// --------------------------------------------------------------------------------- //
// TextureMipData.
// --------------------------------------------------------------------------------- //

TextureMipData::TextureMipData()
{
	m_Width = 0;
	m_Height = 0;
	m_Pitch = 0;
	m_Data = LTNULL;
}


// --------------------------------------------------------------------------------- //
// TextureData functions.
// --------------------------------------------------------------------------------- //

TextureData::TextureData()
{
	m_pSharedTexture	= NULL;
	m_pDataBuffer		= NULL;
    m_bufSize			= 0;
}


TextureData::~TextureData()
{
	if(m_Header.m_IFlags & DTX_MIPSALLOCED)
	{
		for(uint32 i=0; i < m_Header.m_nMipmaps; i++)
		{
            if(m_Mips[i].m_Data) 
			{
                g_dtxInMemSize -= m_Mips[i].m_dataSize;
				dfree(m_Mips[i].m_Data);
            }
		}
	}

    if(m_pDataBuffer) 
	{
		delete[] m_pDataBuffer;
        g_dtxInMemSize -= m_bufSize;
        m_bufSize = 0;
    }
}


// --------------------------------------------------------------------------------- //
// dtx_ functions.
// --------------------------------------------------------------------------------- //

// Based on the value of bSkip, it either memset()s the memory to 0 or reads it from the file.
static void dtx_ReadOrSkip(
	LTBOOL bSkip,
	ILTStream *pStream,
	void *pData,
	uint32 dataLen)
{
	if(bSkip)
	{
		pStream->SeekTo(pStream->GetPos() + dataLen);
	}
	else
	{
		pStream->Read(pData, dataLen);
	}
}

TextureData* dtx_Alloc(BPPIdent bpp, uint32 baseWidth, uint32 baseHeight, uint32 nMipmaps,
	uint32 *pAllocSize, uint32 *pTextureDataSize, uint32 iFlags)
{
	TextureData *pRet;
	TextureMipData *pMip;
	uint32 i, size, width, height;
	uint32 textureDataSize;
	uint8 *pOutData;

	textureDataSize = 0;
	width = baseWidth;
	height = baseHeight;

	uint32 dataHeaderSize = 0;

	for (i=0; i < nMipmaps; i++) 
	{
		size = CalcImageSize(bpp, width, height) + dataHeaderSize;;
		if (iFlags & DTX_CUBEMAP) 
		{ 
			size *= 6; 
        }

		textureDataSize += size;
		width  >>= 1;
		height >>= 1; 
	}

	LT_MEM_TRACK_ALLOC(pRet = new TextureData,LT_MEM_TYPE_TEXTURE);
	if (!pRet) 
		return LTNULL;

	LT_MEM_TRACK_ALLOC(pRet->m_pDataBuffer = new uint8[textureDataSize],LT_MEM_TYPE_TEXTURE);
	if (!pRet->m_pDataBuffer) 
	{
		delete pRet; 
		return LTNULL; 
	}

    pRet->m_bufSize = textureDataSize;
    g_dtxInMemSize += textureDataSize;

	pRet->m_ResHeader.m_Type = LT_RESTYPE_DTX;
	pRet->m_Header.m_ResType = LT_RESTYPE_DTX;
	pRet->m_Header.m_IFlags = DTX_SECTIONSFIXED;
	pRet->m_Header.m_UserFlags = 0;
	pRet->m_Header.m_nMipmaps = (uint16)nMipmaps;
	pRet->m_Header.m_BaseWidth = (uint16)baseWidth;
	pRet->m_Header.m_BaseHeight = (uint16)baseHeight;
	pRet->m_Header.m_Version = CURRENT_DTX_VERSION;
	pRet->m_Header.m_CommandString[0] = '\0';

	pRet->m_AllocSize = sizeof(TextureData) + textureDataSize;
	pRet->m_Link.m_pData = pRet;
	pRet->m_Header.m_nSections = 0;
	pRet->m_Header.m_ExtraLong[0] = pRet->m_Header.m_ExtraLong[1] = pRet->m_Header.m_ExtraLong[2] = 0;
	pRet->m_Header.SetBPPIdent(bpp);	//! moved here by jyl, this line used to be located
										//  before the line that set's m_ExtraLong[...] to 0
										//  which nulls out the call to SetBPPIdent.
	pRet->m_pSharedTexture = LTNULL;
	pRet->m_Flags = 0;

	// Setup the mipmap structures.
	pOutData = pRet->m_pDataBuffer;

	width = baseWidth;
	height = baseHeight;
	for (i=0; i < nMipmaps; i++) 
	{
		pMip = &pRet->m_Mips[i];

		pMip->m_Width = width;
		pMip->m_Height = height;
		pMip->m_DataHeader = pOutData;
		pMip->m_Data = pOutData + dataHeaderSize;
		pMip->m_dataSize = CalcImageSize(bpp, width, height);

		switch( bpp ) {
		case BPP_32:
		  pMip->m_Pitch = (int32)width * sizeof(uint32);
		  break;
		case BPP_32P:
		  pMip->m_Pitch = width;	//  BPP_32P: texture w/ true color pallete
		  break;
		case BPP_16:
		  pMip->m_Pitch = (int32)width * sizeof(uint16);
		  break;
		default:
		  pMip->m_Pitch = 0;
		  break;
		}

		size = pMip->m_dataSize + dataHeaderSize;
		if (iFlags & DTX_CUBEMAP) 
		{ 
			size *= 6; 
		}

		pOutData += size;

		width >>= 1; height >>= 1; 
	}

	if (pAllocSize) 
		*pAllocSize = pRet->m_AllocSize;

	if (pTextureDataSize) 
		*pTextureDataSize = textureDataSize;

	return pRet;
}

LTRESULT dtx_Create(ILTStream *pStream, TextureData **ppOut, uint32& nBaseWidth, uint32& nBaseHeight)
{
	TextureData *pRet;
	uint32 allocSize, textureDataSize;
	

	DtxHeader hdr;
	STREAM_READ(hdr);
	if (hdr.m_ResType != LT_RESTYPE_DTX) 
		RETURN_ERROR(1, dtx_Create, LT_INVALIDDATA);

	// Correct version and valid data?
	if (hdr.m_Version != CURRENT_DTX_VERSION) 
		RETURN_ERROR(1, dtx_Create, LT_INVALIDVERSION);

	if (hdr.m_nMipmaps == 0 || hdr.m_nMipmaps > MAX_DTX_MIPMAPS) 
		RETURN_ERROR(1, dtx_Create, LT_INVALIDDATA);

	
	//figure out the dimensions that we are going to want our texture to start out at
	uint32 nMipOffset = (uint32)LTMAX(0, g_CV_TextureMipMapOffset);
	if(hdr.GetTextureGroup() < MAX_TEXTURE_GROUPS)
	{
		nMipOffset = (uint32)LTMAX(0, g_CV_TextureMipMapOffset + g_CV_TextureGroupOffset[hdr.GetTextureGroup()]);
	}

	//clamp our mipmap offset to the number of mipmaps that we actually have
	nMipOffset = LTMIN(hdr.m_nMipmaps - 1, nMipOffset);

	nBaseWidth  = hdr.m_BaseWidth;
	nBaseHeight = hdr.m_BaseHeight;

	//now we need to determine our dimensions
	uint32 nTexWidth  = hdr.m_BaseWidth  / (1 << nMipOffset);
	uint32 nTexHeight = hdr.m_BaseHeight / (1 << nMipOffset);
	uint32 nNumMips	  = hdr.m_nMipmaps - nMipOffset;

	TextureMipData* pMip = nullptr;
	uint32 y = 0;
	uint32 size = 0;

	// Allocate it.
	pRet = dtx_Alloc(hdr.GetBPPIdent(), nTexWidth, nTexHeight, nNumMips, &allocSize, &textureDataSize);
	if (!pRet) RETURN_ERROR(1, dtx_Create, LT_OUTOFMEMORY);

	dtx_SetupDTXFormat2(pRet->m_Header.GetBPPIdent(), &pRet->m_PFormat);

	pRet->m_pSharedTexture = LTNULL;
	memcpy(&pRet->m_Header, &hdr, sizeof(DtxHeader));

	// Restore dimensions to match allocated data
	pRet->m_Header.m_BaseWidth = static_cast<uint16>(nTexWidth);
	pRet->m_Header.m_BaseHeight = static_cast<uint16>(nTexHeight);
	pRet->m_Header.m_nMipmaps = static_cast<uint16>(nNumMips);

	// Read in mipmap data.
	for (uint32 iMipmap = 0; iMipmap < hdr.m_nMipmaps; iMipmap++)
	{
		// Skip mipmaps before offset (but still read past them in stream)
		bool bSkipImageData = (iMipmap < nMipOffset);

		// Only access allocated mip data for mipmaps we're keeping
		if (!bSkipImageData)
		{
			pMip = &pRet->m_Mips[iMipmap - nMipOffset];
		}

		// Calculate size based on original header dimensions at this mip level
		uint32 mipWidth = hdr.m_BaseWidth >> iMipmap;
		uint32 mipHeight = hdr.m_BaseHeight >> iMipmap;

		if (hdr.GetBPPIdent() == BPP_32)
		{
			size = mipWidth * sizeof(uint32);
			for (y = 0; y < mipHeight; y++)
			{
				if (bSkipImageData)
				{
					pStream->SeekTo(pStream->GetPos() + size);
				}
				else
				{
					pStream->Read(&pMip->m_Data[y * pMip->m_Pitch], size);
				}
			}
		}
		else
		{
			size = CalcImageSize(hdr.GetBPPIdent(), mipWidth, mipHeight);
			dtx_ReadOrSkip(bSkipImageData, pStream, bSkipImageData ? nullptr : pMip->m_Data, size);
		}
	}
	
	//don't bother loading in the sections
	pRet->m_Header.m_nSections = 0;

	// Check the error status.
	if (pStream->ErrorStatus() != LT_OK) 
	{
		dtx_Destroy(pRet);
		RETURN_ERROR(1, dtx_Create, LT_INVALIDDATA); 
	}

	*ppOut = pRet;
	return LT_OK;
}


void dtx_Destroy(TextureData *pTextureData)
{
	if(pTextureData)
	{
		delete pTextureData;
	}
}

// Warning: This isn't very reliable - assumes either 32 bit or compressed...
void dtx_SetupDTXFormat2(BPPIdent bpp, PFormat *pFormat)
{
	if (bpp == BPP_32) 
	{
		pFormat->InitPValueFormat(); 
	}
	else 
	{
		pFormat->Init(bpp, 0, 0, 0, 0); 
	}
}
