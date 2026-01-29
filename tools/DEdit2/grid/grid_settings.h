#pragma once

/// Grid settings and utilities for DEdit2 viewport grid system.
///
/// Provides functions for manipulating grid spacing with power-of-2 increments,
/// and adaptive spacing calculations for zoom-based grid density control.

namespace grid {

/// Minimum allowed grid spacing in world units.
constexpr float kMinSpacing = 1.0f;

/// Maximum allowed grid spacing in world units.
constexpr float kMaxSpacing = 256.0f;

/// Default grid spacing in world units.
constexpr float kDefaultSpacing = 64.0f;

/// Halve the grid spacing, clamping to minimum.
/// @param current Current grid spacing value.
/// @return New spacing value (current / 2), clamped to kMinSpacing.
[[nodiscard]] float HalveSpacing(float current);

/// Double the grid spacing, clamping to maximum.
/// @param current Current grid spacing value.
/// @return New spacing value (current * 2), clamped to kMaxSpacing.
[[nodiscard]] float DoubleSpacing(float current);

/// Compute adaptive grid spacing based on zoom level.
///
/// When zoomed out, the grid spacing is doubled repeatedly until lines
/// are at least min_pixel_spacing pixels apart. This prevents visual clutter
/// from too-dense grid lines at far zoom levels.
///
/// @param base_spacing The configured base grid spacing in world units.
/// @param ortho_zoom The orthographic zoom factor (smaller = zoomed in).
/// @param viewport_height The viewport height in pixels.
/// @param min_pixel_spacing Minimum desired pixels between grid lines (default: 10).
/// @return Effective spacing to use for rendering.
[[nodiscard]] float ComputeAdaptiveSpacing(float base_spacing, float ortho_zoom, float viewport_height,
                                           float min_pixel_spacing = 10.0f);

/// Validate that a spacing value is within allowed range.
/// @param spacing The spacing value to validate.
/// @return true if spacing is within [kMinSpacing, kMaxSpacing].
[[nodiscard]] constexpr bool IsValidSpacing(float spacing) {
  return spacing >= kMinSpacing && spacing <= kMaxSpacing;
}

/// Clamp a spacing value to the allowed range.
/// @param spacing The spacing value to clamp.
/// @return Clamped spacing within [kMinSpacing, kMaxSpacing].
[[nodiscard]] float ClampSpacing(float spacing);

/// Snap a value to the nearest grid step.
/// @param value The value to snap.
/// @param step The grid step size. If <= 0, returns value unchanged.
/// @return The snapped value (nearest multiple of step).
[[nodiscard]] float SnapValue(float value, float step);

} // namespace grid
