#include "csg_hollow.h"

#include "csg_triangulate.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace csg {

namespace {

/// Quantize a vertex to a grid cell for consistent hashing/equality.
/// Returns the grid cell indices for the vertex.
struct VertexGridKey {
  int32_t ix, iy, iz;

  static VertexGridKey FromVertex(const CSGVertex& v) {
    return {static_cast<int32_t>(std::floor(v.x / Tolerance::kVertexWeld)),
            static_cast<int32_t>(std::floor(v.y / Tolerance::kVertexWeld)),
            static_cast<int32_t>(std::floor(v.z / Tolerance::kVertexWeld))};
  }

  bool operator==(const VertexGridKey& other) const { return ix == other.ix && iy == other.iy && iz == other.iz; }
};

/// Hash for vertex position used in adjacency building.
/// Uses grid-based quantization - vertices in the same grid cell are considered equal.
struct VertexHash {
  size_t operator()(const CSGVertex& v) const {
    auto key = VertexGridKey::FromVertex(v);
    return std::hash<int32_t>()(key.ix) ^ (std::hash<int32_t>()(key.iy) << 1) ^ (std::hash<int32_t>()(key.iz) << 2);
  }
};

/// Equality for vertex position - must be consistent with VertexHash.
/// Two vertices are equal if and only if they quantize to the same grid cell.
struct VertexEqual {
  bool operator()(const CSGVertex& a, const CSGVertex& b) const {
    return VertexGridKey::FromVertex(a) == VertexGridKey::FromVertex(b);
  }
};

/// Build a map from each vertex to the polygons that contain it.
std::unordered_map<CSGVertex, std::vector<size_t>, VertexHash, VertexEqual>
BuildVertexToPolygonMap(const std::vector<CSGPolygon>& polygons) {
  std::unordered_map<CSGVertex, std::vector<size_t>, VertexHash, VertexEqual> result;

  for (size_t poly_idx = 0; poly_idx < polygons.size(); ++poly_idx) {
    for (const auto& v : polygons[poly_idx].vertices) {
      result[v].push_back(poly_idx);
    }
  }

  return result;
}

/// Find planes of faces adjacent to a vertex.
std::vector<CSGPlane> GetAdjacentPlanes(const CSGVertex& vertex,
                                         const std::unordered_map<CSGVertex, std::vector<size_t>, VertexHash,
                                                                   VertexEqual>& vertex_map,
                                         const std::vector<CSGPolygon>& polygons) {
  std::vector<CSGPlane> planes;

  auto it = vertex_map.find(vertex);
  if (it == vertex_map.end()) {
    return planes;
  }

  for (size_t poly_idx : it->second) {
    const CSGPlane& plane = polygons[poly_idx].plane;

    // Check if we already have a nearly coincident plane
    bool duplicate = false;
    for (const auto& existing : planes) {
      if (plane.IsCoincidentWith(existing)) {
        duplicate = true;
        break;
      }
    }

    if (!duplicate) {
      planes.push_back(plane);
    }
  }

  return planes;
}

} // namespace

std::optional<CSGVertex> ComputeOffsetVertex(const CSGVertex& vertex, const std::vector<CSGPlane>& planes,
                                              float thickness) {
  if (planes.size() < 3) {
    // Not enough planes to determine a unique point
    // Fall back to simple normal-based offset
    if (planes.empty()) {
      return std::nullopt;
    }

    // Use average normal
    CSGVertex avg_normal(0.0f, 0.0f, 0.0f);
    for (const auto& p : planes) {
      avg_normal += p.normal;
    }
    avg_normal = avg_normal.Normalized();

    return vertex - avg_normal * thickness;
  }

  // Offset each plane by thickness and find intersection
  std::vector<CSGPlane> offset_planes;
  offset_planes.reserve(planes.size());
  for (const auto& p : planes) {
    // Offset plane inward (subtract from distance since we're offsetting along negative normal)
    offset_planes.push_back(p.Offset(-thickness));
  }

  // Try to find intersection of first 3 planes
  auto result = ComputeThreePlaneIntersection(offset_planes[0], offset_planes[1], offset_planes[2]);

  if (!result) {
    // Planes might be nearly parallel - try other combinations
    for (size_t i = 0; i < offset_planes.size() && !result; ++i) {
      for (size_t j = i + 1; j < offset_planes.size() && !result; ++j) {
        for (size_t k = j + 1; k < offset_planes.size() && !result; ++k) {
          result = ComputeThreePlaneIntersection(offset_planes[i], offset_planes[j], offset_planes[k]);
        }
      }
    }
  }

  return result;
}

std::string ValidateHollowThickness(const CSGBrush& brush, float thickness) {
  if (std::abs(thickness) < Tolerance::kEpsilon) {
    return "Wall thickness cannot be zero";
  }

  // Compute brush bounds
  CSGVertex min_bound, max_bound;
  brush.ComputeBounds(min_bound, max_bound);

  float size_x = max_bound.x - min_bound.x;
  float size_y = max_bound.y - min_bound.y;
  float size_z = max_bound.z - min_bound.z;

  float min_dim = std::min({size_x, size_y, size_z});

  // Thickness should be less than half the smallest dimension
  float abs_thickness = std::abs(thickness);
  if (abs_thickness >= min_dim * 0.5f) {
    return "Wall thickness exceeds half the brush's smallest dimension";
  }

  return "";
}

CSGBrush CreateWallSlab(const CSGPolygon& outer_poly, float thickness, const std::vector<CSGPlane>& adjacent_planes) {
  CSGBrush wall;

  if (outer_poly.vertices.size() < 3) {
    return wall;
  }

  // Create inner polygon by offsetting each vertex
  std::vector<CSGVertex> inner_verts;
  inner_verts.reserve(outer_poly.vertices.size());

  for (const auto& v : outer_poly.vertices) {
    // Simple offset along face normal for now
    // A more sophisticated implementation would use adjacent_planes
    CSGVertex offset_v = v - outer_poly.plane.normal * thickness;
    inner_verts.push_back(offset_v);
  }

  // Create outer face (same as original)
  wall.polygons.push_back(outer_poly);

  // Create inner face (reversed winding)
  CSGPolygon inner_poly;
  inner_poly.vertices = inner_verts;
  inner_poly.material_id = outer_poly.material_id;
  inner_poly.ComputePlane();
  inner_poly.Flip(); // Face inward
  wall.polygons.push_back(std::move(inner_poly));

  // Create side faces connecting outer and inner edges
  size_t n = outer_poly.vertices.size();
  for (size_t i = 0; i < n; ++i) {
    size_t next = (i + 1) % n;

    // Quad vertices: outer[i], outer[next], inner[next], inner[i]
    CSGPolygon side;
    side.vertices = {outer_poly.vertices[i], outer_poly.vertices[next], inner_verts[next], inner_verts[i]};
    side.material_id = outer_poly.material_id;

    if (side.ComputePlane() && side.IsValid()) {
      wall.polygons.push_back(std::move(side));
    }
  }

  return wall;
}

HollowResult HollowBrush(const CSGBrush& brush, const HollowOptions& options) {
  HollowResult result;

  if (!brush.IsValid()) {
    result.error_message = "Invalid input brush";
    return result;
  }

  // Validate thickness
  std::string error = ValidateHollowThickness(brush, options.wall_thickness);
  if (!error.empty()) {
    result.error_message = error;
    return result;
  }

  // Build vertex-to-polygon adjacency map
  auto vertex_map = BuildVertexToPolygonMap(brush.polygons);

  // Create a wall slab for each face
  for (size_t i = 0; i < brush.polygons.size(); ++i) {
    // Check face mask if not hollowing all faces
    if (!options.hollow_all_faces) {
      if (i < 32 && (options.face_mask & (1u << i)) == 0) {
        continue; // Skip this face
      }
    }

    const CSGPolygon& face = brush.polygons[i];

    // Get adjacent planes for this face's vertices
    std::vector<CSGPlane> adjacent_planes;
    for (const auto& v : face.vertices) {
      auto planes = GetAdjacentPlanes(v, vertex_map, brush.polygons);
      for (const auto& p : planes) {
        // Only add if not already present
        bool found = false;
        for (const auto& existing : adjacent_planes) {
          if (p.IsCoincidentWith(existing)) {
            found = true;
            break;
          }
        }
        if (!found) {
          adjacent_planes.push_back(p);
        }
      }
    }

    // Create wall slab
    CSGBrush wall = CreateWallSlab(face, options.wall_thickness, adjacent_planes);

    if (!wall.polygons.empty()) {
      result.wall_brushes.push_back(std::move(wall));
    }
  }

  if (result.wall_brushes.empty()) {
    result.error_message = "Hollow operation produced no valid walls";
    return result;
  }

  result.success = true;
  return result;
}

CSGOperationResult HollowBrush(const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
                                float wall_thickness) {
  // Validate input
  if (vertices.empty() || vertices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid vertex data");
  }
  if (indices.empty() || indices.size() % 3 != 0) {
    return CSGOperationResult::Failure("Invalid index data");
  }

  // Convert to CSG representation
  CSGBrush brush = CSGBrush::FromTriangleMesh(vertices, indices);
  if (!brush.IsValid()) {
    return CSGOperationResult::Failure("Failed to create brush from mesh");
  }

  // Perform hollow
  HollowOptions options;
  options.wall_thickness = wall_thickness;

  HollowResult hollow_result = HollowBrush(brush, options);
  if (!hollow_result.success) {
    return CSGOperationResult::Failure(hollow_result.error_message);
  }

  // Convert wall brushes to result
  return CSGOperationResult::Success(std::move(hollow_result.wall_brushes));
}

} // namespace csg
