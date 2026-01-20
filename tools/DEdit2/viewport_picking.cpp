#include "viewport_picking.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <string>

namespace
{
std::string LowerCopy(std::string value)
{
	for (char& ch : value)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return value;
}

bool TryParseVec3(const std::string& value, float out[3])
{
	return std::sscanf(value.c_str(), "%f %f %f", &out[0], &out[1], &out[2]) == 3;
}

bool TryGetRawPropertyVec3(const NodeProperties& props, const char* key, float out[3])
{
	for (const auto& entry : props.raw_properties)
	{
		if (entry.first == key)
		{
			return TryParseVec3(entry.second, out);
		}
	}
	return false;
}

Diligent::float3 Cross(const Diligent::float3& a, const Diligent::float3& b)
{
	return Diligent::float3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

Diligent::float3 Add(const Diligent::float3& a, const Diligent::float3& b)
{
	return Diligent::float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Diligent::float3 Sub(const Diligent::float3& a, const Diligent::float3& b)
{
	return Diligent::float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Diligent::float3 Scale(const Diligent::float3& v, float s)
{
	return Diligent::float3(v.x * s, v.y * s, v.z * s);
}

float Dot(const Diligent::float3& a, const Diligent::float3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
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

bool IsModelResource(const std::string& resource)
{
	if (resource.empty())
	{
		return false;
	}
	const size_t dot = resource.find_last_of('.');
	if (dot == std::string::npos)
	{
		return false;
	}
	std::string ext = LowerCopy(resource.substr(dot + 1));
	return ext == "ltb" || ext == "ltc" || ext == "abc" || ext == "fbx" || ext == "gltf" || ext == "glb";
}

bool TryGetModelBounds(const NodeProperties& props, float out_min[3], float out_max[3])
{
	const bool type_hint =
		props.type == "Model" ||
		props.type == "WorldModel" ||
		props.type == "WorldModelInstance" ||
		props.type == "WorldModelStatic" ||
		props.class_name.find("Model") != std::string::npos;
	if (!IsModelResource(props.resource) && !type_hint)
	{
		return false;
	}

	const float half_x = std::fabs(props.size[0]) * 0.5f * std::max(0.001f, props.scale[0]);
	const float half_y = std::fabs(props.size[1]) * 0.5f * std::max(0.001f, props.scale[1]);
	const float half_z = std::fabs(props.size[2]) * 0.5f * std::max(0.001f, props.scale[2]);
	if (half_x <= 0.0f && half_y <= 0.0f && half_z <= 0.0f)
	{
		return false;
	}

	out_min[0] = props.position[0] - half_x;
	out_min[1] = props.position[1] - half_y;
	out_min[2] = props.position[2] - half_z;
	out_max[0] = props.position[0] + half_x;
	out_max[1] = props.position[1] + half_y;
	out_max[2] = props.position[2] + half_z;
	return true;
}

bool TryGetNodeBoundsInternal(const NodeProperties& props, float out_min[3], float out_max[3])
{
	if (TryGetRawPropertyVec3(props, "bounds_min", out_min) &&
		TryGetRawPropertyVec3(props, "bounds_max", out_max))
	{
		return true;
	}

	if (TryGetModelBounds(props, out_min, out_max))
	{
		return true;
	}

	if (props.range > 0.0f)
	{
		const float r = props.range;
		out_min[0] = props.position[0] - r;
		out_min[1] = props.position[1] - r;
		out_min[2] = props.position[2] - r;
		out_max[0] = props.position[0] + r;
		out_max[1] = props.position[1] + r;
		out_max[2] = props.position[2] + r;
		return true;
	}

	const float half_x = std::fabs(props.size[0]) * 0.5f * std::max(0.001f, props.scale[0]);
	const float half_y = std::fabs(props.size[1]) * 0.5f * std::max(0.001f, props.scale[1]);
	const float half_z = std::fabs(props.size[2]) * 0.5f * std::max(0.001f, props.scale[2]);
	if (half_x <= 0.0f && half_y <= 0.0f && half_z <= 0.0f)
	{
		return false;
	}
	out_min[0] = props.position[0] - half_x;
	out_min[1] = props.position[1] - half_y;
	out_min[2] = props.position[2] - half_z;
	out_max[0] = props.position[0] + half_x;
	out_max[1] = props.position[1] + half_y;
	out_max[2] = props.position[2] + half_z;
	return true;
}

bool RayIntersectsAABB(
	const PickRay& ray,
	const float bounds_min[3],
	const float bounds_max[3],
	float& out_t)
{
	const float eps = 1e-6f;
	float t_min = -1.0e30f;
	float t_max = 1.0e30f;

	const float origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
	const float dir[3] = {ray.dir.x, ray.dir.y, ray.dir.z};

	for (int axis = 0; axis < 3; ++axis)
	{
		if (std::fabs(dir[axis]) < eps)
		{
			if (origin[axis] < bounds_min[axis] || origin[axis] > bounds_max[axis])
			{
				return false;
			}
			continue;
		}

		const float inv_dir = 1.0f / dir[axis];
		float t1 = (bounds_min[axis] - origin[axis]) * inv_dir;
		float t2 = (bounds_max[axis] - origin[axis]) * inv_dir;
		if (t1 > t2)
		{
			std::swap(t1, t2);
		}
		t_min = std::max(t_min, t1);
		t_max = std::min(t_max, t2);
		if (t_max < t_min)
		{
			return false;
		}
	}

	if (t_max < 0.0f)
	{
		return false;
	}
	out_t = t_min >= 0.0f ? t_min : t_max;
	return true;
}

bool RayIntersectsTriangle(
	const PickRay& ray,
	const Diligent::float3& v0,
	const Diligent::float3& v1,
	const Diligent::float3& v2,
	float& out_t)
{
	const float eps = 1e-6f;
	const Diligent::float3 e1 = Sub(v1, v0);
	const Diligent::float3 e2 = Sub(v2, v0);
	const Diligent::float3 p = Cross(ray.dir, e2);
	const float det = Dot(e1, p);
	if (std::fabs(det) < eps)
	{
		return false;
	}
	const float inv_det = 1.0f / det;
	const Diligent::float3 tvec = Sub(ray.origin, v0);
	const float u = Dot(tvec, p) * inv_det;
	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}
	const Diligent::float3 q = Cross(tvec, e1);
	const float v = Dot(ray.dir, q) * inv_det;
	if (v < 0.0f || (u + v) > 1.0f)
	{
		return false;
	}
	const float t = Dot(e2, q) * inv_det;
	if (t < 0.0f)
	{
		return false;
	}
	out_t = t;
	return true;
}
} // namespace

bool TryGetNodeBounds(const NodeProperties& props, float out_min[3], float out_max[3])
{
	return TryGetNodeBoundsInternal(props, out_min, out_max);
}

void ComputeCameraBasis(
	const ViewportPanelState& state,
	Diligent::float3& out_pos,
	Diligent::float3& out_forward,
	Diligent::float3& out_right,
	Diligent::float3& out_up)
{
	const float cp = std::cos(state.orbit_pitch);
	const float sp = std::sin(state.orbit_pitch);
	const float cy = std::cos(state.orbit_yaw);
	const float sy = std::sin(state.orbit_yaw);

	out_forward = Normalize(Diligent::float3(sy * cp, sp, cy * cp));
	const Diligent::float3 world_up(0.0f, 1.0f, 0.0f);
	out_right = Normalize(Cross(world_up, out_forward));
	out_up = Cross(out_forward, out_right);

	if (state.fly_mode)
	{
		out_pos = Diligent::float3(state.fly_pos[0], state.fly_pos[1], state.fly_pos[2]);
	}
	else
	{
		const Diligent::float3 target(state.target[0], state.target[1], state.target[2]);
		out_pos = Diligent::float3(
			target.x - out_forward.x * state.orbit_distance,
			target.y - out_forward.y * state.orbit_distance,
			target.z - out_forward.z * state.orbit_distance);
	}
}

PickRay BuildPickRay(
	const ViewportPanelState& panel,
	const ImVec2& viewport_size,
	const ImVec2& mouse_local)
{
	Diligent::float3 cam_pos;
	Diligent::float3 cam_forward;
	Diligent::float3 cam_right;
	Diligent::float3 cam_up;
	ComputeCameraBasis(panel, cam_pos, cam_forward, cam_right, cam_up);

	const float aspect = viewport_size.y > 0.0f ? (viewport_size.x / viewport_size.y) : 1.0f;
	const float fov = Diligent::PI_F / 4.0f;
	const float tan_half_fov = std::tan(fov * 0.5f);
	const float ndc_x = (mouse_local.x / std::max(1.0f, viewport_size.x)) * 2.0f - 1.0f;
	const float ndc_y = 1.0f - (mouse_local.y / std::max(1.0f, viewport_size.y)) * 2.0f;

	const Diligent::float3 ray_dir = Normalize(Add(
		Add(cam_forward, Scale(cam_right, ndc_x * tan_half_fov * aspect)),
		Scale(cam_up, ndc_y * tan_half_fov)));

	return PickRay{cam_pos, ray_dir};
}

bool TryGetNodePickPosition(const NodeProperties& props, float out[3])
{
	if (TryGetRawPropertyVec3(props, "centroid", out))
	{
		return true;
	}
	float minv[3];
	float maxv[3];
	if (TryGetRawPropertyVec3(props, "bounds_min", minv) && TryGetRawPropertyVec3(props, "bounds_max", maxv))
	{
		out[0] = (minv[0] + maxv[0]) * 0.5f;
		out[1] = (minv[1] + maxv[1]) * 0.5f;
		out[2] = (minv[2] + maxv[2]) * 0.5f;
		return true;
	}
	if (TryGetRawPropertyVec3(props, "pos", out) ||
		TryGetRawPropertyVec3(props, "Pos", out) ||
		TryGetRawPropertyVec3(props, "position", out) ||
		TryGetRawPropertyVec3(props, "Position", out))
	{
		return true;
	}

	out[0] = props.position[0];
	out[1] = props.position[1];
	out[2] = props.position[2];
	return true;
}

bool RaycastBrush(const NodeProperties& props, const PickRay& ray, float& out_t)
{
	if (props.brush_vertices.empty() || props.brush_indices.empty())
	{
		return false;
	}

	float bounds_min[3];
	float bounds_max[3];
	if (TryGetRawPropertyVec3(props, "bounds_min", bounds_min) &&
		TryGetRawPropertyVec3(props, "bounds_max", bounds_max))
	{
		float box_t = 0.0f;
		if (!RayIntersectsAABB(ray, bounds_min, bounds_max, box_t))
		{
			return false;
		}
	}

	const size_t vertex_count = props.brush_vertices.size() / 3;
	float best_t = 1.0e30f;
	bool hit = false;
	for (size_t i = 0; i + 2 < props.brush_indices.size(); i += 3)
	{
		const uint32_t i0 = props.brush_indices[i];
		const uint32_t i1 = props.brush_indices[i + 1];
		const uint32_t i2 = props.brush_indices[i + 2];
		if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
		{
			continue;
		}
		const size_t base0 = static_cast<size_t>(i0) * 3;
		const size_t base1 = static_cast<size_t>(i1) * 3;
		const size_t base2 = static_cast<size_t>(i2) * 3;
		const Diligent::float3 v0(
			props.brush_vertices[base0 + 0],
			props.brush_vertices[base0 + 1],
			props.brush_vertices[base0 + 2]);
		const Diligent::float3 v1(
			props.brush_vertices[base1 + 0],
			props.brush_vertices[base1 + 1],
			props.brush_vertices[base1 + 2]);
		const Diligent::float3 v2(
			props.brush_vertices[base2 + 0],
			props.brush_vertices[base2 + 1],
			props.brush_vertices[base2 + 2]);
		float t = 0.0f;
		if (RayIntersectsTriangle(ray, v0, v1, v2, t))
		{
			if (t < best_t)
			{
				best_t = t;
				hit = true;
			}
		}
	}

	if (hit)
	{
		out_t = best_t;
	}
	return hit;
}

bool RaycastNode(const NodeProperties& props, const PickRay& ray, float& out_t)
{
	if (RaycastBrush(props, ray, out_t))
	{
		return true;
	}

	float bounds_min[3];
	float bounds_max[3];
	if (TryGetNodeBoundsInternal(props, bounds_min, bounds_max))
	{
		return RayIntersectsAABB(ray, bounds_min, bounds_max, out_t);
	}

	float pick_pos[3] = {props.position[0], props.position[1], props.position[2]};
	TryGetNodePickPosition(props, pick_pos);
	const Diligent::float3 point(pick_pos[0], pick_pos[1], pick_pos[2]);
	const Diligent::float3 to_point = Sub(point, ray.origin);
	const float t = Dot(to_point, ray.dir);
	if (t < 0.0f)
	{
		return false;
	}
	const Diligent::float3 closest = Add(ray.origin, Scale(ray.dir, t));
	const Diligent::float3 diff = Sub(point, closest);
	const float dist_sq = Dot(diff, diff);
	const float threshold = 24.0f;
	if (dist_sq <= threshold * threshold)
	{
		out_t = t;
		return true;
	}
	return false;
}
