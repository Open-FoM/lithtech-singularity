#include "bdefs.h"
#include "renderstruct.h"
#include "ltbasedefs.h"
#include "ltbasetypes.h"
#include "pixelformat.h"
#include "dtxmgr.h"
#include "viewparams.h"
#include "de_world.h"
#include "de_sprite.h"
#include "de_objects.h"
#include "ltanimtracker.h"
#include "sysddstructs.h"
#include "world_client_bsp.h"
#include "world_tree.h"
#include "lightmap_compress.h"
#include "ltrotation.h"
#include "ltvector.h"
#include "setupobject.h"
#include "sprite.h"
#include "strtools.h"
#include "ltcodes.h"
#include "texturescriptmgr.h"
#include "texturescriptinstance.h"
#include "ltb.h"
#include "model.h"
#include "debuggeometry.h"
#include "../d3d/rendererconsolevars.h"
#include "iltrenderstyles.h"
#include "rendertarget.h"
#include "iltclient.h"
#include "../d3d/renderstylemap.h"

#ifdef LTJS_SDL_BACKEND
#include "ltjs_main_window_descriptor.h"
#endif

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "ltrenderstyle.h"

#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef __WINDOWS_H__
#include <windows.h>
#define __WINDOWS_H__
#endif

void d3d_InitViewBox2(
	ViewBoxDef* pDef,
	float nearZ,
	float farZ,
	const ViewParams& prev_params,
	float screen_min_x,
	float screen_min_y,
	float screen_max_x,
	float screen_max_y);
bool d3d_InitFrustum2(
	ViewParams* pParams,
	ViewBoxDef* pViewBox,
	float screenMinX,
	float screenMinY,
	float screenMaxX,
	float screenMaxY,
	const LTMatrix* pMat,
	const LTVector& vScale,
	ViewParams::ERenderMode eMode);

namespace
{
static IWorldClientBSP* world_bsp_client = nullptr;
define_holder(IWorldClientBSP, world_bsp_client);

constexpr uint32 kDiligentSurfaceFlagOptimized = 1u << 0;
constexpr uint32 kDiligentSurfaceFlagDirty = 1u << 1;

static ConVar<int> g_CV_ScreenGlow_DrawCanvases("ScreenGlow_DrawCanvases", 1);
static ConVar<int> g_CV_ScreenGlowBlurMultiTap("ScreenGlowBlurMultiTap", 1);

struct DiligentSurface
{
	uint32 width;
	uint32 height;
	uint32 pitch;
	uint32 flags;
	uint32 optimized_transparent_color;
	uint32 data[1];
};

struct DiligentSurfaceGpuData
{
	Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> srv;
	uint32 width = 0;
	uint32 height = 0;
	uint32 transparent_color = OPTIMIZE_NO_TRANSPARENCY;
};

struct DiligentOptimized2DPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct DiligentOptimized2DResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader;
	std::unordered_map<int32, DiligentOptimized2DPipeline> pipelines;
	std::unordered_map<int32, DiligentOptimized2DPipeline> clamped_pipelines;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	uint32 vertex_buffer_size = 0;
};

struct DiligentOptimized2DVertex
{
	float position[2];
	float color[4];
	float uv[2];
};

struct DiligentRBSpriteData
{
	SpriteTracker tracker{};
	Sprite* sprite = nullptr;
};

struct DiligentRBSection
{
	enum { kNumTextures = 2 };

	DiligentRBSection();
	~DiligentRBSection();
	DiligentRBSection(const DiligentRBSection&) = delete;
	DiligentRBSection& operator=(const DiligentRBSection&) = delete;

	SharedTexture* textures[kNumTextures];
	DiligentRBSpriteData* sprite_data[kNumTextures];
	CTextureScriptInstance* texture_effect;
	uint8 shader_code;
	bool fullbright = false;
	uint32 start_index;
	uint32 tri_count;
	uint32 start_vertex;
	uint32 vertex_count;
	uint32 lightmap_width;
	uint32 lightmap_height;
	uint32 lightmap_size;
	std::vector<uint8> lightmap_data;
	Diligent::RefCntAutoPtr<Diligent::ITexture> lightmap_texture;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> lightmap_srv;
};

struct DiligentRBVertex
{
	LTVector position;
	float u0;
	float v0;
	float u1;
	float v1;
	uint32 color;
	LTVector normal;
#ifdef LTJS_USE_TANGENT_AND_BINORMAL
	LTVector tangent;
	LTVector binormal;
#endif
};

struct DiligentRBGeometryPoly
{
	using TVertList = std::vector<LTVector>;
	TVertList vertices;
	LTPlane plane;
	EAABBCorner plane_corner = eAABB_None;
	LTVector min;
	LTVector max;
};

struct DiligentRBOccluder : public DiligentRBGeometryPoly
{
	uint32 id = 0;
	bool enabled = true;
};

struct DiligentRBLightGroup
{
	struct SubLightmap
	{
		uint32 left = 0;
		uint32 top = 0;
		uint32 width = 0;
		uint32 height = 0;
		std::vector<uint8> data;

		bool Load(ILTStream& stream);
	};

	uint32 id = 0;
	LTVector color;
	std::vector<uint8> vertex_intensities;
	std::vector<std::vector<SubLightmap>> section_lightmaps;

	void ClearSectionLightmaps();
};

struct DiligentRenderWorld;
struct DiligentRenderTexture;

struct DiligentRenderBlock
{
	enum { kNumChildren = 2 };
	static constexpr uint32 kInvalidChild = 0xFFFFFFFFu;

	DiligentRenderWorld* world = nullptr;
	LTVector bounds_min;
	LTVector bounds_max;
	LTVector center;
	LTVector half_dims;
	uint32 child_indices[kNumChildren] = {kInvalidChild, kInvalidChild};
	std::vector<std::unique_ptr<DiligentRBSection>> sections;
	std::vector<DiligentRBGeometryPoly> sky_portals;
	std::vector<DiligentRBOccluder> occluders;
	std::vector<DiligentRBLightGroup> light_groups;
	std::vector<DiligentRBVertex> vertices;
	std::vector<uint16> indices;
	bool lightmaps_dirty = true;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> index_buffer;

	bool Load(ILTStream* stream);
	void ExtendSkyBounds(const ViewParams& params, float& min_x, float& min_y, float& max_x, float& max_y) const;
	bool EnsureGpuBuffers();
	bool UpdateLightmaps();
	bool SetLightGroupColor(uint32 id, const LTVector& color);
	DiligentRenderBlock* GetChild(uint32 index) const;
};

struct DiligentRenderWorld
{
	std::vector<std::unique_ptr<DiligentRenderBlock>> render_blocks;
	std::unordered_map<uint32, std::unique_ptr<DiligentRenderWorld>> world_models;

	bool Load(ILTStream* stream);
	bool SetLightGroupColor(uint32 id, const LTVector& color);
	DiligentRenderBlock* GetRenderBlock(uint32 index) const;
};

struct DiligentWorldVertex
{
	float position[3];
	float color[4];
	float uv0[2];
	float uv1[2];
};

struct DiligentWorldConstants
{
	std::array<float, 16> mvp{};
	std::array<float, 16> view{};
	std::array<float, 16> world{};
	float camera_pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float fog_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float fog_params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float dynamic_light_pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float dynamic_light_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DiligentModelConstants
{
	std::array<float, 16> mvp{};
	std::array<float, 16> model{};
	float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float camera_pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float fog_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float fog_params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DiligentWorldResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_textured;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_glow;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_lightmap;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_lightmap_only;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_dual;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_lightmap_dual;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_solid;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_dynamic_light;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_bump;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> index_buffer;
	uint32 vertex_buffer_size = 0;
	uint32 index_buffer_size = 0;
};

struct DiligentModelResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_textured;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_solid;
	std::unordered_map<uint32, Diligent::RefCntAutoPtr<Diligent::IShader>> vertex_shaders;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
};

struct DiligentWorldPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct DiligentModelPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
	bool uses_texture = false;
};

uint64 diligent_hash_combine(uint64 seed, uint64 value);

struct DiligentMeshLayout
{
	std::vector<Diligent::LayoutElement> elements;
	std::array<uint32, 4> strides{};
	std::array<int32, 4> uv_attrib = { -1, -1, -1, -1 };
	int32 position_attrib = -1;
	int32 weights_attrib = -1;
	int32 indices_attrib = -1;
	int32 normal_attrib = -1;
	int32 color_attrib = -1;
	uint32 hash = 0;
};

uint32 diligent_value_type_size(Diligent::VALUE_TYPE value_type)
{
	switch (value_type)
	{
		case Diligent::VT_FLOAT32:
			return sizeof(float);
		case Diligent::VT_UINT8:
			return sizeof(uint8);
		default:
			return 0;
	}
}

uint32 diligent_append_layout_element(
	std::vector<Diligent::LayoutElement>& elements,
	uint32& attrib_index,
	uint32 stream_index,
	uint32 num_components,
	Diligent::VALUE_TYPE value_type,
	bool normalized,
	uint32& offset)
{
	const uint32 assigned_index = attrib_index;
	Diligent::LayoutElement element{attrib_index, stream_index, num_components, value_type, normalized};
	element.RelativeOffset = offset;
	elements.push_back(element);
	offset += diligent_value_type_size(value_type) * num_components;
	++attrib_index;
	return assigned_index;
}

uint32 diligent_get_uv_set_count(uint32 vert_flags)
{
	if (vert_flags & VERTDATATYPE_UVSETS_4)
	{
		return 4;
	}
	if (vert_flags & VERTDATATYPE_UVSETS_3)
	{
		return 3;
	}
	if (vert_flags & VERTDATATYPE_UVSETS_2)
	{
		return 2;
	}
	if (vert_flags & VERTDATATYPE_UVSETS_1)
	{
		return 1;
	}
	return 0;
}

uint32 diligent_get_blend_weight_count(VERTEX_BLEND_TYPE blend_type)
{
	switch (blend_type)
	{
		case eNONINDEXED_B1:
		case eINDEXED_B1:
			return 1;
		case eNONINDEXED_B2:
		case eINDEXED_B2:
			return 2;
		case eNONINDEXED_B3:
		case eINDEXED_B3:
			return 3;
		default:
			return 0;
	}
}

bool diligent_build_stream_layout(
	uint32 vert_flags,
	VERTEX_BLEND_TYPE blend_type,
	uint32 stream_index,
	uint32& attrib_index,
	DiligentMeshLayout& layout,
	bool& non_fixed_pipe_data)
{
	if (!vert_flags)
	{
		layout.strides[stream_index] = 0;
		return false;
	}

	uint32 offset = 0;

	if (vert_flags & VERTDATATYPE_POSITION)
	{
		diligent_append_layout_element(
			layout.elements,
			attrib_index,
			stream_index,
			3,
			Diligent::VT_FLOAT32,
			false,
			offset);
		if (layout.position_attrib < 0)
		{
			layout.position_attrib = static_cast<int32>(attrib_index - 1);
		}

		const uint32 weight_count = diligent_get_blend_weight_count(blend_type);
		if (weight_count > 0)
		{
			const uint32 weight_attrib = diligent_append_layout_element(
				layout.elements,
				attrib_index,
				stream_index,
				weight_count,
				Diligent::VT_FLOAT32,
				false,
				offset);
			if (layout.weights_attrib < 0)
			{
				layout.weights_attrib = static_cast<int32>(weight_attrib);
			}
		}

		if (blend_type == eINDEXED_B1 || blend_type == eINDEXED_B2 || blend_type == eINDEXED_B3)
		{
			const uint32 indices_attrib = diligent_append_layout_element(
				layout.elements,
				attrib_index,
				stream_index,
				4,
				Diligent::VT_UINT8,
				false,
				offset);
			if (layout.indices_attrib < 0)
			{
				layout.indices_attrib = static_cast<int32>(indices_attrib);
			}
		}
	}

	if (vert_flags & VERTDATATYPE_NORMAL)
	{
		diligent_append_layout_element(
			layout.elements,
			attrib_index,
			stream_index,
			3,
			Diligent::VT_FLOAT32,
			false,
			offset);
		if (layout.normal_attrib < 0)
		{
			layout.normal_attrib = static_cast<int32>(attrib_index - 1);
		}
	}

	if (vert_flags & VERTDATATYPE_DIFFUSE)
	{
		diligent_append_layout_element(
			layout.elements,
			attrib_index,
			stream_index,
			4,
			Diligent::VT_UINT8,
			true,
			offset);
		if (layout.color_attrib < 0)
		{
			layout.color_attrib = static_cast<int32>(attrib_index - 1);
		}
	}

	if (vert_flags & VERTDATATYPE_PSIZE)
	{
		diligent_append_layout_element(
			layout.elements,
			attrib_index,
			stream_index,
			1,
			Diligent::VT_FLOAT32,
			false,
			offset);
	}

	const uint32 uv_set_count = diligent_get_uv_set_count(vert_flags);
	for (uint32 uv_index = 0; uv_index < uv_set_count; ++uv_index)
	{
		diligent_append_layout_element(
			layout.elements,
			attrib_index,
			stream_index,
			2,
			Diligent::VT_FLOAT32,
			false,
			offset);
		if (uv_index < layout.uv_attrib.size() && layout.uv_attrib[uv_index] < 0)
		{
			layout.uv_attrib[uv_index] = static_cast<int32>(attrib_index - 1);
		}
	}

	if (vert_flags & VERTDATATYPE_BASISVECTORS)
	{
		diligent_append_layout_element(
			layout.elements,
			attrib_index,
			stream_index,
			3,
			Diligent::VT_FLOAT32,
			false,
			offset);
		diligent_append_layout_element(
			layout.elements,
			attrib_index,
			stream_index,
			3,
			Diligent::VT_FLOAT32,
			false,
			offset);
		non_fixed_pipe_data = true;
	}

	layout.strides[stream_index] = offset;
	return offset > 0;
}

bool diligent_build_mesh_layout(
	const uint32* vert_flags,
	VERTEX_BLEND_TYPE blend_type,
	DiligentMeshLayout& layout,
	bool& non_fixed_pipe_data)
{
	layout.elements.clear();
	layout.strides.fill(0);
	layout.uv_attrib = { -1, -1, -1, -1 };
	layout.position_attrib = -1;
	layout.weights_attrib = -1;
	layout.indices_attrib = -1;
	layout.normal_attrib = -1;
	layout.color_attrib = -1;
	non_fixed_pipe_data = false;

	uint32 attrib_index = 0;
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		diligent_build_stream_layout(vert_flags[stream_index], blend_type, stream_index, attrib_index, layout, non_fixed_pipe_data);
	}

	uint64 hash = diligent_hash_combine(0, static_cast<uint64>(blend_type));
	for (uint32 stream_index = 0; stream_index < 4; ++stream_index)
	{
		hash = diligent_hash_combine(hash, vert_flags[stream_index]);
	}
	layout.hash = static_cast<uint32>(hash);

	return !layout.elements.empty();
}

struct DiligentVertexElementRef
{
	uint32 stream_index = 0;
	uint32 offset = 0;
	uint32 stride = 0;
	bool valid = false;
};

bool diligent_get_layout_element_ref(const DiligentMeshLayout& layout, int32 attrib, DiligentVertexElementRef& ref)
{
	ref = {};
	if (attrib < 0)
	{
		return false;
	}

	for (const auto& element : layout.elements)
	{
		if (element.InputIndex == static_cast<uint32>(attrib))
		{
			ref.stream_index = element.BufferSlot;
			ref.offset = element.RelativeOffset;
			ref.stride = layout.strides[element.BufferSlot];
			ref.valid = ref.stride > 0;
			return ref.valid;
		}
	}

	return false;
}

std::string diligent_hlsl_type_for_element(const Diligent::LayoutElement& element)
{
	const auto components = element.NumComponents;
	const bool normalized = element.IsNormalized;
	if (element.ValueType == Diligent::VT_UINT8)
	{
		const char* base = normalized ? "float" : "uint";
		return components == 1 ? base : std::string(base) + std::to_string(components);
	}

	return components == 1 ? "float" : std::string("float") + std::to_string(components);
}

std::string diligent_attribute_ref(int32 attrib, const char* swizzle)
{
	return "input.attr" + std::to_string(attrib) + swizzle;
}

std::string diligent_build_model_vertex_shader_source(const DiligentMeshLayout& layout)
{
	std::string source;
	source += "cbuffer ModelConstants\n";
	source += "{\n";
	source += "    float4x4 g_MVP;\n";
	source += "    float4x4 g_Model;\n";
	source += "    float4 g_Color;\n";
	source += "    float4 g_CameraPos;\n";
	source += "    float4 g_FogColor;\n";
	source += "    float4 g_FogParams;\n";
	source += "};\n";
	source += "struct VSInput\n";
	source += "{\n";
	for (const auto& element : layout.elements)
	{
		source += "    ";
		source += diligent_hlsl_type_for_element(element);
		source += " attr";
		source += std::to_string(element.InputIndex);
		source += " : ATTRIB";
		source += std::to_string(element.InputIndex);
		source += ";\n";
	}
	source += "};\n";
	source += "struct VSOutput\n";
	source += "{\n";
	source += "    float4 position : SV_POSITION;\n";
	source += "    float4 color : COLOR0;\n";
	source += "    float2 uv : TEXCOORD0;\n";
	source += "    float3 world_pos : TEXCOORD1;\n";
	source += "};\n";
	source += "VSOutput VSMain(VSInput input)\n";
	source += "{\n";
	source += "    VSOutput output;\n";
	source += "    float3 position = ";
	if (layout.position_attrib >= 0)
	{
		source += diligent_attribute_ref(layout.position_attrib, ".xyz");
	}
	else
	{
		source += "float3(0.0f, 0.0f, 0.0f)";
	}
	source += ";\n";
	source += "    float4 vertex_color = ";
	if (layout.color_attrib >= 0)
	{
		source += diligent_attribute_ref(layout.color_attrib, "");
	}
	else
	{
		source += "float4(1.0f, 1.0f, 1.0f, 1.0f)";
	}
	source += ";\n";
	source += "    float2 uv = ";
	if (layout.uv_attrib[0] >= 0)
	{
		source += diligent_attribute_ref(layout.uv_attrib[0], ".xy");
	}
	else
	{
		source += "float2(0.0f, 0.0f)";
	}
	source += ";\n";
	source += "    float4 world_pos = mul(g_Model, float4(position, 1.0f));\n";
	source += "    output.position = mul(g_MVP, float4(position, 1.0f));\n";
	source += "    output.color = vertex_color * g_Color;\n";
	source += "    output.uv = uv;\n";
	source += "    output.world_pos = world_pos.xyz;\n";
	source += "    return output;\n";
	source += "}\n";
	return source;
}

class DiligentRigidMesh : public CDIRigidMesh
{
public:
	DiligentRigidMesh();
	~DiligentRigidMesh() override;

	bool Load(ILTStream& file, LTB_Header& header) override;
	void ReCreateObject() override;
	void FreeDeviceObjects() override;

	uint32 GetVertexCount() override { return m_vertex_count; }
	uint32 GetPolyCount() override { return m_poly_count; }
	uint32 GetBoneEffector() const { return m_bone_effector; }
	uint32 GetIndexCount() const { return m_poly_count * 3; }
	const DiligentMeshLayout& GetLayout() const { return m_layout; }
	Diligent::IBuffer* GetVertexBuffer(uint32 index) const { return index < m_vertex_buffers.size() ? m_vertex_buffers[index].RawPtr() : nullptr; }
	Diligent::IBuffer* GetIndexBuffer() const { return m_index_buffer.RawPtr(); }

private:
	void Reset();
	void FreeAll();

	uint32 m_vertex_count = 0;
	uint32 m_poly_count = 0;
	uint32 m_bone_effector = 0;
	uint32 m_vert_stream_flags[4] = {};
	bool m_non_fixed_pipe_data = false;
	std::array<std::vector<uint8>, 4> m_vertex_data;
	std::vector<uint16> m_index_data;
	std::array<Diligent::RefCntAutoPtr<Diligent::IBuffer>, 4> m_vertex_buffers;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> m_index_buffer;
	DiligentMeshLayout m_layout;
};

struct DiligentBoneSet
{
	uint16 first_vert_index = 0;
	uint16 vert_count = 0;
	uint8 bone_set[4] = {};
	uint32 index_into_index_buffer = 0;
};

struct DiligentDupMap
{
	uint16 src_vert = 0;
	uint16 dst_vert = 0;
};

class DiligentSkelMesh : public CDISkelMesh
{
public:
	DiligentSkelMesh();
	~DiligentSkelMesh() override;

	bool Load(ILTStream& file, LTB_Header& header) override;
	void ReCreateObject() override;
	void FreeDeviceObjects() override;

	uint32 GetVertexCount() override { return m_vertex_count; }
	uint32 GetPolyCount() override { return m_poly_count; }
	uint32 GetIndexCount() const { return m_poly_count * 3; }
	const DiligentMeshLayout& GetLayout() const { return m_layout; }
	Diligent::IBuffer* GetVertexBuffer(uint32 index) const { return index < m_vertex_buffers.size() ? m_vertex_buffers[index].RawPtr() : nullptr; }
	Diligent::IBuffer* GetIndexBuffer() const { return m_index_buffer.RawPtr(); }

	bool UpdateSkinnedVertices(ModelInstance* instance);

private:
	enum RenderMethod
	{
		kRenderDirect,
		kRenderMatrixPalettes
	};

	bool LoadDirect(ILTStream& file);
	bool LoadMatrixPalettes(ILTStream& file);
	void Reset();
	void FreeAll();
	void UpdateLayoutRefs();
	bool UpdateSkinnedVerticesDirect(const std::vector<LTMatrix>& bone_transforms);
	bool UpdateSkinnedVerticesIndexed(const std::vector<LTMatrix>& bone_transforms);

	RenderMethod m_render_method = kRenderDirect;
	uint32 m_vertex_count = 0;
	uint32 m_poly_count = 0;
	uint32 m_max_bones_per_vert = 0;
	uint32 m_max_bones_per_tri = 0;
	uint32 m_min_bone = 0;
	uint32 m_max_bone = 0;
	uint32 m_weight_count = 0;
	VERTEX_BLEND_TYPE m_vert_type = eNO_WORLD_BLENDS;
	uint32 m_vert_stream_flags[4] = {};
	bool m_non_fixed_pipe_data = false;
	bool m_reindexed_bones = false;
	std::vector<uint32> m_reindexed_bone_list;
	std::vector<DiligentBoneSet> m_bone_sets;
	std::array<std::vector<uint8>, 4> m_vertex_data;
	std::vector<uint16> m_index_data;
	std::array<Diligent::RefCntAutoPtr<Diligent::IBuffer>, 4> m_vertex_buffers;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> m_index_buffer;
	DiligentMeshLayout m_layout;
	DiligentVertexElementRef m_position_ref;
	DiligentVertexElementRef m_normal_ref;
	DiligentVertexElementRef m_weights_ref;
	DiligentVertexElementRef m_indices_ref;
	std::array<bool, 4> m_dynamic_streams{};
	std::vector<LTMatrix> m_bone_transforms;
};

class DiligentVAMesh : public CDIVAMesh
{
public:
	DiligentVAMesh();
	~DiligentVAMesh() override;

	bool Load(ILTStream& file, LTB_Header& header) override;
	void ReCreateObject() override;
	void FreeDeviceObjects() override;

	uint32 GetVertexCount() override { return m_vertex_count; }
	uint32 GetPolyCount() override { return m_poly_count; }
	uint32 GetIndexCount() const { return m_poly_count * 3; }
	uint32 GetBoneEffector() const { return m_bone_effector; }
	const DiligentMeshLayout& GetLayout() const { return m_layout; }
	Diligent::IBuffer* GetVertexBuffer(uint32 index) const { return index < m_vertex_buffers.size() ? m_vertex_buffers[index].RawPtr() : nullptr; }
	Diligent::IBuffer* GetIndexBuffer() const { return m_index_buffer.RawPtr(); }

	bool UpdateVA(Model* model, AnimTimeRef* anim_time);

private:
	void Reset();
	void FreeAll();
	void UpdateLayoutRefs();

	uint32 m_vertex_count = 0;
	uint32 m_undup_vertex_count = 0;
	uint32 m_poly_count = 0;
	uint32 m_bone_effector = 0;
	uint32 m_anim_node_idx = 0;
	uint32 m_max_bones_per_vert = 0;
	uint32 m_max_bones_per_tri = 0;
	uint32 m_vert_stream_flags[4] = {};
	bool m_non_fixed_pipe_data = false;
	std::vector<DiligentDupMap> m_dup_map_list;
	std::array<std::vector<uint8>, 4> m_vertex_data;
	std::vector<uint16> m_index_data;
	std::array<Diligent::RefCntAutoPtr<Diligent::IBuffer>, 4> m_vertex_buffers;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> m_index_buffer;
	DiligentMeshLayout m_layout;
	DiligentVertexElementRef m_position_ref;
	std::array<bool, 4> m_dynamic_streams{};
};

enum DiligentWorldPipelineMode : uint8
{
	kWorldPipelineSolid = 0,
	kWorldPipelineTextured = 1,
	kWorldPipelineGlowTextured = 2,
	kWorldPipelineLightmap = 3,
	kWorldPipelineLightmapOnly = 4,
	kWorldPipelineDualTexture = 5,
	kWorldPipelineLightmapDual = 6,
	kWorldPipelineDynamicLight = 7,
	kWorldPipelineBump = 8
};

enum DiligentWorldBlendMode : uint8
{
	kWorldBlendSolid = 0,
	kWorldBlendAlpha = 1,
	kWorldBlendAdditive = 2,
	kWorldBlendAdditiveOne = 3,
	kWorldBlendMultiply = 4
};

enum DiligentWorldDepthMode : uint8
{
	kWorldDepthEnabled = 0,
	kWorldDepthDisabled = 1
};

enum DiligentPCShaderType : uint8
{
	kPcShaderNone = 0,
	kPcShaderGouraud = 1,
	kPcShaderLightmap = 2,
	kPcShaderLightmapTexture = 4,
	kPcShaderSkypan = 5,
	kPcShaderSkyPortal = 6,
	kPcShaderOccluder = 7,
	kPcShaderDualTexture = 8,
	kPcShaderLightmapDualTexture = 9,
	kPcShaderUnknown = 10
};

constexpr uint8 kWorldPipelineSkip = 0xFFu;

struct DiligentWorldPipelineKey
{
	uint8 mode = kWorldPipelineSolid;
	uint8 blend_mode = kWorldBlendSolid;
	uint8 depth_mode = kWorldDepthEnabled;

	bool operator==(const DiligentWorldPipelineKey& other) const
	{
		return mode == other.mode && blend_mode == other.blend_mode && depth_mode == other.depth_mode;
	}
};

struct DiligentWorldPipelineKeyHash
{
	size_t operator()(const DiligentWorldPipelineKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, key.mode);
		hash = diligent_hash_combine(hash, key.blend_mode);
		hash = diligent_hash_combine(hash, key.depth_mode);
		return static_cast<size_t>(hash);
	}
};

struct DiligentWorldPipelines
{
	std::unordered_map<DiligentWorldPipelineKey, DiligentWorldPipeline, DiligentWorldPipelineKeyHash> pipelines;
};

DiligentWorldResources g_world_resources;
DiligentModelResources g_model_resources;
DiligentWorldPipelines g_world_pipelines;
DiligentOptimized2DResources g_optimized_2d_resources;
Diligent::RefCntAutoPtr<Diligent::ITexture> g_debug_white_texture;
Diligent::RefCntAutoPtr<Diligent::ITextureView> g_debug_white_texture_srv;
std::unordered_map<HLTBUFFER, DiligentSurfaceGpuData> g_surface_cache;
std::unique_ptr<DiligentRenderWorld> g_render_world;
std::vector<const DiligentRenderBlock*> g_visible_render_blocks;
std::vector<WorldModelInstance*> g_diligent_solid_world_models;
std::vector<WorldModelInstance*> g_diligent_translucent_world_models;
std::vector<SpriteInstance*> g_diligent_translucent_sprites;
std::vector<SpriteInstance*> g_diligent_noz_sprites;
std::vector<LTPolyGrid*> g_diligent_solid_polygrids;
std::vector<LTPolyGrid*> g_diligent_early_translucent_polygrids;
std::vector<LTPolyGrid*> g_diligent_translucent_polygrids;
CRenderObject* g_render_object_list_head = nullptr;
uint32 g_diligent_object_frame_code = 0;
LTSurfaceBlend g_optimized_2d_blend = LTSURFACEBLEND_ALPHA;
HLTCOLOR g_optimized_2d_color = 0xFFFFFFFF;

constexpr uint32 kDiligentMaxDynamicLights = 64;
DynamicLight* g_diligent_world_dynamic_lights[kDiligentMaxDynamicLights] = {};
uint32 g_diligent_num_world_dynamic_lights = 0;

constexpr uint32 kDiligentGlowMinTextureSize = 16;
constexpr uint32 kDiligentGlowMaxTextureSize = 512;
constexpr uint32 kDiligentGlowMaxFilterSize = 64;
constexpr uint32 kDiligentGlowBlurBatchSize = 8;
constexpr float kDiligentGlowMinElementWeight = 0.01f;
static_assert(kDiligentGlowBlurBatchSize == 8, "Update glow blur shader tap count.");

struct DiligentGlowBlurConstants
{
	std::array<std::array<float, 4>, kDiligentGlowBlurBatchSize> taps{};
	float params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DiligentGlowElement
{
	float tex_offset = 0.0f;
	float weight = 0.0f;
};

struct DiligentGlowState
{
	CRenderStyleMap render_style_map;
	std::unique_ptr<CRenderTarget> glow_target;
	std::unique_ptr<CRenderTarget> blur_target;
	uint32 texture_width = 0;
	uint32 texture_height = 0;
	std::array<DiligentGlowElement, kDiligentGlowMaxFilterSize> filter{};
	uint32 filter_size = 0;
};

DiligentGlowState g_diligent_glow_state;

struct DiligentGlowBlurPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct DiligentGlowBlurResources
{
	Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	DiligentGlowBlurPipeline pipeline_solid;
	DiligentGlowBlurPipeline pipeline_add;
};

DiligentGlowBlurResources g_diligent_glow_blur_resources;

struct DiligentSceneDescScope
{
	SceneDesc*& target;
	SceneDesc* previous;

	DiligentSceneDescScope(SceneDesc*& target, SceneDesc* value)
		: target(target), previous(target)
	{
		target = value;
	}

	~DiligentSceneDescScope()
	{
		target = previous;
	}
};

Diligent::RefCntAutoPtr<Diligent::IRenderDevice> g_render_device;
Diligent::RefCntAutoPtr<Diligent::IDeviceContext> g_immediate_context;
Diligent::RefCntAutoPtr<Diligent::ISwapChain> g_swap_chain;
RenderStruct* g_render_struct = nullptr;
SceneDesc* g_diligent_scene_desc = nullptr;
uint32 g_screen_width = 0;
uint32 g_screen_height = 0;
ViewParams g_ViewParams;
bool g_is_in_3d = false;
bool g_is_in_optimized_2d = false;
bool g_diligent_glow_mode = false;
Diligent::IEngineFactoryVk* g_engine_factory = nullptr;
HWND g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
SDL_Window* g_sdl_window = nullptr;
#endif

bool d3d_InitFrustum(
	ViewParams* params,
	float x_fov,
	float y_fov,
	float near_z,
	float far_z,
	float screen_min_x,
	float screen_min_y,
	float screen_max_x,
	float screen_max_y,
	const LTVector* position,
	const LTRotation* rotation,
	ViewParams::ERenderMode render_mode);

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_buffer(
	uint32 size,
	Diligent::BIND_FLAGS bind_flags,
	const void* data,
	uint32 stride);

DiligentRBSection::DiligentRBSection()
	: texture_effect(nullptr),
	  shader_code(0),
	  fullbright(false),
	  start_index(0),
	  tri_count(0),
	  start_vertex(0),
	  vertex_count(0),
	  lightmap_width(0),
	  lightmap_height(0),
	  lightmap_size(0)
{
	for (uint32 i = 0; i < kNumTextures; ++i)
	{
		textures[i] = nullptr;
		sprite_data[i] = nullptr;
	}
}

DiligentRBSection::~DiligentRBSection()
{
	for (uint32 i = 0; i < kNumTextures; ++i)
	{
		if (textures[i])
		{
			textures[i]->SetRefCount(textures[i]->GetRefCount() - 1);
			textures[i] = nullptr;
		}
		delete sprite_data[i];
		sprite_data[i] = nullptr;
	}

	if (texture_effect)
	{
		CTextureScriptMgr::GetSingleton().ReleaseInstance(texture_effect);
		texture_effect = nullptr;
	}
}

bool DiligentRBLightGroup::SubLightmap::Load(ILTStream& stream)
{
	stream >> left;
	stream >> top;
	stream >> width;
	stream >> height;

	uint32 data_size = 0;
	stream >> data_size;
	data.resize(data_size);
	if (!data.empty())
	{
		stream.Read(data.data(), data_size);
	}

	return true;
}

void DiligentRBLightGroup::ClearSectionLightmaps()
{
	section_lightmaps.clear();
}

ILTStream& operator>>(ILTStream& stream, DiligentRBGeometryPoly& poly)
{
	uint8 vert_count = 0;
	stream >> vert_count;

	poly.vertices.clear();
	poly.vertices.reserve(vert_count);
	poly.min.Init(FLT_MAX, FLT_MAX, FLT_MAX);
	poly.max.Init(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (; vert_count; --vert_count)
	{
		LTVector position;
		stream >> position;
		poly.vertices.push_back(position);
		VEC_MIN(poly.min, poly.min, position);
		VEC_MAX(poly.max, poly.max, position);
	}

	stream >> poly.plane.m_Normal;
	stream >> poly.plane.m_Dist;
	poly.plane_corner = GetAABBPlaneCorner(poly.plane.m_Normal);

	return stream;
}

ILTStream& operator>>(ILTStream& stream, DiligentRBOccluder& poly)
{
	stream >> static_cast<DiligentRBGeometryPoly&>(poly);
	stream >> poly.id;
	return stream;
}

ILTStream& operator>>(ILTStream& stream, DiligentRBLightGroup& light_group)
{
	uint16 length = 0;
	stream >> length;
	light_group.id = 0;
	for (; length; --length)
	{
		uint8 next_char = 0;
		stream >> next_char;
		light_group.id *= 31;
		light_group.id += static_cast<uint32>(next_char);
	}

	stream >> light_group.color;

	uint32 data_length = 0;
	stream >> data_length;
	light_group.vertex_intensities.assign(data_length, 0);
	if (!light_group.vertex_intensities.empty())
	{
		stream.Read(light_group.vertex_intensities.data(), data_length);
	}

	uint32 section_lightmap_count = 0;
	stream >> section_lightmap_count;
	light_group.section_lightmaps.clear();
	light_group.section_lightmaps.resize(section_lightmap_count);
	for (uint32 section_index = 0; section_index < section_lightmap_count; ++section_index)
	{
		uint32 sub_lightmap_count = 0;
		stream >> sub_lightmap_count;
		if (!sub_lightmap_count)
		{
			continue;
		}

		auto& section_lightmaps = light_group.section_lightmaps[section_index];
		section_lightmaps.resize(sub_lightmap_count);
		for (auto& sub_lightmap : section_lightmaps)
		{
			sub_lightmap.Load(stream);
		}
	}

	return stream;
}

bool diligent_is_sprite_texture(const char* filename)
{
	if (!filename)
	{
		return false;
	}

	const uint32 name_length = static_cast<uint32>(strlen(filename));
	if (name_length < 4)
	{
		return false;
	}

	return _stricmp(&filename[name_length - 4], ".spr") == 0;
}

SharedTexture* diligent_load_sprite_data(DiligentRBSection& section, uint32 texture_index, const char* filename)
{
	auto* sprite_data = new DiligentRBSpriteData;
	if (!sprite_data)
	{
		return nullptr;
	}

	FileRef file_ref;
	file_ref.m_FileType = FILE_CLIENTFILE;
	file_ref.m_pFilename = filename;

	if (LoadSprite(&file_ref, &sprite_data->sprite) != LT_OK)
	{
		delete sprite_data;
		return nullptr;
	}

	spr_InitTracker(&sprite_data->tracker, sprite_data->sprite);
	SharedTexture* result = nullptr;
	if (sprite_data->tracker.m_pCurFrame)
	{
		result = sprite_data->tracker.m_pCurFrame->m_pTex;
	}

	section.sprite_data[texture_index] = sprite_data;
	return result;
}

bool diligent_upload_lightmap_texture(DiligentRBSection& section, const std::vector<uint8>& rgb_data)
{
	if (!g_render_device)
	{
		return false;
	}

	if (section.lightmap_width == 0 || section.lightmap_height == 0)
	{
		return false;
	}

	const uint32 pixel_count = section.lightmap_width * section.lightmap_height;
	if (rgb_data.size() < pixel_count * 3)
	{
		return false;
	}

	std::vector<uint8> rgba(pixel_count * 4);
	for (uint32 i = 0; i < pixel_count; ++i)
	{
		rgba[i * 4] = rgb_data[i * 3];
		rgba[i * 4 + 1] = rgb_data[i * 3 + 1];
		rgba[i * 4 + 2] = rgb_data[i * 3 + 2];
		rgba[i * 4 + 3] = 255;
	}

	const Diligent::Uint64 stride = static_cast<Diligent::Uint64>(section.lightmap_width * 4);

	if (section.lightmap_texture && g_immediate_context)
	{
		Diligent::Box dst_box{0, section.lightmap_width, 0, section.lightmap_height};
		Diligent::TextureSubResData subresource;
		subresource.pData = rgba.data();
		subresource.Stride = stride;
		subresource.DepthStride = 0;

		g_immediate_context->UpdateTexture(
			section.lightmap_texture,
			0,
			0,
			dst_box,
			subresource,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		if (!section.lightmap_srv)
		{
			section.lightmap_srv = section.lightmap_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		return section.lightmap_srv != nullptr;
	}

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_lightmap";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = section.lightmap_width;
	desc.Height = section.lightmap_height;
	desc.MipLevels = 1;
	desc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
	desc.Usage = Diligent::USAGE_DEFAULT;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	Diligent::TextureSubResData subresource;
	subresource.pData = rgba.data();
	subresource.Stride = stride;
	subresource.DepthStride = 0;

	Diligent::TextureData init_data{&subresource, 1};
	g_render_device->CreateTexture(desc, &init_data, &section.lightmap_texture);
	if (!section.lightmap_texture)
	{
		return false;
	}

	section.lightmap_srv = section.lightmap_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	return section.lightmap_srv != nullptr;
}

Diligent::ITextureView* diligent_get_lightmap_view(DiligentRBSection& section)
{
	if (section.lightmap_srv)
	{
		return section.lightmap_srv;
	}

	if (section.lightmap_data.empty() || section.lightmap_width == 0 || section.lightmap_height == 0)
	{
		return nullptr;
	}

	const uint32 pixel_count = section.lightmap_width * section.lightmap_height;
	std::vector<uint8> decompressed(pixel_count * 3);
	if (!DecompressLMData(section.lightmap_data.data(), section.lightmap_size, decompressed.data()))
	{
		return nullptr;
	}

	if (!diligent_upload_lightmap_texture(section, decompressed))
	{
		return nullptr;
	}

	return section.lightmap_srv;
}

bool DiligentRenderBlock::Load(ILTStream* stream)
{
	if (!stream)
	{
		return false;
	}

	*stream >> center;
	*stream >> half_dims;
	bounds_min = center - half_dims;
	bounds_max = center + half_dims;

	uint32 section_count = 0;
	uint32 index_offset = 0;
	*stream >> section_count;
	sections.clear();
	sections.reserve(section_count);

	for (uint32 section_index = 0; section_index < section_count; ++section_index)
	{
		char texture_names[DiligentRBSection::kNumTextures][MAX_PATH + 1] = {};
		for (uint32 tex_index = 0; tex_index < DiligentRBSection::kNumTextures; ++tex_index)
		{
			stream->ReadString(texture_names[tex_index], sizeof(texture_names[tex_index]));
		}

		uint8 shader_code = 0;
		*stream >> shader_code;
		uint32 tri_count = 0;
		*stream >> tri_count;

		char texture_effect[MAX_PATH + 1] = {};
		stream->ReadString(texture_effect, sizeof(texture_effect));

		sections.emplace_back(new DiligentRBSection());
		auto& section = *sections.back();
		section.start_index = index_offset;
		section.tri_count = tri_count;
		section.shader_code = shader_code;
		index_offset += tri_count * 3;

		for (uint32 tex_index = 0; tex_index < DiligentRBSection::kNumTextures; ++tex_index)
		{
			SharedTexture* texture = nullptr;
			if (diligent_is_sprite_texture(texture_names[tex_index]))
			{
				texture = diligent_load_sprite_data(section, tex_index, texture_names[tex_index]);
			}
			else if (texture_names[tex_index][0] && g_render_struct)
			{
				texture = g_render_struct->GetSharedTexture(texture_names[tex_index]);
			}
			section.textures[tex_index] = texture;
			if (texture)
			{
				texture->SetRefCount(texture->GetRefCount() + 1);
			}
		}

		section.fullbright = false;
		if (section.textures[0] && g_render_struct)
		{
			TextureData* texture_data = g_render_struct->GetTexture(section.textures[0]);
			if (texture_data && (texture_data->m_Header.m_IFlags & DTX_FULLBRITE) != 0)
			{
				section.fullbright = true;
			}
		}

		*stream >> section.lightmap_width;
		*stream >> section.lightmap_height;
		*stream >> section.lightmap_size;
		if (section.lightmap_size)
		{
			section.lightmap_data.resize(section.lightmap_size);
			stream->Read(section.lightmap_data.data(), section.lightmap_size);
		}

		if (texture_effect[0] != '\0')
		{
			section.texture_effect = CTextureScriptMgr::GetSingleton().GetInstance(texture_effect);
		}
	}

	uint32 vertex_count = 0;
	*stream >> vertex_count;
	vertices.clear();
	vertices.resize(vertex_count);
	if (!vertices.empty())
	{
		stream->Read(vertices.data(), sizeof(DiligentRBVertex) * vertices.size());
	}

	uint32 tri_count = 0;
	*stream >> tri_count;
	indices.clear();
	indices.resize(tri_count * 3);
	if (!indices.empty())
	{
		uint32 section_index = 0;
		uint32 section_left = section_count ? sections[0]->tri_count : 0;
		uint32 min_vertex = vertex_count;
		uint32 max_vertex = 0;
		uint32 index_offset2 = 0;

		for (uint32 tri_index = 0; tri_index < tri_count; ++tri_index)
		{
			uint32 index0 = 0;
			uint32 index1 = 0;
			uint32 index2 = 0;
			*stream >> index0 >> index1 >> index2;
			indices[index_offset2++] = static_cast<uint16>(index0);
			indices[index_offset2++] = static_cast<uint16>(index1);
			indices[index_offset2++] = static_cast<uint16>(index2);

			uint32 poly_index = 0;
			*stream >> poly_index;

			min_vertex = LTMIN(min_vertex, index0);
			min_vertex = LTMIN(min_vertex, index1);
			min_vertex = LTMIN(min_vertex, index2);
			max_vertex = LTMAX(max_vertex, index0);
			max_vertex = LTMAX(max_vertex, index1);
			max_vertex = LTMAX(max_vertex, index2);

			if (section_count)
			{
				--section_left;
				if (!section_left && section_index < section_count)
				{
					sections[section_index]->start_vertex = min_vertex;
					sections[section_index]->vertex_count = (max_vertex - min_vertex) + 1;
					++section_index;
					min_vertex = vertex_count;
					max_vertex = 0;
					if (section_index < section_count)
					{
						section_left = sections[section_index]->tri_count;
					}
				}
			}
		}
	}

	uint32 sky_portal_count = 0;
	*stream >> sky_portal_count;
	sky_portals.clear();
	sky_portals.reserve(sky_portal_count);
	for (; sky_portal_count; --sky_portal_count)
	{
		DiligentRBGeometryPoly poly;
		*stream >> poly;
		sky_portals.push_back(poly);
	}

	uint32 occluder_count = 0;
	*stream >> occluder_count;
	occluders.clear();
	occluders.reserve(occluder_count);
	for (; occluder_count; --occluder_count)
	{
		DiligentRBOccluder poly;
		*stream >> poly;
		occluders.push_back(poly);
	}

	uint32 light_group_count = 0;
	*stream >> light_group_count;
	light_groups.clear();
	light_groups.reserve(light_group_count);
	for (; light_group_count; --light_group_count)
	{
		DiligentRBLightGroup light_group;
		*stream >> light_group;
		light_groups.push_back(light_group);
	}

	uint8 child_flags = 0;
	*stream >> child_flags;
	uint8 mask = 1;
	for (uint32 child_index = 0; child_index < kNumChildren; ++child_index)
	{
		uint32 index = 0;
		*stream >> index;
		if (child_flags & mask)
		{
			child_indices[child_index] = index;
		}
		else
		{
			child_indices[child_index] = kInvalidChild;
		}
		mask <<= 1;
	}

	return true;
}

DiligentRenderBlock* DiligentRenderBlock::GetChild(uint32 index) const
{
	if (!world || index >= kNumChildren)
	{
		return nullptr;
	}

	const uint32 child_index = child_indices[index];
	if (child_index == kInvalidChild)
	{
		return nullptr;
	}

	return world->GetRenderBlock(child_index);
}

void DiligentRenderBlock::ExtendSkyBounds(const ViewParams& params, float& min_x, float& min_y, float& max_x, float& max_y) const
{
	for (const auto& poly : sky_portals)
	{
		if (poly.vertices.empty())
		{
			continue;
		}

		float local_min_x = FLT_MAX;
		float local_min_y = FLT_MAX;
		float local_max_x = -FLT_MAX;
		float local_max_y = -FLT_MAX;

		LTVector prev_vert = poly.vertices.back();
		float prev_near_dist = params.m_ClipPlanes[CPLANE_NEAR_INDEX].DistTo(prev_vert);
		bool prev_clip = prev_near_dist > 0.0f;

		for (const auto& raw_vert : poly.vertices)
		{
			LTVector cur_vert = raw_vert;
			float cur_near_dist = params.m_ClipPlanes[CPLANE_NEAR_INDEX].DistTo(cur_vert);
			bool cur_clip = cur_near_dist > 0.0f;

			if (cur_clip != prev_clip)
			{
				const float denom = cur_near_dist - prev_near_dist;
				const float interpolant = (denom != 0.0f) ? -prev_near_dist / denom : 0.0f;
				LTVector temp;
				VEC_LERP(temp, prev_vert, cur_vert, interpolant);
				cur_vert = temp;
			}

			MatVMul_InPlace_H(&(params.m_FullTransform), &cur_vert);

			local_min_x = LTMIN(local_min_x, cur_vert.x);
			local_min_y = LTMIN(local_min_y, cur_vert.y);
			local_max_x = LTMAX(local_max_x, cur_vert.x);
			local_max_y = LTMAX(local_max_y, cur_vert.y);

			prev_vert = raw_vert;
			prev_near_dist = cur_near_dist;
			prev_clip = cur_clip;
		}

		if ((local_max_x < static_cast<float>(params.m_Rect.left)) ||
			(local_max_y < static_cast<float>(params.m_Rect.top)) ||
			(local_min_x > static_cast<float>(params.m_Rect.right)) ||
			(local_min_y > static_cast<float>(params.m_Rect.bottom)))
		{
			continue;
		}

		local_min_x = LTMAX(static_cast<float>(params.m_Rect.left), local_min_x);
		local_min_y = LTMAX(static_cast<float>(params.m_Rect.top), local_min_y);
		local_max_x = LTMIN(static_cast<float>(params.m_Rect.right), local_max_x);
		local_max_y = LTMIN(static_cast<float>(params.m_Rect.bottom), local_max_y);

		min_x = LTMIN(min_x, local_min_x);
		min_y = LTMIN(min_y, local_min_y);
		max_x = LTMAX(max_x, local_max_x);
		max_y = LTMAX(max_y, local_max_y);
	}
}

bool DiligentRenderBlock::EnsureGpuBuffers()
{
	if (vertex_buffer && index_buffer)
	{
		return true;
	}

	if (vertices.empty() || indices.empty())
	{
		return false;
	}

	std::vector<DiligentWorldVertex> world_vertices;
	world_vertices.resize(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		const auto& src = vertices[i];
		auto& dst = world_vertices[i];
		dst.position[0] = src.position.x;
		dst.position[1] = src.position.y;
		dst.position[2] = src.position.z;

		const uint32 color = src.color;
		dst.color[0] = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
		dst.color[1] = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
		dst.color[2] = static_cast<float>(color & 0xFF) / 255.0f;
		dst.color[3] = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
		dst.uv0[0] = src.u0;
		dst.uv0[1] = src.v0;
		dst.uv1[0] = src.u1;
		dst.uv1[1] = src.v1;
	}

	const uint32 vertex_bytes = static_cast<uint32>(world_vertices.size() * sizeof(DiligentWorldVertex));
	const uint32 index_bytes = static_cast<uint32>(indices.size() * sizeof(uint16));

	vertex_buffer = diligent_create_buffer(vertex_bytes, Diligent::BIND_VERTEX_BUFFER, world_vertices.data(), sizeof(DiligentWorldVertex));
	index_buffer = diligent_create_buffer(index_bytes, Diligent::BIND_INDEX_BUFFER, indices.data(), sizeof(uint16));
	return vertex_buffer && index_buffer;
}

bool DiligentRenderBlock::UpdateLightmaps()
{
	if (!lightmaps_dirty)
	{
		return false;
	}

	if (!g_render_device)
	{
		return false;
	}

	bool updated = false;
	bool has_lightmaps = false;
	for (size_t section_index = 0; section_index < sections.size(); ++section_index)
	{
		auto& section_ptr = sections[section_index];
		if (!section_ptr)
		{
			continue;
		}

		auto& section = *section_ptr;
		if (section.lightmap_size == 0 || section.lightmap_width == 0 || section.lightmap_height == 0)
		{
			continue;
		}

		has_lightmaps = true;
		const uint32 pixel_count = section.lightmap_width * section.lightmap_height;
		std::vector<uint8> lightmap_rgb(pixel_count * 3);
		if (!DecompressLMData(section.lightmap_data.data(), section.lightmap_size, lightmap_rgb.data()))
		{
			continue;
		}

		const uint32 stride = section.lightmap_width * 3;
		for (const auto& light_group : light_groups)
		{
			if (section_index >= light_group.section_lightmaps.size())
			{
				continue;
			}

			const auto& sub_lightmaps = light_group.section_lightmaps[section_index];
			if (sub_lightmaps.empty())
			{
				continue;
			}

			LTVector max_light_add = light_group.color * 255.0f;
			if (static_cast<uint32>(max_light_add.x) == 0 &&
				static_cast<uint32>(max_light_add.y) == 0 &&
				static_cast<uint32>(max_light_add.z) == 0)
			{
				continue;
			}

			for (const auto& sub_lightmap : sub_lightmaps)
			{
				if (sub_lightmap.data.empty() || sub_lightmap.width == 0 || sub_lightmap.height == 0)
				{
					continue;
				}

				uint8* current_texel = lightmap_rgb.data() + sub_lightmap.top * stride + sub_lightmap.left * 3;
				uint8* end_texel = current_texel + sub_lightmap.height * stride;
				uint32 width_remaining = sub_lightmap.width;
				uint32 stride_skip = stride - sub_lightmap.width * 3;
				size_t input_index = 0;
				uint8 run_length = 0;
				uint8 run_value_r = 0;
				uint8 run_value_g = 0;
				uint8 run_value_b = 0;

				for (; current_texel != end_texel; current_texel += 3)
				{
					if (run_length)
					{
						uint32 color_r = current_texel[0] + run_value_r;
						uint32 color_g = current_texel[1] + run_value_g;
						uint32 color_b = current_texel[2] + run_value_b;
						current_texel[0] = static_cast<uint8>(LTMIN(color_r, 0xFF));
						current_texel[1] = static_cast<uint8>(LTMIN(color_g, 0xFF));
						current_texel[2] = static_cast<uint8>(LTMIN(color_b, 0xFF));
						--run_length;
					}
					else
					{
						if (input_index >= sub_lightmap.data.size())
						{
							break;
						}

						uint8 value = sub_lightmap.data[input_index++];
						if (value == 0xFF)
						{
							if (input_index >= sub_lightmap.data.size())
							{
								break;
							}
							run_length = sub_lightmap.data[input_index++];
							if (input_index >= sub_lightmap.data.size())
							{
								break;
							}
							value = sub_lightmap.data[input_index++];
						}

						LTVector light_add = light_group.color * static_cast<float>(value);
						uint32 color_r = current_texel[0] + static_cast<uint32>(light_add.x);
						uint32 color_g = current_texel[1] + static_cast<uint32>(light_add.y);
						uint32 color_b = current_texel[2] + static_cast<uint32>(light_add.z);
						current_texel[0] = static_cast<uint8>(LTMIN(color_r, 0xFF));
						current_texel[1] = static_cast<uint8>(LTMIN(color_g, 0xFF));
						current_texel[2] = static_cast<uint8>(LTMIN(color_b, 0xFF));

						if (run_length)
						{
							run_value_r = static_cast<uint8>(LTMIN(static_cast<uint32>(light_add.x), 0xFF));
							run_value_g = static_cast<uint8>(LTMIN(static_cast<uint32>(light_add.y), 0xFF));
							run_value_b = static_cast<uint8>(LTMIN(static_cast<uint32>(light_add.z), 0xFF));
						}
					}

					--width_remaining;
					if (!width_remaining)
					{
						current_texel += stride_skip;
						width_remaining = sub_lightmap.width;
					}
				}
			}
		}

		if (diligent_upload_lightmap_texture(section, lightmap_rgb))
		{
			updated = true;
		}
	}

	if (updated || has_lightmaps)
	{
		lightmaps_dirty = false;
	}

	return updated;
}

bool DiligentRenderBlock::SetLightGroupColor(uint32 id, const LTVector& color)
{
	for (auto& light_group : light_groups)
	{
		if (light_group.id == id)
		{
			light_group.color = color;
			lightmaps_dirty = true;
			return true;
		}
	}

	return false;
}

bool DiligentRenderWorld::Load(ILTStream* stream)
{
	if (!stream)
	{
		return false;
	}

	uint32 render_block_count = 0;
	*stream >> render_block_count;

	render_blocks.clear();
	render_blocks.reserve(render_block_count);
	for (uint32 block_index = 0; block_index < render_block_count; ++block_index)
	{
		auto block = std::unique_ptr<DiligentRenderBlock>(new DiligentRenderBlock());
		block->world = this;
		if (!block->Load(stream))
		{
			return false;
		}
		render_blocks.emplace_back(std::move(block));
	}

	uint32 world_model_count = 0;
	*stream >> world_model_count;
	world_models.clear();
	for (uint32 model_index = 0; model_index < world_model_count; ++model_index)
	{
		char world_name[MAX_WORLDNAME_LEN + 1] = {};
		stream->ReadString(world_name, sizeof(world_name));

		auto world_model = std::unique_ptr<DiligentRenderWorld>(new DiligentRenderWorld());
		if (!world_model->Load(stream))
		{
			return false;
		}

		const uint32 name_hash = st_GetHashCode(world_name);
		if (world_models.find(name_hash) == world_models.end())
		{
			world_models.emplace(name_hash, std::move(world_model));
		}
	}

	return true;
}

DiligentRenderBlock* DiligentRenderWorld::GetRenderBlock(uint32 index) const
{
	if (index >= render_blocks.size())
	{
		return nullptr;
	}

	return render_blocks[index].get();
}

bool DiligentRenderWorld::SetLightGroupColor(uint32 id, const LTVector& color)
{
	bool updated = false;
	for (const auto& block : render_blocks)
	{
		if (!block)
		{
			continue;
		}

		if (block->SetLightGroupColor(id, color))
		{
			updated |= block->UpdateLightmaps();
		}
	}

	for (auto& world_model : world_models)
	{
		if (world_model.second)
		{
			updated |= world_model.second->SetLightGroupColor(id, color);
		}
	}

	return updated;
}

struct DiligentPsoKey
{
	uint64 render_pass_hash = 0;
	uint32 input_layout_hash = 0;
	int vertex_shader_id = 0;
	int pixel_shader_id = 0;
	uint32 color_format = 0;
	uint32 depth_format = 0;
	uint32 topology = 0;

	bool operator==(const DiligentPsoKey& other) const
	{
		return render_pass_hash == other.render_pass_hash &&
			input_layout_hash == other.input_layout_hash &&
			vertex_shader_id == other.vertex_shader_id &&
			pixel_shader_id == other.pixel_shader_id &&
			color_format == other.color_format &&
			depth_format == other.depth_format &&
			topology == other.topology;
	}
};

struct DiligentModelPipelineKey
{
	DiligentPsoKey pso_key{};
	bool uses_texture = false;

	bool operator==(const DiligentModelPipelineKey& other) const
	{
		return uses_texture == other.uses_texture && pso_key == other.pso_key;
	}
};

uint64 diligent_hash_combine(uint64 seed, uint64 value)
{
	return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

uint32 diligent_float_bits(float value)
{
	uint32 bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

uint64 diligent_hash_texture_stage(const TextureStageOps& stage)
{
	uint64 hash = 0;
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.TextureParam));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.ColorOp));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.ColorArg1));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.ColorArg2));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.AlphaOp));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.AlphaArg1));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.AlphaArg2));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.UVSource));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.UAddress));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.VAddress));
	hash = diligent_hash_combine(hash, static_cast<uint64>(stage.TexFilter));
	hash = diligent_hash_combine(hash, stage.UVTransform_Enable ? 1u : 0u);
	hash = diligent_hash_combine(hash, stage.ProjectTexCoord ? 1u : 0u);
	hash = diligent_hash_combine(hash, stage.TexCoordCount);
	return hash;
}

uint64 diligent_hash_render_pass(const RenderPassOp& pass)
{
	uint64 hash = 0;
	for (uint32 i = 0; i < 4; ++i)
	{
		hash = diligent_hash_combine(hash, diligent_hash_texture_stage(pass.TextureStages[i]));
	}

	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.BlendMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.ZBufferMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.CullMode));
	hash = diligent_hash_combine(hash, pass.TextureFactor);
	hash = diligent_hash_combine(hash, pass.AlphaRef);
	hash = diligent_hash_combine(hash, pass.DynamicLight ? 1u : 0u);
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.ZBufferTestMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.AlphaTestMode));
	hash = diligent_hash_combine(hash, static_cast<uint64>(pass.FillMode));
	hash = diligent_hash_combine(hash, pass.bUseBumpEnvMap ? 1u : 0u);
	hash = diligent_hash_combine(hash, pass.BumpEnvMapStage);
	hash = diligent_hash_combine(hash, diligent_float_bits(pass.fBumpEnvMap_Scale));
	hash = diligent_hash_combine(hash, diligent_float_bits(pass.fBumpEnvMap_Offset));
	return hash;
}

const char* diligent_get_optimized_2d_vertex_shader_source()
{
	return
		"struct VSInput\n"
		"{\n"
		"    float2 position : ATTRIB0;\n"
		"    float4 color : ATTRIB1;\n"
		"    float2 uv : ATTRIB2;\n"
		"};\n"
		"struct VSOutput\n"
		"{\n"
		"    float4 position : SV_POSITION;\n"
		"    float4 color : COLOR0;\n"
		"    float2 uv : TEXCOORD0;\n"
		"    float3 world_pos : TEXCOORD1;\n"
		"};\n"
		"VSOutput VSMain(VSInput input)\n"
		"{\n"
		"    VSOutput output;\n"
		"    output.position = float4(input.position, 0.0f, 1.0f);\n"
		"    output.color = input.color;\n"
		"    output.uv = input.uv;\n"
		"    return output;\n"
		"}\n";
}

const char* diligent_get_glow_blur_vertex_shader_source()
{
	return
		"struct VSInput\n"
		"{\n"
		"    float2 position : ATTRIB0;\n"
		"    float4 color : ATTRIB1;\n"
		"    float2 uv : ATTRIB2;\n"
		"};\n"
		"struct VSOutput\n"
		"{\n"
		"    float4 position : SV_POSITION;\n"
		"    float2 uv : TEXCOORD0;\n"
		"};\n"
		"VSOutput VSMain(VSInput input)\n"
		"{\n"
		"    VSOutput output;\n"
		"    output.position = float4(input.position, 0.0f, 1.0f);\n"
		"    output.uv = input.uv;\n"
		"    return output;\n"
		"}\n";
}

const char* diligent_get_optimized_2d_pixel_shader_source()
{
	return
		"Texture2D g_Texture;\n"
		"SamplerState g_Texture_sampler;\n"
		"struct PSInput\n"
		"{\n"
		"    float4 position : SV_POSITION;\n"
		"    float4 color : COLOR0;\n"
		"    float2 uv : TEXCOORD0;\n"
		"    float3 world_pos : TEXCOORD1;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 texel = g_Texture.Sample(g_Texture_sampler, input.uv);\n"
		"    return texel * input.color;\n"
		"}\n";
}

const char* diligent_get_glow_blur_pixel_shader_source()
{
	return
		"#define GLOW_MAX_TAPS 8\n"
		"Texture2D g_Texture;\n"
		"SamplerState g_Texture_sampler;\n"
		"cbuffer GlowBlurConstants\n"
		"{\n"
		"    float4 g_Taps[GLOW_MAX_TAPS];\n"
		"    float4 g_Params;\n"
		"};\n"
		"struct PSInput\n"
		"{\n"
		"    float4 position : SV_POSITION;\n"
		"    float2 uv : TEXCOORD0;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    int tapCount = (int)g_Params.x;\n"
		"    float3 color = float3(0.0f, 0.0f, 0.0f);\n"
		"    float alpha = 0.0f;\n"
		"    for (int i = 0; i < tapCount; ++i)\n"
		"    {\n"
		"        float2 offset = g_Taps[i].xy;\n"
		"        float weight = g_Taps[i].z;\n"
		"        float4 texel = g_Texture.Sample(g_Texture_sampler, input.uv + offset);\n"
		"        color += texel.rgb * weight;\n"
		"        alpha += texel.a * weight;\n"
		"    }\n"
		"    return float4(color, alpha);\n"
		"}\n";
}

Diligent::BLEND_FACTOR diligent_map_blend_factor(LTSurfaceBlend blend, bool is_source)
{
	switch (blend)
	{
		case LTSURFACEBLEND_ALPHA:
			return is_source ? Diligent::BLEND_FACTOR_SRC_ALPHA : Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
		case LTSURFACEBLEND_SOLID:
			return is_source ? Diligent::BLEND_FACTOR_ONE : Diligent::BLEND_FACTOR_ZERO;
		case LTSURFACEBLEND_ADD:
			return Diligent::BLEND_FACTOR_ONE;
		case LTSURFACEBLEND_MULTIPLY:
			return is_source ? Diligent::BLEND_FACTOR_DEST_COLOR : Diligent::BLEND_FACTOR_ZERO;
		case LTSURFACEBLEND_MULTIPLY2:
			return is_source ? Diligent::BLEND_FACTOR_DEST_COLOR : Diligent::BLEND_FACTOR_SRC_COLOR;
		case LTSURFACEBLEND_MASK:
			return is_source ? Diligent::BLEND_FACTOR_ZERO : Diligent::BLEND_FACTOR_INV_SRC_COLOR;
		case LTSURFACEBLEND_MASKADD:
			return is_source ? Diligent::BLEND_FACTOR_ONE : Diligent::BLEND_FACTOR_INV_SRC_COLOR;
		default:
			return is_source ? Diligent::BLEND_FACTOR_SRC_ALPHA : Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
	}
}

void diligent_fill_optimized_2d_blend_desc(LTSurfaceBlend blend, Diligent::RenderTargetBlendDesc& blend_desc)
{
	blend_desc.BlendEnable = true;
	blend_desc.RenderTargetWriteMask = Diligent::COLOR_MASK_ALL;
	blend_desc.SrcBlend = diligent_map_blend_factor(blend, true);
	blend_desc.DestBlend = diligent_map_blend_factor(blend, false);
	blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
	blend_desc.SrcBlendAlpha = blend_desc.SrcBlend;
	blend_desc.DestBlendAlpha = blend_desc.DestBlend;
	blend_desc.BlendOpAlpha = blend_desc.BlendOp;
}

void diligent_fill_world_blend_desc(DiligentWorldBlendMode blend, Diligent::RenderTargetBlendDesc& blend_desc)
{
	blend_desc.RenderTargetWriteMask = Diligent::COLOR_MASK_ALL;
	if (blend == kWorldBlendSolid)
	{
		blend_desc.BlendEnable = false;
		return;
	}

	blend_desc.BlendEnable = true;
	blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
	blend_desc.BlendOpAlpha = blend_desc.BlendOp;

	if (blend == kWorldBlendAdditive)
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
	}
	else if (blend == kWorldBlendAdditiveOne)
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
	}
	else if (blend == kWorldBlendMultiply)
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ZERO;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_SRC_COLOR;
	}
	else
	{
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
	}

	blend_desc.SrcBlendAlpha = blend_desc.SrcBlend;
	blend_desc.DestBlendAlpha = blend_desc.DestBlend;
}

bool diligent_ensure_optimized_2d_shaders()
{
	if (g_optimized_2d_resources.vertex_shader && g_optimized_2d_resources.pixel_shader)
	{
		return true;
	}

	if (!g_render_device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	Diligent::ShaderDesc vertex_desc;
	vertex_desc.Name = "ltjs_optimized_2d_vs";
	vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
	shader_info.Desc = vertex_desc;
	shader_info.EntryPoint = "VSMain";
	shader_info.Source = diligent_get_optimized_2d_vertex_shader_source();
	g_render_device->CreateShader(shader_info, &g_optimized_2d_resources.vertex_shader);
	if (!g_optimized_2d_resources.vertex_shader)
	{
		return false;
	}

	Diligent::ShaderDesc pixel_desc;
	pixel_desc.Name = "ltjs_optimized_2d_ps";
	pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	shader_info.Desc = pixel_desc;
	shader_info.EntryPoint = "PSMain";
	shader_info.Source = diligent_get_optimized_2d_pixel_shader_source();
	g_render_device->CreateShader(shader_info, &g_optimized_2d_resources.pixel_shader);
	return g_optimized_2d_resources.pixel_shader != nullptr;
}

bool diligent_ensure_optimized_2d_vertex_buffer(uint32 size)
{
	if (g_optimized_2d_resources.vertex_buffer && g_optimized_2d_resources.vertex_buffer_size >= size)
	{
		return true;
	}

	if (!g_render_device || size == 0)
	{
		return false;
	}

	g_optimized_2d_resources.vertex_buffer.Release();
	g_optimized_2d_resources.vertex_buffer_size = 0;

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_optimized_2d_vertices";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = sizeof(DiligentOptimized2DVertex);

	g_render_device->CreateBuffer(desc, nullptr, &g_optimized_2d_resources.vertex_buffer);
	if (!g_optimized_2d_resources.vertex_buffer)
	{
		return false;
	}

	g_optimized_2d_resources.vertex_buffer_size = size;
	return true;
}

DiligentOptimized2DPipeline* diligent_get_optimized_2d_pipeline(LTSurfaceBlend blend, bool clamp_sampler)
{
	auto& pipelines = clamp_sampler ? g_optimized_2d_resources.clamped_pipelines : g_optimized_2d_resources.pipelines;
	auto it = pipelines.find(static_cast<int32>(blend));
	if (it != pipelines.end())
	{
		return &it->second;
	}

	if (!diligent_ensure_optimized_2d_shaders())
	{
		return nullptr;
	}

	if (!g_render_device || !g_swap_chain)
	{
		return nullptr;
	}

	const auto swap_desc = g_swap_chain->GetDesc();

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_optimized_2d_pso";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = swap_desc.ColorBufferFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = swap_desc.DepthBufferFormat;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));

	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	pipeline_info.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;

	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;

	diligent_fill_optimized_2d_blend_desc(blend, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);

	pipeline_info.pVS = g_optimized_2d_resources.vertex_shader;
	pipeline_info.pPS = g_optimized_2d_resources.pixel_shader;

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::ShaderResourceVariableDesc variables[] = {
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};
	pipeline_info.PSODesc.ResourceLayout.Variables = variables;
	pipeline_info.PSODesc.ResourceLayout.NumVariables = static_cast<uint32>(std::size(variables));

	Diligent::ImmutableSamplerDesc sampler_desc;
	sampler_desc.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.AddressU = clamp_sampler ? Diligent::TEXTURE_ADDRESS_CLAMP : Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc.Desc.AddressV = clamp_sampler ? Diligent::TEXTURE_ADDRESS_CLAMP : Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc.Desc.AddressW = clamp_sampler ? Diligent::TEXTURE_ADDRESS_CLAMP : Diligent::TEXTURE_ADDRESS_WRAP;
	sampler_desc.TextureName = "g_Texture";
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler_desc;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

	DiligentOptimized2DPipeline pipeline;
	g_render_device->CreatePipelineState(pipeline_info, &pipeline.pipeline_state);
	if (!pipeline.pipeline_state)
	{
		return nullptr;
	}

	pipeline.pipeline_state->CreateShaderResourceBinding(&pipeline.srb, true);
	auto result = pipelines.emplace(static_cast<int32>(blend), std::move(pipeline));
	return result.second ? &result.first->second : nullptr;
}

bool diligent_ensure_glow_blur_resources()
{
	if (g_diligent_glow_blur_resources.vertex_shader && g_diligent_glow_blur_resources.pixel_shader &&
		g_diligent_glow_blur_resources.constant_buffer)
	{
		return true;
	}

	if (!g_render_device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	if (!g_diligent_glow_blur_resources.vertex_shader)
	{
		Diligent::ShaderDesc vertex_desc;
		vertex_desc.Name = "ltjs_glow_blur_vs";
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		shader_info.Desc = vertex_desc;
		shader_info.EntryPoint = "VSMain";
		shader_info.Source = diligent_get_glow_blur_vertex_shader_source();
		g_render_device->CreateShader(shader_info, &g_diligent_glow_blur_resources.vertex_shader);
	}

	if (!g_diligent_glow_blur_resources.pixel_shader)
	{
		Diligent::ShaderDesc pixel_desc;
		pixel_desc.Name = "ltjs_glow_blur_ps";
		pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		shader_info.Desc = pixel_desc;
		shader_info.EntryPoint = "PSMain";
		shader_info.Source = diligent_get_glow_blur_pixel_shader_source();
		g_render_device->CreateShader(shader_info, &g_diligent_glow_blur_resources.pixel_shader);
	}

	if (!g_diligent_glow_blur_resources.constant_buffer)
	{
		Diligent::BufferDesc desc;
		desc.Name = "ltjs_glow_blur_constants";
		desc.Size = sizeof(DiligentGlowBlurConstants);
		desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		desc.Usage = Diligent::USAGE_DYNAMIC;
		desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		g_render_device->CreateBuffer(desc, nullptr, &g_diligent_glow_blur_resources.constant_buffer);
	}

	return g_diligent_glow_blur_resources.vertex_shader && g_diligent_glow_blur_resources.pixel_shader &&
		g_diligent_glow_blur_resources.constant_buffer;
}

DiligentGlowBlurPipeline* diligent_get_glow_blur_pipeline(LTSurfaceBlend blend)
{
	if (!diligent_ensure_glow_blur_resources())
	{
		return nullptr;
	}

	if (!g_render_device || !g_swap_chain)
	{
		return nullptr;
	}

	DiligentGlowBlurPipeline* pipeline = (blend == LTSURFACEBLEND_SOLID)
		? &g_diligent_glow_blur_resources.pipeline_solid
		: &g_diligent_glow_blur_resources.pipeline_add;

	if (pipeline->pipeline_state && pipeline->srb)
	{
		return pipeline;
	}

	Diligent::TEXTURE_FORMAT color_format = g_swap_chain->GetDesc().ColorBufferFormat;
	Diligent::TEXTURE_FORMAT depth_format = g_swap_chain->GetDesc().DepthBufferFormat;
	if (g_diligent_glow_state.glow_target)
	{
		auto* rtv = g_diligent_glow_state.glow_target->GetRenderTargetView();
		if (rtv && rtv->GetTexture())
		{
			color_format = rtv->GetTexture()->GetDesc().Format;
		}
		auto* dsv = g_diligent_glow_state.glow_target->GetDepthStencilView();
		if (dsv && dsv->GetTexture())
		{
			depth_format = dsv->GetTexture()->GetDesc().Format;
		}
	}

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	pipeline_info.PSODesc.Name = "ltjs_glow_blur";
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = color_format;
	pipeline_info.GraphicsPipeline.DSVFormat = depth_format;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	Diligent::LayoutElement layout_elements[] =
	{
		Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, false},
		Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false}
	};

	pipeline_info.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
	pipeline_info.GraphicsPipeline.InputLayout.NumElements = static_cast<uint32>(std::size(layout_elements));
	pipeline_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
	pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;

	diligent_fill_optimized_2d_blend_desc(blend, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);

	pipeline_info.pVS = g_diligent_glow_blur_resources.vertex_shader;
	pipeline_info.pPS = g_diligent_glow_blur_resources.pixel_shader;

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::ShaderResourceVariableDesc variables[] = {
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};
	pipeline_info.PSODesc.ResourceLayout.Variables = variables;
	pipeline_info.PSODesc.ResourceLayout.NumVariables = static_cast<uint32>(std::size(variables));

	Diligent::ImmutableSamplerDesc sampler_desc;
	sampler_desc.ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc.Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc.Desc.AddressU = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler_desc.Desc.AddressV = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler_desc.Desc.AddressW = Diligent::TEXTURE_ADDRESS_CLAMP;
	sampler_desc.TextureName = "g_Texture";
	pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler_desc;
	pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1;

	g_render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline->pipeline_state);
	if (!pipeline->pipeline_state)
	{
		return nullptr;
	}

	auto* ps_constants = pipeline->pipeline_state->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "GlowBlurConstants");
	if (ps_constants)
	{
		ps_constants->Set(g_diligent_glow_blur_resources.constant_buffer);
	}

	pipeline->pipeline_state->CreateShaderResourceBinding(&pipeline->srb, true);
	return pipeline->pipeline_state && pipeline->srb ? pipeline : nullptr;
}

bool diligent_update_glow_blur_constants(const DiligentGlowBlurConstants& constants)
{
	if (!g_immediate_context || !g_diligent_glow_blur_resources.constant_buffer)
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_immediate_context->MapBuffer(g_diligent_glow_blur_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_immediate_context->UnmapBuffer(g_diligent_glow_blur_resources.constant_buffer, Diligent::MAP_WRITE);
	return true;
}

bool diligent_draw_glow_blur_quad(
	DiligentGlowBlurPipeline* pipeline,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target)
{
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb || !texture_view || !render_target)
	{
		return false;
	}

	constexpr uint32 kVertexCount = 4;
	const uint32 data_size = kVertexCount * sizeof(DiligentOptimized2DVertex);
	if (!diligent_ensure_optimized_2d_vertex_buffer(data_size))
	{
		return false;
	}

	DiligentOptimized2DVertex vertices[kVertexCount];
	vertices[0] = {{-1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}};
	vertices[1] = {{1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}};
	vertices[2] = {{-1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}};
	vertices[3] = {{1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}};

	void* mapped_vertices = nullptr;
	g_immediate_context->MapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	if (!mapped_vertices)
	{
		return false;
	}
	std::memcpy(mapped_vertices, vertices, sizeof(vertices));
	g_immediate_context->UnmapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE);

	auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture");
	if (texture_var)
	{
		texture_var->Set(texture_view);
	}

	g_immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_immediate_context->SetPipelineState(pipeline->pipeline_state);

	Diligent::IBuffer* buffers[] = {g_optimized_2d_resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	g_immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = kVertexCount;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_immediate_context->Draw(draw_attribs);
	return true;
}

bool diligent_build_surface_upload_data(
	const DiligentSurface& surface,
	uint32 transparent_color,
	std::vector<uint32>& out_data,
	uint32& out_stride)
{
	if (surface.width == 0 || surface.height == 0)
	{
		return false;
	}

	out_stride = surface.width * sizeof(uint32);
	out_data.resize(static_cast<size_t>(surface.width) * surface.height);

	const uint32 transparent_mask = transparent_color & 0x00FFFFFFu;
	const bool use_transparency = transparent_color != OPTIMIZE_NO_TRANSPARENCY;
	const size_t pixel_count = out_data.size();

	for (size_t i = 0; i < pixel_count; ++i)
	{
		uint32 pixel = surface.data[i] & 0x00FFFFFFu;
		uint32 alpha = 0xFF000000u;
		if (use_transparency && pixel == transparent_mask)
		{
			alpha = 0x00000000u;
		}
		out_data[i] = pixel | alpha;
	}

	return true;
}

bool diligent_upload_surface_texture(DiligentSurface& surface, DiligentSurfaceGpuData& gpu_data)
{
	if (!g_render_device)
	{
		return false;
	}

	std::vector<uint32> converted;
	uint32 stride = 0;
	if (!diligent_build_surface_upload_data(surface, surface.optimized_transparent_color, converted, stride))
	{
		return false;
	}

	const bool can_update = gpu_data.texture && gpu_data.width == surface.width && gpu_data.height == surface.height;
	if (can_update)
	{
		if (!g_immediate_context)
		{
			return false;
		}

		Diligent::Box dst_box{0, surface.width, 0, surface.height};
		Diligent::TextureSubResData subresource;
		subresource.pData = converted.data();
		subresource.Stride = static_cast<Diligent::Uint64>(stride);
		subresource.DepthStride = 0;

		g_immediate_context->UpdateTexture(
			gpu_data.texture,
			0,
			0,
			dst_box,
			subresource,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		if (!gpu_data.srv)
		{
			gpu_data.srv = gpu_data.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		gpu_data.transparent_color = surface.optimized_transparent_color;
		surface.flags &= ~kDiligentSurfaceFlagDirty;
		return gpu_data.srv != nullptr;
	}

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_optimized_surface";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = surface.width;
	desc.Height = surface.height;
	desc.MipLevels = 1;
	desc.Format = Diligent::TEX_FORMAT_BGRA8_UNORM;
	desc.Usage = Diligent::USAGE_DEFAULT;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	Diligent::TextureSubResData subresource;
	subresource.pData = converted.data();
	subresource.Stride = static_cast<Diligent::Uint64>(stride);
	subresource.DepthStride = 0;

	Diligent::TextureData init_data{&subresource, 1};
	gpu_data.texture.Release();
	gpu_data.srv.Release();
	g_render_device->CreateTexture(desc, &init_data, &gpu_data.texture);
	if (!gpu_data.texture)
	{
		return false;
	}

	gpu_data.srv = gpu_data.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	gpu_data.width = surface.width;
	gpu_data.height = surface.height;
	gpu_data.transparent_color = surface.optimized_transparent_color;
	surface.flags &= ~kDiligentSurfaceFlagDirty;
	return gpu_data.srv != nullptr;
}

bool diligent_draw_optimized_2d_vertices(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view)
{
	if (!vertices || vertex_count == 0 || !texture_view)
	{
		return false;
	}

	if (!g_immediate_context || !g_swap_chain)
	{
		return false;
	}

	const uint32 data_size = vertex_count * sizeof(DiligentOptimized2DVertex);
	if (!diligent_ensure_optimized_2d_vertex_buffer(data_size))
	{
		return false;
	}

	auto* pipeline = diligent_get_optimized_2d_pipeline(blend, false);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	void* mapped_vertices = nullptr;
	g_immediate_context->MapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	std::memcpy(mapped_vertices, vertices, data_size);
	g_immediate_context->UnmapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE);

	auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture");
	if (texture_var)
	{
		texture_var->Set(texture_view);
	}

	auto* render_target = g_swap_chain->GetCurrentBackBufferRTV();
	auto* depth_target = g_swap_chain->GetDepthBufferDSV();
	g_immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_immediate_context->SetPipelineState(pipeline->pipeline_state);

	Diligent::IBuffer* buffers[] = {g_optimized_2d_resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	g_immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = vertex_count;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_immediate_context->Draw(draw_attribs);
	return true;
}

bool diligent_draw_optimized_2d_vertices_to_target(
	const DiligentOptimized2DVertex* vertices,
	uint32 vertex_count,
	LTSurfaceBlend blend,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target,
	bool clamp_sampler)
{
	if (!vertices || vertex_count == 0 || !texture_view || !render_target)
	{
		return false;
	}

	if (!g_immediate_context || !g_swap_chain)
	{
		return false;
	}

	const uint32 data_size = vertex_count * sizeof(DiligentOptimized2DVertex);
	if (!diligent_ensure_optimized_2d_vertex_buffer(data_size))
	{
		return false;
	}

	auto* pipeline = diligent_get_optimized_2d_pipeline(blend, clamp_sampler);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	void* mapped_vertices = nullptr;
	g_immediate_context->MapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	std::memcpy(mapped_vertices, vertices, data_size);
	g_immediate_context->UnmapBuffer(g_optimized_2d_resources.vertex_buffer, Diligent::MAP_WRITE);

	auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture");
	if (texture_var)
	{
		texture_var->Set(texture_view);
	}

	g_immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_immediate_context->SetPipelineState(pipeline->pipeline_state);

	Diligent::IBuffer* buffers[] = {g_optimized_2d_resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	g_immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawAttribs draw_attribs;
	draw_attribs.NumVertices = vertex_count;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_immediate_context->Draw(draw_attribs);
	return true;
}

void diligent_to_clip_space(float x, float y, float& out_x, float& out_y)
{
	out_x = (x / static_cast<float>(g_screen_width)) * 2.0f - 1.0f;
	out_y = 1.0f - (y / static_cast<float>(g_screen_height)) * 2.0f;
}

bool diligent_to_view_clip_space(float x, float y, float& out_x, float& out_y)
{
	const float view_width = g_ViewParams.m_fScreenWidth;
	const float view_height = g_ViewParams.m_fScreenHeight;
	if (view_width <= 0.0f || view_height <= 0.0f)
	{
		out_x = 0.0f;
		out_y = 0.0f;
		return false;
	}

	out_x = (x / view_width) * 2.0f - 1.0f;
	out_y = 1.0f - (y / view_height) * 2.0f;
	return true;
}

Diligent::ITextureView* diligent_get_debug_white_texture_view()
{
	if (g_debug_white_texture_srv)
	{
		return g_debug_white_texture_srv;
	}

	if (!g_render_device)
	{
		return nullptr;
	}

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_debug_white";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.Format = Diligent::TEX_FORMAT_RGBA8_UNORM;
	desc.Usage = Diligent::USAGE_IMMUTABLE;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	const uint32 white_pixel = 0xFFFFFFFFu;
	Diligent::TextureSubResData subresource;
	subresource.pData = &white_pixel;
	subresource.Stride = sizeof(white_pixel);
	subresource.DepthStride = 0;

	Diligent::TextureData init_data{&subresource, 1};
	g_render_device->CreateTexture(desc, &init_data, &g_debug_white_texture);
	if (!g_debug_white_texture)
	{
		return nullptr;
	}

	g_debug_white_texture_srv = g_debug_white_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	return g_debug_white_texture_srv;
}

void diligent_store_matrix_from_lt(const LTMatrix& matrix, std::array<float, 16>& out)
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

uint32 diligent_glow_truncate_to_power_of_two(uint32 value)
{
	uint32 nearest = 0x80000000u;
	while (nearest > value)
	{
		nearest >>= 1;
	}

	return nearest;
}

void diligent_set_viewport(float width, float height)
{
	if (!g_immediate_context)
	{
		return;
	}

	Diligent::Viewport viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	g_immediate_context->SetViewports(1, &viewport, 0, 0);
}

void diligent_set_viewport_rect(float left, float top, float width, float height)
{
	if (!g_immediate_context)
	{
		return;
	}

	Diligent::Viewport viewport;
	viewport.TopLeftX = left;
	viewport.TopLeftY = top;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	g_immediate_context->SetViewports(1, &viewport, 0, 0);
}

const char* diligent_get_world_vertex_shader_source()
{
	return
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"struct VSInput\n"
		"{\n"
		"    float3 Pos : ATTRIB0;\n"
		"    float4 Color : ATTRIB1;\n"
		"    float2 UV0 : ATTRIB2;\n"
		"    float2 UV1 : ATTRIB3;\n"
		"};\n"
		"struct VSOutput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"VSOutput VSMain(VSInput input)\n"
		"{\n"
		"    VSOutput output;\n"
		"    output.Pos = mul(float4(input.Pos, 1.0f), g_Mvp);\n"
		"    output.Color = input.Color;\n"
		"    output.UV0 = input.UV0;\n"
		"    output.UV1 = input.UV1;\n"
		"    output.WorldPos = mul(float4(input.Pos, 1.0f), g_World).xyz;\n"
		"    return output;\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_textured_source()
{
	return
		"Texture2D g_Texture0;\n"
		"SamplerState g_Texture0_sampler;\n"
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 baseColor = g_Texture0.Sample(g_Texture0_sampler, input.UV0) * input.Color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.WorldPos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_glow_source()
{
	return
		"Texture2D g_Texture0;\n"
		"SamplerState g_Texture0_sampler;\n"
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 texel = g_Texture0.Sample(g_Texture0_sampler, input.UV0);\n"
		"    float3 glowColor = texel.rgb * texel.a;\n"
		"    float alpha = texel.a * input.Color.a;\n"
		"    float3 fogged = ApplyFog(glowColor, input.WorldPos);\n"
		"    return float4(fogged, alpha);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_lightmap_source()
{
	return
		"Texture2D g_Texture0;\n"
		"Texture2D g_Texture1;\n"
		"SamplerState g_Texture0_sampler;\n"
		"SamplerState g_Texture1_sampler;\n"
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 baseTexel = g_Texture0.Sample(g_Texture0_sampler, input.UV0);\n"
		"    float4 lmTexel = g_Texture1.Sample(g_Texture1_sampler, input.UV1);\n"
		"    float4 baseColor = baseTexel * lmTexel * input.Color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.WorldPos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_lightmap_only_source()
{
	return
		"Texture2D g_Texture1;\n"
		"SamplerState g_Texture1_sampler;\n"
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 lmTexel = g_Texture1.Sample(g_Texture1_sampler, input.UV1);\n"
		"    float4 baseColor = lmTexel * input.Color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.WorldPos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_dual_source()
{
	return
		"Texture2D g_Texture0;\n"
		"Texture2D g_Texture1;\n"
		"SamplerState g_Texture0_sampler;\n"
		"SamplerState g_Texture1_sampler;\n"
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 baseTexel = g_Texture0.Sample(g_Texture0_sampler, input.UV0);\n"
		"    float4 dualTexel = g_Texture1.Sample(g_Texture1_sampler, input.UV1);\n"
		"    float4 baseColor = baseTexel * dualTexel * input.Color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.WorldPos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_bump_source()
{
	return
		"Texture2D g_Texture0;\n"
		"Texture2D g_Texture1;\n"
		"SamplerState g_Texture0_sampler;\n"
		"SamplerState g_Texture1_sampler;\n"
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float3 normal = g_Texture0.Sample(g_Texture0_sampler, input.UV0).xyz * 2.0f - 1.0f;\n"
		"    float2 env_uv = input.UV1 + normal.xy * g_DynamicLightPos.x;\n"
		"    float4 env = g_Texture1.Sample(g_Texture1_sampler, env_uv);\n"
		"    float intensity = saturate(normal.z);\n"
		"    float3 color = env.rgb * intensity * input.Color.rgb;\n"
		"    float alpha = env.a * input.Color.a;\n"
		"    float3 fogged = ApplyFog(color, input.WorldPos);\n"
		"    return float4(fogged, alpha);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_lightmap_dual_source()
{
	return
		"Texture2D g_Texture0;\n"
		"Texture2D g_Texture1;\n"
		"Texture2D g_Texture2;\n"
		"SamplerState g_Texture0_sampler;\n"
		"SamplerState g_Texture1_sampler;\n"
		"SamplerState g_Texture2_sampler;\n"
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 baseTexel = g_Texture0.Sample(g_Texture0_sampler, input.UV0);\n"
		"    float4 lmTexel = g_Texture1.Sample(g_Texture1_sampler, input.UV1);\n"
		"    float4 dualTexel = g_Texture2.Sample(g_Texture2_sampler, input.UV1);\n"
		"    float4 baseColor = baseTexel * lmTexel * dualTexel * input.Color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.WorldPos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_solid_source()
{
	return
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 baseColor = input.Color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.WorldPos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

const char* diligent_get_world_pixel_shader_dynamic_light_source()
{
	return
		"cbuffer WorldConstants\n"
		"{\n"
		"    float4x4 g_Mvp;\n"
		"    float4x4 g_View;\n"
		"    float4x4 g_World;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"    float4 g_DynamicLightPos;\n"
		"    float4 g_DynamicLightColor;\n"
		"};\n"
		"float GetFogFactor(float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return 0.0f;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    return saturate((dist - fogNear) / fogRange);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 Pos : SV_POSITION;\n"
		"    float4 Color : COLOR0;\n"
		"    float2 UV0 : TEXCOORD0;\n"
		"    float2 UV1 : TEXCOORD1;\n"
		"    float3 WorldPos : TEXCOORD2;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float3 light_vec = input.WorldPos - g_DynamicLightPos.xyz;\n"
		"    float dist = length(light_vec);\n"
		"    float radius = max(g_DynamicLightPos.w, 0.0001f);\n"
		"    float atten = saturate(1.0f - (dist / radius));\n"
		"    float fogFactor = GetFogFactor(input.WorldPos);\n"
		"    float3 color = g_DynamicLightColor.rgb * atten * input.Color.rgb;\n"
		"    color *= (1.0f - fogFactor);\n"
		"    return float4(color, 1.0f);\n"
		"}\n";
}

const char* diligent_get_model_pixel_shader_textured_source()
{
	return
		"Texture2D g_Texture0;\n"
		"SamplerState g_Texture0_sampler;\n"
		"cbuffer ModelConstants\n"
		"{\n"
		"    float4x4 g_MVP;\n"
		"    float4x4 g_Model;\n"
		"    float4 g_Color;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 position : SV_POSITION;\n"
		"    float4 color : COLOR0;\n"
		"    float2 uv : TEXCOORD0;\n"
		"    float3 world_pos : TEXCOORD1;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 baseColor = g_Texture0.Sample(g_Texture0_sampler, input.uv) * input.color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.world_pos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

const char* diligent_get_model_pixel_shader_solid_source()
{
	return
		"cbuffer ModelConstants\n"
		"{\n"
		"    float4x4 g_MVP;\n"
		"    float4x4 g_Model;\n"
		"    float4 g_Color;\n"
		"    float4 g_CameraPos;\n"
		"    float4 g_FogColor;\n"
		"    float4 g_FogParams;\n"
		"};\n"
		"float3 ApplyFog(float3 color, float3 worldPos)\n"
		"{\n"
		"    if (g_CameraPos.w <= 0.5f)\n"
		"    {\n"
		"        return color;\n"
		"    }\n"
		"    float fogNear = g_FogColor.w;\n"
		"    float fogFar = g_FogParams.x;\n"
		"    float fogRange = max(fogFar - fogNear, 0.0001f);\n"
		"    float dist = length(worldPos - g_CameraPos.xyz);\n"
		"    float fogFactor = saturate((dist - fogNear) / fogRange);\n"
		"    return lerp(color, g_FogColor.rgb, fogFactor);\n"
		"}\n"
		"struct PSInput\n"
		"{\n"
		"    float4 position : SV_POSITION;\n"
		"    float4 color : COLOR0;\n"
		"    float2 uv : TEXCOORD0;\n"
		"    float3 world_pos : TEXCOORD1;\n"
		"};\n"
		"float4 PSMain(PSInput input) : SV_TARGET\n"
		"{\n"
		"    float4 baseColor = input.color;\n"
		"    float3 fogged = ApplyFog(baseColor.rgb, input.world_pos);\n"
		"    return float4(fogged, baseColor.a);\n"
		"}\n";
}

bool diligent_ensure_world_constant_buffer()
{
	if (g_world_resources.constant_buffer)
	{
		return true;
	}

	if (!g_render_device)
	{
		return false;
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_world_constants";
	desc.Size = sizeof(DiligentWorldConstants);
	desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

	g_render_device->CreateBuffer(desc, nullptr, &g_world_resources.constant_buffer);
	return g_world_resources.constant_buffer != nullptr;
}

bool diligent_ensure_model_constant_buffer()
{
	if (g_model_resources.constant_buffer)
	{
		return true;
	}

	if (!g_render_device)
	{
		return false;
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_model_constants";
	desc.Size = sizeof(DiligentModelConstants);
	desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

	g_render_device->CreateBuffer(desc, nullptr, &g_model_resources.constant_buffer);
	return g_model_resources.constant_buffer != nullptr;
}

bool diligent_ensure_model_pixel_shaders()
{
	if (g_model_resources.pixel_shader_textured && g_model_resources.pixel_shader_solid)
	{
		return true;
	}

	if (!g_render_device)
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
		g_render_device->CreateShader(shader_info, &g_model_resources.pixel_shader_textured);
	}

	if (!g_model_resources.pixel_shader_solid)
	{
		pixel_desc.Name = "ltjs_model_ps_solid";
		shader_info.Desc = pixel_desc;
		shader_info.Source = diligent_get_model_pixel_shader_solid_source();
		g_render_device->CreateShader(shader_info, &g_model_resources.pixel_shader_solid);
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

	if (!g_render_device)
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
	g_render_device->CreateShader(shader_info, &shader);
	if (!shader)
	{
		return nullptr;
	}

	g_model_resources.vertex_shaders.emplace(layout.hash, shader);
	return shader.RawPtr();
}

bool diligent_ensure_world_shaders()
{
	if (g_world_resources.vertex_shader && g_world_resources.pixel_shader_textured && g_world_resources.pixel_shader_glow && g_world_resources.pixel_shader_lightmap && g_world_resources.pixel_shader_lightmap_only && g_world_resources.pixel_shader_dual && g_world_resources.pixel_shader_lightmap_dual && g_world_resources.pixel_shader_solid && g_world_resources.pixel_shader_dynamic_light && g_world_resources.pixel_shader_bump)
	{
		return true;
	}

	if (!g_render_device)
	{
		return false;
	}

	Diligent::ShaderCreateInfo shader_info;
	shader_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

	if (!g_world_resources.vertex_shader)
	{
		Diligent::ShaderDesc vertex_desc;
		vertex_desc.Name = "ltjs_world_vs";
		vertex_desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		shader_info.Desc = vertex_desc;
		shader_info.EntryPoint = "VSMain";
		shader_info.Source = diligent_get_world_vertex_shader_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.vertex_shader);
		if (!g_world_resources.vertex_shader)
		{
			return false;
		}
	}

	Diligent::ShaderDesc pixel_desc;
	pixel_desc.Name = "ltjs_world_ps_textured";
	pixel_desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	shader_info.Desc = pixel_desc;
	shader_info.EntryPoint = "PSMain";

	if (!g_world_resources.pixel_shader_textured)
	{
		shader_info.Source = diligent_get_world_pixel_shader_textured_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_textured);
	}

	if (!g_world_resources.pixel_shader_glow)
	{
		shader_info.Source = diligent_get_world_pixel_shader_glow_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_glow);
	}

	if (!g_world_resources.pixel_shader_lightmap)
	{
		shader_info.Source = diligent_get_world_pixel_shader_lightmap_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_lightmap);
	}

	if (!g_world_resources.pixel_shader_lightmap_only)
	{
		shader_info.Source = diligent_get_world_pixel_shader_lightmap_only_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_lightmap_only);
	}

	if (!g_world_resources.pixel_shader_dual)
	{
		shader_info.Source = diligent_get_world_pixel_shader_dual_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_dual);
	}

	if (!g_world_resources.pixel_shader_bump)
	{
		shader_info.Source = diligent_get_world_pixel_shader_bump_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_bump);
	}

	if (!g_world_resources.pixel_shader_lightmap_dual)
	{
		shader_info.Source = diligent_get_world_pixel_shader_lightmap_dual_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_lightmap_dual);
	}

	if (!g_world_resources.pixel_shader_solid)
	{
		shader_info.Source = diligent_get_world_pixel_shader_solid_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_solid);
	}

	if (!g_world_resources.pixel_shader_dynamic_light)
	{
		shader_info.Source = diligent_get_world_pixel_shader_dynamic_light_source();
		g_render_device->CreateShader(shader_info, &g_world_resources.pixel_shader_dynamic_light);
	}

	return g_world_resources.pixel_shader_textured && g_world_resources.pixel_shader_glow && g_world_resources.pixel_shader_lightmap && g_world_resources.pixel_shader_lightmap_only && g_world_resources.pixel_shader_dual && g_world_resources.pixel_shader_lightmap_dual && g_world_resources.pixel_shader_solid && g_world_resources.pixel_shader_dynamic_light && g_world_resources.pixel_shader_bump;
}

DiligentWorldPipeline* diligent_get_world_pipeline(
	uint8 mode,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode)
{
	DiligentWorldPipelineKey key{mode, blend_mode, depth_mode};
	auto it = g_world_pipelines.pipelines.find(key);
	if (it != g_world_pipelines.pipelines.end())
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

	if (!g_render_device || !g_swap_chain)
	{
		return nullptr;
	}

	const auto swap_desc = g_swap_chain->GetDesc();

	Diligent::GraphicsPipelineStateCreateInfo pipeline_info;
	if (mode == kWorldPipelineLightmap)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_lightmap";
	}
	else if (mode == kWorldPipelineTextured)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_textured";
	}
	else if (mode == kWorldPipelineGlowTextured)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_glow";
	}
	else if (mode == kWorldPipelineLightmapOnly)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_lightmap_only";
	}
	else if (mode == kWorldPipelineDualTexture)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_dual";
	}
	else if (mode == kWorldPipelineLightmapDual)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_lightmap_dual";
	}
	else if (mode == kWorldPipelineDynamicLight)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_dynamic_light";
	}
	else if (mode == kWorldPipelineBump)
	{
		pipeline_info.PSODesc.Name = "ltjs_world_bump";
	}
	else
	{
		pipeline_info.PSODesc.Name = "ltjs_world_unlit";
	}
	pipeline_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	pipeline_info.GraphicsPipeline.NumRenderTargets = 1;
	pipeline_info.GraphicsPipeline.RTVFormats[0] = swap_desc.ColorBufferFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = swap_desc.DepthBufferFormat;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
	if (mode == kWorldPipelineDynamicLight)
	{
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
		pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_EQUAL;
		auto& blend_desc = pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0];
		blend_desc.BlendEnable = true;
		blend_desc.SrcBlend = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlend = Diligent::BLEND_FACTOR_ONE;
		blend_desc.BlendOp = Diligent::BLEND_OPERATION_ADD;
		blend_desc.SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
		blend_desc.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
		blend_desc.BlendOpAlpha = Diligent::BLEND_OPERATION_ADD;
	}
	else
	{
		if (!depth_enabled || blend_mode != kWorldBlendSolid)
		{
			pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = false;
		}
		if (!depth_enabled)
		{
			pipeline_info.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_ALWAYS;
		}
		diligent_fill_world_blend_desc(blend_mode, pipeline_info.GraphicsPipeline.BlendDesc.RenderTargets[0]);
	}

	pipeline_info.pVS = g_world_resources.vertex_shader;
	if (mode == kWorldPipelineLightmap)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_lightmap;
	}
	else if (mode == kWorldPipelineLightmapOnly)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_lightmap_only;
	}
	else if (mode == kWorldPipelineDualTexture)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_dual;
	}
	else if (mode == kWorldPipelineLightmapDual)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_lightmap_dual;
	}
	else if (mode == kWorldPipelineTextured)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_textured;
	}
	else if (mode == kWorldPipelineGlowTextured)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_glow;
	}
	else if (mode == kWorldPipelineDynamicLight)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_dynamic_light;
	}
	else if (mode == kWorldPipelineBump)
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_bump;
	}
	else
	{
		pipeline_info.pPS = g_world_resources.pixel_shader_solid;
	}

	pipeline_info.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::ShaderResourceVariableDesc variables[] = {
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture0", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture1", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
		{Diligent::SHADER_TYPE_PIXEL, "g_Texture2", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
	};

	Diligent::ImmutableSamplerDesc sampler_desc[3];
	sampler_desc[0].ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc[0].Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[0].Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[0].Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[0].TextureName = "g_Texture0";
	sampler_desc[1].ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc[1].Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[1].Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[1].Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[1].TextureName = "g_Texture1";
	sampler_desc[2].ShaderStages = Diligent::SHADER_TYPE_PIXEL;
	sampler_desc[2].Desc.MinFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[2].Desc.MagFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[2].Desc.MipFilter = Diligent::FILTER_TYPE_LINEAR;
	sampler_desc[2].TextureName = "g_Texture2";

	if (mode == kWorldPipelineTextured || mode == kWorldPipelineGlowTextured)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 1u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1u;
	}
	else if (mode == kWorldPipelineLightmap)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 2u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 2u;
	}
	else if (mode == kWorldPipelineLightmapOnly)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables + 1;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 1u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc + 1;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1u;
	}
	else if (mode == kWorldPipelineDualTexture)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 2u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 2u;
	}
	else if (mode == kWorldPipelineBump)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 2u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 2u;
	}
	else if (mode == kWorldPipelineLightmapDual)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 3u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 3u;
	}
	else
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = nullptr;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 0u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = nullptr;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 0u;
	}

	DiligentWorldPipeline pipeline;
	g_render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
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
	auto result = g_world_pipelines.pipelines.emplace(key, std::move(pipeline));
	return result.second ? &result.first->second : nullptr;
}

bool diligent_ensure_world_vertex_buffer(uint32 size)
{
	if (g_world_resources.vertex_buffer && g_world_resources.vertex_buffer_size >= size)
	{
		return true;
	}

	if (!g_render_device || size == 0)
	{
		return false;
	}

	g_world_resources.vertex_buffer.Release();
	g_world_resources.vertex_buffer_size = 0;

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_world_vertices";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = sizeof(DiligentWorldVertex);

	g_render_device->CreateBuffer(desc, nullptr, &g_world_resources.vertex_buffer);
	if (!g_world_resources.vertex_buffer)
	{
		return false;
	}

	g_world_resources.vertex_buffer_size = size;
	return true;
}

bool diligent_ensure_world_index_buffer(uint32 size)
{
	if (g_world_resources.index_buffer && g_world_resources.index_buffer_size >= size)
	{
		return true;
	}

	if (!g_render_device || size == 0)
	{
		return false;
	}

	g_world_resources.index_buffer.Release();
	g_world_resources.index_buffer_size = 0;

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_world_indices";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_INDEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = sizeof(uint16);

	g_render_device->CreateBuffer(desc, nullptr, &g_world_resources.index_buffer);
	if (!g_world_resources.index_buffer)
	{
		return false;
	}

	g_world_resources.index_buffer_size = size;
	return true;
}

void diligent_set_world_vertex_color(DiligentWorldVertex& vertex, uint32 color)
{
	vertex.color[0] = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
	vertex.color[1] = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
	vertex.color[2] = static_cast<float>(color & 0xFF) / 255.0f;
	vertex.color[3] = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
}

bool diligent_draw_world_immediate(
	const DiligentWorldConstants& constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* texture1_view,
	bool use_bump,
	const DiligentWorldVertex* vertices,
	uint32 vertex_count,
	const uint16* indices,
	uint32 index_count)
{
	if (!g_immediate_context || !g_swap_chain)
	{
		return false;
	}

	if (!vertices || vertex_count == 0 || !indices || index_count == 0)
	{
		return true;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

	const uint32 vertex_bytes = vertex_count * sizeof(DiligentWorldVertex);
	const uint32 index_bytes = index_count * sizeof(uint16);
	if (!diligent_ensure_world_vertex_buffer(vertex_bytes) || !diligent_ensure_world_index_buffer(index_bytes))
	{
		return false;
	}

	void* mapped_vertices = nullptr;
	g_immediate_context->MapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_vertices);
	if (!mapped_vertices)
	{
		return false;
	}
	std::memcpy(mapped_vertices, vertices, vertex_bytes);
	g_immediate_context->UnmapBuffer(g_world_resources.vertex_buffer, Diligent::MAP_WRITE);

	void* mapped_indices = nullptr;
	g_immediate_context->MapBuffer(g_world_resources.index_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_indices);
	if (!mapped_indices)
	{
		return false;
	}
	std::memcpy(mapped_indices, indices, index_bytes);
	g_immediate_context->UnmapBuffer(g_world_resources.index_buffer, Diligent::MAP_WRITE);

	auto* render_target = g_swap_chain->GetCurrentBackBufferRTV();
	auto* depth_target = g_swap_chain->GetDepthBufferDSV();
	g_immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::IBuffer* buffers[] = {g_world_resources.vertex_buffer};
	Diligent::Uint64 offsets[] = {0};
	g_immediate_context->SetVertexBuffers(
		0,
		1,
		buffers,
		offsets,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	g_immediate_context->SetIndexBuffer(g_world_resources.index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	uint8 mode = kWorldPipelineSolid;
	if (use_bump && texture_view && texture1_view)
	{
		mode = kWorldPipelineBump;
	}
	else if (texture_view && texture1_view)
	{
		mode = kWorldPipelineDualTexture;
	}
	else if (texture_view)
	{
		mode = kWorldPipelineTextured;
	}
	auto* pipeline = diligent_get_world_pipeline(mode, blend_mode, depth_mode);
	if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
	{
		return false;
	}

	if (texture_view)
	{
		auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
		if (texture_var)
		{
			texture_var->Set(texture_view);
		}
	}

	if (texture1_view)
	{
		auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
		if (texture_var)
		{
			texture_var->Set(texture1_view);
		}
	}

	g_immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs draw_attribs;
	draw_attribs.NumIndices = index_count;
	draw_attribs.IndexType = Diligent::VT_UINT16;
	draw_attribs.FirstIndexLocation = 0;
	draw_attribs.BaseVertex = 0;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_immediate_context->DrawIndexed(draw_attribs);

	return true;
}

LTMatrix diligent_build_transform(const LTVector& position, const LTRotation& rotation, const LTVector& scale)
{
	LTMatrix matrix;
	quat_ConvertToMatrix(reinterpret_cast<const float*>(&rotation), matrix.m);

	matrix.m[0][0] *= scale.x;
	matrix.m[1][0] *= scale.x;
	matrix.m[2][0] *= scale.x;

	matrix.m[0][1] *= scale.y;
	matrix.m[1][1] *= scale.y;
	matrix.m[2][1] *= scale.y;

	matrix.m[0][2] *= scale.z;
	matrix.m[1][2] *= scale.z;
	matrix.m[2][2] *= scale.z;

	matrix.m[0][3] = position.x;
	matrix.m[1][3] = position.y;
	matrix.m[2][3] = position.z;

	return matrix;
}

DiligentWorldBlendMode diligent_get_object_blend_mode(const LTObject* object)
{
	if (!object)
	{
		return kWorldBlendSolid;
	}

	if (object->m_Flags2 & FLAG2_ADDITIVE)
	{
		return kWorldBlendAdditiveOne;
	}

	if (object->m_Flags2 & FLAG2_MULTIPLY)
	{
		return kWorldBlendMultiply;
	}

	return object->IsTranslucent() ? kWorldBlendAlpha : kWorldBlendSolid;
}

void diligent_fill_world_constants(
	const ViewParams& view_params,
	const LTMatrix& world_matrix,
	bool fog_enabled,
	float fog_near,
	float fog_far,
	DiligentWorldConstants& constants)
{
	LTMatrix mvp = view_params.m_mProjection * view_params.m_mView * world_matrix;
	diligent_store_matrix_from_lt(mvp, constants.mvp);
	diligent_store_matrix_from_lt(view_params.m_mView, constants.view);
	diligent_store_matrix_from_lt(world_matrix, constants.world);
	constants.camera_pos[0] = view_params.m_Pos.x;
	constants.camera_pos[1] = view_params.m_Pos.y;
	constants.camera_pos[2] = view_params.m_Pos.z;
	constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
	constants.fog_color[0] = static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f;
	constants.fog_color[1] = static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f;
	constants.fog_color[2] = static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f;
	constants.fog_color[3] = fog_near;
	constants.fog_params[0] = fog_far;
}

struct DiligentSpriteVertex
{
	LTVector m_Vec;
	uint32 color;
	float u;
	float v;

	void SetupVert(const LTVector& pos, uint32 rgba, float tu, float tv)
	{
		m_Vec = pos;
		color = rgba;
		u = tu;
		v = tv;
	}

	static void ClipExtra(DiligentSpriteVertex* prev, DiligentSpriteVertex* cur, DiligentSpriteVertex* out, float t)
	{
		out->u = prev->u + t * (cur->u - prev->u);
		out->v = prev->v + t * (cur->v - prev->v);
		out->color = cur->color;
	}
};

template<class T>
bool diligent_clip_sprite(SpriteInstance* instance, HPOLY poly, T** points, uint32* point_count, T* out_points)
{
	if (!world_bsp_client || !instance || poly == INVALID_HPOLY)
	{
		return false;
	}

	WorldPoly* world_poly = world_bsp_client->GetPolyFromHPoly(poly);
	if (!world_poly)
	{
		return false;
	}

	float d1 = 0.0f;
	float d2 = 0.0f;
	const float dot = world_poly->GetPlane()->DistTo(g_ViewParams.m_Pos);
	if (dot <= 0.01f)
	{
		return false;
	}

	T* pVerts = *points;
	uint32 nVerts = *point_count;
	T* pOut = out_points;

	LTPlane plane;
	SPolyVertex* end_point = &world_poly->GetVertices()[world_poly->GetNumVertices()];
	SPolyVertex* prev_point = end_point - 1;
	for (SPolyVertex* cur_point = world_poly->GetVertices(); cur_point != end_point; )
	{
		LTVector vec_to;
		VEC_SUB(vec_to, cur_point->m_Vertex->m_Vec, prev_point->m_Vertex->m_Vec);
		VEC_CROSS(plane.m_Normal, vec_to, world_poly->GetPlane()->m_Normal);
		VEC_NORM(plane.m_Normal);
		plane.m_Dist = VEC_DOT(plane.m_Normal, cur_point->m_Vertex->m_Vec);

		#define PLANETEST(pt) (plane.DistTo(pt) > 0.0f)
		#define DOPLANECLIP(pt1, pt2) \
			d1 = plane.DistTo(pt1); \
			d2 = plane.DistTo(pt2); \
			t = -d1 / (d2 - d1); \
			pOut->m_Vec.x = pt1.x + ((pt2.x - pt1.x) * t); \
			pOut->m_Vec.y = pt1.y + ((pt2.y - pt1.y) * t); \
			pOut->m_Vec.z = pt1.z + ((pt2.z - pt1.z) * t);
		#define CLIPTEST PLANETEST
		#define DOCLIP DOPLANECLIP
		#include "../d3d/polyclip.h"
		#undef CLIPTEST
		#undef DOCLIP
		#undef PLANETEST
		#undef DOPLANECLIP

		prev_point = cur_point;
		++cur_point;
	}

	*points = pVerts;
	*point_count = nVerts;
	return true;
}

bool diligent_draw_render_blocks_with_constants(
	const std::vector<const DiligentRenderBlock*>& blocks,
	const DiligentWorldConstants& base_constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode)
{
	if (blocks.empty())
	{
		return true;
	}

	if (!g_immediate_context || !g_swap_chain)
	{
		return false;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &base_constants, sizeof(base_constants));
	g_immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

	auto* render_target = g_swap_chain->GetCurrentBackBufferRTV();
	auto* depth_target = g_swap_chain->GetDepthBufferDSV();
	g_immediate_context->SetRenderTargets(1, &render_target, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	for (const auto* block : blocks)
	{
		if (!block || block->vertices.empty() || block->indices.empty())
		{
			continue;
		}

		if (!block->EnsureGpuBuffers())
		{
			continue;
		}

		block->UpdateLightmaps();

		Diligent::IBuffer* buffers[] = {block->vertex_buffer};
		Diligent::Uint64 offsets[] = {0};
		g_immediate_context->SetVertexBuffers(
			0,
			1,
			buffers,
			offsets,
			Diligent::SET_VERTEX_BUFFERS_FLAG_RESET,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		g_immediate_context->SetIndexBuffer(block->index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		for (const auto& section_ptr : block->sections)
		{
			if (!section_ptr)
			{
				continue;
			}

			auto& section = *section_ptr;
			Diligent::ITextureView* texture_view = nullptr;
			Diligent::ITextureView* dual_texture_view = nullptr;
			if (section.textures[0])
			{
				texture_view = diligent_get_texture_view(section.textures[0], false);
			}
			if (section.textures[1])
			{
				dual_texture_view = diligent_get_texture_view(section.textures[1], false);
			}

			Diligent::ITextureView* lightmap_view = diligent_get_lightmap_view(section);

			uint8 mode = kWorldPipelineSkip;
			switch (static_cast<DiligentPCShaderType>(section.shader_code))
			{
				case kPcShaderGouraud:
					mode = texture_view ? kWorldPipelineTextured : kWorldPipelineSolid;
					break;
				case kPcShaderLightmap:
					mode = lightmap_view ? kWorldPipelineLightmapOnly : kWorldPipelineSolid;
					break;
				case kPcShaderLightmapTexture:
					if (texture_view && lightmap_view)
					{
						mode = kWorldPipelineLightmap;
					}
					else if (texture_view)
					{
						mode = kWorldPipelineTextured;
					}
					else if (lightmap_view)
					{
						mode = kWorldPipelineLightmapOnly;
					}
					else
					{
						mode = kWorldPipelineSolid;
					}
					break;
				case kPcShaderDualTexture:
					if (texture_view && dual_texture_view)
					{
						mode = kWorldPipelineDualTexture;
					}
					else if (texture_view)
					{
						mode = kWorldPipelineTextured;
					}
					else
					{
						mode = kWorldPipelineSolid;
					}
					break;
				case kPcShaderLightmapDualTexture:
					if (texture_view && lightmap_view && dual_texture_view)
					{
						mode = kWorldPipelineLightmapDual;
					}
					else if (texture_view && dual_texture_view)
					{
						mode = kWorldPipelineDualTexture;
					}
					else if (texture_view && lightmap_view)
					{
						mode = kWorldPipelineLightmap;
					}
					else if (texture_view)
					{
						mode = kWorldPipelineTextured;
					}
					else if (lightmap_view)
					{
						mode = kWorldPipelineLightmapOnly;
					}
					else
					{
						mode = kWorldPipelineSolid;
					}
					break;
				case kPcShaderSkypan:
				case kPcShaderSkyPortal:
				case kPcShaderOccluder:
				case kPcShaderNone:
				case kPcShaderUnknown:
				default:
					mode = kWorldPipelineSkip;
					break;
			}

			if (mode == kWorldPipelineSkip)
			{
				continue;
			}

			auto* pipeline = diligent_get_world_pipeline(mode, blend_mode, depth_mode);
			if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
			{
				continue;
			}

			if (mode == kWorldPipelineTextured || mode == kWorldPipelineGlowTextured)
			{
				auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (texture_var)
				{
					texture_var->Set(texture_view);
				}
			}
			else if (mode == kWorldPipelineLightmap)
			{
				auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (base_var)
				{
					base_var->Set(texture_view);
				}
				auto* lightmap_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (lightmap_var)
				{
					lightmap_var->Set(lightmap_view);
				}
			}
			else if (mode == kWorldPipelineLightmapOnly)
			{
				auto* lightmap_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (lightmap_var)
				{
					lightmap_var->Set(lightmap_view);
				}
			}
			else if (mode == kWorldPipelineDualTexture)
			{
				auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (base_var)
				{
					base_var->Set(texture_view);
				}
				auto* dual_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (dual_var)
				{
					dual_var->Set(dual_texture_view);
				}
			}
			else if (mode == kWorldPipelineLightmapDual)
			{
				auto* base_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (base_var)
				{
					base_var->Set(texture_view);
				}
				auto* lightmap_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture1");
				if (lightmap_var)
				{
					lightmap_var->Set(lightmap_view);
				}
				auto* dual_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture2");
				if (dual_var)
				{
					dual_var->Set(dual_texture_view);
				}
			}

			g_immediate_context->SetPipelineState(pipeline->pipeline_state);
			g_immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			Diligent::DrawIndexedAttribs draw_attribs;
			draw_attribs.NumIndices = section.tri_count * 3;
			draw_attribs.IndexType = Diligent::VT_UINT16;
			draw_attribs.FirstIndexLocation = section.start_index;
			draw_attribs.BaseVertex = static_cast<int32>(section.start_vertex);
			draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
			g_immediate_context->DrawIndexed(draw_attribs);
		}
	}

	if (g_diligent_num_world_dynamic_lights == 0)
	{
		return true;
	}

auto* light_pipeline = diligent_get_world_pipeline(kWorldPipelineDynamicLight, kWorldBlendSolid, kWorldDepthEnabled);
	if (!light_pipeline || !light_pipeline->pipeline_state || !light_pipeline->srb)
	{
		return true;
	}

	for (uint32 light_index = 0; light_index < g_diligent_num_world_dynamic_lights; ++light_index)
	{
		const DynamicLight* light = g_diligent_world_dynamic_lights[light_index];
		if (!light || light->m_LightRadius <= 0.0f)
		{
			continue;
		}

		DiligentWorldConstants light_constants = base_constants;
		light_constants.dynamic_light_pos[0] = light->m_Pos.x;
		light_constants.dynamic_light_pos[1] = light->m_Pos.y;
		light_constants.dynamic_light_pos[2] = light->m_Pos.z;
		light_constants.dynamic_light_pos[3] = light->m_LightRadius;
		light_constants.dynamic_light_color[0] = static_cast<float>(light->m_ColorR) / 255.0f;
		light_constants.dynamic_light_color[1] = static_cast<float>(light->m_ColorG) / 255.0f;
		light_constants.dynamic_light_color[2] = static_cast<float>(light->m_ColorB) / 255.0f;
		light_constants.dynamic_light_color[3] = 1.0f;

		void* light_mapped_constants = nullptr;
		g_immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, light_mapped_constants);
		if (!light_mapped_constants)
		{
			continue;
		}
		std::memcpy(light_mapped_constants, &light_constants, sizeof(light_constants));
		g_immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

		for (const auto* block : blocks)
		{
			if (!block || block->vertices.empty() || block->indices.empty())
			{
				continue;
			}

			if (!block->EnsureGpuBuffers())
			{
				continue;
			}

			Diligent::IBuffer* buffers[] = {block->vertex_buffer};
			Diligent::Uint64 offsets[] = {0};
			g_immediate_context->SetVertexBuffers(
				0,
				1,
				buffers,
				offsets,
				Diligent::SET_VERTEX_BUFFERS_FLAG_RESET,
				Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
			g_immediate_context->SetIndexBuffer(block->index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			g_immediate_context->SetPipelineState(light_pipeline->pipeline_state);
			g_immediate_context->CommitShaderResources(light_pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			for (const auto& section_ptr : block->sections)
			{
				if (!section_ptr)
				{
					continue;
				}

				auto& section = *section_ptr;
				if (section.fullbright)
				{
					continue;
				}

				bool can_light = false;
				switch (static_cast<DiligentPCShaderType>(section.shader_code))
				{
					case kPcShaderGouraud:
					case kPcShaderLightmap:
					case kPcShaderLightmapTexture:
					case kPcShaderDualTexture:
					case kPcShaderLightmapDualTexture:
						can_light = true;
						break;
					case kPcShaderSkypan:
					case kPcShaderSkyPortal:
					case kPcShaderOccluder:
					case kPcShaderNone:
					case kPcShaderUnknown:
					default:
						can_light = false;
						break;
				}

				if (!can_light)
				{
					continue;
				}

				Diligent::DrawIndexedAttribs draw_attribs;
				draw_attribs.NumIndices = section.tri_count * 3;
				draw_attribs.IndexType = Diligent::VT_UINT16;
				draw_attribs.FirstIndexLocation = section.start_index;
				draw_attribs.BaseVertex = static_cast<int32>(section.start_vertex);
				draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
				g_immediate_context->DrawIndexed(draw_attribs);
			}
		}
	}

	return true;
}

bool diligent_draw_world_blocks()
{
	if (g_visible_render_blocks.empty())
	{
		return true;
	}

	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0);
	DiligentWorldConstants constants;
	LTMatrix world_matrix;
	world_matrix.Identity();
	LTMatrix mvp = g_ViewParams.m_mProjection * g_ViewParams.m_mView * world_matrix;
	diligent_store_matrix_from_lt(mvp, constants.mvp);
	diligent_store_matrix_from_lt(g_ViewParams.m_mView, constants.view);
	diligent_store_matrix_from_lt(world_matrix, constants.world);
	constants.camera_pos[0] = g_ViewParams.m_Pos.x;
	constants.camera_pos[1] = g_ViewParams.m_Pos.y;
	constants.camera_pos[2] = g_ViewParams.m_Pos.z;
	constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
	constants.fog_color[0] = static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f;
	constants.fog_color[1] = static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f;
	constants.fog_color[2] = static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f;
	constants.fog_color[3] = g_CV_FogNearZ.m_Val;
	constants.fog_params[0] = g_CV_FogFarZ.m_Val;
	if (g_diligent_num_world_dynamic_lights > 0)
	{
		const DynamicLight* light = g_diligent_world_dynamic_lights[0];
		constants.dynamic_light_pos[0] = light->m_Pos.x;
		constants.dynamic_light_pos[1] = light->m_Pos.y;
		constants.dynamic_light_pos[2] = light->m_Pos.z;
		constants.dynamic_light_pos[3] = light->m_LightRadius;
		constants.dynamic_light_color[0] = static_cast<float>(light->m_ColorR) / 255.0f;
		constants.dynamic_light_color[1] = static_cast<float>(light->m_ColorG) / 255.0f;
		constants.dynamic_light_color[2] = static_cast<float>(light->m_ColorB) / 255.0f;
		constants.dynamic_light_color[3] = 1.0f;
	}

	return diligent_draw_render_blocks_with_constants(
		g_visible_render_blocks,
		constants,
		kWorldBlendSolid,
		kWorldDepthEnabled);
}

bool diligent_draw_world_model_list_with_view(
	const std::vector<WorldModelInstance*>& models,
	const ViewParams& view_params,
	float fog_near_z,
	float fog_far_z,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode)
{
	if (models.empty())
	{
		return true;
	}

	const bool fog_enabled_base = (g_CV_FogEnable.m_Val != 0);

	for (auto* instance : models)
	{
		if (!instance)
		{
			continue;
		}

		auto* render_world = diligent_find_world_model(instance);
		if (!render_world)
		{
			continue;
		}

		std::vector<const DiligentRenderBlock*> blocks;
		blocks.reserve(render_world->render_blocks.size());
		for (const auto& block : render_world->render_blocks)
		{
			if (block)
			{
				blocks.push_back(block.get());
			}
		}

		const bool fog_enabled = fog_enabled_base && ((instance->m_Flags & FLAG_FOGDISABLE) == 0);
		const DiligentWorldBlendMode instance_blend =
			blend_mode == kWorldBlendSolid
				? kWorldBlendSolid
				: ((instance->m_Flags2 & FLAG2_ADDITIVE) != 0 ? kWorldBlendAdditive : kWorldBlendAlpha);

		DiligentWorldConstants constants;
		LTMatrix world_matrix = instance->m_Transform;
		LTMatrix mvp = view_params.m_mProjection * view_params.m_mView * world_matrix;
		diligent_store_matrix_from_lt(mvp, constants.mvp);
		diligent_store_matrix_from_lt(view_params.m_mView, constants.view);
		diligent_store_matrix_from_lt(world_matrix, constants.world);
		constants.camera_pos[0] = view_params.m_Pos.x;
		constants.camera_pos[1] = view_params.m_Pos.y;
		constants.camera_pos[2] = view_params.m_Pos.z;
		constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
		constants.fog_color[0] = static_cast<float>(g_CV_FogColorR.m_Val) / 255.0f;
		constants.fog_color[1] = static_cast<float>(g_CV_FogColorG.m_Val) / 255.0f;
		constants.fog_color[2] = static_cast<float>(g_CV_FogColorB.m_Val) / 255.0f;
		constants.fog_color[3] = fog_near_z;
		constants.fog_params[0] = fog_far_z;
		if (g_diligent_num_world_dynamic_lights > 0)
		{
			const DynamicLight* light = g_diligent_world_dynamic_lights[0];
			constants.dynamic_light_pos[0] = light->m_Pos.x;
			constants.dynamic_light_pos[1] = light->m_Pos.y;
			constants.dynamic_light_pos[2] = light->m_Pos.z;
			constants.dynamic_light_pos[3] = light->m_LightRadius;
			constants.dynamic_light_color[0] = static_cast<float>(light->m_ColorR) / 255.0f;
			constants.dynamic_light_color[1] = static_cast<float>(light->m_ColorG) / 255.0f;
			constants.dynamic_light_color[2] = static_cast<float>(light->m_ColorB) / 255.0f;
			constants.dynamic_light_color[3] = 1.0f;
		}

		if (!diligent_draw_render_blocks_with_constants(blocks, constants, instance_blend, depth_mode))
		{
			return false;
		}
	}

	return true;
}

bool diligent_draw_world_model_list(const std::vector<WorldModelInstance*>& models, DiligentWorldBlendMode blend_mode)
{
	return diligent_draw_world_model_list_with_view(
		models,
		g_ViewParams,
		g_CV_FogNearZ.m_Val,
		g_CV_FogFarZ.m_Val,
		blend_mode,
		kWorldDepthEnabled);
}

bool diligent_draw_world_models_glow(const std::vector<WorldModelInstance*>& models, const DiligentWorldConstants& base_constants)
{
	if (models.empty())
	{
		return true;
	}

	for (auto* instance : models)
	{
		if (!instance)
		{
			continue;
		}

		auto* render_world = diligent_find_world_model(instance);
		if (!render_world)
		{
			continue;
		}

		std::vector<const DiligentRenderBlock*> blocks;
		blocks.reserve(render_world->render_blocks.size());
		for (const auto& block : render_world->render_blocks)
		{
			if (block)
			{
				blocks.push_back(block.get());
			}
		}

		DiligentWorldConstants constants = base_constants;
		LTMatrix world_matrix = instance->m_Transform;
		LTMatrix mvp = g_ViewParams.m_mProjection * g_ViewParams.m_mView * world_matrix;
		diligent_store_matrix_from_lt(mvp, constants.mvp);
		diligent_store_matrix_from_lt(world_matrix, constants.world);

		if (!diligent_draw_world_glow(constants, blocks))
		{
			return false;
		}
	}

	return true;
}

bool diligent_collect_sky_extents(const ViewParams& params, float& min_x, float& min_y, float& max_x, float& max_y)
{
	if (g_visible_render_blocks.empty())
	{
		return false;
	}

	min_x = FLT_MAX;
	min_y = FLT_MAX;
	max_x = -FLT_MAX;
	max_y = -FLT_MAX;

	for (const auto* block : g_visible_render_blocks)
	{
		if (!block)
		{
			continue;
		}

		block->ExtendSkyBounds(params, min_x, min_y, max_x, max_y);
	}

	return min_x != FLT_MAX && min_y != FLT_MAX && max_x != -FLT_MAX && max_y != -FLT_MAX;
}

bool diligent_draw_sky_objects(SceneDesc* desc, const ViewParams& sky_params, bool& out_drew)
{
	out_drew = false;
	if (!desc || !desc->m_SkyObjects || desc->m_nSkyObjects <= 0)
	{
		return true;
	}

	std::vector<WorldModelInstance*> solid_world_models;
	std::vector<WorldModelInstance*> translucent_world_models;
	std::vector<SpriteInstance*> translucent_sprites;
	std::vector<LTPolyGrid*> polygrids;

	for (int i = 0; i < desc->m_nSkyObjects; ++i)
	{
		auto* sky_object = desc->m_SkyObjects[i];
		if (!sky_object || (sky_object->m_Flags & FLAG_VISIBLE) == 0)
		{
			continue;
		}

		if (sky_object->HasWorldModel() && g_CV_DrawWorldModels.m_Val)
		{
			auto* instance = sky_object->ToWorldModel();
			if (!instance)
			{
				continue;
			}

			if (sky_object->IsTranslucent())
			{
				translucent_world_models.push_back(instance);
			}
			else
			{
				solid_world_models.push_back(instance);
			}

			continue;
		}

		if (sky_object->m_ObjectType == OT_SPRITE && g_CV_DrawSprites.m_Val)
		{
			auto* sprite = sky_object->ToSprite();
			if (sprite)
			{
				translucent_sprites.push_back(sprite);
			}
			continue;
		}

		if (sky_object->m_ObjectType == OT_POLYGRID && g_CV_DrawPolyGrids.m_Val)
		{
			auto* grid = sky_object->ToPolyGrid();
			if (grid)
			{
				polygrids.push_back(grid);
			}
		}
	}

	if (!solid_world_models.empty())
	{
		out_drew = true;
		if (!diligent_draw_world_model_list_with_view(
				solid_world_models,
				sky_params,
				g_CV_SkyFogNearZ.m_Val,
				g_CV_SkyFogFarZ.m_Val,
				kWorldBlendSolid,
				kWorldDepthDisabled))
		{
			return false;
		}
	}

	if (!translucent_world_models.empty())
	{
		out_drew = true;
		if (!diligent_draw_world_model_list_with_view(
				translucent_world_models,
				sky_params,
				g_CV_SkyFogNearZ.m_Val,
				g_CV_SkyFogFarZ.m_Val,
				kWorldBlendAlpha,
				kWorldDepthDisabled))
		{
			return false;
		}
	}

	if (!polygrids.empty())
	{
		out_drew = true;
		if (!diligent_draw_polygrid_list(polygrids, sky_params, g_CV_SkyFogNearZ.m_Val, g_CV_SkyFogFarZ.m_Val, kWorldDepthDisabled))
		{
			return false;
		}
	}

	if (!translucent_sprites.empty())
	{
		out_drew = true;
		if (!diligent_draw_sprite_list(translucent_sprites, sky_params, g_CV_SkyFogNearZ.m_Val, g_CV_SkyFogFarZ.m_Val, kWorldDepthDisabled))
		{
			return false;
		}
	}

	return true;
}

bool diligent_draw_sky(SceneDesc* desc)
{
	if (!desc || desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	if (!g_CV_DrawSky.m_Val || !desc->m_SkyObjects || desc->m_nSkyObjects <= 0)
	{
		return true;
	}

	float min_x = 0.0f;
	float min_y = 0.0f;
	float max_x = 0.0f;
	float max_y = 0.0f;
	if (!diligent_collect_sky_extents(g_ViewParams, min_x, min_y, max_x, max_y))
	{
		return true;
	}

	if ((max_x - min_x) <= 0.9f || (max_y - min_y) <= 0.9f)
	{
		return true;
	}

	ViewBoxDef view_box;
	d3d_InitViewBox2(&view_box, 0.01f, g_CV_SkyFarZ.m_Val, g_ViewParams, min_x, min_y, max_x, max_y);

	LTMatrix mat = g_ViewParams.m_mInvView;
	mat.SetTranslation(g_ViewParams.m_SkyViewPos);

	ViewParams sky_params;
	if (!d3d_InitFrustum2(
			&sky_params,
			&view_box,
			min_x,
			min_y,
			max_x,
			max_y,
			&mat,
			LTVector(g_CV_SkyScale.m_Val, g_CV_SkyScale.m_Val, g_CV_SkyScale.m_Val),
			g_ViewParams.m_eRenderMode))
	{
		return false;
	}

	ViewParams saved_view = g_ViewParams;
	g_ViewParams = sky_params;

	bool drew = false;
	const bool ok = diligent_draw_sky_objects(desc, sky_params, drew);
	g_ViewParams = saved_view;

	if (!ok)
	{
		return false;
	}

	if (drew && g_immediate_context)
	{
		auto* depth_target = g_swap_chain ? g_swap_chain->GetDepthBufferDSV() : nullptr;
		if (depth_target)
		{
			g_immediate_context->ClearDepthStencil(
				depth_target,
				Diligent::CLEAR_DEPTH_FLAG,
				1.0f,
				0,
				Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}
	}

	return true;
}

DiligentRenderTexture* diligent_get_render_texture(SharedTexture* shared_texture, bool texture_changed);
Diligent::ITextureView* diligent_get_texture_view(SharedTexture* shared_texture, bool texture_changed);

bool diligent_draw_sprite_instance(
	const ViewParams& params,
	SpriteInstance* instance,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	if (!instance)
	{
		return true;
	}

	auto* frame = instance->m_SpriteTracker.m_pCurFrame;
	if (!frame || !frame->m_pTex)
	{
		return true;
	}

	auto* render_texture = diligent_get_render_texture(frame->m_pTex, false);
	if (!render_texture || !render_texture->srv)
	{
		return true;
	}

	const float tex_width = static_cast<float>(render_texture->width);
	const float tex_height = static_cast<float>(render_texture->height);

	const uint32 color =
		(static_cast<uint32>(instance->m_ColorA) << 24) |
		(static_cast<uint32>(instance->m_ColorR) << 16) |
		(static_cast<uint32>(instance->m_ColorG) << 8) |
		static_cast<uint32>(instance->m_ColorB);

	DiligentSpriteVertex sprite_verts[4];
	const bool is_rotatable = (instance->m_Flags & FLAG_ROTATABLESPRITE) != 0;
	const bool really_close = (instance->m_Flags & FLAG_REALLYCLOSE) != 0;

	LTVector vPos = instance->GetPos();
	LTVector vRight;
	LTVector vUp;
	LTVector vForward;
	LTVector vBasisPos;
	LTVector basis_right;
	LTVector basis_up;
	LTVector basis_forward;
	float fZ = 0.0f;

	constexpr float kSpritePositionBias = -20.0f;
	constexpr float kSpriteMinFactorDist = 10.0f;
	constexpr float kSpriteMaxFactorDist = 500.0f;
	constexpr float kSpriteMinFactor = 0.1f;
	constexpr float kSpriteMaxFactor = 2.0f;

	if (is_rotatable)
	{
		LTMatrix rotation_matrix = diligent_build_transform(vPos, instance->m_Rotation, instance->m_Scale);
		rotation_matrix.GetBasisVectors(&vRight, &vUp, &vForward);
		static_cast<void>(vForward);
		vRight *= tex_width;
		vUp *= tex_height;
		vBasisPos = params.m_Pos;
	}
	else
	{
		if (really_close)
		{
			basis_right.Init(1.0f, 0.0f, 0.0f);
			basis_up.Init(0.0f, 1.0f, 0.0f);
			basis_forward.Init(0.0f, 0.0f, 1.0f);
			vBasisPos.Init(0.0f, 0.0f, 0.0f);
		}
		else
		{
			basis_right = params.m_Right;
			basis_up = params.m_Up;
			basis_forward = params.m_Forward;
			vBasisPos = params.m_Pos;
		}

		fZ = (vPos - vBasisPos).Dot(basis_forward);
		if (!really_close && fZ < NEARZ)
		{
			return true;
		}

		float size_x = tex_width * instance->m_Scale.x;
		float size_y = tex_height * instance->m_Scale.y;

		if (instance->m_Flags & FLAG_GLOWSPRITE)
		{
			float factor = (fZ - kSpriteMinFactorDist) / (kSpriteMaxFactorDist - kSpriteMinFactorDist);
			factor = LTCLAMP(factor, 0.0f, 1.0f);
			factor = kSpriteMinFactor + ((kSpriteMaxFactor - kSpriteMinFactor) * factor);
			size_x *= factor;
			size_y *= factor;
		}

		vRight = basis_right * size_x;
		vUp = basis_up * size_y;
	}

	sprite_verts[0].SetupVert(vPos + vUp - vRight, color, 0.0f, 0.0f);
	sprite_verts[1].SetupVert(vPos + vUp + vRight, color, 1.0f, 0.0f);
	sprite_verts[2].SetupVert(vPos + vRight - vUp, color, 1.0f, 1.0f);
	sprite_verts[3].SetupVert(vPos - vRight - vUp, color, 0.0f, 1.0f);

	DiligentSpriteVertex* points = sprite_verts;
	uint32 point_count = 4;
	DiligentSpriteVertex clipped_verts[45];

	if (instance->m_ClipperPoly != INVALID_HPOLY)
	{
		if (!diligent_clip_sprite(instance, instance->m_ClipperPoly, &points, &point_count, clipped_verts))
		{
			return true;
		}
	}

	if (point_count < 3)
	{
		return true;
	}

	if ((instance->m_Flags & FLAG_SPRITEBIAS) && !really_close)
	{
		if (is_rotatable)
		{
			for (uint32 i = 0; i < point_count; ++i)
			{
				LTVector& vPt = points[i].m_Vec;
				LTVector vPtRelCamera = vPt - params.m_Pos;
				float local_z = vPtRelCamera.Dot(params.m_Forward);

				if (local_z <= NEARZ)
				{
					continue;
				}

				float bias_dist = kSpritePositionBias;
				if ((local_z + bias_dist) < NEARZ)
				{
					bias_dist = NEARZ - local_z;
				}

				float scale = 1.0f + bias_dist / local_z;
				vPt = params.m_Right * vPtRelCamera.Dot(params.m_Right) * scale +
					params.m_Up * vPtRelCamera.Dot(params.m_Up) * scale +
					(local_z + bias_dist) * params.m_Forward + params.m_Pos;
			}
		}
		else if (fZ > 0.0f)
		{
			float bias_dist = kSpritePositionBias;
			if ((fZ + bias_dist) < NEARZ)
			{
				bias_dist = NEARZ - fZ;
			}

			float scale = 1.0f + bias_dist / fZ;
			for (uint32 i = 0; i < point_count; ++i)
			{
				LTVector& vPt = points[i].m_Vec;
				vPt = basis_right * (vPt - vBasisPos).Dot(basis_right) * scale +
					basis_up * (vPt - vBasisPos).Dot(basis_up) * scale +
					(fZ + bias_dist) * basis_forward + vBasisPos;
			}
		}
	}

	std::vector<DiligentWorldVertex> vertices;
	vertices.resize(point_count);
	for (uint32 i = 0; i < point_count; ++i)
	{
		auto& dst = vertices[i];
		dst.position[0] = points[i].m_Vec.x;
		dst.position[1] = points[i].m_Vec.y;
		dst.position[2] = points[i].m_Vec.z;
		diligent_set_world_vertex_color(dst, points[i].color);
		dst.uv0[0] = points[i].u;
		dst.uv0[1] = points[i].v;
		dst.uv1[0] = 0.0f;
		dst.uv1[1] = 0.0f;
	}

	std::vector<uint16> indices;
	indices.reserve((point_count - 2) * 3);
	for (uint32 i = 1; i + 1 < point_count; ++i)
	{
		indices.push_back(0);
		indices.push_back(static_cast<uint16>(i));
		indices.push_back(static_cast<uint16>(i + 1));
	}

	DiligentWorldConstants constants;
	LTMatrix world_matrix;
	world_matrix.Identity();
	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((instance->m_Flags & FLAG_FOGDISABLE) == 0);
	diligent_fill_world_constants(params, world_matrix, fog_enabled, fog_near, fog_far, constants);

	const DiligentWorldBlendMode blend_mode = diligent_get_object_blend_mode(instance);
	return diligent_draw_world_immediate(
		constants,
		blend_mode,
		depth_mode,
		render_texture->srv,
		nullptr,
		false,
		vertices.data(),
		static_cast<uint32>(vertices.size()),
		indices.data(),
		static_cast<uint32>(indices.size()));
}

bool diligent_draw_sprite_list(
	const std::vector<SpriteInstance*>& sprites,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	for (auto* sprite : sprites)
	{
		if (!diligent_draw_sprite_instance(params, sprite, fog_near, fog_far, depth_mode))
		{
			return false;
		}
	}

	return true;
}

static LTVector diligent_compute_polygrid_normal(
	const LTPolyGrid* grid,
	int32 x_index,
	int32 y_index,
	float spacing_x,
	float spacing_z,
	float y_scale)
{
	const int32 width = static_cast<int32>(grid->m_Width);
	const int8* data = reinterpret_cast<const int8*>(grid->m_Data);

	const int32 x0 = x_index > 0 ? x_index - 1 : x_index;
	const int32 x1 = x_index + 1 < width ? x_index + 1 : x_index;
	const int32 y0 = y_index > 0 ? y_index - 1 : y_index;
	const int32 y1 = y_index + 1 < height ? y_index + 1 : y_index;

	const float left = static_cast<float>(data[y_index * width + x0]);
	const float right = static_cast<float>(data[y_index * width + x1]);
	const float up = static_cast<float>(data[y0 * width + x_index]);
	const float down = static_cast<float>(data[y1 * width + x_index]);

	LTVector normal(
		(left - right) * y_scale * spacing_z,
		spacing_x * spacing_z,
		(up - down) * y_scale * spacing_x);
	normal.Normalize();
	return normal;
}

static void diligent_compute_envmap_uv(
	const ViewParams& params,
	const LTMatrix& world_matrix,
	const LTVector& local_pos,
	const LTVector& local_normal,
	float& u_out,
	float& v_out)
{
	LTVector world_pos = local_pos;
	world_matrix.Apply(world_pos);

	LTVector world_normal = local_normal;
	world_matrix.Apply3x3(world_normal);
	world_normal.Normalize();

	LTVector view_dir = params.m_Pos - world_pos;
	const float view_mag = view_dir.Mag();
	if (view_mag > 0.0001f)
	{
		view_dir /= view_mag;
	}

	LTVector incident = -view_dir;
	const float dot_val = incident.Dot(world_normal);
	LTVector reflection = incident - (2.0f * dot_val) * world_normal;
	params.m_mView.Apply3x3(reflection);
	const float refl_mag = reflection.Mag();
	if (refl_mag > 0.0001f)
	{
		reflection /= refl_mag;
	}

	u_out = 0.5f + 0.5f * reflection.x;
	v_out = 0.5f - 0.5f * reflection.y;
}

bool diligent_draw_polygrid_instance(
	const ViewParams& params,
	LTPolyGrid* grid,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	if (!grid || !grid->m_Data || !grid->m_Indices || grid->m_nIndices == 0)
	{
		return true;
	}

	if (grid->m_Width < 2 || grid->m_Height < 2)
	{
		return true;
	}

	const bool wants_env_map = (grid->m_pEnvMap && g_CV_EnvMapPolyGrids.m_Val != 0);
	const bool wants_bump_map = wants_env_map && (grid->m_nPGFlags & PG_NORMALMAPSPRITE) && (g_CV_BumpMapPolyGrids.m_Val != 0);

	SharedTexture* texture = nullptr;
	if (grid->m_pSprite && grid->m_SpriteTracker.m_pCurFrame)
	{
		texture = grid->m_SpriteTracker.m_pCurFrame->m_pTex;
	}

	Diligent::ITextureView* texture_view = texture ? diligent_get_texture_view(texture, false) : nullptr;
	Diligent::ITextureView* env_map_view = wants_env_map ? diligent_get_texture_view(grid->m_pEnvMap, false) : nullptr;
	Diligent::ITextureView* base_view = texture_view;
	const bool use_env_map = (base_view && env_map_view);
	const bool use_bump = wants_bump_map && texture_view && env_map_view;

	const float half_width = (static_cast<float>(grid->m_Width) - 1.0f) * 0.5f;
	const float half_height = (static_cast<float>(grid->m_Height) - 1.0f) * 0.5f;

	const float x_inc = grid->GetDims().x * 2.0f / (grid->m_Width - 1);
	const float z_inc = grid->GetDims().z * 2.0f / (grid->m_Height - 1);
	const float y_scale = grid->GetDims().y / 127.0f;
	const float spacing_x = x_inc * 2.0f;
	const float spacing_z = z_inc * 2.0f;

	const float x_start = -half_width * x_inc;
	float curr_x = x_start;
	float curr_z = -half_height * z_inc;

	int8* data_pos = reinterpret_cast<int8*>(grid->m_Data);
	int8* data_end = data_pos + grid->m_Width * grid->m_Height;
	int8* line_end = data_pos + grid->m_Width;

	const float scaled_r = static_cast<float>(grid->m_ColorR) / 255.0f;
	const float scaled_g = static_cast<float>(grid->m_ColorG) / 255.0f;
	const float scaled_b = static_cast<float>(grid->m_ColorB) / 255.0f;
	const float alpha = static_cast<float>(grid->m_ColorA);

	uint32 color_table[256];
	for (uint32 i = 0; i < 256; ++i)
	{
		const auto& base_color = grid->m_ColorTable[i];
		const uint8 r = static_cast<uint8>(LTCLAMP(base_color.x * scaled_r, 0.0f, 255.0f));
		const uint8 g = static_cast<uint8>(LTCLAMP(base_color.y * scaled_g, 0.0f, 255.0f));
		const uint8 b = static_cast<uint8>(LTCLAMP(base_color.z * scaled_b, 0.0f, 255.0f));
		const uint8 a = static_cast<uint8>(LTCLAMP(alpha, 0.0f, 255.0f));
		color_table[i] = (static_cast<uint32>(a) << 24) |
			(static_cast<uint32>(r) << 16) |
			(static_cast<uint32>(g) << 8) |
			static_cast<uint32>(b);
	}

	uint32* color_lookup = color_table + 128;

	const float x_scale = grid->m_xScale / ((grid->m_Width - 1) * x_inc);
	const float z_scale = grid->m_yScale / ((grid->m_Height - 1) * z_inc);

	float uv_scale = 1.0f;
	if (use_bump)
	{
		auto* render_texture = diligent_get_render_texture(texture, false);
		if (render_texture && render_texture->texture_data)
		{
			uv_scale = render_texture->texture_data->m_Header.GetDetailTextureScale();
		}
	}

	const float start_u = static_cast<float>(fmod(grid->m_xPan * uv_scale, 1.0f));
	const float start_v = static_cast<float>(fmod(grid->m_yPan * uv_scale, 1.0f));

	float curr_u = start_u;
	float curr_v = start_v;
	const float u_inc = x_inc * x_scale * uv_scale;
	const float v_inc = z_inc * z_scale * uv_scale;

	LTMatrix world_matrix = diligent_build_transform(grid->GetPos(), grid->m_Rotation, LTVector(1.0f, 1.0f, 1.0f));
	std::vector<DiligentWorldVertex> vertices;
	vertices.reserve(static_cast<size_t>(grid->m_Width) * grid->m_Height);

	int32 x_index = 0;
	int32 y_index = 0;

	if (grid->m_pValidMask)
	{
		uint32* curr_mask = grid->m_pValidMask;
		const uint32 mask_line_adjust = (grid->m_Width % 32) ? 1 : 0;
		uint32 shift = 1;

		while (data_pos < data_end)
		{
			shift = 1;
			while (data_pos < line_end)
			{
				if (*curr_mask & shift)
				{
					DiligentWorldVertex vertex{};
					vertex.position[0] = curr_x;
					const float height_value = static_cast<float>(*data_pos) * y_scale;
					vertex.position[1] = height_value;
					vertex.position[2] = curr_z;
					diligent_set_world_vertex_color(vertex, color_lookup[*data_pos]);
					vertex.uv0[0] = curr_u;
					vertex.uv0[1] = curr_v;
					if (use_env_map)
					{
						const LTVector local_pos(curr_x, height_value, curr_z);
						const LTVector local_normal = diligent_compute_polygrid_normal(grid, x_index, y_index, spacing_x, spacing_z, y_scale);
						diligent_compute_envmap_uv(params, world_matrix, local_pos, local_normal, vertex.uv1[0], vertex.uv1[1]);
					}
					else
					{
						vertex.uv1[0] = 0.0f;
						vertex.uv1[1] = 0.0f;
					}
					vertices.push_back(vertex);
				}

				++data_pos;
				curr_x += x_inc;
				curr_u += u_inc;
				++x_index;

				if (shift == 0x80000000)
				{
					++curr_mask;
					shift = 1;
				}
				else
				{
					shift <<= 1;
				}
			}

			curr_x = x_start;
			line_end += grid->m_Width;
			curr_mask += mask_line_adjust;
			curr_z += z_inc;
			curr_u = start_u;
			curr_v += v_inc;
			x_index = 0;
			++y_index;
		}
	}
	else
	{
		while (data_pos < data_end)
		{
			while (data_pos < line_end)
			{
				DiligentWorldVertex vertex{};
				vertex.position[0] = curr_x;
				const float height_value = static_cast<float>(*data_pos) * y_scale;
				vertex.position[1] = height_value;
				vertex.position[2] = curr_z;
				diligent_set_world_vertex_color(vertex, color_lookup[*data_pos]);
				vertex.uv0[0] = curr_u;
				vertex.uv0[1] = curr_v;
				if (use_env_map)
				{
					const LTVector local_pos(curr_x, height_value, curr_z);
					const LTVector local_normal = diligent_compute_polygrid_normal(grid, x_index, y_index, spacing_x, spacing_z, y_scale);
					diligent_compute_envmap_uv(params, world_matrix, local_pos, local_normal, vertex.uv1[0], vertex.uv1[1]);
				}
				else
				{
					vertex.uv1[0] = 0.0f;
					vertex.uv1[1] = 0.0f;
				}
				vertices.push_back(vertex);

				++data_pos;
				curr_x += x_inc;
				curr_u += u_inc;
				++x_index;
			}

			curr_x = x_start;
			line_end += grid->m_Width;
			curr_z += z_inc;
			curr_u = start_u;
			curr_v += v_inc;
			x_index = 0;
			++y_index;
		}
	}

	if (vertices.empty())
	{
		return true;
	}

	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((grid->m_Flags & FLAG_FOGDISABLE) == 0);
	DiligentWorldConstants constants;
	diligent_fill_world_constants(params, world_matrix, fog_enabled, fog_near, fog_far, constants);
	if (use_bump)
	{
		constants.dynamic_light_pos[0] = g_CV_EnvBumpMapScale.m_Val;
	}

	const DiligentWorldBlendMode blend_mode = diligent_get_object_blend_mode(grid);
	return diligent_draw_world_immediate(
		constants,
		blend_mode,
		depth_mode,
		base_view,
		use_env_map ? env_map_view : nullptr,
		use_bump,
		vertices.data(),
		static_cast<uint32>(vertices.size()),
		grid->m_Indices,
		grid->m_nIndices);
}

bool diligent_draw_polygrid_list(
	const std::vector<LTPolyGrid*>& grids,
	const ViewParams& params,
	float fog_near,
	float fog_far,
	DiligentWorldDepthMode depth_mode)
{
	for (auto* grid : grids)
	{
		if (!diligent_draw_polygrid_instance(params, grid, fog_near, fog_far, depth_mode))
		{
			return false;
		}
	}

	return true;
}

bool diligent_draw_world_glow(const DiligentWorldConstants& constants, const std::vector<const DiligentRenderBlock*>& blocks)
{
	if (blocks.empty())
	{
		return true;
	}

	if (!g_immediate_context)
	{
		return false;
	}

	if (!diligent_ensure_world_constant_buffer())
	{
		return false;
	}

	void* mapped_constants = nullptr;
	g_immediate_context->MapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_immediate_context->UnmapBuffer(g_world_resources.constant_buffer, Diligent::MAP_WRITE);

	for (const auto* block : blocks)
	{
		if (!block || block->vertices.empty() || block->indices.empty())
		{
			continue;
		}

		if (!block->EnsureGpuBuffers())
		{
			continue;
		}

		Diligent::IBuffer* buffers[] = {block->vertex_buffer};
		Diligent::Uint64 offsets[] = {0};
		g_immediate_context->SetVertexBuffers(
			0,
			1,
			buffers,
			offsets,
			Diligent::SET_VERTEX_BUFFERS_FLAG_RESET,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		g_immediate_context->SetIndexBuffer(block->index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		for (const auto& section_ptr : block->sections)
		{
			if (!section_ptr)
			{
				continue;
			}

			auto& section = *section_ptr;
			if (!section.fullbright)
			{
				continue;
			}

			Diligent::ITextureView* texture_view = nullptr;
			if (section.textures[0])
			{
				texture_view = diligent_get_texture_view(section.textures[0], false);
			}

			if (!texture_view)
			{
				continue;
			}

			const uint8 mode = kWorldPipelineGlowTextured;
			auto* pipeline = diligent_get_world_pipeline(mode, kWorldBlendSolid, kWorldDepthEnabled);
			if (!pipeline || !pipeline->pipeline_state || !pipeline->srb)
			{
				continue;
			}

			if (mode == kWorldPipelineGlowTextured)
			{
				auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
				if (texture_var)
				{
					texture_var->Set(texture_view);
				}
			}

			g_immediate_context->SetPipelineState(pipeline->pipeline_state);
			g_immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

			Diligent::DrawIndexedAttribs draw_attribs;
			draw_attribs.NumIndices = section.tri_count * 3;
			draw_attribs.IndexType = Diligent::VT_UINT16;
			draw_attribs.FirstIndexLocation = section.start_index;
			draw_attribs.BaseVertex = static_cast<int32>(section.start_vertex);
			draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
			g_immediate_context->DrawIndexed(draw_attribs);
		}
	}

	return true;
}

DiligentPsoKey diligent_make_pso_key(
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	uint32 input_layout_hash,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format,
	Diligent::PRIMITIVE_TOPOLOGY topology)
{
	DiligentPsoKey key;
	key.render_pass_hash = diligent_hash_render_pass(pass);
	key.input_layout_hash = input_layout_hash;
	key.vertex_shader_id = shader_pass.VertexShaderID;
	key.pixel_shader_id = shader_pass.PixelShaderID;
	key.color_format = static_cast<uint32>(color_format);
	key.depth_format = static_cast<uint32>(depth_format);
	key.topology = static_cast<uint32>(topology);
	return key;
}

struct DiligentPsoKeyHash
{
	size_t operator()(const DiligentPsoKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, key.render_pass_hash);
		hash = diligent_hash_combine(hash, key.input_layout_hash);
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.vertex_shader_id));
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.pixel_shader_id));
		hash = diligent_hash_combine(hash, key.color_format);
		hash = diligent_hash_combine(hash, key.depth_format);
		hash = diligent_hash_combine(hash, key.topology);
		return static_cast<size_t>(hash);
	}
};

struct DiligentModelPipelineKeyHash
{
	size_t operator()(const DiligentModelPipelineKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, key.pso_key.render_pass_hash);
		hash = diligent_hash_combine(hash, key.pso_key.input_layout_hash);
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.pso_key.vertex_shader_id));
		hash = diligent_hash_combine(hash, static_cast<uint64>(key.pso_key.pixel_shader_id));
		hash = diligent_hash_combine(hash, key.pso_key.color_format);
		hash = diligent_hash_combine(hash, key.pso_key.depth_format);
		hash = diligent_hash_combine(hash, key.pso_key.topology);
		hash = diligent_hash_combine(hash, key.uses_texture ? 1u : 0u);
		return static_cast<size_t>(hash);
	}
};

class DiligentPsoCache
{
public:
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> GetOrCreate(
		const DiligentPsoKey& key,
		const Diligent::GraphicsPipelineStateCreateInfo& create_info)
	{
		if (!g_render_device)
		{
			return {};
		}

		auto it = cache_.find(key);
		if (it != cache_.end())
		{
			return it->second;
		}

		Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
		g_render_device->CreatePipelineState(create_info, &pipeline_state);
		if (pipeline_state)
		{
			cache_.emplace(key, pipeline_state);
		}

		return pipeline_state;
	}

	void Reset()
	{
		cache_.clear();
	}

private:
	std::unordered_map<DiligentPsoKey, Diligent::RefCntAutoPtr<Diligent::IPipelineState>, DiligentPsoKeyHash> cache_;
};

DiligentPsoCache g_pso_cache;
std::unordered_map<DiligentModelPipelineKey, DiligentModelPipeline, DiligentModelPipelineKeyHash> g_model_pipelines;

using DiligentTextureArray = std::array<SharedTexture*, 4>;

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

struct DiligentSrbKey
{
	const Diligent::IPipelineState* pipeline_state = nullptr;
	DiligentTextureArray textures{};

	bool operator==(const DiligentSrbKey& other) const
	{
		return pipeline_state == other.pipeline_state && textures == other.textures;
	}
};

struct DiligentSrbKeyHash
{
	size_t operator()(const DiligentSrbKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, static_cast<uint64>(reinterpret_cast<uintptr_t>(key.pipeline_state)));
		for (const auto* texture : key.textures)
		{
			hash = diligent_hash_combine(hash, static_cast<uint64>(reinterpret_cast<uintptr_t>(texture)));
		}
		return static_cast<size_t>(hash);
	}
};

class DiligentSrbCache
{
public:
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> GetOrCreate(
		Diligent::IPipelineState* pipeline_state,
		const DiligentTextureArray& textures)
	{
		if (!pipeline_state)
		{
			return {};
		}

		DiligentSrbKey key;
		key.pipeline_state = pipeline_state;
		key.textures = textures;

		auto it = cache_.find(key);
		if (it != cache_.end())
		{
			return it->second;
		}

		Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
		pipeline_state->CreateShaderResourceBinding(&srb, true);
		if (srb)
		{
			cache_.emplace(key, srb);
		}

		return srb;
	}

	void Reset()
	{
		cache_.clear();
	}

private:
	std::unordered_map<DiligentSrbKey, Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>, DiligentSrbKeyHash> cache_;
};

DiligentSrbCache g_srb_cache;

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

void diligent_fill_model_depth_desc(const RenderPassOp& pass, Diligent::DepthStencilDesc& desc)
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

void diligent_fill_model_raster_desc(const RenderPassOp& pass, Diligent::RasterizerDesc& desc)
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

DiligentModelPipeline* diligent_get_model_pipeline(
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	const DiligentMeshLayout& layout,
	bool uses_texture)
{
	if (!g_render_device || !g_swap_chain)
	{
		return nullptr;
	}

	if (!diligent_ensure_model_constant_buffer() || !diligent_ensure_model_pixel_shaders())
	{
		return nullptr;
	}

	DiligentModelPipelineKey pipeline_key;
	pipeline_key.uses_texture = uses_texture;
	pipeline_key.pso_key = diligent_make_pso_key(
		pass,
		shader_pass,
		layout.hash,
		g_swap_chain->GetDesc().ColorBufferFormat,
		g_swap_chain->GetDesc().DepthBufferFormat,
		Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

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
	pipeline_info.GraphicsPipeline.RTVFormats[0] = g_swap_chain->GetDesc().ColorBufferFormat;
	pipeline_info.GraphicsPipeline.DSVFormat = g_swap_chain->GetDesc().DepthBufferFormat;
	pipeline_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
	sampler_desc.TextureName = "g_Texture0";

	if (uses_texture)
	{
		pipeline_info.PSODesc.ResourceLayout.Variables = variables;
		pipeline_info.PSODesc.ResourceLayout.NumVariables = 1u;
		pipeline_info.PSODesc.ResourceLayout.ImmutableSamplers = &sampler_desc;
		pipeline_info.PSODesc.ResourceLayout.NumImmutableSamplers = 1u;
	}

	DiligentModelPipeline pipeline;
	pipeline.uses_texture = uses_texture;
	g_render_device->CreateGraphicsPipelineState(pipeline_info, &pipeline.pipeline_state);
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

struct DiligentRenderTexture
{
	Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> srv;
	TextureData* texture_data = nullptr;
	uint32 width = 0;
	uint32 height = 0;
	BPPIdent format = BPP_32;
};

SharedTexture* g_drawprim_texture = nullptr;

Diligent::TEXTURE_FORMAT diligent_map_texture_format(BPPIdent format)
{
	switch (format)
	{
		case BPP_32:
			return Diligent::TEX_FORMAT_RGBA8_UNORM;
		case BPP_24:
			return Diligent::TEX_FORMAT_RGB8_UNORM;
		case BPP_16:
			return Diligent::TEX_FORMAT_B5G6R5_UNORM;
		case BPP_S3TC_DXT1:
			return Diligent::TEX_FORMAT_BC1_UNORM;
		case BPP_S3TC_DXT3:
			return Diligent::TEX_FORMAT_BC2_UNORM;
		case BPP_S3TC_DXT5:
			return Diligent::TEX_FORMAT_BC3_UNORM;
		default:
			return Diligent::TEX_FORMAT_UNKNOWN;
	}
}

uint32 diligent_get_mip_count(const TextureData* texture_data)
{
	if (!texture_data)
	{
		return 0;
	}

	uint32 mip_count = texture_data->m_Header.m_nMipmaps;
	if (mip_count == 0)
	{
		mip_count = 1;
	}

	return LTMIN(mip_count, static_cast<uint32>(MAX_DTX_MIPMAPS));
}

void diligent_release_render_texture(SharedTexture* shared_texture)
{
	if (!shared_texture)
	{
		return;
	}

	auto* render_texture = static_cast<DiligentRenderTexture*>(shared_texture->m_pRenderData);
	if (!render_texture)
	{
		return;
	}

	delete render_texture;
	shared_texture->m_pRenderData = nullptr;
}

DiligentRenderTexture* diligent_get_render_texture(SharedTexture* shared_texture, bool texture_changed)
{
	if (!shared_texture)
	{
		return nullptr;
	}

	auto* render_texture = static_cast<DiligentRenderTexture*>(shared_texture->m_pRenderData);
	if (render_texture && !texture_changed)
	{
		return render_texture;
	}

	diligent_release_render_texture(shared_texture);

	if (!g_render_struct || !g_render_device)
	{
		return nullptr;
	}

	TextureData* texture_data = g_render_struct->GetTexture(shared_texture);
	if (!texture_data)
	{
		return nullptr;
	}

	const auto mip_count = diligent_get_mip_count(texture_data);
	if (mip_count == 0)
	{
		return nullptr;
	}

	const auto format = diligent_map_texture_format(texture_data->m_Header.GetBPPIdent());
	if (format == Diligent::TEX_FORMAT_UNKNOWN)
	{
		return nullptr;
	}

	const auto& base_mip = texture_data->m_Mips[0];
	if (base_mip.m_Width == 0 || base_mip.m_Height == 0)
	{
		return nullptr;
	}

	auto* new_render_texture = new DiligentRenderTexture();
	new_render_texture->texture_data = texture_data;
	new_render_texture->format = texture_data->m_Header.GetBPPIdent();

	Diligent::TextureDesc desc;
	desc.Name = "ltjs_shared_texture";
	desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	desc.Width = base_mip.m_Width;
	desc.Height = base_mip.m_Height;
	desc.MipLevels = mip_count;
	desc.Format = format;
	desc.Usage = Diligent::USAGE_DEFAULT;
	desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

	std::vector<Diligent::TextureSubResData> subresources;
	subresources.reserve(mip_count);
	for (uint32 level = 0; level < mip_count; ++level)
	{
		const auto& mip = texture_data->m_Mips[level];
		if (!mip.m_Data)
		{
			break;
		}

		Diligent::TextureSubResData subresource;
		subresource.pData = mip.m_Data;
		subresource.Stride = static_cast<Diligent::Uint64>(mip.m_Pitch);
		subresource.DepthStride = 0;
		subresources.push_back(subresource);
	}

	Diligent::TextureData init_data{
		subresources.data(),
		static_cast<Diligent::Uint32>(subresources.size())};

	g_render_device->CreateTexture(desc, &init_data, &new_render_texture->texture);
	if (!new_render_texture->texture)
	{
		delete new_render_texture;
		return nullptr;
	}

	new_render_texture->srv = new_render_texture->texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	new_render_texture->width = desc.Width;
	new_render_texture->height = desc.Height;

	shared_texture->m_pRenderData = new_render_texture;
	return new_render_texture;
}

Diligent::ITextureView* diligent_get_texture_view(SharedTexture* shared_texture, bool texture_changed)
{
	auto* render_texture = diligent_get_render_texture(shared_texture, texture_changed);
	return render_texture ? render_texture->srv.RawPtr() : nullptr;
}

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_buffer(
	uint32 size,
	Diligent::BIND_FLAGS bind_flags,
	const void* data,
	uint32 stride)
{
	if (!g_render_device || size == 0)
	{
		return {};
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_buffer";
	desc.Size = size;
	desc.BindFlags = bind_flags;
	desc.Usage = data ? Diligent::USAGE_IMMUTABLE : Diligent::USAGE_DEFAULT;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
	desc.ElementByteStride = stride;

	Diligent::BufferData buffer_data;
	if (data)
	{
		buffer_data.pData = data;
		buffer_data.DataSize = size;
	}

	Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
	g_render_device->CreateBuffer(desc, data ? &buffer_data : nullptr, &buffer);
	return buffer;
}

Diligent::RefCntAutoPtr<Diligent::IBuffer> diligent_create_dynamic_vertex_buffer(uint32 size, uint32 stride)
{
	if (!g_render_device || size == 0)
	{
		return {};
	}

	Diligent::BufferDesc desc;
	desc.Name = "ltjs_dynamic_vertex_buffer";
	desc.Size = size;
	desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	desc.Usage = Diligent::USAGE_DYNAMIC;
	desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	desc.ElementByteStride = stride;

	Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
	g_render_device->CreateBuffer(desc, nullptr, &buffer);
	return buffer;
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
	if (!g_render_device)
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
	file.Read(&m_reindexed_bones, sizeof(m_reindexed_bones));
	file.Read(&m_vert_stream_flags[0], sizeof(uint32) * 4);

	bool use_matrix_palettes = false;
	file.Read(&use_matrix_palettes, sizeof(use_matrix_palettes));
	if (use_matrix_palettes)
	{
		m_render_method = kRenderMatrixPalettes;
		return LoadMatrixPalettes(file);
	}

	m_render_method = kRenderDirect;
	return LoadDirect(file);
}

bool DiligentSkelMesh::LoadDirect(ILTStream& file)
{
	switch (m_max_bones_per_tri)
	{
		case 1:
			m_vert_type = eNO_WORLD_BLENDS;
			break;
		case 2:
			m_vert_type = eNONINDEXED_B1;
			break;
		case 3:
			m_vert_type = eNONINDEXED_B2;
			break;
		case 4:
			m_vert_type = eNONINDEXED_B3;
			break;
		default:
			return false;
	}

	m_weight_count = diligent_get_blend_weight_count(m_vert_type);
	if (!diligent_build_mesh_layout(m_vert_stream_flags, m_vert_type, m_layout, m_non_fixed_pipe_data))
	{
		return false;
	}
	UpdateLayoutRefs();

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

	uint32 bone_set_count = 0;
	file.Read(&bone_set_count, sizeof(bone_set_count));
	if (bone_set_count > 0)
	{
		m_bone_sets.resize(bone_set_count);
		file.Read(m_bone_sets.data(), sizeof(DiligentBoneSet) * bone_set_count);
	}

	ReCreateObject();
	return true;
}

bool DiligentSkelMesh::LoadMatrixPalettes(ILTStream& file)
{
	file.Read(&m_min_bone, sizeof(m_min_bone));
	file.Read(&m_max_bone, sizeof(m_max_bone));

	switch (m_max_bones_per_vert)
	{
		case 2:
			m_vert_type = eINDEXED_B1;
			break;
		case 3:
			m_vert_type = eINDEXED_B2;
			break;
		case 4:
			m_vert_type = eINDEXED_B3;
			break;
		default:
			return false;
	}

	m_weight_count = diligent_get_blend_weight_count(m_vert_type);
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

	if (!diligent_build_mesh_layout(m_vert_stream_flags, m_vert_type, m_layout, m_non_fixed_pipe_data))
	{
		return false;
	}
	UpdateLayoutRefs();

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

void DiligentSkelMesh::ReCreateObject()
{
	FreeDeviceObjects();
	UpdateLayoutRefs();
	if (!g_render_device)
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

		if (m_dynamic_streams[i])
		{
			m_vertex_buffers[i] = diligent_create_dynamic_vertex_buffer(
				static_cast<uint32>(m_vertex_data[i].size()),
				stride);
		}
		else
		{
			m_vertex_buffers[i] = diligent_create_buffer(
				static_cast<uint32>(m_vertex_data[i].size()),
				Diligent::BIND_VERTEX_BUFFER,
				m_vertex_data[i].data(),
				stride);
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

bool DiligentSkelMesh::UpdateSkinnedVertices(ModelInstance* instance)
{
	if (!instance || !g_immediate_context)
	{
		return false;
	}

	Model* model = instance->GetModelDB();
	if (!model)
	{
		return false;
	}

	DDMatrix* dd_transforms = instance->GetRenderingTransforms();
	if (!dd_transforms)
	{
		return false;
	}

	const uint32 node_count = model->NumNodes();
	if (node_count == 0)
	{
		return false;
	}

	m_bone_transforms.resize(node_count);
	for (uint32 i = 0; i < node_count; ++i)
	{
		Convert_DDtoDI(dd_transforms[i], m_bone_transforms[i]);
	}

	if (m_render_method == kRenderMatrixPalettes)
	{
		return UpdateSkinnedVerticesIndexed(m_bone_transforms);
	}

	return UpdateSkinnedVerticesDirect(m_bone_transforms);
}

bool DiligentSkelMesh::UpdateSkinnedVerticesDirect(const std::vector<LTMatrix>& bone_transforms)
{
	if (!m_position_ref.valid)
	{
		return false;
	}

	const uint32 position_stream = m_position_ref.stream_index;
	if (!m_vertex_buffers[position_stream] || m_vertex_data[position_stream].empty())
	{
		return false;
	}

	if (m_weight_count > 0 && !m_weights_ref.valid)
	{
		return false;
	}

	if (m_bone_sets.empty())
	{
		return false;
	}

	void* mapped_position = nullptr;
	g_immediate_context->MapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_position);
	if (!mapped_position)
	{
		return false;
	}
	std::memcpy(mapped_position, m_vertex_data[position_stream].data(), m_vertex_data[position_stream].size());

	void* mapped_normal = mapped_position;
	bool normal_mapped = false;
	if (m_normal_ref.valid && m_normal_ref.stream_index != position_stream)
	{
		const uint32 normal_stream = m_normal_ref.stream_index;
		if (!m_vertex_buffers[normal_stream] || m_vertex_data[normal_stream].empty())
		{
			g_immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
			return false;
		}

		g_immediate_context->MapBuffer(m_vertex_buffers[normal_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_normal);
		if (!mapped_normal)
		{
			g_immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
			return false;
		}
		std::memcpy(mapped_normal, m_vertex_data[normal_stream].data(), m_vertex_data[normal_stream].size());
		normal_mapped = true;
	}

	const uint32 bone_count = LTMIN(m_weight_count + 1u, 4u);
	const uint8* position_base = m_vertex_data[position_stream].data();

	for (const auto& bone_set : m_bone_sets)
	{
		uint32 start = bone_set.first_vert_index;
		uint32 end = static_cast<uint32>(bone_set.first_vert_index) + bone_set.vert_count;
		if (start >= m_vertex_count)
		{
			continue;
		}
		end = LTMIN(end, m_vertex_count);

		for (uint32 vertex = start; vertex < end; ++vertex)
		{
			const uint8* base_pos = position_base + vertex * m_position_ref.stride;
			const float* position = reinterpret_cast<const float*>(base_pos + m_position_ref.offset);

			float weights[4] = {};
			float weight_sum = 0.0f;
			if (m_weight_count > 0)
			{
				const uint8* base_weights = m_vertex_data[m_weights_ref.stream_index].data() + vertex * m_weights_ref.stride + m_weights_ref.offset;
				const float* weight_values = reinterpret_cast<const float*>(base_weights);
				for (uint32 i = 0; i < m_weight_count; ++i)
				{
					weights[i] = weight_values[i];
					weight_sum += weights[i];
				}
				if (m_weight_count < bone_count)
				{
					weights[m_weight_count] = 1.0f - weight_sum;
				}
			}
			else
			{
				weights[0] = 1.0f;
			}

			LTVector base_pos_vec(position[0], position[1], position[2]);
			LTVector skinned_pos(0.0f, 0.0f, 0.0f);

			bool has_normal = m_normal_ref.valid;
			LTVector base_normal_vec;
			if (has_normal)
			{
				const uint8* base_normal = m_vertex_data[m_normal_ref.stream_index].data() + vertex * m_normal_ref.stride;
				const float* normal = reinterpret_cast<const float*>(base_normal + m_normal_ref.offset);
				base_normal_vec.Init(normal[0], normal[1], normal[2]);
			}
			LTVector skinned_normal(0.0f, 0.0f, 0.0f);

			for (uint32 i = 0; i < bone_count; ++i)
			{
				const uint8 bone_index = bone_set.bone_set[i];
				if (bone_index == 0xFF || bone_index >= bone_transforms.size())
				{
					continue;
				}

				const float weight = weights[i];
				if (weight == 0.0f)
				{
					continue;
				}

				const LTMatrix& bone_matrix = bone_transforms[bone_index];
				LTVector transformed = bone_matrix * base_pos_vec;
				skinned_pos.x += transformed.x * weight;
				skinned_pos.y += transformed.y * weight;
				skinned_pos.z += transformed.z * weight;

				if (has_normal)
				{
					LTVector transformed_normal;
					MatVMul_3x3(&transformed_normal, &bone_matrix, &base_normal_vec);
					skinned_normal.x += transformed_normal.x * weight;
					skinned_normal.y += transformed_normal.y * weight;
					skinned_normal.z += transformed_normal.z * weight;
				}
			}

			float* out_position = reinterpret_cast<float*>(static_cast<uint8*>(mapped_position) + vertex * m_position_ref.stride + m_position_ref.offset);
			out_position[0] = skinned_pos.x;
			out_position[1] = skinned_pos.y;
			out_position[2] = skinned_pos.z;

			if (has_normal)
			{
				float* out_normal = reinterpret_cast<float*>(static_cast<uint8*>(mapped_normal) + vertex * m_normal_ref.stride + m_normal_ref.offset);
				out_normal[0] = skinned_normal.x;
				out_normal[1] = skinned_normal.y;
				out_normal[2] = skinned_normal.z;
			}
		}
	}

	g_immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
	if (normal_mapped)
	{
		g_immediate_context->UnmapBuffer(m_vertex_buffers[m_normal_ref.stream_index], Diligent::MAP_WRITE);
	}

	return true;
}

bool DiligentSkelMesh::UpdateSkinnedVerticesIndexed(const std::vector<LTMatrix>& bone_transforms)
{
	if (!m_position_ref.valid)
	{
		return false;
	}

	const uint32 position_stream = m_position_ref.stream_index;
	if (!m_vertex_buffers[position_stream] || m_vertex_data[position_stream].empty())
	{
		return false;
	}

	if (m_weight_count > 0 && !m_weights_ref.valid)
	{
		return false;
	}

	if (!m_indices_ref.valid)
	{
		return false;
	}

	void* mapped_position = nullptr;
	g_immediate_context->MapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_position);
	if (!mapped_position)
	{
		return false;
	}
	std::memcpy(mapped_position, m_vertex_data[position_stream].data(), m_vertex_data[position_stream].size());

	void* mapped_normal = mapped_position;
	bool normal_mapped = false;
	if (m_normal_ref.valid && m_normal_ref.stream_index != position_stream)
	{
		const uint32 normal_stream = m_normal_ref.stream_index;
		if (!m_vertex_buffers[normal_stream] || m_vertex_data[normal_stream].empty())
		{
			g_immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
			return false;
		}

		g_immediate_context->MapBuffer(m_vertex_buffers[normal_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_normal);
		if (!mapped_normal)
		{
			g_immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
			return false;
		}
		std::memcpy(mapped_normal, m_vertex_data[normal_stream].data(), m_vertex_data[normal_stream].size());
		normal_mapped = true;
	}

	const uint32 bone_count = LTMIN(m_weight_count + 1u, 4u);
	const uint8* position_base = m_vertex_data[position_stream].data();

	for (uint32 vertex = 0; vertex < m_vertex_count; ++vertex)
	{
		const uint8* base_pos = position_base + vertex * m_position_ref.stride;
		const float* position = reinterpret_cast<const float*>(base_pos + m_position_ref.offset);

		float weights[4] = {};
		float weight_sum = 0.0f;
		if (m_weight_count > 0)
		{
			const uint8* base_weights = m_vertex_data[m_weights_ref.stream_index].data() + vertex * m_weights_ref.stride + m_weights_ref.offset;
			const float* weight_values = reinterpret_cast<const float*>(base_weights);
			for (uint32 i = 0; i < m_weight_count; ++i)
			{
				weights[i] = weight_values[i];
				weight_sum += weights[i];
			}
			if (m_weight_count < bone_count)
			{
				weights[m_weight_count] = 1.0f - weight_sum;
			}
		}
		else
		{
			weights[0] = 1.0f;
		}

		const uint8* base_indices = m_vertex_data[m_indices_ref.stream_index].data() + vertex * m_indices_ref.stride + m_indices_ref.offset;

		LTVector base_pos_vec(position[0], position[1], position[2]);
		LTVector skinned_pos(0.0f, 0.0f, 0.0f);

		bool has_normal = m_normal_ref.valid;
		LTVector base_normal_vec;
		if (has_normal)
		{
			const uint8* base_normal = m_vertex_data[m_normal_ref.stream_index].data() + vertex * m_normal_ref.stride;
			const float* normal = reinterpret_cast<const float*>(base_normal + m_normal_ref.offset);
			base_normal_vec.Init(normal[0], normal[1], normal[2]);
		}
		LTVector skinned_normal(0.0f, 0.0f, 0.0f);

		for (uint32 i = 0; i < bone_count; ++i)
		{
			uint32 bone_index = base_indices[i];
			if (m_reindexed_bones)
			{
				if (bone_index >= m_reindexed_bone_list.size())
				{
					continue;
				}
				bone_index = m_reindexed_bone_list[bone_index];
			}

			if (bone_index >= bone_transforms.size())
			{
				continue;
			}

			const float weight = weights[i];
			if (weight == 0.0f)
			{
				continue;
			}

			const LTMatrix& bone_matrix = bone_transforms[bone_index];
			LTVector transformed = bone_matrix * base_pos_vec;
			skinned_pos.x += transformed.x * weight;
			skinned_pos.y += transformed.y * weight;
			skinned_pos.z += transformed.z * weight;

			if (has_normal)
			{
				LTVector transformed_normal;
				MatVMul_3x3(&transformed_normal, &bone_matrix, &base_normal_vec);
				skinned_normal.x += transformed_normal.x * weight;
				skinned_normal.y += transformed_normal.y * weight;
				skinned_normal.z += transformed_normal.z * weight;
			}
		}

		float* out_position = reinterpret_cast<float*>(static_cast<uint8*>(mapped_position) + vertex * m_position_ref.stride + m_position_ref.offset);
		out_position[0] = skinned_pos.x;
		out_position[1] = skinned_pos.y;
		out_position[2] = skinned_pos.z;

		if (has_normal)
		{
			float* out_normal = reinterpret_cast<float*>(static_cast<uint8*>(mapped_normal) + vertex * m_normal_ref.stride + m_normal_ref.offset);
			out_normal[0] = skinned_normal.x;
			out_normal[1] = skinned_normal.y;
			out_normal[2] = skinned_normal.z;
		}
	}

	g_immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
	if (normal_mapped)
	{
		g_immediate_context->UnmapBuffer(m_vertex_buffers[m_normal_ref.stream_index], Diligent::MAP_WRITE);
	}

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
	m_non_fixed_pipe_data = false;
	m_dup_map_list.clear();
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
	m_dynamic_streams.fill(false);
	for (uint32 i = 0; i < 4; ++i)
	{
		m_vert_stream_flags[i] = 0;
		m_vertex_data[i].clear();
		m_vertex_buffers[i].Release();
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
	file.Read(&m_undup_vertex_count, sizeof(m_undup_vertex_count));
	file.Read(&m_poly_count, sizeof(m_poly_count));
	file.Read(&m_max_bones_per_tri, sizeof(m_max_bones_per_tri));
	file.Read(&m_max_bones_per_vert, sizeof(m_max_bones_per_vert));
	file.Read(&m_vert_stream_flags[0], sizeof(uint32) * 4);
	file.Read(&m_anim_node_idx, sizeof(m_anim_node_idx));
	file.Read(&m_bone_effector, sizeof(m_bone_effector));

	if (!diligent_build_mesh_layout(m_vert_stream_flags, eNO_WORLD_BLENDS, m_layout, m_non_fixed_pipe_data))
	{
		return false;
	}
	UpdateLayoutRefs();

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

	ReCreateObject();
	return true;
}

void DiligentVAMesh::ReCreateObject()
{
	FreeDeviceObjects();
	UpdateLayoutRefs();
	if (!g_render_device)
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

		if (m_dynamic_streams[i])
		{
			m_vertex_buffers[i] = diligent_create_dynamic_vertex_buffer(
				static_cast<uint32>(m_vertex_data[i].size()),
				stride);
		}
		else
		{
			m_vertex_buffers[i] = diligent_create_buffer(
				static_cast<uint32>(m_vertex_data[i].size()),
				Diligent::BIND_VERTEX_BUFFER,
				m_vertex_data[i].data(),
				stride);
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
	if (!model || !anim_time || !g_immediate_context)
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
	g_immediate_context->MapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_position);
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

	g_immediate_context->UnmapBuffer(m_vertex_buffers[position_stream], Diligent::MAP_WRITE);
	return true;
}

void diligent_UpdateWindowHandles(const RenderStructInit* init)
{
	g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_sdl_window = nullptr;
	if (!init)
	{
		return;
	}

	const auto* window_desc = static_cast<const ltjs::MainWindowDescriptor*>(init->m_hWnd);
	if (!window_desc)
	{
		return;
	}

	g_sdl_window = window_desc->sdl_window;
	g_native_window_handle = static_cast<HWND>(window_desc->native_handle);
#else
	if (init)
	{
		g_native_window_handle = static_cast<HWND>(init->m_hWnd);
	}
#endif
}

bool diligent_QueryWindowSize(uint32& width, uint32& height)
{
#ifdef LTJS_SDL_BACKEND
	if (!g_sdl_window)
	{
		return false;
	}

	int window_width = 0;
	int window_height = 0;
	SDL_GetWindowSize(g_sdl_window, &window_width, &window_height);
	if (window_width <= 0 || window_height <= 0)
	{
		return false;
	}

	width = static_cast<uint32>(window_width);
	height = static_cast<uint32>(window_height);
	return true;
#else
	if (!g_native_window_handle)
	{
		return false;
	}

	RECT rect{};
	if (!GetClientRect(g_native_window_handle, &rect))
	{
		return false;
	}

	const int window_width = rect.right - rect.left;
	const int window_height = rect.bottom - rect.top;
	if (window_width <= 0 || window_height <= 0)
	{
		return false;
	}

	width = static_cast<uint32>(window_width);
	height = static_cast<uint32>(window_height);
	return true;
#endif
}

void diligent_UpdateScreenSize(uint32 width, uint32 height)
{
	g_screen_width = width;
	g_screen_height = height;

	if (g_render_struct)
	{
		g_render_struct->m_Width = width;
		g_render_struct->m_Height = height;
	}
}

bool diligent_EnsureDevice()
{
	if (g_render_device && g_immediate_context)
	{
		return true;
	}

	if (!g_engine_factory)
	{
		g_engine_factory = Diligent::LoadAndGetEngineFactoryVk();
	}

	if (!g_engine_factory)
	{
		return false;
	}

	Diligent::EngineVkCreateInfo engine_create_info;
	g_engine_factory->CreateDeviceAndContextsVk(engine_create_info, &g_render_device, &g_immediate_context);
	return g_render_device && g_immediate_context;
}

bool diligent_CreateSwapChain(uint32 width, uint32 height)
{
	if (!g_engine_factory || !g_render_device || !g_immediate_context || !g_native_window_handle)
	{
		return false;
	}

	Diligent::SwapChainDesc swap_chain_desc;
	swap_chain_desc.Width = width;
	swap_chain_desc.Height = height;

	Diligent::Win32NativeWindow window{g_native_window_handle};
	g_engine_factory->CreateSwapChainVk(g_render_device, g_immediate_context, swap_chain_desc, window, &g_swap_chain);
	return static_cast<bool>(g_swap_chain);
}

bool diligent_EnsureSwapChain()
{
	if (!diligent_EnsureDevice())
	{
		return false;
	}

	uint32 width = g_screen_width;
	uint32 height = g_screen_height;
	if (!diligent_QueryWindowSize(width, height))
	{
		if (width == 0 || height == 0)
		{
			return false;
		}
	}

	if (!g_swap_chain)
	{
		if (!diligent_CreateSwapChain(width, height))
		{
			return false;
		}

		diligent_UpdateScreenSize(width, height);
		return true;
	}

	if (width != g_screen_width || height != g_screen_height)
	{
		g_swap_chain->Resize(width, height, Diligent::SURFACE_TRANSFORM_OPTIMAL);
		diligent_UpdateScreenSize(width, height);
	}

	return true;
}

HLTBUFFER diligent_CreateSurface(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return LTNULL;
	}

	const auto pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
	const auto surface_size = sizeof(DiligentSurface) + (pixel_count - 1) * sizeof(uint32);
	DiligentSurface* surface = nullptr;
	LT_MEM_TRACK_ALLOC(surface = static_cast<DiligentSurface*>(LTMemAlloc(surface_size)), LT_MEM_TYPE_RENDERER);
	if (!surface)
	{
		return LTNULL;
	}

	surface->width = static_cast<uint32>(width);
	surface->height = static_cast<uint32>(height);
	surface->pitch = static_cast<uint32>(width) * sizeof(uint32);
	surface->flags = 0;
	surface->optimized_transparent_color = OPTIMIZE_NO_TRANSPARENCY;
	memset(surface->data, 0, pixel_count * sizeof(uint32));
	return static_cast<HLTBUFFER>(surface);
}

void diligent_DeleteSurface(HLTBUFFER surface_handle)
{
	if (!surface_handle)
	{
		return;
	}

	g_surface_cache.erase(surface_handle);
	LTMemFree(surface_handle);
}

void diligent_GetSurfaceInfo(HLTBUFFER surface_handle, uint32* width, uint32* height)
{
	const auto* surface = static_cast<const DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return;
	}

	if (width)
	{
		*width = surface->width;
	}

	if (height)
	{
		*height = surface->height;
	}
}

void* diligent_LockSurface(HLTBUFFER surface_handle, uint32& pitch)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		pitch = 0;
		return nullptr;
	}

	pitch = surface->pitch;
	return surface->data;
}

void diligent_UnlockSurface(HLTBUFFER surface_handle)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (surface)
	{
		surface->flags |= kDiligentSurfaceFlagDirty;
	}
}

bool diligent_OptimizeSurface(HLTBUFFER surface_handle, uint32 transparent_color)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return false;
	}

	surface->flags |= kDiligentSurfaceFlagOptimized | kDiligentSurfaceFlagDirty;
	surface->optimized_transparent_color = transparent_color;
	return true;
}

void diligent_UnoptimizeSurface(HLTBUFFER surface_handle)
{
	auto* surface = static_cast<DiligentSurface*>(surface_handle);
	if (!surface)
	{
		return;
	}

	surface->flags &= ~kDiligentSurfaceFlagOptimized;
	g_surface_cache.erase(surface_handle);
}

bool diligent_GetScreenFormat(PFormat* format)
{
	if (!format)
	{
		return false;
	}

	format->Init(BPP_32, 0xFF000000u, 0x00FF0000u, 0x0000FF00u, 0x000000FFu);
	return true;
}

void diligent_Clear(LTRect*, uint32, LTRGBColor& clear_color)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	const float clear_rgba[4] = {
		static_cast<float>(clear_color.rgb.r) / 255.0f,
		static_cast<float>(clear_color.rgb.g) / 255.0f,
		static_cast<float>(clear_color.rgb.b) / 255.0f,
		static_cast<float>(clear_color.rgb.a) / 255.0f};

	auto* render_target = g_swap_chain->GetCurrentBackBufferRTV();
	auto* depth_target = g_swap_chain->GetDepthBufferDSV();

	if (!render_target)
	{
		return;
	}

	g_immediate_context->ClearRenderTarget(
		render_target,
		clear_rgba,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (depth_target)
	{
		g_immediate_context->ClearDepthStencil(
			depth_target,
			Diligent::CLEAR_DEPTH_FLAG,
			1.0f,
			0,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}
}

bool diligent_Start3D()
{
	g_is_in_3d = true;
	if (g_render_struct)
	{
		++g_render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_End3D()
{
	g_is_in_3d = false;
	if (g_render_struct && g_render_struct->m_nIn3D > 0)
	{
		--g_render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_IsIn3D()
{
	return g_is_in_3d;
}

bool diligent_StartOptimized2D()
{
	g_is_in_optimized_2d = true;
	if (g_render_struct)
	{
		++g_render_struct->m_nInOptimized2D;
	}
	return true;
}

void diligent_EndOptimized2D()
{
	g_is_in_optimized_2d = false;
	if (g_render_struct && g_render_struct->m_nInOptimized2D > 0)
	{
		--g_render_struct->m_nInOptimized2D;
	}
}

bool diligent_IsInOptimized2D()
{
	return g_is_in_optimized_2d;
}

bool diligent_SetOptimized2DBlend(LTSurfaceBlend blend)
{
	g_optimized_2d_blend = blend;
	return true;
}

bool diligent_GetOptimized2DBlend(LTSurfaceBlend& blend)
{
	blend = g_optimized_2d_blend;
	return true;
}

bool diligent_SetOptimized2DColor(HLTCOLOR color)
{
	g_optimized_2d_color = 0xFF000000u | (color & 0x00FFFFFFu);
	return true;
}

bool diligent_GetOptimized2DColor(HLTCOLOR& color)
{
	color = g_optimized_2d_color;
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

bool diligent_get_model_transform(ModelInstance* instance, uint32 bone_index, LTMatrix& transform)
{
	if (!instance)
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

bool diligent_draw_model_instance(ModelInstance* instance);
bool diligent_draw_model_instance_with_render_style_map(ModelInstance* instance, const CRenderStyleMap* render_style_map);

bool diligent_get_model_hook_data(ModelInstance* instance, ModelHookData& hook_data)
{
	if (!instance || !g_diligent_scene_desc)
	{
		return false;
	}

	hook_data.m_ObjectFlags = instance->m_Flags;
	hook_data.m_HookFlags = MHF_USETEXTURE;
	hook_data.m_hObject = reinterpret_cast<HLOCALOBJ>(instance);
	hook_data.m_LightAdd = g_diligent_scene_desc->m_GlobalModelLightAdd;
	hook_data.m_ObjectColor = {instance->m_ColorR, instance->m_ColorG, instance->m_ColorB};

	if (g_diligent_scene_desc->m_ModelHookFn)
	{
		g_diligent_scene_desc->m_ModelHookFn(&hook_data, g_diligent_scene_desc->m_ModelHookUser);
	}

	if (g_CV_Saturate.m_Val)
	{
		hook_data.m_ObjectColor *= g_CV_ModelSaturation.m_Val;
	}

	return true;
}

bool diligent_should_process_model(LTObject* object)
{
	if (!object || !object->IsModel())
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_Flags2 & FLAG2_SKYOBJECT)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_object_frame_code)
	{
		return false;
	}

	return true;
}

DiligentRenderWorld* diligent_find_world_model(const WorldModelInstance* instance)
{
	if (!instance || !g_render_world)
	{
		return nullptr;
	}

	const auto* bsp = instance->GetOriginalBsp();
	if (!bsp)
	{
		return nullptr;
	}

	const uint32 name_hash = st_GetHashCode(bsp->m_WorldName);
	auto it = g_render_world->world_models.find(name_hash);
	return it != g_render_world->world_models.end() ? it->second.get() : nullptr;
}

bool diligent_should_process_world_model(LTObject* object)
{
	if (!object || !object->HasWorldModel())
	{
		return false;
	}

	if (!g_CV_DrawWorldModels.m_Val)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_Flags2 & FLAG2_SKYOBJECT)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_world_model_object(LTObject* object)
{
	if (!diligent_should_process_world_model(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_object_frame_code;

	auto* instance = object->ToWorldModel();
	if (!instance)
	{
		return;
	}

	if (object->IsTranslucent())
	{
		g_diligent_translucent_world_models.push_back(instance);
	}
	else
	{
		g_diligent_solid_world_models.push_back(instance);
	}
}

bool diligent_should_process_sprite(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_SPRITE)
	{
		return false;
	}

	if (!g_CV_DrawSprites.m_Val)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_Flags2 & FLAG2_SKYOBJECT)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_sprite_object(LTObject* object)
{
	if (!diligent_should_process_sprite(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_object_frame_code;

	auto* instance = object->ToSprite();
	if (!instance)
	{
		return;
	}

	if (object->m_Flags & FLAG_SPRITE_NOZ)
	{
		g_diligent_noz_sprites.push_back(instance);
	}
	else
	{
		g_diligent_translucent_sprites.push_back(instance);
	}
}

bool diligent_should_process_polygrid(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_POLYGRID)
	{
		return false;
	}

	if (!g_CV_DrawPolyGrids.m_Val)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if (object->m_Flags2 & FLAG2_SKYOBJECT)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_polygrid_object(LTObject* object)
{
	if (!diligent_should_process_polygrid(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_object_frame_code;

	auto* grid = object->ToPolyGrid();
	if (!grid)
	{
		return;
	}

	if (object->IsTranslucent())
	{
		if (grid->m_nPGFlags & PG_RENDERBEFORETRANSLUCENTS)
		{
			g_diligent_early_translucent_polygrids.push_back(grid);
		}
		else
		{
			g_diligent_translucent_polygrids.push_back(grid);
		}
	}
	else
	{
		g_diligent_solid_polygrids.push_back(grid);
	}
}

bool diligent_should_process_canvas(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_CANVAS)
	{
		return false;
	}

	if ((object->m_Flags & FLAG_VISIBLE) == 0)
	{
		return false;
	}

	if ((object->cd.m_ClientFlags & CF_SOLIDCANVAS) == 0)
	{
		return false;
	}

	if (object->m_WTFrameCode == g_diligent_object_frame_code)
	{
		return false;
	}

	return true;
}

void diligent_process_canvas_object(LTObject* object)
{
	if (!diligent_should_process_canvas(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_object_frame_code;

	Canvas* canvas = object->ToCanvas();
	if (canvas && canvas->m_Fn)
	{
		canvas->m_Fn(canvas, canvas->m_pFnUserData);
	}
}

void diligent_process_light_object(LTObject* object)
{
	if (!object || object->m_ObjectType != OT_LIGHT)
	{
		return;
	}

	if (!g_CV_DynamicLight.m_Val)
	{
		return;
	}

	DynamicLight* light = object->ToDynamicLight();
	if (!light)
	{
		return;
	}

	if (!g_CV_DynamicLightWorld.m_Val && ((light->m_Flags2 & FLAG2_FORCEDYNAMICLIGHTWORLD) == 0))
	{
		return;
	}

	if (g_diligent_num_world_dynamic_lights >= kDiligentMaxDynamicLights)
	{
		return;
	}

	g_diligent_world_dynamic_lights[g_diligent_num_world_dynamic_lights++] = light;
}

void diligent_process_model_attachments(LTObject* object, uint32 depth)
{
	if (!object || depth >= 32 || !g_render_struct || !g_render_struct->ProcessAttachment)
	{
		return;
	}

	Attachment* current = object->m_Attachments;
	while (current)
	{
		LTObject* attached = g_render_struct->ProcessAttachment(object, current);
		if (attached && attached->m_WTFrameCode != g_diligent_object_frame_code)
		{
			if (attached->m_Attachments)
			{
				diligent_process_model_attachments(attached, depth + 1);
			}

			if (diligent_should_process_model(attached))
			{
				attached->m_WTFrameCode = g_diligent_object_frame_code;
				diligent_draw_model_instance(attached->ToModel());
			}
		}

		current = current->m_pNext;
	}
}

void diligent_process_model_object(LTObject* object)
{
	if (!diligent_should_process_model(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_object_frame_code;
	diligent_draw_model_instance(object->ToModel());
	if (object->m_Attachments)
	{
		diligent_process_model_attachments(object, 0);
	}
}

void diligent_process_model_attachments_glow(LTObject* object, uint32 depth, const CRenderStyleMap& render_style_map)
{
	if (!object || depth >= 32 || !g_render_struct || !g_render_struct->ProcessAttachment)
	{
		return;
	}

	Attachment* current = object->m_Attachments;
	while (current)
	{
		LTObject* attached = g_render_struct->ProcessAttachment(object, current);
		if (attached && attached->m_WTFrameCode != g_diligent_object_frame_code)
		{
			if (attached->m_Attachments)
			{
				diligent_process_model_attachments_glow(attached, depth + 1, render_style_map);
			}

			if (diligent_should_process_model(attached))
			{
				attached->m_WTFrameCode = g_diligent_object_frame_code;
				diligent_draw_model_instance_with_render_style_map(attached->ToModel(), &render_style_map);
			}
		}

		current = current->m_pNext;
	}
}

void diligent_process_model_object_glow(LTObject* object, const CRenderStyleMap& render_style_map)
{
	if (!diligent_should_process_model(object))
	{
		return;
	}

	object->m_WTFrameCode = g_diligent_object_frame_code;
	diligent_draw_model_instance_with_render_style_map(object->ToModel(), &render_style_map);
	if (object->m_Attachments)
	{
		diligent_process_model_attachments_glow(object, 0, render_style_map);
	}
}

void diligent_collect_world_dynamic_lights(const ViewParams& params, const WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_LIGHT)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_light_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		const WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_collect_world_dynamic_lights(params, child);
	}
}

void diligent_filter_world_node_for_models(const ViewParams& params, const WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_model_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		const WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_models(params, child);
	}
}

void diligent_filter_world_node_for_world_models(const ViewParams& params, const WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || !object->HasWorldModel())
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_world_model_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		const WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_world_models(params, child);
	}
}

void diligent_filter_world_node_for_sprites(const ViewParams& params, const WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_SPRITE)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_sprite_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		const WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_sprites(params, child);
	}
}

void diligent_filter_world_node_for_polygrids(const ViewParams& params, const WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_POLYGRID)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_polygrid_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		const WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_polygrids(params, child);
	}
}

void diligent_filter_world_node_for_canvases(const ViewParams& params, const WorldTreeNode* node)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object || object->m_ObjectType != OT_CANVAS)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_canvas_object(object);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		const WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_canvases(params, child);
	}
}

void diligent_filter_world_node_for_models_glow(const ViewParams& params, const WorldTreeNode* node, const CRenderStyleMap& render_style_map)
{
	if (!node)
	{
		return;
	}

	auto* list_head = node->m_Objects[NOA_Objects].AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		const LTVector min = object->m_Pos - object->m_Dims;
		const LTVector max = object->m_Pos + object->m_Dims;
		if (!params.ViewAABBIntersect(min, max))
		{
			continue;
		}

		diligent_process_model_object_glow(object, render_style_map);
	}

	if (!node->HasChildren())
	{
		return;
	}

	for (uint32 child_index = 0; child_index < MAX_WTNODE_CHILDREN; ++child_index)
	{
		const WorldTreeNode* child = node->GetChild(child_index);
		if (!child || child->GetNumObjectsOnOrBelow() == 0)
		{
			continue;
		}

		if (!params.ViewAABBIntersect(child->GetBBoxMin(), child->GetBBoxMax()))
		{
			continue;
		}

		diligent_filter_world_node_for_models_glow(params, child, render_style_map);
	}
}

bool diligent_update_model_constants(ModelInstance* instance, const LTMatrix& mvp, const LTMatrix& model_matrix)
{
	if (!instance || !g_immediate_context || !g_model_resources.constant_buffer)
	{
		return false;
	}

	bool fog_enabled = false;
	float fog_color_r = 0.0f;
	float fog_color_g = 0.0f;
	float fog_color_b = 0.0f;
	float fog_near = 0.0f;
	float fog_far = 0.0f;
	if (g_diligent_glow_mode)
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
	constants.color[0] = static_cast<float>(instance->m_ColorR) / 255.0f;
	constants.color[1] = static_cast<float>(instance->m_ColorG) / 255.0f;
	constants.color[2] = static_cast<float>(instance->m_ColorB) / 255.0f;
	constants.color[3] = static_cast<float>(instance->m_ColorA) / 255.0f;
	constants.camera_pos[0] = g_ViewParams.m_Pos.x;
	constants.camera_pos[1] = g_ViewParams.m_Pos.y;
	constants.camera_pos[2] = g_ViewParams.m_Pos.z;
	constants.camera_pos[3] = fog_enabled ? 1.0f : 0.0f;
	constants.fog_color[0] = fog_color_r;
	constants.fog_color[1] = fog_color_g;
	constants.fog_color[2] = fog_color_b;
	constants.fog_color[3] = fog_near;
	constants.fog_params[0] = fog_far;
	constants.fog_params[1] = 0.0f;
	constants.fog_params[2] = 0.0f;
	constants.fog_params[3] = 0.0f;

	void* mapped_constants = nullptr;
	g_immediate_context->MapBuffer(g_model_resources.constant_buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, mapped_constants);
	if (!mapped_constants)
	{
		return false;
	}
	std::memcpy(mapped_constants, &constants, sizeof(constants));
	g_immediate_context->UnmapBuffer(g_model_resources.constant_buffer, Diligent::MAP_WRITE);
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
	if (!instance || !g_immediate_context || !g_swap_chain || !vertex_buffers || !index_buffer)
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

	const bool uses_texture = textures[0] != nullptr;
	DiligentModelPipeline* pipeline = diligent_get_model_pipeline(pass, shader_pass, layout, uses_texture);
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

	g_immediate_context->SetVertexBuffers(
		0,
		4,
		bound_buffers,
		offsets,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

	g_immediate_context->SetIndexBuffer(index_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (uses_texture)
	{
		auto* texture_var = pipeline->srb->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture0");
		if (texture_var)
		{
			texture_var->Set(diligent_get_texture_view(textures[0], false));
		}
	}

	g_immediate_context->SetPipelineState(pipeline->pipeline_state);
	g_immediate_context->CommitShaderResources(pipeline->srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs draw_attribs;
	draw_attribs.NumIndices = index_count;
	draw_attribs.IndexType = Diligent::VT_UINT16;
	draw_attribs.FirstIndexLocation = 0;
	draw_attribs.BaseVertex = 0;
	draw_attribs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	g_immediate_context->DrawIndexed(draw_attribs);

	return true;
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
	diligent_get_model_transform(instance, mesh->GetBoneEffector(), model_matrix);
	LTMatrix mvp = g_ViewParams.m_mProjection * g_ViewParams.m_mView * model_matrix;

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
	LTMatrix mvp = g_ViewParams.m_mProjection * g_ViewParams.m_mView;

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
	diligent_get_model_transform(instance, mesh->GetBoneEffector(), model_matrix);
	LTMatrix mvp = g_ViewParams.m_mProjection * g_ViewParams.m_mView * model_matrix;

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

	CRenderStyle* render_style = nullptr;
	if (!instance->GetRenderStyle(piece->m_iRenderStyle, &render_style) || !render_style)
	{
		continue;
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


		const uint32 pass_count = render_style->GetRenderPassCount();
		if (pass_count == 0)
		{
			continue;
		}

		std::array<SharedTexture*, MAX_PIECE_TEXTURES> piece_textures{};
		diligent_get_model_piece_textures(instance, piece, piece_textures);

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
			if (!render_style->GetRenderPass(pass_index, &pass))
			{
				continue;
			}

			RSRenderPassShaders shader_pass{};
			render_style->GetRenderPassShaders(pass_index, &shader_pass);

			DiligentTextureArray textures = diligent_resolve_textures(pass, piece_textures.data());

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

bool diligent_draw_models(SceneDesc* desc)
{
	if (!desc)
	{
		return true;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return true;
		}

		if (g_render_struct && g_render_struct->IncObjectFrameCode)
		{
			g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
		}
		else
		{
			++g_diligent_object_frame_code;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_model_object(object);
		}

		return true;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	if (!world_bsp_client)
	{
		return true;
	}

	WorldTree* tree = world_bsp_client->ClientTree();
	if (!tree)
	{
		return true;
	}

	if (g_render_struct && g_render_struct->IncObjectFrameCode)
	{
		g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_object_frame_code;
	}

	diligent_filter_world_node_for_models(g_ViewParams, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_model_object(object);
	}

	return true;
}

void diligent_collect_world_models(SceneDesc* desc)
{
	g_diligent_solid_world_models.clear();
	g_diligent_translucent_world_models.clear();

	if (!desc || !g_CV_DrawWorldModels.m_Val)
	{
		return;
	}

	if (g_render_struct && g_render_struct->IncObjectFrameCode)
	{
		g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_world_model_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return;
	}

	if (!world_bsp_client)
	{
		return;
	}

	WorldTree* tree = world_bsp_client->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_world_models(g_ViewParams, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_world_model_object(object);
	}
}

void diligent_collect_sprites(SceneDesc* desc)
{
	g_diligent_translucent_sprites.clear();
	g_diligent_noz_sprites.clear();

	if (!desc || !g_CV_DrawSprites.m_Val)
	{
		return;
	}

	if (g_render_struct && g_render_struct->IncObjectFrameCode)
	{
		g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_sprite_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !world_bsp_client)
	{
		return;
	}

	WorldTree* tree = world_bsp_client->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_sprites(g_ViewParams, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_sprite_object(object);
	}
}

void diligent_collect_polygrids(SceneDesc* desc)
{
	g_diligent_solid_polygrids.clear();
	g_diligent_early_translucent_polygrids.clear();
	g_diligent_translucent_polygrids.clear();

	if (!desc || !g_CV_DrawPolyGrids.m_Val)
	{
		return;
	}

	if (g_render_struct && g_render_struct->IncObjectFrameCode)
	{
		g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_polygrid_object(object);
		}

		return;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL || !world_bsp_client)
	{
		return;
	}

	WorldTree* tree = world_bsp_client->ClientTree();
	if (!tree)
	{
		return;
	}

	diligent_filter_world_node_for_polygrids(g_ViewParams, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_polygrid_object(object);
	}
}

bool diligent_draw_models_glow(SceneDesc* desc, const CRenderStyleMap& render_style_map)
{
	if (!desc)
	{
		return true;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return true;
		}

		if (g_render_struct && g_render_struct->IncObjectFrameCode)
		{
			g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
		}
		else
		{
			++g_diligent_object_frame_code;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_model_object_glow(object, render_style_map);
		}

		return true;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	if (!world_bsp_client)
	{
		return true;
	}

	WorldTree* tree = world_bsp_client->ClientTree();
	if (!tree)
	{
		return true;
	}

	if (g_render_struct && g_render_struct->IncObjectFrameCode)
	{
		g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_object_frame_code;
	}

	diligent_filter_world_node_for_models_glow(g_ViewParams, tree->GetRootNode(), render_style_map);

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_model_object_glow(object, render_style_map);
	}

	return true;
}

bool diligent_draw_solid_canvases_glow(SceneDesc* desc)
{
	if (!desc)
	{
		return true;
	}

	if (!g_CV_DrawCanvases.m_Val || !g_CV_ScreenGlow_DrawCanvases.m_Val)
	{
		return true;
	}

	if (g_render_struct && g_render_struct->IncObjectFrameCode)
	{
		g_diligent_object_frame_code = g_render_struct->IncObjectFrameCode();
	}
	else
	{
		++g_diligent_object_frame_code;
	}

	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (!desc->m_pObjectList || desc->m_ObjectListSize <= 0)
		{
			return true;
		}

		for (int i = 0; i < desc->m_ObjectListSize; ++i)
		{
			LTObject* object = desc->m_pObjectList[i];
			if (!object)
			{
				continue;
			}

			diligent_process_canvas_object(object);
		}

		return true;
	}

	if (desc->m_DrawMode != DRAWMODE_NORMAL)
	{
		return true;
	}

	if (!world_bsp_client)
	{
		return true;
	}

	WorldTree* tree = world_bsp_client->ClientTree();
	if (!tree)
	{
		return true;
	}

	diligent_filter_world_node_for_canvases(g_ViewParams, tree->GetRootNode());

	auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
	for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
	{
		auto* object = static_cast<LTObject*>(current->m_pData);
		if (!object)
		{
			continue;
		}

		diligent_process_canvas_object(object);
	}

	return true;
}

bool diligent_glow_update_filter()
{
	const uint32 requested_size = LTCLAMP(g_CV_ScreenGlowFilterSize.m_Val, 0, static_cast<int>(kDiligentGlowMaxFilterSize));
	g_diligent_glow_state.filter_size = 0;
	if (requested_size == 0)
	{
		return true;
	}

	const float pixel_shift = g_CV_ScreenGlowPixelShift.m_Val;
	const float gauss_radius[2] = {g_CV_ScreenGlowGaussRadius0, g_CV_ScreenGlowGaussRadius1};
	const float gauss_amplitude[2] = {g_CV_ScreenGlowGaussAmp0, g_CV_ScreenGlowGaussAmp1};

	const float filter_center = requested_size > 1 ? (static_cast<float>(requested_size) - 1.0f) / 2.0f : 0.0f;
	const float inv_center_sqr = filter_center > 0.0f ? 1.0f / (filter_center * filter_center) : 0.0f;

	for (uint32 element_index = 0; element_index < requested_size && g_diligent_glow_state.filter_size < kDiligentGlowMaxFilterSize; ++element_index)
	{
		DiligentGlowElement element;
		element.tex_offset = static_cast<float>(element_index) - filter_center + pixel_shift;
		const float radius = element.tex_offset * element.tex_offset * inv_center_sqr;
		element.weight = 0.0f;
		for (uint32 gaussian_index = 0; gaussian_index < 2; ++gaussian_index)
		{
			element.weight += gauss_amplitude[gaussian_index] / std::exp(radius * gauss_radius[gaussian_index]);
		}

		if (element.weight > kDiligentGlowMinElementWeight)
		{
			g_diligent_glow_state.filter[g_diligent_glow_state.filter_size++] = element;
		}
	}

	while (g_diligent_glow_state.filter_size % 4 != 0 && g_diligent_glow_state.filter_size < kDiligentGlowMaxFilterSize)
	{
		g_diligent_glow_state.filter[g_diligent_glow_state.filter_size++] = {};
	}

	float weight_sum = 0.0f;
	for (uint32 index = 0; index < g_diligent_glow_state.filter_size; ++index)
	{
		weight_sum += g_diligent_glow_state.filter[index].weight;
	}
	if (weight_sum > 0.0f)
	{
		const float inv_weight_sum = 1.0f / weight_sum;
		for (uint32 index = 0; index < g_diligent_glow_state.filter_size; ++index)
		{
			g_diligent_glow_state.filter[index].weight *= inv_weight_sum;
		}
	}

	return true;
}

bool diligent_glow_ensure_targets(uint32 width, uint32 height)
{
	width = diligent_glow_truncate_to_power_of_two(width);
	height = diligent_glow_truncate_to_power_of_two(height);
	if (width == 0 || height == 0)
	{
		return false;
	}

	width = LTCLAMP(width, kDiligentGlowMinTextureSize, kDiligentGlowMaxTextureSize);
	height = LTCLAMP(height, kDiligentGlowMinTextureSize, kDiligentGlowMaxTextureSize);

	if (g_diligent_glow_state.glow_target && g_diligent_glow_state.blur_target &&
		g_diligent_glow_state.texture_width == width && g_diligent_glow_state.texture_height == height)
	{
		return true;
	}

	g_diligent_glow_state.glow_target = std::make_unique<CRenderTarget>();
	g_diligent_glow_state.blur_target = std::make_unique<CRenderTarget>();
	if (!g_diligent_glow_state.glow_target || !g_diligent_glow_state.blur_target)
	{
		return false;
	}

	if (g_diligent_glow_state.glow_target->Init(static_cast<int>(width), static_cast<int>(height), RTFMT_X8R8G8B8, STFMT_D24S8) != LT_OK)
	{
		g_diligent_glow_state.glow_target.reset();
		g_diligent_glow_state.blur_target.reset();
		return false;
	}

	if (g_diligent_glow_state.blur_target->Init(static_cast<int>(width), static_cast<int>(height), RTFMT_X8R8G8B8, STFMT_D24S8) != LT_OK)
	{
		g_diligent_glow_state.glow_target.reset();
		g_diligent_glow_state.blur_target.reset();
		return false;
	}

	g_diligent_glow_state.texture_width = width;
	g_diligent_glow_state.texture_height = height;
	return true;
}

void diligent_glow_free_targets()
{
	g_diligent_glow_state.glow_target.reset();
	g_diligent_glow_state.blur_target.reset();
	g_diligent_glow_state.texture_width = 0;
	g_diligent_glow_state.texture_height = 0;
	g_diligent_glow_state.filter_size = 0;
}

bool diligent_glow_draw_fullscreen_quad(
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target,
	float u0,
	float v0,
	float u1,
	float v1,
	float weight,
	LTSurfaceBlend blend)
{
	const float color = weight;
	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{-1.0f, 1.0f}, {color, color, color, color}, {u0, v0}};
	vertices[1] = {{1.0f, 1.0f}, {color, color, color, color}, {u1, v0}};
	vertices[2] = {{-1.0f, -1.0f}, {color, color, color, color}, {u0, v1}};
	vertices[3] = {{1.0f, -1.0f}, {color, color, color, color}, {u1, v1}};
	return diligent_draw_optimized_2d_vertices_to_target(vertices, 4, blend, texture_view, render_target, depth_target, true);
}

bool diligent_glow_draw_debug_quad(
	float left,
	float top,
	float right,
	float bottom,
	float u0,
	float v0,
	float u1,
	float v1,
	float red,
	float green,
	float blue,
	float alpha,
	Diligent::ITextureView* texture_view)
{
	if (!texture_view)
	{
		return false;
	}

	float left_top_x = 0.0f;
	float left_top_y = 0.0f;
	float right_top_x = 0.0f;
	float right_top_y = 0.0f;
	float left_bottom_x = 0.0f;
	float left_bottom_y = 0.0f;
	float right_bottom_x = 0.0f;
	float right_bottom_y = 0.0f;
	if (!diligent_to_view_clip_space(left, top, left_top_x, left_top_y) ||
		!diligent_to_view_clip_space(right, top, right_top_x, right_top_y) ||
		!diligent_to_view_clip_space(left, bottom, left_bottom_x, left_bottom_y) ||
		!diligent_to_view_clip_space(right, bottom, right_bottom_x, right_bottom_y))
	{
		return false;
	}

	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{left_top_x, left_top_y}, {red, green, blue, alpha}, {u0, v0}};
	vertices[1] = {{right_top_x, right_top_y}, {red, green, blue, alpha}, {u1, v0}};
	vertices[2] = {{left_bottom_x, left_bottom_y}, {red, green, blue, alpha}, {u0, v1}};
	vertices[3] = {{right_bottom_x, right_bottom_y}, {red, green, blue, alpha}, {u1, v1}};
	return diligent_draw_optimized_2d_vertices(vertices, 4, LTSURFACEBLEND_SOLID, texture_view);
}

bool diligent_glow_draw_debug_texture(Diligent::ITextureView* texture_view)
{
	if (!texture_view)
	{
		return false;
	}

	const float size_scale = LTCLAMP(g_CV_ScreenGlowShowTextureScale.m_Val, 0.01f, 1.0f);
	const float width = g_ViewParams.m_fScreenWidth * size_scale;
	const float height = g_ViewParams.m_fScreenHeight * size_scale;
	if (width <= 0.0f || height <= 0.0f)
	{
		return false;
	}

	return diligent_glow_draw_debug_quad(0.0f, 0.0f, width, height, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, texture_view);
}

bool diligent_glow_draw_debug_filter()
{
	if (g_diligent_glow_state.filter_size == 0)
	{
		return false;
	}

	auto* texture_view = diligent_get_debug_white_texture_view();
	if (!texture_view)
	{
		return false;
	}

	const float size_scale = LTCLAMP(g_CV_ScreenGlowShowFilterScale.m_Val, 0.01f, 1.0f);
	const float screen_width = g_ViewParams.m_fScreenWidth;
	const float screen_height = g_ViewParams.m_fScreenHeight;
	float left = 0.0f;
	float top = 0.0f;
	float right = screen_width * size_scale;
	float bottom = screen_height * size_scale;

	if (g_CV_ScreenGlowShowTexture.m_Val)
	{
		const float texture_scale = LTCLAMP(g_CV_ScreenGlowShowTextureScale.m_Val, 0.01f, 1.0f);
		const float texture_width = texture_scale * screen_width;
		left = LTMIN(left + texture_width, screen_width);
		right = LTMIN(right + texture_width, screen_width);
		if (left >= right)
		{
			return false;
		}
	}

	const float gutter = 1.0f;
	const uint32 filter_size = g_diligent_glow_state.filter_size;
	const uint32 num_gutters = filter_size > 0 ? filter_size - 1 : 0;
	if ((right - left) < (static_cast<float>(filter_size) + static_cast<float>(num_gutters) * gutter))
	{
		return false;
	}

	const float element_width = ((right - left) - static_cast<float>(num_gutters) * gutter) / static_cast<float>(filter_size);
	constexpr float background_color = 0x22 / 255.0f;
	constexpr float element_green = 0xCC / 255.0f;
	const float max_element = g_CV_ScreenGlowShowFilterRange.m_Val;

	if (!diligent_glow_draw_debug_quad(left, top, right, bottom, 0.0f, 0.0f, 1.0f, 1.0f, background_color, background_color, background_color, background_color, texture_view))
	{
		return false;
	}

	float current_x = left;
	for (uint32 index = 0; index < filter_size; ++index)
	{
		const float weight = g_diligent_glow_state.filter[index].weight;
		float height = LTCLAMP(weight / max_element, 0.0f, 1.0f);
		float element_top = top * height + bottom * (1.0f - height);
		diligent_glow_draw_debug_quad(
			current_x,
			element_top,
			current_x + element_width,
			bottom,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			0.0f,
			element_green,
			0.0f,
			1.0f,
			texture_view);
		current_x += element_width + gutter;
	}

	return true;
}

bool diligent_glow_render_blur_pass(
	CRenderTarget& dest_target,
	Diligent::ITextureView* source_view,
	float u_weight,
	float v_weight)
{
	if (!source_view)
	{
		return false;
	}

	const float uv_scale = g_CV_ScreenGlowUVScale.m_Val;
	const float width = static_cast<float>(g_diligent_glow_state.texture_width);
	const float height = static_cast<float>(g_diligent_glow_state.texture_height);

	if (!g_CV_ScreenGlowBlurMultiTap.m_Val)
	{
		bool first_pass = true;
		for (uint32 index = 0; index < g_diligent_glow_state.filter_size; ++index)
		{
			const auto& element = g_diligent_glow_state.filter[index];
			if (element.weight <= 0.0f)
			{
				continue;
			}

			const float u_offset = uv_scale * element.tex_offset * u_weight / width;
			const float v_offset = uv_scale * element.tex_offset * v_weight / height;
			const float u0 = 0.0f + u_offset;
			const float u1 = 1.0f + u_offset;
			const float v0 = 0.0f + v_offset;
			const float v1 = 1.0f + v_offset;
			if (!diligent_glow_draw_fullscreen_quad(
				source_view,
				dest_target.GetRenderTargetView(),
				dest_target.GetDepthStencilView(),
				u0,
				v0,
				u1,
				v1,
				element.weight,
				first_pass ? LTSURFACEBLEND_SOLID : LTSURFACEBLEND_ADD))
			{
				return false;
			}

			first_pass = false;
		}

		return true;
	}

	auto* pipeline = diligent_get_glow_blur_pipeline(LTSURFACEBLEND_SOLID);
	if (!pipeline)
	{
		return false;
	}

	bool first_pass = true;
	uint32 index = 0;
	while (index < g_diligent_glow_state.filter_size)
	{
		DiligentGlowBlurConstants constants{};
		uint32 tap_count = 0;
		while (index < g_diligent_glow_state.filter_size && tap_count < kDiligentGlowBlurBatchSize)
		{
			const auto& element = g_diligent_glow_state.filter[index++];
			if (element.weight <= 0.0f)
			{
				continue;
			}

			const float u_offset = uv_scale * element.tex_offset * u_weight / width;
			const float v_offset = uv_scale * element.tex_offset * v_weight / height;
			const float weight = element.weight;
			constants.taps[tap_count] = {u_offset, v_offset, weight, 0.0f};
			++tap_count;
		}

		if (tap_count == 0)
		{
			continue;
		}

		constants.params[0] = static_cast<float>(tap_count);
		if (!diligent_update_glow_blur_constants(constants))
		{
			return false;
		}

		const LTSurfaceBlend blend = first_pass ? LTSURFACEBLEND_SOLID : LTSURFACEBLEND_ADD;
		pipeline = diligent_get_glow_blur_pipeline(blend);
		if (!pipeline)
		{
			return false;
		}

		if (!diligent_draw_glow_blur_quad(
			pipeline,
			source_view,
			dest_target.GetRenderTargetView(),
			dest_target.GetDepthStencilView()))
		{
			return false;
		}

		first_pass = false;
	}

	return true;
}

bool diligent_render_screen_glow(SceneDesc* desc)
{
	if (!desc || !g_CV_ScreenGlowEnable.m_Val)
	{
		return true;
	}

	if (!g_immediate_context || !g_swap_chain)
	{
		return false;
	}

	DiligentSceneDescScope scene_desc_scope(g_diligent_scene_desc, desc);

	uint32 texture_size = static_cast<uint32>(g_CV_ScreenGlowTextureSize.m_Val);
	if (!diligent_glow_ensure_targets(texture_size, texture_size))
	{
		return false;
	}

	if (!diligent_glow_update_filter())
	{
		return false;
	}

	auto* glow_target = g_diligent_glow_state.glow_target.get();
	auto* blur_target = g_diligent_glow_state.blur_target.get();
	if (!glow_target || !blur_target)
	{
		return false;
	}

	auto* glow_srv = glow_target->GetShaderResourceView();
	auto* blur_srv = blur_target->GetShaderResourceView();
	if (!glow_srv || !blur_srv)
	{
		return false;
	}

	ViewParams saved_view = g_ViewParams;
	std::vector<const DiligentRenderBlock*> saved_blocks = g_visible_render_blocks;

	constexpr float kGlowNearZ = 7.0f;
	float glow_far_z = g_CV_ScreenGlowFogEnable.m_Val ? g_CV_ScreenGlowFogFarZ.m_Val : g_CV_FarZ.m_Val;
	if (glow_far_z > g_CV_FarZ.m_Val)
	{
		glow_far_z = g_CV_FarZ.m_Val;
	}

	ViewParams glow_params;
	if (!d3d_InitFrustum(
			&glow_params,
			desc->m_xFov,
			desc->m_yFov,
			kGlowNearZ,
			glow_far_z,
			0.0f,
			0.0f,
			static_cast<float>(g_diligent_glow_state.texture_width),
			static_cast<float>(g_diligent_glow_state.texture_height),
			&desc->m_Pos,
			&desc->m_Rotation,
			ViewParams::eRenderMode_Glow))
	{
		g_ViewParams = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}

	g_ViewParams = glow_params;
	g_visible_render_blocks.clear();
	if (g_render_world)
	{
		g_visible_render_blocks.reserve(g_render_world->render_blocks.size());
		for (const auto& block : g_render_world->render_blocks)
		{
			if (!block)
			{
				continue;
			}

			if (g_ViewParams.ViewAABBIntersect(block->bounds_min, block->bounds_max))
			{
				g_visible_render_blocks.push_back(block.get());
			}
		}
	}

	DiligentWorldConstants glow_constants;
	LTMatrix glow_world;
	glow_world.Identity();
	LTMatrix glow_mvp = g_ViewParams.m_mProjection * g_ViewParams.m_mView * glow_world;
	diligent_store_matrix_from_lt(glow_mvp, glow_constants.mvp);
	diligent_store_matrix_from_lt(g_ViewParams.m_mView, glow_constants.view);
	diligent_store_matrix_from_lt(glow_world, glow_constants.world);
	glow_constants.camera_pos[0] = g_ViewParams.m_Pos.x;
	glow_constants.camera_pos[1] = g_ViewParams.m_Pos.y;
	glow_constants.camera_pos[2] = g_ViewParams.m_Pos.z;
	glow_constants.camera_pos[3] = g_CV_ScreenGlowFogEnable.m_Val ? 1.0f : 0.0f;
	glow_constants.fog_color[0] = 0.0f;
	glow_constants.fog_color[1] = 0.0f;
	glow_constants.fog_color[2] = 0.0f;
	glow_constants.fog_color[3] = g_CV_ScreenGlowFogNearZ.m_Val;
	glow_constants.fog_params[0] = g_CV_ScreenGlowFogFarZ.m_Val;

	glow_target->InstallOnDevice();
	diligent_set_viewport(static_cast<float>(g_diligent_glow_state.texture_width), static_cast<float>(g_diligent_glow_state.texture_height));
	if (!diligent_draw_world_glow(glow_constants, g_visible_render_blocks))
	{
		g_ViewParams = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}
	if (!diligent_draw_world_models_glow(g_diligent_solid_world_models, glow_constants))
	{
		g_ViewParams = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}

	g_diligent_glow_mode = true;
	const bool models_ok = diligent_draw_models_glow(desc, g_diligent_glow_state.render_style_map);
	g_diligent_glow_mode = false;
	if (!models_ok)
	{
		g_ViewParams = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}
	if (!diligent_draw_solid_canvases_glow(desc))
	{
		g_ViewParams = saved_view;
		g_visible_render_blocks = saved_blocks;
		return false;
	}

	if (g_diligent_glow_state.filter_size > 0)
	{
		blur_target->InstallOnDevice();
		diligent_set_viewport(static_cast<float>(g_diligent_glow_state.texture_width), static_cast<float>(g_diligent_glow_state.texture_height));
		if (!diligent_glow_render_blur_pass(*blur_target, glow_srv, 1.0f, 0.0f))
		{
			g_ViewParams = saved_view;
			g_visible_render_blocks = saved_blocks;
			return false;
		}

		glow_target->InstallOnDevice();
		diligent_set_viewport(static_cast<float>(g_diligent_glow_state.texture_width), static_cast<float>(g_diligent_glow_state.texture_height));
		if (!diligent_glow_render_blur_pass(*glow_target, blur_srv, 0.0f, 1.0f))
		{
			g_ViewParams = saved_view;
			g_visible_render_blocks = saved_blocks;
			return false;
		}
	}

	const float left = static_cast<float>(desc->m_Rect.left);
	const float top = static_cast<float>(desc->m_Rect.top);
	const float width = static_cast<float>(desc->m_Rect.right - desc->m_Rect.left);
	const float height = static_cast<float>(desc->m_Rect.bottom - desc->m_Rect.top);

	auto* backbuffer_rtv = g_swap_chain->GetCurrentBackBufferRTV();
	auto* backbuffer_dsv = g_swap_chain->GetDepthBufferDSV();
	if (backbuffer_rtv)
	{
		const float u0 = 1.0f / static_cast<float>(g_diligent_glow_state.texture_width);
		const float v0 = 1.0f / static_cast<float>(g_diligent_glow_state.texture_height);
		const float u1 = 1.0f;
		const float v1 = 1.0f;
		diligent_set_viewport_rect(left, top, width, height);
		diligent_glow_draw_fullscreen_quad(glow_srv, backbuffer_rtv, backbuffer_dsv, u0, v0, u1, v1, 1.0f, LTSURFACEBLEND_ADD);
	}

	g_ViewParams = saved_view;
	g_visible_render_blocks = saved_blocks;

	if (g_CV_ScreenGlowShowTexture.m_Val || g_CV_ScreenGlowShowFilter.m_Val)
	{
		diligent_set_viewport_rect(left, top, width, height);
		if (g_CV_ScreenGlowShowTexture.m_Val)
		{
			diligent_glow_draw_debug_texture(glow_srv);
		}
		if (g_CV_ScreenGlowShowFilter.m_Val)
		{
			diligent_glow_draw_debug_filter();
		}
	}

	return true;
}

int diligent_RenderScene(SceneDesc* desc)

{
	if (!desc)
	{
		return RENDER_ERROR;
	}

	if (g_is_in_optimized_2d)
	{
		return RENDER_OK;
	}

	if (!diligent_EnsureSwapChain())
	{
		return RENDER_ERROR;
	}

	constexpr float kNearZ = 7.0f;
	constexpr float kFarZ = 10000.0f;

	const float left = static_cast<float>(desc->m_Rect.left);
	const float top = static_cast<float>(desc->m_Rect.top);
	const float right = static_cast<float>(desc->m_Rect.right);
	const float bottom = static_cast<float>(desc->m_Rect.bottom);

	if (!d3d_InitFrustum(
			&g_ViewParams,
			desc->m_xFov,
			desc->m_yFov,
			kNearZ,
			kFarZ,
			left,
			top,
			right - 0.1f,
			bottom - 0.1f,
			&desc->m_Pos,
			&desc->m_Rotation,
			ViewParams::eRenderMode_Normal))
	{
		return RENDER_ERROR;
	}

	if (g_immediate_context)
	{
		Diligent::Viewport viewport;
		viewport.TopLeftX = left;
		viewport.TopLeftY = top;
		viewport.Width = right - left;
		viewport.Height = bottom - top;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		g_immediate_context->SetViewports(1, &viewport, 0, 0);
	}

	g_visible_render_blocks.clear();
	if (g_render_world)
	{
		g_visible_render_blocks.reserve(g_render_world->render_blocks.size());
		for (const auto& block : g_render_world->render_blocks)
		{
			if (!block)
			{
				continue;
			}

			if (g_ViewParams.ViewAABBIntersect(block->bounds_min, block->bounds_max))
			{
				g_visible_render_blocks.push_back(block.get());
			}
		}
	}

	g_diligent_num_world_dynamic_lights = 0;
	if (desc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		if (desc->m_pObjectList && desc->m_ObjectListSize > 0)
		{
			for (int i = 0; i < desc->m_ObjectListSize; ++i)
			{
				LTObject* object = desc->m_pObjectList[i];
				if (!object)
				{
					continue;
				}

				diligent_process_light_object(object);
			}
		}
	}
	else if (desc->m_DrawMode == DRAWMODE_NORMAL && world_bsp_client)
	{
		WorldTree* tree = world_bsp_client->ClientTree();
		if (tree)
		{
			diligent_collect_world_dynamic_lights(g_ViewParams, tree->GetRootNode());

			auto* list_head = tree->m_AlwaysVisObjects.AsDLink();
			for (LTLink* current = list_head->m_pNext; current != list_head; current = current->m_pNext)
			{
				auto* object = static_cast<LTObject*>(current->m_pData);
				if (!object)
				{
					continue;
				}

				diligent_process_light_object(object);
			}
		}
	}

	if (!diligent_draw_sky(desc))
	{
		return RENDER_ERROR;
	}

	diligent_collect_world_models(desc);
	diligent_collect_polygrids(desc);
	diligent_collect_sprites(desc);

	if (!diligent_draw_world_blocks())
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_world_model_list(g_diligent_solid_world_models, kWorldBlendSolid))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_polygrid_list(g_diligent_solid_polygrids, g_ViewParams, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, kWorldDepthEnabled))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_models(desc))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_polygrid_list(g_diligent_early_translucent_polygrids, g_ViewParams, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, kWorldDepthEnabled))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_world_model_list(g_diligent_translucent_world_models, kWorldBlendAlpha))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_polygrid_list(g_diligent_translucent_polygrids, g_ViewParams, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, kWorldDepthEnabled))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_sprite_list(g_diligent_translucent_sprites, g_ViewParams, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, kWorldDepthEnabled))
	{
		return RENDER_ERROR;
	}

	if (!diligent_draw_sprite_list(g_diligent_noz_sprites, g_ViewParams, g_CV_FogNearZ.m_Val, g_CV_FogFarZ.m_Val, kWorldDepthDisabled))
	{
		return RENDER_ERROR;
	}

	diligent_render_screen_glow(desc);

	return RENDER_OK;
}

void diligent_RenderCommand(int, const char**)
{
}

void diligent_SwapBuffers(uint32)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	g_swap_chain->Present(1);
}

bool diligent_LockScreen(int, int, int, int, void**, long*)
{
	return false;
}

void diligent_UnlockScreen()
{
}

void diligent_BlitToScreen(BlitRequest* request)
{
	if (!request || !request->m_hBuffer || !request->m_pSrcRect || !request->m_pDestRect)
	{
		return;
	}

	if (g_screen_width == 0 || g_screen_height == 0)
	{
		return;
	}

	auto* surface = static_cast<DiligentSurface*>(request->m_hBuffer);
	if (!surface)
	{
		return;
	}

	if ((surface->flags & kDiligentSurfaceFlagOptimized) == 0)
	{
		const uint32 transparent_color = (request->m_BlitOptions & BLIT_TRANSPARENT)
			? request->m_TransparentColor.dwVal
			: OPTIMIZE_NO_TRANSPARENCY;
		diligent_OptimizeSurface(request->m_hBuffer, transparent_color);
	}

	auto& gpu_data = g_surface_cache[request->m_hBuffer];
	const bool needs_upload = !gpu_data.srv || gpu_data.width != surface->width || gpu_data.height != surface->height ||
		gpu_data.transparent_color != surface->optimized_transparent_color || (surface->flags & kDiligentSurfaceFlagDirty);
	if (needs_upload && !diligent_upload_surface_texture(*surface, gpu_data))
	{
		return;
	}

	if (!gpu_data.srv)
	{
		return;
	}

	const float red = static_cast<float>((g_optimized_2d_color >> 16) & 0xFF) / 255.0f;
	const float green = static_cast<float>((g_optimized_2d_color >> 8) & 0xFF) / 255.0f;
	const float blue = static_cast<float>(g_optimized_2d_color & 0xFF) / 255.0f;
	const float alpha = request->m_Alpha;

	const auto& src = *request->m_pSrcRect;
	const auto& dest = *request->m_pDestRect;
	const float u0 = static_cast<float>(src.left) / static_cast<float>(surface->width);
	const float v0 = static_cast<float>(src.top) / static_cast<float>(surface->height);
	const float u1 = static_cast<float>(src.right) / static_cast<float>(surface->width);
	const float v1 = static_cast<float>(src.bottom) / static_cast<float>(surface->height);

	float left = 0.0f;
	float top = 0.0f;
	float right = 0.0f;
	float bottom = 0.0f;
	float right_top = 0.0f;
	float right_bottom = 0.0f;
	diligent_to_clip_space(static_cast<float>(dest.left), static_cast<float>(dest.top), left, top);
	diligent_to_clip_space(static_cast<float>(dest.right), static_cast<float>(dest.bottom), right_bottom, bottom);
	diligent_to_clip_space(static_cast<float>(dest.right), static_cast<float>(dest.top), right_top, top);

	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{left, top}, {red, green, blue, alpha}, {u0, v0}};
	vertices[1] = {{right_top, top}, {red, green, blue, alpha}, {u1, v0}};
	vertices[2] = {{left, bottom}, {red, green, blue, alpha}, {u0, v1}};
	vertices[3] = {{right_bottom, bottom}, {red, green, blue, alpha}, {u1, v1}};

	diligent_draw_optimized_2d_vertices(vertices, 4, g_optimized_2d_blend, gpu_data.srv);
}

void diligent_BlitFromScreen(BlitRequest*)
{
}

bool diligent_WarpToScreen(BlitRequest* request)
{
	if (!request || !request->m_hBuffer || !request->m_pSrcRect || !request->m_pWarpPts)
	{
		return false;
	}

	if (request->m_nWarpPts < 4)
	{
		return false;
	}

	if (g_screen_width == 0 || g_screen_height == 0)
	{
		return false;
	}

	auto* surface = static_cast<DiligentSurface*>(request->m_hBuffer);
	if (!surface)
	{
		return false;
	}

	if ((surface->flags & kDiligentSurfaceFlagOptimized) == 0)
	{
		const uint32 transparent_color = (request->m_BlitOptions & BLIT_TRANSPARENT)
			? request->m_TransparentColor.dwVal
			: OPTIMIZE_NO_TRANSPARENCY;
		diligent_OptimizeSurface(request->m_hBuffer, transparent_color);
	}

	auto& gpu_data = g_surface_cache[request->m_hBuffer];
	const bool needs_upload = !gpu_data.srv || gpu_data.width != surface->width || gpu_data.height != surface->height ||
		gpu_data.transparent_color != surface->optimized_transparent_color || (surface->flags & kDiligentSurfaceFlagDirty);
	if (needs_upload && !diligent_upload_surface_texture(*surface, gpu_data))
	{
		return false;
	}

	if (!gpu_data.srv)
	{
		return false;
	}

	const float red = static_cast<float>((g_optimized_2d_color >> 16) & 0xFF) / 255.0f;
	const float green = static_cast<float>((g_optimized_2d_color >> 8) & 0xFF) / 255.0f;
	const float blue = static_cast<float>(g_optimized_2d_color & 0xFF) / 255.0f;
	const float alpha = request->m_Alpha;

	const auto& src = *request->m_pSrcRect;
	const float u0 = static_cast<float>(src.left) / static_cast<float>(surface->width);
	const float v0 = static_cast<float>(src.top) / static_cast<float>(surface->height);
	const float u1 = static_cast<float>(src.right) / static_cast<float>(surface->width);
	const float v1 = static_cast<float>(src.bottom) / static_cast<float>(surface->height);

	const LTWarpPt& p0 = request->m_pWarpPts[0];
	const LTWarpPt& p1 = request->m_pWarpPts[1];
	const LTWarpPt& p2 = request->m_pWarpPts[2];
	const LTWarpPt& p3 = request->m_pWarpPts[3];

	float x0 = 0.0f;
	float y0 = 0.0f;
	float x1 = 0.0f;
	float y1 = 0.0f;
	float x2 = 0.0f;
	float y2 = 0.0f;
	float x3 = 0.0f;
	float y3 = 0.0f;
	diligent_to_clip_space(p0.dest_x, p0.dest_y, x0, y0);
	diligent_to_clip_space(p1.dest_x, p1.dest_y, x1, y1);
	diligent_to_clip_space(p2.dest_x, p2.dest_y, x2, y2);
	diligent_to_clip_space(p3.dest_x, p3.dest_y, x3, y3);

	DiligentOptimized2DVertex vertices[4];
	vertices[0] = {{x0, y0}, {red, green, blue, alpha}, {u0, v0}};
	vertices[1] = {{x1, y1}, {red, green, blue, alpha}, {u1, v0}};
	vertices[2] = {{x3, y3}, {red, green, blue, alpha}, {u0, v1}};
	vertices[3] = {{x2, y2}, {red, green, blue, alpha}, {u1, v1}};

	return diligent_draw_optimized_2d_vertices(vertices, 4, g_optimized_2d_blend, gpu_data.srv);
}

void diligent_MakeScreenShot(const char*)
{
}

void diligent_MakeCubicEnvMap(const char*, uint32, const SceneDesc&)
{
}

void diligent_ReadConsoleVariables()
{
}

void diligent_GetRenderInfo(RenderInfoStruct*)
{
}

HRENDERCONTEXT diligent_CreateContext()
{
	void* context = nullptr;
	LT_MEM_TRACK_ALLOC(context = LTMemAlloc(1), LT_MEM_TYPE_RENDERER);
	return static_cast<HRENDERCONTEXT>(context);
}

void diligent_DeleteContext(HRENDERCONTEXT context)
{
	if (context)
	{
		LTMemFree(context);
	}
}

void diligent_BindTexture(SharedTexture* texture, bool texture_changed)
{
	diligent_get_texture_view(texture, texture_changed);
}

void diligent_UnbindTexture(SharedTexture* texture)
{
	diligent_release_render_texture(texture);
}

uint32 diligent_GetTextureFormat1(BPPIdent, uint32)
{
	return 0;
}

bool diligent_QueryDDSupport(PFormat*)
{
	return false;
}

bool diligent_GetTextureFormat2(BPPIdent, uint32, PFormat*)
{
	return false;
}

bool diligent_ConvertTexDataToDD(uint8*, PFormat*, uint32, uint32, uint8*, PFormat*, BPPIdent, uint32, uint32, uint32)
{
	return false;
}

void diligent_DrawPrimSetTexture(SharedTexture* texture)
{
	g_drawprim_texture = texture;
	diligent_get_texture_view(texture, false);
}

void diligent_DrawPrimDisableTextures()
{
}

CRenderObject* diligent_CreateRenderObject(CRenderObject::RENDER_OBJECT_TYPES object_type)
{
	CRenderObject* new_object = nullptr;
	switch (object_type)
	{
		case CRenderObject::eDebugLine:
			LT_MEM_TRACK_ALLOC(new_object = new CDIDebugLine(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eDebugPolygon:
			LT_MEM_TRACK_ALLOC(new_object = new CDIDebugPolygon(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eDebugText:
			LT_MEM_TRACK_ALLOC(new_object = new CDIDebugText(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eRigidMesh:
			LT_MEM_TRACK_ALLOC(new_object = new DiligentRigidMesh(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eSkelMesh:
			LT_MEM_TRACK_ALLOC(new_object = new DiligentSkelMesh(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eVAMesh:
			LT_MEM_TRACK_ALLOC(new_object = new DiligentVAMesh(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eNullMesh:
			LT_MEM_TRACK_ALLOC(new_object = new CDIModelDrawable(), LT_MEM_TYPE_RENDERER);
			break;
		default:
			return nullptr;
	}

	if (new_object)
	{
		new_object->SetNext(g_render_object_list_head);
		g_render_object_list_head = new_object;
	}
	return new_object;
}

bool diligent_DestroyRenderObject(CRenderObject* object)
{
	if (!object)
	{
		return false;
	}

	if (object == g_render_object_list_head)
	{
		g_render_object_list_head = object->GetNext();
	}
	else
	{
		if (!g_render_object_list_head)
		{
			return false;
		}
		CRenderObject* previous = g_render_object_list_head;
		while (previous->GetNext() && previous->GetNext() != object)
		{
			previous = previous->GetNext();
		}
		if (previous->GetNext())
		{
			previous->SetNext(previous->GetNext()->GetNext());
		}
		else
		{
			return false;
		}
	}

	delete object;
	return true;
}

bool diligent_LoadWorldData(ILTStream* stream)
{
	if (!stream)
	{
		return false;
	}

	g_render_world.reset();

	auto world = std::unique_ptr<DiligentRenderWorld>(new DiligentRenderWorld());
	if (!world->Load(stream))
	{
		return false;
	}

	g_render_world = std::move(world);
	return true;
}

bool diligent_SetLightGroupColor(uint32 id, const LTVector& color)
{
	if (!g_render_world)
	{
		return false;
	}

	g_render_world->SetLightGroupColor(id, color);
	return true;
}

LTRESULT diligent_SetOccluderEnabled(uint32, bool)
{
	return LT_NOTFOUND;
}

LTRESULT diligent_GetOccluderEnabled(uint32, bool*)
{
	return LT_NOTFOUND;
}

uint32 diligent_GetTextureEffectVarID(const char*, uint32)
{
	return 0;
}

bool diligent_SetTextureEffectVar(uint32, uint32, float)
{
	return false;
}

bool diligent_IsObjectGroupEnabled(uint32)
{
	return false;
}

void diligent_SetObjectGroupEnabled(uint32, bool)
{
}

void diligent_SetAllObjectGroupEnabled()
{
}

bool diligent_AddGlowRenderStyleMapping(const char* source, const char* map_to)
{
	if (!source || !map_to)
	{
		return false;
	}

	return g_diligent_glow_state.render_style_map.AddRenderStyle(source, map_to);
}

bool diligent_SetGlowDefaultRenderStyle(const char* filename)
{
	if (!filename)
	{
		return false;
	}

	return g_diligent_glow_state.render_style_map.SetDefaultRenderStyle(filename);
}

bool diligent_SetNoGlowRenderStyle(const char* filename)
{
	if (!filename)
	{
		return false;
	}

	return g_diligent_glow_state.render_style_map.SetNoGlowRenderStyle(filename);
}

int diligent_Init(RenderStructInit* init)
{
	if (!init)
	{
		return RENDER_ERROR;
	}

	init->m_RendererVersion = LTRENDER_VERSION;
	g_screen_width = init->m_Mode.m_Width;
	g_screen_height = init->m_Mode.m_Height;

	diligent_UpdateWindowHandles(init);
	if (!g_native_window_handle)
	{
		return RENDER_ERROR;
	}

	if (!diligent_EnsureDevice())
	{
		return RENDER_ERROR;
	}

	diligent_UpdateScreenSize(g_screen_width, g_screen_height);
	if (!diligent_EnsureSwapChain())
	{
		return RENDER_ERROR;
	}

	return RENDER_OK;
}

void diligent_Term(bool)
{
	g_swap_chain.Release();
	g_immediate_context.Release();
	g_render_device.Release();
	g_pso_cache.Reset();
	g_srb_cache.Reset();
	g_surface_cache.clear();
	g_render_world.reset();
	g_visible_render_blocks.clear();
	diligent_glow_free_targets();
	g_diligent_glow_state.render_style_map.FreeList();
	while (g_render_object_list_head)
	{
		CRenderObject* next = g_render_object_list_head->GetNext();
		delete g_render_object_list_head;
		g_render_object_list_head = next;
	}
	g_world_pipelines.pipelines.clear();
	g_model_pipelines.clear();
	g_world_resources.vertex_shader.Release();
	g_world_resources.pixel_shader_textured.Release();
	g_world_resources.pixel_shader_glow.Release();
	g_world_resources.pixel_shader_lightmap.Release();
	g_world_resources.pixel_shader_lightmap_only.Release();
	g_world_resources.pixel_shader_solid.Release();
	g_world_resources.pixel_shader_dynamic_light.Release();
	g_world_resources.pixel_shader_bump.Release();
	g_world_resources.constant_buffer.Release();
	g_world_resources.vertex_buffer.Release();
	g_world_resources.index_buffer.Release();
	g_world_resources.vertex_buffer_size = 0;
	g_world_resources.index_buffer_size = 0;
	g_model_resources.vertex_shaders.clear();
	g_model_resources.pixel_shader_textured.Release();
	g_model_resources.pixel_shader_solid.Release();
	g_model_resources.constant_buffer.Release();
	g_debug_white_texture_srv.Release();
	g_debug_white_texture.Release();
	g_optimized_2d_resources.pipelines.clear();
	g_optimized_2d_resources.clamped_pipelines.clear();
	g_optimized_2d_resources.vertex_shader.Release();
	g_optimized_2d_resources.pixel_shader.Release();
	g_optimized_2d_resources.vertex_buffer.Release();
	g_optimized_2d_resources.vertex_buffer_size = 0;
	g_diligent_glow_blur_resources.vertex_shader.Release();
	g_diligent_glow_blur_resources.pixel_shader.Release();
	g_diligent_glow_blur_resources.constant_buffer.Release();
	g_diligent_glow_blur_resources.pipeline_solid.srb.Release();
	g_diligent_glow_blur_resources.pipeline_solid.pipeline_state.Release();
	g_diligent_glow_blur_resources.pipeline_add.srb.Release();
	g_diligent_glow_blur_resources.pipeline_add.pipeline_state.Release();
	g_engine_factory = nullptr;
	g_drawprim_texture = nullptr;
	g_native_window_handle = nullptr;
#ifdef LTJS_SDL_BACKEND
	g_sdl_window = nullptr;
#endif
	g_screen_width = 0;
	g_screen_height = 0;
	g_is_in_3d = false;
	g_is_in_optimized_2d = false;
}

#if !defined(LTJS_USE_DILIGENT_RENDER)
void* diligent_GetD3DDevice()
{
	return nullptr;
}
#endif

Diligent::IRenderDevice* diligent_GetRenderDevice()
{
	return g_render_device.RawPtr();
}

Diligent::IDeviceContext* diligent_GetImmediateContext()
{
	return g_immediate_context.RawPtr();
}

Diligent::ISwapChain* diligent_GetSwapChain()
{
	return g_swap_chain.RawPtr();
}

RMode* rdll_GetSupportedModes()
{
	RMode* mode = nullptr;
	LT_MEM_TRACK_ALLOC(mode = new RMode, LT_MEM_TYPE_RENDERER);
	if (!mode)
	{
		return nullptr;
	}

	LTStrCpy(mode->m_Description, "Diligent (Vulkan)", sizeof(mode->m_Description));
	LTStrCpy(mode->m_InternalName, "DiligentVulkan", sizeof(mode->m_InternalName));
	mode->m_Width = 1280;
	mode->m_Height = 720;
	mode->m_BitDepth = 32;
	mode->m_bHWTnL = true;
	mode->m_pNext = LTNULL;
	return mode;
}

void rdll_FreeModeList(RMode* modes)
{
	auto* current = modes;
	while (current)
	{
		auto* next = current->m_pNext;
		LTMemFree(current);
		current = next;
	}
}
}

Diligent::ITextureView* diligent_get_drawprim_texture_view(SharedTexture* shared_texture, bool texture_changed)
{
	return diligent_get_texture_view(shared_texture, texture_changed);
}

void rdll_RenderDLLSetup(RenderStruct* render_struct)
{
	g_render_struct = render_struct;

	render_struct->Init = diligent_Init;
	render_struct->Term = diligent_Term;
#if !defined(LTJS_USE_DILIGENT_RENDER)
	render_struct->GetD3DDevice = diligent_GetD3DDevice;
#endif
	render_struct->GetRenderDevice = diligent_GetRenderDevice;
	render_struct->GetImmediateContext = diligent_GetImmediateContext;
	render_struct->GetSwapChain = diligent_GetSwapChain;
	render_struct->BindTexture = diligent_BindTexture;
	render_struct->UnbindTexture = diligent_UnbindTexture;
	render_struct->GetTextureDDFormat1 = diligent_GetTextureFormat1;
	render_struct->QueryDDSupport = diligent_QueryDDSupport;
	render_struct->GetTextureDDFormat2 = diligent_GetTextureFormat2;
	render_struct->ConvertTexDataToDD = diligent_ConvertTexDataToDD;
	render_struct->DrawPrimSetTexture = diligent_DrawPrimSetTexture;
	render_struct->DrawPrimDisableTextures = diligent_DrawPrimDisableTextures;
	render_struct->CreateContext = diligent_CreateContext;
	render_struct->DeleteContext = diligent_DeleteContext;
	render_struct->Clear = diligent_Clear;
	render_struct->Start3D = diligent_Start3D;
	render_struct->End3D = diligent_End3D;
	render_struct->IsIn3D = diligent_IsIn3D;
	render_struct->StartOptimized2D = diligent_StartOptimized2D;
	render_struct->EndOptimized2D = diligent_EndOptimized2D;
	render_struct->IsInOptimized2D = diligent_IsInOptimized2D;
	render_struct->SetOptimized2DBlend = diligent_SetOptimized2DBlend;
	render_struct->GetOptimized2DBlend = diligent_GetOptimized2DBlend;
	render_struct->SetOptimized2DColor = diligent_SetOptimized2DColor;
	render_struct->GetOptimized2DColor = diligent_GetOptimized2DColor;
	render_struct->RenderScene = diligent_RenderScene;
	render_struct->RenderCommand = diligent_RenderCommand;
	render_struct->SwapBuffers = diligent_SwapBuffers;
	render_struct->GetScreenFormat = diligent_GetScreenFormat;
	render_struct->CreateSurface = diligent_CreateSurface;
	render_struct->DeleteSurface = diligent_DeleteSurface;
	render_struct->GetSurfaceInfo = diligent_GetSurfaceInfo;
	render_struct->LockSurface = diligent_LockSurface;
	render_struct->UnlockSurface = diligent_UnlockSurface;
	render_struct->OptimizeSurface = diligent_OptimizeSurface;
	render_struct->UnoptimizeSurface = diligent_UnoptimizeSurface;
	render_struct->LockScreen = diligent_LockScreen;
	render_struct->UnlockScreen = diligent_UnlockScreen;
	render_struct->BlitToScreen = diligent_BlitToScreen;
	render_struct->BlitFromScreen = diligent_BlitFromScreen;
	render_struct->WarpToScreen = diligent_WarpToScreen;
	render_struct->MakeScreenShot = diligent_MakeScreenShot;
	render_struct->MakeCubicEnvMap = diligent_MakeCubicEnvMap;
	render_struct->ReadConsoleVariables = diligent_ReadConsoleVariables;
	render_struct->GetRenderInfo = diligent_GetRenderInfo;
	render_struct->CreateRenderObject = diligent_CreateRenderObject;
	render_struct->DestroyRenderObject = diligent_DestroyRenderObject;
	render_struct->LoadWorldData = diligent_LoadWorldData;
	render_struct->SetLightGroupColor = diligent_SetLightGroupColor;
	render_struct->SetOccluderEnabled = diligent_SetOccluderEnabled;
	render_struct->GetOccluderEnabled = diligent_GetOccluderEnabled;
	render_struct->GetTextureEffectVarID = diligent_GetTextureEffectVarID;
	render_struct->SetTextureEffectVar = diligent_SetTextureEffectVar;
	render_struct->IsObjectGroupEnabled = diligent_IsObjectGroupEnabled;
	render_struct->SetObjectGroupEnabled = diligent_SetObjectGroupEnabled;
	render_struct->SetAllObjectGroupEnabled = diligent_SetAllObjectGroupEnabled;
	render_struct->AddGlowRenderStyleMapping = diligent_AddGlowRenderStyleMapping;
	render_struct->SetGlowDefaultRenderStyle = diligent_SetGlowDefaultRenderStyle;
	render_struct->SetNoGlowRenderStyle = diligent_SetNoGlowRenderStyle;
}
