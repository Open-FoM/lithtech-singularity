#include "diligent_model_draw.h"

#include "diligent_buffers.h"
#include "diligent_device.h"
#include "diligent_debug_draw.h"
#include "diligent_internal.h"
#include "diligent_pipeline_cache.h"
#include "diligent_postfx.h"
#include "diligent_texture_cache.h"
#include "diligent_utils.h"
#include "diligent_world_draw.h"

#include "bdefs.h"
#include "de_objects.h"
#include "de_world.h"
#include "iltcommon.h"
#include "renderstruct.h"
#include "ltrenderstyle.h"
#include "ltanimtracker.h"
#include "ltb.h"
#include "ltvector.h"
#include "viewparams.h"
#include "../renderstylemap.h"

#include "diligent_shaders_generated.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

#include "../rendererconsolevars.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>

struct DiligentModelConstants
{
	std::array<float, 16> mvp{};
	std::array<float, 16> model{};
	float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float model_params[4] = {1.0f, 1.0f, 1.0f, 0.0f};
	float camera_pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float fog_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float fog_params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DiligentModelResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_textured;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_solid;
	std::unordered_map<uint32, Diligent::RefCntAutoPtr<Diligent::IShader>> vertex_shaders;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
};

struct DiligentModelPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	bool uses_texture = false;
};

DiligentModelResources g_model_resources;
std::unordered_map<DiligentModelPipelineKey, DiligentModelPipeline, DiligentModelPipelineKeyHash> g_model_pipelines;

bool diligent_ensure_model_constant_buffer()
{
	if (g_model_resources.constant_buffer)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_model_constants";
	desc.Size = sizeof(DiligentModelConstants);
	desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

	g_diligent_state.render_device->CreateBuffer(desc, nullptr, &g_model_resources.constant_buffer);
	return g_model_resources.constant_buffer != nullptr;
}

bool diligent_ensure_model_pixel_shaders()
{
	if (g_model_resources.pixel_shader_textured && g_model_resources.pixel_shader_solid)
	{
		return true;
	}

	if (!g_diligent_state.render_device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	Diligent::ShaderDesc pixel_desc;
	pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	shader_info.Desc = pixel_desc;
	shader_info.EntryPoint = "PSMain";

	if (!g_model_resources.pixel_shader_textured)
	{
		pixel_desc.Name = "ltjs_model_ps_textured";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_model_pixel_shader_textured_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_model_resources.pixel_shader_textured);
	}

	if (!g_model_resources.pixel_shader_solid)
	{
		pixel_desc.Name = "ltjs_model_ps_solid";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_model_pixel_shader_solid_source();
		g_diligent_state.render_device->CreateShader(shader_info, &g_model_resources.pixel_shader_solid);
	}

	return g_model_resources.pixel_shader_textured && g_model_resources.pixel_shader_solid;
}

Diligent::IShader* diligent_get_model_vertex_shader(const DiligentMeshLayout& layout)
{
	auto it = g_model_resources.vertex_shaders.find(layout.hash);
	if (it != g_model_resources.vertex_shaders.end())
	{
		return it->second.RawPtr();
	}

	if (!g_diligent_state.render_device)
	{
		return nullptr;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
	Diligent::ShaderDesc vertex_desc;
	vertex_desc.Name = "ltjs_model_vs";
	vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
	shader_info.Desc = vertex_desc;
	shader_info.EntryPoint = "VSMain";
	std::string source = diligent_build_model_vertex_shader_source(layout);
	shader_info.Source = source.c_str();

	Diligent::RefCntAutoPtr<Diligent::IShader> shader;
	g_diligent_state.render_device->CreateShader(shader_info, &shader);
	if (!shader)
	{
		return nullptr;
	}

	g_model_resources.vertex_shaders.emplace(layout.hash, shader);
	return shader.RawPtr();
}

DiligentTextureArray diligent_resolve_textures(const RenderPassOp& pass, SharedTexture** texture_list)
{
	DiligentTextureArray textures{};
	if (!texture_list)
	{
		return textures;
	}

	for (uint32 stage_index = 0; stage_index < 4; ++stage_index)
	{
		switch (pass.TextureStages[stage_index].TextureParam)
		{
			case RENDERSTYLE_USE_TEXTURE1:
				textures[stage_index] = texture_list[0];
				break;
			case RENDERSTYLE_USE_TEXTURE2:
				textures[stage_index] = texture_list[1];
				break;
			case RENDERSTYLE_USE_TEXTURE3:
				textures[stage_index] = texture_list[2];
				break;
			case RENDERSTYLE_USE_TEXTURE4:
				textures[stage_index] = texture_list[3];
				break;
			default:
				textures[stage_index] = nullptr;
				break;
		}
	}

	return textures;
}

bool diligent_model_debug_enabled()
{
	return g_CV_ModelDebug_DrawBoxes.m_Val != 0 ||
		g_CV_ModelDebug_DrawTouchingLights.m_Val != 0 ||
		g_CV_ModelDebug_DrawSkeleton.m_Val != 0 ||
		g_CV_ModelDebug_DrawOBBS.m_Val != 0 ||
		g_CV_ModelDebug_DrawVertexNormals.m_Val != 0;
}

LTVector diligent_transform_point(const LTMatrix& matrix, const LTVector& point)
{
	LTVector out;
	out.x = matrix.m[0][0] * point.x + matrix.m[0][1] * point.y + matrix.m[0][2] * point.z + matrix.m[0][3];
	out.y = matrix.m[1][0] * point.x + matrix.m[1][1] * point.y + matrix.m[1][2] * point.z + matrix.m[1][3];
	out.z = matrix.m[2][0] * point.x + matrix.m[2][1] * point.y + matrix.m[2][2] * point.z + matrix.m[2][3];
	return out;
}

LTVector diligent_transform_dir(const LTMatrix& matrix, const LTVector& dir)
{
	LTVector out;
	out.x = matrix.m[0][0] * dir.x + matrix.m[0][1] * dir.y + matrix.m[0][2] * dir.z;
	out.y = matrix.m[1][0] * dir.x + matrix.m[1][1] * dir.y + matrix.m[1][2] * dir.z;
	out.z = matrix.m[2][0] * dir.x + matrix.m[2][1] * dir.y + matrix.m[2][2] * dir.z;
	return out;
}

bool diligent_read_vertex_vec3(
	const DiligentMeshLayout& layout,
	const std::array<std::vector<uint8>, 4>& vertex_data,
	int32 attrib_index,
	uint32 vertex_index,
	bool signed_normal,
	LTVector& out)
{
	if (attrib_index < 0)
	{
		return false;
	}

	const Diligent::LayoutElement* element = nullptr;
	for (const auto& elem : layout.elements)
	{
		if (elem.InputIndex == static_cast<uint32>(attrib_index))
		{
			element = &elem;
			break;
		}
	}
	if (!element)
	{
		return false;
	}

	const uint32 stream = element->BufferSlot;
	if (stream >= vertex_data.size())
	{
		return false;
	}
	const uint32 stride = layout.strides[stream];
	if (stride == 0)
	{
		return false;
	}
	const auto& data = vertex_data[stream];
	const size_t offset = static_cast<size_t>(element->RelativeOffset) + static_cast<size_t>(stride) * vertex_index;
	if (offset + sizeof(float) * 3 > data.size())
	{
		return false;
	}

	if (element->ValueType == Diligent::VT_FLOAT32)
	{
		const float* vals = reinterpret_cast<const float*>(data.data() + offset);
		out.x = vals[0];
		out.y = (element->NumComponents > 1) ? vals[1] : 0.0f;
		out.z = (element->NumComponents > 2) ? vals[2] : 0.0f;
		return true;
	}

	if (element->ValueType == Diligent::VT_UINT8)
	{
		const uint8* vals = data.data() + offset;
		float x = vals[0] / 255.0f;
		float y = (element->NumComponents > 1) ? vals[1] / 255.0f : 0.0f;
		float z = (element->NumComponents > 2) ? vals[2] / 255.0f : 0.0f;
		if (element->IsNormalized && signed_normal)
		{
			x = x * 2.0f - 1.0f;
			y = y * 2.0f - 1.0f;
			z = z * 2.0f - 1.0f;
		}
		out.x = x;
		out.y = y;
		out.z = z;
		return true;
	}

	return false;
}

void diligent_debug_add_vertex_normals(
	const DiligentMeshLayout& layout,
	const std::array<std::vector<uint8>, 4>& vertex_data,
	uint32 vertex_count,
	const LTMatrix& model_matrix,
	const LTRGBColor& color)
{
	if (layout.position_attrib < 0 || layout.normal_attrib < 0 || vertex_count == 0)
	{
		return;
	}

	constexpr uint32 kMaxNormals = 2048;
	const uint32 step = vertex_count > kMaxNormals ? (vertex_count / kMaxNormals) : 1;
	const float normal_scale = 5.0f;

	for (uint32 i = 0; i < vertex_count; i += step)
	{
		LTVector pos;
		LTVector normal;
		if (!diligent_read_vertex_vec3(layout, vertex_data, layout.position_attrib, i, false, pos))
		{
			continue;
		}
		if (!diligent_read_vertex_vec3(layout, vertex_data, layout.normal_attrib, i, true, normal))
		{
			continue;
		}

		const LTVector world_pos = diligent_transform_point(model_matrix, pos);
		LTVector world_normal = diligent_transform_dir(model_matrix, normal);
		world_normal.Norm();
		const LTVector end = world_pos + world_normal * normal_scale;
		diligent_debug_add_line(world_pos, end, color);
	}
}

Diligent::BLEND_FACTOR diligent_map_blend_factor(ERenStyle_BlendMode blend_mode, bool source)
{
	switch (blend_mode)
	{
		case RENDERSTYLE_BLEND_ADD:
			return source ? Diligent::BLEND_FACTOR_ONE : Diligent::BLEND_FACTOR_ONE;
		case RENDERSTYLE_BLEND_MOD_SRCALPHA:
			return source ? Diligent::BLEND_FACTOR_SRC_ALPHA : Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
		case RENDERSTYLE_BLEND_MOD_SRCCOLOR:
			return source ? Diligent::BLEND_FACTOR_SRC_COLOR : Diligent::BLEND_FACTOR_INV_SRC_COLOR;
		case RENDERSTYLE_BLEND_MOD_DSTCOLOR:
			return source ? Diligent::BLEND_FACTOR_DEST_COLOR : Diligent::BLEND_FACTOR_INV_DEST_COLOR;
		default:
			return source ? Diligent::BLEND_FACTOR_ONE : Diligent::BLEND_FACTOR_ZERO;
	}
}

void diligent_fill_model_blend_desc(const RenderPassOp& pass, Diligent::RenderTargetBlendDesc& desc)
{
	desc.BlendEnable = pass.BlendMode != RENDERSTYLE_NOBLEND;
	desc.SrcBlend = diligent_map_blend_factor(pass.BlendMode, true);
	desc.DestBlend = diligent_map_blend_factor(pass.BlendMode, false);
	desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
	desc.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
	desc.DestBlendAlpha = Diligent::BLEND_FACTOR_ZERO;
	desc.BlendOpAlpha = Diligent::BLEND_OPERATION_ADD;
	desc.RenderTargetWriteMask = Diligent::COLOR_MASK_ALL;
}

Diligent::COMPARISON_FUNCTION diligent_map_test_mode(ERenStyle_TestMode test_mode)
{
	switch (test_mode)
	{
		case RENDERSTYLE_ALPHATEST_LESS:
			return Diligent::COMPARISON_FUNC_LESS;
		case RENDERSTYLE_ALPHATEST_LESSEQUAL:
			return Diligent::COMPARISON_FUNC_LESS_EQUAL;
		case RENDERSTYLE_ALPHATEST_GREATER:
			return Diligent::COMPARISON_FUNC_GREATER;
		case RENDERSTYLE_ALPHATEST_GREATEREQUAL:
			return Diligent::COMPARISON_FUNC_GREATER_EQUAL;
		case RENDERSTYLE_ALPHATEST_EQUAL:
			return Diligent::COMPARISON_FUNC_EQUAL;
		case RENDERSTYLE_ALPHATEST_NOTEQUAL:
			return Diligent::COMPARISON_FUNC_NOT_EQUAL;
		default:
			return Diligent::COMPARISON_FUNC_LESS_EQUAL;
	}
}

void diligent_fill_model_depth_desc(const RenderPassOp& pass, Diligent::DepthStencilStateDesc& desc)
{
	switch (pass.ZBufferMode)
	{
		case RENDERSTYLE_NOZ:
			desc.DepthEnable = false;
			desc.DepthWriteEnable = false;
			break;
		case RENDERSTYLE_ZRO:
			desc.DepthEnable = true;
			desc.DepthWriteEnable = false;
			break;
		default:
			desc.DepthEnable = true;
			desc.DepthWriteEnable = true;
			break;
	}
	desc.DepthFunc = diligent_map_test_mode(pass.ZBufferTestMode);
}

void diligent_fill_model_raster_desc(const RenderPassOp& pass, Diligent::RasterizerStateDesc& desc)
{
	switch (pass.CullMode)
	{
		case RENDERSTYLE_CULL_NONE:
			desc.CullMode = Diligent::CULL_MODE_NONE;
			break;
		case RENDERSTYLE_CULL_CCW:
			desc.CullMode = Diligent::CULL_MODE_BACK;
			desc.FrontCounterClockwise = true;
			break;
		case RENDERSTYLE_CULL_CW:
			desc.CullMode = Diligent::CULL_MODE_BACK;
			desc.FrontCounterClockwise = false;
			break;
		default:
			desc.CullMode = Diligent::CULL_MODE_BACK;
			break;
	}

	switch (pass.FillMode)
	{
		case RENDERSTYLE_WIRE:
			desc.FillMode = Diligent::FILL_MODE_WIREFRAME;
			break;
		default:
			desc.FillMode = Diligent::FILL_MODE_SOLID;
			break;
	}
}

DiligentModelPipeline* diligent_get_model_pipeline_for_target(
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentMeshLayout& layout,
	bool uses_texture,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format)
{
	if (!g_diligent_state.render_device || !g_diligent_state.swap_chain)
	{
		return nullptr;
	}

	if (!diligent_ensure_model_constant_buffer() || !diligent_ensure_model_pixel_shaders())
	{
		return nullptr;
	}

	DiligentModelPipelineKey pipeline_key;
	pipeline_key.uses_texture = uses_texture;
	const uint8 sample_count = static_cast<uint8>(diligent_get_active_sample_count());
	pipeline_key.pso_key = diligent_make_pso_key(
		pass,
		shader_pass,
		layout.hash,
		color_format,
		depth_format,
		Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		sample_count);

	auto it = g_model_pipelines.find(pipeline_key);
	if (it != g_model_pipelines.end())
	{
		return &it->second;
	}

	Diligent::IShader* vertex_shader = diligent_get_model_vertex_shader(layout);
	if (!vertex_shader)
	{
		return nullptr;
	}

	Diligent::IShader* pixel_shader = uses_texture ? g_model_resources.pixel_shader_textured.RawPtr() : g_model_resources.pixel_shader_solid.RawPtr();
	if (!pixel_shader)
	{
		return nullptr;
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_model_pso";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = color_format;
	pipeline_info.GraphicsPipeline.DSVFormat = depth_format;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline_info.GraphicsPipeline.SmplDesc.Count = sample_count;
	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout.elements.data();
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(layout.elements.size());

	diligent_fill_model_raster_desc(pass, pipeline_info.GraphicsPipeline.RasterizerDesc);
	diligent_fill_model_depth_desc(pass, pipeline_info.GraphicsPipeline.DepthStencilDesc);
	diligent_fill_model_blend_desc(pass, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);

	pipeline_info.pVS = vertex_shader;
	pipeline_info.pPS = pixel_shader;

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::ShaderResourceVariableDesc variables[] = {
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture0", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	Diligent::ImmutableSamplerDesc sampler_desc;
	sampler_desc.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.SamplerOrTextureName = "g_Texture0_sampler";
	diligent_apply_anisotropy(sampler_desc.Desc);

	if (uses_texture)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 1u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1u;
	}

	DiligentModelPipeline pipeline;
	pipeline.uses_texture = uses_texture;
	g_diligent_state.render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* vs_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "ModelConstants");
	if (vs_constants)
	{
		vs_constants->Set(g_model_resources.constant_buffer);
	}

	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "ModelConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_model_resources.constant_buffer);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	auto result = g_model_pipelines.emplace(pipeline_key, std::move(pipeline));
	return result.second ? &result.first->second : nullptr;
}

DiligentModelPipeline* diligent_get_model_pipeline(
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentMeshLayout& layout,
	bool uses_texture)
{
	if (!g_diligent_state.swap_chain)
	{
		return nullptr;
	}

	const auto& swap_desc = g_diligent_state.swap_chain->GetDesc();
	return diligent_get_model_pipeline_for_target(
		pass,
		shader_pass,
		layout,
		uses_texture,
		swap_desc.ColorBufferFormat,
		swap_desc.DepthBufferFormat);
}

DiligentRigidMesh::DiligentRigidMesh()
{
	Reset();
}

DiligentRigidMesh::~DiligentRigidMesh()
{
	FreeAll();
}

void DiligentRigidMesh::Reset()
{
	m_vertex_count = 0;
	m_poly_count = 0;
	m_bone_effector = 0;
	m_non_fixed_pipe_data = false;
	m_layout.elements.clear();
	m_layout.strides.fill(0);
	m_layout.uv_attrib = { -1, -1, -1, -1 };
	m_layout.position_attrib = -1;
	m_layout.weights_attrib = -1;
	m_layout.indices_attrib = -1;
	m_layout.normal_attrib = -1;
	m_layout.color_attrib = -1;
	m_layout.hash = 0;
	m_index_data.clear();
	m_index_buffer.Release();
	for (uint32 i = 0; i < 4; ++i)
	{
		m_vert_stream_flags[i] = 0;
		m_vertex_data[i].clear();
		m_vertex_buffers[i].Release();
	}
}

void DiligentRigidMesh::FreeAll()
{
	FreeDeviceObjects();
	Reset();
}

void DiligentRigidMesh::FreeDeviceObjects()
{
	for (auto& buffer : m_vertex_buffers)
	{
		buffer.Release();
	}
	m_index_buffer.Release();
}

bool DiligentRigidMesh::Load(ILTStream& file, LTB_Header& header)
{
	if (header.m_iFileType != LTB_D3D_MODEL_FILE)
	{
		return false;
	}
	if (header.m_iVersion != CD3D_LTB_LOAD_VERSION)
	{
		return false;
	}

	FreeAll();

	uint32 obj_size = 0;
	file.Read(&obj_size, sizeof(obj_size));
	static_cast<void>(obj_size);
	file.Read(&m_vertex_count, sizeof(m_vertex_count));
	file.Read(&m_poly_count, sizeof(m_poly_count));
	uint32 max_bones_per_vert = 0;
	uint32 max_bones_per_tri = 0;
	file.Read(&max_bones_per_tri, sizeof(max_bones_per_tri));
	file.Read(&max_bones_per_vert, sizeof(max_bones_per_vert));
	file.Read(&m_vert_stream_flags[0], sizeof(uint32) * 4);
	file.Read(&m_bone_effector, sizeof(m_bone_effector));

	if (max_bones_per_tri != 1 || max_bones_per_vert != 1)
	{
		return false;
	}

	if (!diligent_build_mesh_layout(m_vert_stream_flags, eNO_WORLD_BLENDS, m_layout, m_non_fixed_pipe_data))
	{
		return false;
	}

	for (uint32 i = 0; i < 4; ++i)
	{
		if (!m_vert_stream_flags[i])
		{
			continue;
		}

		const uint32 stride = m_layout.strides[i];
		if (stride == 0 || m_vertex_count == 0)
		{
			return false;
		}

		const uint32 size = stride * m_vertex_count;
		m_vertex_data[i].resize(size);
		file.Read(m_vertex_data[i].data(), size);
	}

	const uint32 index_count = m_poly_count * 3;
	m_index_data.resize(index_count);
	if (index_count > 0)
	{
		file.Read(m_index_data.data(), sizeof(uint16) * index_count);
	}

	ReCreateObject();
	return true;
}

void DiligentRigidMesh::ReCreateObject()
{
	FreeDeviceObjects();
	if (!g_diligent_state.render_device)
	{
		return;
	}

	for (uint32 i = 0; i < 4; ++i)
	{
		if (m_vertex_data[i].empty())
		{
			continue;
		}

		const uint32 stride = m_layout.strides[i];
		if (stride == 0)
		{
			continue;
		}

		m_vertex_buffers[i] = diligent_create_buffer(
			static_cast<uint32>(m_vertex_data[i].size()),
			Diligent::BIND_VERTEX_BUFFER,
			m_vertex_data[i].data(),
			stride);
	}

	if (!m_index_data.empty())
	{
		m_index_buffer = diligent_create_buffer(
			static_cast<uint32>(m_index_data.size() * sizeof(uint16)),
			Diligent::BIND_INDEX_BUFFER,
			m_index_data.data(),
			sizeof(uint16));
	}
}

DiligentSkelMesh::DiligentSkelMesh()
{
	Reset();
}

DiligentSkelMesh::~DiligentSkelMesh()
{
	FreeAll();
}

void DiligentSkelMesh::Reset()
{
	m_render_method = kRenderDirect;
	m_vertex_count = 0;
	m_poly_count = 0;
	m_max_bones_per_vert = 0;
	m_max_bones_per_tri = 0;
	m_min_bone = 0;
	m_max_bone = 0;
	m_weight_count = 0;
	m_vert_type = eNO_WORLD_BLENDS;
	m_non_fixed_pipe_data = false;
	m_reindexed_bones = false;
	m_reindexed_bone_list.clear();
	m_bone_sets.clear();
	m_index_data.clear();
	m_layout.elements.clear();
	m_layout.strides.fill(0);
	m_layout.uv_attrib = { -1, -1, -1, -1 };
	m_layout.position_attrib = -1;
	m_layout.weights_attrib = -1;
	m_layout.indices_attrib = -1;
	m_layout.normal_attrib = -1;
	m_layout.color_attrib = -1;
	m_layout.hash = 0;
	m_position_ref = {};
	m_normal_ref = {};
	m_weights_ref = {};
	m_indices_ref = {};
	m_dynamic_streams.fill(false);
	m_bone_transforms.clear();
	for (uint32 i = 0; i < 4; ++i)
	{
		m_vert_stream_flags[i] = 0;
		m_vertex_data[i].clear();
		m_vertex_buffers[i].Release();
	}
	m_index_buffer.Release();
}

void DiligentSkelMesh::FreeAll()
{
	FreeDeviceObjects();
	Reset();
}

void DiligentSkelMesh::FreeDeviceObjects()
{
	for (auto& buffer : m_vertex_buffers)
	{
		buffer.Release();
	}
	m_index_buffer.Release();
}

void DiligentSkelMesh::UpdateLayoutRefs()
{
	m_position_ref = {};
	m_normal_ref = {};
	m_weights_ref = {};
	m_indices_ref = {};
	static_cast<void>(diligent_get_layout_element_ref(m_layout, m_layout.position_attrib, m_position_ref));
	static_cast<void>(diligent_get_layout_element_ref(m_layout, m_layout.normal_attrib, m_normal_ref));
	static_cast<void>(diligent_get_layout_element_ref(m_layout, m_layout.weights_attrib, m_weights_ref));
	static_cast<void>(diligent_get_layout_element_ref(m_layout, m_layout.indices_attrib, m_indices_ref));

	m_dynamic_streams.fill(false);
	if (m_position_ref.valid)
	{
		m_dynamic_streams[m_position_ref.stream_index] = true;
	}
	if (m_normal_ref.valid)
	{
		m_dynamic_streams[m_normal_ref.stream_index] = true;
	}
	if (m_weights_ref.valid)
	{
		m_dynamic_streams[m_weights_ref.stream_index] = true;
	}
	if (m_indices_ref.valid)
	{
		m_dynamic_streams[m_indices_ref.stream_index] = true;
	}
}

bool DiligentSkelMesh::Load(ILTStream& file, LTB_Header& header)
{
	if (header.m_iFileType != LTB_D3D_MODEL_FILE)
	{
		return false;
	}
	if (header.m_iVersion != CD3D_LTB_LOAD_VERSION)
	{
		return false;
	}

	FreeAll();

	uint32 obj_size = 0;
	file.Read(&obj_size, sizeof(obj_size));
	static_cast<void>(obj_size);
	file.Read(&m_vertex_count, sizeof(m_vertex_count));
	file.Read(&m_poly_count, sizeof(m_poly_count));
	file.Read(&m_max_bones_per_tri, sizeof(m_max_bones_per_tri));
	file.Read(&m_max_bones_per_vert, sizeof(m_max_bones_per_vert));
	file.Read(&m_vert_stream_flags[0], sizeof(uint32) * 4);
	file.Read(&m_min_bone, sizeof(m_min_bone));
	file.Read(&m_max_bone, sizeof(m_max_bone));
	file.Read(&m_weight_count, sizeof(m_weight_count));
	file.Read(&m_vert_type, sizeof(m_vert_type));
	file.Read(&m_render_method, sizeof(m_render_method));
	file.Read(&m_reindexed_bones, sizeof(m_reindexed_bones));

	if (m_max_bones_per_vert == 0)
	{
		return false;
	}

	if (!diligent_build_mesh_layout(m_vert_stream_flags, m_vert_type, m_layout, m_non_fixed_pipe_data))
	{
		return false;
	}

	for (uint32 i = 0; i < 4; ++i)
	{
		if (!m_vert_stream_flags[i])
		{
			continue;
		}

		const uint32 stride = m_layout.strides[i];
		if (stride == 0 || m_vertex_count == 0)
		{
			return false;
		}

		const uint32 size = stride * m_vertex_count;
		m_vertex_data[i].resize(size);
		file.Read(m_vertex_data[i].data(), size);
	}

	const uint32 index_count = m_poly_count * 3;
	m_index_data.resize(index_count);
	if (index_count > 0)
	{
		file.Read(m_index_data.data(), sizeof(uint16) * index_count);
	}

	if (m_render_method == kRenderMatrixPalettes)
	{
		if (!LoadMatrixPalettes(file))
		{
			return false;
		}
	}
	else
	{
		if (!LoadDirect(file))
		{
			return false;
		}
	}

	UpdateLayoutRefs();
	ReCreateObject();
	return true;
}

bool DiligentSkelMesh::LoadDirect(ILTStream& file)
{
	uint32 size = 0;
	file.Read(&size, sizeof(size));
	if (size == 0)
	{
		return true;
	}

	m_bone_transforms.resize(size);
	file.Read(m_bone_transforms.data(), size * sizeof(LTMatrix));
	return true;
}

bool DiligentSkelMesh::LoadMatrixPalettes(ILTStream& file)
{
	uint32 count = 0;
	file.Read(&count, sizeof(count));
	if (count == 0)
	{
		return true;
	}

	m_bone_sets.resize(count);
	file.Read(m_bone_sets.data(), sizeof(DiligentBoneSet) * count);
	if (m_reindexed_bones)
	{
		uint32 bone_count = 0;
		file.Read(&bone_count, sizeof(bone_count));
		if (bone_count > 0)
		{
			m_reindexed_bone_list.resize(bone_count);
			file.Read(m_reindexed_bone_list.data(), sizeof(uint32) * bone_count);
		}
	}

	return true;
}

void DiligentSkelMesh::ReCreateObject()
{
	FreeDeviceObjects();
	if (!g_diligent_state.render_device)
	{
		return;
	}

	for (uint32 i = 0; i < 4; ++i)
	{
		if (m_vertex_data[i].empty())
		{
			continue;
		}

		const uint32 stride = m_layout.strides[i];
		if (stride == 0)
		{
			continue;
		}

		const Diligent::BIND_FLAGS bind_flags = m_dynamic_streams[i]
			? Diligent::BIND_VERTEX_BUFFER
			: Diligent::BIND_VERTEX_BUFFER;

		const Diligent::USAGE usage = m_dynamic_streams[i]
			? Diligent::USAGE_DYNAMIC
			: Diligent::USAGE_DEFAULT;

		const Diligent::CPU_ACCESS_FLAGS access_flags = m_dynamic_streams[i]
			? Diligent::CPU_ACCESS_WRITE
			: Diligent::CPU_ACCESS_NONE;

		Diligent::BufferDesc desc;
		desc.Name = "ltjs_skel_vertices";
		desc.Size = static_cast<uint32>(m_vertex_data[i].size());
		desc.BindFlags = bind_flags;
		desc.Usage = usage;
		desc.CPUAccessFlags = access_flags;
		desc.ElementByteStride = stride;

		if (m_dynamic_streams[i])
		{
			g_diligent_state.render_device->CreateBuffer(desc, nullptr, &m_vertex_buffers[i]);
		}
		else
		{
			Diligent::BufferData init_data;
			init_data.pData = m_vertex_data[i].data();
			init_data.DataSize = desc.Size;
			g_diligent_state.render_device->CreateBuffer(desc, &init_data, &m_vertex_buffers[i]);
		}
	}

	if (!m_index_data.empty())
	{
		m_index_buffer = diligent_create_buffer(
			static_cast<uint32>(m_index_data.size() * sizeof(uint16)),
			Diligent::BIND_INDEX_BUFFER,
			m_index_data.data(),
			sizeof(uint16));
	}
}

bool diligent_get_model_transform_raw(ModelInstance* instance, uint32 bone_index, LTMatrix& transform)
{
	if (!instance)
	{
		transform.Identity();
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model || bone_index >= model->NumNodes())
	{
		transform.Identity();
		return false;
	}

	if (!instance->GetCachedTransform(bone_index, transform))
	{
		transform.Identity();
		return false;
	}

	return true;
}

bool diligent_get_model_transform_render(ModelInstance* instance, uint32 bone_index, LTMatrix& transform)
{
	if (!diligent_get_model_transform_raw(instance, bone_index, transform))
	{
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		transform.Identity();
		return false;
	}

	// Match engine rendering transforms: remove bind-pose global transform.
	transform = transform * model->GetNode(bone_index)->GetInvGlobalTransform();
	return true;
}

bool DiligentSkelMesh::UpdateSkinnedVertices(ModelInstance* instance)
{
	if (!instance)
	{
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return false;
	}

	if (m_render_method == kRenderDirect)
	{
		if (!m_bone_transforms.empty())
		{
			return UpdateSkinnedVerticesDirect(m_bone_transforms);
		}

		return false;
	}

	const uint32 bone_count = model->NumNodes();
	if (bone_count == 0)
	{
		return false;
	}

	if (m_bone_transforms.size() != bone_count)
	{
		m_bone_transforms.resize(bone_count);
	}

	for (uint32 bone_index = 0; bone_index < bone_count; ++bone_index)
	{
		LTMatrix transform;
		if (!diligent_get_model_transform_render(instance, bone_index, transform))
		{
			return false;
		}
		m_bone_transforms[bone_index] = transform;
	}

	return UpdateSkinnedVerticesIndexed(m_bone_transforms);
}

bool DiligentSkelMesh::UpdateSkinnedVerticesDirect(const std::vector<LTMatrix>& bone_transforms)
{
	if (!g_diligent_state.immediate_context)
	{
		return false;
	}

	if (!m_position_ref.valid)
	{
		return false;
	}

	const uint32 position_stream = m_position_ref.stream_index;
	if (!m_vertex_buffers[position_stream] || m_vertex_data[position_stream].empty())
	{
		return false;
	}

	const uint32 vertex_count = m_vertex_count;
	const uint32 weight_count = diligent_get_blend_weight_count(m_vert_type);
	if (weight_count == 0)
	{
		return false;
	}

	const uint32 weights_stream = m_weights_ref.stream_index;
	const uint32 indices_stream = m_indices_ref.stream_index;
	if (!m_vertex_buffers[weights_stream] || !m_vertex_buffers[indices_stream])
	{
		return false;
	}

	void* mapped_position = nullptr;
	g_diligent_state.immediate_context->MapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_position);
	if (!mapped_position)
	{
		return false;
	}

	const uint8* weights = m_vertex_data[weights_stream].data();
	const uint8* indices = m_vertex_data[indices_stream].data();

	const uint32 position_stride = m_position_ref.stride;
	const uint32 weights_stride = m_weights_ref.stride;
	const uint32 indices_stride = m_indices_ref.stride;

	for (uint32 vert = 0; vert < vertex_count; ++vert)
	{
		const uint8* weights_ptr = weights + weights_stride * vert + m_weights_ref.offset;
		const uint8* indices_ptr = indices + indices_stride * vert + m_indices_ref.offset;
		float* out_position = reinterpret_cast<float*>(static_cast<uint8*>(mapped_position) + position_stride * vert + m_position_ref.offset);

		LTVector final_pos(0.0f, 0.0f, 0.0f);
		for (uint32 weight_index = 0; weight_index < weight_count; ++weight_index)
		{
			const uint8 bone_index = indices_ptr[weight_index];
			if (bone_index >= bone_transforms.size())
			{
				continue;
			}

			const float weight = weights_ptr[weight_index] / 255.0f;
			const LTMatrix& transform = bone_transforms[bone_index];
			const LTVector position(out_position[0], out_position[1], out_position[2]);
			LTVector transformed = diligent_transform_point(transform, position);
			final_pos += transformed * weight;
		}

		out_position[0] = final_pos.x;
		out_position[1] = final_pos.y;
		out_position[2] = final_pos.z;
	}

	g_diligent_state.immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
	return true;
}

bool DiligentSkelMesh::UpdateSkinnedVerticesIndexed(const std::vector<LTMatrix>& bone_transforms)
{
	if (!g_diligent_state.immediate_context)
	{
		return false;
	}

	if (!m_position_ref.valid)
	{
		return false;
	}

	const uint32 position_stream = m_position_ref.stream_index;
	if (!m_vertex_buffers[position_stream] || m_vertex_data[position_stream].empty())
	{
		return false;
	}

	const uint32 vertex_count = m_vertex_count;
	const uint32 weight_count = diligent_get_blend_weight_count(m_vert_type);
	if (weight_count == 0)
	{
		return false;
	}

	const uint32 weights_stream = m_weights_ref.stream_index;
	const uint32 indices_stream = m_indices_ref.stream_index;
	if (!m_vertex_buffers[weights_stream] || !m_vertex_buffers[indices_stream])
	{
		return false;
	}

	void* mapped_position = nullptr;
	g_diligent_state.immediate_context->MapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_position);
	if (!mapped_position)
	{
		return false;
	}

	const uint8* weights = m_vertex_data[weights_stream].data();
	const uint8* indices = m_vertex_data[indices_stream].data();

	const uint32 position_stride = m_position_ref.stride;
	const uint32 weights_stride = m_weights_ref.stride;
	const uint32 indices_stride = m_indices_ref.stride;

	for (uint32 set_index = 0; set_index < m_bone_sets.size(); ++set_index)
	{
		const DiligentBoneSet& bone_set = m_bone_sets[set_index];
		const uint32 vert_end = bone_set.first_vert_index + bone_set.vert_count;

		for (uint32 vert = bone_set.first_vert_index; vert < vert_end && vert < vertex_count; ++vert)
		{
			const uint8* weights_ptr = weights + weights_stride * vert + m_weights_ref.offset;
			const uint8* indices_ptr = indices + indices_stride * vert + m_indices_ref.offset;
			float* out_position = reinterpret_cast<float*>(static_cast<uint8*>(mapped_position) + position_stride * vert + m_position_ref.offset);

			LTVector final_pos(0.0f, 0.0f, 0.0f);
			for (uint32 weight_index = 0; weight_index < weight_count; ++weight_index)
			{
				const uint8 index = indices_ptr[weight_index];
				if (index >= 4)
				{
					continue;
				}

				const uint8 bone_index = bone_set.bone_set[index];
				if (bone_index >= bone_transforms.size())
				{
					continue;
				}

				const float weight = weights_ptr[weight_index] / 255.0f;
				const LTMatrix& transform = bone_transforms[bone_index];
				const LTVector position(out_position[0], out_position[1], out_position[2]);
				LTVector transformed = diligent_transform_point(transform, position);
				final_pos += transformed * weight;
			}

			out_position[0] = final_pos.x;
			out_position[1] = final_pos.y;
			out_position[2] = final_pos.z;
		}
	}

	g_diligent_state.immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
	return true;
}

DiligentVAMesh::DiligentVAMesh()
{
	Reset();
}

DiligentVAMesh::~DiligentVAMesh()
{
	FreeAll();
}

void DiligentVAMesh::Reset()
{
	m_vertex_count = 0;
	m_undup_vertex_count = 0;
	m_poly_count = 0;
	m_bone_effector = 0;
	m_anim_node_idx = 0;
	m_max_bones_per_vert = 0;
	m_max_bones_per_tri = 0;
	std::fill(std::begin(m_vert_stream_flags), std::end(m_vert_stream_flags), 0u);
	m_non_fixed_pipe_data = false;
	m_dup_map_list.clear();
	m_layout.elements.clear();
	m_layout.strides.fill(0);
	m_layout.uv_attrib = { -1, -1, -1, -1 };
	m_layout.position_attrib = -1;
	m_layout.weights_attrib = -1;
	m_layout.indices_attrib = -1;
	m_layout.normal_attrib = -1;
	m_layout.color_attrib = -1;
	m_layout.hash = 0;
	m_position_ref = {};
	m_dynamic_streams.fill(false);
	m_vertex_data.fill({});
	m_index_data.clear();
	for (auto& buffer : m_vertex_buffers)
	{
		buffer.Release();
	}
	m_index_buffer.Release();
}

void DiligentVAMesh::FreeAll()
{
	FreeDeviceObjects();
	Reset();
}

void DiligentVAMesh::FreeDeviceObjects()
{
	for (auto& buffer : m_vertex_buffers)
	{
		buffer.Release();
	}
	m_index_buffer.Release();
}

void DiligentVAMesh::UpdateLayoutRefs()
{
	m_position_ref = {};
	static_cast<void>(diligent_get_layout_element_ref(m_layout, m_layout.position_attrib, m_position_ref));

	m_dynamic_streams.fill(false);
	if (m_position_ref.valid)
	{
		m_dynamic_streams[m_position_ref.stream_index] = true;
	}
}

bool DiligentVAMesh::Load(ILTStream& file, LTB_Header& header)
{
	if (header.m_iFileType != LTB_D3D_MODEL_FILE)
	{
		return false;
	}
	if (header.m_iVersion != CD3D_LTB_LOAD_VERSION)
	{
		return false;
	}

	FreeAll();

	uint32 obj_size = 0;
	file.Read(&obj_size, sizeof(obj_size));
	static_cast<void>(obj_size);
	file.Read(&m_vertex_count, sizeof(m_vertex_count));
	file.Read(&m_poly_count, sizeof(m_poly_count));
	file.Read(&m_max_bones_per_tri, sizeof(m_max_bones_per_tri));
	file.Read(&m_max_bones_per_vert, sizeof(m_max_bones_per_vert));
	file.Read(&m_vert_stream_flags[0], sizeof(uint32) * 4);
	file.Read(&m_bone_effector, sizeof(m_bone_effector));
	file.Read(&m_anim_node_idx, sizeof(m_anim_node_idx));

	if (m_max_bones_per_vert != 1 || m_max_bones_per_tri != 1)
	{
		return false;
	}

	if (!diligent_build_mesh_layout(m_vert_stream_flags, eNO_WORLD_BLENDS, m_layout, m_non_fixed_pipe_data))
	{
		return false;
	}

	for (uint32 i = 0; i < 4; ++i)
	{
		if (!m_vert_stream_flags[i])
		{
			continue;
		}

		const uint32 stride = m_layout.strides[i];
		if (stride == 0 || m_vertex_count == 0)
		{
			return false;
		}

		const uint32 size = stride * m_vertex_count;
		m_vertex_data[i].resize(size);
		file.Read(m_vertex_data[i].data(), size);
	}

	const uint32 index_count = m_poly_count * 3;
	m_index_data.resize(index_count);
	if (index_count > 0)
	{
		file.Read(m_index_data.data(), sizeof(uint16) * index_count);
	}

	uint32 dup_count = 0;
	file.Read(&dup_count, sizeof(dup_count));
	if (dup_count > 0)
	{
		m_dup_map_list.resize(dup_count);
		file.Read(m_dup_map_list.data(), sizeof(DiligentDupMap) * dup_count);
	}

	UpdateLayoutRefs();
	ReCreateObject();
	return true;
}

void DiligentVAMesh::ReCreateObject()
{
	FreeDeviceObjects();
	if (!g_diligent_state.render_device)
	{
		return;
	}

	for (uint32 i = 0; i < 4; ++i)
	{
		if (m_vertex_data[i].empty())
		{
			continue;
		}

		const uint32 stride = m_layout.strides[i];
		if (stride == 0)
		{
			continue;
		}

		const Diligent::BIND_FLAGS bind_flags = m_dynamic_streams[i]
			? Diligent::BIND_VERTEX_BUFFER
			: Diligent::BIND_VERTEX_BUFFER;

		const Diligent::USAGE usage = m_dynamic_streams[i]
			? Diligent::USAGE_DYNAMIC
			: Diligent::USAGE_DEFAULT;

		const Diligent::CPU_ACCESS_FLAGS access_flags = m_dynamic_streams[i]
			? Diligent::CPU_ACCESS_WRITE
			: Diligent::CPU_ACCESS_NONE;

		Diligent::BufferDesc desc;
		desc.Name = "ltjs_va_vertices";
		desc.Size = static_cast<uint32>(m_vertex_data[i].size());
		desc.BindFlags = bind_flags;
		desc.Usage = usage;
		desc.CPUAccessFlags = access_flags;
		desc.ElementByteStride = stride;

		if (m_dynamic_streams[i])
		{
			g_diligent_state.render_device->CreateBuffer(desc, nullptr, &m_vertex_buffers[i]);
		}
		else
		{
			Diligent::BufferData init_data;
			init_data.pData = m_vertex_data[i].data();
			init_data.DataSize = desc.Size;
			g_diligent_state.render_device->CreateBuffer(desc, &init_data, &m_vertex_buffers[i]);
		}
	}

	if (!m_index_data.empty())
	{
		m_index_buffer = diligent_create_buffer(
			static_cast<uint32>(m_index_data.size() * sizeof(uint16)),
			Diligent::BIND_INDEX_BUFFER,
			m_index_data.data(),
			sizeof(uint16));
	}
}

bool DiligentVAMesh::UpdateVA(Model* model, AnimTimeRef* anim_time)
{
	if (!model || !anim_time || !g_diligent_state.immediate_context)
	{
		return false;
	}

	if (!m_position_ref.valid)
	{
		return false;
	}

	const uint32 position_stream = m_position_ref.stream_index;
	if (!m_vertex_buffers[position_stream] || m_vertex_data[position_stream].empty())
	{
		return false;
	}

	ModelAnim* anims[2] = {
		model->GetAnim(anim_time->m_Prev.m_iAnim),
		model->GetAnim(anim_time->m_Cur.m_iAnim)
	};
	if (!anims[0] || !anims[1])
	{
		return false;
	}

	AnimNode* anim_nodes[2] = {
		anims[0]->GetAnimNode(m_anim_node_idx),
		anims[1]->GetAnimNode(m_anim_node_idx)
	};
	if (!anim_nodes[0] || !anim_nodes[1])
	{
		return false;
	}

	CDefVertexLst* def_verts[2] = {
		anim_nodes[0]->GetVertexData(anim_time->m_Prev.m_iFrame),
		anim_nodes[1]->GetVertexData(anim_time->m_Cur.m_iFrame)
	};
	if (!def_verts[0])
	{
		return false;
	}
	if (!def_verts[1])
	{
		def_verts[1] = def_verts[0];
	}

	void* mapped_position = nullptr;
	g_diligent_state.immediate_context->MapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_position);
	if (!mapped_position)
	{
		return false;
	}
	std::memcpy(mapped_position, m_vertex_data[position_stream].data(), m_vertex_data[position_stream].size());

	const float percent = anim_time->m_Percent;
	const uint32 vertex_limit = LTMIN(m_undup_vertex_count, m_vertex_count);
	for (uint32 vertex = 0; vertex < vertex_limit; ++vertex)
	{
		const float* prev_val = def_verts[0]->getValue(vertex);
		const float* cur_val = def_verts[1]->getValue(vertex);

		float value[3];
		value[0] = prev_val[0] + ((cur_val[0] - prev_val[0]) * percent);
		value[1] = prev_val[1] + ((cur_val[1] - prev_val[1]) * percent);
		value[2] = prev_val[2] + ((cur_val[2] - prev_val[2]) * percent);

		float* out_position = reinterpret_cast<float*>(static_cast<uint8*>(mapped_position) + vertex * m_position_ref.stride + m_position_ref.offset);
		out_position[0] = value[0];
		out_position[1] = value[1];
		out_position[2] = value[2];
	}

	for (const auto& dup : m_dup_map_list)
	{
		if (dup.src_vert >= m_vertex_count || dup.dst_vert >= m_vertex_count)
		{
			continue;
		}

		float* src_position = reinterpret_cast<float*>(static_cast<uint8*>(mapped_position) + dup.src_vert * m_position_ref.stride + m_position_ref.offset);
		float* dst_position = reinterpret_cast<float*>(static_cast<uint8*>(mapped_position) + dup.dst_vert * m_position_ref.stride + m_position_ref.offset);
		dst_position[0] = src_position[0];
		dst_position[1] = src_position[1];
		dst_position[2] = src_position[2];
	}

	g_diligent_state.immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
	return true;
}

bool diligent_get_model_piece_textures(
	ModelInstance* instance,
	ModelPiece* piece,
	std::array<SharedTexture*, MAX_PIECE_TEXTURES>& textures)
{
	textures.fill(nullptr);
	if (!instance || !piece)
	{
		return false;
	}

	uint32 texture_count = 0;
	for (; texture_count < piece->m_nNumTextures && texture_count < MAX_PIECE_TEXTURES; ++texture_count)
	{
		const int32 texture_index = piece->m_iTextures[texture_count];
		if (texture_index >= 0 && texture_index < MAX_MODEL_TEXTURES)
		{
			textures[texture_count] = instance->m_pSkins[texture_index];
		}
	}

	return true;
}

bool diligent_get_model_hook_data(ModelInstance* instance, ModelHookData& hook_data)
{
	if (!instance || !g_diligent_state.scene_desc)
	{
		return false;
	}

	hook_data.m_ObjectFlags = instance->m_Flags;
	hook_data.m_HookFlags = MHF_USETEXTURE;
	hook_data.m_hObject = reinterpret_cast<HLOCALOBJ>(instance);
	hook_data.m_LightAdd = g_diligent_state.scene_desc->m_GlobalModelLightAdd;
	hook_data.m_ObjectColor = {
		static_cast<float>(instance->m_ColorR),
		static_cast<float>(instance->m_ColorG),
		static_cast<float>(instance->m_ColorB)};

	if (g_diligent_state.scene_desc->m_ModelHookFn)
	{
		g_diligent_state.scene_desc->m_ModelHookFn(&hook_data, g_diligent_state.scene_desc->m_ModelHookUser);
	}

	if (g_CV_Saturate.m_Val)
	{
		hook_data.m_ObjectColor *= g_CV_ModelSaturation.m_Val;
	}

	return true;
}

void diligent_debug_add_model_skeleton(ModelInstance* instance, Model* model, const LTRGBColor& color)
{
	if (!instance || !model)
	{
		return;
	}

	const uint32 node_count = model->NumNodes();
	for (uint32 node_index = 0; node_index < node_count; ++node_index)
	{
		ModelNode* node = model->GetNode(node_index);
		if (!node)
		{
			continue;
		}
		const uint32 parent_index = node->GetParentNodeIndex();
		if (parent_index == NODEPARENT_NONE || parent_index >= node_count)
		{
			continue;
		}

		LTMatrix node_transform;
		LTMatrix parent_transform;
		if (!diligent_get_model_transform_raw(instance, node_index, node_transform) ||
			!diligent_get_model_transform_raw(instance, parent_index, parent_transform))
		{
			continue;
		}

		const LTVector node_pos(node_transform.m[0][3], node_transform.m[1][3], node_transform.m[2][3]);
		const LTVector parent_pos(parent_transform.m[0][3], parent_transform.m[1][3], parent_transform.m[2][3]);
		diligent_debug_add_line(parent_pos, node_pos, color);
	}
}

void diligent_debug_add_model_boxes(ModelInstance* instance)
{
	if (!instance)
	{
		return;
	}

	const LTRGBColor box_color = diligent_make_debug_color(255, 64, 64);
	diligent_debug_add_aabb(instance->m_Pos, instance->m_Dims, box_color);
}

void diligent_debug_add_model_obbs(ModelInstance* instance)
{
#if(MODEL_OBB)
	if (!instance)
	{
		return;
	}

	if (!instance->IsCollisionObjectsEnabled())
	{
		instance->EnableCollisionObjects();
	}

	const uint32 count = instance->NumCollisionObjects();
	if (count == 0)
	{
		return;
	}

	const LTRGBColor obb_color = diligent_make_debug_color(64, 255, 64);
	for (uint32 i = 0; i < count; ++i)
	{
		const ModelOBB* obb = instance->GetCollisionObject(i);
		if (!obb)
		{
			continue;
		}
		diligent_debug_add_obb(*obb, obb_color);
	}
#else
	static_cast<void>(instance);
#endif
}

void diligent_debug_add_model_vertex_normals(ModelInstance* instance, CDIModelDrawable* drawable)
{
	if (!instance || !drawable)
	{
		return;
	}

	const LTRGBColor color = diligent_make_debug_color(255, 0, 255);
	if (drawable->GetType() == CRenderObject::eRigidMesh)
	{
		auto* mesh = static_cast<DiligentRigidMesh*>(drawable);
		LTMatrix model_matrix;
		diligent_get_model_transform_raw(instance, mesh->GetBoneEffector(), model_matrix);
		diligent_debug_add_vertex_normals(
			mesh->GetLayout(),
			mesh->GetVertexData(),
			mesh->GetVertexCount(),
			model_matrix,
			color);
	}
	else if (drawable->GetType() == CRenderObject::eSkelMesh)
	{
		auto* mesh = static_cast<DiligentSkelMesh*>(drawable);
		LTMatrix model_matrix;
		model_matrix.Identity();
		diligent_debug_add_vertex_normals(
			mesh->GetLayout(),
			mesh->GetVertexData(),
			mesh->GetVertexCount(),
			model_matrix,
			color);
	}
	else if (drawable->GetType() == CRenderObject::eVAMesh)
	{
		auto* mesh = static_cast<DiligentVAMesh*>(drawable);
		LTMatrix model_matrix;
		diligent_get_model_transform_raw(instance, mesh->GetBoneEffector(), model_matrix);
		diligent_debug_add_vertex_normals(
			mesh->GetLayout(),
			mesh->GetVertexData(),
			mesh->GetVertexCount(),
			model_matrix,
			color);
	}
}

void diligent_debug_add_model_touching_lights(ModelInstance* instance)
{
	if (!instance)
	{
		return;
	}

	const float radius = instance->m_Dims.Mag();
	for (uint32 i = 0; i < g_diligent_num_world_dynamic_lights; ++i)
	{
		const DynamicLight* light = g_diligent_world_dynamic_lights[i];
		if (!light)
		{
			continue;
		}

		const LTVector delta = instance->m_Pos - light->m_Pos;
		if (delta.Mag() <= (light->m_LightRadius + radius))
		{
			const LTRGBColor touch_color = diligent_make_debug_color(255, 255, 64);
			diligent_debug_add_aabb(instance->m_Pos, instance->m_Dims, touch_color);
			break;
		}
	}
}

void diligent_debug_add_model_info(ModelInstance* instance)
{
	if (!instance || !diligent_model_debug_enabled())
	{
		return;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return;
	}

	if (g_CV_ModelDebug_DrawBoxes.m_Val)
	{
		diligent_debug_add_model_boxes(instance);
	}
	if (g_CV_ModelDebug_DrawOBBS.m_Val)
	{
		diligent_debug_add_model_obbs(instance);
	}
	if (g_CV_ModelDebug_DrawSkeleton.m_Val)
	{
		const LTRGBColor skel_color = diligent_make_debug_color(64, 192, 255);
		diligent_debug_add_model_skeleton(instance, model, skel_color);
	}
	if (g_CV_ModelDebug_DrawTouchingLights.m_Val)
	{
		diligent_debug_add_model_touching_lights(instance);
	}
}

bool diligent_update_model_constants(ModelInstance* instance, const LTMatrix& mvp, const LTMatrix& model_matrix)
{
	if (!instance || !g_diligent_state.immediate_context || !g_model_resources.constant_buffer)
	{
		return false;
	}

	bool fog_enabled = false;
	float fog_color_r = 0.0f;
	float fog_color_g = 0.0f;
	float fog_color_b = 0.0f;
	float fog_near = 0.0f;
	float fog_far = 0.0f;
	if (g_diligent_shadow_mode)
	{
		fog_enabled = false;
	}
	else if (g_diligent_state.glow_mode)
	{
		fog_enabled = g_CV_ScreenGlowFogEnable.m_Val != 0;
		fog_near = g_CV_ScreenGlowFogNearZ.m_Val;
		fog_far = g_CV_ScreenGlowFogFarZ.m_Val;
	}
	else
	{
		fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((instance->m_Flags & FLAG_FOGDISABLE) == 0);
		fog_color_r = static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f;
		fog_color_g = static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f;
		fog_color_b = static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f;
		fog_near = g_CV_FogNearZ.m_Val;
		fog_far = g_CV_FogFarZ.m_Val;
	}

	DiligentModelConstants constants;
	diligent_store_matrix_from_lt(mvp, constants.mvp);
	diligent_store_matrix_from_lt(model_matrix, constants.model);
	LTVector object_color(
		static_cast<float>(instance->m_ColorR),
		static_cast<float>(instance->m_ColorG),
		static_cast<float>(instance->m_ColorB));
	LTVector light_add(0.0f, 0.0f, 0.0f);
	ModelHookData hook_data{};
	if (diligent_get_model_hook_data(instance, hook_data))
	{
		object_color = hook_data.m_ObjectColor;
		light_add = hook_data.m_LightAdd;
	}

	if (!g_CV_LightModels.m_Val)
	{
		object_color.Init(255.0f, 255.0f, 255.0f);
		light_add.Init(0.0f, 0.0f, 0.0f);
	}

	LTVector final_color = object_color + light_add;
	final_color.x = LTCLAMP(final_color.x, 0.0f, 255.0f);
	final_color.y = LTCLAMP(final_color.y, 0.0f, 255.0f);
	final_color.z = LTCLAMP(final_color.z, 0.0f, 255.0f);
	constants.color[0] = final_color.x / 255.0f;
	constants.color[1] = final_color.y / 255.0f;
	constants.color[2] = final_color.z / 255.0f;
	constants.color[3] = static_cast<float>(instance->m_ColorA) / 255.0f;
	constants.model_params[0] = g_CV_LightModels.m_Val != 0 ? 1.0f : 0.0f;
	constants.model_params[1] = g_CV_ModelApplyAmbient.m_Val != 0 ? 1.0f : 0.0f;
	constants.model_params[2] = g_CV_ModelApplySun.m_Val != 0 ? 1.0f : 0.0f;
	constants.model_params[3] = 0.0f;
	constants.camera_pos[0] = g_diligent_state.view_params.m_Pos.x;
	constants.camera_pos[1] = g_diligent_state.view_params.m_Pos.y;
	constants.camera_pos[2] = g_diligent_state.view_params.m_Pos.z;
	constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
	constants.fog_color[0] = fog_color_r;
	constants.fog_color[1] = fog_color_g;
	constants.fog_color[2] = fog_color_b;
	constants.fog_color[3] = fog_near;
	constants.fog_params[0] = fog_far;
	constants.fog_params[1] = diligent_get_tonemap_enabled();
	const float output_is_srgb = (!g_diligent_state.glow_mode && !g_diligent_shadow_mode)
		? diligent_get_swapchain_output_is_srgb()
		: 0.0f;
	constants.fog_params[2] = output_is_srgb;
	constants.fog_params[3] = 0.0f;

	void* mapped_constants = nullptr;
	g_diligent_state.immediate_context->MapBuffer(g_model_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_diligent_state.immediate_context->UnmapBuffer(g_model_resources.constant_buffer, Diligent::MAP_WRITE);
	return true;
}

bool diligent_draw_mesh_with_pipeline_for_target(
	ModelInstance* instance,
	const DiligentMeshLayout& layout,
	Diligent::IBuffer* const* vertex_buffers,
	Diligent::IBuffer* index_buffer,
	uint32 index_count,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentTextureArray& textures,
	const LTMatrix& mvp,
	const LTMatrix& model_matrix,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format)
{
	if (!instance || !g_diligent_state.immediate_context || !vertex_buffers || !index_buffer)
	{
		return false;
	}

	if (index_count == 0)
	{
		return false;
	}

	if (layout.position_attrib < 0)
	{
		return false;
	}

	const bool uses_texture = (g_CV_ShadersEnabled.m_Val != 0) && (textures[0] != nullptr);
	DiligentModelPipeline* pipeline = diligent_get_model_pipeline_for_target(
		pass,
		shader_pass,
		layout,
		uses_texture,
		color_format,
		depth_format);
	if (!pipeline)
	{
		return false;
	}

	if (!diligent_update_model_constants(instance, mvp, model_matrix))
	{
		return false;
	}

	Diligent::IBuffer* bound_buffers[4] = {};
	Diligent::Uint64 offsets[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		bound_buffers[stream_index] = vertex_buffers[stream_index];
	}

	g_diligent_state.immediate_context->SetVertexBuffers(
		0,
		4,
		bound_buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

	g_diligent_state.immediate_context->SetIndexBuffer(index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (uses_texture)
	{
		auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
		if (texture_var)
		{
			texture_var->Set(diligent_get_texture_view(textures[0], false));
		}
	}

	g_diligent_state.immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_diligent_state.immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs draw_attribs;
	draw_attribs.NumIndices = index_count;
	draw_attribs.IndexType = Diligent::VT_UINT16;
	draw_attribs.FirstIndexLocation = 0;
	draw_attribs.BaseVertex = 0;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_diligent_state.immediate_context->DrawIndexed(draw_attribs);

	return true;
}

bool diligent_draw_mesh_with_pipeline(
	ModelInstance* instance,
	const DiligentMeshLayout& layout,
	Diligent::IBuffer* const* vertex_buffers,
	Diligent::IBuffer* index_buffer,
	uint32 index_count,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentTextureArray& textures,
	const LTMatrix& mvp,
	const LTMatrix& model_matrix)
{
	if (!g_diligent_state.swap_chain)
	{
		return false;
	}

	const auto& swap_desc = g_diligent_state.swap_chain->GetDesc();
	return diligent_draw_mesh_with_pipeline_for_target(
		instance,
		layout,
		vertex_buffers,
		index_buffer,
		index_count,
		pass,
		shader_pass,
		textures,
		mvp,
		model_matrix,
		swap_desc.ColorBufferFormat,
		swap_desc.DepthBufferFormat);
}

bool diligent_draw_rigid_mesh(
	ModelInstance* instance,
	DiligentRigidMesh* mesh,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentTextureArray& textures)
{
	if (!instance || !mesh)
	{
		return false;
	}

	if (mesh->GetIndexCount() == 0 || mesh->GetVertexCount() == 0)
	{
		return false;
	}

	const auto& layout = mesh->GetLayout();
	LTMatrix model_matrix;
	diligent_get_model_transform_raw(instance, mesh->GetBoneEffector(), model_matrix);
	LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * model_matrix;

	Diligent::IBuffer* vertex_buffers[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		vertex_buffers[stream_index] = mesh->GetVertexBuffer(stream_index);
	}

	return diligent_draw_mesh_with_pipeline(
		instance,
		layout,
		vertex_buffers,
		mesh->GetIndexBuffer(),
		mesh->GetIndexCount(),
		pass,
		shader_pass,
		textures,
		mvp,
		model_matrix);
}

bool diligent_draw_skel_mesh(
	ModelInstance* instance,
	DiligentSkelMesh* mesh,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentTextureArray& textures)
{
	if (!instance || !mesh)
	{
		return false;
	}

	if (mesh->GetIndexCount() == 0 || mesh->GetVertexCount() == 0)
	{
		return false;
	}

	const auto& layout = mesh->GetLayout();
	LTMatrix model_matrix;
	model_matrix.Identity();
	LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView;

	Diligent::IBuffer* vertex_buffers[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		vertex_buffers[stream_index] = mesh->GetVertexBuffer(stream_index);
	}

	return diligent_draw_mesh_with_pipeline(
		instance,
		layout,
		vertex_buffers,
		mesh->GetIndexBuffer(),
		mesh->GetIndexCount(),
		pass,
		shader_pass,
		textures,
		mvp,
		model_matrix);
}

bool diligent_draw_va_mesh(
	ModelInstance* instance,
	DiligentVAMesh* mesh,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentTextureArray& textures)
{
	if (!instance || !mesh)
	{
		return false;
	}

	if (mesh->GetIndexCount() == 0 || mesh->GetVertexCount() == 0)
	{
		return false;
	}

	const auto& layout = mesh->GetLayout();
	LTMatrix model_matrix;
	diligent_get_model_transform_raw(instance, mesh->GetBoneEffector(), model_matrix);
	LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * model_matrix;

	Diligent::IBuffer* vertex_buffers[4] = {};
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		vertex_buffers[stream_index] = mesh->GetVertexBuffer(stream_index);
	}

	return diligent_draw_mesh_with_pipeline(
		instance,
		layout,
		vertex_buffers,
		mesh->GetIndexBuffer(),
		mesh->GetIndexCount(),
		pass,
		shader_pass,
		textures,
		mvp,
		model_matrix);
}

bool diligent_draw_model_instance_with_render_style_map(ModelInstance* instance, const CRenderStyleMap* render_style_map)
{
	if (!instance)
	{
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return false;
	}

	bool use_no_glow = false;
	if (render_style_map)
	{
		ModelHookData hook_data{};
		if (diligent_get_model_hook_data(instance, hook_data))
		{
			use_no_glow = (hook_data.m_HookFlags & MHF_NOGLOW) != 0;
		}
	}

	const uint32 piece_count = model->NumPieces();
	for (uint32 piece_index = 0; piece_index < piece_count; ++piece_index)
	{
		if (instance->IsPieceHidden(piece_index))
		{
			continue;
		}

		ModelPiece* piece = model->GetPiece(piece_index);
		if (!piece)
		{
			continue;
		}

		CDIModelDrawable* lod = piece->GetLOD(0);
		if (!lod)
		{
			continue;
		}

		DiligentRigidMesh* rigid_mesh = nullptr;
		DiligentSkelMesh* skel_mesh = nullptr;
		DiligentVAMesh* va_mesh = nullptr;

		switch (lod->GetType())
		{
			case CRenderObject::eRigidMesh:
				rigid_mesh = static_cast<DiligentRigidMesh*>(lod);
				break;
			case CRenderObject::eSkelMesh:
				skel_mesh = static_cast<DiligentSkelMesh*>(lod);
				break;
			case CRenderObject::eVAMesh:
				va_mesh = static_cast<DiligentVAMesh*>(lod);
				break;
			default:
				break;
		}

		if (!rigid_mesh && !skel_mesh && !va_mesh)
		{
			continue;
		}

		if (g_CV_ModelDebug_DrawVertexNormals.m_Val)
		{
			diligent_debug_add_model_vertex_normals(instance, lod);
		}

		CRenderStyle* render_style = nullptr;
		bool use_default_render_style = false;
		if (!instance->GetRenderStyle(piece->m_iRenderStyle, &render_style) || !render_style)
		{
			use_default_render_style = true;
		}

		if (render_style_map)
		{
			CRenderStyle* mapped_style = nullptr;
			if (use_no_glow)
			{
				mapped_style = render_style_map->GetNoGlowRenderStyle();
			}
			else
			{
				mapped_style = render_style_map->MapRenderStyle(render_style);
			}
			if (mapped_style)
			{
				render_style = mapped_style;
			}
		}

		std::array<SharedTexture*, MAX_PIECE_TEXTURES> piece_textures{};
		diligent_get_model_piece_textures(instance, piece, piece_textures);

		const uint32 pass_count = use_default_render_style ? 1u : render_style->GetRenderPassCount();
		if (pass_count == 0)
		{
			continue;
		}

		if (skel_mesh)
		{
			if (!skel_mesh->UpdateSkinnedVertices(instance))
			{
				continue;
			}
		}
		else if (va_mesh)
		{
			if (!va_mesh->UpdateVA(model, &instance->m_AnimTracker.m_TimeRef))
			{
				continue;
			}
		}

		for (uint32 pass_index = 0; pass_index < pass_count; ++pass_index)
		{
			RenderPassOp pass{};
			RSRenderPassShaders shader_pass{};

			if (!use_default_render_style)
			{
				if (!render_style->GetRenderPass(pass_index, &pass))
				{
					continue;
				}
				render_style->GetRenderPassShaders(pass_index, &shader_pass);
			}
			else
			{
				for (auto& stage : pass.TextureStages)
				{
					stage.TextureParam = RENDERSTYLE_NOTEXTURE;
					stage.ColorOp = RENDERSTYLE_COLOROP_DISABLE;
					stage.ColorArg1 = RENDERSTYLE_COLORARG_CURRENT;
					stage.ColorArg2 = RENDERSTYLE_COLORARG_CURRENT;
					stage.AlphaOp = RENDERSTYLE_ALPHAOP_DISABLE;
					stage.AlphaArg1 = RENDERSTYLE_ALPHAARG_CURRENT;
					stage.AlphaArg2 = RENDERSTYLE_ALPHAARG_CURRENT;
					stage.UVSource = RENDERSTYLE_UVFROM_MODELDATA_UVSET1;
					stage.UAddress = RENDERSTYLE_UVADDR_WRAP;
					stage.VAddress = RENDERSTYLE_UVADDR_WRAP;
					stage.TexFilter = RENDERSTYLE_TEXFILTER_BILINEAR;
					stage.UVTransform_Enable = false;
					std::fill(std::begin(stage.UVTransform_Matrix), std::end(stage.UVTransform_Matrix), 0.0f);
					stage.ProjectTexCoord = false;
					stage.TexCoordCount = 2;
				}

				pass.TextureStages[0].TextureParam = RENDERSTYLE_USE_TEXTURE1;
				pass.TextureStages[0].ColorOp = RENDERSTYLE_COLOROP_MODULATE;
				pass.TextureStages[0].ColorArg1 = RENDERSTYLE_COLORARG_TEXTURE;
				pass.TextureStages[0].ColorArg2 = RENDERSTYLE_COLORARG_DIFFUSE;
				pass.TextureStages[0].AlphaOp = RENDERSTYLE_ALPHAOP_MODULATE;
				pass.TextureStages[0].AlphaArg1 = RENDERSTYLE_ALPHAARG_TEXTURE;
				pass.TextureStages[0].AlphaArg2 = RENDERSTYLE_ALPHAARG_DIFFUSE;

				pass.BlendMode = RENDERSTYLE_NOBLEND;
				pass.ZBufferMode = RENDERSTYLE_ZRW;
				pass.CullMode = RENDERSTYLE_CULL_NONE;
				pass.TextureFactor = 0xFFFFFFFF;
				pass.AlphaRef = 0;
				pass.DynamicLight = false;
				pass.ZBufferTestMode = RENDERSTYLE_ALPHATEST_LESSEQUAL;
				pass.AlphaTestMode = RENDERSTYLE_NOALPHATEST;
				pass.FillMode = RENDERSTYLE_FILL;
				pass.bUseBumpEnvMap = false;
				pass.BumpEnvMapStage = 0;
				pass.fBumpEnvMap_Scale = 0.0f;
				pass.fBumpEnvMap_Offset = 0.0f;
			}

			if (g_CV_WireframeModels.m_Val)
			{
				pass.FillMode = RENDERSTYLE_WIRE;
			}
			if (!g_CV_TextureModels.m_Val)
			{
				for (auto& stage : pass.TextureStages)
				{
					stage.TextureParam = RENDERSTYLE_NOTEXTURE;
				}
			}

			DiligentTextureArray textures = diligent_resolve_textures(pass, piece_textures.data());
			if (!g_CV_TextureModels.m_Val)
			{
				textures.fill(nullptr);
			}

			if (rigid_mesh)
			{
				diligent_draw_rigid_mesh(instance, rigid_mesh, pass, shader_pass, textures);
			}
			else if (skel_mesh)
			{
				diligent_draw_skel_mesh(instance, skel_mesh, pass, shader_pass, textures);
			}
			else if (va_mesh)
			{
				diligent_draw_va_mesh(instance, va_mesh, pass, shader_pass, textures);
			}
		}
	}

	return true;
}

bool diligent_draw_model_instance(ModelInstance* instance)
{
	return diligent_draw_model_instance_with_render_style_map(instance, nullptr);
}

bool diligent_draw_model_instance_shadow(
	ModelInstance* instance,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format,
	uint8 shadow_r,
	uint8 shadow_g,
	uint8 shadow_b)
{
	if (!instance)
	{
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return false;
	}

	const uint8 old_r = instance->m_ColorR;
	const uint8 old_g = instance->m_ColorG;
	const uint8 old_b = instance->m_ColorB;
	instance->m_ColorR = shadow_r;
	instance->m_ColorG = shadow_g;
	instance->m_ColorB = shadow_b;

	bool ok = true;
	const uint32 piece_count = model->NumPieces();
	for (uint32 piece_index = 0; piece_index < piece_count; ++piece_index)
	{
		if (instance->IsPieceHidden(piece_index))
		{
			continue;
		}

		ModelPiece* piece = model->GetPiece(piece_index);
		if (!piece)
		{
			continue;
		}

		const int32 lod_bias = g_CV_ModelLODOffset.m_Val;
		const LTVector to_camera = g_diligent_state.view_params.m_Pos - instance->m_Pos;
		const float dist = to_camera.Mag();
		CDIModelDrawable* lod = piece->GetLODFromDist(lod_bias, dist);
		if (!lod)
		{
			lod = piece->GetLOD(0);
		}
		if (!lod)
		{
			continue;
		}

		DiligentRigidMesh* rigid_mesh = nullptr;
		DiligentSkelMesh* skel_mesh = nullptr;
		DiligentVAMesh* va_mesh = nullptr;

		switch (lod->GetType())
		{
			case CRenderObject::eRigidMesh:
				rigid_mesh = static_cast<DiligentRigidMesh*>(lod);
				break;
			case CRenderObject::eSkelMesh:
				skel_mesh = static_cast<DiligentSkelMesh*>(lod);
				break;
			case CRenderObject::eVAMesh:
				va_mesh = static_cast<DiligentVAMesh*>(lod);
				break;
			default:
				break;
		}

		if (!rigid_mesh && !skel_mesh && !va_mesh)
		{
			continue;
		}

		if (skel_mesh)
		{
			if (!skel_mesh->UpdateSkinnedVertices(instance))
			{
				ok = false;
				break;
			}
		}
		else if (va_mesh)
		{
			if (!va_mesh->UpdateVA(model, &instance->m_AnimTracker.m_TimeRef))
			{
				ok = false;
				break;
			}
		}

		DiligentTextureArray textures{};
		if (rigid_mesh)
		{
			LTMatrix model_matrix;
			diligent_get_model_transform_raw(instance, rigid_mesh->GetBoneEffector(), model_matrix);
			LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * model_matrix;

			Diligent::IBuffer* vertex_buffers[4] = {};
			for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
			{
				vertex_buffers[stream_index] = rigid_mesh->GetVertexBuffer(stream_index);
			}

			ok = diligent_draw_mesh_with_pipeline_for_target(
				instance,
				rigid_mesh->GetLayout(),
				vertex_buffers,
				rigid_mesh->GetIndexBuffer(),
				rigid_mesh->GetIndexCount(),
				pass,
				shader_pass,
				textures,
				mvp,
				model_matrix,
				color_format,
				depth_format);
		}
		else if (skel_mesh)
		{
			LTMatrix model_matrix;
			model_matrix.Identity();
			LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView;

			Diligent::IBuffer* vertex_buffers[4] = {};
			for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
			{
				vertex_buffers[stream_index] = skel_mesh->GetVertexBuffer(stream_index);
			}

			ok = diligent_draw_mesh_with_pipeline_for_target(
				instance,
				skel_mesh->GetLayout(),
				vertex_buffers,
				skel_mesh->GetIndexBuffer(),
				skel_mesh->GetIndexCount(),
				pass,
				shader_pass,
				textures,
				mvp,
				model_matrix,
				color_format,
				depth_format);
		}
		else if (va_mesh)
		{
			LTMatrix model_matrix;
			diligent_get_model_transform_raw(instance, va_mesh->GetBoneEffector(), model_matrix);
			LTMatrix mvp = g_diligent_state.view_params.m_mProjection * g_diligent_state.view_params.m_mView * model_matrix;

			Diligent::IBuffer* vertex_buffers[4] = {};
			for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
			{
				vertex_buffers[stream_index] = va_mesh->GetVertexBuffer(stream_index);
			}

			ok = diligent_draw_mesh_with_pipeline_for_target(
				instance,
				va_mesh->GetLayout(),
				vertex_buffers,
				va_mesh->GetIndexBuffer(),
				va_mesh->GetIndexCount(),
				pass,
				shader_pass,
				textures,
				mvp,
				model_matrix,
				color_format,
				depth_format);
		}

		if (!ok)
		{
			break;
		}
	}

	instance->m_ColorR = old_r;
	instance->m_ColorG = old_g;
	instance->m_ColorB = old_b;
	return ok;
}

bool diligent_draw_model_shadow_with_attachments(
	ModelInstance* instance,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format,
	uint8 shadow_r,
	uint8 shadow_g,
	uint8 shadow_b)
{
	if (!diligent_draw_model_instance_shadow(instance, pass, shader_pass, color_format, depth_format, shadow_r, shadow_g, shadow_b))
	{
		return false;
	}

	ILTCommon* common_client = diligent_get_common_client();
	if (!common_client)
	{
		return true;
	}

	for (Attachment* attachment = instance->m_Attachments; attachment; attachment = attachment->m_pNext)
	{
		HOBJECT parent = nullptr;
		HOBJECT child = nullptr;
		common_client->GetAttachmentObjects(reinterpret_cast<HATTACHMENT>(attachment), parent, child);
		auto* child_obj = reinterpret_cast<LTObject*>(child);
		if (!child_obj || child_obj->m_ObjectType != OT_MODEL)
		{
			continue;
		}

		ModelInstance* child_instance = child_obj->ToModel();
		if (!child_instance || child_instance == instance)
		{
			continue;
		}

		if (!diligent_draw_model_instance_shadow(child_instance, pass, shader_pass, color_format, depth_format, shadow_r, shadow_g, shadow_b))
		{
			return false;
		}
	}

	return true;
}

void diligent_release_model_resources()
{
	g_model_pipelines.clear();
	g_model_resources.vertex_shaders.clear();
	g_model_resources.pixel_shader_textured.Release();
	g_model_resources.pixel_shader_solid.Release();
	g_model_resources.constant_buffer.Release();
}
