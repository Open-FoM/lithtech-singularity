#include "bdefs.h"
#include "ltassert.h"
#include "customfontfilemgr.h"

#include <algorithm>

#if defined(_WIN32)
#include "dsys_interface.h"

//IClientFileMgr
#include "client_filemgr.h"
static IClientFileMgr *client_filemgr;
define_holder(IClientFileMgr, client_filemgr);

#define LTARRAYSIZE(a)		(sizeof(a) / (sizeof((a)[0])))


//-------------------------------------------
// Utility functions
//-------------------------------------------

//this is a function that will broadcast to all applications that the fonts have changed. This should
//be done whenever a font is added or removed. This is primarily for Win9x support
static void BroadcastFontChangeMessage()
{
	static const uint32 knTimeoutMS = 30;
	DWORD_PTR nResult;
	SendMessageTimeout(HWND_BROADCAST, WM_FONTCHANGE, (WPARAM)0, (LPARAM)0, SMTO_ABORTIFHUNG | SMTO_BLOCK, knTimeoutMS, &nResult); 
}

//-------------------------------------------
//CCustomFontFile
//-------------------------------------------
CCustomFontFile::CCustomFontFile() :
	m_bExtracted(false)
{
	LTStrCpy(m_pszFilename, "", LTARRAYSIZE(m_pszFilename));
	LTStrCpy(m_pszFamilyName, "", LTARRAYSIZE(m_pszFamilyName));
}

CCustomFontFile::~CCustomFontFile()
{
}

//-------------------------------------------
//CCustomFontFileMgr
//-------------------------------------------
CCustomFontFileMgr::CCustomFontFileMgr()
{
}

CCustomFontFileMgr::~CCustomFontFileMgr()
{
	//we need to ensure that we do not have any font files left over

	if ( !m_FileList.empty() )
	{
		DEBUG_PRINT ( 1, ("Error: Custom font files were not properly freed") );
	}

	//and now free them
	while(!m_FileList.empty())
	{
		UnregisterCustomFontFile(*(m_FileList.begin()));
	}
}

//provides access to the custom font file manager singleton
CCustomFontFileMgr& CCustomFontFileMgr::GetSingleton()
{
	static CCustomFontFileMgr sSingleton;
	return sSingleton;
}



//called to register a custom font file. This will return NULL on an error
CCustomFontFile* CCustomFontFileMgr::RegisterCustomFontFile(const char* pszRelResource)
{
	//make sure the parameters are valid
	if(!pszRelResource)
		return NULL;

	//allocate our resource that will hold onto this
	CCustomFontFile* pNewFile;
	LT_MEM_TRACK_ALLOC(pNewFile = new CCustomFontFile, LT_MEM_TYPE_UI);

	//bail if out of memory
	if(!pNewFile)
		return NULL;


	//determine the absolute file name that is not within a rez file
	if( GetOrCopyClientFile(pszRelResource, pNewFile->m_pszFilename, LTARRAYSIZE(pNewFile->m_pszFilename), pNewFile->m_bExtracted) != LT_OK )
	{
		DEBUG_PRINT( 1, ("Warning: Failed to extract the font from the resouce file or find the font file"));

		//we failed to extract the client file
		delete pNewFile;
		return NULL;
	}

	//we now have the name of the file, we therefore need to register this font with windows

	//Note that this is the ideal version, but this is not supported on Win9x
	//int nNumFontsAdded = AddFontResourceEx(pNewFile->GetFilename(), FR_PRIVATE, NULL);

	//this is the alternate version that is supported on 9x, but must broadcast to other applications
	//that the fonts have changed and they have access to this font
	int nNumFontsAdded = AddFontResource(pNewFile->GetFilename());

	if(nNumFontsAdded < 1)
	{
		DEBUG_PRINT ( 1, ("Warning: Windows failed to properly add the specified font"));

		//windows failed to add this font resource
		delete pNewFile;
		return NULL;
	}

	//another Win9x task, is to make sure that all applications are notified that we have changed the fonts
	BroadcastFontChangeMessage();

	//the file has now been properly setup, so add it to our list
	m_FileList.push_back(pNewFile);

	//and return our file reference
	return pNewFile;
}

//called to unregister a custom font file. This will return false if the custom font file
//is not valid. Note that this will delete the font file so the pointer must not be used
//after this call.
bool CCustomFontFileMgr::UnregisterCustomFontFile(CCustomFontFile* pFontFile)
{
	//make sure the parameters are valid
	if(!pFontFile)
		return false;

	//find this file in our list
	TCustomFontFileList::iterator itFile = std::find(m_FileList.begin(), m_FileList.end(), pFontFile);

	//now make sure that this is within the list
	if(itFile == m_FileList.end())
	{
		//doesn't come from our list!
		DEBUG_PRINT(1, ( "Error: Attempted to remove a custom font file that wasn't in the list. Most likely releasing the same resource twice."));
		return false;
	}

	//remove this file from our list
	m_FileList.erase(itFile);

	//unregister this font from windows. This must use the same flags as it was registered with, again this
	//is the optimal version, but the other version must be used for Win9x support
	//if(!RemoveFontResourceEx(pFontFile->GetFilename(), FR_PRIVATE, NULL))

	if(!RemoveFontResource(pFontFile->GetFilename()))
	{
		//it failed. We can't really do anything about this, but throw an assert anyway
		DEBUG_PRINT ( 1, ( "Warning: Windows failed to remove the font resource"));
	}

	//another Win9x task, is to make sure that all applications are notified that we have changed the fonts
	BroadcastFontChangeMessage();

	//and now try and remove this file if it was extracted out
	if(pFontFile->IsExtracted())
	{
		//handle deleting the custom font file
		DeleteFile( pFontFile->GetFilename());
	}

	//and delete the file
	delete pFontFile;
	pFontFile = NULL;

	//and success
	return true;
}

const char* CCustomFontFileMgr::FindFontFileByFamily(const char* pszFamily) const
{
	(void)pszFamily;
	return NULL;
}

#else  // !_WIN32

#include "SDL3/SDL_error.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "client_filemgr.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

#ifndef LTARRAYSIZE
#define LTARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

//IClientFileMgr
static IClientFileMgr *client_filemgr;
define_holder(IClientFileMgr, client_filemgr);

namespace
{
bool EnsureTtfInit()
{
	static bool s_inited = false;
	if (!s_inited)
	{
		if (TTF_Init() != 0)
		{
			DEBUG_PRINT(1, ("TTF_Init failed: %s", SDL_GetError()));
			return false;
		}
		s_inited = true;
	}
	return true;
}

void NormalizeFamilyName(const char* src, char* dest, size_t dest_len)
{
	if (!dest_len)
	{
		return;
	}

	if (!src)
	{
		dest[0] = '\0';
		return;
	}

	size_t out = 0;
	for (; src[out] && out + 1 < dest_len; ++out)
	{
		dest[out] = static_cast<char>(std::tolower(static_cast<unsigned char>(src[out])));
	}
	dest[out] = '\0';
}

bool FileExists(const char* path)
{
	return path && (access(path, R_OK) == 0);
}

bool ExtractClientFile(const char* pszRelResource, char* pszOutName, size_t outLen, bool& bExtracted)
{
	bExtracted = false;

	if (!pszRelResource || !pszOutName || outLen == 0)
	{
		return false;
	}

	if (FileExists(pszRelResource))
	{
		LTStrCpy(pszOutName, pszRelResource, outLen);
		return true;
	}

	if (!client_filemgr)
	{
		return false;
	}

	FileRef file_ref;
	file_ref.m_FileType = FILE_ANYFILE;
	file_ref.m_pFilename = pszRelResource;

	ILTStream* stream = client_filemgr->OpenFile(&file_ref);
	if (!stream)
	{
		return false;
	}

	uint32 length = 0;
	if (stream->GetLen(&length) != LT_OK || length == 0)
	{
		stream->Release();
		return false;
	}

	std::string temp_template = "/tmp/ltjs_font_XXXXXX";
	int fd = mkstemp(&temp_template[0]);
	if (fd == -1)
	{
		stream->Release();
		return false;
	}

	std::string buffer;
	buffer.resize(length);
	if (stream->Read(buffer.data(), length) != LT_OK)
	{
		close(fd);
		unlink(temp_template.c_str());
		stream->Release();
		return false;
	}

	stream->Release();

	ssize_t written = write(fd, buffer.data(), length);
	close(fd);
	if (written != static_cast<ssize_t>(length))
	{
		unlink(temp_template.c_str());
		return false;
	}

	LTStrCpy(pszOutName, temp_template.c_str(), outLen);
	bExtracted = true;
	return true;
}

bool GetFontFamilyName(const char* font_path, char* family_out, size_t family_len)
{
	if (!EnsureTtfInit())
	{
		return false;
	}

	TTF_Font* font = TTF_OpenFont(font_path, 16);
	if (!font)
	{
		return false;
	}

	const char* family = TTF_GetFontFamilyName(font);
	if (family && family_out && family_len > 0)
	{
		NormalizeFamilyName(family, family_out, family_len);
	}
	else if (family_out && family_len > 0)
	{
		family_out[0] = '\0';
	}

	TTF_CloseFont(font);
	return true;
}
} // namespace

//-------------------------------------------
//CCustomFontFile
//-------------------------------------------
CCustomFontFile::CCustomFontFile() :
	m_bExtracted(false)
{
	LTStrCpy(m_pszFilename, "", LTARRAYSIZE(m_pszFilename));
	LTStrCpy(m_pszFamilyName, "", LTARRAYSIZE(m_pszFamilyName));
}

CCustomFontFile::~CCustomFontFile()
{
}

//-------------------------------------------
//CCustomFontFileMgr
//-------------------------------------------
CCustomFontFileMgr::CCustomFontFileMgr()
{
}

CCustomFontFileMgr::~CCustomFontFileMgr()
{
	if (!m_FileList.empty())
	{
		DEBUG_PRINT(1, ("Error: Custom font files were not properly freed"));
	}

	while (!m_FileList.empty())
	{
		UnregisterCustomFontFile(*(m_FileList.begin()));
	}
}

CCustomFontFileMgr& CCustomFontFileMgr::GetSingleton()
{
	static CCustomFontFileMgr sSingleton;
	return sSingleton;
}

CCustomFontFile* CCustomFontFileMgr::RegisterCustomFontFile(const char* pszRelResource)
{
	if (!pszRelResource)
	{
		return NULL;
	}

	CCustomFontFile* pNewFile;
	LT_MEM_TRACK_ALLOC(pNewFile = new CCustomFontFile, LT_MEM_TYPE_UI);
	if (!pNewFile)
	{
		return NULL;
	}

	if (!ExtractClientFile(pszRelResource, pNewFile->m_pszFilename, LTARRAYSIZE(pNewFile->m_pszFilename), pNewFile->m_bExtracted))
	{
		DEBUG_PRINT(1, ("Warning: Failed to extract the font from the resource file or find the font file"));
		delete pNewFile;
		return NULL;
	}

	if (!GetFontFamilyName(pNewFile->GetFilename(), pNewFile->m_pszFamilyName, LTARRAYSIZE(pNewFile->m_pszFamilyName)))
	{
		DEBUG_PRINT(1, ("Warning: Failed to read font family name for %s", pNewFile->GetFilename()));
		pNewFile->m_pszFamilyName[0] = '\0';
	}

	m_FileList.push_back(pNewFile);
	return pNewFile;
}

bool CCustomFontFileMgr::UnregisterCustomFontFile(CCustomFontFile* pFontFile)
{
	if (!pFontFile)
	{
		return false;
	}

	TCustomFontFileList::iterator itFile = std::find(m_FileList.begin(), m_FileList.end(), pFontFile);
	if (itFile == m_FileList.end())
	{
		DEBUG_PRINT(1, ("Error: Attempted to remove a custom font file that wasn't in the list. Most likely releasing the same resource twice."));
		return false;
	}

	m_FileList.erase(itFile);

	if (pFontFile->IsExtracted())
	{
		unlink(pFontFile->GetFilename());
	}

	delete pFontFile;
	return true;
}

const char* CCustomFontFileMgr::FindFontFileByFamily(const char* pszFamily) const
{
	if (!pszFamily || !*pszFamily)
	{
		return NULL;
	}

	char normalized[128];
	NormalizeFamilyName(pszFamily, normalized, LTARRAYSIZE(normalized));

	for (const auto* font_file : m_FileList)
	{
		if (font_file && std::strcmp(font_file->GetFamilyName(), normalized) == 0)
		{
			return font_file->GetFilename();
		}
	}

	return NULL;
}

#endif // _WIN32
