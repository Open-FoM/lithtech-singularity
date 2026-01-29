#include "brush/brush_primitive.h"

#include <gtest/gtest.h>

#include <algorithm>

// Helper to validate all indices are within valid range
static void ExpectValidIndices(const PrimitiveResult& result) {
  const size_t vertex_count = result.vertices.size() / 3;
  for (size_t i = 0; i < result.indices.size(); ++i) {
    EXPECT_LT(result.indices[i], vertex_count)
        << "Index " << i << " (value=" << result.indices[i] << ") exceeds vertex count " << vertex_count;
  }
}

// -----------------------------------------------------------------------------
// Box Tests
// -----------------------------------------------------------------------------

TEST(BrushPrimitive, CreateBox_DefaultParams) {
  BoxParams params;
  auto result = CreatePrimitiveBox(params);

  EXPECT_TRUE(result.success);
  // Box has 8 vertices
  EXPECT_EQ(result.vertices.size(), 8 * 3);
  // Box has 6 faces * 2 triangles * 3 indices = 36 indices
  EXPECT_EQ(result.indices.size(), 36);
}

TEST(BrushPrimitive, CreateBox_CustomDimensions) {
  BoxParams params;
  params.size[0] = 32.0f;   // width
  params.size[1] = 128.0f;  // height
  params.size[2] = 64.0f;   // depth

  auto result = CreatePrimitiveBox(params);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.vertices.size(), 8 * 3);
  EXPECT_EQ(result.indices.size(), 36);

  // Verify extents match expected dimensions
  float min_x = result.vertices[0], max_x = result.vertices[0];
  float min_y = result.vertices[1], max_y = result.vertices[1];
  float min_z = result.vertices[2], max_z = result.vertices[2];

  for (size_t i = 0; i < 8; ++i) {
    min_x = std::min(min_x, result.vertices[i * 3 + 0]);
    max_x = std::max(max_x, result.vertices[i * 3 + 0]);
    min_y = std::min(min_y, result.vertices[i * 3 + 1]);
    max_y = std::max(max_y, result.vertices[i * 3 + 1]);
    min_z = std::min(min_z, result.vertices[i * 3 + 2]);
    max_z = std::max(max_z, result.vertices[i * 3 + 2]);
  }

  EXPECT_FLOAT_EQ(max_x - min_x, 32.0f);
  EXPECT_FLOAT_EQ(max_y - min_y, 128.0f);
  EXPECT_FLOAT_EQ(max_z - min_z, 64.0f);
}

TEST(BrushPrimitive, CreateBox_CenterOffset) {
  BoxParams params;
  params.center[0] = 100.0f;
  params.center[1] = 200.0f;
  params.center[2] = 300.0f;
  params.size[0] = 64.0f;
  params.size[1] = 64.0f;
  params.size[2] = 64.0f;

  auto result = CreatePrimitiveBox(params);

  EXPECT_TRUE(result.success);

  // Calculate actual center from vertices
  float cx = 0.0f, cy = 0.0f, cz = 0.0f;
  for (size_t i = 0; i < 8; ++i) {
    cx += result.vertices[i * 3 + 0];
    cy += result.vertices[i * 3 + 1];
    cz += result.vertices[i * 3 + 2];
  }
  cx /= 8.0f;
  cy /= 8.0f;
  cz /= 8.0f;

  EXPECT_FLOAT_EQ(cx, 100.0f);
  EXPECT_FLOAT_EQ(cy, 200.0f);
  EXPECT_FLOAT_EQ(cz, 300.0f);
}

TEST(BrushPrimitive, CreateBox_ValidIndices) {
  BoxParams params;
  auto result = CreatePrimitiveBox(params);

  EXPECT_TRUE(result.success);
  ExpectValidIndices(result);
}

// -----------------------------------------------------------------------------
// Cylinder Tests
// -----------------------------------------------------------------------------

TEST(BrushPrimitive, CreateCylinder_DefaultParams) {
  CylinderParams params;
  auto result = CreatePrimitiveCylinder(params);

  EXPECT_TRUE(result.success);
  // Default 8 sides: 2*8 ring + 2 poles = 18 vertices
  EXPECT_EQ(result.vertices.size(), 18 * 3);
}

TEST(BrushPrimitive, CreateCylinder_MinSides) {
  CylinderParams params;
  params.sides = 1;  // Should clamp to 3

  auto result = CreatePrimitiveCylinder(params);

  EXPECT_TRUE(result.success);
  // 3 sides: 2*3 ring + 2 poles = 8 vertices
  EXPECT_EQ(result.vertices.size(), 8 * 3);
}

TEST(BrushPrimitive, CreateCylinder_MaxSides) {
  CylinderParams params;
  params.sides = 100;  // Should clamp to 64

  auto result = CreatePrimitiveCylinder(params);

  EXPECT_TRUE(result.success);
  // 64 sides: 2*64 ring + 2 poles = 130 vertices
  EXPECT_EQ(result.vertices.size(), 130 * 3);
}

TEST(BrushPrimitive, CreateCylinder_VertexCount) {
  // Test various side counts
  for (int sides = 3; sides <= 16; ++sides) {
    CylinderParams params;
    params.sides = sides;

    auto result = CreatePrimitiveCylinder(params);

    EXPECT_TRUE(result.success);
    // Vertex count = 2*sides (rings) + 2 (poles)
    EXPECT_EQ(result.vertices.size(), static_cast<size_t>((2 * sides + 2) * 3))
        << "Failed for sides=" << sides;
  }
}

TEST(BrushPrimitive, CreateCylinder_ValidIndices) {
  CylinderParams params;
  params.sides = 12;
  auto result = CreatePrimitiveCylinder(params);

  EXPECT_TRUE(result.success);
  ExpectValidIndices(result);
}

// -----------------------------------------------------------------------------
// Pyramid Tests
// -----------------------------------------------------------------------------

TEST(BrushPrimitive, CreatePyramid_PointedApex) {
  PyramidParams params;
  params.top_radius = 0.0f;  // Pointed apex
  params.sides = 4;

  auto result = CreatePrimitivePyramid(params);

  EXPECT_TRUE(result.success);
  // Pointed pyramid: sides (base ring) + 1 (apex) + 1 (base center) = sides + 2
  EXPECT_EQ(result.vertices.size(), 6 * 3);  // 4 + 2 = 6
}

TEST(BrushPrimitive, CreatePyramid_Frustum) {
  PyramidParams params;
  params.top_radius = 32.0f;  // Frustum (truncated pyramid)
  params.sides = 4;

  auto result = CreatePrimitivePyramid(params);

  EXPECT_TRUE(result.success);
  // Frustum: 2*sides (rings) + 2 (centers) = 2*4 + 2 = 10
  EXPECT_EQ(result.vertices.size(), 10 * 3);
}

TEST(BrushPrimitive, CreatePyramid_SidesClamping) {
  // Test minimum clamping
  {
    PyramidParams params;
    params.sides = 1;  // Should clamp to 3
    auto result = CreatePrimitivePyramid(params);
    EXPECT_TRUE(result.success);
    // 3 sides pointed: 3 + 2 = 5 vertices
    EXPECT_EQ(result.vertices.size(), 5 * 3);
  }

  // Test maximum clamping
  {
    PyramidParams params;
    params.sides = 100;  // Should clamp to 32
    auto result = CreatePrimitivePyramid(params);
    EXPECT_TRUE(result.success);
    // 32 sides pointed: 32 + 2 = 34 vertices
    EXPECT_EQ(result.vertices.size(), 34 * 3);
  }
}

TEST(BrushPrimitive, CreatePyramid_ValidIndices) {
  PyramidParams params;
  params.sides = 6;
  params.top_radius = 20.0f;  // Test frustum variant
  auto result = CreatePrimitivePyramid(params);

  EXPECT_TRUE(result.success);
  ExpectValidIndices(result);
}

// -----------------------------------------------------------------------------
// Sphere/Dome Tests
// -----------------------------------------------------------------------------

TEST(BrushPrimitive, CreateSphere_FullSphere) {
  SphereParams params;
  params.dome = false;

  auto result = CreatePrimitiveSphere(params);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.vertices.size(), 0u);
  EXPECT_GT(result.indices.size(), 0u);
}

TEST(BrushPrimitive, CreateSphere_Dome) {
  SphereParams params;
  params.dome = true;

  auto result = CreatePrimitiveSphere(params);

  EXPECT_TRUE(result.success);
  // Dome should have fewer vertices than full sphere (roughly half plus cap)
  EXPECT_GT(result.vertices.size(), 0u);
  EXPECT_GT(result.indices.size(), 0u);

  // All Y values should be >= center.y for a dome
  const float center_y = params.center[1];
  for (size_t i = 0; i < result.vertices.size() / 3; ++i) {
    EXPECT_GE(result.vertices[i * 3 + 1], center_y - 0.001f)
        << "Vertex " << i << " Y=" << result.vertices[i * 3 + 1] << " is below dome base";
  }
}

TEST(BrushPrimitive, CreateSphere_SubdivisionClamping) {
  // Test horizontal clamping
  {
    SphereParams params;
    params.horizontal_subdivisions = 2;  // Should clamp to 4
    params.vertical_subdivisions = 4;
    auto result = CreatePrimitiveSphere(params);
    EXPECT_TRUE(result.success);
  }

  // Test vertical clamping
  {
    SphereParams params;
    params.horizontal_subdivisions = 8;
    params.vertical_subdivisions = 1;  // Should clamp to 2
    auto result = CreatePrimitiveSphere(params);
    EXPECT_TRUE(result.success);
  }

  // Test maximum clamping
  {
    SphereParams params;
    params.horizontal_subdivisions = 100;  // Should clamp to 32
    params.vertical_subdivisions = 50;     // Should clamp to 16
    auto result = CreatePrimitiveSphere(params);
    EXPECT_TRUE(result.success);
  }
}

TEST(BrushPrimitive, CreateSphere_ValidIndices) {
  SphereParams params;
  params.horizontal_subdivisions = 12;
  params.vertical_subdivisions = 6;
  auto result = CreatePrimitiveSphere(params);

  EXPECT_TRUE(result.success);
  ExpectValidIndices(result);
}

// -----------------------------------------------------------------------------
// Plane Tests
// -----------------------------------------------------------------------------

TEST(BrushPrimitive, CreatePlane_DefaultOrientation) {
  PlaneParams params;
  // Default is XZ plane with Y-up normal
  auto result = CreatePrimitivePlane(params);

  EXPECT_TRUE(result.success);
  // Plane always has 4 vertices, 6 indices (2 triangles)
  EXPECT_EQ(result.vertices.size(), 4 * 3);
  EXPECT_EQ(result.indices.size(), 6);
}

TEST(BrushPrimitive, CreatePlane_OrientationPresets) {
  // Test XY plane (facing Z)
  {
    PlaneParams params;
    params.normal[0] = 0.0f;
    params.normal[1] = 0.0f;
    params.normal[2] = 1.0f;
    auto result = CreatePrimitivePlane(params);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.vertices.size(), 4 * 3);
  }

  // Test YZ plane (facing X)
  {
    PlaneParams params;
    params.normal[0] = 1.0f;
    params.normal[1] = 0.0f;
    params.normal[2] = 0.0f;
    auto result = CreatePrimitivePlane(params);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.vertices.size(), 4 * 3);
  }
}

TEST(BrushPrimitive, CreatePlane_QuadGeometry) {
  PlaneParams params;
  params.width = 100.0f;
  params.height = 50.0f;
  auto result = CreatePrimitivePlane(params);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.vertices.size(), 4 * 3);
  EXPECT_EQ(result.indices.size(), 6);
  ExpectValidIndices(result);
}

// -----------------------------------------------------------------------------
// Polygon Utility Tests
// -----------------------------------------------------------------------------

TEST(BrushPrimitive, IsPolygonConvex_Triangle) {
  // Any triangle is convex
  std::vector<float> triangle = {
      0.0f, 0.0f,    // vertex 0
      100.0f, 0.0f,  // vertex 1
      50.0f, 100.0f  // vertex 2
  };

  EXPECT_TRUE(IsPolygonConvex(triangle));
}

TEST(BrushPrimitive, IsPolygonConvex_Square) {
  // Convex square
  std::vector<float> square = {
      0.0f, 0.0f,      // vertex 0
      100.0f, 0.0f,    // vertex 1
      100.0f, 100.0f,  // vertex 2
      0.0f, 100.0f     // vertex 3
  };

  EXPECT_TRUE(IsPolygonConvex(square));
}

TEST(BrushPrimitive, IsPolygonConvex_ConcaveShape) {
  // L-shaped concave polygon
  std::vector<float> concave = {
      0.0f, 0.0f,     // vertex 0
      100.0f, 0.0f,   // vertex 1
      100.0f, 50.0f,  // vertex 2
      50.0f, 50.0f,   // vertex 3 - concave vertex
      50.0f, 100.0f,  // vertex 4
      0.0f, 100.0f    // vertex 5
  };

  EXPECT_FALSE(IsPolygonConvex(concave));
}

TEST(BrushPrimitive, IsPolygonConvex_DegenerateCases) {
  // Empty polygon
  std::vector<float> empty;
  EXPECT_TRUE(IsPolygonConvex(empty));

  // Single vertex
  std::vector<float> single = {0.0f, 0.0f};
  EXPECT_TRUE(IsPolygonConvex(single));

  // Two vertices (line)
  std::vector<float> line = {0.0f, 0.0f, 100.0f, 100.0f};
  EXPECT_TRUE(IsPolygonConvex(line));
}

TEST(BrushPrimitive, ExtrudePolygon_SimpleTriangle) {
  std::vector<float> triangle = {
      0.0f, 0.0f,    // vertex 0
      100.0f, 0.0f,  // vertex 1
      50.0f, 100.0f  // vertex 2
  };
  float normal[3] = {0.0f, 1.0f, 0.0f};  // Extrude along Y
  float height = 64.0f;

  auto result = ExtrudePolygon(triangle, height, normal, 0.0f);

  EXPECT_TRUE(result.success);
  // 3 bottom + 3 top + 2 centers = 8 vertices
  EXPECT_EQ(result.vertices.size(), 8 * 3);
  // Bottom cap: 3 triangles, Top cap: 3 triangles, Sides: 3 quads (6 triangles) = 12 triangles
  EXPECT_EQ(result.indices.size(), 12 * 3);
  ExpectValidIndices(result);
}

TEST(BrushPrimitive, ExtrudePolygon_NegativeHeight) {
  std::vector<float> square = {
      0.0f, 0.0f,      // vertex 0
      100.0f, 0.0f,    // vertex 1
      100.0f, 100.0f,  // vertex 2
      0.0f, 100.0f     // vertex 3
  };
  float normal[3] = {0.0f, 1.0f, 0.0f};
  float height = -32.0f;  // Negative extrusion

  auto result = ExtrudePolygon(square, height, normal, 0.0f);

  EXPECT_TRUE(result.success);
  // 4 bottom + 4 top + 2 centers = 10 vertices
  EXPECT_EQ(result.vertices.size(), 10 * 3);
  ExpectValidIndices(result);
}

TEST(BrushPrimitive, ExtrudePolygon_InvalidInput) {
  // Too few vertices
  std::vector<float> line = {0.0f, 0.0f, 100.0f, 100.0f};
  float normal[3] = {0.0f, 1.0f, 0.0f};

  auto result = ExtrudePolygon(line, 64.0f, normal, 0.0f);

  EXPECT_FALSE(result.success);
}

TEST(BrushPrimitive, ExtrudePolygon_ExplicitBasis_XZPlane) {
  // Test explicit basis for XZ plane (u=X, v=Z, normal=Y)
  // This simulates vertices drawn in top-down view
  std::vector<float> square = {
      0.0f, 0.0f,      // u=0, v=0 -> X=0, Z=0
      100.0f, 0.0f,    // u=100, v=0 -> X=100, Z=0
      100.0f, 100.0f,  // u=100, v=100 -> X=100, Z=100
      0.0f, 100.0f     // u=0, v=100 -> X=0, Z=100
  };
  float normal[3] = {0.0f, 1.0f, 0.0f};
  float tangent[3] = {1.0f, 0.0f, 0.0f};    // u -> X
  float bitangent[3] = {0.0f, 0.0f, 1.0f};  // v -> Z
  float height = 64.0f;

  auto result = ExtrudePolygon(square, height, normal, tangent, bitangent, 0.0f);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.vertices.size(), 10 * 3);  // 4 bottom + 4 top + 2 centers
  ExpectValidIndices(result);

  // Verify first vertex is at (0, 0, 0) - bottom face at plane_offset=0
  EXPECT_FLOAT_EQ(result.vertices[0], 0.0f);  // X
  EXPECT_FLOAT_EQ(result.vertices[1], 0.0f);  // Y (plane_offset)
  EXPECT_FLOAT_EQ(result.vertices[2], 0.0f);  // Z

  // Verify second vertex is at (100, 0, 0)
  EXPECT_FLOAT_EQ(result.vertices[3], 100.0f);  // X
  EXPECT_FLOAT_EQ(result.vertices[4], 0.0f);    // Y
  EXPECT_FLOAT_EQ(result.vertices[5], 0.0f);    // Z
}

TEST(BrushPrimitive, ExtrudePolygon_ExplicitBasis_YZPlane) {
  // Test explicit basis for YZ plane (u=Y, v=Z, normal=X)
  std::vector<float> triangle = {
      0.0f, 0.0f,    // u=0, v=0 -> Y=0, Z=0
      50.0f, 0.0f,   // u=50, v=0 -> Y=50, Z=0
      25.0f, 50.0f   // u=25, v=50 -> Y=25, Z=50
  };
  float normal[3] = {1.0f, 0.0f, 0.0f};
  float tangent[3] = {0.0f, 1.0f, 0.0f};    // u -> Y
  float bitangent[3] = {0.0f, 0.0f, 1.0f};  // v -> Z
  float height = 32.0f;

  auto result = ExtrudePolygon(triangle, height, normal, tangent, bitangent, 10.0f);

  EXPECT_TRUE(result.success);
  ExpectValidIndices(result);

  // Verify first vertex is at (10, 0, 0) - plane_offset=10 along X
  EXPECT_FLOAT_EQ(result.vertices[0], 10.0f);  // X (plane_offset)
  EXPECT_FLOAT_EQ(result.vertices[1], 0.0f);   // Y
  EXPECT_FLOAT_EQ(result.vertices[2], 0.0f);   // Z
}
