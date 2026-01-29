#include "world.h"

#include "ltanode.h"
#include "ltadefaultalloc.h"
#include "ltanodebuilder.h"
#include "ltanodewriter.h"

#include <algorithm>
#include <cctype>

// Note: Core World methods (GetDisplayName, Clear, FindWorldRootNode,
// ExtractWorldProperties, ApplyWorldProperties, DetectWorldFormat, CreateEmptyWorld)
// are defined in world_core.cpp to allow testing without LTA dependencies.

std::optional<World> LoadWorld(
    const fs::path& path,
    const fs::path& project_root,
    std::string& error) {
  if (!fs::exists(path)) {
    error = "World file not found: " + path.string();
    return std::nullopt;
  }

  World world;
  world.source_path = fs::absolute(path);

  // Use existing BuildSceneTree function to load the world
  world.nodes = BuildSceneTree(path.string(), world.properties, error);

  if (!error.empty()) {
    return std::nullopt;
  }

  // Extract world properties from the loaded scene
  world.ExtractWorldProperties();

  // If no name was found, use the filename
  if (world.world_props.name.empty()) {
    world.world_props.name = path.stem().string();
  }

  // Try to find compiled world path
  if (!project_root.empty()) {
    fs::path compiled_name = path.stem();
    compiled_name += ".dat";

    // Look in same directory first
    fs::path compiled_in_same_dir = path.parent_path() / compiled_name;
    if (fs::exists(compiled_in_same_dir)) {
      world.compiled_path = compiled_in_same_dir;
    }
  }

  error.clear();
  return world;
}

namespace {

/// Write a property to the builder as (key value) pair.
void WriteProperty(CLTANodeBuilder& builder, const char* key, const char* value, bool quote_value = true) {
  builder.Push();
  builder.AddValue(key, false);
  builder.AddValue(value, quote_value);
  builder.Pop();
}

void WriteProperty(CLTANodeBuilder& builder, const char* key, float value) {
  builder.Push();
  builder.AddValue(key, false);
  builder.AddValue(value);
  builder.Pop();
}

void WriteProperty(CLTANodeBuilder& builder, const char* key, int value) {
  builder.Push();
  builder.AddValue(key, false);
  builder.AddValue(static_cast<int32>(value));
  builder.Pop();
}

void WriteProperty(CLTANodeBuilder& builder, const char* key, bool value) {
  builder.Push();
  builder.AddValue(key, false);
  builder.AddValue(value ? 1 : 0);
  builder.Pop();
}

void WritePropertyVec3(CLTANodeBuilder& builder, const char* key, const float* values) {
  builder.Push();
  builder.AddValue(key, false);
  builder.Push();
  builder.AddValue(values[0]);
  builder.AddValue(values[1]);
  builder.AddValue(values[2]);
  builder.Pop();
  builder.Pop();
}

/// Write node properties to the builder.
void WriteNodeProperties(
    CLTANodeBuilder& builder,
    const NodeProperties& props,
    const std::string& node_name) {
  builder.Push("properties", false);

  // Write type/class
  if (!props.type.empty()) {
    WriteProperty(builder, "type", props.type.c_str());
  }

  // Write name
  if (!node_name.empty()) {
    WriteProperty(builder, "name", node_name.c_str());
  }

  // Write position if non-zero
  if (props.position[0] != 0.0f || props.position[1] != 0.0f || props.position[2] != 0.0f) {
    WritePropertyVec3(builder, "pos", props.position);
  }

  // Write rotation if non-zero
  if (props.rotation[0] != 0.0f || props.rotation[1] != 0.0f || props.rotation[2] != 0.0f) {
    WritePropertyVec3(builder, "rotation", props.rotation);
  }

  // Write scale if non-default
  if (props.scale[0] != 1.0f || props.scale[1] != 1.0f || props.scale[2] != 1.0f) {
    WritePropertyVec3(builder, "scale", props.scale);
  }

  // Write color
  WritePropertyVec3(builder, "color", props.color);

  // Write visibility/frozen flags
  if (!props.visible) {
    WriteProperty(builder, "hidden", true);
  }
  if (props.frozen) {
    WriteProperty(builder, "frozen", true);
  }

  // Write resource if set
  if (!props.resource.empty()) {
    WriteProperty(builder, "resource", props.resource.c_str());
  }

  // Write raw properties that we don't know how to categorize
  for (const auto& [key, value] : props.raw_properties) {
    // Skip properties we've already written
    if (key == "type" || key == "name" || key == "pos" || key == "rotation" ||
        key == "scale" || key == "color" || key == "hidden" || key == "frozen" ||
        key == "resource") {
      continue;
    }
    WriteProperty(builder, key.c_str(), value.c_str());
  }

  builder.Pop();  // properties
}

/// Recursively write a world node and its children.
void WriteWorldNodeRecursive(
    CLTANodeBuilder& builder,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props,
    int node_id) {
  if (node_id < 0 || static_cast<size_t>(node_id) >= nodes.size()) {
    return;
  }

  const TreeNode& node = nodes[static_cast<size_t>(node_id)];
  if (node.deleted) {
    return;
  }

  const NodeProperties& prop = props[static_cast<size_t>(node_id)];

  builder.Push("worldnode", false);

  // Write label (node name)
  builder.Push("label", false);
  builder.AddValue(node.name.c_str(), true);
  builder.Pop();

  // Write type
  builder.Push("type", false);
  builder.AddValue(prop.type.empty() ? "null" : prop.type.c_str(), true);
  builder.Pop();

  // Write properties
  WriteNodeProperties(builder, prop, node.name);

  // Write children
  if (!node.children.empty()) {
    builder.Push("childlist", false);
    builder.Push();  // Anonymous list for children
    for (int child_id : node.children) {
      WriteWorldNodeRecursive(builder, nodes, props, child_id);
    }
    builder.Pop();  // Anonymous list
    builder.Pop();  // childlist
  }

  builder.Pop();  // worldnode
}

}  // namespace

bool SaveWorld(
    const World& world,
    const fs::path& path,
    WorldFormat format,
    std::string& error) {
  if (format == WorldFormat::Binary) {
    error = "Cannot save to compiled binary format";
    return false;
  }

  if (format == WorldFormat::Unknown) {
    error = "Unknown world format";
    return false;
  }

  if (world.nodes.empty()) {
    error = "Cannot save empty world";
    return false;
  }

  const bool compress = (format == WorldFormat::LTC);

  // Use default allocator for building
  CLTADefaultAlloc allocator;
  CLTANodeBuilder builder(&allocator);

  builder.Init();

  // Build the nodehierarchy section
  builder.Push("nodehierarchy", false);

  // Find and write root nodes (nodes that aren't children of any other node)
  std::vector<bool> is_child(world.nodes.size(), false);
  for (size_t i = 0; i < world.nodes.size(); ++i) {
    if (!world.nodes[i].deleted) {
      for (int child_id : world.nodes[i].children) {
        if (child_id >= 0 && static_cast<size_t>(child_id) < is_child.size()) {
          is_child[static_cast<size_t>(child_id)] = true;
        }
      }
    }
  }

  for (size_t i = 0; i < world.nodes.size(); ++i) {
    if (!world.nodes[i].deleted && !is_child[i]) {
      WriteWorldNodeRecursive(builder, world.nodes, world.properties, static_cast<int>(i));
    }
  }

  builder.Pop();  // nodehierarchy

  builder.PopAll();

  CLTANode* root = builder.DetachHead();
  if (root == nullptr) {
    error = "Failed to build LTA node tree";
    return false;
  }

  // Write to file
  if (!CLTANodeWriter::SaveNode(root, path.string().c_str(), compress)) {
    error = "Failed to write world file: " + path.string();
    allocator.FreeNode(root);
    return false;
  }

  allocator.FreeNode(root);
  error.clear();
  return true;
}
