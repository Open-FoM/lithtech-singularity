#include "aabb.h"

EAABBCorner GetAABBPlaneCorner(const LTVector &vNormal)
{
	static LTVector aCornerDir[8] = {
		LTVector(-1.0f, 1.0f, -1.0f),
		LTVector(1.0f, 1.0f, -1.0f),
		LTVector(-1.0f, -1.0f, -1.0f),
		LTVector(1.0f, -1.0f, -1.0f),
		LTVector(-1.0f, 1.0f, 1.0f),
		LTVector(1.0f, 1.0f, 1.0f),
		LTVector(-1.0f, -1.0f, 1.0f),
		LTVector(1.0f, -1.0f, 1.0f)
	};

	uint32 nBestCorner = 0;
	float fBestCornerDot = -1.0f;
	for (uint32 nCornerLoop = 0; nCornerLoop < 8; ++nCornerLoop)
	{
		float fCornerDot = vNormal.Dot(aCornerDir[nCornerLoop]);
		if (fCornerDot > fBestCornerDot)
		{
			nBestCorner = nCornerLoop;
			fBestCornerDot = fCornerDot;
		}
	}

	return static_cast<EAABBCorner>(nBestCorner);
}
