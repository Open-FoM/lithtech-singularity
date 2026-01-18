#ifndef __LIBLTINFO_H__
#include "libltinfo.h"
#endif

#ifdef LIBLTINFO_OUTPUT_REDIRECTION

#ifndef __OUTPUTREDIR_H__
#include "outputredir.h"
#endif

#include <cstdio>
#include <cstdint>
#include <ltassert.h>
#include <dsys.h>

bool COutputRedir::OpenLogFile(ECOutput_ReDir logfile, const char* pFilename)
{
	FILE* f = std::fopen(pFilename, "w");
	m_pLogFiles[logfile - OUTPUT_REDIR_FILE0] = reinterpret_cast<std::uintptr_t>(f);
	return (f != nullptr);
}

bool COutputRedir::CloseLogFile(ECOutput_ReDir logfile)
{
	FILE* f = reinterpret_cast<FILE*>(m_pLogFiles[logfile - OUTPUT_REDIR_FILE0]);
	if (!f)
	{
		return false;
	}

	const int32 result = std::fclose(f);
	m_pLogFiles[logfile - OUTPUT_REDIR_FILE0] = 0;
	return (result == 0);
}

void COutputRedir::OutputToFile(uint32 index)
{
	FILE* f = reinterpret_cast<FILE*>(m_pLogFiles[index]);
	if (f)
	{
		std::fputs(m_pPrintBuffer, f);
	}
}

void COutputRedir::OutputToCONIO()
{
	std::printf("%s", m_pPrintBuffer);
}

void COutputRedir::OutputToDLL()
{
	ASSERT(!"Output redirection to a logging DLL is not implemented on this platform.");
}

void COutputRedir::OutputToASSERT()
{
	ASSERT(!m_pPrintBuffer);
}

void COutputRedir::OutputToConsole()
{
	dsi_PrintToConsole(m_pPrintBuffer);
}

void COutputRedir::OutputToIDE()
{
	std::fprintf(stderr, "%s", m_pPrintBuffer);
}

#endif // LIBLTINFO_OUTPUT_REDIRECTION
