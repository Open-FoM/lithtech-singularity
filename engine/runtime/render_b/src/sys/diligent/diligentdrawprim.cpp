#include "bdefs.h"
#include "diligentdrawprim.h"

#include "diligent_render.h"
#include "render.h"
#include "rendererconsolevars.h"
#include "de_objects.h"
#include "viewparams.h"
#include "ilttexinterface.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#include "diligent_shaders_generated.h"

#include <array>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

static ILTTexInterface* pTexInterface;
define_holder(ILTTexInterface, pTexInterface);

extern int32 g_ScreenWidth;
extern int32 g_ScreenHeight;
extern ViewParams g_ViewParams;

namespace
{
struct DrawPrimConstants
{
	std::array<float, 16> mvp{};
	std::array<float, 16> view{};
	std::array<float, 4> fog_color{};
	std::array<float, 4> fog_params{};
	int32 color_op = 0;
	int32 alpha_test_mode = 0;
	int32 padding[2]{};
};

struct DrawPrimPipelineKey
{
	Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	int32 blend_mode = 0;
	int32 z_mode = 0;
	int32 fill_mode = 0;
	int32 cull_mode = 0;
	bool textured = false;

	bool operator==(const DrawPrimPipelineKey& other) const
	{
		return topology == other.topology &&
			blend_mode == other.blend_mode &&
			z_mode == other.z_mode &&
			fill_mode == other.fill_mode &&
			cull_mode == other.cull_mode &&
			textured == other.textured;
	}
};

struct DrawPrimPipelineKeyHash
{
	size_t operator()(const DrawPrimPipelineKey& key) const noexcept
	{
		size_t hash_value = 0;
		hash_value ^= std::hash<int32>{}(static_cast<int32>(key.topology)) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
		hash_value ^= std::hash<int32>{}(key.blend_mode) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
		hash_value ^= std::hash<int32>{}(key.z_mode) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
		hash_value ^= std::hash<int32>{}(key.fill_mode) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
		hash_value ^= std::hash<int32>{}(key.cull_mode) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
		hash_value ^= std::hash<bool>{}(key.textured) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
		return hash_value;
	}
};

struct DrawPrimPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct AutoDrawPrimBlock
{
	AutoDrawPrimBlock(ILTDrawPrim* draw_prim)
		: draw_prim_(draw_prim)
	{
		if (draw_prim_)
		{
			draw_prim_->BeginDrawPrim();
		}
	}

	~AutoDrawPrimBlock()
	{
		if (draw_prim_)
		{
			draw_prim_->EndDrawPrim();
		}
	}

	ILTDrawPrim* draw_prim_ = nullptr;
};

void StoreMatrixFromLTMatrix(const LTMatrix& matrix, std::array<float, 16>& out)
{
	out[0] = matrix.m[0][0];
	out[1] = matrix.m[1][0];
	out[2] = matrix.m[2][0];
	out[3] = matrix.m[3][0];
	out[4] = matrix.m[0][1];
	out[5] = matrix.m[1][1];
	out[6] = matrix.m[2][1];
	out[7] = matrix.m[3][1];
	out[8] = matrix.m[0][2];
	out[9] = matrix.m[1][2];
	out[10] = matrix.m[2][2];
	out[11] = matrix.m[3][2];
	out[12] = matrix.m[0][3];
	out[13] = matrix.m[1][3];
	out[14] = matrix.m[2][3];
	out[15] = matrix.m[3][3];
}

LTMatrix BuildPerspectiveMatrix(float fov_y, float aspect, float near_z, float far_z)
{
	const float y_scale = 1.0f / std::tan(fov_y * 0.5f);
	const float x_scale = y_scale / aspect;
	LTMatrix matrix;
	matrix.Identity();
	matrix.m[0][0] = x_scale;
	matrix.m[1][1] = y_scale;
	matrix.m[2][2] = far_z / (far_z - near_z);
	matrix.m[2][3] = 1.0f;
	matrix.m[3][2] = (-far_z * near_z) / (far_z - near_z);
	matrix.m[3][3] = 0.0f;
	return matrix;
}

Diligent::CULL_MODE MapCullMode(ELTDPCullMode mode)
{
	switch (mode)
	{
		case DRAWPRIM_CULL_NONE:
			return Diligent::CULL_MODE_NONE;
		case DRAWPRIM_CULL_CCW:
			return Diligent::CULL_MODE_BACK;
		case DRAWPRIM_CULL_CW:
			return Diligent::CULL_MODE_FRONT;
		default:
			return Diligent::CULL_MODE_NONE;
	}
}

Diligent::FILL_MODE MapFillMode(ELTDPFillMode mode)
{
	switch (mode)
	{
		case DRAWPRIM_WIRE:
			return Diligent::FILL_MODE_WIREFRAME;
		case DRAWPRIM_FILL:
			return Diligent::FILL_MODE_SOLID;
		default:
			return Diligent::FILL_MODE_SOLID;
	}
}

void FillBlendState(Diligent::RenderTargetBlendDesc& blend_desc, ELTBlendMode mode)
{
	blend_desc.BlendEnable = mode != DRAWPRIM_NOBLEND;
	blend_desc.RenderTargetWriteMask = Diligent::COLOR_MASK_ALL;

	if (!blend_desc.BlendEnable)
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_ZERO;
		blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
		blend_desc.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlendAlpha = Diligent::BLEND_FACTOR_ZERO;
		blend_desc.BlendOpAlpha = Diligent::BLEND_OPERATION_ADD;
		return;
	}

	switch (mode)
	{
		case DRAWPRIM_BLEND_ADD:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ONE;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
			break;
		case DRAWPRIM_BLEND_SATURATE:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_INV_DEST_COLOR;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
			break;
		case DRAWPRIM_BLEND_MOD_SRCALPHA:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
			break;
		case DRAWPRIM_BLEND_MOD_SRCCOLOR:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_COLOR;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_COLOR;
			break;
		case DRAWPRIM_BLEND_MOD_DSTCOLOR:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_DEST_COLOR;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_INV_DEST_COLOR;
			break;
		case DRAWPRIM_BLEND_MUL_SRCALPHA_ONE:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
			break;
		case DRAWPRIM_BLEND_MUL_SRCALPHA:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_ZERO;
			break;
		case DRAWPRIM_BLEND_MUL_SRCCOL_DSTCOL:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_COLOR;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_DEST_COLOR;
			break;
		case DRAWPRIM_BLEND_MUL_SRCCOL_ONE:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_COLOR;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
			break;
		case DRAWPRIM_BLEND_MUL_DSTCOL_ZERO:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_DEST_COLOR;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_ZERO;
			break;
		case DRAWPRIM_NOBLEND:
		default:
			blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ONE;
			blend_desc.DestBlend = Diligent::BLEND_FACTOR_ZERO;
			break;
	}

	blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
	blend_desc.SrcBlendAlpha = blend_desc.SrcBlend;
	blend_desc.DestBlendAlpha = blend_desc.DestBlend;
	blend_desc.BlendOpAlpha = Diligent::BLEND_OPERATION_ADD;
}

}

CDiligentDrawPrim::DrawPrimVertex CDiligentDrawPrim::MakeVertex(float x, float y, float z, const LT_VERTRGBA& rgba, float u, float v)
{
	DrawPrimVertex vertex;
	vertex.x = x;
	vertex.y = y;
	vertex.z = z;
	vertex.r = static_cast<float>(rgba.r) / 255.0f;
	vertex.g = static_cast<float>(rgba.g) / 255.0f;
	vertex.b = static_cast<float>(rgba.b) / 255.0f;
	vertex.a = static_cast<float>(rgba.a) / 255.0f;
	vertex.u = u;
	vertex.v = v;
	return vertex;
}

void CDiligentDrawPrim::AppendTriangle(std::vector<DrawPrimVertex>& vertices, const DrawPrimVertex& first, const DrawPrimVertex& second, const DrawPrimVertex& third)
{
	vertices.push_back(first);
	vertices.push_back(second);
	vertices.push_back(third);
}

struct DiligentDrawPrimResources
{
	std::array<float, 16> mvp{};
	std::array<float, 16> view{};
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_textured;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_solid;
	uint32 vertex_buffer_size = 0;
	std::unordered_map<DrawPrimPipelineKey, DrawPrimPipeline, DrawPrimPipelineKeyHash> pipelines;
};

define_interface(CDiligentDrawPrim, ILTDrawPrim);
instantiate_interface(CDiligentDrawPrim, ILTDrawPrim, Internal);

CDiligentDrawPrim::CDiligentDrawPrim()
	: m_resources(std::make_unique<DiligentDrawPrimResources>())
{
}

bool CDiligentDrawPrim::VerifyValid() const
{
	return r_GetRenderDevice() != nullptr && r_GetImmediateContext() != nullptr && r_GetSwapChain() != nullptr;
}

static bool EnsureConstantBuffer(DiligentDrawPrimResources& resources)
{
	if (resources.constant_buffer)
	{
		return true;
	}

	auto* device = r_GetRenderDevice();
	if (!device)
	{
		return false;
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_drawprim_constants";
	desc.Size = sizeof(DrawPrimConstants);
	desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

	device->CreateBuffer(desc, nullptr, &resources.constant_buffer);
	return resources.constant_buffer != nullptr;
}

static bool EnsureVertexBuffer(DiligentDrawPrimResources& resources, uint32 size)
{
	if (resources.vertex_buffer && resources.vertex_buffer_size >= size)
	{
		return true;
	}

	auto* device = r_GetRenderDevice();
	if (!device)
	{
		return false;
	}

	resources.vertex_buffer.Release();
	resources.vertex_buffer_size = 0;

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_drawprim_vertices";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = sizeof(CDiligentDrawPrim::DrawPrimVertex);

	device->CreateBuffer(desc, nullptr, &resources.vertex_buffer);
	if (!resources.vertex_buffer)
	{
		return false;
	}

	resources.vertex_buffer_size = size;
	return true;
}

static bool EnsureShaders(DiligentDrawPrimResources& resources)
{
	if (resources.vertex_shader && resources.pixel_shader_textured && resources.pixel_shader_solid)
	{
		return true;
	}

	auto* device = r_GetRenderDevice();
	if (!device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	Diligent::ShaderDesc shader_desc;
	shader_desc.Name = "ltjs_drawprim_vs";
	shader_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;

	shader_info.Desc = shader_desc;
	shader_info.EntryPoint = "VSMain";
	shader_info.Source = GetVertexShaderSource();
	device->CreateShader(shader_info, &resources.vertex_shader);

	if (!resources.vertex_shader)
	{
		return false;
	}

	shader_desc.Name = "ltjs_drawprim_ps_textured";
	shader_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	shader_info.Desc = shader_desc;
	shader_info.EntryPoint = "PSMain";
	shader_info.Source = GetPixelShaderSourceTextured();
	device->CreateShader(shader_info, &resources.pixel_shader_textured);

	shader_desc.Name = "ltjs_drawprim_ps_solid";
	shader_info.Desc = shader_desc;
	shader_info.Source = GetPixelShaderSourceSolid();
	device->CreateShader(shader_info, &resources.pixel_shader_solid);

	return resources.pixel_shader_textured != nullptr && resources.pixel_shader_solid != nullptr;
}

static DrawPrimPipeline* GetPipeline(
	DiligentDrawPrimResources& resources,
	const DrawPrimPipelineKey& key)
{
	auto it = resources.pipelines.find(key);
	if (it != resources.pipelines.end())
	{
		return &it->second;
	}

	if (!EnsureConstantBuffer(resources) || !EnsureShaders(resources))
	{
		return nullptr;
	}

	auto* device = r_GetRenderDevice();
	auto* swap_chain = r_GetSwapChain();
	if (!device || !swap_chain)
	{
		return nullptr;
	}

	const auto swap_desc = swap_chain->GetDesc();

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_drawprim_pso";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = swap_desc.ColorBufferFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = swap_desc.DepthBufferFormat;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = key.topology;

	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));

	pipeline_info.GraphicsPipeline.RasterizerDesc.FillMode = MapFillMode(static_cast<ELTDPFillMode>(key.fill_mode));
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = MapCullMode(static_cast<ELTDPCullMode>(key.cull_mode));
	pipeline_info.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;

	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = key.z_mode != DRAWPRIM_NOZ;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = key.z_mode == DRAWPRIM_ZRW;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;

	FillBlendState(pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0], static_cast<ELTBlendMode>(key.blend_mode));

	pipeline_info.pVS = resources.vertex_shader;
	pipeline_info.pPS = key.textured ? resources.pixel_shader_textured : resources.pixel_shader_solid;

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	std::vector<Diligent::ShaderResourceVariableDesc> variables;
	if (key.textured)
	{
		variables.push_back({Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		pipeline_info.PSODesc.ResourceLayout.Variables = variables.data();
		pipeline_info.PSODesc.ResourceLayout.NumVariables = static_cast<uint32>(variables.size());

		Diligent::ImmutableSamplerDesc sampler_desc;
		sampler_desc.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
		sampler_desc.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
		sampler_desc.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
		sampler_desc.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
		sampler_desc.TextureName = "g_Texture";
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;
	}

	DrawPrimPipeline pipeline;
	device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	auto* vs_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "DrawPrimConstants");
	if (vs_constants)
	{
		vs_constants->Set(resources.constant_buffer);
	}

	auto* ps_constants = pipeline.pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "DrawPrimConstants");
	if (ps_constants)
	{
		ps_constants->Set(resources.constant_buffer);
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	resources.pipelines.emplace(key, pipeline);
	return &resources.pipelines.find(key)->second;
}

LTRESULT CDiligentDrawPrim::BeginDrawPrim()
{
	if (m_block_count++ > 0)
	{
		return LT_OK;
	}

	return LT_OK;
}

LTRESULT CDiligentDrawPrim::EndDrawPrim()
{
	if (m_block_count == 0)
	{
		return LT_ERROR;
	}

	m_block_count--;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetCamera(const HOBJECT hCamera)
{
	m_pCamera = hCamera;
	m_transform_dirty = true;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetTexture(const HTEXTURE hTexture)
{
	m_pTexture = hTexture;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetTransformType(const ELTTransformType eType)
{
	m_eTransType = eType;
	m_transform_dirty = true;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetColorOp(const ELTColorOp eColorOp)
{
	m_ColorOp = eColorOp;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetAlphaBlendMode(const ELTBlendMode eBlendMode)
{
	m_BlendMode = eBlendMode;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetZBufferMode(const ELTZBufferMode eZBufferMode)
{
	m_eZBufferMode = eZBufferMode;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetAlphaTestMode(const ELTTestMode eTestMode)
{
	m_eTestMode = eTestMode;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetClipMode(const ELTClipMode eClipMode)
{
	m_eClipType = eClipMode;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetFillMode(ELTDPFillMode eFillMode)
{
	m_eFillMode = eFillMode;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetCullMode(ELTDPCullMode eCullMode)
{
	m_eCullMode = eCullMode;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetFogEnable(bool bFogEnable)
{
	m_bFogEnable = bFogEnable;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetReallyClose(bool bReallyClose)
{
	m_bReallyClose = bReallyClose;
	m_transform_dirty = true;
	return LT_OK;
}

LTRESULT CDiligentDrawPrim::SetEffectShaderID(uint32 nEffectShaderID)
{
	m_nEffectShaderID = nEffectShaderID;
	return LT_OK;
}

void CDiligentDrawPrim::SetUVWH(LT_POLYGT4* pPrim, HTEXTURE pTex, float u, float v, float w, float h)
{
	if (!pPrim)
	{
		return;
	}

	uint32 ttw = 0;
	uint32 tth = 0;
	float tw = 0.0f;
	float th = 0.0f;

	if (pTex && pTexInterface && pTexInterface->GetTextureDims(pTex, ttw, tth) == LT_OK)
	{
		tw = static_cast<float>(ttw);
		th = static_cast<float>(tth);

		const float factor = 1.0f;
		const float pixelcenterh = 0.05f / tw;
		const float pixelcenterv = 0.05f / th;

		pPrim->verts[0].u = u / tw + pixelcenterh;
		pPrim->verts[0].v = v / th + pixelcenterv;
		pPrim->verts[1].u = (u + w + factor) / tw + pixelcenterh;
		pPrim->verts[1].v = v / th + pixelcenterv;
		pPrim->verts[2].u = (u + w + factor) / tw + pixelcenterh;
		pPrim->verts[2].v = (v + h + factor) / th + pixelcenterv;
		pPrim->verts[3].u = u / tw + pixelcenterh;
		pPrim->verts[3].v = (v + h + factor) / th + pixelcenterv;
	}
	else
	{
		pPrim->verts[0].u = pPrim->verts[1].u = pPrim->verts[2].u = pPrim->verts[3].u = 0.0f;
		pPrim->verts[0].v = pPrim->verts[1].v = pPrim->verts[2].v = pPrim->verts[3].v = 0.0f;
	}
}

bool CDiligentDrawPrim::UpdateTransformMatrix()
{
	auto& resources = *m_resources;
	if (!m_transform_dirty)
	{
		return true;
	}

	LTMatrix identity;
	identity.Identity();

	if (m_bReallyClose)
	{
		const float aspect = g_ScreenHeight > 0 ? static_cast<float>(g_ScreenWidth) / static_cast<float>(g_ScreenHeight) : 1.0f;
		const float fov = g_CV_PVModelFOV.m_Val * 0.01745329251994f;
		const float near_z = g_CV_ModelNear.m_Val;
		const float far_z = g_CV_ModelFar.m_Val;
		LTMatrix proj = BuildPerspectiveMatrix(fov, aspect, near_z, far_z);
		StoreMatrixFromLTMatrix(proj, resources.mvp);
		StoreMatrixFromLTMatrix(identity, resources.view);
		m_transform_dirty = false;
		return true;
	}

	if (m_eTransType == DRAWPRIM_TRANSFORM_SCREEN || !m_pCamera)
	{
		const float width = g_ScreenWidth > 0 ? static_cast<float>(g_ScreenWidth) : 1.0f;
		const float height = g_ScreenHeight > 0 ? static_cast<float>(g_ScreenHeight) : 1.0f;
		const float sx = 2.0f / width;
		const float sy = -2.0f / height;
		resources.mvp = {
			sx, 0.0f, 0.0f, 0.0f,
			0.0f, sy, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f, 1.0f
		};
		StoreMatrixFromLTMatrix(identity, resources.view);
		m_transform_dirty = false;
		return true;
	}

	auto* camera_object = static_cast<LTObject*>(m_pCamera);
	if (!camera_object)
	{
		return false;
	}

	auto* camera = camera_object->ToCamera();
	if (!camera)
	{
		return false;
	}

	ViewParams draw_params;
	if (!d3d_InitFrustum(
			&draw_params,
			camera->m_xFov,
			camera->m_yFov,
			g_ViewParams.m_NearZ,
			g_ViewParams.m_FarZ,
			camera->m_Left,
			camera->m_Top,
			camera->m_Right,
			camera->m_Bottom,
			&camera->m_Pos,
			&camera->m_Rotation,
			ViewParams::eRenderMode_Normal))
	{
		return false;
	}

	if (m_eTransType == DRAWPRIM_TRANSFORM_CAMERA)
	{
		StoreMatrixFromLTMatrix(draw_params.m_mProjection, resources.mvp);
		StoreMatrixFromLTMatrix(identity, resources.view);
	}
	else
	{
		LTMatrix mvp = draw_params.m_mProjection * draw_params.m_mView;
		StoreMatrixFromLTMatrix(mvp, resources.mvp);
		StoreMatrixFromLTMatrix(draw_params.m_mView, resources.view);
	}

	m_transform_dirty = false;
	return true;
}

void CDiligentDrawPrim::ApplyViewport()
{
	auto* context = r_GetImmediateContext();
	if (!context)
	{
		return;
	}

	Diligent::Viewport viewport;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = m_bReallyClose ? 0.1f : 1.0f;

	if (m_pCamera)
	{
		auto* camera_object = static_cast<LTObject*>(m_pCamera);
		auto* camera = camera_object ? camera_object->ToCamera() : nullptr;
		if (camera)
		{
			viewport.TopLeftX = static_cast<float>(camera->m_Left);
			viewport.TopLeftY = static_cast<float>(camera->m_Top);
			viewport.Width = static_cast<float>(camera->m_Right - camera->m_Left);
			viewport.Height = static_cast<float>(camera->m_Bottom - camera->m_Top);
			context->SetViewports(1, &viewport, 0, 0);
			return;
		}
	}

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(g_ScreenWidth);
	viewport.Height = static_cast<float>(g_ScreenHeight);
	context->SetViewports(1, &viewport, 0, 0);
}

LTRESULT CDiligentDrawPrim::SubmitDraw(const std::vector<DrawPrimVertex>& vertices, Diligent::PRIMITIVE_TOPOLOGY topology, bool textured)
{
	if (vertices.empty())
	{
		return LT_OK;
	}

	if (!VerifyValid())
	{
		return LT_ERROR;
	}

	AutoDrawPrimBlock auto_block(this);

	if (!UpdateTransformMatrix())
	{
		return LT_ERROR;
	}

	auto& resources = *m_resources;
	const bool use_texture = textured && m_pTexture != nullptr;

	DrawPrimConstants constants;
	constants.mvp = resources.mvp;
	constants.view = resources.view;

	const float fog_near = g_CV_FogNearZ.m_Val;
	const float fog_far = g_CV_FogFarZ.m_Val;
	const bool fog_enabled = m_bFogEnable && (fog_far != fog_near);
	constants.fog_color = {
		static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f,
		static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f,
		static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f,
		1.0f
	};
	constants.fog_params = {fog_near, fog_far, fog_enabled ? 1.0f : 0.0f, 0.0f};

	constants.color_op = static_cast<int32>(m_ColorOp);
	constants.alpha_test_mode = static_cast<int32>(m_eTestMode);

	if (!EnsureConstantBuffer(resources))
	{
		return LT_ERROR;
	}

	auto* context = r_GetImmediateContext();
	void* mapped_constants = nullptr;
	context->MapBuffer(resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	context->UnmapBuffer(resources.constant_buffer, Diligent::MAP_WRITE);

	const uint32 vertex_data_size = static_cast<uint32>(vertices.size() * sizeof(DrawPrimVertex));
	if (!EnsureVertexBuffer(resources, vertex_data_size))
	{
		return LT_ERROR;
	}

	void* mapped_vertices = nullptr;
	context->MapBuffer(resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	std::memcpy(mapped_vertices, vertices.data(), vertex_data_size);
	context->UnmapBuffer(resources.vertex_buffer, Diligent::MAP_WRITE);

	DrawPrimPipelineKey key;
	key.topology = topology;
	key.blend_mode = m_BlendMode;
	key.z_mode = m_eZBufferMode;
	key.fill_mode = m_eFillMode;
	key.cull_mode = m_eCullMode;
	key.textured = use_texture;

	DrawPrimPipeline* pipeline = GetPipeline(resources, key);
	if (!pipeline || !pipeline->pipeline_state)
	{
		return LT_ERROR;
	}

	if (use_texture)
	{
		auto* texture_view = diligent_get_drawprim_texture_view(m_pTexture, false);
		if (texture_view && pipeline->srb)
		{
			auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture");
			if (texture_var)
			{
				texture_var->Set(texture_view);
			}
		}
	}

	auto* swap_chain = r_GetSwapChain();
	auto* render_target = swap_chain->GetCurrentBackBufferRTV();
	auto* depth_target = swap_chain->GetDepthBufferDSV();
	context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	ApplyViewport();

	context->SetPipelineState(pipeline->pipeline_state);

	Diligent::IBuffer* buffers[] = {resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	context->SetVertexBuffers(0, 1, buffers, offsets, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (pipeline->srb)
	{
		context->CommitShaderResources(pipeline->srb, Diligent::COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
	}

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = static_cast<uint32>(vertices.size());
	context->Draw(draw_attribs);

	return LT_OK;
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYGT3* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 3);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYGT3& prim = pPrim[index];
		AppendTriangle(vertices,
			MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.verts[0].rgba, prim.verts[0].u, prim.verts[0].v),
			MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.verts[1].rgba, prim.verts[1].u, prim.verts[1].v),
			MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.verts[2].rgba, prim.verts[2].u, prim.verts[2].v));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYFT3* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 3);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYFT3& prim = pPrim[index];
		AppendTriangle(vertices,
			MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.rgba, prim.verts[0].u, prim.verts[0].v),
			MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.rgba, prim.verts[1].u, prim.verts[1].v),
			MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.rgba, prim.verts[2].u, prim.verts[2].v));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYG3* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 3);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYG3& prim = pPrim[index];
		AppendTriangle(vertices,
			MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.verts[0].rgba, 0.0f, 0.0f),
			MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.verts[1].rgba, 0.0f, 0.0f),
			MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.verts[2].rgba, 0.0f, 0.0f));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYF3* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 3);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYF3& prim = pPrim[index];
		AppendTriangle(vertices,
			MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.rgba, 0.0f, 0.0f),
			MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.rgba, 0.0f, 0.0f),
			MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.rgba, 0.0f, 0.0f));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYGT4* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 6);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYGT4& prim = pPrim[index];
		const auto v0 = MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.verts[0].rgba, prim.verts[0].u, prim.verts[0].v);
		const auto v1 = MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.verts[1].rgba, prim.verts[1].u, prim.verts[1].v);
		const auto v2 = MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.verts[2].rgba, prim.verts[2].u, prim.verts[2].v);
		const auto v3 = MakeVertex(prim.verts[3].x, prim.verts[3].y, prim.verts[3].z, prim.verts[3].rgba, prim.verts[3].u, prim.verts[3].v);
		AppendTriangle(vertices, v0, v1, v2);
		AppendTriangle(vertices, v0, v2, v3);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYGT4** ppPrim, const uint32 nCount)
{
	if (!ppPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 6);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYGT4* prim = ppPrim[index];
		if (!prim)
		{
			continue;
		}
		const auto v0 = MakeVertex(prim->verts[0].x, prim->verts[0].y, prim->verts[0].z, prim->verts[0].rgba, prim->verts[0].u, prim->verts[0].v);
		const auto v1 = MakeVertex(prim->verts[1].x, prim->verts[1].y, prim->verts[1].z, prim->verts[1].rgba, prim->verts[1].u, prim->verts[1].v);
		const auto v2 = MakeVertex(prim->verts[2].x, prim->verts[2].y, prim->verts[2].z, prim->verts[2].rgba, prim->verts[2].u, prim->verts[2].v);
		const auto v3 = MakeVertex(prim->verts[3].x, prim->verts[3].y, prim->verts[3].z, prim->verts[3].rgba, prim->verts[3].u, prim->verts[3].v);
		AppendTriangle(vertices, v0, v1, v2);
		AppendTriangle(vertices, v0, v2, v3);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYFT4* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 6);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYFT4& prim = pPrim[index];
		const auto v0 = MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.rgba, prim.verts[0].u, prim.verts[0].v);
		const auto v1 = MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.rgba, prim.verts[1].u, prim.verts[1].v);
		const auto v2 = MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.rgba, prim.verts[2].u, prim.verts[2].v);
		const auto v3 = MakeVertex(prim.verts[3].x, prim.verts[3].y, prim.verts[3].z, prim.rgba, prim.verts[3].u, prim.verts[3].v);
		AppendTriangle(vertices, v0, v1, v2);
		AppendTriangle(vertices, v0, v2, v3);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYG4* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 6);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYG4& prim = pPrim[index];
		const auto v0 = MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.verts[0].rgba, 0.0f, 0.0f);
		const auto v1 = MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.verts[1].rgba, 0.0f, 0.0f);
		const auto v2 = MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.verts[2].rgba, 0.0f, 0.0f);
		const auto v3 = MakeVertex(prim.verts[3].x, prim.verts[3].y, prim.verts[3].z, prim.verts[3].rgba, 0.0f, 0.0f);
		AppendTriangle(vertices, v0, v1, v2);
		AppendTriangle(vertices, v0, v2, v3);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_POLYF4* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 6);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_POLYF4& prim = pPrim[index];
		const auto v0 = MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.rgba, 0.0f, 0.0f);
		const auto v1 = MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.rgba, 0.0f, 0.0f);
		const auto v2 = MakeVertex(prim.verts[2].x, prim.verts[2].y, prim.verts[2].z, prim.rgba, 0.0f, 0.0f);
		const auto v3 = MakeVertex(prim.verts[3].x, prim.verts[3].y, prim.verts[3].z, prim.rgba, 0.0f, 0.0f);
		AppendTriangle(vertices, v0, v1, v2);
		AppendTriangle(vertices, v0, v2, v3);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_LINEGT* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 2);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_LINEGT& prim = pPrim[index];
		vertices.push_back(MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.verts[0].rgba, prim.verts[0].u, prim.verts[0].v));
		vertices.push_back(MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.verts[1].rgba, prim.verts[1].u, prim.verts[1].v));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_LINEFT* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 2);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_LINEFT& prim = pPrim[index];
		vertices.push_back(MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.rgba, prim.verts[0].u, prim.verts[0].v));
		vertices.push_back(MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.rgba, prim.verts[1].u, prim.verts[1].v));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_LINEG* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 2);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_LINEG& prim = pPrim[index];
		vertices.push_back(MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.verts[0].rgba, 0.0f, 0.0f));
		vertices.push_back(MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.verts[1].rgba, 0.0f, 0.0f));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrim(LT_LINEF* pPrim, const uint32 nCount)
{
	if (!pPrim)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount * 2);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_LINEF& prim = pPrim[index];
		vertices.push_back(MakeVertex(prim.verts[0].x, prim.verts[0].y, prim.verts[0].z, prim.rgba, 0.0f, 0.0f));
		vertices.push_back(MakeVertex(prim.verts[1].x, prim.verts[1].y, prim.verts[1].z, prim.rgba, 0.0f, 0.0f));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrimPoint(LT_VERTGT* pVerts, const uint32 nCount)
{
	if (!pVerts)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_VERTGT& vert = pVerts[index];
		vertices.push_back(MakeVertex(vert.x, vert.y, vert.z, vert.rgba, vert.u, vert.v));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrimPoint(LT_VERTG* pVerts, const uint32 nCount)
{
	if (!pVerts)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve(nCount);
	for (uint32 index = 0; index < nCount; ++index)
	{
		const LT_VERTG& vert = pVerts[index];
		vertices.push_back(MakeVertex(vert.x, vert.y, vert.z, vert.rgba, 0.0f, 0.0f));
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrimFan(LT_VERTGT* pVerts, const uint32 nCount)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	const DrawPrimVertex base = MakeVertex(pVerts[0].x, pVerts[0].y, pVerts[0].z, pVerts[0].rgba, pVerts[0].u, pVerts[0].v);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const auto v1 = MakeVertex(pVerts[index - 1].x, pVerts[index - 1].y, pVerts[index - 1].z, pVerts[index - 1].rgba, pVerts[index - 1].u, pVerts[index - 1].v);
		const auto v2 = MakeVertex(pVerts[index].x, pVerts[index].y, pVerts[index].z, pVerts[index].rgba, pVerts[index].u, pVerts[index].v);
		AppendTriangle(vertices, base, v1, v2);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrimFan(LT_VERTFT* pVerts, const uint32 nCount, LT_VERTRGBA rgba)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	const DrawPrimVertex base = MakeVertex(pVerts[0].x, pVerts[0].y, pVerts[0].z, rgba, pVerts[0].u, pVerts[0].v);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const auto v1 = MakeVertex(pVerts[index - 1].x, pVerts[index - 1].y, pVerts[index - 1].z, rgba, pVerts[index - 1].u, pVerts[index - 1].v);
		const auto v2 = MakeVertex(pVerts[index].x, pVerts[index].y, pVerts[index].z, rgba, pVerts[index].u, pVerts[index].v);
		AppendTriangle(vertices, base, v1, v2);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrimFan(LT_VERTG* pVerts, const uint32 nCount)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	const DrawPrimVertex base = MakeVertex(pVerts[0].x, pVerts[0].y, pVerts[0].z, pVerts[0].rgba, 0.0f, 0.0f);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const auto v1 = MakeVertex(pVerts[index - 1].x, pVerts[index - 1].y, pVerts[index - 1].z, pVerts[index - 1].rgba, 0.0f, 0.0f);
		const auto v2 = MakeVertex(pVerts[index].x, pVerts[index].y, pVerts[index].z, pVerts[index].rgba, 0.0f, 0.0f);
		AppendTriangle(vertices, base, v1, v2);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrimFan(LT_VERTF* pVerts, const uint32 nCount, LT_VERTRGBA rgba)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	const DrawPrimVertex base = MakeVertex(pVerts[0].x, pVerts[0].y, pVerts[0].z, rgba, 0.0f, 0.0f);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const auto v1 = MakeVertex(pVerts[index - 1].x, pVerts[index - 1].y, pVerts[index - 1].z, rgba, 0.0f, 0.0f);
		const auto v2 = MakeVertex(pVerts[index].x, pVerts[index].y, pVerts[index].z, rgba, 0.0f, 0.0f);
		AppendTriangle(vertices, base, v1, v2);
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrimStrip(LT_VERTGT* pVerts, const uint32 nCount)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const uint32 base = index - 2;
		DrawPrimVertex first = MakeVertex(pVerts[base].x, pVerts[base].y, pVerts[base].z, pVerts[base].rgba, pVerts[base].u, pVerts[base].v);
		DrawPrimVertex second = MakeVertex(pVerts[base + 1].x, pVerts[base + 1].y, pVerts[base + 1].z, pVerts[base + 1].rgba, pVerts[base + 1].u, pVerts[base + 1].v);
		DrawPrimVertex third = MakeVertex(pVerts[base + 2].x, pVerts[base + 2].y, pVerts[base + 2].z, pVerts[base + 2].rgba, pVerts[base + 2].u, pVerts[base + 2].v);
		if ((index & 1) == 0)
		{
			AppendTriangle(vertices, first, second, third);
		}
		else
		{
			AppendTriangle(vertices, second, first, third);
		}
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrimStrip(LT_VERTFT* pVerts, const uint32 nCount, LT_VERTRGBA rgba)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const uint32 base = index - 2;
		DrawPrimVertex first = MakeVertex(pVerts[base].x, pVerts[base].y, pVerts[base].z, rgba, pVerts[base].u, pVerts[base].v);
		DrawPrimVertex second = MakeVertex(pVerts[base + 1].x, pVerts[base + 1].y, pVerts[base + 1].z, rgba, pVerts[base + 1].u, pVerts[base + 1].v);
		DrawPrimVertex third = MakeVertex(pVerts[base + 2].x, pVerts[base + 2].y, pVerts[base + 2].z, rgba, pVerts[base + 2].u, pVerts[base + 2].v);
		if ((index & 1) == 0)
		{
			AppendTriangle(vertices, first, second, third);
		}
		else
		{
			AppendTriangle(vertices, second, first, third);
		}
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true);
}

LTRESULT CDiligentDrawPrim::DrawPrimStrip(LT_VERTG* pVerts, const uint32 nCount)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const uint32 base = index - 2;
		DrawPrimVertex first = MakeVertex(pVerts[base].x, pVerts[base].y, pVerts[base].z, pVerts[base].rgba, 0.0f, 0.0f);
		DrawPrimVertex second = MakeVertex(pVerts[base + 1].x, pVerts[base + 1].y, pVerts[base + 1].z, pVerts[base + 1].rgba, 0.0f, 0.0f);
		DrawPrimVertex third = MakeVertex(pVerts[base + 2].x, pVerts[base + 2].y, pVerts[base + 2].z, pVerts[base + 2].rgba, 0.0f, 0.0f);
		if ((index & 1) == 0)
		{
			AppendTriangle(vertices, first, second, third);
		}
		else
		{
			AppendTriangle(vertices, second, first, third);
		}
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}

LTRESULT CDiligentDrawPrim::DrawPrimStrip(LT_VERTF* pVerts, const uint32 nCount, LT_VERTRGBA rgba)
{
	if (!pVerts || nCount < 3)
	{
		return LT_ERROR;
	}

	std::vector<DrawPrimVertex> vertices;
	vertices.reserve((nCount - 2) * 3);
	for (uint32 index = 2; index < nCount; ++index)
	{
		const uint32 base = index - 2;
		DrawPrimVertex first = MakeVertex(pVerts[base].x, pVerts[base].y, pVerts[base].z, rgba, 0.0f, 0.0f);
		DrawPrimVertex second = MakeVertex(pVerts[base + 1].x, pVerts[base + 1].y, pVerts[base + 1].z, rgba, 0.0f, 0.0f);
		DrawPrimVertex third = MakeVertex(pVerts[base + 2].x, pVerts[base + 2].y, pVerts[base + 2].z, rgba, 0.0f, 0.0f);
		if ((index & 1) == 0)
		{
			AppendTriangle(vertices, first, second, third);
		}
		else
		{
			AppendTriangle(vertices, second, first, third);
		}
	}

	return SubmitDraw( vertices, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
}
