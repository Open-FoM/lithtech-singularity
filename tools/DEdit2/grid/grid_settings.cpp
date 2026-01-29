#include "grid_settings.h"

#include <algorithm>
#include <cmath>

namespace grid {

float HalveSpacing(float current) {
  return std::max(current / 2.0f, kMinSpacing);
}

float DoubleSpacing(float current) {
  return std::min(current * 2.0f, kMaxSpacing);
}

float ComputeAdaptiveSpacing(float base_spacing, float ortho_zoom, float viewport_height,
                             float min_pixel_spacing) {
  // Calculate how many world units are visible in the viewport height
  // ortho_zoom controls the visible half-height: visible_half_height = ortho_zoom * 400.0f
  // (This matches the calculation in ComputeViewportViewProj)
  const float visible_height = ortho_zoom * 800.0f;

  // Calculate pixels per world unit
  const float pixels_per_unit = viewport_height / visible_height;

  // Calculate current pixel spacing
  float spacing = base_spacing;
  float pixel_spacing = spacing * pixels_per_unit;

  // Double spacing until lines are far enough apart
  while (pixel_spacing < min_pixel_spacing && spacing < kMaxSpacing) {
    spacing *= 2.0f;
    pixel_spacing = spacing * pixels_per_unit;
  }

  return std::min(spacing, kMaxSpacing);
}

float ClampSpacing(float spacing) {
  return std::clamp(spacing, kMinSpacing, kMaxSpacing);
}

float SnapValue(float value, float step) {
  if (step <= 0.0f) {
    return value;
  }
  return std::round(value / step) * step;
}

} // namespace grid
