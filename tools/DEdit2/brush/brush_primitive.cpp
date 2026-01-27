#include "brush/brush_primitive.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

void AddVertex(std::vector<float>& verts, float x, float y, float z) {
  verts.push_back(x);
  verts.push_back(y);
  verts.push_back(z);
}

void AddTriangle(std::vector<uint32_t>& indices, uint32_t a, uint32_t b, uint32_t c) {
  indices.push_back(a);
  indices.push_back(b);
  indices.push_back(c);
}

void AddQuad(std::vector<uint32_t>& indices, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  // Two triangles: ABC and ACD
  AddTriangle(indices, a, b, c);
  AddTriangle(indices, a, c, d);
}

}  // namespace

PrimitiveResult CreatePrimitiveBox(const BoxParams& params) {
  PrimitiveResult result;
  const float cx = params.center[0];
  const float cy = params.center[1];
  const float cz = params.center[2];
  const float hx = params.size[0] * 0.5f;
  const float hy = params.size[1] * 0.5f;
  const float hz = params.size[2] * 0.5f;

  // 8 vertices of the box
  // Bottom face (Y-)
  AddVertex(result.vertices, cx - hx, cy - hy, cz - hz);  // 0: left-bottom-back
  AddVertex(result.vertices, cx + hx, cy - hy, cz - hz);  // 1: right-bottom-back
  AddVertex(result.vertices, cx + hx, cy - hy, cz + hz);  // 2: right-bottom-front
  AddVertex(result.vertices, cx - hx, cy - hy, cz + hz);  // 3: left-bottom-front
  // Top face (Y+)
  AddVertex(result.vertices, cx - hx, cy + hy, cz - hz);  // 4: left-top-back
  AddVertex(result.vertices, cx + hx, cy + hy, cz - hz);  // 5: right-top-back
  AddVertex(result.vertices, cx + hx, cy + hy, cz + hz);  // 6: right-top-front
  AddVertex(result.vertices, cx - hx, cy + hy, cz + hz);  // 7: left-top-front

  // 12 triangles (6 faces * 2 triangles)
  // Bottom face (Y-): 0, 3, 2 and 0, 2, 1 (CCW when viewed from below)
  AddQuad(result.indices, 0, 3, 2, 1);

  // Top face (Y+): 4, 5, 6 and 4, 6, 7 (CCW when viewed from above)
  AddQuad(result.indices, 4, 5, 6, 7);

  // Front face (Z+): 3, 7, 6 and 3, 6, 2
  AddQuad(result.indices, 3, 7, 6, 2);

  // Back face (Z-): 1, 5, 4 and 1, 4, 0
  AddQuad(result.indices, 1, 5, 4, 0);

  // Left face (X-): 0, 4, 7 and 0, 7, 3
  AddQuad(result.indices, 0, 4, 7, 3);

  // Right face (X+): 2, 6, 5 and 2, 5, 1
  AddQuad(result.indices, 2, 6, 5, 1);

  result.success = true;
  return result;
}

PrimitiveResult CreatePrimitiveCylinder(const CylinderParams& params) {
  PrimitiveResult result;

  const int sides = std::clamp(params.sides, 3, 32);
  const float cx = params.center[0];
  const float cy = params.center[1];
  const float cz = params.center[2];
  const float half_height = params.height * 0.5f;
  const float radius = params.radius;

  // Generate vertices for top and bottom rings
  const float angle_step = kTwoPi / static_cast<float>(sides);

  // Bottom ring vertices (indices 0 to sides-1)
  for (int i = 0; i < sides; ++i) {
    const float angle = static_cast<float>(i) * angle_step;
    const float x = cx + radius * std::cos(angle);
    const float z = cz + radius * std::sin(angle);
    AddVertex(result.vertices, x, cy - half_height, z);
  }

  // Top ring vertices (indices sides to 2*sides-1)
  for (int i = 0; i < sides; ++i) {
    const float angle = static_cast<float>(i) * angle_step;
    const float x = cx + radius * std::cos(angle);
    const float z = cz + radius * std::sin(angle);
    AddVertex(result.vertices, x, cy + half_height, z);
  }

  // Bottom center vertex (index 2*sides)
  AddVertex(result.vertices, cx, cy - half_height, cz);
  const uint32_t bottom_center = static_cast<uint32_t>(2 * sides);

  // Top center vertex (index 2*sides+1)
  AddVertex(result.vertices, cx, cy + half_height, cz);
  const uint32_t top_center = static_cast<uint32_t>(2 * sides + 1);

  // Bottom cap triangles (fan from center)
  for (int i = 0; i < sides; ++i) {
    const uint32_t curr = static_cast<uint32_t>(i);
    const uint32_t next = static_cast<uint32_t>((i + 1) % sides);
    AddTriangle(result.indices, bottom_center, next, curr);  // CCW from below
  }

  // Top cap triangles (fan from center)
  for (int i = 0; i < sides; ++i) {
    const uint32_t curr = static_cast<uint32_t>(sides + i);
    const uint32_t next = static_cast<uint32_t>(sides + (i + 1) % sides);
    AddTriangle(result.indices, top_center, curr, next);  // CCW from above
  }

  // Side faces (quads as triangle pairs)
  for (int i = 0; i < sides; ++i) {
    const uint32_t bot_curr = static_cast<uint32_t>(i);
    const uint32_t bot_next = static_cast<uint32_t>((i + 1) % sides);
    const uint32_t top_curr = static_cast<uint32_t>(sides + i);
    const uint32_t top_next = static_cast<uint32_t>(sides + (i + 1) % sides);
    AddQuad(result.indices, bot_curr, bot_next, top_next, top_curr);
  }

  result.success = true;
  return result;
}

PrimitiveResult CreatePrimitivePyramid(const PyramidParams& params) {
  PrimitiveResult result;

  const int sides = std::clamp(params.sides, 3, 32);
  const float cx = params.center[0];
  const float cy = params.center[1];
  const float cz = params.center[2];
  const float half_height = params.height * 0.5f;
  const float radius = params.radius;

  // Generate base ring vertices
  const float angle_step = kTwoPi / static_cast<float>(sides);

  // Base ring vertices (indices 0 to sides-1)
  for (int i = 0; i < sides; ++i) {
    const float angle = static_cast<float>(i) * angle_step;
    const float x = cx + radius * std::cos(angle);
    const float z = cz + radius * std::sin(angle);
    AddVertex(result.vertices, x, cy - half_height, z);
  }

  // Apex vertex (index sides)
  AddVertex(result.vertices, cx, cy + half_height, cz);
  const uint32_t apex = static_cast<uint32_t>(sides);

  // Base center vertex for cap (index sides+1)
  AddVertex(result.vertices, cx, cy - half_height, cz);
  const uint32_t base_center = static_cast<uint32_t>(sides + 1);

  // Base cap triangles (fan from center)
  for (int i = 0; i < sides; ++i) {
    const uint32_t curr = static_cast<uint32_t>(i);
    const uint32_t next = static_cast<uint32_t>((i + 1) % sides);
    AddTriangle(result.indices, base_center, next, curr);  // CCW from below
  }

  // Side face triangles (to apex)
  for (int i = 0; i < sides; ++i) {
    const uint32_t curr = static_cast<uint32_t>(i);
    const uint32_t next = static_cast<uint32_t>((i + 1) % sides);
    AddTriangle(result.indices, curr, next, apex);
  }

  result.success = true;
  return result;
}

PrimitiveResult CreatePrimitiveSphere(const SphereParams& params) {
  PrimitiveResult result;

  const int h_sub = std::clamp(params.horizontal_subdivisions, 4, 32);
  const int v_sub = std::clamp(params.vertical_subdivisions, 2, 16);
  const float cx = params.center[0];
  const float cy = params.center[1];
  const float cz = params.center[2];
  const float radius = params.radius;
  const bool dome = params.dome;

  // For dome, only generate upper hemisphere
  const int v_start = dome ? v_sub : 0;
  const int v_end = 2 * v_sub;
  const int v_count = v_end - v_start + 1;

  // Generate vertices using spherical coordinates
  // Vertical: from bottom pole (-Y) to top pole (+Y)
  // Horizontal: around Y axis

  // Vertex indices are: ring * h_sub + segment
  // Plus pole vertices at the end

  std::vector<uint32_t> vertex_map;  // Maps (v_ring, h_seg) to vertex index

  // Generate ring vertices
  for (int v = v_start; v <= v_end; ++v) {
    // Phi goes from PI (bottom) to 0 (top)
    const float phi = kPi * (1.0f - static_cast<float>(v) / static_cast<float>(2 * v_sub));
    const float y = cy + radius * std::cos(phi);
    const float ring_radius = radius * std::sin(phi);

    if (v == 0 || v == 2 * v_sub) {
      // Pole vertex
      AddVertex(result.vertices, cx, y, cz);
    } else {
      // Ring vertices
      for (int h = 0; h < h_sub; ++h) {
        const float theta = kTwoPi * static_cast<float>(h) / static_cast<float>(h_sub);
        const float x = cx + ring_radius * std::cos(theta);
        const float z = cz + ring_radius * std::sin(theta);
        AddVertex(result.vertices, x, y, z);
      }
    }
  }

  // Build faces
  // Track current vertex index for each ring
  uint32_t vert_idx = 0;

  for (int v = v_start; v < v_end; ++v) {
    const bool curr_is_pole = (v == 0);
    const bool next_is_pole = (v + 1 == 2 * v_sub);

    if (curr_is_pole) {
      // Bottom pole: triangles from pole to first ring
      const uint32_t pole = vert_idx++;
      for (int h = 0; h < h_sub; ++h) {
        const uint32_t curr = vert_idx + static_cast<uint32_t>(h);
        const uint32_t next = vert_idx + static_cast<uint32_t>((h + 1) % h_sub);
        AddTriangle(result.indices, pole, next, curr);
      }
    } else if (next_is_pole) {
      // Top pole: triangles from last ring to pole
      const uint32_t ring_start = vert_idx;
      vert_idx += static_cast<uint32_t>(h_sub);
      const uint32_t pole = vert_idx++;
      for (int h = 0; h < h_sub; ++h) {
        const uint32_t curr = ring_start + static_cast<uint32_t>(h);
        const uint32_t next = ring_start + static_cast<uint32_t>((h + 1) % h_sub);
        AddTriangle(result.indices, curr, next, pole);
      }
    } else {
      // Regular ring-to-ring quads
      const uint32_t curr_ring = vert_idx;
      vert_idx += static_cast<uint32_t>(h_sub);
      const uint32_t next_ring = vert_idx;
      for (int h = 0; h < h_sub; ++h) {
        const uint32_t h_next = static_cast<uint32_t>((h + 1) % h_sub);
        const uint32_t bl = curr_ring + static_cast<uint32_t>(h);
        const uint32_t br = curr_ring + h_next;
        const uint32_t tl = next_ring + static_cast<uint32_t>(h);
        const uint32_t tr = next_ring + h_next;
        AddQuad(result.indices, bl, br, tr, tl);
      }
    }
  }

  // For dome, add bottom cap
  if (dome) {
    // Find the equator ring (v = v_sub)
    // It should be the first ring after any pole adjustment
    const uint32_t equator_start = 0;
    const uint32_t cap_center = static_cast<uint32_t>(result.vertices.size() / 3);
    AddVertex(result.vertices, cx, cy, cz);  // Center at y=cy (equator level)

    for (int h = 0; h < h_sub; ++h) {
      const uint32_t curr = equator_start + static_cast<uint32_t>(h);
      const uint32_t next = equator_start + static_cast<uint32_t>((h + 1) % h_sub);
      AddTriangle(result.indices, cap_center, curr, next);
    }
  }

  result.success = true;
  return result;
}

PrimitiveResult CreatePrimitivePlane(const PlaneParams& params) {
  PrimitiveResult result;

  const float cx = params.center[0];
  const float cy = params.center[1];
  const float cz = params.center[2];
  const float half_w = params.width * 0.5f;
  const float half_h = params.height * 0.5f;

  // Compute tangent vectors from normal
  float nx = params.normal[0];
  float ny = params.normal[1];
  float nz = params.normal[2];

  // Normalize
  const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (len < 1e-6f) {
    return result;  // Invalid normal
  }
  nx /= len;
  ny /= len;
  nz /= len;

  // Find a perpendicular vector (tangent)
  float tx, ty, tz;
  if (std::abs(ny) < 0.9f) {
    // Cross with Y-up
    tx = nz;
    ty = 0.0f;
    tz = -nx;
  } else {
    // Cross with Z-forward
    tx = ny;
    ty = -nx;
    tz = 0.0f;
  }

  // Normalize tangent
  const float tlen = std::sqrt(tx * tx + ty * ty + tz * tz);
  tx /= tlen;
  ty /= tlen;
  tz /= tlen;

  // Compute bitangent (cross normal x tangent)
  const float bx = ny * tz - nz * ty;
  const float by = nz * tx - nx * tz;
  const float bz = nx * ty - ny * tx;

  // Four corners: center +/- half_w*tangent +/- half_h*bitangent
  AddVertex(result.vertices, cx - half_w * tx - half_h * bx, cy - half_w * ty - half_h * by,
            cz - half_w * tz - half_h * bz);  // 0: -T, -B
  AddVertex(result.vertices, cx + half_w * tx - half_h * bx, cy + half_w * ty - half_h * by,
            cz + half_w * tz - half_h * bz);  // 1: +T, -B
  AddVertex(result.vertices, cx + half_w * tx + half_h * bx, cy + half_w * ty + half_h * by,
            cz + half_w * tz + half_h * bz);  // 2: +T, +B
  AddVertex(result.vertices, cx - half_w * tx + half_h * bx, cy - half_w * ty + half_h * by,
            cz - half_w * tz + half_h * bz);  // 3: -T, +B

  // Two triangles for the quad
  AddQuad(result.indices, 0, 1, 2, 3);

  result.success = true;
  return result;
}
