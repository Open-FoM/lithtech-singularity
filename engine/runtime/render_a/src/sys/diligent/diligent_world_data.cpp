#include "diligent_world_data.h"
#include "diligent_state.h"

#include "diligent_buffers.h"

#include "bdefs.h"
#include "de_sprite.h"
#include "de_world.h"
#include "lightmap_compress.h"
#include "ltmatrix.h"
#include "renderstruct.h"
#include "dtxmgr.h"
#include "setupobject.h"
#include "sprite.h"
#include "strtools.h"
#include "texturescriptinstance.h"
#include "texturescriptmgr.h"
#include "viewparams.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

#include "../rendererconsolevars.h"

#include <cstring>
#include <limits>

#ifndef CPLANE_NEAR_INDEX
#define CPLANE_NEAR_INDEX 0
#endif

struct DiligentRBSpriteData
{
	SpriteTracker tracker{};
	Sprite* sprite = nullptr;
};

namespace
{

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

	return stricmp(&filename[name_length - 4], ".spr") == 0;
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
	if (!g_diligent_state.render_device)
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

	if (section.lightmap_texture && g_diligent_state.immediate_context)
	{
		Diligent::Box dst_box{0, section.lightmap_width, 0, section.lightmap_height};
		Diligent::TextureSubResData subresource;
		subresource.pData = rgba.data();
		subresource.Stride = stride;
		subresource.DepthStride = 0;

		g_diligent_state.immediate_context->UpdateTexture(
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
	g_diligent_state.render_device->CreateTexture(desc, &init_data, &section.lightmap_texture);
	if (!section.lightmap_texture)
	{
		return false;
	}

	section.lightmap_srv = section.lightmap_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	return section.lightmap_srv != nullptr;
}

bool diligent_GetStreamRemaining(ILTStream* stream, uint32& out_remaining)
{
	if (!stream)
	{
		return false;
	}

	uint32 pos = 0;
	uint32 len = 0;
	if (stream->GetPos(&pos) != LT_OK || stream->GetLen(&len) != LT_OK)
	{
		return false;
	}

	if (len < pos)
	{
		return false;
	}

	out_remaining = len - pos;
	return true;
}
} // namespace

std::unique_ptr<DiligentRenderWorld> g_render_world;

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

	bool any_light = false;
	for (const auto value : decompressed)
	{
		if (value != 0)
		{
			any_light = true;
			break;
		}
	}
	if (!any_light)
	{
		section.lightmap_data.clear();
		section.lightmap_size = 0;
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
			else if (texture_names[tex_index][0] && g_diligent_state.render_struct)
			{
				texture = g_diligent_state.render_struct->GetSharedTexture(texture_names[tex_index]);
			}
			section.textures[tex_index] = texture;
			if (texture)
			{
				texture->SetRefCount(texture->GetRefCount() + 1);
			}
		}

		section.fullbright = false;
		if (section.textures[0] && g_diligent_state.render_struct)
		{
			TextureData* texture_data = g_diligent_state.render_struct->GetTexture(section.textures[0]);
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

		// Filter out LightAnim placeholder sections - don't add to pipeline
		if (texture_names[0][0] && strstr(texture_names[0], "LightAnim") != nullptr)
		{
			// Clean up texture ref counts
			for (uint32 tex_index = 0; tex_index < DiligentRBSection::kNumTextures; ++tex_index)
			{
				if (section.textures[tex_index])
				{
					section.textures[tex_index]->SetRefCount(section.textures[tex_index]->GetRefCount() - 1);
				}
			}
			sections.pop_back();
		}
	}

	// Update section_count after filtering out LightAnim sections
	section_count = static_cast<uint32>(sections.size());

	uint32 vertex_count = 0;
	*stream >> vertex_count;
	uint32 remaining = 0;
	if (diligent_GetStreamRemaining(stream, remaining))
	{
		const uint64 min_vertex_size =
#ifdef LTJS_USE_TANGENT_AND_BINORMAL
			sizeof(DiligentRBVertexLegacy);
#else
			sizeof(DiligentRBVertex);
#endif
		const uint64 max_vertices = remaining / min_vertex_size;
		if (vertex_count > max_vertices)
		{
			dsi_ConsolePrint("Diligent: invalid vertex count %u (remaining=%u).", vertex_count, remaining);
			return false;
		}
	}
	uint32 vertex_pos = 0;
	stream->GetPos(&vertex_pos);
	vertices.clear();
	vertices.resize(vertex_count);
	if (!vertices.empty())
	{
		stream->Read(vertices.data(), sizeof(DiligentRBVertex) * vertices.size());
	}

	uint32 tri_count = 0;
	*stream >> tri_count;
	auto validate_tri_count = [&](uint32 value) -> bool
	{
		if (value > (std::numeric_limits<uint32>::max() / 3))
		{
			return false;
		}
		if (diligent_GetStreamRemaining(stream, remaining))
		{
			const uint64 bytes_per_tri = sizeof(uint32) * 4;
			const uint64 max_tris = bytes_per_tri ? (remaining / bytes_per_tri) : 0;
			return value <= max_tris;
		}
		return true;
	};

#ifdef LTJS_USE_TANGENT_AND_BINORMAL
	if (!validate_tri_count(tri_count))
	{
		stream->SeekTo(vertex_pos);
		std::vector<DiligentRBVertexLegacy> legacy_vertices;
		legacy_vertices.resize(vertex_count);
		if (!legacy_vertices.empty())
		{
			stream->Read(legacy_vertices.data(), sizeof(DiligentRBVertexLegacy) * legacy_vertices.size());
		}
		*stream >> tri_count;
		if (!validate_tri_count(tri_count))
		{
			dsi_ConsolePrint("Diligent: invalid triangle count %u.", tri_count);
			return false;
		}

		for (size_t i = 0; i < vertices.size(); ++i)
		{
			const auto& src = legacy_vertices[i];
			auto& dst = vertices[i];
			dst.position = src.position;
			dst.u0 = src.u0;
			dst.v0 = src.v0;
			dst.u1 = src.u1;
			dst.v1 = src.v1;
			dst.color = src.color;
			dst.normal = src.normal;
			dst.tangent.Init(0.0f, 0.0f, 0.0f);
			dst.binormal.Init(0.0f, 0.0f, 0.0f);
		}
	}
	else
	{
		// tri_count validated for tangent/binormal layout.
	}
#else
	if (!validate_tri_count(tri_count))
	{
		dsi_ConsolePrint("Diligent: invalid triangle count %u.", tri_count);
		return false;
	}
#endif
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
	if (!sections.empty())
	{
		bool base_vertex = true;
		for (const auto& section_ptr : sections)
		{
			if (section_ptr && section_ptr->start_vertex != 0)
			{
				base_vertex = false;
				break;
			}
		}
		use_base_vertex = base_vertex;
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
		if (g_CV_Fullbright)
		{
			dst.color[0] = 1.0f;
			dst.color[1] = 1.0f;
			dst.color[2] = 1.0f;
			dst.color[3] = 1.0f;
		}
		else
		{
			dst.color[0] = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
			dst.color[1] = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
			dst.color[2] = static_cast<float>(color & 0xFF) / 255.0f;
			dst.color[3] = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
		}
		dst.uv0[0] = src.u0;
		dst.uv0[1] = src.v0;
		dst.uv1[0] = src.u1;
		dst.uv1[1] = src.v1;
		dst.normal[0] = src.normal.x;
		dst.normal[1] = src.normal.y;
		dst.normal[2] = src.normal.z;
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

	if (!g_diligent_state.render_device)
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
