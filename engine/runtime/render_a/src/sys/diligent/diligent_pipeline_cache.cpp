#include "diligent_pipeline_cache.h"
#include "diligent_state.h"

#include "diligent_utils.h"

#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"

static uint64 diligent_hash_texture_stage(const TextureStageOps& stage)
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

static uint64 diligent_hash_render_pass(const RenderPassOp& pass)
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

size_t DiligentPsoKeyHash::operator()(const DiligentPsoKey& key) const noexcept
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

size_t DiligentModelPipelineKeyHash::operator()(const DiligentModelPipelineKey& key) const noexcept
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

Diligent::RefCntAutoPtr<Diligent::IPipelineState> DiligentPsoCache::GetOrCreate(
	const DiligentPsoKey& key,
	const Diligent::GraphicsPipelineStateCreateInfo& create_info)
{
	if (!g_diligent_state.render_device)
	{
		return {};
	}

	auto it = cache_.find(key);
	if (it != cache_.end())
	{
		return it->second;
	}

	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	g_diligent_state.render_device->CreatePipelineState(create_info, &pipeline_state);
	if (pipeline_state)
	{
		cache_.emplace(key, pipeline_state);
	}

	return pipeline_state;
}

void DiligentPsoCache::Reset()
{
	cache_.clear();
}

size_t DiligentSrbKeyHash::operator()(const DiligentSrbKey& key) const noexcept
{
	uint64 hash = 0;
	hash = diligent_hash_combine(hash, static_cast<uint64>(reinterpret_cast<uintptr_t>(key.pipeline_state)));
	for (const auto* texture : key.textures)
	{
		hash = diligent_hash_combine(hash, static_cast<uint64>(reinterpret_cast<uintptr_t>(texture)));
	}
	return static_cast<size_t>(hash);
}

Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> DiligentSrbCache::GetOrCreate(
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

void DiligentSrbCache::Reset()
{
	cache_.clear();
}

DiligentPsoCache g_pso_cache;
DiligentSrbCache g_srb_cache;
