
// This module implements all the dsi_interface functions.

#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <cassert>
#include <memory>

#include "bdefs.h"
#include "stdlterror.h"
#include "bibendovsky_spul_path_utils.h"
#include "stringmgr.h"
#include "sysfile.h"
#include "de_objects.h"
#include "servermgr.h"
#include "classbind.h"
#include "bindmgr.h"
#include "console.h"
#include "iltinfo.h"
#include "version_info.h"
#include "render.h"
#include <cstring>

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

#ifdef DE_CLIENT_COMPILE
// IClientFileMgr
#include "client_filemgr.h"
static IClientFileMgr* client_file_mgr;
define_holder(IClientFileMgr, client_file_mgr);
#endif // DE_CLIENT_COMPILE

// IInstanceHandleClient
static IInstanceHandleClient* instance_handle_client;
define_holder(IInstanceHandleClient, instance_handle_client);

// IInstanceHandleServer
#include "iservershell.h"
static IInstanceHandleServer* instance_handle_server;
define_holder(IInstanceHandleServer, instance_handle_server);

#ifdef DE_CLIENT_COMPILE
#include "clientmgr.h"
#endif // DE_CLIENT_COMPILE

#ifdef LTJS_SDL_BACKEND
ltjs::ShellResourceMgr* g_hResourceModule = nullptr;

#define DO_CODE(x)  LT_##x, IDS_##x

struct LTSysResultString {
	unsigned long dResult;
	unsigned long string_id;
};

static LTSysResultString s_StringMap[] = {
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

#define STRINGMAP_SIZE (sizeof(s_StringMap) / sizeof(s_StringMap[0]))
#endif // LTJS_SDL_BACKEND

namespace ltjs
{
namespace ul = bibendovsky::spul;
} // ltjs



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
		if (s_StringMap[i].dResult == resultCode)
		{
			bFound = true;
			stringID = s_StringMap[i].string_id;
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
	int status;

	const auto object_file_name = ltjs::ul::PathUtils::append("game", "object.dylib");

	// Load the object shared library.
	int version;
	status = cb_LoadModule(object_file_name.c_str(), false, pInfo->m_ClassModule, &version);

	// Check for errors.
	if (status == CB_CANTFINDMODULE)
	{
		sm_SetupError(LT_INVALIDOBJECTDLL, object_file_name.c_str());
		RETURN_ERROR_PARAM(1, LoadObjectsInDirectory, LT_INVALIDOBJECTDLL, object_file_name.c_str());
	}
	else if (status == CB_NOTCLASSMODULE)
	{
		sm_SetupError(LT_INVALIDOBJECTDLL, object_file_name.c_str());
		RETURN_ERROR_PARAM(1, LoadObjectsInDirectory, LT_INVALIDOBJECTDLL, object_file_name.c_str());
	}
	else if (status == CB_VERSIONMISMATCH)
	{
		sm_SetupError(LT_INVALIDOBJECTDLLVERSION, object_file_name.c_str(), version, SERVEROBJ_VERSION);
		RETURN_ERROR_PARAM(1, LoadObjectsInDirectory, LT_INVALIDOBJECTDLLVERSION, object_file_name.c_str());
	}

#ifndef LTJS_SDL_BACKEND
	// Get sres.dylib.
	const auto sres_file_name = ltjs::ul::PathUtils::append("game", "sres.dylib");
	if (bm_BindModule(sres_file_name.c_str(), false, pInfo->m_hServerResourceModule) != BIND_NOERROR)
	{
		cb_UnloadModule(pInfo->m_ClassModule);
		sm_SetupError(LT_ERRORCOPYINGFILE, sres_file_name.c_str());
		RETURN_ERROR_PARAM(1, LoadServerObjects, LT_ERRORCOPYINGFILE, sres_file_name.c_str());
	}
#endif // LTJS_SDL_BACKEND

	// Let the library know its instance handle.
	if (instance_handle_server != nullptr)
	{
		instance_handle_server->SetInstanceHandle(pInfo->m_ClassModule.m_hModule);
	}

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
#ifdef DE_CLIENT_COMPILE
	RMode mode = g_RMode;

	if (mode.m_Width == 0 || mode.m_Height == 0)
	{
		mode.m_Width = static_cast<uint32>(g_ScreenWidth);
		mode.m_Height = static_cast<uint32>(g_ScreenHeight);
	}

	if (mode.m_BitDepth == 0)
	{
		mode.m_BitDepth = 32;
	}

	if (mode.m_InternalName[0] == '\0')
	{
		LTStrCpy(mode.m_InternalName, "sdl", sizeof(mode.m_InternalName));
	}

	if (mode.m_Description[0] == '\0')
	{
		LTStrCpy(mode.m_Description, "SDL Renderer", sizeof(mode.m_Description));
	}

	mode.m_bHWTnL = true;
	mode.m_pNext = nullptr;

	auto* out_mode = new RMode;
	*out_mode = mode;
	return out_mode;
#else
	return nullptr;
#endif // DE_CLIENT_COMPILE
}

void dsi_RelinquishRenderModes(RMode *pMode)
{
#ifdef DE_CLIENT_COMPILE
	delete pMode;
#else
	static_cast<void>(pMode);
#endif // DE_CLIENT_COMPILE
}

LTRESULT dsi_GetRenderMode(RMode *pMode)
{
#ifdef DE_CLIENT_COMPILE
	if (!pMode)
	{
		return LT_ERROR;
	}

	std::memcpy(pMode, &g_RMode, sizeof(RMode));
	return LT_OK;
#else
	static_cast<void>(pMode);
	return LT_ERROR;
#endif // DE_CLIENT_COMPILE
}

LTRESULT dsi_SetRenderMode(RMode *pMode)
{
#ifdef DE_CLIENT_COMPILE
	RMode currentMode;
	char message[256];

	if (!pMode)
	{
		return LT_ERROR;
	}

	if (r_TermRender(1, false) != LT_OK)
	{
#ifdef LTJS_SDL_BACKEND
		auto formatter = ltjs::ShellStringFormatter{};
		dsi_SetupMessage(message, sizeof(message) - 1, LT_UNABLETORESTOREVIDEO, formatter);
#else
		dsi_SetupMessage(message, sizeof(message) - 1, LT_UNABLETORESTOREVIDEO, LTNULL);
#endif // LTJS_SDL_BACKEND
		dsi_OnClientShutdown(message);
		RETURN_ERROR(0, SetRenderMode, LT_UNABLETORESTOREVIDEO);
	}

	std::memcpy(&currentMode, &g_RMode, sizeof(RMode));

	if (r_InitRender(pMode) != LT_OK)
	{
		if (r_InitRender(&currentMode) != LT_OK)
		{
			RETURN_ERROR(0, SetRenderMode, LT_UNABLETORESTOREVIDEO);
		}

		RETURN_ERROR(1, SetRenderMode, LT_KEPTSAMEMODE);
	}

#ifdef DE_CLIENT_COMPILE
	g_ClientGlob.m_bRendererShutdown = LTFALSE;
#endif // DE_CLIENT_COMPILE
	return LT_OK;
#else
	static_cast<void>(pMode);
	return LT_ERROR;
#endif // DE_CLIENT_COMPILE
}

LTRESULT dsi_ShutdownRender(uint32 flags)
{
#ifdef DE_CLIENT_COMPILE
	r_TermRender(1, true);

#ifdef DE_CLIENT_COMPILE
#ifdef LTJS_SDL_BACKEND
	if (flags & RSHUTDOWN_MINIMIZEWINDOW)
	{
		SDL_MinimizeWindow(g_ClientGlob.m_hMainWnd.sdl_window);
	}

	if (flags & RSHUTDOWN_HIDEWINDOW)
	{
		SDL_HideWindow(g_ClientGlob.m_hMainWnd.sdl_window);
	}
#endif // LTJS_SDL_BACKEND
#endif // DE_CLIENT_COMPILE

#ifdef DE_CLIENT_COMPILE
	g_ClientGlob.m_bRendererShutdown = LTTRUE;
#endif // DE_CLIENT_COMPILE
	return LT_OK;
#else
	static_cast<void>(flags);
	return LT_OK;
#endif // DE_CLIENT_COMPILE
}

LTRESULT _GetOrCopyClientFile(char *pTempPath, char *pFilename, char *pOutName, int outNameLen)
{
return LTTRUE;      // DAN - temporary
}

LTRESULT dsi_InitClientShellDE()
{
#ifdef DE_CLIENT_COMPILE
	int status;

	g_pClientMgr->m_hClientResourceModule = nullptr;
#ifndef LTJS_SDL_BACKEND
	g_pClientMgr->m_hLocalizedClientResourceModule = nullptr;
#endif // LTJS_SDL_BACKEND
	g_pClientMgr->m_hShellModule = nullptr;

	const auto cshell_file_name = ltjs::ul::PathUtils::append("game", "cshell.dylib");

	status = bm_BindModule(cshell_file_name.c_str(), false, g_pClientMgr->m_hShellModule);
	if (status == BIND_CANTFINDMODULE)
	{
		g_pClientMgr->SetupError(LT_MISSINGSHELLDLL, cshell_file_name.c_str());
		RETURN_ERROR(1, InitClientShellDE, LT_MISSINGSHELLDLL);
	}

	if (!i_client_shell)
	{
		g_pClientMgr->SetupError(LT_INVALIDSHELLDLL, cshell_file_name.c_str());
		RETURN_ERROR(1, InitClientShellDE, LT_INVALIDSHELLDLL);
	}

#ifndef LTJS_SDL_BACKEND
	const auto cres_file_name = ltjs::ul::PathUtils::append("game", "cres.dylib");
	status = bm_BindModule(cres_file_name.c_str(), false, g_pClientMgr->m_hClientResourceModule);
	if (status == BIND_CANTFINDMODULE)
	{
		bm_UnbindModule(g_pClientMgr->m_hShellModule);
		g_pClientMgr->m_hShellModule = nullptr;

		g_pClientMgr->SetupError(LT_INVALIDSHELLDLL, cres_file_name.c_str());
		RETURN_ERROR_PARAM(1, InitClientShellDE, LT_INVALIDSHELLDLL, cres_file_name.c_str());
	}
#endif // LTJS_SDL_BACKEND

	if (instance_handle_client)
	{
		instance_handle_client->SetInstanceHandle(g_pClientMgr->m_hShellModule);
	}

	return LT_OK;
#else
	return LT_OK;
#endif // DE_CLIENT_COMPILE
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
#ifdef DE_CLIENT_COMPILE
	return g_ClientGlob.m_bInputEnabled;
#else
	return LTFALSE;
#endif
}

uint16 dsi_NumKeyDowns()
{
#ifdef DE_CLIENT_COMPILE
	return g_ClientGlob.m_nKeyDowns;
#else
	return 0;
#endif
}

uint16 dsi_NumKeyUps()
{
#ifdef DE_CLIENT_COMPILE
	return g_ClientGlob.m_nKeyUps;
#else
	return 0;
#endif
}

uint32 dsi_GetKeyDown(uint32 i)
{
#ifdef DE_CLIENT_COMPILE
	ASSERT(i < MAX_KEYBUFFER);
	return g_ClientGlob.m_KeyDowns[i];
#else
	static_cast<void>(i);
	return 0;
#endif
}

uint32 dsi_GetKeyDownRep(uint32 i)
{
#ifdef DE_CLIENT_COMPILE
	ASSERT(i < MAX_KEYBUFFER);
	return g_ClientGlob.m_KeyDownReps[i];
#else
	static_cast<void>(i);
	return 0;
#endif
}

uint32 dsi_GetKeyUp(uint32 i)
{
#ifdef DE_CLIENT_COMPILE
	ASSERT(i < MAX_KEYBUFFER);
	return g_ClientGlob.m_KeyUps[i];
#else
	static_cast<void>(i);
	return 0;
#endif
}

void dsi_ClearKeyDowns()
{
#ifdef DE_CLIENT_COMPILE
	g_ClientGlob.m_nKeyDowns = 0;
#endif
}

void dsi_ClearKeyUps()
{
#ifdef DE_CLIENT_COMPILE
	g_ClientGlob.m_nKeyUps = 0;
#endif
}

void dsi_ClearKeyMessages()
{
#ifdef DE_CLIENT_COMPILE
#ifdef LTJS_SDL_BACKEND
	SDL_FlushEvent(SDL_EVENT_KEY_DOWN);
	SDL_FlushEvent(SDL_EVENT_KEY_UP);
#endif // LTJS_SDL_BACKEND
#endif // DE_CLIENT_COMPILE
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

    va_start(marker, pMsg);
    vsnprintf(msg, 999, pMsg, marker);
    va_end(marker);

    if (g_pServerMgr && g_pServerMgr->m_pServerAppHandler) {
        g_pServerMgr->m_pServerAppHandler->ConsoleOutputFn(msg);
    } else {
        const auto msg_len = std::strlen(msg);
        if (msg_len > 0 && msg[msg_len - 1] == '\n') {
            std::fputs(msg, stderr);
        } else {
            std::fprintf(stderr, "%s\n", msg);
        }
    }
}

void* dsi_GetInstanceHandle()
{
#ifdef DE_CLIENT_COMPILE
	return g_ClientGlob.m_hInstance;
#else
	return nullptr;
#endif // DE_CLIENT_COMPILE
}

void* dsi_GetMainWindow()
{
#ifdef DE_CLIENT_COMPILE
#ifdef LTJS_SDL_BACKEND
	return &g_ClientGlob.m_hMainWnd;
#else
	return g_ClientGlob.m_hMainWnd;
#endif // LTJS_SDL_BACKEND
#else
	return nullptr;
#endif // DE_CLIENT_COMPILE
}

LTRESULT dsi_DoErrorMessage(char *pMessage)
{
#ifdef DE_CLIENT_COMPILE
	if (pMessage)
	{
		con_PrintString(CONRGB(255,255,255), 0, pMessage);
	}
#else
	if (pMessage)
	{
		dsi_PrintToConsole("%s", pMessage);
	}
#endif // DE_CLIENT_COMPILE

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

struct ILtStreamUDeleter
{
	void operator()(
		ILTStream* resource) const noexcept
	{
		assert(resource);
		resource->Release();
	}
}; // ILtStreamUDeleter

using ILtStreamUPtr = std::unique_ptr<ILTStream, ILtStreamUDeleter>;

ltjs::Index dsi_get_file_size(
	const char* path) noexcept
{
#ifdef DE_CLIENT_COMPILE
	if (!path || path[0] == '\0' || !client_file_mgr)
	{
		return 0;
	}

	auto file_ref = FileRef{};
	file_ref.m_FileType = FILE_ANYFILE;
	file_ref.m_pFilename = path;

	auto file_stream = ILtStreamUPtr{client_file_mgr->OpenFile(&file_ref)};
	if (!file_stream)
	{
		return 0;
	}

	auto lt_file_size = uint32{};
	const auto get_len_result = file_stream->GetLen(&lt_file_size);
	if (get_len_result != LT_OK)
	{
		return 0;
	}

	return static_cast<ltjs::Index>(lt_file_size);
#else
	static_cast<void>(path);
	return 0;
#endif // DE_CLIENT_COMPILE
}

bool dsi_load_file_into_memory(
	const char* path,
	void* buffer,
	ltjs::Index max_buffer_size) noexcept
{
#ifdef DE_CLIENT_COMPILE
	if (!path || path[0] == '\0' ||
		!buffer ||
		max_buffer_size <= 0 ||
		!client_file_mgr)
	{
		return false;
	}

	auto file_ref = FileRef{};
	file_ref.m_FileType = FILE_ANYFILE;
	file_ref.m_pFilename = path;

	auto file_stream = ILtStreamUPtr{client_file_mgr->OpenFile(&file_ref)};
	if (!file_stream)
	{
		return false;
	}

	const auto read_result = file_stream->Read(buffer, static_cast<uint32>(max_buffer_size));
	return read_result == LT_OK;
#else
	static_cast<void>(path);
	static_cast<void>(buffer);
	static_cast<void>(max_buffer_size);
	return false;
#endif // DE_CLIENT_COMPILE
}
#endif // LTJS_SDL_BACKEND
