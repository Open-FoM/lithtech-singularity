#pragma once

#include "editor_state.h"
#include "ui_viewport.h"

#include "DiligentCore/Common/interface/BasicMath.hpp"
#include "imgui.h"

struct GizmoHandle
{
	ViewportPanelState::GizmoAxis axis = ViewportPanelState::GizmoAxis::None;
	int axis_index = -1;
	Diligent::float3 dir = Diligent::float3(0.0f, 0.0f, 0.0f);
	Diligent::float3 ring_u = Diligent::float3(0.0f, 0.0f, 0.0f);
	Diligent::float3 ring_v = Diligent::float3(0.0f, 0.0f, 0.0f);
	ImVec2 start = ImVec2(0.0f, 0.0f);
	ImVec2 end = ImVec2(0.0f, 0.0f);
	float screen_len = 0.0f;
	float world_len = 0.0f;
};

struct GizmoDrawState
{
	bool origin_visible = false;
	ImVec2 origin_screen = ImVec2(0.0f, 0.0f);
	ImVec2 viewport_size = ImVec2(0.0f, 0.0f);
	Diligent::float4x4 view_proj = Diligent::float4x4::Identity();
	Diligent::float3 origin = Diligent::float3(0.0f, 0.0f, 0.0f);
	GizmoHandle handles[3];
};

bool ProjectWorldToScreen(
	const Diligent::float4x4& view_proj,
	const float position[3],
	const ImVec2& viewport_size,
	ImVec2& out_screen);

bool BuildGizmoDrawState(
	const ViewportPanelState& panel,
	const NodeProperties& props,
	const Diligent::float3& cam_pos,
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_size,
	GizmoDrawState& out_state);

void DrawGizmo(
	const ViewportPanelState& panel,
	const GizmoDrawState& state,
	const ImVec2& viewport_pos,
	ImDrawList* draw_list);

bool UpdateGizmoInteraction(
	ViewportPanelState& panel,
	const GizmoDrawState& state,
	NodeProperties& props,
	const ImVec2& mouse_local,
	bool mouse_down,
	bool mouse_clicked);
