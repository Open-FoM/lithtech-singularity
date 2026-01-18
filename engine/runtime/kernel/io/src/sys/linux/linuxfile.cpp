// ------------------------------------------------------------------ //
// Includes..
// ------------------------------------------------------------------ //

#include <sys/stat.h>
#include <cstdint>

#include "bdefs.h"
#include <string.h>
//#include <io.h>
#include <stdio.h>
#include <dirent.h>
#include "sysfile.h"
#include "de_memory.h"
#include "dutil.h"
//#include "dsys_interface.h"
#include "syslthread.h"
#include "syscounter.h"
#include "rezmgr.h"
#include "genltstream.h"

// define this if you want all RezMgr reads to be made thread safe!
// (This currently doesn't make anythnig else thread safe including using a Stream in two 
//  threads at once!)
#define LT_FILE_THREADSAFE

// console output of file access
// 0 - no output (default)
// 1 - display file open calls
// 2 - display file open and close calls
// 3 - display file open and close calls
// 4 - display file open and close and read calls
extern int32 g_CV_ShowFileAccess;

// PlayDemo profile info.
uint32 g_PD_FOpen=0;

typedef struct FileTree_t
{
	TreeType	m_TreeType;
	CRezMgr*	m_pRezMgr;
#ifdef LT_FILE_THREADSAFE
	LCriticalSection m_CriticalSection;
#endif
	CRezItm*	m_pLastRezItm;
	uint32		m_nLastRezPos;
	char		m_BaseName[1];
} FileTree;


class BaseFileStream : public CGenLTStream
{
public:

		BaseFileStream()
		{
			m_FileLen = 0;
			m_SeekOffset = 0;
			m_pFile = LTNULL;
			m_pTree = LTNULL;
			m_ErrorStatus = 0;
			m_nNumReadCalls = 0;
			m_nTotalBytesRead = 0;
		}

	virtual	~BaseFileStream()
	{
		if (g_CV_ShowFileAccess >= 2)
		{
			dsi_ConsolePrint("close stream %p num reads = %u bytes read = %u", 
				this, m_nNumReadCalls, m_nTotalBytesRead);
		}

		if(m_pTree->m_TreeType == UnixTree && m_pFile)
		{
			fclose(m_pFile);
		}
	}

	LTRESULT ErrorStatus()
	{
		return m_ErrorStatus ? LT_ERROR : LT_OK;
	}

	LTRESULT GetLen(uint32 *len)
	{
		*len = m_FileLen;
		return LT_OK;
	}

	LTRESULT Write(const void *pData, uint32 dataLen)
	{
		// Can't write to these streams.
		return LT_ERROR;
	}

	unsigned long	m_FileLen;		// Stored when the file is opened.
	unsigned long	m_SeekOffset;	// Seek offset (used in rezfiles) (NOTE: in a rezmgr rez file this is the current position inside the resource).
	FILE			*m_pFile;
	struct FileTree_t	*m_pTree;
	int				m_ErrorStatus;
	unsigned long	m_nNumReadCalls;
	unsigned long	m_nTotalBytesRead;
};


class UnixFileStream : public BaseFileStream
{
public:

	void	Release();

	LTRESULT GetPos(uint32 *pos)
	{
		*pos = (uint32)(ftell(m_pFile) - m_SeekOffset);
		return LT_OK;
	}
	
	LTRESULT SeekTo(uint32 offset)
	{
		long ret;

		ret = fseek(m_pFile, offset + m_SeekOffset, SEEK_SET);
		if(ret == 0)
		{
			return LT_OK;
		}
		else
		{
			m_ErrorStatus = 1;
			return LT_ERROR;
		}
	}

	LTRESULT Read(void *pData, uint32 size)
	{
		size_t sizeRead;

		if (g_CV_ShowFileAccess >= 4)
		{
			dsi_ConsolePrint("read %u bytes from stream %p",size, this);
		}

		if(m_ErrorStatus == 1)
		{
			memset(pData, 0, size);
			return LT_ERROR;
		}

		if(size != 0)
		{
			sizeRead = fread(pData, 1, size, m_pFile);
			if(sizeRead == size)
			{
				m_nNumReadCalls++;
				m_nTotalBytesRead += sizeRead;
				return LT_OK;
			}
			else
			{			
				memset(pData, 0, size);
				m_ErrorStatus = 1;
				return LT_ERROR;
			}
		}

		return LT_OK;
	}
};

class RezFileStream : public BaseFileStream
{
public:
	RezFileStream() :
		m_pRezItm(LTNULL)
	{
	}

	void	Release();

	LTRESULT GetPos(uint32 *pos)
	{
		*pos = m_SeekOffset;
		return LT_OK;
	}

	LTRESULT SeekTo(uint32 offset)
	{
		if (m_pRezItm->Seek(offset))
		{
			m_SeekOffset = offset;
			return LT_OK;
		}
		else
		{
			m_ErrorStatus = 1;
			return LT_ERROR;
		}
	}

	LTRESULT Read(void *pData, uint32 size)
	{
		size_t sizeRead;

		if (size != 0)
		{
#ifdef LT_FILE_THREADSAFE
			CSAccess access(&m_pTree->m_CriticalSection);
#endif

			if ((m_pTree->m_pLastRezItm == m_pRezItm) && (m_pTree->m_nLastRezPos == m_SeekOffset))
			{
				sizeRead = m_pRezItm->Read(pData, size);
			}
			else
			{
				sizeRead = m_pRezItm->Read(pData, size, m_SeekOffset);
			}

			m_SeekOffset += sizeRead;
			m_pTree->m_pLastRezItm = m_pRezItm;
			m_pTree->m_nLastRezPos = m_SeekOffset;
			if (sizeRead != size)
			{
				memset(pData, 0, size);
				m_ErrorStatus = 1;
				return LT_ERROR;
			}
		}
		return LT_OK;
	}

	CRezItm* m_pRezItm;
};

static ObjectBank<UnixFileStream> g_UnixFileStreamBank(8, 8);
static ObjectBank<RezFileStream> g_RezFileStreamBank(8, 8);


void UnixFileStream::Release()
{
	g_UnixFileStreamBank.Free(this);
}

void RezFileStream::Release()
{
	g_RezFileStreamBank.Free(this);
}

// LTFindInfo::m_pInternal..
typedef struct LTFindData
{
	FileTree		*m_pTree;
	long			m_Handle;
	_finddata_t		m_Data;

	CRezDir*		m_pCurDir;
	CRezTyp*		m_pCurTyp;
	CRezItm*		m_pCurItm;
	CRezDir*		m_pCurSubDir;
};



// ------------------------------------------------------------------ //
// Interface functions.
// ------------------------------------------------------------------ //

void df_Init()
{
}

void df_Term()
{
}

int df_OpenTree(const char *pName, HLTFileTree *&pTreePointer)
{
	long allocSize;
	FileTree *pTree;
	struct stat info;
	
	pTreePointer = NULL;

	// See if it exists..
	if (stat(pName,&info) != 0)
		return -1;

	allocSize = sizeof(FileTree) + strlen(pName);
	if (S_ISDIR(info.st_mode))
	{
		pTree = (FileTree*)dalloc_z(allocSize);
		pTree->m_TreeType = UnixTree;
		pTree->m_pRezMgr = LTNULL;
		pTree->m_pLastRezItm = LTNULL;
		pTree->m_nLastRezPos = 0;
	}
	else
	{
		LT_MEM_TRACK_ALLOC(pTree = (FileTree*)dalloc_z(allocSize),LT_MEM_TYPE_FILE);
		pTree->m_TreeType = RezFileTree;
		pTree->m_pLastRezItm = LTNULL;
		pTree->m_nLastRezPos = 0;

		LT_MEM_TRACK_ALLOC(pTree->m_pRezMgr = new CRezMgr,LT_MEM_TYPE_FILE);
		if (pTree->m_pRezMgr == LTNULL)
			return -2;

		if(!pTree->m_pRezMgr->Open(pName))
		{
			delete pTree->m_pRezMgr;
			dfree(pTree);
			return -2;
		}

		pTree->m_pRezMgr->SetDirSeparators("/\\");
	}

	strcpy(pTree->m_BaseName, pName);
	pTreePointer = (HLTFileTree*)pTree;
	return 0;
}


void df_CloseTree(HLTFileTree* hTree)
{
	FileTree *pTree;

	pTree = (FileTree*)hTree;
	if(!pTree)
		return;

	if (pTree->m_pRezMgr != LTNULL)
	{
		delete pTree->m_pRezMgr;
		pTree->m_pRezMgr = LTNULL;
	}

	dfree(pTree);
}


TreeType df_GetTreeType(HLTFileTree* hTree)
{
	FileTree *pTree;

	pTree = (FileTree*)hTree;
	if(!pTree)
		return UnixTree;

	return pTree->m_TreeType;
}


void df_BuildName(char *pPart1, char *pPart2, char *pOut)
{
	if(pPart1[0] == 0)
	{
		strcpy(pOut, pPart2);
	}
	else
	{
		sprintf(pOut, "%s/%s", pPart1, pPart2);
	}
}


int df_GetFileInfo(HLTFileTree* hTree, const char *pName, LTFindInfo *pInfo)
{
	FileTree *pTree;
	char fullName[500];
	CRezItm* pRezItm;
	_finddata_t data;
	std::intptr_t handle;
	std::intptr_t curRet;

	pTree = (FileTree*)hTree;
	if(!pTree)
		return false;

	if(pTree->m_TreeType == UnixTree)
	{
		LTSNPrintF(fullName, sizeof(fullName), "%s/%s", pTree->m_BaseName, pName);

		handle = _findfirst(fullName, &data);
		curRet = handle;
		while(curRet != -1)
		{
			if(!(data.attrib & _A_SUBDIR))
			{
				LTStrCpy(pInfo->m_Name, data.name, sizeof(pInfo->m_Name));
				pInfo->m_Type = FILE_TYPE;
				pInfo->m_Size = data.size;
				memcpy(&pInfo->m_Date, &data.time_write, 4);

				if(handle != -1)
				{
					_findclose(handle);
				}
				return true;
			}

			curRet = _findnext(handle, &data);
		}

		if(handle != -1)
		{
			_findclose(handle);
		}
		return false;
	}
	else
	{
		pRezItm = pTree->m_pRezMgr->GetRezFromDosPath(pName);
		if (pRezItm != LTNULL)
		{
			strncpy(pInfo->m_Name, pRezItm->GetName(), sizeof(pInfo->m_Name)-1);
			pInfo->m_Type = FILE_TYPE;
			pInfo->m_Size = pRezItm->GetSize();
			pInfo->m_Date = pRezItm->GetTime();
			return true;
		}
		else
		{
			return false;
		}
	}
}


int df_GetDirInfo(HLTFileTree hTree, char *pName)
{
	FileTree *pTree;
	char fullName[500];
	_finddata_t data;
	std::intptr_t handle;
	std::intptr_t curRet;
	CRezDir* pRezDir;

	pTree = (FileTree*)hTree;
	if(!pTree) return 0;

	if(pTree->m_TreeType == UnixTree)
	{
		LTSNPrintF(fullName, sizeof(fullName), "%s/%s", pTree->m_BaseName, pName);

		handle = _findfirst(fullName, &data);
		curRet = handle;
		while(curRet != -1)
		{
			if(data.attrib & _A_SUBDIR)
			{
				if(handle != -1)
				{
					_findclose(handle);
				}
				return 1;
			}

			curRet = _findnext(handle, &data);
		}

		if(handle != -1)
		{
			_findclose(handle);
		}
		return 0;
	}
	else
	{
		pRezDir = pTree->m_pRezMgr->GetDirFromPath(pName);
		return (pRezDir != LTNULL) ? 1 : 0;
	}
}


int df_GetFullFilename(HLTFileTree hTree, char *pName, char *pOutName, int maxLen)
{
	FileTree *pTree;

	pTree = (FileTree*)hTree;
	if(!pTree)
		return 0;

	if(pTree->m_TreeType != UnixTree)
		return 0;
	
	sprintf(pOutName, "%s/%s", pTree->m_BaseName, pName);
	return 1;
}


ILTStream* df_Open(HLTFileTree* hTree, const char *pName, int openMode)
{
	char fullName[500];
	FileTree *pTree;
	FILE *fp;
	UnixFileStream *pUnixStream;
	unsigned long seekOffset, fileLen;
	CRezItm* pRezItm;

	pTree = (FileTree*)hTree;
	if(!pTree)
		return NULL;

	if(pTree->m_TreeType == UnixTree)
	{
		sprintf(fullName, "%s/%s", pTree->m_BaseName, pName);
		CountAdder cntAdd(&g_PD_FOpen);
		if (! (fp = fopen(fullName, "rb")) )
			return NULL;

		fseek(fp, 0, SEEK_END);
		fileLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		seekOffset = 0;

		// Use fp to setup the stream.
		pUnixStream = g_UnixFileStreamBank.Allocate();
		pUnixStream->m_pFile = fp;
		pUnixStream->m_pTree = pTree;
		pUnixStream->m_FileLen = fileLen;
		pUnixStream->m_SeekOffset = seekOffset;

		if (g_CV_ShowFileAccess >= 1)
		{
			dsi_ConsolePrint("stream %p open file %s size = %u",pUnixStream,pName,fileLen);
		}

		return pUnixStream;
	}
	else
	{
		pRezItm = pTree->m_pRezMgr->GetRezFromDosPath(pName);
		if (pRezItm == LTNULL) return LTNULL;

		RezFileStream *pRezStream = g_RezFileStreamBank.Allocate();
		pRezStream->m_pRezItm = pRezItm;
		pRezStream->m_pTree = pTree;
		pRezStream->m_FileLen = pRezItm->GetSize();
		pRezStream->m_SeekOffset = 0;

		if (g_CV_ShowFileAccess >= 1)
		{
			dsi_ConsolePrint("stream %p open rez %s size = %u",pRezStream,pName,pRezItm->GetSize());
		}

		return pRezStream;
	}
}


int df_FindNext(HLTFileTree* hTree, const char *pDirName, LTFindInfo *pInfo)
{
	LTFindData *pFindData;
	char filter[400];
	FileTree *pTree;
	std::intptr_t curRet;
	CRezItm* pRezItm;
	CRezDir* pRezDir;
	CRezTyp* pRezTyp;
	CRezDir* pRezSubDir;
	char	 sItemType[5];

	if(!hTree)
		return 0;

	pTree = (FileTree*)hTree;
	if(pTree->m_TreeType == UnixTree)
	{
		if(!pInfo->m_pInternal)
		{
			LTSNPrintF(filter, sizeof(filter), "%s/%s/*.*", pTree->m_BaseName, pDirName);

			LT_MEM_TRACK_ALLOC(pFindData = (LTFindData*)dalloc(sizeof(LTFindData)),LT_MEM_TYPE_FILE);
			pFindData->m_pTree = pTree;
			pFindData->m_Handle = _findfirst(filter, &pFindData->m_Data);
			curRet = pFindData->m_Handle;

			pInfo->m_pInternal = pFindData;
		}
		else
		{
			pFindData = (LTFindData*)pInfo->m_pInternal;
			curRet = _findnext(pFindData->m_Handle, &pFindData->m_Data);
		}

		while(1)
		{
			if(curRet == -1)
			{
				pInfo->m_pInternal = LTNULL;

				if(pFindData->m_Handle != -1)
					_findclose(pFindData->m_Handle);

				dfree(pFindData);
				return 0;
			}

			if(pFindData->m_Data.name[0] == '.')
			{
				curRet = _findnext(pFindData->m_Handle, &pFindData->m_Data);
			}
			else
			{
				break;
			}
		}

		LTStrCpy(pInfo->m_Name, pFindData->m_Data.name, sizeof(pInfo->m_Name));
		memcpy(&pInfo->m_Date, &pFindData->m_Data.time_write, 4);
		pInfo->m_Size = pFindData->m_Data.size;
		pInfo->m_Type = (pFindData->m_Data.attrib & _A_SUBDIR) ? DIRECTORY_TYPE : FILE_TYPE;

		return 1;
	}
	else
	{
		if(pInfo->m_pInternal == LTNULL)
		{
			pRezDir = pTree->m_pRezMgr->GetDirFromPath(pDirName);
			if(pRezDir == LTNULL)
			{
				return 0;
			}

			pRezTyp = pRezDir->GetFirstType();
			if (pRezTyp != LTNULL)
			{
				pRezItm = pRezDir->GetFirstItem(pRezTyp);
			}
			else 
			{ 
				pRezItm = LTNULL;
			}

			if (pRezItm == LTNULL)
			{
				pRezSubDir = pRezDir->GetFirstSubDir();
				if (pRezSubDir == LTNULL)
				{
					return 0;
				}
			}
			else pRezSubDir = LTNULL;

			LT_MEM_TRACK_ALLOC(pFindData = (LTFindData*)dalloc(sizeof(LTFindData)),LT_MEM_TYPE_FILE);
			pInfo->m_pInternal = pFindData;

			pFindData->m_pTree = pTree;
			pFindData->m_pCurDir = pRezDir;
			pFindData->m_pCurTyp = pRezTyp;
			pFindData->m_pCurItm = pRezItm;
			pFindData->m_pCurSubDir = pRezSubDir;
		}
		else
		{
			pFindData = (LTFindData*)pInfo->m_pInternal;

			if (pFindData->m_pCurItm != LTNULL)
			{
				pFindData->m_pCurItm = pFindData->m_pCurDir->GetNextItem(pFindData->m_pCurItm);

				while(pFindData->m_pCurItm == LTNULL)
				{
					pFindData->m_pCurTyp = pFindData->m_pCurDir->GetNextType(pFindData->m_pCurTyp);
					if (pFindData->m_pCurTyp == LTNULL)
					{
						pFindData->m_pCurSubDir = pFindData->m_pCurDir->GetFirstSubDir();
						if (pFindData->m_pCurSubDir == LTNULL)
						{
							dfree(pInfo->m_pInternal);
							pInfo->m_pInternal = LTNULL;
							return 0;
						}
						break;
					}

					pFindData->m_pCurItm = pFindData->m_pCurDir->GetFirstItem(pFindData->m_pCurTyp);
				}
			}
			else
			{
				pFindData->m_pCurSubDir = pFindData->m_pCurDir->GetNextSubDir(pFindData->m_pCurSubDir);
				if (pFindData->m_pCurSubDir == LTNULL)
				{
					dfree(pInfo->m_pInternal);
					pInfo->m_pInternal = LTNULL;
					return 0;
				}
			}
		}

		if (pFindData->m_pCurItm != LTNULL)
		{
			LTStrCpy(pInfo->m_Name, pFindData->m_pCurItm->GetName(), sizeof(pInfo->m_Name)-4);
			pTree->m_pRezMgr->TypeToStr(pFindData->m_pCurItm->GetType(),sItemType);
			LTStrCat(pInfo->m_Name, ".", sizeof(pInfo->m_Name));
			LTStrCat(pInfo->m_Name, sItemType, sizeof(pInfo->m_Name));
			pInfo->m_Date = pFindData->m_pCurItm->GetTime();
			pInfo->m_Type = FILE_TYPE;
			pInfo->m_Size = pFindData->m_pCurItm->GetSize();
		}
		else if (pFindData->m_pCurSubDir != LTNULL)
		{
			LTStrCpy(pInfo->m_Name, pFindData->m_pCurSubDir->GetDirName(), sizeof(pInfo->m_Name));
			pInfo->m_Date = pFindData->m_pCurSubDir->GetTime();
			pInfo->m_Type = DIRECTORY_TYPE;
			pInfo->m_Size = 0;
		}
		else
		{
			return 0;
		}

		return 1;
	}
}


void df_FindClose(LTFindInfo *pInfo)
{
	LTFindData *pFindData;

	if(!pInfo || !pInfo->m_pInternal)
		return;

	pFindData = (LTFindData*)pInfo->m_pInternal;
	
	if(pFindData)
	{
		if(pFindData->m_pTree->m_TreeType == UnixTree)
		{
			_findclose(pFindData->m_Handle);
		}

		dfree(pFindData);
		pInfo->m_pInternal = LTNULL;
	}
}


#define SAVEBUFSIZE 1024*16

// Save the contents of a steam to a file
// returns 1 if successful 0 if an error occured
int df_Save(ILTStream *hFile, const char* pName)
{
	unsigned char* pSaveBuf;
	uint32 nBytesSaved;
	uint32 nAmountToRead, fileLen;
	FILE* pSaveFile;

	if(!hFile || hFile->ErrorStatus() != LT_OK)
		return 0;

	pSaveFile = fopen(pName, "wb");
	if (pSaveFile == LTNULL) return 0;

	LT_MEM_TRACK_ALLOC(pSaveBuf = new uint8[SAVEBUFSIZE],LT_MEM_TYPE_FILE);
	if (pSaveBuf == LTNULL)
	{
		fclose(pSaveFile);
		return 0;
	}

	hFile->SeekTo(0);

	nBytesSaved = 0;

	hFile->GetLen(&fileLen);
	while (nBytesSaved < fileLen)
	{
		nAmountToRead = fileLen - nBytesSaved;
		if (nAmountToRead > SAVEBUFSIZE) nAmountToRead = SAVEBUFSIZE;

		hFile->Read(pSaveBuf, nAmountToRead);
		if (hFile->ErrorStatus() != LT_OK) break;

		if (fwrite(pSaveBuf, nAmountToRead, 1, pSaveFile) != 1) break;
		else nBytesSaved += nAmountToRead;
	}

	delete [] pSaveBuf;
	fclose(pSaveFile);

	if (nBytesSaved != fileLen) return 0;
	else return 1;
}

int df_GetRawInfo(HLTFileTree *hTree, const char *pName, char* sFileName, unsigned int nMaxFileName, uint32* nPos, uint32* nSize)
{
	char fullName[500];
	FileTree *pTree;
	FILE *fp;
	CRezItm* pRezItm;

	pTree = (FileTree*)hTree;
	if(!pTree) return 0;

	if(pTree->m_TreeType == UnixTree || pTree->m_TreeType == DosTree)
	{
		if (pTree->m_TreeType == UnixTree)
		{
			LTSNPrintF(fullName, sizeof(fullName), "%s/%s", pTree->m_BaseName, pName);
		}
		else
		{
			LTSNPrintF(fullName, sizeof(fullName), "%s\\%s", pTree->m_BaseName, pName);
		}

		fp = fopen(fullName, "rb");
		if(!fp)	return 0;

		fseek(fp, 0, SEEK_END);
		*nSize = ftell(fp);
		*nPos = 0;
		fclose(fp);

		LTStrCpy(sFileName, fullName, nMaxFileName);
		if (nMaxFileName <= strlen(fullName)) return 0;

		return 1;
	}
	else
	{
		pRezItm = pTree->m_pRezMgr->GetRezFromDosPath(pName);
		if (pRezItm == LTNULL) return 0;

		const char* rez_name = pRezItm->DirectRead_GetFullRezName();
		LTStrCpy(sFileName, rez_name, nMaxFileName);
		if (nMaxFileName <= strlen(rez_name)) return 0;

		*nPos = pRezItm->DirectRead_GetFileOffset();
		*nSize = pRezItm->GetSize();
		return 1;
	}
}
