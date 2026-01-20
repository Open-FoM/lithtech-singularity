#pragma once

#include "bsp_view.h"

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

struct NodeProperties
{
	std::string type;
	bool visible = true;
	bool locked = false;
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
};

struct TreeUiState
{
	int pending_create_parent = -1;
	bool pending_create_folder = false;
	int rename_node_id = -1;
	bool open_rename_popup = false;
	char rename_buffer[128] = {};
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
