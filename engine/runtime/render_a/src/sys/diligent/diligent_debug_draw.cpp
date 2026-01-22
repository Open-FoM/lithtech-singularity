#include "diligent_debug_draw.h"

#include "bdefs.h"
#include "diligent_device.h"
#include "diligent_internal.h"
#include "diligent_model_draw.h"
#include "diligent_postfx.h"
#include "diligent_utils.h"
#include "diligent_world_draw.h"
#include "diligent_world_data.h"
#include "ltmatrix.h"
#include "viewparams.h"
#include "world_client_bsp.h"
#include "world_tree.h"
#include "../rendererconsolevars.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

struct DiligentLinePipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct DiligentLinePipelineKey
{
	uint8 blend = 0;
	uint8 depth = 0;

	bool operator==(const DiligentLinePipelineKey& other) const
	{
		return blend == other.blend && depth == other.depth;
	}
};

struct DiligentLinePipelineKeyHash
{
	size_t operator()(const DiligentLinePipelineKey& key) const noexcept
	{
		return (static_cast<size_t>(key.blend) << 8) ^ static_cast<size_t>(key.depth);
	}
};

struct DiligentLinePipelines
{
	std::unordered_map<DiligentLinePipelineKey, DiligentLinePipeline, DiligentLinePipelineKeyHash> pipelines;
};

DiligentLinePipelines g_line_pipelines;

std::vector<DiligentDebugLineVertex> g_diligent_debug_lines;
Diligent::RefCntAutoPtr<Diligent::IBuffer> g_diligent_debug_line_buffer;
uint32 g_diligent_debug_line_buffer_size = 0;

} // namespace
DiligentLinePipeline* diligent_get_line_pipeline(
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode)
{
	DiligentLinePipelineKey key{static_cast<uint8>(blend_mode), static_cast<uint8>(depth_mode)};
	auto it = g_line_pipelines.pipelines.find(key);
	if (it != g_line_pipelines.pipelines.end())
	{
		return &it->second;
	}

	if (!diligent_ensure_world_shaders())
	{
		return nullptr;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return nullptr;
	}

	if (!g_diligent_state.render_device || !g_diligent_state.swap_chain)
	{
		return nullptr;
	}

	const auto swap_desc = g_diligent_state.swap_chain->GetDesc();

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_line";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = swap_desc.ColorBufferFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = swap_desc.DepthBufferFormat;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;

	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{3, 0, 2, Diligent::VT_FLOAT32, false}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	const bool depth_enabled = (depth_mode != kWorldDepthDisabled);
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = depth_enabled;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = depth_enabled;
	if (!depth_enabled || blend_mode != kWorldBlendSolid)
	{
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
	}
	if (!depth_enabled)
	{
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_ALWAYS;
	}
	diligent_fill_world_blend_desc(blend_mode, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);

	pipeline_info.pVS = g_world_resources.vertex_shader;
	pipeline_info.pPS = g_world_resources.pixel_shader_solid;
	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
	pipeline_info.PSODesc.ResourceLayout.Variables = nullptr;
	pipeline_info.PSODesc.ResourceLayout.NumVariables = 0u;
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = nullptr;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 0u;

	DiligentLinePipeline pipeline;
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* vs_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "WorldConstants");
	if (vs_constants)
	{
		vs_constants->Set(g_world_resources.constant_buffer);
	}

	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "WorldConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_world_resources.constant_buffer);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	auto result = g_line_pipelines.pipelines.emplace(key, std::move(pipeline));
	return result.second ? &result.first->second : nullptr;
}

bool diligent_draw_debug_lines()
{
	if (g_diligent_debug_lines.empty())
	{
		return true;
	}

	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	auto* pipeline = diligent_get_line_pipeline(kWorldBlendSolid, kWorldDepthEnabled);
	if (!pipeline || !pipeline->pipeline_state)
	{
		return false;
	}

	const uint32 data_size = static_cast<uint32>(g_diligent_debug_lines.size() * sizeof(DiligentDebugLineVertex));
	if (!g_diligent_debug_line_buffer || g_diligent_debug_line_buffer_size < data_size)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_debug_lines";
		desc.Size = data_size;
		desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		desc.ElementByteStride = sizeof(DiligentDebugLineVertex);
		g_diligent_debug_line_buffer.Release();
		g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_diligent_debug_line_buffer);
		if (!g_diligent_debug_line_buffer)
		{
			return false;
		}
		g_diligent_debug_line_buffer_size = data_size;
	}

	void* mapped_vertices = nullptr;
	g_diligent_state.immediate_context->MapBuffer(
		g_diligent_debug_line_buffer,
		Diligent::MAP_WRITE,
		Diligent::MAP_FLAG_DISCARD,
		mapped_vertices);
	if (!mapped_vertices)
	{
		return false;
	}
	std::memcpy(mapped_vertices, g_diligent_debug_lines.data(), data_size);
	g_diligent_state.immediate_context->UnmapBuffer(g_diligent_debug_line_buffer, Diligent::MAP_WRITE);

	DiligentWorldConstants constants;
	LTMatrix world_matrix;
	world_matrix.Identity();
	LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * world_matrix;
	diligent_store_matrix_from_lt(mvp, constants.mvp);
	diligent_store_matrix_from_lt(g_diligent_state.view_params.m_mView, constants.view);
	diligent_store_matrix_from_lt(world_matrix, constants.world);
	constants.camera_pos[0] = g_diligent_state.view_params.m_Pos.x;
	constants.camera_pos[1] = g_diligent_state.view_params.m_Pos.y;
	constants.camera_pos[2] = g_diligent_state.view_params.m_Pos.z;
	constants.camera_pos[3] = 0.0f;
	constants.fog_color[0] = 0.0f;
	constants.fog_color[1] = 0.0f;
	constants.fog_color[2] = 0.0f;
	constants.fog_color[3] = 0.0f;
	constants.fog_params[0] = 0.0f;
	constants.fog_params[1] = diligent_get_tonemap_enabled();
	constants.fog_params[2] = diligent_get_swapchain_output_is_srgb();
	constants.fog_params[3] = 0.0f;

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

	auto* render_target = diligent_get_active_render_target();
	auto* depth_target = diligent_get_active_depth_target();
	g_diligent_state.immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::IBuffer* buffer = g_diligent_debug_line_buffer.RawPtr();
	Diligent::Uint64 offsets[] = {0};
	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		1,
		&buffer,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
	if (pipeline->srb)
	{
		g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = static_cast<uint32>(g_diligent_debug_lines.size());
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->Draw(draw_attribs);
	return true;
}

bool diligent_draw_line_vertices(
	const DiligentWorldConstants& constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	const DiligentWorldVertex* vertices,
	uint32 vertex_count)
{
	if (!g_diligent_state.immediate_context || !g_diligent_state.swap_chain)
	{
		return false;
	}

	if (!vertices || vertex_count == 0)
	{
		return true;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

	const uint32 vertex_bytes = vertex_count * sizeof(DiligentWorldVertex);
	if (!diligent_ensure_world_vertex_buffer(vertex_bytes))
	{
		return false;
	}

	void* mapped_vertices = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	if (!mapped_vertices)
	{
		return false;
	}
	std::memcpy(mapped_vertices, vertices, vertex_bytes);
	g_diligent_state.immediate_context->UnmapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE);

	auto* render_target = diligent_get_active_render_target();
	auto* depth_target = diligent_get_active_depth_target();
	g_diligent_state.immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::IBuffer* buffers[] = {g_world_resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

	auto* pipeline = diligent_get_line_pipeline(blend_mode, depth_mode);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = vertex_count;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->Draw(draw_attribs);
	return true;
}

bool diligent_world_debug_enabled()
{
	return g_CV_DrawWorldTree.m_Val != 0 ||
		g_CV_DrawWorldLeaves.m_Val != 0 ||
		g_CV_DrawWorldPortals.m_Val != 0;
}

bool diligent_debug_lines_enabled()
{
	return diligent_model_debug_enabled() || diligent_world_debug_enabled();
}

LTRGBColor diligent_make_debug_color(uint8 r, uint8 g, uint8 b, uint8 a)
{
	LTRGBColor color{};
	color.rgb.r = r;
	color.rgb.g = g;
	color.rgb.b = b;
	color.rgb.a = a;
	return color;
}

void diligent_debug_clear_lines()
{
	g_diligent_debug_lines.clear();
}

void diligent_debug_add_line(const LTVector& from, const LTVector& to, const LTRGBColor& color)
{
	DiligentDebugLineVertex v0{};
	DiligentDebugLineVertex v1{};
	v0.position[0] = from.x;
	v0.position[1] = from.y;
	v0.position[2] = from.z;
	v1.position[0] = to.x;
	v1.position[1] = to.y;
	v1.position[2] = to.z;
	const float r = static_cast<float>(color.rgb.r) / 255.0f;
	const float g = static_cast<float>(color.rgb.g) / 255.0f;
	const float b = static_cast<float>(color.rgb.b) / 255.0f;
	const float a = static_cast<float>(color.rgb.a) / 255.0f;
	v0.color[0] = r;
	v0.color[1] = g;
	v0.color[2] = b;
	v0.color[3] = a;
	v1.color[0] = r;
	v1.color[1] = g;
	v1.color[2] = b;
	v1.color[3] = a;
	g_diligent_debug_lines.push_back(v0);
	g_diligent_debug_lines.push_back(v1);
}

void diligent_debug_add_aabb(const LTVector& center, const LTVector& dims, const LTRGBColor& color)
{
	const LTVector min = center - dims;
	const LTVector max = center + dims;
	const LTVector p0(min.x, min.y, min.z);
	const LTVector p1(max.x, min.y, min.z);
	const LTVector p2(max.x, max.y, min.z);
	const LTVector p3(min.x, max.y, min.z);
	const LTVector p4(min.x, min.y, max.z);
	const LTVector p5(max.x, min.y, max.z);
	const LTVector p6(max.x, max.y, max.z);
	const LTVector p7(min.x, max.y, max.z);

	diligent_debug_add_line(p0, p1, color);
	diligent_debug_add_line(p1, p2, color);
	diligent_debug_add_line(p2, p3, color);
	diligent_debug_add_line(p3, p0, color);
	diligent_debug_add_line(p4, p5, color);
	diligent_debug_add_line(p5, p6, color);
	diligent_debug_add_line(p6, p7, color);
	diligent_debug_add_line(p7, p4, color);
	diligent_debug_add_line(p0, p4, color);
	diligent_debug_add_line(p1, p5, color);
	diligent_debug_add_line(p2, p6, color);
	diligent_debug_add_line(p3, p7, color);
}

void diligent_debug_add_world_node_bounds(const WorldTreeNode* node, const LTRGBColor& color)
{
	if (!node)
	{
		return;
	}

	const LTVector& min = node->GetBBoxMin();
	const LTVector& max = node->GetBBoxMax();
	if (!g_diligent_state.view_params.ViewAABBIntersect(min, max))
	{
		return;
	}

	const LTVector center(
		(min.x + max.x) * 0.5f,
		(min.y + max.y) * 0.5f,
		(min.z + max.z) * 0.5f);
	const LTVector dims(
		(max.x - min.x) * 0.5f,
		(max.y - min.y) * 0.5f,
		(max.z - min.z) * 0.5f);
	diligent_debug_add_aabb(center, dims, color);
}

void diligent_debug_add_world_tree_node(const WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	const bool has_children = node->HasChildren();
	if (!has_children && g_CV_DrawWorldLeaves.m_Val != 0)
	{
		diligent_debug_add_world_node_bounds(node, diligent_make_debug_color(255, 220, 64, 220));
	}
	else if (has_children && g_CV_DrawWorldTree.m_Val != 0)
	{
		diligent_debug_add_world_node_bounds(node, diligent_make_debug_color(80, 220, 120, 200));
	}

	if (has_children)
	{
		for (uint32 child_index = 0; child_index < 4; ++child_index)
		{
			diligent_debug_add_world_tree_node(node->GetChild(child_index));
		}
	}
}

void diligent_debug_add_world_portals()
{
	if (g_visible_render_blocks.empty())
	{
		return;
	}

	const LTRGBColor color = diligent_make_debug_color(96, 200, 255, 220);
	for (const auto* block : g_visible_render_blocks)
	{
		if (!block)
		{
			continue;
		}
		for (const auto& poly : block->sky_portals)
		{
			const size_t count = poly.vertices.size();
			if (count < 2)
			{
				continue;
			}
			for (size_t i = 0; i < count; ++i)
			{
				const LTVector& from = poly.vertices[i];
				const LTVector& to = poly.vertices[(i + 1) % count];
				diligent_debug_add_line(from, to, color);
			}
		}
	}
}

void diligent_debug_add_world_lines()
{
	if (!diligent_world_debug_enabled())
	{
		return;
	}

	if (g_CV_DrawWorldTree.m_Val != 0 || g_CV_DrawWorldLeaves.m_Val != 0)
	{
		if (auto* bsp_client = diligent_get_world_bsp_client())
		{
			WorldTree* tree = bsp_client->ClientTree();
			if (tree)
			{
				diligent_debug_add_world_tree_node(tree->GetRootNode());
			}
		}
	}

	if (g_CV_DrawWorldPortals.m_Val != 0)
	{
		diligent_debug_add_world_portals();
	}
}

void diligent_debug_add_obb(const ModelOBB& obb, const LTRGBColor& color)
{
	const LTVector axis_x = obb.m_Basis[0] * obb.m_Size.x;
	const LTVector axis_y = obb.m_Basis[1] * obb.m_Size.y;
	const LTVector axis_z = obb.m_Basis[2] * obb.m_Size.z;
	const LTVector center = obb.m_Pos;

	LTVector corners[8];
	corners[0] = center - axis_x - axis_y - axis_z;
	corners[1] = center + axis_x - axis_y - axis_z;
	corners[2] = center + axis_x + axis_y - axis_z;
	corners[3] = center - axis_x + axis_y - axis_z;
	corners[4] = center - axis_x - axis_y + axis_z;
	corners[5] = center + axis_x - axis_y + axis_z;
	corners[6] = center + axis_x + axis_y + axis_z;
	corners[7] = center - axis_x + axis_y + axis_z;

	diligent_debug_add_line(corners[0], corners[1], color);
	diligent_debug_add_line(corners[1], corners[2], color);
	diligent_debug_add_line(corners[2], corners[3], color);
	diligent_debug_add_line(corners[3], corners[0], color);
	diligent_debug_add_line(corners[4], corners[5], color);
	diligent_debug_add_line(corners[5], corners[6], color);
	diligent_debug_add_line(corners[6], corners[7], color);
	diligent_debug_add_line(corners[7], corners[4], color);
	diligent_debug_add_line(corners[0], corners[4], color);
	diligent_debug_add_line(corners[1], corners[5], color);
	diligent_debug_add_line(corners[2], corners[6], color);
	diligent_debug_add_line(corners[3], corners[7], color);
}

void diligent_debug_term()
{
	g_line_pipelines.pipelines.clear();
	g_diligent_debug_lines.clear();
	g_diligent_debug_line_buffer.Release();
	g_diligent_debug_line_buffer_size = 0;
}
