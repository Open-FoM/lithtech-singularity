/**
 * diligent_mesh_layout.h
 *
 * This header defines the Mesh Layout portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_MESH_LAYOUT_H
#define LTJS_DILIGENT_MESH_LAYOUT_H

#include "ltbasedefs.h"
#include "renderobject.h"

#include "Graphics/GraphicsEngine/interface/InputLayout.h"

#include <array>
#include <string>
#include <vector>

/// Describes a mesh input layout and attribute indices.
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

/// Resolved stream/offset info for a specific vertex attribute.
struct DiligentVertexElementRef
{
	uint32 stream_index = 0;
	uint32 offset = 0;
	uint32 stride = 0;
	bool valid = false;
};

/// Returns the number of blend weights for a given blend type.
uint32 diligent_get_blend_weight_count(VERTEX_BLEND_TYPE blend_type);
/// Builds a Diligent input layout from engine vertex flags.
bool diligent_build_mesh_layout(
	const uint32* vert_flags,
	VERTEX_BLEND_TYPE blend_type,
	DiligentMeshLayout& layout,
	bool& non_fixed_pipe_data);
/// Resolves stream/offset/stride for a layout attribute index.
bool diligent_get_layout_element_ref(const DiligentMeshLayout& layout, int32 attrib, DiligentVertexElementRef& ref);
/// Builds a HLSL vertex shader string matching the input layout.
std::string diligent_build_model_vertex_shader_source(const DiligentMeshLayout& layout);

#endif
