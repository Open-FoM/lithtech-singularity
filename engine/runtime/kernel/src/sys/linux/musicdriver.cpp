#include "musicdriver.h"

musicdriver_status music_InitDriver(char *pMusicDLLName, SMusicMgr *pMusicMgr)
{
	(void)pMusicDLLName;
	if (pMusicMgr)
	{
		pMusicMgr->m_bValid = false;
		pMusicMgr->m_bEnabled = false;
	}

	// TODO: [CRITICAL] No music driver on non-Windows builds yet.
	return MUSICDRIVER_INVALIDDLL;
}

void music_TermDriver()
{
}
