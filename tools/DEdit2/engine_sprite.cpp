#include "lt_stream.h"

#include "sprite.h"
#include "client_filemgr.h"

#include <string>

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

	ILTStream* stream = OpenFileStream(pFilename->m_pFilename);
	if (!stream)
	{
		return LT_ERROR;
	}

	Sprite* sprite = spr_Create(stream);
	stream->Release();
	if (!sprite)
	{
		return LT_ERROR;
	}

	*ppSprite = sprite;
	return LT_OK;
}
