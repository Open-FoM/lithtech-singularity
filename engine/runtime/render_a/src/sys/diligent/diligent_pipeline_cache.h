#ifndef LTJS_DILIGENT_PIPELINE_CACHE_H
#define LTJS_DILIGENT_PIPELINE_CACHE_H

#include "ltbasedefs.h"
#include "ltrenderstyle.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"

#include <array>
#include <unordered_map>

class SharedTexture;

using DiligentTextureArray = std::array<SharedTexture*, 4>;

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

struct DiligentPsoKeyHash
{
	size_t operator()(const DiligentPsoKey& key) const noexcept;
};

struct DiligentModelPipelineKeyHash
{
	size_t operator()(const DiligentModelPipelineKey& key) const noexcept;
};

class DiligentPsoCache
{
public:
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> GetOrCreate(
		const DiligentPsoKey& key,
		const Diligent::GraphicsPipelineStateCreateInfo& create_info);

	void Reset();

private:
	std::unordered_map<DiligentPsoKey, Diligent::RefCntAutoPtr<Diligent::IPipelineState>, DiligentPsoKeyHash> cache_;
};

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
	size_t operator()(const DiligentSrbKey& key) const noexcept;
};

class DiligentSrbCache
{
public:
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> GetOrCreate(
		Diligent::IPipelineState* pipeline_state,
		const DiligentTextureArray& textures);

	void Reset();

private:
	std::unordered_map<DiligentSrbKey, Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>, DiligentSrbKeyHash> cache_;
};

extern DiligentPsoCache g_pso_cache;
extern DiligentSrbCache g_srb_cache;

DiligentPsoKey diligent_make_pso_key(
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	uint32 input_layout_hash,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format,
	Diligent::PRIMITIVE_TOPOLOGY topology);

#endif
