#include "viewport_gizmo.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kPi = 3.1415926f;
constexpr int kRingSegments = 64;
constexpr float kRingRadiusScale = 0.65f;
constexpr float kPickThreshold = 9.0f;
constexpr float kLabelOffset = 8.0f;

Diligent::float3 Cross(const Diligent::float3& a, const Diligent::float3& b)
{
	return Diligent::float3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

Diligent::float3 Normalize(const Diligent::float3& v)
{
	const float len_sq = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len_sq <= 0.0f)
	{
		return Diligent::float3(0.0f, 0.0f, 0.0f);
	}
	const float inv_len = 1.0f / std::sqrt(len_sq);
	return Diligent::float3(v.x * inv_len, v.y * inv_len, v.z * inv_len);
}

Diligent::float3 RotateVectorEulerXYZ(const Diligent::float3& v, const float rot_deg[3])
{
	const float rx = rot_deg[0] * kPi / 180.0f;
	const float ry = rot_deg[1] * kPi / 180.0f;
	const float rz = rot_deg[2] * kPi / 180.0f;

	float cx = std::cos(rx);
	float sx = std::sin(rx);
	float cy = std::cos(ry);
	float sy = std::sin(ry);
	float cz = std::cos(rz);
	float sz = std::sin(rz);

	Diligent::float3 out = v;

	// X axis
	out = Diligent::float3(out.x, out.y * cx - out.z * sx, out.y * sx + out.z * cx);
	// Y axis
	out = Diligent::float3(out.x * cy + out.z * sy, out.y, -out.x * sy + out.z * cy);
	// Z axis
	out = Diligent::float3(out.x * cz - out.y * sz, out.x * sz + out.y * cz, out.z);

	return out;
}

Diligent::float3x3 EulerToMatrixXYZ(const float rot_deg[3])
{
	const float rx = rot_deg[0] * kPi / 180.0f;
	const float ry = rot_deg[1] * kPi / 180.0f;
	const float rz = rot_deg[2] * kPi / 180.0f;

	const Diligent::float3x3 rxm = Diligent::float3x3::RotationX(rx);
	const Diligent::float3x3 rym = Diligent::float3x3::RotationY(ry);
	const Diligent::float3x3 rzm = Diligent::float3x3::RotationZ(rz);
	return Diligent::float3x3::Mul(Diligent::float3x3::Mul(rxm, rym), rzm);
}

void MatrixToEulerXYZ(const Diligent::float3x3& m, float out_deg[3])
{
	float sy = -m[0][2];
	sy = std::max(-1.0f, std::min(1.0f, sy));
	const float y = std::asin(sy);
	const float cy = std::cos(y);

	float x = 0.0f;
	float z = 0.0f;
	if (std::fabs(cy) > 1e-5f)
	{
		x = std::atan2(m[1][2], m[2][2]);
		z = std::atan2(m[0][1], m[0][0]);
	}
	else
	{
		x = std::atan2(m[1][0], m[1][1]);
		z = 0.0f;
	}

	out_deg[0] = x * 180.0f / kPi;
	out_deg[1] = y * 180.0f / kPi;
	out_deg[2] = z * 180.0f / kPi;
}

float DistancePointToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b)
{
	const ImVec2 ab(b.x - a.x, b.y - a.y);
	const ImVec2 ap(p.x - a.x, p.y - a.y);
	const float ab_len2 = ab.x * ab.x + ab.y * ab.y;
	if (ab_len2 <= 0.0001f)
	{
		const float dx = p.x - a.x;
		const float dy = p.y - a.y;
		return std::sqrt(dx * dx + dy * dy);
	}
	const float t = std::max(0.0f, std::min(1.0f, (ap.x * ab.x + ap.y * ab.y) / ab_len2));
	const float proj_x = a.x + ab.x * t;
	const float proj_y = a.y + ab.y * t;
	const float dx = p.x - proj_x;
	const float dy = p.y - proj_y;
	return std::sqrt(dx * dx + dy * dy);
}

ImU32 AxisColor(ViewportPanelState::GizmoAxis axis, bool active)
{
	switch (axis)
	{
		case ViewportPanelState::GizmoAxis::X:
			return active ? IM_COL32(255, 120, 120, 255) : IM_COL32(220, 90, 90, 220);
		case ViewportPanelState::GizmoAxis::Y:
			return active ? IM_COL32(120, 255, 140, 255) : IM_COL32(90, 220, 110, 220);
		case ViewportPanelState::GizmoAxis::Z:
			return active ? IM_COL32(120, 170, 255, 255) : IM_COL32(90, 140, 220, 220);
		default:
			return IM_COL32(200, 200, 200, 200);
	}
}

const char* AxisLabel(ViewportPanelState::GizmoAxis axis)
{
	switch (axis)
	{
		case ViewportPanelState::GizmoAxis::X:
			return "X";
		case ViewportPanelState::GizmoAxis::Y:
			return "Y";
		case ViewportPanelState::GizmoAxis::Z:
			return "Z";
		default:
			return "";
	}
}

float SnapValue(float value, float step)
{
	if (step <= 0.0f)
	{
		return value;
	}
	return std::round(value / step) * step;
}

const GizmoHandle* FindHandle(const GizmoDrawState& state, ViewportPanelState::GizmoAxis axis)
{
	for (const auto& handle : state.handles)
	{
		if (handle.axis == axis)
		{
			return &handle;
		}
	}
	return nullptr;
}

bool ComputeRingPoint(
	const GizmoDrawState& state,
	const GizmoHandle& handle,
	float ring_radius,
	float angle,
	ImVec2& out_screen)
{
	const float c = std::cos(angle) * ring_radius;
	const float s = std::sin(angle) * ring_radius;
	const Diligent::float3 ring(handle.ring_u.x * c + handle.ring_v.x * s,
		handle.ring_u.y * c + handle.ring_v.y * s,
		handle.ring_u.z * c + handle.ring_v.z * s);
	float ring_pos[3] = {
		state.origin.x + ring.x,
		state.origin.y + ring.y,
		state.origin.z + ring.z};
	return ProjectWorldToScreen(state.view_proj, ring_pos, state.viewport_size, out_screen);
}
}

bool ProjectWorldToScreen(
	const Diligent::float4x4& view_proj,
	const float position[3],
	const ImVec2& viewport_size,
	ImVec2& out_screen)
{
	const Diligent::float4 world(position[0], position[1], position[2], 1.0f);
	const Diligent::float4 clip = view_proj * world;
	if (clip.w <= 0.0001f)
	{
		return false;
	}

	const float inv_w = 1.0f / clip.w;
	const float ndc_x = clip.x * inv_w;
	const float ndc_y = clip.y * inv_w;
	const float ndc_z = clip.z * inv_w;
	if (ndc_z < 0.0f || ndc_z > 1.0f)
	{
		return false;
	}

	const float x = (ndc_x * 0.5f + 0.5f) * viewport_size.x;
	const float y = (1.0f - (ndc_y * 0.5f + 0.5f)) * viewport_size.y;
	out_screen = ImVec2(x, y);
	return true;
}

bool BuildGizmoDrawState(
	const ViewportPanelState& panel,
	const NodeProperties& props,
	const Diligent::float3& cam_pos,
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_size,
	GizmoDrawState& out_state)
{
	out_state = GizmoDrawState{};
	out_state.viewport_size = viewport_size;
	out_state.view_proj = view_proj;
	out_state.origin = Diligent::float3(props.position[0], props.position[1], props.position[2]);

	const float dist_to_cam = std::sqrt(
		(out_state.origin.x - cam_pos.x) * (out_state.origin.x - cam_pos.x) +
		(out_state.origin.y - cam_pos.y) * (out_state.origin.y - cam_pos.y) +
		(out_state.origin.z - cam_pos.z) * (out_state.origin.z - cam_pos.z));
	const float handle_len = std::max(80.0f, dist_to_cam * 0.15f) * std::max(0.1f, panel.gizmo_size);

	Diligent::float3 axis_dirs[3] = {
		Diligent::float3(1.0f, 0.0f, 0.0f),
		Diligent::float3(0.0f, 1.0f, 0.0f),
		Diligent::float3(0.0f, 0.0f, 1.0f)};
	const bool use_local = panel.gizmo_mode == ViewportPanelState::GizmoMode::Rotate ?
		(panel.rotation_space == ViewportPanelState::GizmoSpace::Local) :
		(panel.gizmo_space == ViewportPanelState::GizmoSpace::Local);
	if (use_local)
	{
		for (auto& dir : axis_dirs)
		{
			dir = Normalize(RotateVectorEulerXYZ(dir, props.rotation));
		}
	}

	out_state.handles[0].axis = ViewportPanelState::GizmoAxis::X;
	out_state.handles[1].axis = ViewportPanelState::GizmoAxis::Y;
	out_state.handles[2].axis = ViewportPanelState::GizmoAxis::Z;

	out_state.origin_visible = ProjectWorldToScreen(view_proj, props.position, viewport_size, out_state.origin_screen);
	if (!out_state.origin_visible)
	{
		return false;
	}

	for (int i = 0; i < 3; ++i)
	{
		GizmoHandle& handle = out_state.handles[i];
		handle.axis_index = i;
		handle.dir = axis_dirs[i];
		handle.world_len = handle_len * std::max(0.1f, panel.gizmo_axis_scale[i]);

		const Diligent::float3 axis_dir = handle.dir;
		const Diligent::float3 ref = (std::fabs(axis_dir.y) < 0.9f) ?
			Diligent::float3(0.0f, 1.0f, 0.0f) : Diligent::float3(1.0f, 0.0f, 0.0f);
		handle.ring_u = Normalize(Cross(ref, axis_dir));
		handle.ring_v = Cross(axis_dir, handle.ring_u);

		float axis_end[3] = {
			props.position[0] + handle.dir.x * handle.world_len,
			props.position[1] + handle.dir.y * handle.world_len,
			props.position[2] + handle.dir.z * handle.world_len};
		ImVec2 axis_screen;
		if (ProjectWorldToScreen(view_proj, axis_end, viewport_size, axis_screen))
		{
			handle.start = out_state.origin_screen;
			handle.end = axis_screen;
			const float dx = axis_screen.x - out_state.origin_screen.x;
			const float dy = axis_screen.y - out_state.origin_screen.y;
			handle.screen_len = std::sqrt(dx * dx + dy * dy);
		}
	}

	return true;
}

void DrawGizmo(
	const ViewportPanelState& panel,
	const GizmoDrawState& state,
	const ImVec2& viewport_pos,
	ImDrawList* draw_list)
{
	if (!state.origin_visible || !draw_list)
	{
		return;
	}

	if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Rotate)
	{
		for (const auto& handle : state.handles)
		{
			const float ring_radius = handle.world_len * kRingRadiusScale;
			const bool active_axis = panel.gizmo_axis == handle.axis && panel.gizmo_dragging;
			ImU32 color = AxisColor(handle.axis, active_axis);

			ImVec2 prev;
			bool prev_valid = false;
			for (int seg = 0; seg <= kRingSegments; ++seg)
			{
				const float angle = static_cast<float>(seg) / static_cast<float>(kRingSegments) * 2.0f * kPi;
				ImVec2 screen_point;
				if (!ComputeRingPoint(state, handle, ring_radius, angle, screen_point))
				{
					prev_valid = false;
					continue;
				}
				if (prev_valid)
				{
					draw_list->AddLine(
						ImVec2(viewport_pos.x + prev.x, viewport_pos.y + prev.y),
						ImVec2(viewport_pos.x + screen_point.x, viewport_pos.y + screen_point.y),
						color,
						active_axis ? 2.5f : 1.5f);
				}
				prev = screen_point;
				prev_valid = true;
			}

			ImVec2 label_point;
			if (ComputeRingPoint(state, handle, ring_radius, 0.0f, label_point))
			{
				ImVec2 screen_dir(label_point.x - state.origin_screen.x, label_point.y - state.origin_screen.y);
				const float len2 = screen_dir.x * screen_dir.x + screen_dir.y * screen_dir.y;
				if (len2 > 0.001f)
				{
					const float inv_len = 1.0f / std::sqrt(len2);
					screen_dir.x *= inv_len;
					screen_dir.y *= inv_len;
				}
				const ImVec2 label_pos(
					viewport_pos.x + label_point.x + screen_dir.x * kLabelOffset,
					viewport_pos.y + label_point.y + screen_dir.y * kLabelOffset);
				draw_list->AddText(label_pos, color, AxisLabel(handle.axis));
			}
		}
	}
	else
	{
		for (const auto& handle : state.handles)
		{
			if (handle.screen_len <= 0.0f)
			{
				continue;
			}
			const bool active_axis = panel.gizmo_axis == handle.axis && panel.gizmo_dragging;
			const ImU32 color = AxisColor(handle.axis, active_axis);
			draw_list->AddLine(
				ImVec2(viewport_pos.x + handle.start.x, viewport_pos.y + handle.start.y),
				ImVec2(viewport_pos.x + handle.end.x, viewport_pos.y + handle.end.y),
				color,
				active_axis ? 3.0f : 2.0f);

			const ImVec2 dir_screen(handle.end.x - handle.start.x, handle.end.y - handle.start.y);
			const float len2 = dir_screen.x * dir_screen.x + dir_screen.y * dir_screen.y;
			ImVec2 dir_norm(0.0f, 0.0f);
			if (len2 > 0.001f)
			{
				const float inv_len = 1.0f / std::sqrt(len2);
				dir_norm.x = dir_screen.x * inv_len;
				dir_norm.y = dir_screen.y * inv_len;
			}
			const ImVec2 label_pos(
				viewport_pos.x + handle.end.x + dir_norm.x * kLabelOffset,
				viewport_pos.y + handle.end.y + dir_norm.y * kLabelOffset);
			draw_list->AddText(label_pos, color, AxisLabel(handle.axis));
		}
	}
}

bool UpdateGizmoInteraction(
	ViewportPanelState& panel,
	const GizmoDrawState& state,
	NodeProperties& props,
	const ImVec2& mouse_local,
	bool mouse_down,
	bool mouse_clicked)
{
	if (!state.origin_visible)
	{
		if (panel.gizmo_dragging && !mouse_down)
		{
			panel.gizmo_dragging = false;
			panel.gizmo_axis = ViewportPanelState::GizmoAxis::None;
		}
		return false;
	}

	if (panel.gizmo_dragging)
	{
		if (!mouse_down)
		{
			panel.gizmo_dragging = false;
			panel.gizmo_axis = ViewportPanelState::GizmoAxis::None;
			return false;
		}

		const GizmoHandle* active = FindHandle(state, panel.gizmo_axis);
		if (!active || active->screen_len <= 0.0f)
		{
			return false;
		}

		const ImVec2 axis_dir(
			(active->end.x - active->start.x) / active->screen_len,
			(active->end.y - active->start.y) / active->screen_len);
		const ImVec2 delta(
			mouse_local.x - panel.gizmo_drag_start_mouse.x,
			mouse_local.y - panel.gizmo_drag_start_mouse.y);
		const float scalar = delta.x * axis_dir.x + delta.y * axis_dir.y;
		const float world_per_pixel = active->world_len / active->screen_len;
		const int axis_index = active->axis_index;

		if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Translate)
		{
			props.position[0] = panel.gizmo_start_pos[0];
			props.position[1] = panel.gizmo_start_pos[1];
			props.position[2] = panel.gizmo_start_pos[2];
			float world_delta = scalar * world_per_pixel;
			if (panel.snap_translate && panel.snap_translate_axis[axis_index])
			{
				world_delta = SnapValue(world_delta, panel.snap_translate_step);
			}
			props.position[axis_index] += world_delta;
		}
		else if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Rotate)
		{
			props.rotation[0] = panel.gizmo_start_rot[0];
			props.rotation[1] = panel.gizmo_start_rot[1];
			props.rotation[2] = panel.gizmo_start_rot[2];
			const float angle_now = std::atan2(
				mouse_local.y - state.origin_screen.y,
				mouse_local.x - state.origin_screen.x);
			float delta_rad = angle_now - panel.gizmo_drag_start_angle;
			float delta_deg = delta_rad * 180.0f / kPi;
			if (panel.snap_rotate && panel.snap_rotate_axis[axis_index])
			{
				delta_deg = SnapValue(delta_deg, panel.snap_rotate_step);
			}
			delta_rad = delta_deg * kPi / 180.0f;
			const Diligent::float3x3 base = EulerToMatrixXYZ(panel.gizmo_start_rot);
			Diligent::float3x3 delta;
			if (axis_index == 0)
			{
				delta = Diligent::float3x3::RotationX(delta_rad);
			}
			else if (axis_index == 1)
			{
				delta = Diligent::float3x3::RotationY(delta_rad);
			}
			else
			{
				delta = Diligent::float3x3::RotationZ(delta_rad);
			}

			Diligent::float3x3 combined;
			if (panel.rotation_space == ViewportPanelState::GizmoSpace::World)
			{
				combined = Diligent::float3x3::Mul(delta, base);
			}
			else
			{
				combined = Diligent::float3x3::Mul(base, delta);
			}
			MatrixToEulerXYZ(combined, props.rotation);
		}
		else
		{
			props.scale[0] = panel.gizmo_start_scale[0];
			props.scale[1] = panel.gizmo_start_scale[1];
			props.scale[2] = panel.gizmo_start_scale[2];
			float delta_scale = scalar * 0.01f;
			if (panel.snap_scale && panel.snap_scale_axis[axis_index])
			{
				delta_scale = SnapValue(delta_scale, panel.snap_scale_step);
			}
			props.scale[axis_index] = std::max(0.01f, props.scale[axis_index] + delta_scale);
		}

		return false;
	}

	if (!mouse_clicked)
	{
		return false;
	}

	float best_dist = kPickThreshold;
	ViewportPanelState::GizmoAxis hit_axis = ViewportPanelState::GizmoAxis::None;
	if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Rotate)
	{
		for (const auto& handle : state.handles)
		{
			const float ring_radius = handle.world_len * kRingRadiusScale;
			ImVec2 prev;
			bool prev_valid = false;
			for (int seg = 0; seg <= kRingSegments; ++seg)
			{
				const float angle = static_cast<float>(seg) / static_cast<float>(kRingSegments) * 2.0f * kPi;
				ImVec2 screen_point;
				if (!ComputeRingPoint(state, handle, ring_radius, angle, screen_point))
				{
					prev_valid = false;
					continue;
				}
				if (prev_valid)
				{
					const float dist = DistancePointToSegment(mouse_local, prev, screen_point);
					if (dist < best_dist)
					{
						best_dist = dist;
						hit_axis = handle.axis;
					}
				}
				prev = screen_point;
				prev_valid = true;
			}
		}
	}
	else
	{
		for (const auto& handle : state.handles)
		{
			if (handle.screen_len <= 0.0f)
			{
				continue;
			}
			const float dist = DistancePointToSegment(mouse_local, handle.start, handle.end);
			if (dist < best_dist)
			{
				best_dist = dist;
				hit_axis = handle.axis;
			}
		}
	}

	if (hit_axis == ViewportPanelState::GizmoAxis::None)
	{
		return false;
	}

	panel.gizmo_dragging = true;
	panel.gizmo_axis = hit_axis;
	panel.gizmo_drag_start_mouse = mouse_local;
	panel.gizmo_drag_start_angle = std::atan2(
		mouse_local.y - state.origin_screen.y,
		mouse_local.x - state.origin_screen.x);
	panel.gizmo_start_pos[0] = props.position[0];
	panel.gizmo_start_pos[1] = props.position[1];
	panel.gizmo_start_pos[2] = props.position[2];
	panel.gizmo_start_rot[0] = props.rotation[0];
	panel.gizmo_start_rot[1] = props.rotation[1];
	panel.gizmo_start_rot[2] = props.rotation[2];
	panel.gizmo_start_scale[0] = props.scale[0];
	panel.gizmo_start_scale[1] = props.scale[1];
	panel.gizmo_start_scale[2] = props.scale[2];
	return true;
}
