#ifndef LTJS_DILIGENT_WORLD_DRAW_H
#define LTJS_DILIGENT_WORLD_DRAW_H

#include "diligent_world_data.h"
#include "diligent_utils.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/Shader.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"

#include <array>
#include <unordered_map>
#include <vector>

class WorldModelInstance;
class ViewParams;
struct DynamicLight;
struct LTMatrix;

namespace Diligent
{
	class ITextureView;
}

constexpr uint32 kDiligentMaxDynamicLights = 64;

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
	kWorldPipelineBump = 8,
	kWorldPipelineVolumeEffect = 9,
	kWorldPipelineShadowProject = 10,
	kWorldPipelineFlat = 11,
	kWorldPipelineNormals = 12
};

enum DiligentWorldShadingMode : int
{
	kWorldShadingTextured = 0,
	kWorldShadingFlat = 1,
	kWorldShadingNormals = 2
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

struct DiligentShadowProjectConstants
{
	std::array<float, 16> world_to_shadow{};
	float light_dir[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float proj_center[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DiligentWorldPipelineKey
{
	uint8 mode = kWorldPipelineSolid;
	uint8 blend_mode = kWorldBlendSolid;
	uint8 depth_mode = kWorldDepthEnabled;
	uint8 wireframe = 0;

	bool operator==(const DiligentWorldPipelineKey& other) const
	{
		return mode == other.mode &&
			blend_mode == other.blend_mode &&
			depth_mode == other.depth_mode &&
			wireframe == other.wireframe;
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
		hash = diligent_hash_combine(hash, key.wireframe);
		return static_cast<size_t>(hash);
	}
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
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_flat;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_normals;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_dynamic_light;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_bump;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_volume_effect;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_shadow_project;
	Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader_debug;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> constant_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> shadow_project_constants;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertex_buffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> index_buffer;
	uint32 vertex_buffer_size = 0;
	uint32 index_buffer_size = 0;
};

struct DiligentWorldPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

struct DiligentWorldPipelines
{
	std::unordered_map<DiligentWorldPipelineKey, DiligentWorldPipeline, DiligentWorldPipelineKeyHash> pipelines;
};

extern DiligentWorldResources g_world_resources;
extern DiligentWorldPipelines g_world_pipelines;
extern std::vector<DiligentRenderBlock*> g_visible_render_blocks;
extern std::vector<WorldModelInstance*> g_diligent_solid_world_models;
extern std::vector<WorldModelInstance*> g_diligent_translucent_world_models;
extern DynamicLight* g_diligent_world_dynamic_lights[kDiligentMaxDynamicLights];
extern uint32 g_diligent_num_world_dynamic_lights;
extern bool g_diligent_shadow_mode;

int diligent_get_world_shading_mode();
void diligent_fill_world_blend_desc(DiligentWorldBlendMode blend, Diligent::RenderTargetBlendDesc& blend_desc);
DiligentWorldPipeline* diligent_get_world_pipeline(
	uint8 mode,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	bool wireframe);

bool diligent_ensure_world_constant_buffer();
bool diligent_ensure_shadow_project_constants();
bool diligent_ensure_world_shaders();
bool diligent_ensure_world_vertex_buffer(uint32 size);
bool diligent_ensure_world_index_buffer(uint32 size);

void diligent_set_world_vertex_color(DiligentWorldVertex& vertex, uint32 color);
void diligent_set_world_vertex_normal(DiligentWorldVertex& vertex, const LTVector& normal);
void diligent_fill_world_constants(
	const ViewParams& view_params,
	const LTMatrix& world_matrix,
	bool fog_enabled,
	float fog_near,
	float fog_far,
	DiligentWorldConstants& constants);

void diligent_apply_texture_effect_constants(
	const DiligentRBSection& section,
	uint8 pipeline_mode,
	DiligentWorldConstants& constants);

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
	uint32 index_count);

bool diligent_draw_world_immediate_mode(
	const DiligentWorldConstants& constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	uint8 mode,
	Diligent::ITextureView* texture_view,
	const DiligentWorldVertex* vertices,
	uint32 vertex_count,
	const uint16* indices,
	uint32 index_count);

bool diligent_draw_render_blocks_with_constants(
	const std::vector<DiligentRenderBlock*>& blocks,
	const DiligentWorldConstants& base_constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode);

bool diligent_draw_world_blocks();

bool diligent_draw_world_model_list_with_view(
	const std::vector<WorldModelInstance*>& models,
	const ViewParams& view_params,
	float fog_near_z,
	float fog_far_z,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode);

bool diligent_draw_world_model_list(
	const std::vector<WorldModelInstance*>& models,
	DiligentWorldBlendMode blend_mode);

bool diligent_draw_world_models_glow(
	const std::vector<WorldModelInstance*>& models,
	const DiligentWorldConstants& base_constants);

bool diligent_draw_world_glow(
	const DiligentWorldConstants& constants,
	const std::vector<DiligentRenderBlock*>& blocks);

bool diligent_update_world_constants_buffer(const DiligentWorldConstants& constants);

#endif
