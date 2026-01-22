#ifndef LTJS_DILIGENT_WORLD_DATA_H
#define LTJS_DILIGENT_WORLD_DATA_H

#include "ltbasedefs.h"
#include "ltplane.h"
#include "ltvector.h"
#include "viewparams.h"
#include "../aabb.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

class SharedTexture;
class CTextureScriptInstance;
class ILTStream;

struct DiligentRBSection
{
	enum { kNumTextures = 2 };

	DiligentRBSection();
	~DiligentRBSection();
	DiligentRBSection(const DiligentRBSection&) = delete;
	DiligentRBSection& operator=(const DiligentRBSection&) = delete;

	SharedTexture* textures[kNumTextures];
	struct DiligentRBSpriteData* sprite_data[kNumTextures];
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

#ifdef LTJS_USE_TANGENT_AND_BINORMAL
struct DiligentRBVertexLegacy
{
	LTVector position;
	float u0;
	float v0;
	float u1;
	float v1;
	uint32 color;
	LTVector normal;
};
#endif

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
	bool use_base_vertex = true;
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
	float normal[3];
};

struct DiligentDebugLineVertex
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
	std::array<std::array<float, 16>, 4> tex_effect_matrices{};
	std::array<std::array<int32, 4>, 4> tex_effect_params{};
	std::array<std::array<int32, 4>, 4> tex_effect_uv{};
};

extern std::unique_ptr<DiligentRenderWorld> g_render_world;

Diligent::ITextureView* diligent_get_lightmap_view(DiligentRBSection& section);

#endif
