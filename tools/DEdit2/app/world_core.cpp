#include "world.h"
#include "node_props.h"

#include <algorithm>
#include <cctype>

namespace {

/// Convert string to lowercase for case-insensitive comparison.
std::string ToLower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

}  // namespace

std::string World::GetDisplayName() const {
  if (!world_props.name.empty()) {
    return world_props.name;
  }
  if (!source_path.empty()) {
    return source_path.stem().string();
  }
  return "Untitled";
}

void World::Clear() {
  source_path.clear();
  compiled_path.clear();
  nodes.clear();
  properties.clear();
  world_props = WorldProperties{};
}

int World::FindWorldRootNode() const {
  if (nodes.empty()) {
    return -1;
  }

  // The root is typically the first non-deleted node
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (!nodes[i].deleted) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void World::ExtractWorldProperties() {
  // Find the WorldProperties node in the scene hierarchy
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].deleted) {
      continue;
    }

    const auto& props = properties[i];
    if (props.type == "WorldProperties" || props.type == "WorldNode") {
      // Extract world-level properties
      world_props.ambient[0] = props.ambient[0];
      world_props.ambient[1] = props.ambient[1];
      world_props.ambient[2] = props.ambient[2];

      world_props.background_color[0] = props.background_color[0];
      world_props.background_color[1] = props.background_color[1];
      world_props.background_color[2] = props.background_color[2];

      world_props.fog_enabled = props.fog_enabled;
      world_props.fog_near = props.fog_near;
      world_props.fog_far = props.fog_far;
      world_props.fog_density = props.fog_density;

      world_props.far_z = props.far_z;
      world_props.gravity = props.gravity;

      // Extract name from raw_properties if available
      for (const auto& [key, value] : props.raw_properties) {
        std::string lower_key = ToLower(key);
        if (lower_key == "name" || lower_key == "worldname") {
          world_props.name = value;
        } else if (lower_key == "author") {
          world_props.author = value;
        } else if (lower_key == "description") {
          world_props.description = value;
        } else if (lower_key == "skyref" || lower_key == "skyreference") {
          world_props.sky_reference = value;
        }
      }
      break;
    }
  }
}

void World::ApplyWorldProperties() {
  // Find the WorldProperties node and update it
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].deleted) {
      continue;
    }

    auto& props = properties[i];
    if (props.type == "WorldProperties" || props.type == "WorldNode") {
      props.ambient[0] = world_props.ambient[0];
      props.ambient[1] = world_props.ambient[1];
      props.ambient[2] = world_props.ambient[2];

      props.background_color[0] = world_props.background_color[0];
      props.background_color[1] = world_props.background_color[1];
      props.background_color[2] = world_props.background_color[2];

      props.fog_enabled = world_props.fog_enabled;
      props.fog_near = world_props.fog_near;
      props.fog_far = world_props.fog_far;
      props.fog_density = world_props.fog_density;

      props.far_z = world_props.far_z;
      props.gravity = world_props.gravity;
      break;
    }
  }
}

WorldFormat DetectWorldFormat(const fs::path& path) {
  std::string ext = ToLower(path.extension().string());

  if (ext == ".lta") {
    return WorldFormat::LTA;
  }
  if (ext == ".ltc") {
    return WorldFormat::LTC;
  }
  if (ext == ".dat" || ext == ".world") {
    return WorldFormat::Binary;
  }
  return WorldFormat::Unknown;
}

World CreateEmptyWorld(std::string_view name) {
  World world;
  world.world_props.name = std::string(name);

  // Create minimal world structure
  TreeNode root_node;
  root_node.name = "WorldNode";
  root_node.is_folder = false;
  root_node.deleted = false;

  NodeProperties root_props = MakeProps("WorldNode");
  root_props.ambient[0] = 0.2f;
  root_props.ambient[1] = 0.2f;
  root_props.ambient[2] = 0.2f;

  world.nodes.push_back(std::move(root_node));
  world.properties.push_back(std::move(root_props));

  world.ExtractWorldProperties();

  return world;
}
