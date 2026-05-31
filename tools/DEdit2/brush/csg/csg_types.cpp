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

uint64_t OrderedEdgeKey(uint32_t v0, uint32_t v1) {
  uint32_t lo = std::min(v0, v1);
  uint32_t hi = std::max(v0, v1);
  return (static_cast<uint64_t>(lo) << 32) | static_cast<uint64_t>(hi);
}

uint64_t DirectedEdgeKey(uint32_t from, uint32_t to) {
  return (static_cast<uint64_t>(from) << 32) | static_cast<uint64_t>(to);
}

struct PlanePolygonGroup {
  CSGPlane plane;
  std::vector<CSGPolygon> polygons;
};

std::vector<CSGPolygon> MergeCoplanarTriangles(const std::vector<CSGPolygon>& triangles) {
  if (triangles.empty()) {
    return {};
  }
  if (triangles.size() == 1) {
    return triangles;
  }

  std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertex_ids;
  std::vector<CSGVertex> ordered_vertices;
  ordered_vertices.reserve(triangles.size() * 3);

  auto getVertexId = [&](const CSGVertex& v) -> uint32_t {
    VertexKey key = MakeVertexKey(v, Tolerance::kVertexWeld);
    auto it = vertex_ids.find(key);
    if (it != vertex_ids.end()) {
      return it->second;
    }

    uint32_t id = static_cast<uint32_t>(ordered_vertices.size());
    vertex_ids[key] = id;
    ordered_vertices.push_back(v);
    return id;
  };

  std::unordered_map<uint64_t, int> edge_counts;
  std::vector<std::pair<uint32_t, uint32_t>> edges;
  edges.reserve(triangles.size() * 3);

  for (const auto& tri : triangles) {
    for (size_t i = 0; i < 3; ++i) {
      uint32_t a = getVertexId(tri.vertices[i]);
      uint32_t b = getVertexId(tri.vertices[(i + 1) % 3]);
      edges.emplace_back(a, b);
      ++edge_counts[OrderedEdgeKey(a, b)];
    }
  }

  std::vector<std::pair<uint32_t, uint32_t>> boundary_edges;
  boundary_edges.reserve(edges.size());

  for (const auto& edge : edges) {
    if (edge_counts[OrderedEdgeKey(edge.first, edge.second)] == 1) {
      boundary_edges.push_back(edge);
    }
  }
  if (boundary_edges.empty()) {
    return {};
  }

  std::unordered_map<uint32_t, std::vector<uint32_t>> outgoing;
  for (const auto& edge : boundary_edges) {
    outgoing[edge.first].push_back(edge.second);
  }

  for (const auto& pair : outgoing) {
    // In expected closed-loop input, each boundary vertex appears once as a start of a directed edge.
    if (pair.second.size() != 1) {
      return {};
    }
  }

  std::unordered_set<uint64_t> visited;
  std::vector<std::vector<uint32_t>> loops;

  for (const auto& edge : boundary_edges) {
    uint64_t start_key = DirectedEdgeKey(edge.first, edge.second);
    if (visited.find(start_key) != visited.end()) {
      continue;
    }

    std::vector<uint32_t> loop;
    loop.reserve(boundary_edges.size());

    uint32_t prev = edge.first;
    uint32_t current = edge.second;
    loop.push_back(prev);
    loop.push_back(current);
    visited.insert(start_key);

    for (size_t step = 0; step < boundary_edges.size() + 2; ++step) {
      auto it = outgoing.find(current);
      if (it == outgoing.end() || it->second.empty()) {
        return {};
      }

      uint32_t next = it->second.front();
      if (next == prev && it->second.size() > 1) {
        next = it->second.back();
      }
      if (next == prev) {
        return {};
      }

      uint64_t next_key = DirectedEdgeKey(current, next);
      if (next == loop.front()) {
        visited.insert(next_key);
        break;
      }
      if (visited.find(next_key) != visited.end()) {
        return {};
      }

      visited.insert(next_key);
      loop.push_back(next);
      prev = current;
      current = next;
    }

    if (loop.size() < 3) {
      return {};
    }
    loops.push_back(std::move(loop));
  }

  std::vector<CSGPolygon> merged_polygons;
  merged_polygons.reserve(loops.size());

  for (auto& loop : loops) {
    CSGPolygon merged_poly;
    merged_poly.vertices.reserve(loop.size());
    for (uint32_t id : loop) {
      merged_poly.vertices.push_back(ordered_vertices[id]);
    }
    if (!merged_poly.ComputePlane() || !merged_poly.IsValid()) {
      return {};
    }
    if (merged_poly.plane.normal.Dot(triangles[0].plane.normal) < 0.0f) {
      merged_poly.Flip();
    }
    merged_polygons.push_back(std::move(merged_poly));
  }

  return merged_polygons;
}

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

  std::vector<CSGPolygon> triangles;
  triangles.reserve(indices.size() / 3);

  for (size_t i = 0; i < indices.size(); i += 3) {
    CSGVertex v0 = getVertex(indices[i]);
    CSGVertex v1 = getVertex(indices[i + 1]);
    CSGVertex v2 = getVertex(indices[i + 2]);

    CSGPolygon poly;
    poly.vertices = {v0, v1, v2};

    if (poly.ComputePlane() && poly.IsValid()) {
      triangles.push_back(std::move(poly));
    }
  }

  // Group triangles by coplanar planes then merge each connected set into larger faces.
  std::vector<PlanePolygonGroup> plane_groups;
  for (auto& tri : triangles) {
    bool merged = false;
    for (auto& group : plane_groups) {
      if (tri.plane.IsCoincidentWith(group.plane)) {
        if (tri.plane.normal.Dot(group.plane.normal) < 0.0f) {
          tri.Flip();
        }
        group.polygons.push_back(std::move(tri));
        merged = true;
        break;
      }
    }

    if (!merged) {
      plane_groups.push_back({tri.plane, {std::move(tri)}});
    }
  }

  for (auto& group : plane_groups) {
    std::vector<CSGPolygon> merged_polygons = MergeCoplanarTriangles(group.polygons);
    if (!merged_polygons.empty()) {
      for (auto& polygon : merged_polygons) {
        brush.polygons.push_back(std::move(polygon));
      }
      continue;
    }

    for (auto& polygon : group.polygons) {
      brush.polygons.push_back(std::move(polygon));
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
