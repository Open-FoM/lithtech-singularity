#include "project.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace {

/// Resolve a path relative to root, or return absolute if already absolute.
fs::path ResolvePath(const fs::path& root, const fs::path& relative_or_absolute) {
  if (relative_or_absolute.empty()) {
    return {};
  }
  if (relative_or_absolute.is_absolute()) {
    return relative_or_absolute;
  }
  return root / relative_or_absolute;
}

/// Trim whitespace from string.
std::string Trim(std::string_view str) {
  size_t start = 0;
  while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])) != 0) {
    ++start;
  }
  size_t end = str.size();
  while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])) != 0) {
    --end;
  }
  return std::string(str.substr(start, end - start));
}

/// Case-insensitive string comparison.
bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  return std::equal(a.begin(), a.end(), b.begin(), [](char c1, char c2) {
    return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
  });
}

}  // namespace

fs::path Project::GetTexturesFullPath() const {
  return ResolvePath(root_path, textures_path);
}

fs::path Project::GetObjectsFullPath() const {
  return ResolvePath(root_path, objects_path);
}

fs::path Project::GetModelsFullPath() const {
  return ResolvePath(root_path, models_path);
}

fs::path Project::GetPrefabsFullPath() const {
  return ResolvePath(root_path, prefabs_path);
}

fs::path Project::GetWorldsFullPath() const {
  return ResolvePath(root_path, worlds_path);
}

fs::path Project::GetSoundsFullPath() const {
  return ResolvePath(root_path, sounds_path);
}

ProjectValidationResult Project::Validate() const {
  ProjectValidationResult result;

  if (root_path.empty()) {
    result.valid = false;
    result.errors.push_back("Project root path is empty");
    return result;
  }

  if (!fs::exists(root_path)) {
    result.valid = false;
    result.errors.push_back("Project root path does not exist: " + root_path.string());
    return result;
  }

  // Check required paths
  auto worlds_full = GetWorldsFullPath();
  if (!worlds_full.empty() && !fs::exists(worlds_full)) {
    result.warnings.push_back("Worlds directory not found: " + worlds_full.string());
  }

  // Check optional paths
  auto textures_full = GetTexturesFullPath();
  if (!textures_full.empty() && !fs::exists(textures_full)) {
    result.warnings.push_back("Textures directory not found: " + textures_full.string());
  }

  auto objects_full = GetObjectsFullPath();
  if (!objects_full.empty() && !fs::exists(objects_full)) {
    result.warnings.push_back("Objects directory not found: " + objects_full.string());
  }

  auto models_full = GetModelsFullPath();
  if (!models_full.empty() && !fs::exists(models_full)) {
    result.warnings.push_back("Models directory not found: " + models_full.string());
  }

  auto prefabs_full = GetPrefabsFullPath();
  if (!prefabs_full.empty() && !fs::exists(prefabs_full)) {
    result.warnings.push_back("Prefabs directory not found: " + prefabs_full.string());
  }

  auto sounds_full = GetSoundsFullPath();
  if (!sounds_full.empty() && !fs::exists(sounds_full)) {
    result.warnings.push_back("Sounds directory not found: " + sounds_full.string());
  }

  return result;
}

void Project::Clear() {
  name.clear();
  root_path.clear();
  dep_file_path.clear();
  textures_path.clear();
  objects_path.clear();
  models_path.clear();
  prefabs_path.clear();
  worlds_path.clear();
  sounds_path.clear();
}

std::optional<Project> LoadProject(const fs::path& dep_path, std::string& error) {
  if (!fs::exists(dep_path)) {
    error = "Project file not found: " + dep_path.string();
    return std::nullopt;
  }

  std::ifstream file(dep_path);
  if (!file.is_open()) {
    error = "Failed to open project file: " + dep_path.string();
    return std::nullopt;
  }

  Project project;
  project.dep_file_path = fs::absolute(dep_path);
  project.root_path = project.dep_file_path.parent_path();

  std::string line;
  int line_number = 0;

  while (std::getline(file, line)) {
    ++line_number;
    line = Trim(line);

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#' || line[0] == ';') {
      continue;
    }

    // Parse key=value
    auto eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
      // Try space-separated format: Key "Value"
      auto space_pos = line.find(' ');
      if (space_pos != std::string::npos) {
        std::string key = Trim(line.substr(0, space_pos));
        std::string value = Trim(line.substr(space_pos + 1));
        // Remove surrounding quotes if present
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
          value = value.substr(1, value.size() - 2);
        }

        if (EqualsIgnoreCase(key, "ProjectName")) {
          project.name = value;
        } else if (EqualsIgnoreCase(key, "TexturePath") || EqualsIgnoreCase(key, "TexturesDir")) {
          project.textures_path = value;
        } else if (EqualsIgnoreCase(key, "ObjectPath") || EqualsIgnoreCase(key, "ObjectsDir")) {
          project.objects_path = value;
        } else if (EqualsIgnoreCase(key, "ModelPath") || EqualsIgnoreCase(key, "ModelsDir")) {
          project.models_path = value;
        } else if (EqualsIgnoreCase(key, "PrefabPath") || EqualsIgnoreCase(key, "PrefabsDir")) {
          project.prefabs_path = value;
        } else if (EqualsIgnoreCase(key, "WorldPath") || EqualsIgnoreCase(key, "WorldsDir")) {
          project.worlds_path = value;
        } else if (EqualsIgnoreCase(key, "SoundPath") || EqualsIgnoreCase(key, "SoundsDir")) {
          project.sounds_path = value;
        }
        continue;
      }
      continue;  // Skip malformed lines
    }

    std::string key = Trim(line.substr(0, eq_pos));
    std::string value = Trim(line.substr(eq_pos + 1));

    // Remove surrounding quotes if present
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
      value = value.substr(1, value.size() - 2);
    }

    if (EqualsIgnoreCase(key, "ProjectName")) {
      project.name = value;
    } else if (EqualsIgnoreCase(key, "TexturePath") || EqualsIgnoreCase(key, "TexturesDir")) {
      project.textures_path = value;
    } else if (EqualsIgnoreCase(key, "ObjectPath") || EqualsIgnoreCase(key, "ObjectsDir")) {
      project.objects_path = value;
    } else if (EqualsIgnoreCase(key, "ModelPath") || EqualsIgnoreCase(key, "ModelsDir")) {
      project.models_path = value;
    } else if (EqualsIgnoreCase(key, "PrefabPath") || EqualsIgnoreCase(key, "PrefabsDir")) {
      project.prefabs_path = value;
    } else if (EqualsIgnoreCase(key, "WorldPath") || EqualsIgnoreCase(key, "WorldsDir")) {
      project.worlds_path = value;
    } else if (EqualsIgnoreCase(key, "SoundPath") || EqualsIgnoreCase(key, "SoundsDir")) {
      project.sounds_path = value;
    }
  }

  // Default name from directory if not specified
  if (project.name.empty()) {
    project.name = project.root_path.filename().string();
  }

  error.clear();
  return project;
}

bool SaveProject(const Project& project, std::string& error) {
  if (project.dep_file_path.empty()) {
    error = "No project file path specified";
    return false;
  }

  std::ofstream file(project.dep_file_path);
  if (!file.is_open()) {
    error = "Failed to create project file: " + project.dep_file_path.string();
    return false;
  }

  // Write in key=value format
  file << "# DEdit2 Project File\n";
  file << "ProjectName=" << project.name << "\n";

  if (!project.textures_path.empty()) {
    file << "TexturesDir=" << project.textures_path.string() << "\n";
  }
  if (!project.objects_path.empty()) {
    file << "ObjectsDir=" << project.objects_path.string() << "\n";
  }
  if (!project.models_path.empty()) {
    file << "ModelsDir=" << project.models_path.string() << "\n";
  }
  if (!project.prefabs_path.empty()) {
    file << "PrefabsDir=" << project.prefabs_path.string() << "\n";
  }
  if (!project.worlds_path.empty()) {
    file << "WorldsDir=" << project.worlds_path.string() << "\n";
  }
  if (!project.sounds_path.empty()) {
    file << "SoundsDir=" << project.sounds_path.string() << "\n";
  }

  if (!file.good()) {
    error = "Error writing project file: " + project.dep_file_path.string();
    return false;
  }

  error.clear();
  return true;
}

Project CreateDefaultProject(const fs::path& root_path, std::string_view name) {
  Project project;
  project.name = std::string(name);
  project.root_path = fs::absolute(root_path);
  project.dep_file_path = project.root_path / (std::string(name) + ".dep");

  // Default relative paths
  project.textures_path = "Textures";
  project.objects_path = "Objects";
  project.models_path = "Models";
  project.prefabs_path = "Prefabs";
  project.worlds_path = "Worlds";
  project.sounds_path = "Sounds";

  return project;
}
