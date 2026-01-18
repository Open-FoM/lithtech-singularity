#ifndef __VIEWPARAMS_H__
#define __VIEWPARAMS_H__

#ifndef __LTRECT_H__
#include "ltrect.h"
#endif

#ifndef __LTMATRIX_H__
#include "ltmatrix.h"
#endif

#ifndef __LTPLANE_H__
#include "ltplane.h"
#endif

#ifndef __AABB_H__
#include "aabb.h"
#endif

#define NUM_CLIPPLANES 6

struct ViewBoxDef
{
	LTVector m_COP;
	float m_WindowSize[2];
	float m_NearZ;
	float m_FarZ;
};

class ViewParams
{
public:
	bool ViewAABBIntersect(const LTVector &vBoxMin, const LTVector &vBoxMax) const;

public:
	ViewBoxDef m_ViewBox;
	LTRect m_Rect;
	float m_fScreenWidth;
	float m_fScreenHeight;
	float m_NearZ;
	float m_FarZ;
	LTMatrix m_mInvView;
	LTMatrix m_mView;
	LTMatrix m_mProjection;
	LTMatrix m_mWorldEnvMap;
	LTMatrix m_DeviceTimesProjection;
	LTMatrix m_FullTransform;
	LTMatrix m_mIdentity;
	LTMatrix m_mInvWorld;
	LTVector m_ViewPoints[8];
	LTVector m_ViewAABBMin;
	LTVector m_ViewAABBMax;
	LTPlane m_ClipPlanes[NUM_CLIPPLANES];
	EAABBCorner m_AABBPlaneCorner[NUM_CLIPPLANES];
	LTVector m_Up;
	LTVector m_Right;
	LTVector m_Forward;
	LTVector m_Pos;
	LTVector m_SkyViewPos;

	enum ERenderMode
	{
		eRenderMode_Normal,
		eRenderMode_Glow
	};

	ERenderMode m_eRenderMode;
};

void d3d_InitViewBox2(
	ViewBoxDef* pDef,
	float nearZ,
	float farZ,
	const ViewParams& prev_params,
	float screen_min_x,
	float screen_min_y,
	float screen_max_x,
	float screen_max_y);

bool lt_InitFrustrum(
	ViewParams* pParams,
	ViewBoxDef* pViewBox,
	float screenMinX,
	float screenMinY,
	float screenMaxX,
	float screenMaxY,
	const LTMatrix* pMat,
	const LTVector& vScale,
	ViewParams::ERenderMode eMode);

bool d3d_InitFrustum(
	ViewParams* params,
	float x_fov,
	float y_fov,
	float near_z,
	float far_z,
	float screen_min_x,
	float screen_min_y,
	float screen_max_x,
	float screen_max_y,
	const LTVector* position,
	const LTRotation* rotation,
	ViewParams::ERenderMode render_mode);

#endif
