/**
 * diligent_world_draw.h
 *
 * This header defines the World Draw portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
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

/// Maximum number of dynamic lights tracked for world rendering.
constexpr uint32 kDiligentMaxDynamicLights = 64;

/// World rendering pipeline variants.
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

/// High-level shading mode overrides for world rendering.
enum DiligentWorldShadingMode : int
{
	kWorldShadingTextured = 0,
	kWorldShadingFlat = 1,
	kWorldShadingNormals = 2
};

/// Blend modes used by world pipelines.
enum DiligentWorldBlendMode : uint8
{
	kWorldBlendSolid = 0,
	kWorldBlendAlpha = 1,
	kWorldBlendAdditive = 2,
	kWorldBlendAdditiveOne = 3,
	kWorldBlendMultiply = 4
};

enum class DiligentWorldSectionFilter : uint8
{
	Normal = 0,
	LightAnim = 1
};

/// Depth mode used by world pipelines.
enum DiligentWorldDepthMode : uint8
{
	kWorldDepthEnabled = 0,
	kWorldDepthDisabled = 1
};

/// Legacy PC shader classification for world sections.
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

/// Constants for shadow projection passes.
struct DiligentShadowProjectConstants
{
	std::array<float, 16> world_to_shadow{};
	float light_dir[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float proj_center[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

/// Hashable pipeline key for world pipeline caching.
struct DiligentWorldPipelineKey
{
	uint8 mode = kWorldPipelineSolid;
	uint8 blend_mode = kWorldBlendSolid;
	uint8 depth_mode = kWorldDepthEnabled;
	uint8 wireframe = 0;
	uint8 sample_count = 1;

	bool operator==(const DiligentWorldPipelineKey& other) const
	{
		return mode == other.mode &&
			blend_mode == other.blend_mode &&
			depth_mode == other.depth_mode &&
			wireframe == other.wireframe &&
			sample_count == other.sample_count;
	}
};

/// Hash functor for DiligentWorldPipelineKey.
struct DiligentWorldPipelineKeyHash
{
	size_t operator()(const DiligentWorldPipelineKey& key) const noexcept
	{
		uint64 hash = 0;
		hash = diligent_hash_combine(hash, key.mode);
		hash = diligent_hash_combine(hash, key.blend_mode);
		hash = diligent_hash_combine(hash, key.depth_mode);
		hash = diligent_hash_combine(hash, key.wireframe);
		hash = diligent_hash_combine(hash, key.sample_count);
		return static_cast<size_t>(hash);
	}
};

/// GPU resources shared by world rendering.
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

/// Cached pipeline state for a specific world pipeline key.
struct DiligentWorldPipeline
{
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> pipeline_state;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
};

/// Collection of cached world pipelines.
struct DiligentWorldPipelines
{
	std::unordered_map<DiligentWorldPipelineKey, DiligentWorldPipeline, DiligentWorldPipelineKeyHash> pipelines;
};

/// Shared world render resources.
extern DiligentWorldResources g_world_resources;
/// Cached world pipeline collection.
extern DiligentWorldPipelines g_world_pipelines;
/// Visible render blocks for the current frame.
extern std::vector<DiligentRenderBlock*> g_visible_render_blocks;
/// Visible opaque world models for the current frame.
extern std::vector<WorldModelInstance*> g_diligent_solid_world_models;
/// Visible translucent world models for the current frame.
extern std::vector<WorldModelInstance*> g_diligent_translucent_world_models;
/// Dynamic lights affecting world rendering this frame.
extern DynamicLight* g_diligent_world_dynamic_lights[kDiligentMaxDynamicLights];
/// Number of active dynamic lights in g_diligent_world_dynamic_lights.
extern uint32 g_diligent_num_world_dynamic_lights;
/// True when rendering in shadow mode.
extern bool g_diligent_shadow_mode;

/// \brief Returns the effective world shading mode based on console vars.
/// \details This is typically driven by renderer console variables to force
///          flat or normals visualization during debugging.
int diligent_get_world_shading_mode();
/// \brief Fills a blend descriptor for the requested world blend mode.
/// \details Used when constructing pipeline state objects.
void diligent_fill_world_blend_desc(DiligentWorldBlendMode blend, Diligent::RenderTargetBlendDesc& blend_desc);
/// \brief Retrieves or creates a world pipeline for the given settings.
/// \details Pipelines are cached by a composite key of mode, blend, depth,
///          and wireframe to avoid redundant PSO creation.
/// \code
/// auto* pipeline = diligent_get_world_pipeline(kWorldPipelineTextured, kWorldBlendSolid, kWorldDepthEnabled, false);
/// if (!pipeline) { /* handle error */ }
/// \endcode
DiligentWorldPipeline* diligent_get_world_pipeline(
	uint8 mode,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	bool wireframe);

/// \brief Ensures the world constants buffer is allocated.
bool diligent_ensure_world_constant_buffer();
/// \brief Ensures the shadow projection constants buffer is allocated.
bool diligent_ensure_shadow_project_constants();
/// \brief Ensures all world shaders are compiled and ready.
bool diligent_ensure_world_shaders();
/// \brief Ensures the world vertex buffer is allocated to \p size bytes.
bool diligent_ensure_world_vertex_buffer(uint32 size);
/// \brief Ensures the world index buffer is allocated to \p size bytes.
bool diligent_ensure_world_index_buffer(uint32 size);

/// \brief Writes packed color into a world vertex.
void diligent_set_world_vertex_color(DiligentWorldVertex& vertex, uint32 color);
/// \brief Writes normal vector into a world vertex.
void diligent_set_world_vertex_normal(DiligentWorldVertex& vertex, const LTVector& normal);
/// \brief Populates world constants for the given view and fog settings.
/// \details This is typically invoked once per frame before world rendering.
void diligent_fill_world_constants(
	const ViewParams& view_params,
	const LTMatrix& world_matrix,
	bool fog_enabled,
	float fog_near,
	float fog_far,
	DiligentWorldConstants& constants);

/// \brief Applies texture effect transforms into world constants for a section.
/// \details Texture effect matrices are applied per section based on shader
///          stage metadata and effect parameters.
void diligent_apply_texture_effect_constants(
	const DiligentRBSection& section,
	uint8 pipeline_mode,
	DiligentWorldConstants& constants);

/// \brief Draws world geometry from immediate-mode vertex/index data.
/// \details This path is used by editor/debug tools that provide transient
///          vertex data rather than pre-baked render blocks.
/// \code
/// DiligentWorldConstants constants{};
/// // ... fill constants ...
/// diligent_draw_world_immediate(constants, kWorldBlendSolid, kWorldDepthEnabled, tex0, tex1, false, verts, vcount, indices, icount);
/// \endcode
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

/// \brief Draws world geometry using an explicit pipeline mode.
/// \details Lets callers override the pipeline selection without relying
///          on per-section shader metadata.
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

/// \brief Draws cached render blocks using precomputed constants.
/// \details This is the primary world draw path used by diligent_RenderScene.
bool diligent_draw_render_blocks_with_constants(
	const std::vector<DiligentRenderBlock*>& blocks,
	const DiligentWorldConstants& base_constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	DiligentWorldSectionFilter section_filter = DiligentWorldSectionFilter::Normal);

/// \brief Draws all visible world render blocks for the current frame.
bool diligent_draw_world_blocks();

/// \brief Draws world-model instances using explicit view/fog parameters.
/// \details Used by editor views or multi-pass rendering where view parameters
///          differ from the global renderer state.
bool diligent_draw_world_model_list_with_view(
	const std::vector<WorldModelInstance*>& models,
	const ViewParams& view_params,
	float fog_near_z,
	float fog_far_z,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode);

/// \brief Draws world-model instances using current frame view/fog parameters.
bool diligent_draw_world_model_list(
	const std::vector<WorldModelInstance*>& models,
	DiligentWorldBlendMode blend_mode);

/// \brief Draws glow passes for world-model instances.
bool diligent_draw_world_models_glow(
	const std::vector<WorldModelInstance*>& models,
	const DiligentWorldConstants& base_constants);

/// \brief Draws glow passes for world render blocks.
bool diligent_draw_world_glow(
	const DiligentWorldConstants& constants,
	const std::vector<DiligentRenderBlock*>& blocks);

/// \brief Uploads world constants to the GPU.
/// \details This should be called after diligent_fill_world_constants and
///          before issuing world draw calls that rely on the buffer.
bool diligent_update_world_constants_buffer(const DiligentWorldConstants& constants);

#endif
