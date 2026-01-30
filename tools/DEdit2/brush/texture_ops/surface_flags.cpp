#include "surface_flags.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace texture_ops {

namespace {

/// Trim whitespace from both ends of a string view.
std::string_view Trim(std::string_view str) {
  while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())) != 0) {
    str.remove_prefix(1);
  }
  while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())) != 0) {
    str.remove_suffix(1);
  }
  return str;
}

/// Convert string to lowercase for comparison.
std::string ToLower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

} // namespace

void ApplyFlagsToPolygon(csg::CSGPolygon& polygon, const SetFlagsOptions& options) {
  if (options.replace_all) {
    polygon.face_props.flags = options.flags_to_set;
  } else {
    // Set specified flags
    polygon.face_props.flags |= options.flags_to_set;
    // Clear specified flags
    polygon.face_props.flags &= ~options.flags_to_clear;
  }
}

size_t SetFlagsOnFaces(std::vector<csg::CSGPolygon>& polygons,
                        const std::vector<uint32_t>& face_indices,
                        const SetFlagsOptions& options) {
  size_t modified = 0;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      ApplyFlagsToPolygon(polygons[idx], options);
      ++modified;
    }
  }

  return modified;
}

size_t SetFlagsOnAllFaces(std::vector<csg::CSGPolygon>& polygons,
                           const SetFlagsOptions& options) {
  for (auto& polygon : polygons) {
    ApplyFlagsToPolygon(polygon, options);
  }
  return polygons.size();
}

size_t SetFlagOnFaces(std::vector<csg::CSGPolygon>& polygons,
                       const std::vector<uint32_t>& face_indices,
                       SurfaceFlags flag) {
  size_t modified = 0;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      polygons[idx].face_props.flags |= flag;
      ++modified;
    }
  }

  return modified;
}

size_t ClearFlagOnFaces(std::vector<csg::CSGPolygon>& polygons,
                         const std::vector<uint32_t>& face_indices,
                         SurfaceFlags flag) {
  size_t modified = 0;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      polygons[idx].face_props.flags &= ~flag;
      ++modified;
    }
  }

  return modified;
}

size_t ToggleFlagOnFaces(std::vector<csg::CSGPolygon>& polygons,
                          const std::vector<uint32_t>& face_indices,
                          SurfaceFlags flag) {
  size_t modified = 0;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      polygons[idx].face_props.flags ^= flag;
      ++modified;
    }
  }

  return modified;
}

size_t SetAlphaRefOnFaces(std::vector<csg::CSGPolygon>& polygons,
                           const std::vector<uint32_t>& face_indices,
                           uint8_t alpha_ref) {
  size_t modified = 0;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      polygons[idx].face_props.alpha_ref = alpha_ref;
      ++modified;
    }
  }

  return modified;
}

size_t SetAlphaRefOnAllFaces(std::vector<csg::CSGPolygon>& polygons,
                              uint8_t alpha_ref) {
  for (auto& polygon : polygons) {
    polygon.face_props.alpha_ref = alpha_ref;
  }
  return polygons.size();
}

SurfaceFlags GetCommonFlags(const std::vector<csg::CSGPolygon>& polygons,
                             const std::vector<uint32_t>& face_indices) {
  if (face_indices.empty()) {
    return SurfaceFlags::None;
  }

  // Start with all flags set
  SurfaceFlags common = static_cast<SurfaceFlags>(~0u);

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      common &= polygons[idx].face_props.flags;
    }
  }

  return common;
}

SurfaceFlags GetUnionFlags(const std::vector<csg::CSGPolygon>& polygons,
                            const std::vector<uint32_t>& face_indices) {
  SurfaceFlags result = SurfaceFlags::None;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      result |= polygons[idx].face_props.flags;
    }
  }

  return result;
}

bool AllFacesHaveFlag(const std::vector<csg::CSGPolygon>& polygons,
                       const std::vector<uint32_t>& face_indices,
                       SurfaceFlags flag) {
  if (face_indices.empty()) {
    return false;
  }

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      if (!HasFlag(polygons[idx].face_props.flags, flag)) {
        return false;
      }
    }
  }

  return true;
}

bool AnyFaceHasFlag(const std::vector<csg::CSGPolygon>& polygons,
                     const std::vector<uint32_t>& face_indices,
                     SurfaceFlags flag) {
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      if (HasFlag(polygons[idx].face_props.flags, flag)) {
        return true;
      }
    }
  }

  return false;
}

std::vector<uint32_t> FindFacesWithFlag(const std::vector<csg::CSGPolygon>& polygons,
                                         SurfaceFlags flag) {
  std::vector<uint32_t> result;

  for (size_t i = 0; i < polygons.size(); ++i) {
    if (HasFlag(polygons[i].face_props.flags, flag)) {
      result.push_back(static_cast<uint32_t>(i));
    }
  }

  return result;
}

std::vector<uint32_t> FindFacesWithAlphaTest(const std::vector<csg::CSGPolygon>& polygons) {
  std::vector<uint32_t> result;

  for (size_t i = 0; i < polygons.size(); ++i) {
    if (polygons[i].face_props.alpha_ref > 0) {
      result.push_back(static_cast<uint32_t>(i));
    }
  }

  return result;
}

int GetCommonAlphaRef(const std::vector<csg::CSGPolygon>& polygons,
                       const std::vector<uint32_t>& face_indices) {
  if (face_indices.empty()) {
    return -1;
  }

  int common = -1;

  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      int alpha = polygons[idx].face_props.alpha_ref;
      if (common == -1) {
        common = alpha;
      } else if (common != alpha) {
        return -1; // Mixed values
      }
    }
  }

  return common;
}

std::string FlagsToString(SurfaceFlags flags) {
  if (flags == SurfaceFlags::None) {
    return "None";
  }

  std::ostringstream oss;
  bool first = true;

  auto append = [&](const char* name) {
    if (!first) {
      oss << ", ";
    }
    oss << name;
    first = false;
  };

  if (HasFlag(flags, SurfaceFlags::Solid)) {
    append("Solid");
  }
  if (HasFlag(flags, SurfaceFlags::Invisible)) {
    append("Invisible");
  }
  if (HasFlag(flags, SurfaceFlags::Transparent)) {
    append("Transparent");
  }
  if (HasFlag(flags, SurfaceFlags::Sky)) {
    append("Sky");
  }
  if (HasFlag(flags, SurfaceFlags::Fullbright)) {
    append("Fullbright");
  }
  if (HasFlag(flags, SurfaceFlags::FlatShade)) {
    append("FlatShade");
  }
  if (HasFlag(flags, SurfaceFlags::Lightmap)) {
    append("Lightmap");
  }
  if (HasFlag(flags, SurfaceFlags::Portal)) {
    append("Portal");
  }
  if (HasFlag(flags, SurfaceFlags::Mirror)) {
    append("Mirror");
  }

  return oss.str();
}

SurfaceFlags StringToFlags(std::string_view str) {
  SurfaceFlags result = SurfaceFlags::None;

  size_t start = 0;
  while (start < str.size()) {
    size_t end = str.find(',', start);
    if (end == std::string_view::npos) {
      end = str.size();
    }

    std::string_view token = Trim(str.substr(start, end - start));
    std::string lower = ToLower(token);

    if (lower == "solid") {
      result |= SurfaceFlags::Solid;
    } else if (lower == "invisible" || lower == "nodraw") {
      result |= SurfaceFlags::Invisible;
    } else if (lower == "transparent") {
      result |= SurfaceFlags::Transparent;
    } else if (lower == "sky") {
      result |= SurfaceFlags::Sky;
    } else if (lower == "fullbright") {
      result |= SurfaceFlags::Fullbright;
    } else if (lower == "flatshade") {
      result |= SurfaceFlags::FlatShade;
    } else if (lower == "lightmap") {
      result |= SurfaceFlags::Lightmap;
    } else if (lower == "portal") {
      result |= SurfaceFlags::Portal;
    } else if (lower == "mirror") {
      result |= SurfaceFlags::Mirror;
    }

    start = end + 1;
  }

  return result;
}

} // namespace texture_ops
