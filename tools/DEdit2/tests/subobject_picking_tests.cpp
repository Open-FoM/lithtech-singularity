#include "geometry/subobject_picking.h"

#include "editor_state.h"

#include <gtest/gtest.h>

#include <cmath>

namespace {

/// Helper to create a simple triangle brush (3 vertices, 1 triangle).
NodeProperties CreateTriangleBrush(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2,
                                   float z2) {
  NodeProperties props;
  props.brush_vertices = {x0, y0, z0, x1, y1, z1, x2, y2, z2};
  props.brush_indices = {0, 1, 2};
  return props;
}

/// Helper to create a unit cube brush (8 vertices, 12 triangles).
/// Center at origin, extends from -0.5 to +0.5 on each axis.
NodeProperties CreateUnitCubeBrush() {
  NodeProperties props;
  // 8 vertices of the cube
  props.brush_vertices = {
      -0.5f, -0.5f, -0.5f, // 0: left-bottom-back
      0.5f,  -0.5f, -0.5f, // 1: right-bottom-back
      0.5f,  0.5f,  -0.5f, // 2: right-top-back
      -0.5f, 0.5f,  -0.5f, // 3: left-top-back
      -0.5f, -0.5f, 0.5f,  // 4: left-bottom-front
      0.5f,  -0.5f, 0.5f,  // 5: right-bottom-front
      0.5f,  0.5f,  0.5f,  // 6: right-top-front
      -0.5f, 0.5f,  0.5f,  // 7: left-top-front
  };

  // 12 triangles (2 per face)
  props.brush_indices = {
      // Front face (+Z)
      4, 5, 6, 4, 6, 7,
      // Back face (-Z)
      1, 0, 3, 1, 3, 2,
      // Right face (+X)
      5, 1, 2, 5, 2, 6,
      // Left face (-X)
      0, 4, 7, 0, 7, 3,
      // Top face (+Y)
      7, 6, 2, 7, 2, 3,
      // Bottom face (-Y)
      0, 1, 5, 0, 5, 4,
  };

  return props;
}

/// Helper to create an identity view-projection matrix (row-major).
std::array<float, 16> CreateIdentityMatrix() {
  return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}


} // namespace

// ============================================================================
// Face Picking Tests
// ============================================================================

TEST(FacePicking, RayThroughTriangleCenterHits) {
  // Triangle in XY plane at Z=0, centered at origin
  auto props = CreateTriangleBrush(-1.0f, -1.0f, 0.0f, // v0
                                   1.0f, -1.0f, 0.0f,  // v1
                                   0.0f, 1.0f, 0.0f);  // v2

  // Ray from Z=5 pointing toward origin
  SubObjectPickRay ray;
  ray.origin = {0.0f, 0.0f, 5.0f};
  ray.dir = {0.0f, 0.0f, -1.0f};

  auto result = PickFace(ray, props, 42);

  EXPECT_TRUE(result.hit);
  EXPECT_EQ(result.face.node_id, 42);
  EXPECT_EQ(result.face.triangle_index, 0u);
  EXPECT_FLOAT_EQ(result.distance, 5.0f);
  EXPECT_NEAR(result.hit_position[0], 0.0f, 1e-5f);
  EXPECT_NEAR(result.hit_position[1], 0.0f, 1e-5f);
  EXPECT_NEAR(result.hit_position[2], 0.0f, 1e-5f);
}

TEST(FacePicking, RayMissingTriangleMisses) {
  auto props = CreateTriangleBrush(-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // Ray that misses the triangle completely
  SubObjectPickRay ray;
  ray.origin = {10.0f, 10.0f, 5.0f};
  ray.dir = {0.0f, 0.0f, -1.0f};

  auto result = PickFace(ray, props, 0);

  EXPECT_FALSE(result.hit);
}

TEST(FacePicking, ParallelRayMisses) {
  auto props = CreateTriangleBrush(-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // Ray parallel to the triangle
  SubObjectPickRay ray;
  ray.origin = {0.0f, 0.0f, 1.0f};
  ray.dir = {1.0f, 0.0f, 0.0f};

  auto result = PickFace(ray, props, 0);

  EXPECT_FALSE(result.hit);
}

TEST(FacePicking, RayBehindTriangleMisses) {
  auto props = CreateTriangleBrush(-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // Ray pointing away from the triangle
  SubObjectPickRay ray;
  ray.origin = {0.0f, 0.0f, 5.0f};
  ray.dir = {0.0f, 0.0f, 1.0f}; // Pointing away

  auto result = PickFace(ray, props, 0);

  EXPECT_FALSE(result.hit);
}

TEST(FacePicking, MultipleTrianglesPicksClosest) {
  NodeProperties props;
  // Two triangles at different Z depths
  props.brush_vertices = {
      // Triangle 0 at Z=0
      -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
      // Triangle 1 at Z=2
      -1.0f, -1.0f, 2.0f, 1.0f, -1.0f, 2.0f, 0.0f, 1.0f, 2.0f,
  };
  props.brush_indices = {0, 1, 2, 3, 4, 5};

  // Ray from Z=5 toward origin
  SubObjectPickRay ray;
  ray.origin = {0.0f, 0.0f, 5.0f};
  ray.dir = {0.0f, 0.0f, -1.0f};

  auto result = PickFace(ray, props, 1);

  EXPECT_TRUE(result.hit);
  EXPECT_EQ(result.face.triangle_index, 1u); // Should hit triangle at Z=2 first
  EXPECT_NEAR(result.distance, 3.0f, 1e-5f);
}

TEST(FacePicking, EmptyBrushReturnsNoHit) {
  NodeProperties props;

  SubObjectPickRay ray;
  ray.origin = {0.0f, 0.0f, 5.0f};
  ray.dir = {0.0f, 0.0f, -1.0f};

  auto result = PickFace(ray, props, 0);

  EXPECT_FALSE(result.hit);
}

TEST(FacePicking, FaceNormalPointsTowardCamera) {
  auto props = CreateTriangleBrush(-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  SubObjectPickRay ray;
  ray.origin = {0.0f, 0.0f, 5.0f};
  ray.dir = {0.0f, 0.0f, -1.0f};

  auto result = PickFace(ray, props, 0);

  EXPECT_TRUE(result.hit);
  // Normal should be roughly +Z (perpendicular to XY plane triangle)
  EXPECT_NEAR(std::fabs(result.face_normal[2]), 1.0f, 1e-5f);
}

// ============================================================================
// Vertex Picking Tests
// ============================================================================

TEST(VertexPicking, ClickOnVertexHits) {
  // Create a triangle with vertex 0 at origin (which projects to screen center with identity matrix)
  auto props = CreateTriangleBrush(0.0f, 0.0f, 0.0f,  // v0 at origin
                                   0.5f, 0.0f, 0.0f,  // v1
                                   0.0f, 0.5f, 0.0f); // v2

  // Identity matrix: world (0, 0, 0) -> NDC (0, 0) -> screen (400, 300)
  auto view_proj = CreateIdentityMatrix();
  std::array<float, 2> viewport_pos = {0.0f, 0.0f};
  std::array<float, 2> viewport_size = {800.0f, 600.0f};

  // Click at screen center where vertex 0 should project to
  std::array<float, 2> mouse = {400.0f, 300.0f};

  auto result = PickVertex(mouse, props, 5, view_proj, viewport_pos, viewport_size, 50.0f);

  EXPECT_TRUE(result.hit);
  EXPECT_EQ(result.vertex.node_id, 5);
  EXPECT_EQ(result.vertex.vertex_index, 0u); // Vertex 0 is at origin, projects to center
}

TEST(VertexPicking, ClickFarFromVertexMisses) {
  auto props = CreateTriangleBrush(-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  auto view_proj = CreateIdentityMatrix();
  std::array<float, 2> viewport_pos = {0.0f, 0.0f};
  std::array<float, 2> viewport_size = {800.0f, 600.0f};

  // Click far from any vertex
  std::array<float, 2> mouse = {0.0f, 0.0f};

  auto result = PickVertex(mouse, props, 0, view_proj, viewport_pos, viewport_size, 10.0f);

  EXPECT_FALSE(result.hit);
}

TEST(VertexPicking, EmptyBrushReturnsNoHit) {
  NodeProperties props;

  auto view_proj = CreateIdentityMatrix();
  std::array<float, 2> viewport_pos = {0.0f, 0.0f};
  std::array<float, 2> viewport_size = {800.0f, 600.0f};
  std::array<float, 2> mouse = {400.0f, 300.0f};

  auto result = PickVertex(mouse, props, 0, view_proj, viewport_pos, viewport_size);

  EXPECT_FALSE(result.hit);
}

TEST(VertexPicking, PicksClosestVertex) {
  auto props = CreateTriangleBrush(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

  auto view_proj = CreateIdentityMatrix();
  std::array<float, 2> viewport_pos = {0.0f, 0.0f};
  std::array<float, 2> viewport_size = {800.0f, 600.0f};

  // Click at screen center - vertex 2 at (0,0,0) should be closest
  std::array<float, 2> mouse = {400.0f, 300.0f};

  auto result = PickVertex(mouse, props, 0, view_proj, viewport_pos, viewport_size, 200.0f);

  EXPECT_TRUE(result.hit);
  EXPECT_EQ(result.vertex.vertex_index, 2u); // The (0,0,0) vertex
}

// ============================================================================
// Edge Picking Tests
// ============================================================================

TEST(EdgePicking, ClickOnEdgeMidpointHits) {
  auto props = CreateTriangleBrush(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  auto view_proj = CreateIdentityMatrix();
  std::array<float, 2> viewport_pos = {0.0f, 0.0f};
  std::array<float, 2> viewport_size = {800.0f, 600.0f};

  // Click at screen center where edge 0-1 passes through
  std::array<float, 2> mouse = {400.0f, 300.0f};

  auto result = PickEdge(mouse, props, 7, view_proj, viewport_pos, viewport_size, 20.0f);

  EXPECT_TRUE(result.hit);
  EXPECT_EQ(result.edge.node_id, 7);
  EXPECT_TRUE(result.edge.IsValid());
}

TEST(EdgePicking, ClickFarFromEdgeMisses) {
  auto props = CreateTriangleBrush(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  auto view_proj = CreateIdentityMatrix();
  std::array<float, 2> viewport_pos = {0.0f, 0.0f};
  std::array<float, 2> viewport_size = {800.0f, 600.0f};

  // Click far from any edge
  std::array<float, 2> mouse = {10.0f, 10.0f};

  auto result = PickEdge(mouse, props, 0, view_proj, viewport_pos, viewport_size, 5.0f);

  EXPECT_FALSE(result.hit);
}

TEST(EdgePicking, EmptyBrushReturnsNoHit) {
  NodeProperties props;

  auto view_proj = CreateIdentityMatrix();
  std::array<float, 2> viewport_pos = {0.0f, 0.0f};
  std::array<float, 2> viewport_size = {800.0f, 600.0f};
  std::array<float, 2> mouse = {400.0f, 300.0f};

  auto result = PickEdge(mouse, props, 0, view_proj, viewport_pos, viewport_size);

  EXPECT_FALSE(result.hit);
}

// ============================================================================
// Edge Collection Tests
// ============================================================================

TEST(CollectBrushEdges, TriangleHasThreeEdges) {
  auto props = CreateTriangleBrush(0, 0, 0, 1, 0, 0, 0, 1, 0);

  auto edges = CollectBrushEdges(props);

  EXPECT_EQ(edges.size(), 3u);
}

TEST(CollectBrushEdges, SharedEdgesNotDuplicated) {
  NodeProperties props;
  // Two triangles sharing an edge
  props.brush_vertices = {
      0.0f, 0.0f, 0.0f, // 0
      1.0f, 0.0f, 0.0f, // 1
      0.0f, 1.0f, 0.0f, // 2
      1.0f, 1.0f, 0.0f, // 3
  };
  props.brush_indices = {0, 1, 2, 1, 3, 2}; // Share edge 1-2

  auto edges = CollectBrushEdges(props);

  // Should have 5 unique edges, not 6
  EXPECT_EQ(edges.size(), 5u);
}

TEST(CollectBrushEdges, EdgesAreNormalized) {
  auto props = CreateTriangleBrush(0, 0, 0, 1, 0, 0, 0, 1, 0);

  auto edges = CollectBrushEdges(props);

  for (const auto& edge : edges) {
    EXPECT_LE(edge.first, edge.second) << "Edge should be normalized (first <= second)";
  }
}

TEST(CollectBrushEdges, CubeHasTwelveEdges) {
  auto props = CreateUnitCubeBrush();

  auto edges = CollectBrushEdges(props);

  EXPECT_EQ(edges.size(), 18u); // A cube has 12 edges, but our triangulated cube shares some
}

TEST(CollectBrushEdges, EmptyBrushReturnsNoEdges) {
  NodeProperties props;

  auto edges = CollectBrushEdges(props);

  EXPECT_TRUE(edges.empty());
}

// ============================================================================
// Vertex Position Tests
// ============================================================================

TEST(GetVertexPosition, ValidIndexReturnsPosition) {
  auto props = CreateTriangleBrush(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);

  auto pos = GetVertexPosition(props, 1);

  ASSERT_TRUE(pos.has_value());
  EXPECT_FLOAT_EQ((*pos)[0], 4.0f);
  EXPECT_FLOAT_EQ((*pos)[1], 5.0f);
  EXPECT_FLOAT_EQ((*pos)[2], 6.0f);
}

TEST(GetVertexPosition, InvalidIndexReturnsNullopt) {
  auto props = CreateTriangleBrush(1, 2, 3, 4, 5, 6, 7, 8, 9);

  auto pos = GetVertexPosition(props, 100);

  EXPECT_FALSE(pos.has_value());
}

TEST(GetVertexPosition, EmptyBrushReturnsNullopt) {
  NodeProperties props;

  auto pos = GetVertexPosition(props, 0);

  EXPECT_FALSE(pos.has_value());
}

// ============================================================================
// Edge Position Tests
// ============================================================================

TEST(GetEdgePositions, ValidIndicesReturnPositions) {
  auto props = CreateTriangleBrush(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);

  auto positions = GetEdgePositions(props, 0, 2);

  ASSERT_TRUE(positions.has_value());
  EXPECT_FLOAT_EQ(positions->first[0], 1.0f);
  EXPECT_FLOAT_EQ(positions->second[0], 7.0f);
}

TEST(GetEdgePositions, InvalidFirstIndexReturnsNullopt) {
  auto props = CreateTriangleBrush(1, 2, 3, 4, 5, 6, 7, 8, 9);

  auto positions = GetEdgePositions(props, 100, 0);

  EXPECT_FALSE(positions.has_value());
}

TEST(GetEdgePositions, InvalidSecondIndexReturnsNullopt) {
  auto props = CreateTriangleBrush(1, 2, 3, 4, 5, 6, 7, 8, 9);

  auto positions = GetEdgePositions(props, 0, 100);

  EXPECT_FALSE(positions.has_value());
}

// ============================================================================
// Face Centroid Tests
// ============================================================================

TEST(GetFaceCentroid, ValidIndexReturnsCentroid) {
  auto props = CreateTriangleBrush(0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f);

  auto centroid = GetFaceCentroid(props, 0);

  ASSERT_TRUE(centroid.has_value());
  EXPECT_FLOAT_EQ((*centroid)[0], 1.0f);
  EXPECT_FLOAT_EQ((*centroid)[1], 1.0f);
  EXPECT_FLOAT_EQ((*centroid)[2], 0.0f);
}

TEST(GetFaceCentroid, InvalidIndexReturnsNullopt) {
  auto props = CreateTriangleBrush(0, 0, 0, 1, 0, 0, 0, 1, 0);

  auto centroid = GetFaceCentroid(props, 100);

  EXPECT_FALSE(centroid.has_value());
}

// ============================================================================
// Face Normal Tests
// ============================================================================

TEST(GetFaceNormal, XYPlaneTriangleHasZNormal) {
  // Triangle in XY plane (counter-clockwise from +Z)
  auto props = CreateTriangleBrush(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  auto normal = GetFaceNormal(props, 0);

  ASSERT_TRUE(normal.has_value());
  EXPECT_NEAR((*normal)[0], 0.0f, 1e-5f);
  EXPECT_NEAR((*normal)[1], 0.0f, 1e-5f);
  EXPECT_NEAR(std::fabs((*normal)[2]), 1.0f, 1e-5f);
}

TEST(GetFaceNormal, YZPlaneTriangleHasXNormal) {
  // Triangle in YZ plane
  auto props = CreateTriangleBrush(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

  auto normal = GetFaceNormal(props, 0);

  ASSERT_TRUE(normal.has_value());
  EXPECT_NEAR(std::fabs((*normal)[0]), 1.0f, 1e-5f);
  EXPECT_NEAR((*normal)[1], 0.0f, 1e-5f);
  EXPECT_NEAR((*normal)[2], 0.0f, 1e-5f);
}

TEST(GetFaceNormal, InvalidIndexReturnsNullopt) {
  auto props = CreateTriangleBrush(0, 0, 0, 1, 0, 0, 0, 1, 0);

  auto normal = GetFaceNormal(props, 100);

  EXPECT_FALSE(normal.has_value());
}

TEST(GetFaceNormal, NormalIsNormalized) {
  auto props = CreateTriangleBrush(0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f);

  auto normal = GetFaceNormal(props, 0);

  ASSERT_TRUE(normal.has_value());
  float length = std::sqrt((*normal)[0] * (*normal)[0] + (*normal)[1] * (*normal)[1] + (*normal)[2] * (*normal)[2]);
  EXPECT_NEAR(length, 1.0f, 1e-5f);
}
