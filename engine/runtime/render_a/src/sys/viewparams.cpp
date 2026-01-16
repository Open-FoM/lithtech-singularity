#include "viewparams.h"

bool ViewParams::ViewAABBIntersect(const LTVector &vBoxMin, const LTVector &vBoxMax) const
{
	if ((vBoxMin.x > m_ViewAABBMax.x) ||
		(vBoxMin.y > m_ViewAABBMax.y) ||
		(vBoxMin.z > m_ViewAABBMax.z) ||
		(vBoxMax.x < m_ViewAABBMin.x) ||
		(vBoxMax.y < m_ViewAABBMin.y) ||
		(vBoxMax.z < m_ViewAABBMin.z))
	{
		return false;
	}

	for (uint32 nPlaneLoop = 0; nPlaneLoop < NUM_CLIPPLANES; ++nPlaneLoop)
	{
		if (GetAABBPlaneSideBack(m_AABBPlaneCorner[nPlaneLoop], m_ClipPlanes[nPlaneLoop], vBoxMin, vBoxMax))
		{
			return false;
		}
	}

	return true;
}
