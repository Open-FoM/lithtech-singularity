#include "texture_replace.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace texture_ops {

namespace {

/// Convert a string to lowercase.
std::string ToLower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

/// Simple wildcard pattern matching.
/// Supports * (match any sequence) and ? (match single char).
bool WildcardMatch(std::string_view str, std::string_view pattern) {
  size_t s = 0, p = 0;
  size_t star_idx = std::string_view::npos;
  size_t match_idx = 0;

  while (s < str.size()) {
    // Matching characters or ?
    if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == str[s])) {
      ++s;
      ++p;
    }
    // * found, record position
    else if (p < pattern.size() && pattern[p] == '*') {
      star_idx = p;
      match_idx = s;
      ++p;
    }
    // Mismatch after *, backtrack
    else if (star_idx != std::string_view::npos) {
      p = star_idx + 1;
      ++match_idx;
      s = match_idx;
    }
    // No match
    else {
      return false;
    }
  }

  // Check remaining pattern characters (must all be *)
  while (p < pattern.size() && pattern[p] == '*') {
    ++p;
  }

  return p == pattern.size();
}

} // namespace

bool MatchesTexturePattern(std::string_view texture_name,
                            std::string_view pattern,
                            bool case_sensitive) {
  if (pattern.empty()) {
    return texture_name.empty();
  }

  if (case_sensitive) {
    return WildcardMatch(texture_name, pattern);
  }

  return WildcardMatch(ToLower(texture_name), ToLower(pattern));
}

bool TextureNamesEqual(std::string_view a, std::string_view b, bool case_sensitive) {
  if (a.size() != b.size()) {
    return false;
  }

  if (case_sensitive) {
    return a == b;
  }

  return ToLower(a) == ToLower(b);
}

bool ReplacePolygonTexture(csg::CSGPolygon& polygon,
                           std::string_view old_texture,
                           std::string_view new_texture,
                           const ReplaceTextureOptions& options) {
  bool matches = false;

  if (options.use_pattern) {
    matches = MatchesTexturePattern(polygon.face_props.texture_name, old_texture, options.case_sensitive);
  } else {
    matches = TextureNamesEqual(polygon.face_props.texture_name, old_texture, options.case_sensitive);
  }

  if (matches) {
    if (!options.preserve_mapping) {
      polygon.face_props.mapping = TextureMapping();
    }
    polygon.face_props.texture_name = std::string(new_texture);
    return true;
  }

  return false;
}

size_t ReplaceTextureInPolygons(std::vector<csg::CSGPolygon>& polygons,
                                 const ReplaceTextureOptions& options) {
  size_t modified = 0;

  for (auto& polygon : polygons) {
    if (ReplacePolygonTexture(polygon, options.old_texture, options.new_texture, options)) {
      ++modified;
    }
  }

  return modified;
}

size_t ReplaceTextureInFaces(std::vector<csg::CSGPolygon>& polygons,
                              const std::vector<uint32_t>& face_indices,
                              const ReplaceTextureOptions& options) {
  size_t modified = 0;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      if (ReplacePolygonTexture(polygons[idx], options.old_texture, options.new_texture, options)) {
        ++modified;
      }
    }
  }

  return modified;
}

std::vector<uint32_t> FindFacesWithTexture(const std::vector<csg::CSGPolygon>& polygons,
                                            std::string_view texture_name,
                                            bool case_sensitive) {
  std::vector<uint32_t> result;

  for (size_t i = 0; i < polygons.size(); ++i) {
    if (TextureNamesEqual(polygons[i].face_props.texture_name, texture_name, case_sensitive)) {
      result.push_back(static_cast<uint32_t>(i));
    }
  }

  return result;
}

std::vector<uint32_t> FindFacesMatchingPattern(const std::vector<csg::CSGPolygon>& polygons,
                                                std::string_view pattern,
                                                bool case_sensitive) {
  std::vector<uint32_t> result;

  for (size_t i = 0; i < polygons.size(); ++i) {
    if (MatchesTexturePattern(polygons[i].face_props.texture_name, pattern, case_sensitive)) {
      result.push_back(static_cast<uint32_t>(i));
    }
  }

  return result;
}

std::vector<std::string> GetUniqueTextures(const std::vector<csg::CSGPolygon>& polygons) {
  std::set<std::string> unique_set;

  for (const auto& polygon : polygons) {
    if (!polygon.face_props.texture_name.empty()) {
      unique_set.insert(polygon.face_props.texture_name);
    }
  }

  return std::vector<std::string>(unique_set.begin(), unique_set.end());
}

size_t CountFacesWithTexture(const std::vector<csg::CSGPolygon>& polygons,
                              std::string_view texture_name,
                              bool case_sensitive) {
  size_t count = 0;

  for (const auto& polygon : polygons) {
    if (TextureNamesEqual(polygon.face_props.texture_name, texture_name, case_sensitive)) {
      ++count;
    }
  }

  return count;
}

} // namespace texture_ops
