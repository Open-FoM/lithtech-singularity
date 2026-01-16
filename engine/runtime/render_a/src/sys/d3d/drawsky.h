#ifndef __DRAWSKY_H__
#define __DRAWSKY_H__

// This module performs all the sky rendering.


#ifndef __VIEWPARAMS_H__
#include "viewparams.h"
#endif

// Draw the sky to a specified set of extents
void d3d_DrawSkyExtents(const ViewParams& Params, float fMinX, float fMinY, float fMaxX, float fMaxY);
					  

#endif  // __DRAWSKY_H__


