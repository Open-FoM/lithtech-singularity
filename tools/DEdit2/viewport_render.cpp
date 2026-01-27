#include "viewport_render.h"

#include "ui_viewport.h"

#include "DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
struct GridVertex
{
	Diligent::float3 position;
	Diligent::float4 color;
};

struct GridConstants
{
	Diligent::float4x4 view_proj;
	Diligent::float3 grid_origin;
	float padding = 0.0f;
};

Diligent::float3 Cross(const Diligent::float3& a, const Diligent::float3& b)
{
	return Diligent::float3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
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

Diligent::float4x4 LookAtLH(const Diligent::float3& eye, const Diligent::float3& target, const Diligent::float3& up)
{
	const Diligent::float3 zaxis = Normalize(Diligent::float3(target.x - eye.x, target.y - eye.y, target.z - eye.z));
	const Diligent::float3 xaxis = Normalize(Cross(up, zaxis));
	const Diligent::float3 yaxis = Cross(zaxis, xaxis);

	return Diligent::float4x4(
		xaxis.x, yaxis.x, zaxis.x, 0.0f,
		xaxis.y, yaxis.y, zaxis.y, 0.0f,
		xaxis.z, yaxis.z, zaxis.z, 0.0f,
		-Dot(xaxis, eye), -Dot(yaxis, eye), -Dot(zaxis, eye), 1.0f);
}

Diligent::float4x4 LookAtLH(const Diligent::float3& eye, const Diligent::float3& target)
{
	return LookAtLH(eye, target, Diligent::float3(0.0f, 1.0f, 0.0f));
}

void BuildGridVertices(
	const ViewportGridRenderer& renderer,
	std::vector<GridVertex>& out_vertices,
	uint32_t& out_grid_count,
	uint32_t& out_axis_count)
{
	out_vertices.clear();
	out_grid_count = 0;
	out_axis_count = 0;
	const float extent = renderer.grid_half_extent;
	const float spacing = renderer.grid_spacing;
	if (extent <= 0.0f || spacing <= 0.0f)
	{
		return;
	}

	const int line_count = static_cast<int>(std::floor(extent / spacing));
	const int major_every = std::max(1, renderer.major_line_every);
	const Diligent::float4 minor_color(0.35f, 0.37f, 0.40f, 0.6f);
	const Diligent::float4 major_color(0.55f, 0.58f, 0.62f, 0.8f);
	const Diligent::float4 axis_x_color(0.9f, 0.35f, 0.35f, 1.0f);
	const Diligent::float4 axis_z_color(0.35f, 0.55f, 0.9f, 1.0f);
	const Diligent::float4 axis_y_color(0.35f, 0.9f, 0.45f, 1.0f);

	for (int i = -line_count; i <= line_count; ++i)
	{
		const float coord = static_cast<float>(i) * spacing;
		const bool is_major = (i % major_every) == 0;
		const Diligent::float4 color = is_major ? major_color : minor_color;

		out_vertices.push_back({Diligent::float3(coord, 0.0f, -extent), color});
		out_vertices.push_back({Diligent::float3(coord, 0.0f, extent), color});
		out_vertices.push_back({Diligent::float3(-extent, 0.0f, coord), color});
		out_vertices.push_back({Diligent::float3(extent, 0.0f, coord), color});
	}

	const uint32_t grid_count = static_cast<uint32_t>(out_vertices.size());

	out_vertices.push_back({Diligent::float3(-extent, 0.0f, 0.0f), axis_x_color});
	out_vertices.push_back({Diligent::float3(extent, 0.0f, 0.0f), axis_x_color});
	out_vertices.push_back({Diligent::float3(0.0f, 0.0f, -extent), axis_z_color});
	out_vertices.push_back({Diligent::float3(0.0f, 0.0f, extent), axis_z_color});
	out_vertices.push_back({Diligent::float3(0.0f, 0.0f, 0.0f), axis_y_color});
	out_vertices.push_back({Diligent::float3(0.0f, extent * 0.12f, 0.0f), axis_y_color});

	const uint32_t axis_count = static_cast<uint32_t>(out_vertices.size()) - grid_count;
	out_grid_count = grid_count;
	out_axis_count = axis_count;
}

} // namespace

bool InitViewportGridRenderer(
	Diligent::IRenderDevice* device,
	Diligent::TEXTURE_FORMAT rtv_format,
	Diligent::TEXTURE_FORMAT dsv_format,
	ViewportGridRenderer& renderer)
{
	if (!device)
	{
		return false;
	}

	std::vector<GridVertex> vertices;
	uint32_t grid_count = 0;
	uint32_t axis_count = 0;
	BuildGridVertices(renderer, vertices, grid_count, axis_count);
	if (vertices.empty())
	{
		return false;
	}

	renderer.grid_vertex_count = grid_count;
	renderer.axis_vertex_count = axis_count;

	Diligent::BufferDesc vb_desc;
	vb_desc.Name = "DEdit2 Viewport Grid VB";
	vb_desc.Usage = Diligent::USAGE_IMMUTABLE;
	vb_desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	vb_desc.Size = static_cast<Diligent::Uint64>(vertices.size() * sizeof(GridVertex));

	Diligent::BufferData vb_data;
	vb_data.pData = vertices.data();
	vb_data.DataSize = vb_desc.Size;
	device->CreateBuffer(vb_desc, &vb_data, &renderer.vertex_buffer);
	if (!renderer.vertex_buffer)
	{
		return false;
	}

	Diligent::BufferDesc cb_desc;
	cb_desc.Name = "DEdit2 Viewport Grid CB";
	cb_desc.Usage = Diligent::USAGE_DYNAMIC;
	cb_desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	cb_desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	cb_desc.Size = sizeof(GridConstants);
	device->CreateBuffer(cb_desc, nullptr, &renderer.constant_buffer);
	if (!renderer.constant_buffer)
	{
		return false;
	}

	static const char* vs_source = R"(
cbuffer Camera : register(b0)
{
    float4x4 g_ViewProj;
    float3 g_GridOrigin;
    float g_Pad;
};

struct VSInput
{
    float3 Pos   : ATTRIB0;
    float4 Color : ATTRIB1;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR0;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.Pos = mul(g_ViewProj, float4(input.Pos + g_GridOrigin, 1.0f));
    output.Color = input.Color;
    return output;
}
)";

	static const char* ps_source = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET
{
    return input.Color;
}
)";

	Diligent::ShaderCreateInfo shader_ci;
	shader_ci.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	Diligent::RefCntAutoPtr<Diligent::IShader> vs;
	shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
	shader_ci.Desc.Name = "DEdit2 Grid VS";
	shader_ci.EntryPoint = "main";
	shader_ci.Source = vs_source;
	device->CreateShader(shader_ci, &vs);
	if (!vs)
	{
		return false;
	}

	Diligent::RefCntAutoPtr<Diligent::IShader> ps;
	shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	shader_ci.Desc.Name = "DEdit2 Grid PS";
	shader_ci.EntryPoint = "main";
	shader_ci.Source = ps_source;
	device->CreateShader(shader_ci, &ps);
	if (!ps)
	{
		return false;
	}

	Diligent::GraphicsPipelineStateCreateInfo pso_ci;
	pso_ci.PSODesc.Name = "DEdit2 Viewport Grid PSO";
	pso_ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
	pso_ci.pVS = vs;
	pso_ci.pPS = ps;

	auto& gp = pso_ci.GraphicsPipeline;
	gp.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
	gp.NumRenderTargets = 1;
	gp.RTVFormats[0] = rtv_format;
	gp.DSVFormat = dsv_format;
	gp.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	gp.DepthStencilDesc.DepthEnable = true;
	gp.DepthStencilDesc.DepthWriteEnable = false;
	gp.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;

	Diligent::LayoutElement layout[] = {
		Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false}};
	gp.InputLayout.LayoutElements = layout;
	gp.InputLayout.NumElements = static_cast<Diligent::Uint32>(sizeof(layout) / sizeof(layout[0]));

	device->CreateGraphicsPipelineState(pso_ci, &renderer.pipeline);
	if (!renderer.pipeline)
	{
		return false;
	}

	if (auto* var = renderer.pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Camera"))
	{
		var->Set(renderer.constant_buffer);
	}
	renderer.pipeline->CreateShaderResourceBinding(&renderer.srb, true);
	return renderer.srb != nullptr;
}

void UpdateViewportGridConstants(
	Diligent::IDeviceContext* context,
	ViewportGridRenderer& renderer,
	const Diligent::float4x4& view_proj,
	const Diligent::float3& grid_origin)
{
	if (!context || !renderer.constant_buffer)
	{
		return;
	}

	Diligent::MapHelper<GridConstants> constants(context, renderer.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
	constants->view_proj = view_proj;
	constants->grid_origin = grid_origin;
}

void DrawViewportGrid(
	Diligent::IDeviceContext* context,
	ViewportGridRenderer& renderer,
	bool draw_grid,
	bool draw_axes)
{
	if (!context || !renderer.pipeline || !renderer.vertex_buffer)
	{
		return;
	}
	if (!draw_grid && !draw_axes)
	{
		return;
	}

	context->SetPipelineState(renderer.pipeline);
	Diligent::IBuffer* vb = renderer.vertex_buffer;
	const Diligent::Uint64 offset = 0;
	context->SetVertexBuffers(0, 1, &vb, &offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
	context->CommitShaderResources(renderer.srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (draw_grid && renderer.grid_vertex_count > 0)
	{
		Diligent::DrawAttribs draw_attrs(renderer.grid_vertex_count, Diligent::DRAW_FLAG_VERIFY_ALL, 1, 0);
		context->Draw(draw_attrs);
	}
	if (draw_axes && renderer.axis_vertex_count > 0)
	{
		Diligent::DrawAttribs draw_attrs(renderer.axis_vertex_count, Diligent::DRAW_FLAG_VERIFY_ALL, 1, renderer.grid_vertex_count);
		context->Draw(draw_attrs);
	}
}

Diligent::float4x4 ComputeViewportViewProj(
	const ViewportPanelState& state,
	float aspect_ratio)
{
	const float near_z = 1.0f;
	const float far_z = 20000.0f;

	// Handle orthographic views
	if (IsOrthographicView(state))
	{
		const float half_height = state.ortho_zoom * 400.0f;  // Base size scaled by zoom
		const float half_width = half_height * std::max(0.01f, aspect_ratio);

		Diligent::float3 eye(0.0f, 0.0f, 0.0f);
		Diligent::float3 target(0.0f, 0.0f, 0.0f);
		Diligent::float3 up(0.0f, 1.0f, 0.0f);

		const float cam_dist = 10000.0f;  // Far enough to see everything

		// ortho_center[0] = horizontal screen axis, ortho_center[1] = vertical screen axis
		// ortho_depth = preserved depth along the view axis
		switch (state.view_mode)
		{
		case ViewportPanelState::ViewMode::Top:
			// Looking toward -Y from +Y position; X=right, Z=up on screen
			eye = Diligent::float3(state.ortho_center[0], cam_dist + state.ortho_depth, state.ortho_center[1]);
			target = Diligent::float3(state.ortho_center[0], state.ortho_depth, state.ortho_center[1]);
			up = Diligent::float3(0.0f, 0.0f, 1.0f);
			break;
		case ViewportPanelState::ViewMode::Bottom:
			// Looking toward +Y from -Y position; X=right, -Z=up on screen
			eye = Diligent::float3(state.ortho_center[0], -cam_dist + state.ortho_depth, state.ortho_center[1]);
			target = Diligent::float3(state.ortho_center[0], state.ortho_depth, state.ortho_center[1]);
			up = Diligent::float3(0.0f, 0.0f, -1.0f);
			break;
		case ViewportPanelState::ViewMode::Front:
			// Looking toward +Z from -Z position; X=right, Y=up on screen
			eye = Diligent::float3(state.ortho_center[0], state.ortho_center[1], -cam_dist + state.ortho_depth);
			target = Diligent::float3(state.ortho_center[0], state.ortho_center[1], state.ortho_depth);
			up = Diligent::float3(0.0f, 1.0f, 0.0f);
			break;
		case ViewportPanelState::ViewMode::Back:
			// Looking toward -Z from +Z position; -X=right, Y=up on screen
			eye = Diligent::float3(state.ortho_center[0], state.ortho_center[1], cam_dist + state.ortho_depth);
			target = Diligent::float3(state.ortho_center[0], state.ortho_center[1], state.ortho_depth);
			up = Diligent::float3(0.0f, 1.0f, 0.0f);
			break;
		case ViewportPanelState::ViewMode::Left:
			// Looking toward +X from -X position; Z=right, Y=up on screen
			eye = Diligent::float3(-cam_dist + state.ortho_depth, state.ortho_center[1], state.ortho_center[0]);
			target = Diligent::float3(state.ortho_depth, state.ortho_center[1], state.ortho_center[0]);
			up = Diligent::float3(0.0f, 1.0f, 0.0f);
			break;
		case ViewportPanelState::ViewMode::Right:
			// Looking toward -X from +X position; -Z=right, Y=up on screen
			eye = Diligent::float3(cam_dist + state.ortho_depth, state.ortho_center[1], state.ortho_center[0]);
			target = Diligent::float3(state.ortho_depth, state.ortho_center[1], state.ortho_center[0]);
			up = Diligent::float3(0.0f, 1.0f, 0.0f);
			break;
		default:
			break;
		}

		const Diligent::float4x4 view = LookAtLH(eye, target, up);
		const Diligent::float4x4 proj = Diligent::float4x4::OrthoOffCenter(
			-half_width, half_width, -half_height, half_height, near_z, far_z, false);
		return view * proj;
	}

	// Perspective view
	const float yaw = state.orbit_yaw;
	const float pitch = state.orbit_pitch;
	const float cp = std::cos(pitch);
	const float sp = std::sin(pitch);
	const float cy = std::cos(yaw);
	const float sy = std::sin(yaw);

	const Diligent::float3 forward(sy * cp, sp, cy * cp);
	Diligent::float3 eye(0.0f, 0.0f, 0.0f);
	Diligent::float3 target(0.0f, 0.0f, 0.0f);

	if (state.fly_mode)
	{
		eye = Diligent::float3(state.fly_pos[0], state.fly_pos[1], state.fly_pos[2]);
		target = Diligent::float3(eye.x + forward.x, eye.y + forward.y, eye.z + forward.z);
	}
	else
	{
		target = Diligent::float3(state.target[0], state.target[1], state.target[2]);
		eye = Diligent::float3(target.x - forward.x * state.orbit_distance,
			target.y - forward.y * state.orbit_distance,
			target.z - forward.z * state.orbit_distance);
	}

	const Diligent::float4x4 view = LookAtLH(eye, target);
	const float fov = Diligent::PI_F / 4.0f;
	const Diligent::float4x4 proj = Diligent::float4x4::Projection(fov, std::max(0.01f, aspect_ratio), near_z, far_z, false);
	return view * proj;
}
