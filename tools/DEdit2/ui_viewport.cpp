#include "ui_viewport.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
float Clamp(float value, float min_value, float max_value)
{
	return std::max(min_value, std::min(value, max_value));
}

void ComputeForward(float yaw, float pitch, float out[3])
{
	const float cp = std::cos(pitch);
	const float sp = std::sin(pitch);
	const float cy = std::cos(yaw);
	const float sy = std::sin(yaw);

	out[0] = sy * cp;
	out[1] = sp;
	out[2] = cy * cp;
}

ImVec2 Add(const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}
} // namespace

void ResetViewportPanel(ViewportPanelState& state, const ImVec2& size)
{
	(void)size;
	state.orbit_yaw = 0.75f;
	state.orbit_pitch = -0.45f;
	state.orbit_distance = 1200.0f;
	state.target[0] = 0.0f;
	state.target[1] = 0.0f;
	state.target[2] = 0.0f;
	SyncFlyFromOrbit(state);
	state.initialized = true;
}

void SyncFlyFromOrbit(ViewportPanelState& state)
{
	float forward[3] = {0.0f, 0.0f, 1.0f};
	ComputeForward(state.orbit_yaw, state.orbit_pitch, forward);

	state.fly_pos[0] = state.target[0] - forward[0] * state.orbit_distance;
	state.fly_pos[1] = state.target[1] - forward[1] * state.orbit_distance;
	state.fly_pos[2] = state.target[2] - forward[2] * state.orbit_distance;
}

void SyncOrbitFromFly(ViewportPanelState& state)
{
	const float distance = std::max(1.0f, state.orbit_distance);
	float forward[3] = {0.0f, 0.0f, 1.0f};
	ComputeForward(state.orbit_yaw, state.orbit_pitch, forward);

	state.target[0] = state.fly_pos[0] + forward[0] * distance;
	state.target[1] = state.fly_pos[1] + forward[1] * distance;
	state.target[2] = state.fly_pos[2] + forward[2] * distance;
	state.orbit_distance = distance;
}

void UpdateViewportControls(ViewportPanelState& state, const ImVec2& origin, const ImVec2& size, bool hovered)
{
	(void)origin;
	if (!state.initialized && size.x > 0.0f && size.y > 0.0f)
	{
		ResetViewportPanel(state, size);
	}

	if (!hovered)
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	if (state.fly_mode)
	{
		float forward[3] = {0.0f, 0.0f, 1.0f};
		ComputeForward(state.orbit_yaw, state.orbit_pitch, forward);

		if (io.MouseWheel != 0.0f)
		{
			const float dolly = io.MouseWheel * state.fly_speed * 0.1f;
			state.fly_pos[0] += forward[0] * dolly;
			state.fly_pos[1] += forward[1] * dolly;
			state.fly_pos[2] += forward[2] * dolly;
		}

		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
		{
			const float look_speed = 0.005f;
			state.orbit_yaw += io.MouseDelta.x * look_speed;
			state.orbit_pitch += io.MouseDelta.y * look_speed;
			state.orbit_pitch = Clamp(state.orbit_pitch, -1.4f, 1.4f);
			ComputeForward(state.orbit_yaw, state.orbit_pitch, forward);
		}

		const float right[3] = {std::cos(state.orbit_yaw), 0.0f, -std::sin(state.orbit_yaw)};
		const float up[3] = {0.0f, 1.0f, 0.0f};

		float move[3] = {0.0f, 0.0f, 0.0f};
		if (!io.WantCaptureKeyboard)
		{
			if (ImGui::IsKeyDown(ImGuiKey_W))
			{
				move[0] += forward[0];
				move[1] += forward[1];
				move[2] += forward[2];
			}
			if (ImGui::IsKeyDown(ImGuiKey_S))
			{
				move[0] -= forward[0];
				move[1] -= forward[1];
				move[2] -= forward[2];
			}
			if (ImGui::IsKeyDown(ImGuiKey_D))
			{
				move[0] += right[0];
				move[1] += right[1];
				move[2] += right[2];
			}
			if (ImGui::IsKeyDown(ImGuiKey_A))
			{
				move[0] -= right[0];
				move[1] -= right[1];
				move[2] -= right[2];
			}
			if (ImGui::IsKeyDown(ImGuiKey_E))
			{
				move[0] += up[0];
				move[1] += up[1];
				move[2] += up[2];
			}
			if (ImGui::IsKeyDown(ImGuiKey_Q))
			{
				move[0] -= up[0];
				move[1] -= up[1];
				move[2] -= up[2];
			}
		}

		const float length = std::sqrt(move[0] * move[0] + move[1] * move[1] + move[2] * move[2]);
		if (length > 0.0f)
		{
			const float speed = state.fly_speed * (io.KeyShift ? 3.0f : (io.KeyCtrl ? 0.25f : 1.0f));
			const float scale = (speed * io.DeltaTime) / length;
			state.fly_pos[0] += move[0] * scale;
			state.fly_pos[1] += move[1] * scale;
			state.fly_pos[2] += move[2] * scale;
		}
	}
	else
	{
		if (io.MouseWheel != 0.0f)
		{
			const float zoom_factor = std::pow(1.1f, -io.MouseWheel);
			state.orbit_distance = Clamp(state.orbit_distance * zoom_factor, 50.0f, 20000.0f);
		}

		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
		{
			const float orbit_speed = 0.005f;
			state.orbit_yaw += io.MouseDelta.x * orbit_speed;
			state.orbit_pitch += io.MouseDelta.y * orbit_speed;
			state.orbit_pitch = Clamp(state.orbit_pitch, -1.4f, 1.4f);
		}

		if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
		{
			const float cp = std::cos(state.orbit_pitch);
			const float sp = std::sin(state.orbit_pitch);
			const float cy = std::cos(state.orbit_yaw);
			const float sy = std::sin(state.orbit_yaw);

			const float right[3] = {cy, 0.0f, -sy};
			const float up[3] = {sy * sp, cp, cy * sp};
			const float pan_speed = 0.0025f * state.orbit_distance;

			state.target[0] -= right[0] * io.MouseDelta.x * pan_speed;
			state.target[1] -= right[1] * io.MouseDelta.x * pan_speed;
			state.target[2] -= right[2] * io.MouseDelta.x * pan_speed;

			state.target[0] += up[0] * io.MouseDelta.y * pan_speed;
			state.target[1] += up[1] * io.MouseDelta.y * pan_speed;
			state.target[2] += up[2] * io.MouseDelta.y * pan_speed;
		}
	}
}

void DrawViewportOverlay(const ViewportPanelState& state, ImDrawList* draw_list, const ImVec2& origin, const ImVec2& size)
{
	if (!draw_list)
	{
		return;
	}

	const ImVec2 max = Add(origin, size);
	draw_list->PushClipRect(origin, max, true);

	char status[128];
	if (state.fly_mode)
	{
		std::snprintf(
			status,
			sizeof(status),
			"Fly %.0f | Pos %.0f %.0f %.0f | RMB look, WASD move",
			state.fly_speed,
			state.fly_pos[0],
			state.fly_pos[1],
			state.fly_pos[2]);
	}
	else
	{
		std::snprintf(
			status,
			sizeof(status),
			"Orbit %.0f | Target %.0f %.0f %.0f | RMB orbit, MMB pan",
			state.orbit_distance,
			state.target[0],
			state.target[1],
			state.target[2]);
	}
	draw_list->AddText(ImVec2(origin.x + 8.0f, origin.y + 8.0f), IM_COL32(220, 220, 220, 200), status);

	draw_list->PopClipRect();
}
