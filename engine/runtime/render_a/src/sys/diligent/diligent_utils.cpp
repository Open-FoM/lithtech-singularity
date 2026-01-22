#include "diligent_utils.h"

#include "diligent_state.h"
#include "ltrotation.h"
#include "ltvector.h"
#include "viewparams.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"

#include <cstring>

uint64 diligent_hash_combine(uint64 seed, uint64 value)
{
	return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

uint32 diligent_float_bits(float value)
{
	uint32 bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

void diligent_to_clip_space(float x, float y, float& out_x, float& out_y)
{
	out_x = (x / static_cast<float>(g_diligent_state.screen_width)) * 2.0f - 1.0f;
	out_y = 1.0f - (y / static_cast<float>(g_diligent_state.screen_height)) * 2.0f;
}

bool diligent_to_view_clip_space(float x, float y, float& out_x, float& out_y)
{
	const float view_width = g_diligent_state.view_params.m_fScreenWidth;
	const float view_height = g_diligent_state.view_params.m_fScreenHeight;
	if (view_width <= 0.0f || view_height <= 0.0f)
	{
		out_x = 0.0f;
		out_y = 0.0f;
		return false;
	}

	out_x = (x / view_width) * 2.0f - 1.0f;
	out_y = 1.0f - (y / view_height) * 2.0f;
	return true;
}

void diligent_store_matrix_from_lt(const LTMatrix& matrix, std::array<float, 16>& out)
{
	out[0] = matrix.m[0][0];
	out[1] = matrix.m[1][0];
	out[2] = matrix.m[2][0];
	out[3] = matrix.m[3][0];
	out[4] = matrix.m[0][1];
	out[5] = matrix.m[1][1];
	out[6] = matrix.m[2][1];
	out[7] = matrix.m[3][1];
	out[8] = matrix.m[0][2];
	out[9] = matrix.m[1][2];
	out[10] = matrix.m[2][2];
	out[11] = matrix.m[3][2];
	out[12] = matrix.m[0][3];
	out[13] = matrix.m[1][3];
	out[14] = matrix.m[2][3];
	out[15] = matrix.m[3][3];
}

void diligent_store_identity_matrix(std::array<float, 16>& out)
{
	out = {};
	out[0] = 1.0f;
	out[5] = 1.0f;
	out[10] = 1.0f;
	out[15] = 1.0f;
}

LTMatrix diligent_build_transform(const LTVector& position, const LTRotation& rotation, const LTVector& scale)
{
	LTMatrix matrix;
	quat_ConvertToMatrix(reinterpret_cast<const float*>(&rotation), matrix.m);

	matrix.m[0][0] *= scale.x;
	matrix.m[1][0] *= scale.x;
	matrix.m[2][0] *= scale.x;

	matrix.m[0][1] *= scale.y;
	matrix.m[1][1] *= scale.y;
	matrix.m[2][1] *= scale.y;

	matrix.m[0][2] *= scale.z;
	matrix.m[1][2] *= scale.z;
	matrix.m[2][2] *= scale.z;

	matrix.m[0][3] = position.x;
	matrix.m[1][3] = position.y;
	matrix.m[2][3] = position.z;

	return matrix;
}

void diligent_set_viewport(float width, float height)
{
	if (!g_diligent_state.immediate_context)
	{
		return;
	}

	Diligent::Viewport viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	g_diligent_state.immediate_context->SetViewports(1, &viewport, 0, 0);
}

void diligent_set_viewport_rect(float left, float top, float width, float height)
{
	if (!g_diligent_state.immediate_context)
	{
		return;
	}

	Diligent::Viewport viewport;
	viewport.TopLeftX = left;
	viewport.TopLeftY = top;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	g_diligent_state.immediate_context->SetViewports(1, &viewport, 0, 0);
}
