/**
 * diligent_pipeline_cache.h
 *
 * This header defines the Pipeline Cache portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
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

/// Fixed-size array of up to four textures for pipeline caching.
using DiligentTextureArray = std::array<SharedTexture*, 4>;

/// Key describing a graphics pipeline state configuration.
struct DiligentPsoKey
{
	uint64 render_pass_hash = 0;
	uint32 input_layout_hash = 0;
	int vertex_shader_id = 0;
	int pixel_shader_id = 0;
	uint32 color_format = 0;
	uint32 depth_format = 0;
	uint32 topology = 0;
	uint8 sample_count = 1;

	bool operator==(const DiligentPsoKey& other) const
	{
		return render_pass_hash == other.render_pass_hash &&
			input_layout_hash == other.input_layout_hash &&
			vertex_shader_id == other.vertex_shader_id &&
			pixel_shader_id == other.pixel_shader_id &&
			color_format == other.color_format &&
			depth_format == other.depth_format &&
			topology == other.topology &&
			sample_count == other.sample_count;
	}
};

/// Pipeline key extended with model-specific texture usage.
struct DiligentModelPipelineKey
{
	DiligentPsoKey pso_key{};
	bool uses_texture = false;

	bool operator==(const DiligentModelPipelineKey& other) const
	{
		return uses_texture == other.uses_texture && pso_key == other.pso_key;
	}
};

/// Hash functor for DiligentPsoKey.
struct DiligentPsoKeyHash
{
	size_t operator()(const DiligentPsoKey& key) const noexcept;
};

/// Hash functor for DiligentModelPipelineKey.
struct DiligentModelPipelineKeyHash
{
	size_t operator()(const DiligentModelPipelineKey& key) const noexcept;
};

/// \brief Cache for graphics pipeline states keyed by DiligentPsoKey.
/// \details Used to avoid redundant PSO creation when render state is repeated.
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

/// Key describing shader resource bindings for a pipeline state.
struct DiligentSrbKey
{
	const Diligent::IPipelineState* pipeline_state = nullptr;
	DiligentTextureArray textures{};

	bool operator==(const DiligentSrbKey& other) const
	{
		return pipeline_state == other.pipeline_state && textures == other.textures;
	}
};

/// Hash functor for DiligentSrbKey.
struct DiligentSrbKeyHash
{
	size_t operator()(const DiligentSrbKey& key) const noexcept;
};

/// \brief Cache for shader resource bindings keyed by pipeline and textures.
/// \details Helps reduce SRB churn when rendering with repeated texture sets.
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

/// Global pipeline state cache.
extern DiligentPsoCache g_pso_cache;
/// Global shader resource binding cache.
extern DiligentSrbCache g_srb_cache;

/// \brief Builds a pipeline key from render pass/shader metadata and formats.
/// \code
/// auto key = diligent_make_pso_key(pass, shader_pass, layout_hash, color_fmt, depth_fmt, Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
/// \endcode
DiligentPsoKey diligent_make_pso_key(
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	uint32 input_layout_hash,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format,
	Diligent::PRIMITIVE_TOPOLOGY topology,
	uint8 sample_count = 1);

#endif
