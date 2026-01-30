#include "csg_types.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace csg {

// =============================================================================
// CSGPlane Implementation
// =============================================================================

std::optional<CSGPlane> CSGPlane::FromPoints(const CSGVertex& a, const CSGVertex& b, const CSGVertex& c) {
  CSGVertex ab = b - a;
  CSGVertex ac = c - a;
  CSGVertex normal = ab.Cross(ac);

  float lenSq = normal.LengthSquared();
  if (lenSq < Tolerance::kDegenerateArea) {
    return std::nullopt; // Points are collinear
  }

  normal = normal / std::sqrt(lenSq);
  float distance = normal.Dot(a);
  return CSGPlane(normal, distance);
}

CSGPlane CSGPlane::FromPointAndNormal(const CSGVertex& point, const CSGVertex& normal) {
  CSGVertex n = normal.Normalized();
  return CSGPlane(n, n.Dot(point));
}

PlaneSide CSGPlane::ClassifyPoint(const CSGVertex& point, float thickness) const {
  float dist = DistanceTo(point);
  if (dist > thickness) {
    return PlaneSide::Front;
  }
  if (dist < -thickness) {
    return PlaneSide::Back;
  }
  return PlaneSide::On;
}

// =============================================================================
// CSGPolygon Implementation
// =============================================================================

bool CSGPolygon::ComputePlane() {
  if (vertices.size() < 3) {
    return false;
  }

  // Newell's method for computing polygon normal (handles non-planar polygons)
  CSGVertex normal(0.0f, 0.0f, 0.0f);

  for (size_t i = 0; i < vertices.size(); ++i) {
    const CSGVertex& curr = vertices[i];
    const CSGVertex& next = vertices[(i + 1) % vertices.size()];

    normal.x += (curr.y - next.y) * (curr.z + next.z);
    normal.y += (curr.z - next.z) * (curr.x + next.x);
    normal.z += (curr.x - next.x) * (curr.y + next.y);
  }

  float lenSq = normal.LengthSquared();
  if (lenSq < Tolerance::kDegenerateArea) {
    return false;
  }

  normal = normal / std::sqrt(lenSq);
  float distance = normal.Dot(vertices[0]);
  plane = CSGPlane(normal, distance);
  return true;
}

bool CSGPolygon::IsValid() const {
  if (vertices.size() < 3) {
    return false;
  }
  return Area() > Tolerance::kDegenerateArea;
}

bool CSGPolygon::IsConvex() const {
  if (vertices.size() < 3) {
    return false;
  }
  if (vertices.size() == 3) {
    return true; // Triangles are always convex
  }

  // Check that all cross products point in the same direction as the plane normal
  const CSGVertex& n = plane.normal;

  for (size_t i = 0; i < vertices.size(); ++i) {
    const CSGVertex& prev = vertices[(i + vertices.size() - 1) % vertices.size()];
    const CSGVertex& curr = vertices[i];
    const CSGVertex& next = vertices[(i + 1) % vertices.size()];

    CSGVertex e1 = curr - prev;
    CSGVertex e2 = next - curr;
    CSGVertex cross = e1.Cross(e2);

    if (cross.Dot(n) < -Tolerance::kEpsilon) {
      return false; // Reflex vertex found
    }
  }

  return true;
}

float CSGPolygon::Area() const {
  if (vertices.size() < 3) {
    return 0.0f;
  }

  // Sum of cross products method (works for non-planar polygons too)
  CSGVertex total(0.0f, 0.0f, 0.0f);
  const CSGVertex& v0 = vertices[0];

  for (size_t i = 1; i < vertices.size() - 1; ++i) {
    CSGVertex e1 = vertices[i] - v0;
    CSGVertex e2 = vertices[i + 1] - v0;
    total += e1.Cross(e2);
  }

  return total.Length() * 0.5f;
}

CSGVertex CSGPolygon::Centroid() const {
  if (vertices.empty()) {
    return CSGVertex(0.0f, 0.0f, 0.0f);
  }

  CSGVertex sum(0.0f, 0.0f, 0.0f);
  for (const auto& v : vertices) {
    sum += v;
  }
  return sum / static_cast<float>(vertices.size());
}

void CSGPolygon::Flip() {
  std::reverse(vertices.begin(), vertices.end());
  if (!uvs.empty()) {
    std::reverse(uvs.begin(), uvs.end());
  }
  plane = plane.Flipped();
}

PlaneSide CSGPolygon::ClassifyAgainstPlane(const CSGPlane& splitPlane) const {
  int frontCount = 0;
  int backCount = 0;
  int onCount = 0;

  for (const auto& v : vertices) {
    PlaneSide side = splitPlane.ClassifyPoint(v);
    switch (side) {
    case PlaneSide::Front:
      ++frontCount;
      break;
    case PlaneSide::Back:
      ++backCount;
      break;
    case PlaneSide::On:
      ++onCount;
      break;
    default:
      break;
    }
  }

  if (frontCount > 0 && backCount > 0) {
    return PlaneSide::Spanning;
  }
  if (frontCount > 0) {
    return PlaneSide::Front;
  }
  if (backCount > 0) {
    return PlaneSide::Back;
  }
  return PlaneSide::On;
}

// =============================================================================
// CSGBrush Implementation
// =============================================================================

namespace {

struct VertexKey {
  int32_t ix, iy, iz;

  bool operator==(const VertexKey& other) const { return ix == other.ix && iy == other.iy && iz == other.iz; }
};

struct VertexKeyHash {
  size_t operator()(const VertexKey& k) const {
    size_t h1 = std::hash<int32_t>()(k.ix);
    size_t h2 = std::hash<int32_t>()(k.iy);
    size_t h3 = std::hash<int32_t>()(k.iz);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

VertexKey MakeVertexKey(const CSGVertex& v, float cellSize) {
  return VertexKey{static_cast<int32_t>(std::floor(v.x / cellSize)),
                   static_cast<int32_t>(std::floor(v.y / cellSize)),
                   static_cast<int32_t>(std::floor(v.z / cellSize))};
}

struct EdgeKey {
  uint32_t v0, v1;

  EdgeKey(uint32_t a, uint32_t b) : v0(std::min(a, b)), v1(std::max(a, b)) {}

  bool operator==(const EdgeKey& other) const { return v0 == other.v0 && v1 == other.v1; }
};

struct EdgeKeyHash {
  size_t operator()(const EdgeKey& k) const {
    size_t h1 = std::hash<uint32_t>()(k.v0);
    size_t h2 = std::hash<uint32_t>()(k.v1);
    return h1 ^ (h2 << 1);
  }
};

} // namespace

CSGBrush CSGBrush::FromTriangleMesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) {
  CSGBrush brush;

  if (vertices.empty() || indices.empty() || indices.size() % 3 != 0) {
    return brush;
  }

  // Helper to get vertex from flat array
  auto getVertex = [&vertices](uint32_t idx) -> CSGVertex {
    return CSGVertex(vertices[idx * 3], vertices[idx * 3 + 1], vertices[idx * 3 + 2]);
  };

  // Group triangles by their plane normal
  // For now, treat each triangle as a separate polygon
  // TODO: Merge coplanar adjacent triangles into larger polygons

  for (size_t i = 0; i < indices.size(); i += 3) {
    CSGVertex v0 = getVertex(indices[i]);
    CSGVertex v1 = getVertex(indices[i + 1]);
    CSGVertex v2 = getVertex(indices[i + 2]);

    CSGPolygon poly;
    poly.vertices = {v0, v1, v2};

    if (poly.ComputePlane() && poly.IsValid()) {
      brush.polygons.push_back(std::move(poly));
    }
  }

  return brush;
}

void CSGBrush::ToTriangleMesh(std::vector<float>& out_vertices, std::vector<uint32_t>& out_indices) const {
  out_vertices.clear();
  out_indices.clear();

  // Use spatial hashing to weld duplicate vertices
  std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertex_map;
  float cellSize = Tolerance::kVertexWeld;

  auto addVertex = [&](const CSGVertex& v) -> uint32_t {
    VertexKey key = MakeVertexKey(v, cellSize);

    auto it = vertex_map.find(key);
    if (it != vertex_map.end()) {
      return it->second;
    }

    uint32_t idx = static_cast<uint32_t>(out_vertices.size() / 3);
    out_vertices.push_back(v.x);
    out_vertices.push_back(v.y);
    out_vertices.push_back(v.z);
    vertex_map[key] = idx;
    return idx;
  };

  // Triangulate each polygon using fan triangulation
  for (const auto& poly : polygons) {
    if (poly.vertices.size() < 3) {
      continue;
    }

    // Fan triangulation from first vertex
    uint32_t i0 = addVertex(poly.vertices[0]);
    for (size_t i = 1; i < poly.vertices.size() - 1; ++i) {
      uint32_t i1 = addVertex(poly.vertices[i]);
      uint32_t i2 = addVertex(poly.vertices[i + 1]);

      out_indices.push_back(i0);
      out_indices.push_back(i1);
      out_indices.push_back(i2);
    }
  }
}

void CSGBrush::ComputeBounds(CSGVertex& out_min, CSGVertex& out_max) const {
  out_min = CSGVertex(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max());
  out_max = CSGVertex(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                      std::numeric_limits<float>::lowest());

  for (const auto& poly : polygons) {
    for (const auto& v : poly.vertices) {
      out_min.x = std::min(out_min.x, v.x);
      out_min.y = std::min(out_min.y, v.y);
      out_min.z = std::min(out_min.z, v.z);
      out_max.x = std::max(out_max.x, v.x);
      out_max.y = std::max(out_max.y, v.y);
      out_max.z = std::max(out_max.z, v.z);
    }
  }
}

bool CSGBrush::IsValid() const {
  if (polygons.empty()) {
    return false;
  }
  return std::all_of(polygons.begin(), polygons.end(), [](const CSGPolygon& p) { return p.IsValid(); });
}

bool CSGBrush::IsManifold() const {
  if (polygons.size() < 4) {
    return false; // Minimum for closed 3D shape
  }

  // Build edge usage map - each edge should be used exactly twice
  std::unordered_map<EdgeKey, int, EdgeKeyHash> edge_counts;

  // First, assign unique indices to vertices using spatial hashing
  std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertex_map;
  float cellSize = Tolerance::kVertexWeld;

  auto getVertexIndex = [&](const CSGVertex& v) -> uint32_t {
    VertexKey key = MakeVertexKey(v, cellSize);
    auto it = vertex_map.find(key);
    if (it != vertex_map.end()) {
      return it->second;
    }
    uint32_t idx = static_cast<uint32_t>(vertex_map.size());
    vertex_map[key] = idx;
    return idx;
  };

  for (const auto& poly : polygons) {
    for (size_t i = 0; i < poly.vertices.size(); ++i) {
      uint32_t v0 = getVertexIndex(poly.vertices[i]);
      uint32_t v1 = getVertexIndex(poly.vertices[(i + 1) % poly.vertices.size()]);
      EdgeKey edge(v0, v1);
      edge_counts[edge]++;
    }
  }

  // Check all edges are used exactly twice
  for (const auto& [edge, count] : edge_counts) {
    if (count != 2) {
      return false;
    }
  }

  return true;
}

void CSGBrush::Invert() {
  for (auto& poly : polygons) {
    poly.Flip();
  }
}

CSGBrush CSGBrush::Clone() const {
  CSGBrush clone;
  clone.polygons = polygons;
  return clone;
}

size_t CSGBrush::TotalVertexCount() const {
  size_t count = 0;
  for (const auto& poly : polygons) {
    count += poly.vertices.size();
  }
  return count;
}

// =============================================================================
// Utility Functions
// =============================================================================

std::optional<CSGVertex> ComputeLinePlaneIntersection(const CSGVertex& v0, const CSGVertex& v1,
                                                       const CSGPlane& plane) {
  CSGVertex dir = v1 - v0;
  float denom = plane.normal.Dot(dir);

  if (std::abs(denom) < Tolerance::kEpsilon) {
    return std::nullopt; // Line is parallel to plane
  }

  float t = (plane.distance - plane.normal.Dot(v0)) / denom;

  // Allow slightly outside [0,1] to handle numerical precision
  if (t < -Tolerance::kEpsilon || t > 1.0f + Tolerance::kEpsilon) {
    return std::nullopt; // Intersection outside segment
  }

  return v0 + dir * t;
}

std::optional<CSGVertex> ComputeThreePlaneIntersection(const CSGPlane& p1, const CSGPlane& p2, const CSGPlane& p3) {
  // Using Cramer's rule to solve the system:
  // n1 . P = d1
  // n2 . P = d2
  // n3 . P = d3

  CSGVertex n1 = p1.normal;
  CSGVertex n2 = p2.normal;
  CSGVertex n3 = p3.normal;

  CSGVertex n2xn3 = n2.Cross(n3);
  float denom = n1.Dot(n2xn3);

  if (std::abs(denom) < Tolerance::kEpsilon) {
    return std::nullopt; // Planes don't intersect at a point (parallel or coincident)
  }

  CSGVertex n3xn1 = n3.Cross(n1);
  CSGVertex n1xn2 = n1.Cross(n2);

  CSGVertex result = (n2xn3 * p1.distance + n3xn1 * p2.distance + n1xn2 * p3.distance) / denom;
  return result;
}

} // namespace csg
