#include "uv_fit.h"
#include "texture_applicator.h"

#include <algorithm>
#include <limits>

namespace texture_ops {

void ComputeUVBounds(const std::vector<UV>& uvs, UV& out_min, UV& out_max) {
  if (uvs.empty()) {
    out_min = UV(0.0f, 0.0f);
    out_max = UV(0.0f, 0.0f);
    return;
  }

  out_min = UV(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  out_max = UV(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

  for (const auto& uv : uvs) {
    out_min.u = std::min(out_min.u, uv.u);
    out_min.v = std::min(out_min.v, uv.v);
    out_max.u = std::max(out_max.u, uv.u);
    out_max.v = std::max(out_max.v, uv.v);
  }
}

void NormalizePolygonUVs(csg::CSGPolygon& polygon) {
  if (polygon.uvs.empty()) {
    polygon.EnsureUVSize();
    // Generate default UVs if none exist
    polygon.uvs = GeneratePlanarUVs(polygon, 0.0625f);
  }

  UV min_uv, max_uv;
  ComputeUVBounds(polygon.uvs, min_uv, max_uv);

  float range_u = max_uv.u - min_uv.u;
  float range_v = max_uv.v - min_uv.v;

  // Avoid division by zero
  if (range_u < Tolerance::kEpsilon) range_u = 1.0f;
  if (range_v < Tolerance::kEpsilon) range_v = 1.0f;

  for (auto& uv : polygon.uvs) {
    uv.u = (uv.u - min_uv.u) / range_u;
    uv.v = (uv.v - min_uv.v) / range_v;
  }
}

void FitPolygonUVs(csg::CSGPolygon& polygon, const FitTextureOptions& options) {
  if (polygon.uvs.empty()) {
    polygon.EnsureUVSize();
    polygon.uvs = GeneratePlanarUVs(polygon, 0.0625f);
  }

  UV min_uv, max_uv;
  ComputeUVBounds(polygon.uvs, min_uv, max_uv);

  float range_u = max_uv.u - min_uv.u;
  float range_v = max_uv.v - min_uv.v;

  if (range_u < Tolerance::kEpsilon) range_u = 1.0f;
  if (range_v < Tolerance::kEpsilon) range_v = 1.0f;

  // Available range after padding
  float available = 1.0f - 2.0f * options.padding;
  if (available < 0.1f) available = 0.1f;

  float scale_u = available / range_u;
  float scale_v = available / range_v;

  if (options.maintain_aspect) {
    float tex_aspect = static_cast<float>(options.texture_width) /
                       static_cast<float>(options.texture_height);
    float poly_aspect = range_u / range_v;

    // Adjust scale to maintain aspect ratio
    if (poly_aspect > tex_aspect) {
      // Polygon is wider than texture, constrain by U
      scale_v = scale_u * tex_aspect;
    } else {
      // Polygon is taller than texture, constrain by V
      scale_u = scale_v / tex_aspect;
    }
  }

  // Apply transformation
  for (auto& uv : polygon.uvs) {
    uv.u = (uv.u - min_uv.u) * scale_u;
    uv.v = (uv.v - min_uv.v) * scale_v;
  }

  if (options.center) {
    // Recompute bounds after scaling
    ComputeUVBounds(polygon.uvs, min_uv, max_uv);
    float center_u = (min_uv.u + max_uv.u) * 0.5f;
    float center_v = (min_uv.v + max_uv.v) * 0.5f;

    float offset_u = 0.5f - center_u;
    float offset_v = 0.5f - center_v;

    for (auto& uv : polygon.uvs) {
      uv.u += offset_u;
      uv.v += offset_v;
    }
  } else {
    // Just apply padding offset
    for (auto& uv : polygon.uvs) {
      uv.u += options.padding;
      uv.v += options.padding;
    }
  }
}

void FitTextureToPolygon(csg::CSGPolygon& polygon, const FitTextureOptions& options) {
  // First generate planar UVs based on face normal
  polygon.uvs = GeneratePlanarUVs(polygon, 1.0f);

  // Then fit them to the [0,1] range
  FitPolygonUVs(polygon, options);
}

size_t FitTextureToBrushFaces(std::vector<csg::CSGPolygon>& polygons,
                               const std::vector<uint32_t>& face_indices,
                               const FitTextureOptions& options) {
  size_t modified = 0;
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      FitTextureToPolygon(polygons[idx], options);
      ++modified;
    }
  }
  return modified;
}

size_t FitTextureToAllFaces(std::vector<csg::CSGPolygon>& polygons,
                             const FitTextureOptions& options) {
  for (auto& polygon : polygons) {
    FitTextureToPolygon(polygon, options);
  }
  return polygons.size();
}

} // namespace texture_ops
