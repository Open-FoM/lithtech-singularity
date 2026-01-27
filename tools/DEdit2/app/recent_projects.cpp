#include "app/recent_projects.h"

#include "app/string_utils.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{
std::string GetRecentProjectsPath()
{
  char* pref_path = SDL_GetPrefPath("LithTech", "DEdit2");
  if (!pref_path)
  {
    return {};
  }
  std::string path = pref_path;
  SDL_free(pref_path);
  if (!path.empty() && path.back() != '/' && path.back() != '\\')
  {
    path.push_back('/');
  }
  path += "recent_projects.txt";
  return path;
}

bool IsValidRecentProjectPath(const std::string& path)
{
  if (path.empty())
  {
    return false;
  }
  std::error_code ec;
  return fs::exists(path, ec) && fs::is_directory(path, ec);
}
}

void LoadRecentProjects(std::vector<std::string>& recent_projects)
{
  recent_projects.clear();
  const std::string path = GetRecentProjectsPath();
  if (path.empty())
  {
    return;
  }

  std::ifstream input(path);
  if (!input.is_open())
  {
    return;
  }

  std::unordered_set<std::string> seen;
  std::string line;
  while (std::getline(input, line))
  {
    std::string trimmed = TrimLine(line);
    if (trimmed.empty())
    {
      continue;
    }
    if (seen.insert(trimmed).second)
    {
      recent_projects.push_back(trimmed);
    }
  }
}

void PruneRecentProjects(std::vector<std::string>& recent_projects)
{
  const size_t before = recent_projects.size();
  recent_projects.erase(
    std::remove_if(
      recent_projects.begin(),
      recent_projects.end(),
      [](const std::string& path)
      {
        return !IsValidRecentProjectPath(path);
      }),
    recent_projects.end());
  if (recent_projects.size() != before)
  {
    SaveRecentProjects(recent_projects);
  }
}

void SaveRecentProjects(const std::vector<std::string>& recent_projects)
{
  const std::string path = GetRecentProjectsPath();
  if (path.empty())
  {
    return;
  }

  std::ofstream output(path, std::ios::trunc);
  if (!output.is_open())
  {
    return;
  }

  for (const auto& entry : recent_projects)
  {
    output << entry << '\n';
  }
}

void PushRecentProject(std::vector<std::string>& recent_projects, const std::string& path)
{
  if (!IsValidRecentProjectPath(path))
  {
    return;
  }

  auto existing = std::find(recent_projects.begin(), recent_projects.end(), path);
  if (existing != recent_projects.end())
  {
    recent_projects.erase(existing);
  }
  recent_projects.insert(recent_projects.begin(), path);
  constexpr size_t kMaxRecent = 10;
  if (recent_projects.size() > kMaxRecent)
  {
    recent_projects.resize(kMaxRecent);
  }
  SaveRecentProjects(recent_projects);
}
