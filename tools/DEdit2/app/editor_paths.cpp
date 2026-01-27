#include "app/editor_paths.h"

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

EditorPaths ParseEditorPaths(int argc, char** argv)
{
  EditorPaths paths{};
  if (const char* env_root = std::getenv("LTJS_PROJECT_ROOT"))
  {
    paths.project_root = env_root;
  }
  if (const char* env_world = std::getenv("LTJS_WORLD_FILE"))
  {
    paths.world_file = env_world;
  }

  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];
    if ((arg == "--project-root" || arg == "--project") && (i + 1) < argc)
    {
      paths.project_root = argv[++i];
    }
    else if ((arg == "--world" || arg == "--map") && (i + 1) < argc)
    {
      paths.world_file = argv[++i];
    }
  }

  if (!paths.world_file.empty())
  {
    fs::path world_path = fs::path(paths.world_file);
    if (world_path.is_relative() && !paths.project_root.empty())
    {
      world_path = fs::path(paths.project_root) / world_path;
    }
    paths.world_file = world_path.string();
  }

  return paths;
}

std::string FindCompiledWorldPath(const std::string& world_file, const std::string& project_root)
{
  if (world_file.empty())
  {
    return std::string();
  }

  fs::path world_path(world_file);
  if (world_path.is_relative() && !project_root.empty())
  {
    world_path = fs::path(project_root) / world_path;
  }
  if (world_path.extension() == ".dat")
  {
    return world_path.string();
  }

  fs::path candidate = world_path;
  candidate.replace_extension(".dat");
  if (fs::exists(candidate))
  {
    return candidate.string();
  }

  if (!project_root.empty())
  {
    const fs::path root(project_root);
    const fs::path basename = world_path.stem();
    const fs::path worlds_dat = root / "Worlds" / (basename.string() + ".dat");
    if (fs::exists(worlds_dat))
    {
      return worlds_dat.string();
    }
    const fs::path rez_worlds_dat = root / "Rez" / "Worlds" / (basename.string() + ".dat");
    if (fs::exists(rez_worlds_dat))
    {
      return rez_worlds_dat.string();
    }
  }

  return std::string();
}

std::string ResolveProjectRoot(const std::string& selected_path)
{
  if (selected_path.empty())
  {
    return {};
  }

  std::error_code ec;
  fs::path chosen = fs::path(selected_path);
  if (!fs::is_directory(chosen, ec))
  {
    return selected_path;
  }

  fs::path probe = chosen;
  for (int i = 0; i < 8; ++i)
  {
    if (fs::exists(probe / "Rez", ec) || fs::exists(probe / "Worlds", ec))
    {
      return probe.string();
    }
    const fs::path parent = probe.parent_path();
    if (parent.empty() || parent == probe)
    {
      break;
    }
    probe = parent;
  }

  return chosen.string();
}
