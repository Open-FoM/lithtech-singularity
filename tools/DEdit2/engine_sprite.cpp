#include "engine_render.h"
#include "lt_stream.h"

#include "de_sprite.h"
#include "iltstream.h"
#include "iltspritecontrol.h"
#include "ltbasedefs.h"
#include "renderstruct.h"
#include "client_filemgr.h"

#include <cstring>
#include <string>

extern RenderStruct g_Render;

namespace
{
void FreeSprite(Sprite* sprite)
{
	if (!sprite)
	{
		return;
	}

	if (sprite->m_Anims)
	{
		for (uint32 i = 0; i < sprite->m_nAnims; ++i)
		{
			delete[] sprite->m_Anims[i].m_Frames;
			sprite->m_Anims[i].m_Frames = nullptr;
		}
		delete[] sprite->m_Anims;
		sprite->m_Anims = nullptr;
	}

	delete sprite;
}
} // namespace

LTRESULT LoadSprite(FileRef* pFilename, Sprite** ppSprite)
{
	if (!ppSprite)
	{
		return LT_ERROR;
	}

	*ppSprite = nullptr;
	if (!pFilename || !pFilename->m_pFilename || pFilename->m_pFilename[0] == '\0')
	{
		return LT_ERROR;
	}

	std::string path;
	if (TextureCache* cache = GetEngineTextureCache())
	{
		path = cache->ResolveResourcePath(pFilename->m_pFilename, ".spr");
	}
	if (path.empty())
	{
		path = pFilename->m_pFilename;
	}

	ILTStream* stream = OpenFileStream(path);
	if (!stream)
	{
		return LT_ERROR;
	}

	Sprite* sprite = new Sprite();
	sprite->m_nAnims = 1;
	sprite->m_Anims = new SpriteAnim[1]{};
	std::strncpy(sprite->m_Anims[0].m_sName, "Untitled", sizeof(sprite->m_Anims[0].m_sName) - 1);

	ILTStream* pStream = stream;
	uint32 nFrames = 0;
	uint32 nFrameRate = 0;
	uint32 bTransparent = 0;
	uint32 bTranslucent = 0;
	uint32 colourKey = 0;

	STREAM_READ(nFrames);
	STREAM_READ(nFrameRate);
	STREAM_READ(bTransparent);
	STREAM_READ(bTranslucent);
	STREAM_READ(colourKey);

	if (nFrames == 0 || nFrames > 100000)
	{
		FreeSprite(sprite);
		stream->Release();
		return LT_ERROR;
	}

	SpriteAnim& anim = sprite->m_Anims[0];
	anim.m_nFrames = nFrames;
	anim.m_MsFrameRate = nFrameRate;
	anim.m_MsAnimLength = (nFrameRate != 0) ? ((1000 / nFrameRate) * nFrames) : 0;
	anim.m_bKeyed = static_cast<LTBOOL>(bTransparent);
	anim.m_bTranslucent = static_cast<LTBOOL>(bTranslucent);
	anim.m_ColourKey = colourKey;
	anim.m_Frames = new SpriteEntry[nFrames]{};

	for (uint32 i = 0; i < nFrames; ++i)
	{
		uint16 strLen = 0;
		STREAM_READ(strLen);
		if (strLen == 0 || strLen > 1024)
		{
			FreeSprite(sprite);
			stream->Release();
			return LT_ERROR;
		}

		std::string frameName(strLen, '\0');
		if (pStream->Read(frameName.data(), strLen) != LT_OK)
		{
			FreeSprite(sprite);
			stream->Release();
			return LT_ERROR;
		}

		SharedTexture* texture = nullptr;
		if (g_Render.GetSharedTexture)
		{
			texture = g_Render.GetSharedTexture(frameName.c_str());
		}
		anim.m_Frames[i].m_pTex = texture;
	}

	if (pStream->ErrorStatus() != LT_OK)
	{
		FreeSprite(sprite);
		stream->Release();
		return LT_ERROR;
	}

	stream->Release();
	*ppSprite = sprite;
	return LT_OK;
}

void spr_InitTracker(SpriteTracker* pTracker, Sprite* pSprite)
{
	if (!pTracker || !pSprite || !pSprite->m_Anims)
	{
		return;
	}

	pTracker->m_pSprite = pSprite;
	pTracker->m_pCurAnim = &pSprite->m_Anims[0];
	if (pTracker->m_pCurAnim->m_nFrames > 0)
	{
		pTracker->m_pCurFrame = &pTracker->m_pCurAnim->m_Frames[0];
	}
	else
	{
		pTracker->m_pCurFrame = nullptr;
	}

	pTracker->m_MsCurTime = 0;
	pTracker->m_Flags = SC_PLAY | SC_LOOP;
}
