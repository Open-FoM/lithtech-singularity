#pragma once

#include <cstdint>
#include <functional>

/// Reference to a vertex within a brush's geometry.
/// The vertex_index is the index into the brush's unique vertex list (brush_vertices / 3).
struct VertexRef {
  int node_id = -1;            ///< The brush node ID
  uint32_t vertex_index = 0;   ///< Index into brush_vertices (index * 3 = base offset)

  [[nodiscard]] bool operator==(const VertexRef& other) const {
    return node_id == other.node_id && vertex_index == other.vertex_index;
  }

  [[nodiscard]] bool operator!=(const VertexRef& other) const { return !(*this == other); }

  [[nodiscard]] bool IsValid() const { return node_id >= 0; }
};

/// Reference to an edge within a brush's geometry.
/// An edge is defined by two vertex indices. The indices are normalized so that
/// vertex_a < vertex_b for consistent hashing and comparison.
struct EdgeRef {
  int node_id = -1;
  uint32_t vertex_a = 0;  ///< First vertex index (always < vertex_b)
  uint32_t vertex_b = 0;  ///< Second vertex index (always > vertex_a)

  /// Create an EdgeRef with normalized vertex order.
  static EdgeRef Create(int node_id, uint32_t v0, uint32_t v1) {
    EdgeRef ref;
    ref.node_id = node_id;
    if (v0 < v1) {
      ref.vertex_a = v0;
      ref.vertex_b = v1;
    } else {
      ref.vertex_a = v1;
      ref.vertex_b = v0;
    }
    return ref;
  }

  [[nodiscard]] bool operator==(const EdgeRef& other) const {
    return node_id == other.node_id && vertex_a == other.vertex_a && vertex_b == other.vertex_b;
  }

  [[nodiscard]] bool operator!=(const EdgeRef& other) const { return !(*this == other); }

  [[nodiscard]] bool IsValid() const { return node_id >= 0 && vertex_a != vertex_b; }
};

/// Reference to a face (triangle) within a brush's geometry.
/// The triangle_index is the index into the brush's triangle list (brush_indices / 3).
struct FaceRef {
  int node_id = -1;
  uint32_t triangle_index = 0;  ///< Index into brush_indices (index * 3 = base offset)

  [[nodiscard]] bool operator==(const FaceRef& other) const {
    return node_id == other.node_id && triangle_index == other.triangle_index;
  }

  [[nodiscard]] bool operator!=(const FaceRef& other) const { return !(*this == other); }

  [[nodiscard]] bool IsValid() const { return node_id >= 0; }
};

// Hash functions for use in unordered containers

struct VertexRefHash {
  std::size_t operator()(const VertexRef& ref) const {
    // Combine node_id and vertex_index into a single hash
    std::size_t h1 = std::hash<int>{}(ref.node_id);
    std::size_t h2 = std::hash<uint32_t>{}(ref.vertex_index);
    return h1 ^ (h2 << 1);
  }
};

struct EdgeRefHash {
  std::size_t operator()(const EdgeRef& ref) const {
    std::size_t h1 = std::hash<int>{}(ref.node_id);
    std::size_t h2 = std::hash<uint32_t>{}(ref.vertex_a);
    std::size_t h3 = std::hash<uint32_t>{}(ref.vertex_b);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

struct FaceRefHash {
  std::size_t operator()(const FaceRef& ref) const {
    std::size_t h1 = std::hash<int>{}(ref.node_id);
    std::size_t h2 = std::hash<uint32_t>{}(ref.triangle_index);
    return h1 ^ (h2 << 1);
  }
};
