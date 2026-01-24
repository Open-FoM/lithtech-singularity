#include <cstdarg>
#include <cstdio>
#include <cstdlib>

extern void DEdit2_Log(const char* fmt, ...);
extern void DEdit2_LogV(const char* fmt, va_list args);

void dsi_PrintToConsole(const char* msg, ...)
{
	if (!msg)
	{
		return;
	}

	va_list args;
	va_start(args, msg);
	DEdit2_LogV(msg, args);
	va_end(args);
	fflush(stderr);
}

void dsi_OnMemoryFailure()
{
	DEdit2_Log("Engine memory allocation failed.");
	std::abort();
}

void dsi_OnReturnError(int err)
{
	DEdit2_Log("Engine reported error: %d", err);
}
