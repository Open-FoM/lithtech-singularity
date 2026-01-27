#pragma once

#include <string>

struct EditorPaths
{
  std::string project_root;
  std::string world_file;
};

EditorPaths ParseEditorPaths(int argc, char** argv);
std::string ResolveProjectRoot(const std::string& selected_path);
std::string FindCompiledWorldPath(const std::string& world_file, const std::string& project_root);
