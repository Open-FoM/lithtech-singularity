#include "app/project.h"

#include <gtest/gtest.h>

#include <fstream>

class ProjectTest : public ::testing::Test {
protected:
  fs::path temp_dir;

  void SetUp() override {
    temp_dir = fs::temp_directory_path() / "dedit2_project_test";
    fs::create_directories(temp_dir);
  }

  void TearDown() override {
    fs::remove_all(temp_dir);
  }

  void WriteDepFile(const std::string& filename, const std::string& content) {
    std::ofstream file(temp_dir / filename);
    file << content;
  }
};

TEST_F(ProjectTest, ParseValidDepFile)
{
  WriteDepFile("test.dep",
    "ProjectName=MyGame\n"
    "TexturesDir=Textures\n"
    "ObjectsDir=Objects\n"
    "ModelsDir=Models\n"
    "PrefabsDir=Prefabs\n"
    "WorldsDir=Worlds\n"
    "SoundsDir=Sounds\n"
  );

  std::string error;
  auto project = LoadProject(temp_dir / "test.dep", error);

  ASSERT_TRUE(project.has_value());
  EXPECT_TRUE(error.empty());
  EXPECT_EQ("MyGame", project->name);
  EXPECT_EQ("Textures", project->textures_path.string());
  EXPECT_EQ("Objects", project->objects_path.string());
  EXPECT_EQ("Models", project->models_path.string());
  EXPECT_EQ("Prefabs", project->prefabs_path.string());
  EXPECT_EQ("Worlds", project->worlds_path.string());
  EXPECT_EQ("Sounds", project->sounds_path.string());
}

TEST_F(ProjectTest, ParseMissingKeysReturnsDefaults)
{
  WriteDepFile("minimal.dep",
    "ProjectName=MinimalProject\n"
  );

  std::string error;
  auto project = LoadProject(temp_dir / "minimal.dep", error);

  ASSERT_TRUE(project.has_value());
  EXPECT_EQ("MinimalProject", project->name);
  // Missing paths should be empty
  EXPECT_TRUE(project->textures_path.empty());
  EXPECT_TRUE(project->objects_path.empty());
}

TEST_F(ProjectTest, ParseHandlesWhitespaceAndComments)
{
  WriteDepFile("whitespace.dep",
    "  ProjectName = SpacedProject  \n"
    "# This is a comment\n"
    "TexturesDir=  Tex  \n"
    "\n"
    "ObjectsDir=Obj\n"
  );

  std::string error;
  auto project = LoadProject(temp_dir / "whitespace.dep", error);

  ASSERT_TRUE(project.has_value());
  EXPECT_EQ("SpacedProject", project->name);
  EXPECT_EQ("Tex", project->textures_path.string());
  EXPECT_EQ("Obj", project->objects_path.string());
}

TEST_F(ProjectTest, LoadFailsForMissingFile)
{
  std::string error;
  auto project = LoadProject(temp_dir / "nonexistent.dep", error);

  EXPECT_FALSE(project.has_value());
  EXPECT_FALSE(error.empty());
}

TEST_F(ProjectTest, SaveAndReloadRoundTrip)
{
  Project original;
  original.name = "RoundTripTest";
  original.root_path = temp_dir;
  original.dep_file_path = temp_dir / "roundtrip.dep";
  original.textures_path = "GameTextures";
  original.objects_path = "GameObjects";
  original.models_path = "GameModels";
  original.prefabs_path = "GamePrefabs";
  original.worlds_path = "GameWorlds";
  original.sounds_path = "GameSounds";

  std::string error;
  ASSERT_TRUE(SaveProject(original, error));
  EXPECT_TRUE(error.empty());

  auto reloaded = LoadProject(temp_dir / "roundtrip.dep", error);
  ASSERT_TRUE(reloaded.has_value());
  EXPECT_EQ(original.name, reloaded->name);
  EXPECT_EQ(original.textures_path, reloaded->textures_path);
  EXPECT_EQ(original.objects_path, reloaded->objects_path);
  EXPECT_EQ(original.models_path, reloaded->models_path);
  EXPECT_EQ(original.prefabs_path, reloaded->prefabs_path);
  EXPECT_EQ(original.worlds_path, reloaded->worlds_path);
  EXPECT_EQ(original.sounds_path, reloaded->sounds_path);
}

TEST_F(ProjectTest, CreateDefaultProjectIsValid)
{
  Project project = CreateDefaultProject(temp_dir, "DefaultTest");

  EXPECT_TRUE(project.IsValid());
  EXPECT_EQ("DefaultTest", project.name);
  EXPECT_EQ(temp_dir, project.root_path);
  // Should have default paths set
  EXPECT_FALSE(project.textures_path.empty());
  EXPECT_FALSE(project.worlds_path.empty());
}

TEST_F(ProjectTest, ValidateReportsWarningsForMissingPaths)
{
  Project project = CreateDefaultProject(temp_dir, "ValidationTest");

  // Paths don't exist yet - should generate warnings but still be valid
  // (only root_path missing makes it invalid)
  auto result = project.Validate();
  EXPECT_TRUE(result.valid);  // Valid because root exists
  EXPECT_FALSE(result.warnings.empty());  // Warnings for missing resource dirs

  // Create the worlds directory
  fs::create_directories(project.GetWorldsFullPath());

  // Re-validate - should have fewer warnings
  auto result2 = project.Validate();
  EXPECT_TRUE(result2.valid);
  // Fewer warnings now that worlds dir exists
  EXPECT_LE(result2.warnings.size(), result.warnings.size());
}

TEST_F(ProjectTest, ValidateFailsForMissingRoot)
{
  Project project;
  project.root_path = temp_dir / "nonexistent";

  auto result = project.Validate();
  EXPECT_FALSE(result.valid);
  EXPECT_FALSE(result.errors.empty());
}

TEST_F(ProjectTest, GetFullPathResolvesRelative)
{
  Project project;
  project.root_path = temp_dir;
  project.textures_path = "Textures";
  project.worlds_path = "/absolute/path/worlds";

  // Relative path should be resolved
  EXPECT_EQ(temp_dir / "Textures", project.GetTexturesFullPath());

  // Absolute path should be returned as-is
  EXPECT_EQ("/absolute/path/worlds", project.GetWorldsFullPath().string());
}

TEST(ProjectBasic, ClearResetsAllFields)
{
  Project project;
  project.name = "Test";
  project.root_path = "/some/path";
  project.textures_path = "Tex";

  project.Clear();

  EXPECT_TRUE(project.name.empty());
  EXPECT_TRUE(project.root_path.empty());
  EXPECT_TRUE(project.textures_path.empty());
  EXPECT_FALSE(project.IsValid());
}
