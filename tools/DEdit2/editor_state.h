#pragma once

#include "bsp_view.h"
#include "brush/texture_ops/uv_types.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct TreeNode
{
	std::string name;
	std::vector<int> children;
	bool is_folder = false;
	bool deleted = false;
};

/// Per-face texture data for brush serialization.
/// Stores texture properties for each face of a brush.
struct BrushFaceTextureData
{
	std::string texture_name;                                            ///< Texture path/name
	texture_ops::TextureMapping mapping;                                 ///< UV mapping parameters
	uint32_t surface_flags = static_cast<uint32_t>(texture_ops::SurfaceFlags::Default);
	uint8_t alpha_ref = 0;                                               ///< Alpha test reference value

	/// Create from FaceProperties.
	static BrushFaceTextureData FromFaceProperties(const texture_ops::FaceProperties& props) {
		BrushFaceTextureData data;
		data.texture_name = props.texture_name;
		data.mapping = props.mapping;
		data.surface_flags = static_cast<uint32_t>(props.flags);
		data.alpha_ref = props.alpha_ref;
		return data;
	}

	/// Convert to FaceProperties.
	[[nodiscard]] texture_ops::FaceProperties ToFaceProperties() const {
		texture_ops::FaceProperties props;
		props.texture_name = texture_name;
		props.mapping = mapping;
		props.flags = static_cast<texture_ops::SurfaceFlags>(surface_flags);
		props.alpha_ref = alpha_ref;
		return props;
	}
};

struct NodeProperties
{
	std::string type;
	bool visible = true;
	bool frozen = false;
	float position[3] = {0.0f, 0.0f, 0.0f};
	float rotation[3] = {0.0f, 0.0f, 0.0f};
	float scale[3] = {1.0f, 1.0f, 1.0f};
	float color[3] = {1.0f, 1.0f, 1.0f};
	float background_color[3] = {0.0f, 0.0f, 0.0f};
	float ambient[3] = {0.2f, 0.2f, 0.2f};
	float size[3] = {10.0f, 5.0f, 10.0f};
	float intensity = 3.0f;
	float range = 20.0f;
	float temperature = 6500.0f;
	float gravity = -9.8f;
	float fog_density = 0.01f;
	float fog_near = 0.0f;
	float fog_far = 2000.0f;
	float far_z = 10000.0f;
	float height_scale = 10.0f;
	float model_scale = 1.0f;
	float volume = 0.8f;
	bool cast_shadows = true;
	bool use_temperature = false;
	bool fog_enabled = false;
	bool sky_pan_enabled = false;
	bool srgb = true;
	bool mipmaps = true;
	bool loop = false;
	int compression_mode = 0;
	float sky_pan_scale[2] = {1.0f, 1.0f};
	float sky_pan_auto_pan[2] = {0.0f, 0.0f};
	std::string resource;
	std::string class_name;
	std::string sky_pan_texture;
	std::vector<std::pair<std::string, std::string>> raw_properties;
	int brush_index = -1;
	std::vector<float> brush_vertices;
	std::vector<uint32_t> brush_indices;

	// EPIC-10: Texture data for brushes
	std::vector<float> brush_uvs;                        ///< Per-vertex UV coords (size = num_verts * 2)
	std::vector<BrushFaceTextureData> brush_face_textures; ///< Per-face texture properties
};

struct TreeUiState
{
	int pending_create_parent = -1;
	bool pending_create_folder = false;
	int rename_node_id = -1;
	bool open_rename_popup = false;
	char rename_buffer[128] = {};

	/// Node ID to focus viewport on (set by double-click in tree).
	/// -1 means no focus request. Caller should reset to -1 after handling.
	int focus_request_id = -1;
};

enum class SelectionTarget
{
	Project,
	Scene
};

NodeProperties MakeProps(const char* type);
std::vector<TreeNode> BuildProjectTree(
	const std::string& root_path,
	std::vector<NodeProperties>& out_props,
	std::string& error);
std::vector<TreeNode> BuildSceneTree(
	const std::string& world_file,
	std::vector<NodeProperties>& out_props,
	std::string& error);
struct BspTreeBuildOptions
{
	enum class SurfaceGrouping
	{
		Material,
		RenderGroup,
		Lightmap
	};

	SurfaceGrouping grouping = SurfaceGrouping::Material;
};

std::vector<TreeNode> BuildSceneTreeFromBsp(
	const BspWorldView& view,
	std::vector<NodeProperties>& out_props,
	std::string& error,
	const BspTreeBuildOptions& options = {});
const char* GetNodeName(const std::vector<TreeNode>& nodes, int node_id);
int AddChildNode(
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int parent_id,
	const char* name,
	bool is_folder,
	const char* type_name);
