#ifndef __RENDERERFRAMESTATS_H__
#define __RENDERERFRAMESTATS_H__

#include <cstring>

#ifndef __LTRENDERERSTATS_H__
#include "ltrendererstats.h"
#endif

// Stub for non-D3D renderers: clears stats and reports success.
inline bool GetFrameStats(LTRendererStats &refStats)
{
	std::memset(&refStats, 0, sizeof(refStats));
	return false;
}

#endif // __RENDERERFRAMESTATS_H__
