#include "uv_transform.h"
#include "texture_applicator.h"

#include <cmath>

namespace texture_ops {

namespace {
constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
} // namespace

bool UVTransform::IsIdentity() const {
  return std::abs(offset.u) < Tolerance::kEpsilon &&
         std::abs(offset.v) < Tolerance::kEpsilon &&
         std::abs(scale.u - 1.0f) < Tolerance::kEpsilon &&
         std::abs(scale.v - 1.0f) < Tolerance::kEpsilon &&
         std::abs(rotation) < Tolerance::kEpsilon;
}

UV UVTransform::Apply(const UV& uv) const {
  // Translate to pivot
  float u = uv.u - pivot.u;
  float v = uv.v - pivot.v;

  // Scale
  u *= scale.u;
  v *= scale.v;

  // Rotate
  if (std::abs(rotation) > Tolerance::kEpsilon) {
    float rad = rotation * kDegToRad;
    float cos_r = std::cos(rad);
    float sin_r = std::sin(rad);
    float new_u = u * cos_r - v * sin_r;
    float new_v = u * sin_r + v * cos_r;
    u = new_u;
    v = new_v;
  }

  // Translate back from pivot and apply offset
  return UV(u + pivot.u + offset.u, v + pivot.v + offset.v);
}

void TransformPolygonUVs(csg::CSGPolygon& polygon, const UVTransform& transform) {
  if (transform.IsIdentity()) {
    return;
  }

  polygon.EnsureUVSize();
  for (auto& uv : polygon.uvs) {
    uv = transform.Apply(uv);
  }
}

size_t TransformBrushFaceUVs(std::vector<csg::CSGPolygon>& polygons,
                              const std::vector<uint32_t>& face_indices,
                              const UVTransform& transform) {
  if (transform.IsIdentity()) {
    return 0;
  }

  size_t modified = 0;
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      TransformPolygonUVs(polygons[idx], transform);
      ++modified;
    }
  }
  return modified;
}

size_t TransformAllBrushUVs(std::vector<csg::CSGPolygon>& polygons,
                             const UVTransform& transform) {
  if (transform.IsIdentity()) {
    return 0;
  }

  for (auto& polygon : polygons) {
    TransformPolygonUVs(polygon, transform);
  }
  return polygons.size();
}

size_t ResetFaceUVs(std::vector<csg::CSGPolygon>& polygons,
                     const std::vector<uint32_t>& face_indices,
                     float scale) {
  size_t modified = 0;
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      polygons[idx].uvs = GeneratePlanarUVs(polygons[idx], scale);
      ++modified;
    }
  }
  return modified;
}

UV FlipUVHorizontal(const UV& uv, float pivot_u) {
  return UV(2.0f * pivot_u - uv.u, uv.v);
}

UV FlipUVVertical(const UV& uv, float pivot_v) {
  return UV(uv.u, 2.0f * pivot_v - uv.v);
}

size_t FlipUVsHorizontal(std::vector<csg::CSGPolygon>& polygons,
                          const std::vector<uint32_t>& face_indices) {
  size_t modified = 0;
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      auto& polygon = polygons[idx];
      polygon.EnsureUVSize();

      // Compute centroid U for pivot
      float pivot_u = 0.0f;
      for (const auto& uv : polygon.uvs) {
        pivot_u += uv.u;
      }
      pivot_u /= static_cast<float>(polygon.uvs.size());

      for (auto& uv : polygon.uvs) {
        uv = FlipUVHorizontal(uv, pivot_u);
      }
      ++modified;
    }
  }
  return modified;
}

size_t FlipUVsVertical(std::vector<csg::CSGPolygon>& polygons,
                        const std::vector<uint32_t>& face_indices) {
  size_t modified = 0;
  for (uint32_t idx : face_indices) {
    if (idx < polygons.size()) {
      auto& polygon = polygons[idx];
      polygon.EnsureUVSize();

      // Compute centroid V for pivot
      float pivot_v = 0.0f;
      for (const auto& uv : polygon.uvs) {
        pivot_v += uv.v;
      }
      pivot_v /= static_cast<float>(polygon.uvs.size());

      for (auto& uv : polygon.uvs) {
        uv = FlipUVVertical(uv, pivot_v);
      }
      ++modified;
    }
  }
  return modified;
}

void RotatePolygonUVs(csg::CSGPolygon& polygon, float degrees) {
  if (std::abs(degrees) < Tolerance::kEpsilon) {
    return;
  }

  polygon.EnsureUVSize();

  // Compute centroid for pivot
  UV centroid = polygon.UVCentroid();

  float rad = degrees * kDegToRad;
  float cos_r = std::cos(rad);
  float sin_r = std::sin(rad);

  for (auto& uv : polygon.uvs) {
    float u = uv.u - centroid.u;
    float v = uv.v - centroid.v;
    uv.u = u * cos_r - v * sin_r + centroid.u;
    uv.v = u * sin_r + v * cos_r + centroid.v;
  }
}

} // namespace texture_ops
