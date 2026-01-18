// ------------------------------------------------------------------------- //
//
// FILE      : D S Y S _ I N T E R F A C E . H
//
// CREATED   : 11/12/99
//
// AUTHOR    : 
//
// COPYRIGHT : Monolith Productions Inc.
//
// ORIGN     : Orig LithTech
//
// ------------------------------------------------------------------------- //

// This header defines the main system-dependent engine functions.

#ifndef __DSYS_INTERFACE_H__
#define __DSYS_INTERFACE_H__


// ------------------------------------------------------------------------- //
// Includes this module depends on
// ------------------------------------------------------------------------- //
#include "ltbasetypes.h"
#include "ltbasedefs.h"
#include <stdarg.h>
#include <setjmp.h>

#ifdef LTJS_SDL_BACKEND
#include "ltjs_main_window_descriptor.h"
#include "ltjs_system_event_mgr.h"
namespace ltjs { class ShellStringFormatter; }
#endif // LTJS_SDL_BACKEND

#ifdef DE_CLIENT_COMPILE
#define MAX_KEYBUFFER       100
#define SOUND_DRIVER_NAME_LEN   32
#define SOUND_DRIVER_NAME_ARG   "+sounddll"

class ClientGlob {
	public:
		ClientGlob () {
			m_bIsConsoleUp		= false;
			m_bConsoleEnabled	= true;
			m_bInputEnabled		= true;

			m_pGameResources	= nullptr;
			m_pWorldName		= nullptr;
		}

	LTBOOL          m_bProcessWindowMessages;
	jmp_buf         m_MemoryJmp;

#ifdef LTJS_SDL_BACKEND
	ltjs::MainWindowDescriptor m_hMainWnd{};
	ltjs::SystemEventMgr* system_event_mgr{};
#else
	void*           m_hMainWnd;
#endif // LTJS_SDL_BACKEND

	void*           m_hInstance;

	const char*     m_WndClassName;
	const char*     m_WndCaption;

	LTBOOL          m_bInitializingRenderer;
	LTBOOL          m_bBreakOnError;
	LTBOOL          m_bClientActive;
	LTBOOL          m_bLostFocus;
	LTBOOL          m_bAppClosing;
	LTBOOL          m_bDialogUp;
	LTBOOL          m_bRendererShutdown;

	LTBOOL          m_bHost;
	char*           m_pGameResources;

	const char*     m_pWorldName;
	char            m_CachePath[500];

#ifdef LTJS_SDL_BACKEND
	unsigned int m_KeyDowns[MAX_KEYBUFFER];
	unsigned int m_KeyUps[MAX_KEYBUFFER];
	unsigned int m_KeyDownReps[MAX_KEYBUFFER];
#else
	uint32          m_KeyDowns[MAX_KEYBUFFER];
	uint32          m_KeyUps[MAX_KEYBUFFER];
	LTBOOL          m_KeyDownReps[MAX_KEYBUFFER];
#endif // LTJS_SDL_BACKEND

	uint16          m_nKeyDowns;
	uint16          m_nKeyUps;

	LTBOOL          m_bIsConsoleUp;
	LTBOOL          m_bInputEnabled;

	char            m_ExitMessage[500];

	char            m_acSoundDriverName[SOUND_DRIVER_NAME_LEN];

	bool            m_bConsoleEnabled;
};

extern ClientGlob g_ClientGlob;
#endif // DE_CLIENT_COMPILE


// ------------------------------------------------------------------------- //
// Externs (Public C data)
// ------------------------------------------------------------------------- //

// ------------------------------------------------------------------------- //
// Typedefs
// ------------------------------------------------------------------------- //

// ------------------------------------------------------------------------- //
// Prototypes (Public C methods)
// ------------------------------------------------------------------------- //

class CClientMgr;
class CClassMgr;
class LTVersionInfo;

// These are called in the startup code (in client.cpp and server_interface.cpp).
// They initialize the system-dependent modules.
// 0 = success
// 1 = couldn't load resource module (de_msg.dll).
int dsi_Init();
void dsi_Term();

// Called (in debug version) when any function uses RETURN_ERROR.
// (Good place to keep a breakpoint!)
void dsi_OnReturnError(int err);

// ClientDE implementations.
RMode* dsi_GetRenderModes();
void dsi_RelinquishRenderModes(RMode *pMode);
LTRESULT dsi_GetRenderMode(RMode *pMode);
LTRESULT dsi_SetRenderMode(RMode *pMode);
LTRESULT dsi_ShutdownRender(uint32 flags);

// Initializes the cshell and cres DLLs (copies them into a temp directory).	
LTRESULT dsi_InitClientShellDE();
LTRESULT dsi_LoadServerObjects(CClassMgr *pInfo);

// Called when we run out of memory.  Shuts down everything
// and comes up with an error message.
void dsi_OnMemoryFailure();

void dsi_Sleep(uint32 ms);
void dsi_ServerSleep(uint32 ms);

// Client-only things.
void dsi_ClientSleep(uint32 ms);

LTBOOL dsi_IsInputEnabled();

uint16 dsi_NumKeyDowns();
uint16 dsi_NumKeyUps();
uint32 dsi_GetKeyDown(uint32 i);
uint32 dsi_GetKeyDownRep(uint32 i);
uint32 dsi_GetKeyUp(uint32 i);
void dsi_ClearKeyDowns();
void dsi_ClearKeyUps();
void dsi_ClearKeyMessages();

LTBOOL dsi_IsConsoleUp();
void dsi_SetConsoleUp(LTBOOL bUp);
void dsi_SetConsoleEnable(bool bEnabled);
bool dsi_IsConsoleEnabled();
LTBOOL dsi_IsClientActive();
void dsi_OnClientShutdown( char *pMsg );

LTRESULT dsi_GetVersionInfo(LTVersionInfo &info);

char* dsi_GetDefaultWorld();


// Sets up a message for a LTRESULT.
#ifndef LTJS_SDL_BACKEND
LTRESULT dsi_SetupMessage(char *pMsg, int maxMsgLen, LTRESULT dResult, va_list marker);
#else
LTRESULT dsi_SetupMessage(
	char* pMsg,
	int maxMsgLen,
	LTRESULT dResult,
	ltjs::ShellStringFormatter& formatter);
#endif // LTJS_SDL_BACKEND
		  
// Puts an error message in the console if the renderer is initialized or
// a message box otherwise.
LTRESULT dsi_DoErrorMessage(char *pMessage);



void dsi_PrintToConsole(const char *pMsg, ...);	// Print to console.

void* dsi_GetInstanceHandle();	// Returns an HINSTANCE.
void* dsi_GetResourceModule();	// Returns an HINSTANCE.
void* dsi_GetMainWindow();		// Returns an HWND.

// Message box.
void dsi_MessageBox(const char *pMsg, const char *pTitle);

#ifdef LTJS_SDL_BACKEND
void* dsi_get_system_event_handler_mgr() noexcept;
#endif // LTJS_SDL_BACKEND

// ------------------------------------------------------------------------- //
// Class Definitions
// ------------------------------------------------------------------------- //

#endif  // __DSYS_INTERFACE_H__
