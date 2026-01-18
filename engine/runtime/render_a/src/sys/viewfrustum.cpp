#include "viewparams.h"
#include "ltrotation.h"
#include "ltbasedefs.h"

#include <cmath>

namespace
{
constexpr float kMaxFarZ = 500000.0f;
constexpr uint32 kPlaneNearIndex = 0;
constexpr uint32 kPlaneFarIndex = 1;
constexpr uint32 kPlaneLeftIndex = 2;
constexpr uint32 kPlaneTopIndex = 3;
constexpr uint32 kPlaneRightIndex = 4;
constexpr uint32 kPlaneBottomIndex = 5;

int32 lt_RoundFloatToInt(float value)
{
	return static_cast<int32>(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

void lt_SetupPerspectiveMatrix(LTMatrix* matrix, float near_z, float far_z)
{
	matrix->Identity();
	matrix->m[2][2] = far_z / (far_z - near_z);
	matrix->m[2][3] = (-far_z * near_z) / (far_z - near_z);
	matrix->m[3][2] = 1.0f;
	matrix->m[3][3] = 0.0f;
}

void lt_InitViewBox(ViewBoxDef* view_box, float near_z, float far_z, float x_fov, float y_fov)
{
	float x_temp = MATH_HALFPI - (x_fov * 0.5f);
	float y_temp = MATH_HALFPI - (y_fov * 0.5f);

	view_box->m_NearZ = near_z;
	view_box->m_FarZ = far_z;
	view_box->m_WindowSize[0] = static_cast<float>(std::cos(x_temp) / std::sin(x_temp));
	view_box->m_WindowSize[1] = static_cast<float>(std::cos(y_temp) / std::sin(y_temp));
	view_box->m_COP.Init(0.0f, 0.0f, 1.0f);
}
}

bool lt_InitFrustrum(
	ViewParams* params,
	ViewBoxDef* view_box,
	float screen_min_x,
	float screen_min_y,
	float screen_max_x,
	float screen_max_y,
	const LTMatrix* matrix,
	const LTVector& scale,
	ViewParams::ERenderMode render_mode)
{
	if (!params || !view_box || !matrix)
	{
		return false;
	}

	LTMatrix temp_world;
	LTMatrix rotation;
	LTMatrix scale_matrix;
	LTMatrix fov_scale;
	LTMatrix back_transform;
	LTMatrix device_transform;
	LTMatrix back_translate;
	LTMatrix projection_transform;
	float left_x = 0.0f;
	float right_x = 0.0f;
	float top_y = 0.0f;
	float bottom_y = 0.0f;
	float normal_z = 0.0f;
	LTVector forward_vec;
	LTVector z_plane_pos;

	params->m_mIdentity.Identity();
	params->m_mInvView = *matrix;
	params->m_ViewBox = *view_box;
	matrix->GetTranslation(params->m_Pos);

	params->m_FarZ = view_box->m_FarZ;
	if (params->m_FarZ < 3.0f)
	{
		params->m_FarZ = 3.0f;
	}
	if (params->m_FarZ > kMaxFarZ)
	{
		params->m_FarZ = kMaxFarZ;
	}
	params->m_NearZ = view_box->m_NearZ;

	params->m_Rect.left = lt_RoundFloatToInt(screen_min_x);
	params->m_Rect.top = lt_RoundFloatToInt(screen_min_y);
	params->m_Rect.right = lt_RoundFloatToInt(screen_max_x);
	params->m_Rect.bottom = lt_RoundFloatToInt(screen_max_y);

	rotation = *matrix;
	rotation.SetTranslation(0.0f, 0.0f, 0.0f);
	Mat_GetBasisVectors(&rotation, &params->m_Right, &params->m_Up, &params->m_Forward);
	MatTranspose3x3(&rotation);
	back_translate.Init(
		1.0f, 0.0f, 0.0f, -params->m_Pos.x,
		0.0f, 1.0f, 0.0f, -params->m_Pos.y,
		0.0f, 0.0f, 1.0f, -params->m_Pos.z,
		0.0f, 0.0f, 0.0f, 1.0f);
	MatMul(&temp_world, &rotation, &back_translate);

	scale_matrix.Init(
		scale.x, 0.0f, 0.0f, 0.0f,
		0.0f, scale.y, 0.0f, 0.0f,
		0.0f, 0.0f, scale.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	MatMul(&params->m_mView, &scale_matrix, &temp_world);

	LTMatrix shear;
	shear.Init(
		1.0f, 0.0f, -view_box->m_COP.x / view_box->m_COP.z, 0.0f,
		0.0f, 1.0f, -view_box->m_COP.y / view_box->m_COP.z, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	const float fov_x_scale = view_box->m_COP.z / view_box->m_WindowSize[0];
	const float fov_y_scale = view_box->m_COP.z / view_box->m_WindowSize[1];
	fov_scale.Init(
		fov_x_scale, 0.0f, 0.0f, 0.0f,
		0.0f, fov_y_scale, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	lt_SetupPerspectiveMatrix(&projection_transform, view_box->m_NearZ, params->m_FarZ);

	params->m_fScreenWidth = (screen_max_x - screen_min_x);
	params->m_fScreenHeight = (screen_max_y - screen_min_y);

	Mat_Identity(&device_transform);
	device_transform.m[0][0] = params->m_fScreenWidth * 0.5f - 0.0001f;
	device_transform.m[0][3] = screen_min_x + params->m_fScreenWidth * 0.5f;
	device_transform.m[1][1] = -(params->m_fScreenHeight * 0.5f - 0.0001f);
	device_transform.m[1][3] = screen_min_y + params->m_fScreenHeight * 0.5f;

	params->m_DeviceTimesProjection = device_transform * projection_transform;
	params->m_FullTransform = params->m_DeviceTimesProjection * fov_scale * shear * params->m_mView;
	params->m_mProjection = projection_transform * fov_scale * shear;

	float x_near_z = (params->m_NearZ * view_box->m_WindowSize[0]) / view_box->m_COP.z;
	float y_near_z = (params->m_NearZ * view_box->m_WindowSize[1]) / view_box->m_COP.z;
	float x_far_z = (params->m_FarZ * view_box->m_WindowSize[0]) / view_box->m_COP.z;
	float y_far_z = (params->m_FarZ * view_box->m_WindowSize[1]) / view_box->m_COP.z;

	params->m_ViewPoints[0].Init(-x_near_z, y_near_z, params->m_NearZ);
	params->m_ViewPoints[1].Init(x_near_z, y_near_z, params->m_NearZ);
	params->m_ViewPoints[2].Init(-x_near_z, -y_near_z, params->m_NearZ);
	params->m_ViewPoints[3].Init(x_near_z, -y_near_z, params->m_NearZ);
	params->m_ViewPoints[4].Init(-x_far_z, y_far_z, params->m_FarZ);
	params->m_ViewPoints[5].Init(x_far_z, y_far_z, params->m_FarZ);
	params->m_ViewPoints[6].Init(-x_far_z, -y_far_z, params->m_FarZ);
	params->m_ViewPoints[7].Init(x_far_z, -y_far_z, params->m_FarZ);

	for (uint32 i = 0; i < 8; ++i)
	{
		MatVMul_InPlace_Transposed3x3(&params->m_mView, &params->m_ViewPoints[i]);
		params->m_ViewPoints[i] += params->m_Pos;
	}

	params->m_ViewAABBMin = params->m_ViewPoints[0];
	params->m_ViewAABBMax = params->m_ViewPoints[0];
	for (uint32 i = 1; i < 8; ++i)
	{
		VEC_MIN(params->m_ViewAABBMin, params->m_ViewAABBMin, params->m_ViewPoints[i]);
		VEC_MAX(params->m_ViewAABBMax, params->m_ViewAABBMax, params->m_ViewPoints[i]);
	}

	left_x = view_box->m_COP.x - view_box->m_WindowSize[0];
	right_x = view_box->m_COP.x + view_box->m_WindowSize[0];
	top_y = view_box->m_COP.y + view_box->m_WindowSize[1];
	bottom_y = view_box->m_COP.y - view_box->m_WindowSize[1];
	normal_z = view_box->m_COP.z;

	LTPlane cs_clip_planes[NUM_CLIPPLANES];
	cs_clip_planes[kPlaneNearIndex].m_Normal.Init(0.0f, 0.0f, 1.0f);
	cs_clip_planes[kPlaneNearIndex].m_Dist = 1.0f;
	cs_clip_planes[kPlaneFarIndex].m_Normal.Init(0.0f, 0.0f, -1.0f);
	cs_clip_planes[kPlaneFarIndex].m_Dist = -params->m_FarZ;
	cs_clip_planes[kPlaneLeftIndex].m_Normal.Init(normal_z, 0.0f, -left_x);
	cs_clip_planes[kPlaneRightIndex].m_Normal.Init(-normal_z, 0.0f, right_x);
	cs_clip_planes[kPlaneTopIndex].m_Normal.Init(0.0f, -normal_z, top_y);
	cs_clip_planes[kPlaneBottomIndex].m_Normal.Init(0.0f, normal_z, -bottom_y);

	cs_clip_planes[kPlaneLeftIndex].m_Normal.Norm();
	cs_clip_planes[kPlaneTopIndex].m_Normal.Norm();
	cs_clip_planes[kPlaneRightIndex].m_Normal.Norm();
	cs_clip_planes[kPlaneBottomIndex].m_Normal.Norm();

	cs_clip_planes[kPlaneLeftIndex].m_Dist = 0.0f;
	cs_clip_planes[kPlaneRightIndex].m_Dist = 0.0f;
	cs_clip_planes[kPlaneTopIndex].m_Dist = 0.0f;
	cs_clip_planes[kPlaneBottomIndex].m_Dist = 0.0f;

	back_transform = params->m_mView;
	MatTranspose3x3(&back_transform);
	for (uint32 i = 0; i < NUM_CLIPPLANES; ++i)
	{
		if (i != kPlaneNearIndex && i != kPlaneFarIndex)
		{
			MatVMul_3x3(&params->m_ClipPlanes[i].m_Normal, &back_transform, &cs_clip_planes[i].m_Normal);
			params->m_ClipPlanes[i].m_Dist = params->m_ClipPlanes[i].m_Normal.Dot(params->m_Pos);
		}
	}

	forward_vec.Init(rotation.m[2][0], rotation.m[2][1], rotation.m[2][2]);

	z_plane_pos = forward_vec * view_box->m_NearZ;
	z_plane_pos += params->m_Pos;
	MatVMul_3x3(&params->m_ClipPlanes[kPlaneNearIndex].m_Normal, &back_transform, &cs_clip_planes[kPlaneNearIndex].m_Normal);
	params->m_ClipPlanes[kPlaneNearIndex].m_Dist = params->m_ClipPlanes[kPlaneNearIndex].m_Normal.Dot(z_plane_pos);

	z_plane_pos = forward_vec * params->m_FarZ;
	z_plane_pos += params->m_Pos;
	MatVMul_3x3(&params->m_ClipPlanes[kPlaneFarIndex].m_Normal, &back_transform, &cs_clip_planes[kPlaneFarIndex].m_Normal);
	params->m_ClipPlanes[kPlaneFarIndex].m_Dist = params->m_ClipPlanes[kPlaneFarIndex].m_Normal.Dot(z_plane_pos);

	for (uint32 i = 0; i < NUM_CLIPPLANES; ++i)
	{
		params->m_AABBPlaneCorner[i] = GetAABBPlaneCorner(params->m_ClipPlanes[i].m_Normal);
	}

	params->m_mInvWorld.Identity();
	params->m_mWorldEnvMap.SetBasisVectors(&params->m_Right, &params->m_Up, &params->m_Forward);
	params->m_eRenderMode = render_mode;
	return true;
}

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
	ViewParams::ERenderMode render_mode)
{
	if (!params || !position || !rotation)
	{
		return false;
	}

	ViewBoxDef view_box;
	lt_InitViewBox(&view_box, near_z, far_z, x_fov, y_fov);

	LTMatrix matrix;
	rotation->ConvertToMatrix(matrix);
	matrix.SetTranslation(*position);

	return lt_InitFrustrum(
		params,
		&view_box,
		screen_min_x,
		screen_min_y,
		screen_max_x,
		screen_max_y,
		&matrix,
		LTVector(1.0f, 1.0f, 1.0f),
		render_mode);
}

void d3d_InitViewBox2(
	ViewBoxDef* pDef,
	float nearZ,
	float farZ,
	const ViewParams& prev_params,
	float /*screen_min_x*/,
	float /*screen_min_y*/,
	float /*screen_max_x*/,
	float /*screen_max_y*/)
{
	if (!pDef)
	{
		return;
	}

	*pDef = prev_params.m_ViewBox;
	pDef->m_NearZ = nearZ;
	pDef->m_FarZ = farZ;
}
