#pragma once

#include <string>
#include <vector>

void LoadRecentProjects(std::vector<std::string>& recent_projects);
void PruneRecentProjects(std::vector<std::string>& recent_projects);
void SaveRecentProjects(const std::vector<std::string>& recent_projects);
void PushRecentProject(std::vector<std::string>& recent_projects, const std::string& path);
