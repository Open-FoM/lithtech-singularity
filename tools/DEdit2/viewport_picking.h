#pragma once

#include "editor_state.h"
#include "ui_viewport.h"

#include "DiligentCore/Common/interface/BasicMath.hpp"

#include "imgui.h"

#include <cstdint>

struct PickRay
{
	Diligent::float3 origin;
	Diligent::float3 dir;
};

void ComputeCameraBasis(
	const ViewportPanelState& state,
	Diligent::float3& out_pos,
	Diligent::float3& out_forward,
	Diligent::float3& out_right,
	Diligent::float3& out_up);

PickRay BuildPickRay(
	const ViewportPanelState& panel,
	const ImVec2& viewport_size,
	const ImVec2& mouse_local);

bool TryGetNodePickPosition(const NodeProperties& props, float out[3]);
bool TryGetNodeBounds(const NodeProperties& props, float out_min[3], float out_max[3]);
bool RaycastNode(const NodeProperties& props, const PickRay& ray, float& out_t);
