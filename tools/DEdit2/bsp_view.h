#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BspBounds
{
	float min[3] = {0.0f, 0.0f, 0.0f};
	float max[3] = {0.0f, 0.0f, 0.0f};
};

struct BspSurfaceView
{
	uint32_t id = 0;
	std::string material;
	uint32_t material_id = 0;
	int32_t render_group = -1;
	int32_t lightmap_index = -1;
	uint32_t surface_flags = 0;
	uint32_t poly_count = 0;
	BspBounds bounds;
	float centroid[3] = {0.0f, 0.0f, 0.0f};
};

struct BspWorldModelView
{
	uint32_t id = 0;
	std::string name;
	BspBounds bounds;
	std::vector<BspSurfaceView> surfaces;
};

struct BspObjectView
{
	uint32_t id = 0;
	std::string class_name;
	std::string name;
	float position[3] = {0.0f, 0.0f, 0.0f};
	float rotation[3] = {0.0f, 0.0f, 0.0f};
	float scale[3] = {1.0f, 1.0f, 1.0f};
	std::string filename;  // Model/texture path from Filename, Model, ModelName properties
	std::vector<std::pair<std::string, std::string>> string_props;  // All string properties for inspection
};

struct BspWorldView
{
	uint32_t version = 1;
	std::string world_name;
	BspBounds world_bounds;
	std::vector<BspWorldModelView> worldmodels;
	std::vector<BspObjectView> objects;
};

// Implemented by the loader/renderer to expose a snapshot of the compiled world.
bool GetBspWorldView(const std::string& world_path, BspWorldView& out_view, std::string& out_error);
