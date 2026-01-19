
// This module implements all the dsi_interface functions.

#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include "bdefs.h"
#include "stdlterror.h"
#include "stringmgr.h"
#include "sysfile.h"
#include "de_objects.h"
#include "servermgr.h"
#include "classbind.h"
#include "bindmgr.h"
#include "console.h"
#include "iltinfo.h"
#include "version_info.h"

#ifdef LTJS_SDL_BACKEND
#include "SDL3/SDL.h"
#include "ltjs_shell_string_formatter.h"
#include "ltjs_shared_data_mgr.h"
#include "ltjs_shell_resource_mgr.h"
#include "ltmsg_resource_ids.h"
#endif // LTJS_SDL_BACKEND

//IClientShell game client shell object.
#include "iclientshell.h"
static IClientShell *i_client_shell;
define_holder(IClientShell, i_client_shell);

#ifdef LTJS_SDL_BACKEND
ltjs::ShellResourceMgr* g_hResourceModule = nullptr;

#define DO_CODE(x)  LT_##x, IDS_##x

struct LTSysResultString {
	unsigned long dResult;
	unsigned long string_id;
};

LTSysResultString g_StringMap[] = {
	DO_CODE(SERVERERROR),
	DO_CODE(ERRORLOADINGRENDERDLL),
	DO_CODE(MISSINGWORLDMODEL),
	DO_CODE(CANTLOADGAMERESOURCES),
	DO_CODE(CANTINITIALIZEINPUT),
	DO_CODE(MISSINGSHELLDLL),
	DO_CODE(INVALIDSHELLDLL),
	DO_CODE(INVALIDSHELLDLLVERSION),
	DO_CODE(CANTCREATECLIENTSHELL),
	DO_CODE(UNABLETOINITSOUND),
	DO_CODE(MISSINGWORLDFILE),
	DO_CODE(INVALIDWORLDFILE),
	DO_CODE(INVALIDSERVERPACKET),
	DO_CODE(MISSINGSPRITEFILE),
	DO_CODE(INVALIDSPRITEFILE),
	DO_CODE(MISSINGMODELFILE),
	DO_CODE(INVALIDMODELFILE),
	DO_CODE(UNABLETORESTOREVIDEO),
	DO_CODE(MISSINGCLASS),
	DO_CODE(CANTCREATESERVERSHELL),
	DO_CODE(INVALIDOBJECTDLLVERSION),
	DO_CODE(ERRORINITTINGNETDRIVER),
	DO_CODE(USERCANCELED),
	DO_CODE(CANTRESTOREOBJECT),
	DO_CODE(NOGAMERESOURCES),
	DO_CODE(ERRORCOPYINGFILE),
	DO_CODE(INVALIDNETVERSION)
};

#define STRINGMAP_SIZE (sizeof(g_StringMap) / sizeof(g_StringMap[0]))
#endif // LTJS_SDL_BACKEND



void dsi_OnReturnError(int err)
{
}

static LTBOOL dsi_LoadResourceModule()
{
#ifdef LTJS_SDL_BACKEND
	g_hResourceModule = ltjs::get_shared_data_mgr().get_ltmsg_mgr();
	return g_hResourceModule != nullptr;
#else
	// Resource modules are Windows-only; SDL builds use ltjs_shell_resource_mgr for strings/cursors.
	return LTTRUE;      // DAN - temporary
#endif // LTJS_SDL_BACKEND
}

static void dsi_UnloadResourceModule()
{
#ifdef LTJS_SDL_BACKEND
	g_hResourceModule = nullptr;
#endif // LTJS_SDL_BACKEND
}


#ifndef LTJS_SDL_BACKEND
LTRESULT dsi_SetupMessage(char *pMsg, int maxMsgLen, LTRESULT dResult, va_list marker)
{
	static_cast<void>(marker);
	if (!pMsg || maxMsgLen <= 0)
	{
		return LT_ERROR;
	}

	LTSNPrintF(pMsg, maxMsgLen, "LTRESULT 0x%08X", dResult);
	return LT_ERROR;
}
#else
LTRESULT dsi_SetupMessage(
	char* pMsg,
	int maxMsgLen,
	LTRESULT dResult,
	ltjs::ShellStringFormatter& formatter)
{
	if (!pMsg || maxMsgLen <= 0)
	{
		return LT_ERROR;
	}

	pMsg[0] = 0;

	if (!g_hResourceModule)
	{
		LTSNPrintF(pMsg, maxMsgLen, "<missing resource DLL>");
		return LT_ERROR;
	}

	// Try to find the error code.
	auto bFound = false;
	auto resultCode = ERROR_CODE(dResult);
	auto stringID = static_cast<unsigned long>(0);

	for (auto i = 0; i < STRINGMAP_SIZE; ++i)
	{
		if (g_StringMap[i].dResult == resultCode)
		{
			bFound = true;
			stringID = g_StringMap[i].string_id;
			break;
		}
	}

	if (bFound)
	{
		char tempBuffer[500];
		const auto string_size = g_hResourceModule->load_string(stringID, tempBuffer, sizeof(tempBuffer) - 1);
		if (string_size > 0)
		{
			formatter.format(tempBuffer, string_size, pMsg, maxMsgLen);
			return LT_OK;
		}
	}

	LTSNPrintF(pMsg, maxMsgLen, "<invalid resource DLL>");
	return LT_ERROR;
}
#endif // LTJS_SDL_BACKEND


int dsi_Init()
{
	dm_Init();	// Memory manager.
	str_Init();	// String manager.
	df_Init();	// File manager.
//	obj_Init();	// Object manager.
//	packet_Init();

	if (dsi_LoadResourceModule())
	{
		return 0;
	}

	dsi_Term();
	return 1;
}

void dsi_Term()
{
//	packet_Term();
//	obj_Term();
	df_Term();
	str_Term();
	dm_Term();
	dsi_UnloadResourceModule();
	return;
}

void* dsi_GetResourceModule()
{
#ifdef LTJS_SDL_BACKEND
	return g_hResourceModule;
#else
	// Resource modules are Windows-only; SDL builds use ltjs_shell_resource_mgr for strings/cursors.
	return NULL;      // DAN - temporary
#endif // LTJS_SDL_BACKEND
}


LTRESULT _GetOrCopyFile(char *pTempPath, char *pFilename, char *pOutName, int outNameLen)
{
    return LTTRUE;      // DAN - temporary
}


LTRESULT dsi_LoadServerObjects(CClassMgr *pInfo)
{
	char* pGameServerObjectName = "libobject.so";

    //load the GameServer shared object
    int version;
    int status = cb_LoadModule(pGameServerObjectName, false, pInfo->m_ClassModule, &version);

    //check for errors.
    if (status == CB_CANTFINDMODULE) 
	{
        return LT_INVALIDOBJECTDLL;
    }
    else if (status == CB_NOTCLASSMODULE)
	{
        return LT_INVALIDOBJECTDLL;
    }
    else if (status == CB_VERSIONMISMATCH) 
	{
		return LT_INVALIDOBJECTDLLVERSION;
	}
	
/*	
	    // Get sres.dll.
	bFileCopied = false;
    if ((GetOrCopyFile("sres.dll", fileName, sizeof(fileName),bFileCopied) != LT_OK)
        || (bm_BindModule(fileName, bFileCopied, pClassMgr->m_hServerResourceModule) != BIND_NOERROR))
    {
		cb_UnloadModule( pClassMgr->m_ClassModule );

        sm_SetupError(LT_ERRORCOPYINGFILE, "sres.dll");
        RETURN_ERROR_PARAM(1, LoadServerObjects, LT_ERRORCOPYINGFILE, "sres.dll");
    }

    //let the dll know it's instance handle.
    if (instance_handle_server != NULL) 
	{
        instance_handle_server->SetInstanceHandle( pClassMgr->m_ClassModule.m_hModule );
    }
*/
	
	//cb_LoadModule(fileName, false, pInfo->m_ClassModule, &version);

	/*
	pInfo->m_hShellModule = (ShellModule*)malloc(sizeof(ShellModule));
	pInfo->m_hShellModule->m_hModule = NULL;
	pInfo->m_CreateShellFn =
		pInfo->m_hShellModule->m_CreateFn = (CreateShellFn) CreateServerShell;
	pInfo->m_DeleteShellFn =
		pInfo->m_hShellModule->m_DeleteFn = (DeleteShellFn) DeleteServerShell;
	*/

	return LT_OK;
}

void dsi_Sleep(uint32 ms)
{
// Several possible implementations:
// poll (requires sys/poll.h)
//	poll(NULL, 0, ms);
// select (requires sys/time.h, sys/types.h, unistd.h)
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = ms*1000;
	select(0, NULL, NULL, NULL, &timeout);
// SIGALRM (requires sys/time.h, unistd.h)
//	itimerval timerconfig;
//	memset(&timerconfig, 0, sizeof(timerconfig));
//	timerconfig.it_value.tv_usec = ms*1000;
//	setitimer(ITIMER_REAL, &timerconfig, NULL);
//	pause();
}

LTRESULT dsi_GetVersionInfo(LTVersionInfo &info)
{
	info.m_MajorVersion = LT_VI_VER_MAJOR;
	info.m_MinorVersion = LT_VI_VER_MINOR;
	return LT_OK;
}

void dsi_ServerSleep(uint32 ms)
{ dsi_Sleep(ms); }

extern int32 g_ScreenWidth, g_ScreenHeight;	// Console variables.

static void dsi_GetDLLModes(char *pDLLName, RMode **pMyList)
{
}


RMode* dsi_GetRenderModes()
{
return NULL;      // DAN - temporary
}

void dsi_RelinquishRenderModes(RMode *pMode)
{
}

LTRESULT dsi_GetRenderMode(RMode *pMode)
{
return LTTRUE;      // DAN - temporary
}

LTRESULT dsi_SetRenderMode(RMode *pMode)
{
return LTTRUE;      // DAN - temporary
}

LTRESULT dsi_ShutdownRender(uint32 flags)
{
return LTTRUE;      // DAN - temporary
}

LTRESULT _GetOrCopyClientFile(char *pTempPath, char *pFilename, char *pOutName, int outNameLen)
{
return LTTRUE;      // DAN - temporary
}

LTRESULT dsi_InitClientShellDE()
{
//	g_pClientMgr->m_hShellModule = NULL;
//	g_pClientMgr->m_hClientResourceModule = NULL;

	// have the user's cshell and the clientMgr exchange info
	if ((i_client_shell == NULL ))
    {
		CRITICAL_ERROR("dsys_interface", "Can't create CShell\n");
	}

	return LT_OK;
}

void dsi_OnMemoryFailure()
{
}

// Client-only functions.
void dsi_ClientSleep(uint32 ms)
{
	dsi_ServerSleep(ms);
}

LTBOOL dsi_IsInputEnabled()
{
return LTTRUE;      // DAN - temporary
}

uint16 dsi_NumKeyDowns()
{
return 0;      // DAN - temporary
}

uint16 dsi_NumKeyUps()
{
return 0;     // DAN - temporary
}

uint32 dsi_GetKeyDown(uint32 i)
{
return 0;     // DAN - temporary
}

uint32 dsi_GetKeyDownRep(uint32 i)
{
return 0;     // DAN - temporary
}

uint32 dsi_GetKeyUp(uint32 i)
{
return 0;     // DAN - temporary
}

void dsi_ClearKeyDowns()
{
}

void dsi_ClearKeyUps()
{
}

void dsi_ClearKeyMessages()
{
}

LTBOOL dsi_IsConsoleUp()
{
    return LTFALSE;
}

void dsi_SetConsoleUp(LTBOOL bUp)
{
}

void dsi_SetConsoleEnable(bool bEnabled)
{
#ifdef DE_CLIENT_COMPILE
	g_ClientGlob.m_bConsoleEnabled = bEnabled;
#else
	(void)bEnabled;
#endif
}

bool dsi_IsConsoleEnabled()
{
#ifdef DE_CLIENT_COMPILE
	return g_ClientGlob.m_bConsoleEnabled;
#else
	return false;
#endif
}

LTBOOL dsi_IsClientActive()
{
	return TRUE;
}

void dsi_OnClientShutdown( char *pMsg )
{
}

char* dsi_GetDefaultWorld()
{
return NULL;     // DAN - temporary
}


#include "server_interface.h"

extern CServerMgr *g_pServerMgr;

void dsi_PrintToConsole(const char *pMsg, ...) {
    va_list marker;
    char msg[1000];

    if (g_pServerMgr && g_pServerMgr->m_pServerAppHandler) {
        va_start(marker, pMsg);
        vsnprintf(msg, 999, pMsg, marker);
        va_end(marker);

        g_pServerMgr->m_pServerAppHandler->ConsoleOutputFn(msg);
    }
}

void* dsi_GetInstanceHandle()
{
return NULL;     // DAN - temporary
}

void* dsi_GetMainWindow()
{
return NULL;     // DAN - temporary
}

LTRESULT dsi_DoErrorMessage(char *pMessage)
{
	if (pMessage)
	{
		con_PrintString(CONRGB(255,255,255), 0, pMessage);
	}

#ifdef DE_CLIENT_COMPILE
#ifdef LTJS_SDL_BACKEND
	if (pMessage && g_ClientGlob.m_hMainWnd.sdl_window)
	{
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			g_ClientGlob.m_WndCaption ? g_ClientGlob.m_WndCaption : "LithTech",
			pMessage,
			g_ClientGlob.m_hMainWnd.sdl_window
		);
	}
#endif // LTJS_SDL_BACKEND
#endif // DE_CLIENT_COMPILE

	return LT_OK;
}

void dsi_MessageBox(const char *pMessage, const char *pTitle)
{
#ifdef DE_CLIENT_COMPILE
#ifdef LTJS_SDL_BACKEND
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_INFORMATION,
		pTitle ? pTitle : "LithTech",
		pMessage ? pMessage : "",
		g_ClientGlob.m_hMainWnd.sdl_window
	);
#else
	static_cast<void>(pMessage);
	static_cast<void>(pTitle);
#endif // LTJS_SDL_BACKEND
#else
	static_cast<void>(pMessage);
	static_cast<void>(pTitle);
#endif // DE_CLIENT_COMPILE
}

#ifdef LTJS_SDL_BACKEND
void* dsi_get_system_event_handler_mgr() noexcept
{
#ifdef DE_CLIENT_COMPILE
	return g_ClientGlob.system_event_mgr ? g_ClientGlob.system_event_mgr->get_handler_mgr() : nullptr;
#else
	return nullptr;
#endif
}
#endif // LTJS_SDL_BACKEND
