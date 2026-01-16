#ifndef __AABB_H__
#define __AABB_H__

#ifndef __LTBASEDEFS_H__
#include "ltbasedefs.h"
#endif

#ifndef __LTPLANE_H__
#include "ltplane.h"
#endif

enum EAABBCorner {
	eAABB_NearTopLeft = 0,
	eAABB_NearTopRight = 1,
	eAABB_NearBottomLeft = 2,
	eAABB_NearBottomRight = 3,
	eAABB_FarTopLeft = 4,
	eAABB_FarTopRight = 5,
	eAABB_FarBottomLeft = 6,
	eAABB_FarBottomRight = 7,
	eAABB_None = 8,
};

EAABBCorner GetAABBPlaneCorner(const LTVector &vNormal);

inline LTVector GetAABBCornerPos(EAABBCorner nCorner, const LTVector &vBoxMin, const LTVector &vBoxMax)
{
	switch (nCorner)
	{
		case eAABB_NearTopLeft :
			return LTVector(vBoxMin.x, vBoxMax.y, vBoxMin.z);
		case eAABB_NearTopRight :
			return LTVector(vBoxMax.x, vBoxMax.y, vBoxMin.z);
		case eAABB_NearBottomLeft :
			return LTVector(vBoxMin.x, vBoxMin.y, vBoxMin.z);
		case eAABB_NearBottomRight :
			return LTVector(vBoxMax.x, vBoxMin.y, vBoxMin.z);
		case eAABB_FarTopLeft :
			return LTVector(vBoxMin.x, vBoxMax.y, vBoxMax.z);
		case eAABB_FarTopRight :
			return LTVector(vBoxMax.x, vBoxMax.y, vBoxMax.z);
		case eAABB_FarBottomLeft :
			return LTVector(vBoxMin.x, vBoxMin.y, vBoxMax.z);
		case eAABB_FarBottomRight :
			return LTVector(vBoxMax.x, vBoxMin.y, vBoxMax.z);
		default :
			ASSERT("Invalid box corner requested");
			return vBoxMin;
	}
}

inline EAABBCorner GetAABBOtherCorner(EAABBCorner nCorner)
{
	static EAABBCorner aTransCorner[8] = {
		eAABB_FarBottomRight,
		eAABB_FarBottomLeft,
		eAABB_FarTopRight,
		eAABB_FarTopLeft,
		eAABB_NearBottomRight,
		eAABB_NearBottomLeft,
		eAABB_NearTopRight,
		eAABB_NearTopLeft
	};

	ASSERT((uint32)nCorner < 8);

	return aTransCorner[nCorner];
}

inline PolySide GetAABBPlaneSide(EAABBCorner eCorner, const LTPlane &cPlane, const LTVector &vBoxMin, const LTVector &vBoxMax)
{
	float fNearDist = cPlane.DistTo(GetAABBCornerPos(eCorner, vBoxMin, vBoxMax));
	if (fNearDist < 0.001f)
		return BackSide;
	float fFarDist = cPlane.DistTo(GetAABBCornerPos(GetAABBOtherCorner(eCorner), vBoxMin, vBoxMax));
	if (fFarDist > 0.001f)
		return FrontSide;
	else
		return Intersect;
}

inline bool GetAABBPlaneSideBack(EAABBCorner eCorner, const LTPlane &cPlane, const LTVector &vBoxMin, const LTVector &vBoxMax)
{
	float fNearDist = cPlane.DistTo(GetAABBCornerPos(eCorner, vBoxMin, vBoxMax));
	return fNearDist < 0.001f;
}

inline bool GetAABBPlaneSideFront(EAABBCorner eCorner, const LTPlane &cPlane, const LTVector &vBoxMin, const LTVector &vBoxMax)
{
	float fFarDist = cPlane.DistTo(GetAABBCornerPos(GetAABBOtherCorner(eCorner), vBoxMin, vBoxMax));
	return fFarDist > 0.001f;
}

#endif
