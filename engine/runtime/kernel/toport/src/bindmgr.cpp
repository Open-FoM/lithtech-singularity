
#include "bdefs.h"
#include "bindmgr.h"
#include "ltmodule.h"
#include "syslibraryloader.h"

#include <cstdio>
#include <string>

namespace
{
constexpr auto kBindTypeServer = 0;
constexpr auto kBindTypeDll = 1;

struct NonWinBind
{
	HLTMODULE m_hInstance = nullptr;
	int m_Type = kBindTypeDll;
	std::string m_sTempFileName{};
};

CBindModuleType* bm_CreateHandleBinding(const char* pModuleName, void* pHandle)
{
	if (!pHandle)
	{
		pHandle = LTLibraryLoader::GetMainHandle();
	}

	NonWinBind* pBind = nullptr;
	LT_MEM_TRACK_ALLOC((pBind = new NonWinBind), LT_MEM_TYPE_MISC);
	pBind->m_hInstance = static_cast<HLTMODULE>(pHandle);
	pBind->m_Type = kBindTypeDll;
	pBind->m_sTempFileName = pModuleName ? pModuleName : "";
	return reinterpret_cast<CBindModuleType*>(pBind);
}
} // namespace


// --------------------------------------------------------- //
// Main interface functions.
// --------------------------------------------------------- //

int bm_BindModule(const char *pModuleName, bool bTempFile, CBindModuleType *&pModule)
{
	const bool bModuleAlreadyLoaded = LTLibraryLoader::IsLibraryLoaded(pModuleName);

	HLTMODULE hModule = LTLibraryLoader::OpenLibrary(pModuleName);
	if (!hModule)
	{
		return BIND_CANTFINDMODULE;
	}

	if (!bModuleAlreadyLoaded)
	{
		auto pSetMasterFn =
			reinterpret_cast<TSetMasterFn>(LTLibraryLoader::GetProcAddress(hModule, "SetMasterDatabase"));
		if (pSetMasterFn)
		{
			pSetMasterFn(GetMasterDatabase());
		}
	}

	pModule = bm_CreateHandleBinding(bTempFile ? pModuleName : "", reinterpret_cast<void*>(hModule));
	return BIND_NOERROR;
}


void bm_UnbindModule(CBindModuleType *hModule)
{
	auto* pBind = reinterpret_cast<NonWinBind*>(hModule);
	ASSERT(pBind);

	if (pBind->m_Type == kBindTypeDll)
	{
		LTLibraryLoader::CloseLibrary(pBind->m_hInstance);
	}

	if (!pBind->m_sTempFileName.empty())
	{
		::remove(pBind->m_sTempFileName.c_str());
		pBind->m_sTempFileName.clear();
	}

	delete pBind;
}


LTRESULT bm_SetInstanceHandle(CBindModuleType *hModule)
{
	using SetInstanceHandleFn = void (*)(void*);

	auto* pBind = reinterpret_cast<NonWinBind*>(hModule);
	if (!pBind)
	{
		RETURN_ERROR(1, bm_SetInstanceHandle, LT_INVALIDPARAMS);
	}

	auto* fn = reinterpret_cast<SetInstanceHandleFn>(
		LTLibraryLoader::GetProcAddress(pBind->m_hInstance, "SetInstanceHandle"));
	if (fn)
	{
		fn(reinterpret_cast<void*>(pBind->m_hInstance));
	}

	return LT_OK;
}


LTRESULT bm_GetInstanceHandle(CBindModuleType *hModule, void **pHandle)
{
	auto* pBind = reinterpret_cast<NonWinBind*>(hModule);
	if (!pBind)
	{
		RETURN_ERROR(1, bm_GetInstanceHandle, LT_INVALIDPARAMS);
	}

	*pHandle = reinterpret_cast<void*>(pBind->m_hInstance);
	return LT_OK;
}


void* bm_GetFunctionPointer(CBindModuleType *hModule, const char *pFunctionName)
{
	auto* pBind = reinterpret_cast<NonWinBind*>(hModule);
	ASSERT(pBind);

	return reinterpret_cast<void*>(LTLibraryLoader::GetProcAddress(pBind->m_hInstance, pFunctionName));
}














