#include "brush/csg/csg_carve.h"

#include <gtest/gtest.h>

namespace csg {

namespace {

// Helper to create a simple box brush
CSGBrush MakeBoxBrush(float cx, float cy, float cz, float size) {
  float half = size / 2.0f;
  CSGBrush brush;

  // Front face
  CSGPolygon front;
  front.vertices = {CSGVertex(cx - half, cy - half, cz + half), CSGVertex(cx + half, cy - half, cz + half),
                    CSGVertex(cx + half, cy + half, cz + half), CSGVertex(cx - half, cy + half, cz + half)};
  front.ComputePlane();
  brush.polygons.push_back(front);

  // Back face
  CSGPolygon back;
  back.vertices = {CSGVertex(cx + half, cy - half, cz - half), CSGVertex(cx - half, cy - half, cz - half),
                   CSGVertex(cx - half, cy + half, cz - half), CSGVertex(cx + half, cy + half, cz - half)};
  back.ComputePlane();
  brush.polygons.push_back(back);

  // Left face
  CSGPolygon left;
  left.vertices = {CSGVertex(cx - half, cy - half, cz - half), CSGVertex(cx - half, cy - half, cz + half),
                   CSGVertex(cx - half, cy + half, cz + half), CSGVertex(cx - half, cy + half, cz - half)};
  left.ComputePlane();
  brush.polygons.push_back(left);

  // Right face
  CSGPolygon right;
  right.vertices = {CSGVertex(cx + half, cy - half, cz + half), CSGVertex(cx + half, cy - half, cz - half),
                    CSGVertex(cx + half, cy + half, cz - half), CSGVertex(cx + half, cy + half, cz + half)};
  right.ComputePlane();
  brush.polygons.push_back(right);

  // Top face
  CSGPolygon top;
  top.vertices = {CSGVertex(cx - half, cy + half, cz + half), CSGVertex(cx + half, cy + half, cz + half),
                  CSGVertex(cx + half, cy + half, cz - half), CSGVertex(cx - half, cy + half, cz - half)};
  top.ComputePlane();
  brush.polygons.push_back(top);

  // Bottom face
  CSGPolygon bottom;
  bottom.vertices = {CSGVertex(cx - half, cy - half, cz - half), CSGVertex(cx + half, cy - half, cz - half),
                     CSGVertex(cx + half, cy - half, cz + half), CSGVertex(cx - half, cy - half, cz + half)};
  bottom.ComputePlane();
  brush.polygons.push_back(bottom);

  return brush;
}

} // namespace

// =============================================================================
// Intersection Tests
// =============================================================================

TEST(CSGCarve, BrushesIntersect_Overlapping) {
  CSGBrush a = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
  CSGBrush b = MakeBoxBrush(32.0f, 0.0f, 0.0f, 64.0f);

  EXPECT_TRUE(BrushesIntersect(a, b));
}

TEST(CSGCarve, BrushesIntersect_Touching) {
  CSGBrush a = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
  CSGBrush b = MakeBoxBrush(64.0f, 0.0f, 0.0f, 64.0f);

  EXPECT_TRUE(BrushesIntersect(a, b)); // Edges touch
}

TEST(CSGCarve, BrushesIntersect_Disjoint) {
  CSGBrush a = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
  CSGBrush b = MakeBoxBrush(200.0f, 0.0f, 0.0f, 64.0f);

  EXPECT_FALSE(BrushesIntersect(a, b));
}

// =============================================================================
// Basic Carve Tests
// =============================================================================

TEST(CSGCarve, CarveBrush_NoIntersection) {
  CSGBrush target = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
  CSGBrush cutter = MakeBoxBrush(200.0f, 0.0f, 0.0f, 32.0f);

  auto result = CarveBrush(target, cutter);

  // No intersection - should return original brush unchanged
  EXPECT_EQ(result.size(), 1u);
}

TEST(CSGCarve, CarveBrush_PartialOverlap) {
  CSGBrush target = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
  CSGBrush cutter = MakeBoxBrush(32.0f, 0.0f, 0.0f, 32.0f);

  auto result = CarveBrush(target, cutter);

  // Should produce multiple fragments
  EXPECT_GT(result.size(), 0u);
}

TEST(CSGCarve, CarveBrush_CutterInsideTarget) {
  CSGBrush target = MakeBoxBrush(0.0f, 0.0f, 0.0f, 128.0f);
  CSGBrush cutter = MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f);

  auto result = CarveBrush(target, cutter);

  // Should produce fragments around the carved hole
  EXPECT_GT(result.size(), 0u);
}

TEST(CSGCarve, CarveBrush_TargetInsideCutter) {
  CSGBrush target = MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f);
  CSGBrush cutter = MakeBoxBrush(0.0f, 0.0f, 0.0f, 128.0f);

  auto result = CarveBrush(target, cutter);

  // Target is entirely inside cutter - should be removed
  EXPECT_TRUE(result.empty());
}

// =============================================================================
// Multiple Target Tests
// =============================================================================

TEST(CSGCarve, CarveBrushes_MultipleTargets) {
  std::vector<CSGBrush> targets;
  targets.push_back(MakeBoxBrush(-48.0f, 0.0f, 0.0f, 32.0f));
  targets.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f));
  targets.push_back(MakeBoxBrush(48.0f, 0.0f, 0.0f, 32.0f));

  CSGBrush cutter = MakeBoxBrush(0.0f, 0.0f, 0.0f, 48.0f);

  CarveResult result = CarveBrushes(targets, cutter);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.affected_count, 0u);
}

TEST(CSGCarve, CarveBrushes_NoAffected) {
  std::vector<CSGBrush> targets;
  targets.push_back(MakeBoxBrush(200.0f, 0.0f, 0.0f, 32.0f));

  CSGBrush cutter = MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f);

  CarveResult result = CarveBrushes(targets, cutter);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.affected_count, 0u);
  EXPECT_EQ(result.carved_brushes.size(), 1u); // Original unchanged
}

// =============================================================================
// Triangle Mesh API Tests
// =============================================================================

TEST(CSGCarve, CarveBrush_TriangleMeshAPI) {
  // Target box
  std::vector<float> target_vertices;
  std::vector<uint32_t> target_indices;
  {
    CSGBrush brush = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
    brush.ToTriangleMesh(target_vertices, target_indices);
  }

  // Cutter box
  std::vector<float> cutter_vertices;
  std::vector<uint32_t> cutter_indices;
  {
    CSGBrush brush = MakeBoxBrush(32.0f, 0.0f, 0.0f, 32.0f);
    brush.ToTriangleMesh(cutter_vertices, cutter_indices);
  }

  CSGOperationResult result = CarveBrush(target_vertices, target_indices, cutter_vertices, cutter_indices);

  EXPECT_TRUE(result.success);
  EXPECT_FALSE(result.results.empty());
}

TEST(CSGCarve, CarveBrush_InvalidInput) {
  std::vector<float> target_vertices;
  std::vector<uint32_t> target_indices;
  std::vector<float> cutter_vertices;
  std::vector<uint32_t> cutter_indices;

  CSGOperationResult result = CarveBrush(target_vertices, target_indices, cutter_vertices, cutter_indices);

  EXPECT_FALSE(result.success);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(CSGCarve, CarveBrushes_EmptyTargets) {
  std::vector<CSGBrush> targets;
  CSGBrush cutter = MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f);

  CarveResult result = CarveBrushes(targets, cutter);

  EXPECT_FALSE(result.success);
}

TEST(CSGCarve, CarveBrushes_InvalidCutter) {
  std::vector<CSGBrush> targets;
  targets.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f));

  CSGBrush cutter; // Empty/invalid

  CarveResult result = CarveBrushes(targets, cutter);

  EXPECT_FALSE(result.success);
}

TEST(CSGCarve, CarveBrush_SameBrush) {
  CSGBrush target = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f);
  CSGBrush cutter = MakeBoxBrush(0.0f, 0.0f, 0.0f, 64.0f); // Same as target

  auto result = CarveBrush(target, cutter);

  // Should be entirely carved away
  EXPECT_TRUE(result.empty());
}

// =============================================================================
// Options Tests
// =============================================================================

TEST(CSGCarve, CarveBrushes_KeepEmptyResults) {
  std::vector<CSGBrush> targets;
  targets.push_back(MakeBoxBrush(0.0f, 0.0f, 0.0f, 32.0f));

  CSGBrush cutter = MakeBoxBrush(0.0f, 0.0f, 0.0f, 128.0f);

  CarveOptions options;
  options.keep_empty_results = true;

  CarveResult result = CarveBrushes(targets, cutter, options);

  EXPECT_TRUE(result.success);
  // Target was entirely carved away
  EXPECT_EQ(result.affected_count, 1u);
}

} // namespace csg
