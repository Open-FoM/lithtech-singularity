#ifndef __SYSDRAWPRIM_H__
#define __SYSDRAWPRIM_H__


#ifdef __NULLREND
    #include "sys/null/nulldrawprim.h"
#else

	#if defined(LTJS_USE_DILIGENT_RENDER)
	#include "sys/diligent/diligentdrawprim.h"
	#else
		#error "D3D renderer removed; enable LTJS_USE_DILIGENT_RENDER."
	#endif

	#ifdef __XBOXREND
	#include "sys/xbox/xbox_drawprim.h"
	#endif

#endif

#endif
