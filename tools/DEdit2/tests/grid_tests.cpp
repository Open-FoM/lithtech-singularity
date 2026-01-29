#include "grid/grid_settings.h"

#include <gtest/gtest.h>

// -----------------------------------------------------------------------------
// HalveSpacing tests
// -----------------------------------------------------------------------------

TEST(GridSettings, HalveSpacing_NormalValue) {
  EXPECT_FLOAT_EQ(grid::HalveSpacing(64.0f), 32.0f);
  EXPECT_FLOAT_EQ(grid::HalveSpacing(32.0f), 16.0f);
  EXPECT_FLOAT_EQ(grid::HalveSpacing(16.0f), 8.0f);
}

TEST(GridSettings, HalveSpacing_ClampsToMinimum) {
  // Values at or below minimum should clamp
  EXPECT_FLOAT_EQ(grid::HalveSpacing(1.0f), grid::kMinSpacing);
  EXPECT_FLOAT_EQ(grid::HalveSpacing(2.0f), 1.0f);

  // Edge case: halving 1.5 would give 0.75, but should clamp to 1.0
  EXPECT_FLOAT_EQ(grid::HalveSpacing(1.5f), grid::kMinSpacing);
}

// -----------------------------------------------------------------------------
// DoubleSpacing tests
// -----------------------------------------------------------------------------

TEST(GridSettings, DoubleSpacing_NormalValue) {
  EXPECT_FLOAT_EQ(grid::DoubleSpacing(64.0f), 128.0f);
  EXPECT_FLOAT_EQ(grid::DoubleSpacing(32.0f), 64.0f);
  EXPECT_FLOAT_EQ(grid::DoubleSpacing(16.0f), 32.0f);
}

TEST(GridSettings, DoubleSpacing_ClampsToMaximum) {
  // Values at or above maximum should clamp
  EXPECT_FLOAT_EQ(grid::DoubleSpacing(256.0f), grid::kMaxSpacing);
  EXPECT_FLOAT_EQ(grid::DoubleSpacing(128.0f), 256.0f);

  // Edge case: doubling 200 would give 400, but should clamp to 256
  EXPECT_FLOAT_EQ(grid::DoubleSpacing(200.0f), grid::kMaxSpacing);
}

// -----------------------------------------------------------------------------
// Round-trip tests
// -----------------------------------------------------------------------------

TEST(GridSettings, HalveDoubleRoundTrip) {
  // Halving then doubling (or vice versa) should return to original
  // when not at bounds
  const float original = 64.0f;
  EXPECT_FLOAT_EQ(grid::DoubleSpacing(grid::HalveSpacing(original)), original);
  EXPECT_FLOAT_EQ(grid::HalveSpacing(grid::DoubleSpacing(original)), original);
}

TEST(GridSettings, StandardSpacingSequence) {
  // Verify the standard power-of-2 sequence: 1 -> 2 -> 4 -> 8 -> 16 -> 32 -> 64 -> 128 -> 256
  float spacing = 1.0f;
  EXPECT_FLOAT_EQ(spacing, 1.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 2.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 4.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 8.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 16.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 32.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 64.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 128.0f);

  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 256.0f);

  // At max, doubling should stay at 256
  spacing = grid::DoubleSpacing(spacing);
  EXPECT_FLOAT_EQ(spacing, 256.0f);
}

// -----------------------------------------------------------------------------
// AdaptiveSpacing tests
// -----------------------------------------------------------------------------

TEST(GridSettings, AdaptiveSpacing_ZoomedIn) {
  // When zoomed in, spacing should stay at base value
  // ortho_zoom = 0.5 means we're zoomed in (half the visible area)
  // viewport_height = 800 pixels
  // visible_height = 0.5 * 800 = 400 world units
  // pixels_per_unit = 800 / 400 = 2
  // pixel_spacing = 16 * 2 = 32 pixels (> 10, no adjustment needed)
  const float result = grid::ComputeAdaptiveSpacing(16.0f, 0.5f, 800.0f);
  EXPECT_FLOAT_EQ(result, 16.0f);
}

TEST(GridSettings, AdaptiveSpacing_ZoomedOut) {
  // When zoomed out, spacing should double to maintain visibility
  // ortho_zoom = 4.0 means we're zoomed out (4x the visible area)
  // viewport_height = 800 pixels
  // visible_height = 4.0 * 800 = 3200 world units
  // pixels_per_unit = 800 / 3200 = 0.25
  // pixel_spacing = 16 * 0.25 = 4 pixels (< 10, need to double)
  // After doubling: 32 * 0.25 = 8 pixels (< 10, need to double again)
  // After doubling: 64 * 0.25 = 16 pixels (> 10, stop)
  const float result = grid::ComputeAdaptiveSpacing(16.0f, 4.0f, 800.0f);
  EXPECT_FLOAT_EQ(result, 64.0f);
}

TEST(GridSettings, AdaptiveSpacing_ClampsToMaximum) {
  // Even when extremely zoomed out, should not exceed kMaxSpacing
  const float result = grid::ComputeAdaptiveSpacing(16.0f, 100.0f, 800.0f);
  EXPECT_LE(result, grid::kMaxSpacing);
}

// -----------------------------------------------------------------------------
// Validation and clamping tests
// -----------------------------------------------------------------------------

TEST(GridSettings, IsValidSpacing_ValidRange) {
  EXPECT_TRUE(grid::IsValidSpacing(1.0f));
  EXPECT_TRUE(grid::IsValidSpacing(64.0f));
  EXPECT_TRUE(grid::IsValidSpacing(256.0f));
}

TEST(GridSettings, IsValidSpacing_InvalidRange) {
  EXPECT_FALSE(grid::IsValidSpacing(0.5f));
  EXPECT_FALSE(grid::IsValidSpacing(0.0f));
  EXPECT_FALSE(grid::IsValidSpacing(-1.0f));
  EXPECT_FALSE(grid::IsValidSpacing(257.0f));
  EXPECT_FALSE(grid::IsValidSpacing(1000.0f));
}

TEST(GridSettings, ClampSpacing_ClampsToRange) {
  EXPECT_FLOAT_EQ(grid::ClampSpacing(0.5f), grid::kMinSpacing);
  EXPECT_FLOAT_EQ(grid::ClampSpacing(500.0f), grid::kMaxSpacing);
  EXPECT_FLOAT_EQ(grid::ClampSpacing(64.0f), 64.0f);
}
