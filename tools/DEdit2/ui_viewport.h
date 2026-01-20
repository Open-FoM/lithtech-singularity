#pragma once

#include "imgui.h"

struct ViewportPanelState
{
	bool initialized = false;
	bool show_grid = true;
	bool show_axes = true;
	float orbit_yaw = 0.0f;
	float orbit_pitch = 0.0f;
	float orbit_distance = 800.0f;
	float target[3] = {0.0f, 0.0f, 0.0f};
	bool fly_mode = false;
	float fly_pos[3] = {0.0f, 0.0f, 0.0f};
	float fly_speed = 800.0f;
	float grid_spacing = 64.0f;
	enum class GizmoMode
	{
		Translate,
		Rotate,
		Scale
	};
	enum class GizmoAxis
	{
		None,
		X,
		Y,
		Z
	};
	enum class GizmoSpace
	{
		World,
		Local
	};
	GizmoMode gizmo_mode = GizmoMode::Translate;
	GizmoAxis gizmo_axis = GizmoAxis::None;
	GizmoSpace gizmo_space = GizmoSpace::World;
	GizmoSpace rotation_space = GizmoSpace::World;
	bool gizmo_dragging = false;
	ImVec2 gizmo_drag_start_mouse = ImVec2(0.0f, 0.0f);
	float gizmo_drag_start_angle = 0.0f;
	float gizmo_start_pos[3] = {0.0f, 0.0f, 0.0f};
	float gizmo_start_rot[3] = {0.0f, 0.0f, 0.0f};
	float gizmo_start_scale[3] = {1.0f, 1.0f, 1.0f};
	float gizmo_size = 1.0f;
	float gizmo_axis_scale[3] = {1.0f, 1.0f, 1.0f};
	bool snap_translate = false;
	bool snap_rotate = false;
	bool snap_scale = false;
	bool snap_translate_axis[3] = {true, true, true};
	bool snap_rotate_axis[3] = {true, true, true};
	bool snap_scale_axis[3] = {true, true, true};
	float snap_translate_step = 1.0f;
	float snap_rotate_step = 15.0f;
	float snap_scale_step = 0.1f;
};

void ResetViewportPanel(ViewportPanelState& state, const ImVec2& size);
void SyncFlyFromOrbit(ViewportPanelState& state);
void SyncOrbitFromFly(ViewportPanelState& state);
void UpdateViewportControls(ViewportPanelState& state, const ImVec2& origin, const ImVec2& size, bool hovered);
void DrawViewportOverlay(const ViewportPanelState& state, ImDrawList* draw_list, const ImVec2& origin, const ImVec2& size);
