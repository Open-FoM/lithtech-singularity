/**
 * diligent_model_draw.h
 *
 * This header defines the Model Draw portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_MODEL_DRAW_H
#define LTJS_DILIGENT_MODEL_DRAW_H

#include "diligent_mesh_layout.h"
#include "diligent_pipeline_cache.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Buffer.h"

#include "ltmatrix.h"
#include "model.h"

#include <array>
#include <vector>

class CRenderStyleMap;
class ModelInstance;
struct RenderPassOp;
struct RSRenderPassShaders;
struct SceneDesc;
struct ModelHookData;
class ILTStream;
struct LTB_Header;

/// Rigid mesh implementation backed by Diligent vertex/index buffers.
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
	const std::array<std::vector<uint8>, 4>& GetVertexData() const { return m_vertex_data; }
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

/// Subset of bones affecting a vertex range for skinned rendering.
struct DiligentBoneSet
{
	uint16 first_vert_index = 0;
	uint16 vert_count = 0;
	uint8 bone_set[4] = {};
	uint32 index_into_index_buffer = 0;
};

/// Maps duplicated vertices back to source indices.
struct DiligentDupMap
{
	uint16 src_vert = 0;
	uint16 dst_vert = 0;
};

/// Skinned mesh implementation backed by Diligent buffers.
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
	const std::array<std::vector<uint8>, 4>& GetVertexData() const { return m_vertex_data; }
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

/// Vertex-animated mesh implementation backed by Diligent buffers.
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
	const std::array<std::vector<uint8>, 4>& GetVertexData() const { return m_vertex_data; }
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

/// \brief Draws a model instance using its default render styles.
/// \details This is the standard model rendering path used by scene rendering.
/// \code
/// if (!diligent_draw_model_instance(instance)) { /* handle error */ }
/// \endcode
bool diligent_draw_model_instance(ModelInstance* instance);
/// \brief Draws a model instance using an explicit render-style map.
/// \details Used by glow passes or editor overrides to remap render styles.
bool diligent_draw_model_instance_with_render_style_map(ModelInstance* instance, const CRenderStyleMap* render_style_map);
/// \brief Retrieves model hook data and invokes hook callbacks if present.
bool diligent_get_model_hook_data(ModelInstance* instance, ModelHookData& hook_data);

/// \brief Draws a model instance and its attachments into a shadow pass.
/// \details This is used for projected shadows onto world geometry.
bool diligent_draw_model_shadow_with_attachments(
	ModelInstance* instance,
	const RenderPassOp& pass,
	const RSRenderPassShaders& shader_pass,
	Diligent::TEXTURE_FORMAT color_format,
	Diligent::TEXTURE_FORMAT depth_format,
	uint8 shadow_r,
	uint8 shadow_g,
	uint8 shadow_b);

/// \brief Returns true if model debug visualization is enabled.
bool diligent_model_debug_enabled();
/// \brief Adds debug visualization for a model instance to the debug draw list.
void diligent_debug_add_model_info(ModelInstance* instance);
/// \brief Releases cached model GPU resources.
void diligent_release_model_resources();

#endif
