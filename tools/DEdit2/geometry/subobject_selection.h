#pragma once

#include "subobject_refs.h"

#include <array>
#include <optional>
#include <unordered_set>
#include <vector>

struct NodeProperties;

/// Selection state for geometry editing modes.
/// Maintains separate selections for vertices, edges, and faces.
/// Only one type of selection is active at a time based on the current EditMode.
class SubObjectSelection {
public:
  // Vertex selection (active in Vertex mode)
  std::unordered_set<VertexRef, VertexRefHash> selected_vertices;
  std::optional<VertexRef> hovered_vertex;
  std::optional<VertexRef> primary_vertex;

  // Edge selection (active in Edge mode)
  std::unordered_set<EdgeRef, EdgeRefHash> selected_edges;
  std::optional<EdgeRef> hovered_edge;
  std::optional<EdgeRef> primary_edge;

  // Face selection (active in Face mode)
  std::unordered_set<FaceRef, FaceRefHash> selected_faces;
  std::optional<FaceRef> hovered_face;
  std::optional<FaceRef> primary_face;

  /// Clear all vertex selection.
  void ClearVertices();

  /// Clear all edge selection.
  void ClearEdges();

  /// Clear all face selection.
  void ClearFaces();

  /// Clear all selections (vertices, edges, faces).
  void ClearAll();

  /// Clear selection for elements belonging to a specific node.
  /// Called when a node is deleted or deselected in object mode.
  void ClearForNode(int node_id);

  /// Check if any vertices are selected.
  [[nodiscard]] bool HasVertexSelection() const;

  /// Check if any edges are selected.
  [[nodiscard]] bool HasEdgeSelection() const;

  /// Check if any faces are selected.
  [[nodiscard]] bool HasFaceSelection() const;

  /// Check if there is any selection at all.
  [[nodiscard]] bool HasAnySelection() const;

  /// Get the count of selected vertices.
  [[nodiscard]] size_t VertexCount() const;

  /// Get the count of selected edges.
  [[nodiscard]] size_t EdgeCount() const;

  /// Get the count of selected faces.
  [[nodiscard]] size_t FaceCount() const;

  /// Get unique node IDs that have selected vertices.
  [[nodiscard]] std::vector<int> GetNodesWithSelectedVertices() const;

  /// Get unique node IDs that have selected edges.
  [[nodiscard]] std::vector<int> GetNodesWithSelectedEdges() const;

  /// Get unique node IDs that have selected faces.
  [[nodiscard]] std::vector<int> GetNodesWithSelectedFaces() const;

  /// Compute the center position of selected vertices.
  /// @param props Node properties array to look up vertex positions.
  /// @return Center position as [x, y, z], or nullopt if no selection.
  [[nodiscard]] std::optional<std::array<float, 3>> ComputeVertexSelectionCenter(
      const std::vector<NodeProperties>& props) const;

  /// Compute the center position of selected edges.
  /// @param props Node properties array to look up vertex positions.
  /// @return Center position as [x, y, z], or nullopt if no selection.
  [[nodiscard]] std::optional<std::array<float, 3>> ComputeEdgeSelectionCenter(
      const std::vector<NodeProperties>& props) const;

  /// Compute the center position of selected faces.
  /// @param props Node properties array to look up vertex positions.
  /// @return Center position as [x, y, z], or nullopt if no selection.
  [[nodiscard]] std::optional<std::array<float, 3>> ComputeFaceSelectionCenter(
      const std::vector<NodeProperties>& props) const;

  /// Select a vertex, replacing existing selection.
  void SelectVertex(const VertexRef& ref);

  /// Add a vertex to the selection.
  void AddVertex(const VertexRef& ref);

  /// Remove a vertex from the selection.
  void RemoveVertex(const VertexRef& ref);

  /// Toggle vertex selection.
  void ToggleVertex(const VertexRef& ref);

  /// Select an edge, replacing existing selection.
  void SelectEdge(const EdgeRef& ref);

  /// Add an edge to the selection.
  void AddEdge(const EdgeRef& ref);

  /// Remove an edge from the selection.
  void RemoveEdge(const EdgeRef& ref);

  /// Toggle edge selection.
  void ToggleEdge(const EdgeRef& ref);

  /// Select a face, replacing existing selection.
  void SelectFace(const FaceRef& ref);

  /// Add a face to the selection.
  void AddFace(const FaceRef& ref);

  /// Remove a face from the selection.
  void RemoveFace(const FaceRef& ref);

  /// Toggle face selection.
  void ToggleFace(const FaceRef& ref);

  /// Check if a vertex is selected.
  [[nodiscard]] bool IsVertexSelected(const VertexRef& ref) const;

  /// Check if an edge is selected.
  [[nodiscard]] bool IsEdgeSelected(const EdgeRef& ref) const;

  /// Check if a face is selected.
  [[nodiscard]] bool IsFaceSelected(const FaceRef& ref) const;
};
