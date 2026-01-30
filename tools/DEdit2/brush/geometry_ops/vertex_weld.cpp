#include "vertex_weld.h"

#include <unordered_map>
#include <unordered_set>

namespace geometry_ops {

namespace {

/// Build a map from global vertex index to (polygon_index, local_vertex_index).
struct VertexLocation {
  size_t polygon_index;
  size_t local_index;
};

std::unordered_map<uint32_t, VertexLocation> BuildVertexIndexMap(const csg::CSGBrush& brush) {
  std::unordered_map<uint32_t, VertexLocation> map;
  uint32_t global_idx = 0;
  for (size_t pi = 0; pi < brush.polygons.size(); ++pi) {
    for (size_t vi = 0; vi < brush.polygons[pi].vertices.size(); ++vi) {
      map[global_idx] = {pi, vi};
      ++global_idx;
    }
  }
  return map;
}

/// Get the vertex at a global index.
csg::CSGVertex GetVertexByGlobalIndex(const csg::CSGBrush& brush, uint32_t global_idx) {
  uint32_t current = 0;
  for (const auto& poly : brush.polygons) {
    if (global_idx < current + poly.vertices.size()) {
      return poly.vertices[global_idx - current];
    }
    current += static_cast<uint32_t>(poly.vertices.size());
  }
  return csg::CSGVertex();
}

/// Compute the centroid of a set of vertices.
csg::CSGVertex ComputeCentroid(const csg::CSGBrush& brush, const std::vector<uint32_t>& vertex_indices) {
  if (vertex_indices.empty()) {
    return csg::CSGVertex();
  }

  csg::CSGVertex sum;
  for (uint32_t idx : vertex_indices) {
    sum += GetVertexByGlobalIndex(brush, idx);
  }
  return sum / static_cast<float>(vertex_indices.size());
}

/// Check if a polygon is degenerate (has fewer than 3 unique vertices).
bool IsPolygonDegenerate(const csg::CSGPolygon& poly) {
  if (poly.vertices.size() < 3) {
    return true;
  }

  // Count unique vertices (within tolerance)
  std::vector<csg::CSGVertex> unique;
  for (const auto& v : poly.vertices) {
    bool found = false;
    for (const auto& u : unique) {
      if (v.NearlyEquals(u, csg::Tolerance::kVertexWeld)) {
        found = true;
        break;
      }
    }
    if (!found) {
      unique.push_back(v);
    }
  }

  return unique.size() < 3;
}

/// Remove consecutive duplicate vertices from a polygon.
void RemoveConsecutiveDuplicates(csg::CSGPolygon& poly) {
  if (poly.vertices.size() < 2) {
    return;
  }

  std::vector<csg::CSGVertex> cleaned;
  cleaned.reserve(poly.vertices.size());

  for (size_t i = 0; i < poly.vertices.size(); ++i) {
    size_t next = (i + 1) % poly.vertices.size();
    if (!poly.vertices[i].NearlyEquals(poly.vertices[next], csg::Tolerance::kVertexWeld)) {
      cleaned.push_back(poly.vertices[i]);
    }
  }

  poly.vertices = std::move(cleaned);
}

} // namespace

VertexWeldResult WeldVertices(const csg::CSGBrush& brush, const std::vector<uint32_t>& vertex_indices,
                               const VertexWeldOptions& options) {
  VertexWeldResult result;

  if (brush.polygons.empty()) {
    result.success = false;
    result.error_message = "Cannot weld vertices on empty brush";
    return result;
  }

  if (vertex_indices.size() < 2) {
    // Nothing to weld - need at least 2 vertices
    result.success = true;
    result.result_brush = brush.Clone();
    result.vertices_merged = 0;
    return result;
  }

  // Build vertex index map
  auto index_map = BuildVertexIndexMap(brush);

  // Validate all indices and count total vertices
  uint32_t total_vertices = 0;
  for (const auto& poly : brush.polygons) {
    total_vertices += static_cast<uint32_t>(poly.vertices.size());
  }

  for (uint32_t idx : vertex_indices) {
    if (idx >= total_vertices) {
      result.success = false;
      result.error_message = "Vertex index " + std::to_string(idx) + " is out of range (brush has " +
                             std::to_string(total_vertices) + " vertices)";
      return result;
    }
  }

  // Compute the centroid of the selected vertices
  csg::CSGVertex centroid = ComputeCentroid(brush, vertex_indices);

  // Create set of indices to weld for fast lookup
  std::unordered_set<uint32_t> weld_set(vertex_indices.begin(), vertex_indices.end());

  // Clone brush and replace vertices
  result.result_brush = brush.Clone();

  uint32_t global_idx = 0;
  for (auto& poly : result.result_brush.polygons) {
    for (auto& v : poly.vertices) {
      if (weld_set.count(global_idx) > 0) {
        v = centroid;
      }
      ++global_idx;
    }
  }

  // Clean up polygons
  size_t removed_count = 0;
  std::vector<csg::CSGPolygon> cleaned_polygons;
  cleaned_polygons.reserve(result.result_brush.polygons.size());

  for (auto& poly : result.result_brush.polygons) {
    // Remove consecutive duplicates
    RemoveConsecutiveDuplicates(poly);

    // Check if polygon is still valid
    if (!IsPolygonDegenerate(poly)) {
      // Recompute plane after vertex changes
      poly.ComputePlane();
      cleaned_polygons.push_back(std::move(poly));
    } else {
      ++removed_count;
    }
  }

  result.result_brush.polygons = std::move(cleaned_polygons);
  result.vertices_merged = vertex_indices.size();
  result.degenerate_faces_removed = removed_count;
  result.success = true;

  return result;
}

std::vector<std::vector<uint32_t>> FindWeldableVertices(const csg::CSGBrush& brush, float threshold) {
  std::vector<std::vector<uint32_t>> groups;

  if (brush.polygons.empty()) {
    return groups;
  }

  // Collect all vertices with their global indices
  std::vector<std::pair<uint32_t, csg::CSGVertex>> all_vertices;
  uint32_t global_idx = 0;
  for (const auto& poly : brush.polygons) {
    for (const auto& v : poly.vertices) {
      all_vertices.emplace_back(global_idx, v);
      ++global_idx;
    }
  }

  const float threshold_sq = threshold * threshold;

  // Track which vertices have been assigned to groups
  std::vector<bool> visited(all_vertices.size(), false);

  // Use BFS to find connected components (transitive grouping)
  for (size_t start = 0; start < all_vertices.size(); ++start) {
    if (visited[start]) {
      continue;
    }

    // BFS to find all vertices transitively connected to this one
    std::vector<uint32_t> group;
    std::vector<size_t> queue;
    queue.push_back(start);
    visited[start] = true;

    while (!queue.empty()) {
      size_t current = queue.back();
      queue.pop_back();
      group.push_back(all_vertices[current].first);

      // Find all unvisited vertices within threshold of current vertex
      for (size_t j = 0; j < all_vertices.size(); ++j) {
        if (visited[j]) {
          continue;
        }

        float dist_sq = (all_vertices[current].second - all_vertices[j].second).LengthSquared();
        if (dist_sq < threshold_sq) {
          visited[j] = true;
          queue.push_back(j);
        }
      }
    }

    // Only add groups with 2+ vertices
    if (group.size() >= 2) {
      groups.push_back(std::move(group));
    }
  }

  return groups;
}

} // namespace geometry_ops
