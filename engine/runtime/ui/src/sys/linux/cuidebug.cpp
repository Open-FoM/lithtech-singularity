// -----------------------------------------------------------------------
//
// MODULE  : CUIDEBUG.CPP
//
// PURPOSE : implements CUI debug output for non-Windows builds
//
// CREATED : 1/01
//
// -----------------------------------------------------------------------

#include "cuidebug.h"

#if defined __DEBUG || defined _DEBUG

#include <cstdarg>
#include <cstdio>
#include <cstring>

char CUIDebug::sm_pText[256];

void CUIDebug::DebugPrint(const char* pFormat, ...)
{
	if (!pFormat) {
		return;
	}

	va_list args;
	va_start(args, pFormat);
	vsnprintf(sm_pText, sizeof(sm_pText), pFormat, args);
	va_end(args);

	sm_pText[sizeof(sm_pText) - 1] = '\0';
	fputs(sm_pText, stderr);
}

#endif // __DEBUG || _DEBUG
