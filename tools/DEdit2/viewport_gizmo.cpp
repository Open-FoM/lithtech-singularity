#include "viewport_gizmo.h"

#include "ui_scene.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kPi = 3.1415926f;
constexpr int kRingSegments = 64;
constexpr float kRingRadiusScale = 0.65f;
constexpr float kPickThreshold = 9.0f;
constexpr float kLabelOffset = 8.0f;
constexpr float kPlaneHandleScale = 0.25f;   ///< Plane handle position relative to axis length
constexpr float kUniformScaleBoxScale = 0.12f; ///< Center box size for uniform scale
constexpr float kScreenRingScale = 1.2f;     ///< Screen-space ring radius scale

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
		case ViewportPanelState::GizmoAxis::XY:
			return active ? IM_COL32(200, 200, 120, 255) : IM_COL32(160, 160, 90, 180);
		case ViewportPanelState::GizmoAxis::XZ:
			return active ? IM_COL32(200, 120, 200, 255) : IM_COL32(160, 90, 160, 180);
		case ViewportPanelState::GizmoAxis::YZ:
			return active ? IM_COL32(120, 200, 200, 255) : IM_COL32(90, 160, 160, 180);
		case ViewportPanelState::GizmoAxis::All:
			return active ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 200);
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
		case ViewportPanelState::GizmoAxis::XY:
			return "XY";
		case ViewportPanelState::GizmoAxis::XZ:
			return "XZ";
		case ViewportPanelState::GizmoAxis::YZ:
			return "YZ";
		case ViewportPanelState::GizmoAxis::All:
			return "All";
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
	// Diligent math is row-vector based; keep CPU projection consistent with shader math.
	const Diligent::float4 clip = world * view_proj;
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
	if (!props.visible)
	{
		return false;
	}
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
		// Draw axis handles
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

		// Draw plane handles (small squares at axis pair intersections) for translate mode
		if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Translate)
		{
			// XY plane handle
			const GizmoHandle& x_handle = state.handles[0];
			const GizmoHandle& y_handle = state.handles[1];
			const GizmoHandle& z_handle = state.handles[2];

			auto draw_plane_handle = [&](const GizmoHandle& h1, const GizmoHandle& h2, ViewportPanelState::GizmoAxis plane_axis) {
				if (h1.screen_len <= 0.0f || h2.screen_len <= 0.0f)
				{
					return;
				}
				const float offset1 = h1.screen_len * kPlaneHandleScale;
				const float offset2 = h2.screen_len * kPlaneHandleScale;
				const ImVec2 dir1((h1.end.x - h1.start.x) / h1.screen_len, (h1.end.y - h1.start.y) / h1.screen_len);
				const ImVec2 dir2((h2.end.x - h2.start.x) / h2.screen_len, (h2.end.y - h2.start.y) / h2.screen_len);

				ImVec2 corners[4] = {
					ImVec2(state.origin_screen.x, state.origin_screen.y),
					ImVec2(state.origin_screen.x + dir1.x * offset1, state.origin_screen.y + dir1.y * offset1),
					ImVec2(state.origin_screen.x + dir1.x * offset1 + dir2.x * offset2, state.origin_screen.y + dir1.y * offset1 + dir2.y * offset2),
					ImVec2(state.origin_screen.x + dir2.x * offset2, state.origin_screen.y + dir2.y * offset2)
				};

				const bool active = panel.gizmo_axis == plane_axis && panel.gizmo_dragging;
				const ImU32 fill_color = AxisColor(plane_axis, active);
				const ImU32 fill_alpha = active ? IM_COL32(255, 255, 255, 100) : IM_COL32(255, 255, 255, 60);
				const ImU32 combined = (fill_color & 0x00FFFFFF) | (fill_alpha & 0xFF000000);

				draw_list->AddQuadFilled(
					ImVec2(viewport_pos.x + corners[0].x, viewport_pos.y + corners[0].y),
					ImVec2(viewport_pos.x + corners[1].x, viewport_pos.y + corners[1].y),
					ImVec2(viewport_pos.x + corners[2].x, viewport_pos.y + corners[2].y),
					ImVec2(viewport_pos.x + corners[3].x, viewport_pos.y + corners[3].y),
					combined);
			};

			draw_plane_handle(x_handle, y_handle, ViewportPanelState::GizmoAxis::XY);
			draw_plane_handle(x_handle, z_handle, ViewportPanelState::GizmoAxis::XZ);
			draw_plane_handle(y_handle, z_handle, ViewportPanelState::GizmoAxis::YZ);
		}

		// Draw center box for uniform scale
		if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Scale)
		{
			const float box_size = state.handles[0].screen_len * kUniformScaleBoxScale;
			if (box_size > 2.0f)
			{
				const bool active = panel.gizmo_axis == ViewportPanelState::GizmoAxis::All && panel.gizmo_dragging;
				const ImU32 color = AxisColor(ViewportPanelState::GizmoAxis::All, active);
				const ImVec2 center(viewport_pos.x + state.origin_screen.x, viewport_pos.y + state.origin_screen.y);
				draw_list->AddRectFilled(
					ImVec2(center.x - box_size, center.y - box_size),
					ImVec2(center.x + box_size, center.y + box_size),
					color);
			}
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

bool BuildGizmoDrawStateMultiSelect(
	const ViewportPanelState& panel,
	const ScenePanelState& scene_panel,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	const Diligent::float3& cam_pos,
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_size,
	GizmoDrawState& out_state)
{
	if (!HasSelection(scene_panel))
	{
		return false;
	}

	// Compute selection center
	const std::array<float, 3> center = ComputeSelectionCenter(scene_panel, props);

	// Get primary selection for rotation reference (when using local space)
	const int primary_id = scene_panel.primary_selection;
	const bool has_valid_primary = primary_id >= 0 &&
		static_cast<size_t>(primary_id) < props.size() &&
		static_cast<size_t>(primary_id) < nodes.size() &&
		!nodes[primary_id].deleted && !nodes[primary_id].is_folder;

	// Create a temporary props struct for building gizmo state
	NodeProperties temp_props;
	temp_props.visible = true;
	temp_props.position[0] = center[0];
	temp_props.position[1] = center[1];
	temp_props.position[2] = center[2];

	// Use primary selection's rotation for local space gizmo orientation
	if (has_valid_primary)
	{
		temp_props.rotation[0] = props[primary_id].rotation[0];
		temp_props.rotation[1] = props[primary_id].rotation[1];
		temp_props.rotation[2] = props[primary_id].rotation[2];
	}

	// Build the basic gizmo state
	if (!BuildGizmoDrawState(panel, temp_props, cam_pos, view_proj, viewport_size, out_state))
	{
		return false;
	}

	// Mark as multi-select and collect affected node IDs
	out_state.is_multi_select = SelectionCount(scene_panel) > 1;
	out_state.affected_node_ids.clear();

	const size_t count = std::min(nodes.size(), props.size());
	for (int id : scene_panel.selected_ids)
	{
		if (id >= 0 && static_cast<size_t>(id) < count &&
			!nodes[id].deleted && !nodes[id].is_folder && !props[id].frozen)
		{
			out_state.affected_node_ids.push_back(id);
		}
	}

	return !out_state.affected_node_ids.empty();
}

bool UpdateGizmoInteractionMultiSelect(
	ViewportPanelState& panel,
	GizmoDrawState& state,
	const ScenePanelState& scene_panel,
	const std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	const ImVec2& mouse_local,
	bool mouse_down,
	bool mouse_clicked,
	UndoStack* undo_stack)
{
	if (!state.origin_visible || state.affected_node_ids.empty())
	{
		if (panel.gizmo_dragging && !mouse_down)
		{
			panel.gizmo_dragging = false;
			panel.gizmo_axis = ViewportPanelState::GizmoAxis::None;
		}
		return false;
	}

	const bool shift_held = ImGui::GetIO().KeyShift;
	const bool ctrl_held = ImGui::GetIO().KeyCtrl;
	const float fine_adjust_factor = shift_held ? 0.1f : 1.0f;

	// Determine effective snap state (Ctrl toggles snap)
	const bool snap_translate_eff = panel.snap_translate != ctrl_held;
	const bool snap_rotate_eff = panel.snap_rotate != ctrl_held;
	const bool snap_scale_eff = panel.snap_scale != ctrl_held;

	if (panel.gizmo_dragging)
	{
		// Check for Escape to cancel
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			// Restore all transforms from start state
			for (size_t i = 0; i < state.affected_node_ids.size(); ++i)
			{
				const int id = state.affected_node_ids[i];
				if (id < 0 || static_cast<size_t>(id) >= props.size())
				{
					continue;
				}
				if (i < state.start_transforms.size())
				{
					const TransformState& start = state.start_transforms[i];
					props[id].position[0] = start.position[0];
					props[id].position[1] = start.position[1];
					props[id].position[2] = start.position[2];
					props[id].rotation[0] = start.rotation[0];
					props[id].rotation[1] = start.rotation[1];
					props[id].rotation[2] = start.rotation[2];
					props[id].scale[0] = start.scale[0];
					props[id].scale[1] = start.scale[1];
					props[id].scale[2] = start.scale[2];
				}
				if (i < state.start_vertices.size() && !state.start_vertices[i].empty())
				{
					props[id].brush_vertices = state.start_vertices[i];
				}
			}
			panel.gizmo_dragging = false;
			panel.gizmo_axis = ViewportPanelState::GizmoAxis::None;
			return false;
		}

		if (!mouse_down)
		{
			// Drag ended - push undo action
			if (undo_stack != nullptr)
			{
				std::vector<TransformChange> changes;
				for (size_t i = 0; i < state.affected_node_ids.size(); ++i)
				{
					const int id = state.affected_node_ids[i];
					if (id < 0 || static_cast<size_t>(id) >= props.size())
					{
						continue;
					}
					TransformChange change;
					change.node_id = id;
					if (i < state.start_transforms.size())
					{
						change.before = state.start_transforms[i];
					}
					change.after.position[0] = props[id].position[0];
					change.after.position[1] = props[id].position[1];
					change.after.position[2] = props[id].position[2];
					change.after.rotation[0] = props[id].rotation[0];
					change.after.rotation[1] = props[id].rotation[1];
					change.after.rotation[2] = props[id].rotation[2];
					change.after.scale[0] = props[id].scale[0];
					change.after.scale[1] = props[id].scale[1];
					change.after.scale[2] = props[id].scale[2];
					if (i < state.start_vertices.size() && !state.start_vertices[i].empty())
					{
						change.before_vertices = state.start_vertices[i];
						change.after_vertices = props[id].brush_vertices;
					}
					changes.push_back(std::move(change));
				}
				if (!changes.empty())
				{
					undo_stack->PushTransform(UndoTarget::Scene, std::move(changes));
				}
			}
			panel.gizmo_dragging = false;
			panel.gizmo_axis = ViewportPanelState::GizmoAxis::None;
			return false;
		}

		// Continue dragging - apply delta to all selected nodes
		const ImVec2 delta(
			mouse_local.x - panel.gizmo_drag_start_mouse.x,
			mouse_local.y - panel.gizmo_drag_start_mouse.y);

		// Handle plane and uniform scale axes
		const bool is_plane_axis = panel.gizmo_axis == ViewportPanelState::GizmoAxis::XY ||
			panel.gizmo_axis == ViewportPanelState::GizmoAxis::XZ ||
			panel.gizmo_axis == ViewportPanelState::GizmoAxis::YZ;
		const bool is_uniform_scale = panel.gizmo_axis == ViewportPanelState::GizmoAxis::All;

		if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Translate)
		{
			Diligent::float3 world_delta(0.0f, 0.0f, 0.0f);

			if (is_plane_axis)
			{
				// Get the two handles for the plane
				int idx1 = 0, idx2 = 1;
				if (panel.gizmo_axis == ViewportPanelState::GizmoAxis::XZ) { idx1 = 0; idx2 = 2; }
				else if (panel.gizmo_axis == ViewportPanelState::GizmoAxis::YZ) { idx1 = 1; idx2 = 2; }

				const GizmoHandle& h1 = state.handles[idx1];
				const GizmoHandle& h2 = state.handles[idx2];
				if (h1.screen_len <= 0.0f || h2.screen_len <= 0.0f)
				{
					return false;
				}

				const ImVec2 dir1((h1.end.x - h1.start.x) / h1.screen_len, (h1.end.y - h1.start.y) / h1.screen_len);
				const ImVec2 dir2((h2.end.x - h2.start.x) / h2.screen_len, (h2.end.y - h2.start.y) / h2.screen_len);
				const float world_per_pixel1 = h1.world_len / h1.screen_len;
				const float world_per_pixel2 = h2.world_len / h2.screen_len;

				float scalar1 = (delta.x * dir1.x + delta.y * dir1.y) * fine_adjust_factor;
				float scalar2 = (delta.x * dir2.x + delta.y * dir2.y) * fine_adjust_factor;
				float delta1 = scalar1 * world_per_pixel1;
				float delta2 = scalar2 * world_per_pixel2;

				if (snap_translate_eff)
				{
					if (panel.snap_translate_axis[idx1]) delta1 = SnapValue(delta1, panel.snap_translate_step);
					if (panel.snap_translate_axis[idx2]) delta2 = SnapValue(delta2, panel.snap_translate_step);
				}

				world_delta.x = h1.dir.x * delta1 + h2.dir.x * delta2;
				world_delta.y = h1.dir.y * delta1 + h2.dir.y * delta2;
				world_delta.z = h1.dir.z * delta1 + h2.dir.z * delta2;
			}
			else
			{
				// Single axis
				const GizmoHandle* active = FindHandle(state, panel.gizmo_axis);
				if (active == nullptr || active->screen_len <= 0.0f)
				{
					return false;
				}

				const ImVec2 axis_dir(
					(active->end.x - active->start.x) / active->screen_len,
					(active->end.y - active->start.y) / active->screen_len);
				float scalar = (delta.x * axis_dir.x + delta.y * axis_dir.y) * fine_adjust_factor;
				const float world_per_pixel = active->world_len / active->screen_len;
				const int axis_index = active->axis_index;

				float world_scalar = scalar * world_per_pixel;
				if (snap_translate_eff && panel.snap_translate_axis[axis_index])
				{
					world_scalar = SnapValue(world_scalar, panel.snap_translate_step);
				}

				world_delta.x = active->dir.x * world_scalar;
				world_delta.y = active->dir.y * world_scalar;
				world_delta.z = active->dir.z * world_scalar;
			}

			// Apply translation delta to all affected nodes
			for (size_t i = 0; i < state.affected_node_ids.size(); ++i)
			{
				const int id = state.affected_node_ids[i];
				if (id < 0 || static_cast<size_t>(id) >= props.size())
				{
					continue;
				}
				if (i >= state.start_transforms.size())
				{
					continue;
				}
				const TransformState& start = state.start_transforms[i];
				props[id].position[0] = start.position[0] + world_delta.x;
				props[id].position[1] = start.position[1] + world_delta.y;
				props[id].position[2] = start.position[2] + world_delta.z;

				// Also move brush vertices if present
				if (i < state.start_vertices.size() && !state.start_vertices[i].empty())
				{
					props[id].brush_vertices = state.start_vertices[i];
					for (size_t v = 0; v < props[id].brush_vertices.size(); v += 3)
					{
						props[id].brush_vertices[v] += world_delta.x;
						props[id].brush_vertices[v + 1] += world_delta.y;
						props[id].brush_vertices[v + 2] += world_delta.z;
					}
				}
			}
		}
		else if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Rotate)
		{
			const GizmoHandle* active = FindHandle(state, panel.gizmo_axis);
			if (active == nullptr || active->screen_len <= 0.0f)
			{
				return false;
			}
			const int axis_index = active->axis_index;

			const float angle_now = std::atan2(
				mouse_local.y - state.origin_screen.y,
				mouse_local.x - state.origin_screen.x);
			float delta_rad = (angle_now - panel.gizmo_drag_start_angle) * fine_adjust_factor;
			float delta_deg = delta_rad * 180.0f / kPi;
			if (snap_rotate_eff && panel.snap_rotate_axis[axis_index])
			{
				delta_deg = SnapValue(delta_deg, panel.snap_rotate_step);
			}
			delta_rad = delta_deg * kPi / 180.0f;

			// Build rotation matrix for the delta
			Diligent::float3x3 delta_rot;
			if (axis_index == 0)
			{
				delta_rot = Diligent::float3x3::RotationX(delta_rad);
			}
			else if (axis_index == 1)
			{
				delta_rot = Diligent::float3x3::RotationY(delta_rad);
			}
			else
			{
				delta_rot = Diligent::float3x3::RotationZ(delta_rad);
			}

			// Apply rotation to all affected nodes around the gizmo origin
			for (size_t i = 0; i < state.affected_node_ids.size(); ++i)
			{
				const int id = state.affected_node_ids[i];
				if (id < 0 || static_cast<size_t>(id) >= props.size())
				{
					continue;
				}
				if (i >= state.start_transforms.size())
				{
					continue;
				}
				const TransformState& start = state.start_transforms[i];

				// Rotate position around gizmo origin
				Diligent::float3 offset(
					start.position[0] - state.origin.x,
					start.position[1] - state.origin.y,
					start.position[2] - state.origin.z);
				Diligent::float3 rotated = delta_rot * offset;
				props[id].position[0] = state.origin.x + rotated.x;
				props[id].position[1] = state.origin.y + rotated.y;
				props[id].position[2] = state.origin.z + rotated.z;

				// Compose rotation with node's existing rotation
				const Diligent::float3x3 base = EulerToMatrixXYZ(start.rotation);
				Diligent::float3x3 combined;
				if (panel.rotation_space == ViewportPanelState::GizmoSpace::World)
				{
					combined = Diligent::float3x3::Mul(delta_rot, base);
				}
				else
				{
					combined = Diligent::float3x3::Mul(base, delta_rot);
				}
				MatrixToEulerXYZ(combined, props[id].rotation);

				// Rotate brush vertices if present
				if (i < state.start_vertices.size() && !state.start_vertices[i].empty())
				{
					props[id].brush_vertices = state.start_vertices[i];
					for (size_t v = 0; v < props[id].brush_vertices.size(); v += 3)
					{
						Diligent::float3 vert_offset(
							props[id].brush_vertices[v] - state.origin.x,
							props[id].brush_vertices[v + 1] - state.origin.y,
							props[id].brush_vertices[v + 2] - state.origin.z);
						Diligent::float3 vert_rotated = delta_rot * vert_offset;
						props[id].brush_vertices[v] = state.origin.x + vert_rotated.x;
						props[id].brush_vertices[v + 1] = state.origin.y + vert_rotated.y;
						props[id].brush_vertices[v + 2] = state.origin.z + vert_rotated.z;
					}
				}
			}
		}
		else // Scale
		{
			float delta_scale = 0.0f;
			if (is_uniform_scale)
			{
				// Use average of X and Y mouse delta for uniform scale
				const float avg_delta = (delta.x + delta.y) * 0.5f * fine_adjust_factor;
				delta_scale = avg_delta * 0.01f;
				if (snap_scale_eff)
				{
					delta_scale = SnapValue(delta_scale, panel.snap_scale_step);
				}
			}
			else
			{
				// Single axis scale
				const GizmoHandle* active = FindHandle(state, panel.gizmo_axis);
				if (active == nullptr || active->screen_len <= 0.0f)
				{
					return false;
				}

				const ImVec2 axis_dir(
					(active->end.x - active->start.x) / active->screen_len,
					(active->end.y - active->start.y) / active->screen_len);
				const float scalar = (delta.x * axis_dir.x + delta.y * axis_dir.y) * fine_adjust_factor;
				delta_scale = scalar * 0.01f;
				if (snap_scale_eff && panel.snap_scale_axis[active->axis_index])
				{
					delta_scale = SnapValue(delta_scale, panel.snap_scale_step);
				}
			}

			for (size_t i = 0; i < state.affected_node_ids.size(); ++i)
			{
				const int id = state.affected_node_ids[i];
				if (id < 0 || static_cast<size_t>(id) >= props.size())
				{
					continue;
				}
				if (i >= state.start_transforms.size())
				{
					continue;
				}
				const TransformState& start = state.start_transforms[i];
				props[id].scale[0] = start.scale[0];
				props[id].scale[1] = start.scale[1];
				props[id].scale[2] = start.scale[2];

				if (is_uniform_scale)
				{
					// Apply to all axes
					props[id].scale[0] = std::max(0.01f, props[id].scale[0] + delta_scale);
					props[id].scale[1] = std::max(0.01f, props[id].scale[1] + delta_scale);
					props[id].scale[2] = std::max(0.01f, props[id].scale[2] + delta_scale);
				}
				else
				{
					// Single axis
					const GizmoHandle* active = FindHandle(state, panel.gizmo_axis);
					if (active != nullptr)
					{
						props[id].scale[active->axis_index] = std::max(0.01f, props[id].scale[active->axis_index] + delta_scale);
					}
				}
			}
		}

		return false;
	}

	// Not dragging - check for click to start drag
	if (!mouse_clicked)
	{
		return false;
	}

	// Hit test gizmo handles
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
		// Test axis handles
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

		// Test plane handles for translate mode
		if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Translate)
		{
			auto test_plane_handle = [&](const GizmoHandle& h1, const GizmoHandle& h2, ViewportPanelState::GizmoAxis plane_axis) {
				if (h1.screen_len <= 0.0f || h2.screen_len <= 0.0f)
				{
					return;
				}
				const float offset1 = h1.screen_len * kPlaneHandleScale;
				const float offset2 = h2.screen_len * kPlaneHandleScale;
				const ImVec2 dir1((h1.end.x - h1.start.x) / h1.screen_len, (h1.end.y - h1.start.y) / h1.screen_len);
				const ImVec2 dir2((h2.end.x - h2.start.x) / h2.screen_len, (h2.end.y - h2.start.y) / h2.screen_len);

				// Check if point is inside the quad
				const ImVec2 local_mouse(mouse_local.x - state.origin_screen.x, mouse_local.y - state.origin_screen.y);
				const float u = (local_mouse.x * dir1.x + local_mouse.y * dir1.y);
				const float v = (local_mouse.x * dir2.x + local_mouse.y * dir2.y);
				if (u >= 0.0f && u <= offset1 && v >= 0.0f && v <= offset2)
				{
					hit_axis = plane_axis;
					best_dist = 0.0f;
				}
			};

			test_plane_handle(state.handles[0], state.handles[1], ViewportPanelState::GizmoAxis::XY);
			test_plane_handle(state.handles[0], state.handles[2], ViewportPanelState::GizmoAxis::XZ);
			test_plane_handle(state.handles[1], state.handles[2], ViewportPanelState::GizmoAxis::YZ);
		}

		// Test center box for uniform scale
		if (panel.gizmo_mode == ViewportPanelState::GizmoMode::Scale)
		{
			const float box_size = state.handles[0].screen_len * kUniformScaleBoxScale;
			if (box_size > 2.0f)
			{
				const float dx = mouse_local.x - state.origin_screen.x;
				const float dy = mouse_local.y - state.origin_screen.y;
				if (std::fabs(dx) <= box_size && std::fabs(dy) <= box_size)
				{
					hit_axis = ViewportPanelState::GizmoAxis::All;
					best_dist = 0.0f;
				}
			}
		}
	}

	if (hit_axis == ViewportPanelState::GizmoAxis::None)
	{
		return false;
	}

	// Start dragging - store start transforms for all affected nodes
	panel.gizmo_dragging = true;
	panel.gizmo_axis = hit_axis;
	panel.gizmo_drag_start_mouse = mouse_local;
	panel.gizmo_drag_start_angle = std::atan2(
		mouse_local.y - state.origin_screen.y,
		mouse_local.x - state.origin_screen.x);

	state.start_transforms.clear();
	state.start_vertices.clear();
	state.start_transforms.reserve(state.affected_node_ids.size());
	state.start_vertices.reserve(state.affected_node_ids.size());

	for (int id : state.affected_node_ids)
	{
		if (id < 0 || static_cast<size_t>(id) >= props.size())
		{
			state.start_transforms.emplace_back();
			state.start_vertices.emplace_back();
			continue;
		}
		TransformState start;
		start.position[0] = props[id].position[0];
		start.position[1] = props[id].position[1];
		start.position[2] = props[id].position[2];
		start.rotation[0] = props[id].rotation[0];
		start.rotation[1] = props[id].rotation[1];
		start.rotation[2] = props[id].rotation[2];
		start.scale[0] = props[id].scale[0];
		start.scale[1] = props[id].scale[1];
		start.scale[2] = props[id].scale[2];
		state.start_transforms.push_back(start);
		state.start_vertices.push_back(props[id].brush_vertices);
	}

	// Also store in panel for backwards compatibility
	if (!state.affected_node_ids.empty())
	{
		const int first_id = state.affected_node_ids[0];
		if (first_id >= 0 && static_cast<size_t>(first_id) < props.size())
		{
			panel.gizmo_start_pos[0] = props[first_id].position[0];
			panel.gizmo_start_pos[1] = props[first_id].position[1];
			panel.gizmo_start_pos[2] = props[first_id].position[2];
			panel.gizmo_start_rot[0] = props[first_id].rotation[0];
			panel.gizmo_start_rot[1] = props[first_id].rotation[1];
			panel.gizmo_start_rot[2] = props[first_id].rotation[2];
			panel.gizmo_start_scale[0] = props[first_id].scale[0];
			panel.gizmo_start_scale[1] = props[first_id].scale[1];
			panel.gizmo_start_scale[2] = props[first_id].scale[2];
		}
	}

	return true;
}
