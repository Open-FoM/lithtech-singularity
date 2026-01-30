#pragma once

/// @file texture_replace.h
/// @brief Texture search and replace operations (US-10-06).
///
/// Provides functions to find and replace textures across brushes
/// with support for selection-only mode and pattern matching.

#include "brush/csg/csg_types.h"
#include "uv_types.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace texture_ops {

/// Options for texture replacement operation.
struct ReplaceTextureOptions {
  std::string old_texture;          ///< Texture name to find (case-insensitive)
  std::string new_texture;          ///< Texture name to replace with
  bool selection_only = false;      ///< Only replace in specified brushes
  bool preserve_mapping = true;     ///< Keep UV mapping when replacing
  bool case_sensitive = false;      ///< Case-sensitive texture matching
  bool use_pattern = false;         ///< Treat old_texture as wildcard pattern
};

/// Result of a texture replacement operation.
struct ReplaceTextureResult {
  bool success = false;
  size_t faces_modified = 0;
  size_t brushes_modified = 0;
  std::string error_message;

  [[nodiscard]] static ReplaceTextureResult Success(size_t faces, size_t brushes) {
    ReplaceTextureResult result;
    result.success = true;
    result.faces_modified = faces;
    result.brushes_modified = brushes;
    return result;
  }

  [[nodiscard]] static ReplaceTextureResult Failure(std::string message) {
    ReplaceTextureResult result;
    result.success = false;
    result.error_message = std::move(message);
    return result;
  }
};

/// Information about a texture match found during search.
struct TextureMatch {
  int brush_index = -1;             ///< Index of brush containing the match
  uint32_t face_index = 0;          ///< Face index within the brush
  std::string texture_name;         ///< The matched texture name
};

/// Result of a texture search operation.
struct SearchTextureResult {
  std::vector<TextureMatch> matches;
  size_t total_matches = 0;
};

/// Check if a texture name matches a pattern (case-insensitive).
/// @param texture_name Texture name to check.
/// @param pattern Pattern to match (supports * wildcard).
/// @param case_sensitive Whether to match case-sensitively.
/// @return true if the texture name matches the pattern.
[[nodiscard]] bool MatchesTexturePattern(std::string_view texture_name,
                                          std::string_view pattern,
                                          bool case_sensitive = false);

/// Check if two texture names are equal (with case sensitivity option).
/// @param a First texture name.
/// @param b Second texture name.
/// @param case_sensitive Whether to compare case-sensitively.
/// @return true if the names are equal.
[[nodiscard]] bool TextureNamesEqual(std::string_view a,
                                      std::string_view b,
                                      bool case_sensitive = false);

/// Replace texture on a single polygon.
/// @param polygon Polygon to modify.
/// @param old_texture Texture name to find.
/// @param new_texture Texture name to replace with.
/// @param options Replacement options.
/// @return true if the texture was replaced.
bool ReplacePolygonTexture(csg::CSGPolygon& polygon,
                           std::string_view old_texture,
                           std::string_view new_texture,
                           const ReplaceTextureOptions& options);

/// Replace textures across all polygons.
/// @param polygons Vector of polygons to modify.
/// @param options Replacement options.
/// @return Number of faces modified.
size_t ReplaceTextureInPolygons(std::vector<csg::CSGPolygon>& polygons,
                                 const ReplaceTextureOptions& options);

/// Replace textures in specific faces.
/// @param polygons Vector of polygons to modify.
/// @param face_indices Indices of faces to check and replace.
/// @param options Replacement options.
/// @return Number of faces modified.
size_t ReplaceTextureInFaces(std::vector<csg::CSGPolygon>& polygons,
                              const std::vector<uint32_t>& face_indices,
                              const ReplaceTextureOptions& options);

/// Search for polygons using a specific texture.
/// @param polygons Vector of polygons to search.
/// @param texture_name Texture name to find.
/// @param case_sensitive Whether to match case-sensitively.
/// @return Vector of face indices that use the texture.
[[nodiscard]] std::vector<uint32_t> FindFacesWithTexture(
    const std::vector<csg::CSGPolygon>& polygons,
    std::string_view texture_name,
    bool case_sensitive = false);

/// Search for polygons matching a texture pattern.
/// @param polygons Vector of polygons to search.
/// @param pattern Texture pattern to match (supports * wildcard).
/// @param case_sensitive Whether to match case-sensitively.
/// @return Vector of face indices that match the pattern.
[[nodiscard]] std::vector<uint32_t> FindFacesMatchingPattern(
    const std::vector<csg::CSGPolygon>& polygons,
    std::string_view pattern,
    bool case_sensitive = false);

/// Get a list of all unique textures used in polygons.
/// @param polygons Vector of polygons to scan.
/// @return Sorted vector of unique texture names.
[[nodiscard]] std::vector<std::string> GetUniqueTextures(
    const std::vector<csg::CSGPolygon>& polygons);

/// Count how many faces use a specific texture.
/// @param polygons Vector of polygons to count.
/// @param texture_name Texture name to count.
/// @param case_sensitive Whether to match case-sensitively.
/// @return Number of faces using the texture.
[[nodiscard]] size_t CountFacesWithTexture(
    const std::vector<csg::CSGPolygon>& polygons,
    std::string_view texture_name,
    bool case_sensitive = false);

} // namespace texture_ops
