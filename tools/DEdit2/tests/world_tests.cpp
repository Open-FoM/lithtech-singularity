#include "app/world.h"
#include "editor_state.h"

#include <gtest/gtest.h>

TEST(World, CreateEmptyWorldHasRootNode)
{
  World world = CreateEmptyWorld("TestWorld");

  EXPECT_FALSE(world.IsEmpty());
  EXPECT_EQ(1u, world.nodes.size());
  EXPECT_EQ(1u, world.properties.size());
  EXPECT_EQ("WorldNode", world.nodes[0].name);
  EXPECT_FALSE(world.nodes[0].deleted);
}

TEST(World, CreateEmptyWorldSetsName)
{
  World world = CreateEmptyWorld("MyLevel");

  EXPECT_EQ("MyLevel", world.world_props.name);
}

TEST(World, IsEmptyReturnsTrueForEmptyNodes)
{
  World world;
  EXPECT_TRUE(world.IsEmpty());

  world.nodes.push_back(TreeNode{});
  EXPECT_FALSE(world.IsEmpty());
}

TEST(World, GetDisplayNameUsesWorldName)
{
  World world;
  world.world_props.name = "My Custom World";

  EXPECT_EQ("My Custom World", world.GetDisplayName());
}

TEST(World, GetDisplayNameFallsBackToFilename)
{
  World world;
  world.source_path = "/path/to/level01.lta";
  world.world_props.name = "";

  EXPECT_EQ("level01", world.GetDisplayName());
}

TEST(World, GetDisplayNameReturnsUntitledWhenEmpty)
{
  World world;
  EXPECT_EQ("Untitled", world.GetDisplayName());
}

TEST(World, ClearResetsAllFields)
{
  World world = CreateEmptyWorld("Test");
  world.source_path = "/path/to/file.lta";
  world.compiled_path = "/path/to/file.dat";

  world.Clear();

  EXPECT_TRUE(world.IsEmpty());
  EXPECT_TRUE(world.source_path.empty());
  EXPECT_TRUE(world.compiled_path.empty());
  EXPECT_TRUE(world.world_props.name.empty());
}

TEST(World, FindWorldRootNodeReturnsFirstNonDeleted)
{
  World world;

  // Empty world
  EXPECT_EQ(-1, world.FindWorldRootNode());

  // Add some nodes
  TreeNode deleted_node;
  deleted_node.deleted = true;
  world.nodes.push_back(deleted_node);

  TreeNode root_node;
  root_node.name = "Root";
  root_node.deleted = false;
  world.nodes.push_back(root_node);

  // Should return index 1 (first non-deleted)
  EXPECT_EQ(1, world.FindWorldRootNode());
}

TEST(World, ExtractWorldPropertiesFromNodes)
{
  World world;

  // Create a WorldProperties node
  TreeNode root;
  root.name = "WorldProperties";
  root.deleted = false;
  world.nodes.push_back(root);

  NodeProperties props = MakeProps("WorldProperties");
  props.ambient[0] = 0.5f;
  props.ambient[1] = 0.6f;
  props.ambient[2] = 0.7f;
  props.fog_enabled = true;
  props.fog_near = 100.0f;
  props.fog_far = 5000.0f;
  props.raw_properties.emplace_back("Name", "ExtractedWorld");
  props.raw_properties.emplace_back("Author", "TestAuthor");
  world.properties.push_back(props);

  world.ExtractWorldProperties();

  EXPECT_FLOAT_EQ(0.5f, world.world_props.ambient[0]);
  EXPECT_FLOAT_EQ(0.6f, world.world_props.ambient[1]);
  EXPECT_FLOAT_EQ(0.7f, world.world_props.ambient[2]);
  EXPECT_TRUE(world.world_props.fog_enabled);
  EXPECT_FLOAT_EQ(100.0f, world.world_props.fog_near);
  EXPECT_FLOAT_EQ(5000.0f, world.world_props.fog_far);
  EXPECT_EQ("ExtractedWorld", world.world_props.name);
  EXPECT_EQ("TestAuthor", world.world_props.author);
}

TEST(World, ApplyWorldPropertiesToNodes)
{
  World world;

  // Create a WorldNode
  TreeNode root;
  root.name = "WorldNode";
  root.deleted = false;
  world.nodes.push_back(root);

  NodeProperties props = MakeProps("WorldNode");
  world.properties.push_back(props);

  // Modify world properties
  world.world_props.ambient[0] = 0.3f;
  world.world_props.ambient[1] = 0.4f;
  world.world_props.ambient[2] = 0.5f;
  world.world_props.fog_enabled = true;
  world.world_props.gravity = -15.0f;

  world.ApplyWorldProperties();

  // Check that node properties were updated
  EXPECT_FLOAT_EQ(0.3f, world.properties[0].ambient[0]);
  EXPECT_FLOAT_EQ(0.4f, world.properties[0].ambient[1]);
  EXPECT_FLOAT_EQ(0.5f, world.properties[0].ambient[2]);
  EXPECT_TRUE(world.properties[0].fog_enabled);
  EXPECT_FLOAT_EQ(-15.0f, world.properties[0].gravity);
}

TEST(WorldFormat, DetectWorldFormatFromExtension)
{
  EXPECT_EQ(WorldFormat::LTA, DetectWorldFormat("/path/to/file.lta"));
  EXPECT_EQ(WorldFormat::LTA, DetectWorldFormat("/path/to/file.LTA"));
  EXPECT_EQ(WorldFormat::LTC, DetectWorldFormat("/path/to/file.ltc"));
  EXPECT_EQ(WorldFormat::LTC, DetectWorldFormat("/path/to/file.LTC"));
  EXPECT_EQ(WorldFormat::Binary, DetectWorldFormat("/path/to/file.dat"));
  EXPECT_EQ(WorldFormat::Binary, DetectWorldFormat("/path/to/file.world"));
  EXPECT_EQ(WorldFormat::Unknown, DetectWorldFormat("/path/to/file.txt"));
  EXPECT_EQ(WorldFormat::Unknown, DetectWorldFormat("/path/to/file"));
}

TEST(WorldProperties, DefaultValues)
{
  WorldProperties props;

  // Check default ambient
  EXPECT_FLOAT_EQ(0.2f, props.ambient[0]);
  EXPECT_FLOAT_EQ(0.2f, props.ambient[1]);
  EXPECT_FLOAT_EQ(0.2f, props.ambient[2]);

  // Check default fog settings
  EXPECT_FALSE(props.fog_enabled);
  EXPECT_FLOAT_EQ(0.0f, props.fog_near);
  EXPECT_FLOAT_EQ(2000.0f, props.fog_far);

  // Check other defaults
  EXPECT_FLOAT_EQ(10000.0f, props.far_z);
  EXPECT_FLOAT_EQ(-9.8f, props.gravity);
}
