#include "diligent_mesh_layout.h"
#include "diligent_utils.h"

static uint32 diligent_value_type_size(Diligent::VALUE_TYPE value_type)
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

static uint32 diligent_append_layout_element(
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

static uint32 diligent_get_uv_set_count(uint32 vert_flags)
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

static bool diligent_build_stream_layout(
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

static std::string diligent_hlsl_type_for_element(const Diligent::LayoutElement& element)
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

static std::string diligent_attribute_ref(int32 attrib, const char* swizzle)
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
	source += "    float4 g_ModelParams;\n";
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
	source += "    float3 lit_color = vertex_color.rgb;\n";
	source += "    if (g_ModelParams.x < 0.5f)\n";
	source += "    {\n";
	source += "        lit_color = float3(1.0f, 1.0f, 1.0f);\n";
	source += "    }\n";
	source += "    else\n";
	source += "    {\n";
	source += "        float light_scale = g_ModelParams.y + g_ModelParams.z;\n";
	source += "        if (light_scale <= 0.0f)\n";
	source += "        {\n";
	source += "            lit_color = float3(1.0f, 1.0f, 1.0f);\n";
	source += "        }\n";
	source += "        else\n";
	source += "        {\n";
	source += "            lit_color *= (light_scale * 0.5f);\n";
	source += "        }\n";
	source += "    }\n";
	source += "    output.color = float4(lit_color, vertex_color.a) * g_Color;\n";
	source += "    output.uv = uv;\n";
	source += "    output.world_pos = world_pos.xyz;\n";
	source += "    return output;\n";
	source += "}\n";
	return source;
}
