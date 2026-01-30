#pragma once

/// @file surface_flags.h
/// @brief Surface flags and alpha operations (US-10-07, US-10-08).
///
/// Provides functions to manipulate surface flags and alpha reference
/// values on brush faces.

#include "brush/csg/csg_types.h"
#include "uv_types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace texture_ops {

/// Options for setting surface flags.
struct SetFlagsOptions {
  SurfaceFlags flags_to_set = SurfaceFlags::None;    ///< Flags to turn on
  SurfaceFlags flags_to_clear = SurfaceFlags::None;  ///< Flags to turn off
  bool replace_all = false;                           ///< Replace all flags instead of modifying
};

/// Result of a surface flags operation.
struct SurfaceFlagsResult {
  bool success = false;
  size_t faces_modified = 0;
  std::string error_message;

  [[nodiscard]] static SurfaceFlagsResult Success(size_t count) {
    SurfaceFlagsResult result;
    result.success = true;
    result.faces_modified = count;
    return result;
  }

  [[nodiscard]] static SurfaceFlagsResult Failure(std::string message) {
    SurfaceFlagsResult result;
    result.success = false;
    result.error_message = std::move(message);
    return result;
  }
};

/// Options for alpha reference operations.
struct AlphaRefOptions {
  uint8_t alpha_ref = 128;   ///< Alpha reference value (0-255)
  bool enable = true;        ///< Enable alpha testing (ref > 0)
};

/// Apply flag modifications to a single polygon.
/// @param polygon Polygon to modify.
/// @param options Flag modification options.
void ApplyFlagsToPolygon(csg::CSGPolygon& polygon, const SetFlagsOptions& options);

/// Set surface flags on specific faces.
/// @param polygons Vector of polygons to modify.
/// @param face_indices Indices of faces to modify.
/// @param options Flag modification options.
/// @return Number of faces modified.
size_t SetFlagsOnFaces(std::vector<csg::CSGPolygon>& polygons,
                        const std::vector<uint32_t>& face_indices,
                        const SetFlagsOptions& options);

/// Set surface flags on all polygons.
/// @param polygons Vector of polygons to modify.
/// @param options Flag modification options.
/// @return Number of faces modified.
size_t SetFlagsOnAllFaces(std::vector<csg::CSGPolygon>& polygons,
                           const SetFlagsOptions& options);

/// Set a specific flag on faces.
/// @param polygons Vector of polygons to modify.
/// @param face_indices Indices of faces to modify.
/// @param flag The flag to set.
/// @return Number of faces modified.
size_t SetFlagOnFaces(std::vector<csg::CSGPolygon>& polygons,
                       const std::vector<uint32_t>& face_indices,
                       SurfaceFlags flag);

/// Clear a specific flag on faces.
/// @param polygons Vector of polygons to modify.
/// @param face_indices Indices of faces to modify.
/// @param flag The flag to clear.
/// @return Number of faces modified.
size_t ClearFlagOnFaces(std::vector<csg::CSGPolygon>& polygons,
                         const std::vector<uint32_t>& face_indices,
                         SurfaceFlags flag);

/// Toggle a specific flag on faces.
/// @param polygons Vector of polygons to modify.
/// @param face_indices Indices of faces to modify.
/// @param flag The flag to toggle.
/// @return Number of faces modified.
size_t ToggleFlagOnFaces(std::vector<csg::CSGPolygon>& polygons,
                          const std::vector<uint32_t>& face_indices,
                          SurfaceFlags flag);

/// Set alpha reference value on specific faces.
/// @param polygons Vector of polygons to modify.
/// @param face_indices Indices of faces to modify.
/// @param alpha_ref Alpha reference value (0 = disable, 1-255 = threshold).
/// @return Number of faces modified.
size_t SetAlphaRefOnFaces(std::vector<csg::CSGPolygon>& polygons,
                           const std::vector<uint32_t>& face_indices,
                           uint8_t alpha_ref);

/// Set alpha reference value on all polygons.
/// @param polygons Vector of polygons to modify.
/// @param alpha_ref Alpha reference value.
/// @return Number of faces modified.
size_t SetAlphaRefOnAllFaces(std::vector<csg::CSGPolygon>& polygons,
                              uint8_t alpha_ref);

/// Get common flags across specified faces.
/// Returns flags that are set on ALL specified faces.
/// @param polygons Vector of polygons to check.
/// @param face_indices Indices of faces to check.
/// @return Flags that are common to all specified faces.
[[nodiscard]] SurfaceFlags GetCommonFlags(const std::vector<csg::CSGPolygon>& polygons,
                                           const std::vector<uint32_t>& face_indices);

/// Get union of flags across specified faces.
/// Returns any flag that is set on at least one face.
/// @param polygons Vector of polygons to check.
/// @param face_indices Indices of faces to check.
/// @return Union of all flags on specified faces.
[[nodiscard]] SurfaceFlags GetUnionFlags(const std::vector<csg::CSGPolygon>& polygons,
                                          const std::vector<uint32_t>& face_indices);

/// Check if all specified faces have a flag set.
/// @param polygons Vector of polygons to check.
/// @param face_indices Indices of faces to check.
/// @param flag The flag to check.
/// @return true if all faces have the flag set.
[[nodiscard]] bool AllFacesHaveFlag(const std::vector<csg::CSGPolygon>& polygons,
                                     const std::vector<uint32_t>& face_indices,
                                     SurfaceFlags flag);

/// Check if any specified face has a flag set.
/// @param polygons Vector of polygons to check.
/// @param face_indices Indices of faces to check.
/// @param flag The flag to check.
/// @return true if any face has the flag set.
[[nodiscard]] bool AnyFaceHasFlag(const std::vector<csg::CSGPolygon>& polygons,
                                   const std::vector<uint32_t>& face_indices,
                                   SurfaceFlags flag);

/// Find faces that have a specific flag set.
/// @param polygons Vector of polygons to search.
/// @param flag The flag to search for.
/// @return Vector of face indices that have the flag set.
[[nodiscard]] std::vector<uint32_t> FindFacesWithFlag(
    const std::vector<csg::CSGPolygon>& polygons,
    SurfaceFlags flag);

/// Find faces with alpha testing enabled.
/// @param polygons Vector of polygons to search.
/// @return Vector of face indices with alpha_ref > 0.
[[nodiscard]] std::vector<uint32_t> FindFacesWithAlphaTest(
    const std::vector<csg::CSGPolygon>& polygons);

/// Get the common alpha ref value across faces, or -1 if mixed.
/// @param polygons Vector of polygons to check.
/// @param face_indices Indices of faces to check.
/// @return Common alpha ref value, or -1 if values differ.
[[nodiscard]] int GetCommonAlphaRef(const std::vector<csg::CSGPolygon>& polygons,
                                     const std::vector<uint32_t>& face_indices);

/// Convert surface flags to a human-readable string.
/// @param flags The flags to describe.
/// @return Comma-separated list of flag names.
[[nodiscard]] std::string FlagsToString(SurfaceFlags flags);

/// Parse surface flags from a string.
/// @param str Comma-separated list of flag names.
/// @return Parsed flags, or SurfaceFlags::None if invalid.
[[nodiscard]] SurfaceFlags StringToFlags(std::string_view str);

} // namespace texture_ops
