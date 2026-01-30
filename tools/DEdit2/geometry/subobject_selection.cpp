#include "subobject_selection.h"

#include "editor_state.h"

#include <unordered_set>

void SubObjectSelection::ClearVertices() {
  selected_vertices.clear();
  hovered_vertex.reset();
  primary_vertex.reset();
}

void SubObjectSelection::ClearEdges() {
  selected_edges.clear();
  hovered_edge.reset();
  primary_edge.reset();
}

void SubObjectSelection::ClearFaces() {
  selected_faces.clear();
  hovered_face.reset();
  primary_face.reset();
}

void SubObjectSelection::ClearAll() {
  ClearVertices();
  ClearEdges();
  ClearFaces();
}

void SubObjectSelection::ClearForNode(int node_id) {
  // Clear vertices for this node
  for (auto it = selected_vertices.begin(); it != selected_vertices.end();) {
    if (it->node_id == node_id) {
      it = selected_vertices.erase(it);
    } else {
      ++it;
    }
  }
  if (hovered_vertex && hovered_vertex->node_id == node_id) {
    hovered_vertex.reset();
  }
  if (primary_vertex && primary_vertex->node_id == node_id) {
    primary_vertex.reset();
    // Find new primary if any vertices remain selected
    if (!selected_vertices.empty()) {
      primary_vertex = *selected_vertices.begin();
    }
  }

  // Clear edges for this node
  for (auto it = selected_edges.begin(); it != selected_edges.end();) {
    if (it->node_id == node_id) {
      it = selected_edges.erase(it);
    } else {
      ++it;
    }
  }
  if (hovered_edge && hovered_edge->node_id == node_id) {
    hovered_edge.reset();
  }
  if (primary_edge && primary_edge->node_id == node_id) {
    primary_edge.reset();
    if (!selected_edges.empty()) {
      primary_edge = *selected_edges.begin();
    }
  }

  // Clear faces for this node
  for (auto it = selected_faces.begin(); it != selected_faces.end();) {
    if (it->node_id == node_id) {
      it = selected_faces.erase(it);
    } else {
      ++it;
    }
  }
  if (hovered_face && hovered_face->node_id == node_id) {
    hovered_face.reset();
  }
  if (primary_face && primary_face->node_id == node_id) {
    primary_face.reset();
    if (!selected_faces.empty()) {
      primary_face = *selected_faces.begin();
    }
  }
}

bool SubObjectSelection::HasVertexSelection() const { return !selected_vertices.empty(); }

bool SubObjectSelection::HasEdgeSelection() const { return !selected_edges.empty(); }

bool SubObjectSelection::HasFaceSelection() const { return !selected_faces.empty(); }

bool SubObjectSelection::HasAnySelection() const {
  return HasVertexSelection() || HasEdgeSelection() || HasFaceSelection();
}

size_t SubObjectSelection::VertexCount() const { return selected_vertices.size(); }

size_t SubObjectSelection::EdgeCount() const { return selected_edges.size(); }

size_t SubObjectSelection::FaceCount() const { return selected_faces.size(); }

std::vector<int> SubObjectSelection::GetNodesWithSelectedVertices() const {
  std::unordered_set<int> nodes;
  for (const auto& ref : selected_vertices) {
    nodes.insert(ref.node_id);
  }
  return std::vector<int>(nodes.begin(), nodes.end());
}

std::vector<int> SubObjectSelection::GetNodesWithSelectedEdges() const {
  std::unordered_set<int> nodes;
  for (const auto& ref : selected_edges) {
    nodes.insert(ref.node_id);
  }
  return std::vector<int>(nodes.begin(), nodes.end());
}

std::vector<int> SubObjectSelection::GetNodesWithSelectedFaces() const {
  std::unordered_set<int> nodes;
  for (const auto& ref : selected_faces) {
    nodes.insert(ref.node_id);
  }
  return std::vector<int>(nodes.begin(), nodes.end());
}

std::optional<std::array<float, 3>> SubObjectSelection::ComputeVertexSelectionCenter(
    const std::vector<NodeProperties>& props) const {
  if (selected_vertices.empty()) {
    return std::nullopt;
  }

  float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
  size_t count = 0;

  for (const auto& ref : selected_vertices) {
    if (ref.node_id < 0 || static_cast<size_t>(ref.node_id) >= props.size()) {
      continue;
    }
    const auto& p = props[static_cast<size_t>(ref.node_id)];
    size_t base = static_cast<size_t>(ref.vertex_index) * 3;
    if (base + 2 >= p.brush_vertices.size()) {
      continue;
    }
    sum_x += p.brush_vertices[base];
    sum_y += p.brush_vertices[base + 1];
    sum_z += p.brush_vertices[base + 2];
    ++count;
  }

  if (count == 0) {
    return std::nullopt;
  }

  float inv_count = 1.0f / static_cast<float>(count);
  return std::array<float, 3>{sum_x * inv_count, sum_y * inv_count, sum_z * inv_count};
}

std::optional<std::array<float, 3>> SubObjectSelection::ComputeEdgeSelectionCenter(
    const std::vector<NodeProperties>& props) const {
  if (selected_edges.empty()) {
    return std::nullopt;
  }

  float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
  size_t count = 0;

  for (const auto& ref : selected_edges) {
    if (ref.node_id < 0 || static_cast<size_t>(ref.node_id) >= props.size()) {
      continue;
    }
    const auto& p = props[static_cast<size_t>(ref.node_id)];
    size_t base_a = static_cast<size_t>(ref.vertex_a) * 3;
    size_t base_b = static_cast<size_t>(ref.vertex_b) * 3;
    if (base_a + 2 >= p.brush_vertices.size() || base_b + 2 >= p.brush_vertices.size()) {
      continue;
    }
    // Edge midpoint
    sum_x += (p.brush_vertices[base_a] + p.brush_vertices[base_b]) * 0.5f;
    sum_y += (p.brush_vertices[base_a + 1] + p.brush_vertices[base_b + 1]) * 0.5f;
    sum_z += (p.brush_vertices[base_a + 2] + p.brush_vertices[base_b + 2]) * 0.5f;
    ++count;
  }

  if (count == 0) {
    return std::nullopt;
  }

  float inv_count = 1.0f / static_cast<float>(count);
  return std::array<float, 3>{sum_x * inv_count, sum_y * inv_count, sum_z * inv_count};
}

std::optional<std::array<float, 3>> SubObjectSelection::ComputeFaceSelectionCenter(
    const std::vector<NodeProperties>& props) const {
  if (selected_faces.empty()) {
    return std::nullopt;
  }

  float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
  size_t count = 0;

  for (const auto& ref : selected_faces) {
    if (ref.node_id < 0 || static_cast<size_t>(ref.node_id) >= props.size()) {
      continue;
    }
    const auto& p = props[static_cast<size_t>(ref.node_id)];
    size_t idx_base = static_cast<size_t>(ref.triangle_index) * 3;
    if (idx_base + 2 >= p.brush_indices.size()) {
      continue;
    }

    // Get triangle vertices
    uint32_t i0 = p.brush_indices[idx_base];
    uint32_t i1 = p.brush_indices[idx_base + 1];
    uint32_t i2 = p.brush_indices[idx_base + 2];

    size_t v0_base = static_cast<size_t>(i0) * 3;
    size_t v1_base = static_cast<size_t>(i1) * 3;
    size_t v2_base = static_cast<size_t>(i2) * 3;

    if (v0_base + 2 >= p.brush_vertices.size() || v1_base + 2 >= p.brush_vertices.size() ||
        v2_base + 2 >= p.brush_vertices.size()) {
      continue;
    }

    // Triangle centroid
    float cx = (p.brush_vertices[v0_base] + p.brush_vertices[v1_base] + p.brush_vertices[v2_base]) / 3.0f;
    float cy =
        (p.brush_vertices[v0_base + 1] + p.brush_vertices[v1_base + 1] + p.brush_vertices[v2_base + 1]) / 3.0f;
    float cz =
        (p.brush_vertices[v0_base + 2] + p.brush_vertices[v1_base + 2] + p.brush_vertices[v2_base + 2]) / 3.0f;

    sum_x += cx;
    sum_y += cy;
    sum_z += cz;
    ++count;
  }

  if (count == 0) {
    return std::nullopt;
  }

  float inv_count = 1.0f / static_cast<float>(count);
  return std::array<float, 3>{sum_x * inv_count, sum_y * inv_count, sum_z * inv_count};
}

void SubObjectSelection::SelectVertex(const VertexRef& ref) {
  selected_vertices.clear();
  selected_vertices.insert(ref);
  primary_vertex = ref;
}

void SubObjectSelection::AddVertex(const VertexRef& ref) {
  selected_vertices.insert(ref);
  if (!primary_vertex) {
    primary_vertex = ref;
  }
}

void SubObjectSelection::RemoveVertex(const VertexRef& ref) {
  selected_vertices.erase(ref);
  if (primary_vertex && *primary_vertex == ref) {
    primary_vertex = selected_vertices.empty() ? std::nullopt : std::optional(*selected_vertices.begin());
  }
}

void SubObjectSelection::ToggleVertex(const VertexRef& ref) {
  if (IsVertexSelected(ref)) {
    RemoveVertex(ref);
  } else {
    AddVertex(ref);
  }
}

void SubObjectSelection::SelectEdge(const EdgeRef& ref) {
  selected_edges.clear();
  selected_edges.insert(ref);
  primary_edge = ref;
}

void SubObjectSelection::AddEdge(const EdgeRef& ref) {
  selected_edges.insert(ref);
  if (!primary_edge) {
    primary_edge = ref;
  }
}

void SubObjectSelection::RemoveEdge(const EdgeRef& ref) {
  selected_edges.erase(ref);
  if (primary_edge && *primary_edge == ref) {
    primary_edge = selected_edges.empty() ? std::nullopt : std::optional(*selected_edges.begin());
  }
}

void SubObjectSelection::ToggleEdge(const EdgeRef& ref) {
  if (IsEdgeSelected(ref)) {
    RemoveEdge(ref);
  } else {
    AddEdge(ref);
  }
}

void SubObjectSelection::SelectFace(const FaceRef& ref) {
  selected_faces.clear();
  selected_faces.insert(ref);
  primary_face = ref;
}

void SubObjectSelection::AddFace(const FaceRef& ref) {
  selected_faces.insert(ref);
  if (!primary_face) {
    primary_face = ref;
  }
}

void SubObjectSelection::RemoveFace(const FaceRef& ref) {
  selected_faces.erase(ref);
  if (primary_face && *primary_face == ref) {
    primary_face = selected_faces.empty() ? std::nullopt : std::optional(*selected_faces.begin());
  }
}

void SubObjectSelection::ToggleFace(const FaceRef& ref) {
  if (IsFaceSelected(ref)) {
    RemoveFace(ref);
  } else {
    AddFace(ref);
  }
}

bool SubObjectSelection::IsVertexSelected(const VertexRef& ref) const {
  return selected_vertices.find(ref) != selected_vertices.end();
}

bool SubObjectSelection::IsEdgeSelected(const EdgeRef& ref) const {
  return selected_edges.find(ref) != selected_edges.end();
}

bool SubObjectSelection::IsFaceSelected(const FaceRef& ref) const {
  return selected_faces.find(ref) != selected_faces.end();
}
