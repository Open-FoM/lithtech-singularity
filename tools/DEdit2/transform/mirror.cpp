#include "transform/mirror.h"

#include "editor_state.h"
#include "ui_scene.h"
#include "viewport_picking.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace {

/// Reverses triangle winding order by swapping indices in each triangle.
void ReverseTriangleWinding(std::vector<uint32_t>& indices) {
  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    std::swap(indices[i + 1], indices[i + 2]);
  }
}

/// Computes bounding box for a node considering actual geometry bounds.
/// Uses brush vertices if available, then TryGetNodeBounds, then falls back to position.
bool GetNodeGeometryBounds(const NodeProperties& props, float out_min[3], float out_max[3]) {
  // First try to use brush vertices for exact bounds
  if (!props.brush_vertices.empty() && props.brush_vertices.size() >= 3) {
    out_min[0] = out_max[0] = props.brush_vertices[0];
    out_min[1] = out_max[1] = props.brush_vertices[1];
    out_min[2] = out_max[2] = props.brush_vertices[2];
    for (size_t i = 3; i < props.brush_vertices.size(); i += 3) {
      out_min[0] = std::min(out_min[0], props.brush_vertices[i]);
      out_max[0] = std::max(out_max[0], props.brush_vertices[i]);
      out_min[1] = std::min(out_min[1], props.brush_vertices[i + 1]);
      out_max[1] = std::max(out_max[1], props.brush_vertices[i + 1]);
      out_min[2] = std::min(out_min[2], props.brush_vertices[i + 2]);
      out_max[2] = std::max(out_max[2], props.brush_vertices[i + 2]);
    }
    return true;
  }

  // Try TryGetNodeBounds for models and other nodes with extent data
  if (TryGetNodeBounds(props, out_min, out_max)) {
    return true;
  }

  // Fall back to position as a point
  out_min[0] = out_max[0] = props.position[0];
  out_min[1] = out_max[1] = props.position[1];
  out_min[2] = out_max[2] = props.position[2];
  return true;
}

/// Computes the actual geometry bounding box for the selection.
/// Unlike ComputeSelectionBounds which only uses positions, this considers
/// brush vertices and node bounds for accurate mirror center calculation.
bool ComputeSelectionGeometryBounds(
    const ScenePanelState& scene_panel,
    const std::vector<TreeNode>& nodes,
    const std::vector<NodeProperties>& props,
    float out_min[3],
    float out_max[3]) {
  bool has_valid = false;
  float global_min[3] = {std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max()};
  float global_max[3] = {std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest()};

  for (int id : scene_panel.selected_ids) {
    if (id < 0 || static_cast<size_t>(id) >= props.size()) {
      continue;
    }
    if (static_cast<size_t>(id) >= nodes.size()) {
      continue;
    }
    if (nodes[id].deleted || nodes[id].is_folder) {
      continue;
    }

    float node_min[3], node_max[3];
    if (GetNodeGeometryBounds(props[id], node_min, node_max)) {
      global_min[0] = std::min(global_min[0], node_min[0]);
      global_min[1] = std::min(global_min[1], node_min[1]);
      global_min[2] = std::min(global_min[2], node_min[2]);
      global_max[0] = std::max(global_max[0], node_max[0]);
      global_max[1] = std::max(global_max[1], node_max[1]);
      global_max[2] = std::max(global_max[2], node_max[2]);
      has_valid = true;
    }
  }

  if (has_valid) {
    out_min[0] = global_min[0];
    out_min[1] = global_min[1];
    out_min[2] = global_min[2];
    out_max[0] = global_max[0];
    out_max[1] = global_max[1];
    out_max[2] = global_max[2];
  }
  return has_valid;
}

}  // namespace

void MirrorSelection(
    ScenePanelState& scene_panel,
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    MirrorAxis axis) {
  if (!HasSelection(scene_panel)) {
    return;
  }

  // Compute selection bounding box center using actual geometry bounds
  float bounds_min[3];
  float bounds_max[3];
  if (!ComputeSelectionGeometryBounds(scene_panel, nodes, props, bounds_min, bounds_max)) {
    return;
  }

  const float center[3] = {
      (bounds_min[0] + bounds_max[0]) * 0.5f,
      (bounds_min[1] + bounds_max[1]) * 0.5f,
      (bounds_min[2] + bounds_max[2]) * 0.5f};

  // Determine which axis component to mirror
  int axis_idx = 0;
  switch (axis) {
    case MirrorAxis::X:
      axis_idx = 0;
      break;
    case MirrorAxis::Y:
      axis_idx = 1;
      break;
    case MirrorAxis::Z:
      axis_idx = 2;
      break;
  }

  // Mirror each selected node
  for (int id : scene_panel.selected_ids) {
    if (id < 0 || static_cast<size_t>(id) >= props.size() ||
        static_cast<size_t>(id) >= nodes.size()) {
      continue;
    }
    if (nodes[id].deleted || nodes[id].is_folder) {
      continue;
    }

    NodeProperties& node_props = props[id];

    // Check if this node has brush geometry
    if (!node_props.brush_vertices.empty() && !node_props.brush_indices.empty()) {
      // Mirror brush vertices around the center
      for (size_t i = axis_idx; i < node_props.brush_vertices.size(); i += 3) {
        float& coord = node_props.brush_vertices[i];
        coord = 2.0f * center[axis_idx] - coord;
      }

      // Reverse triangle winding to maintain correct face orientation
      ReverseTriangleWinding(node_props.brush_indices);

      // Update position to new centroid
      if (node_props.brush_vertices.size() >= 3) {
        const size_t vertex_count = node_props.brush_vertices.size() / 3;
        double sum[3] = {0.0, 0.0, 0.0};
        for (size_t i = 0; i < node_props.brush_vertices.size(); i += 3) {
          sum[0] += node_props.brush_vertices[i];
          sum[1] += node_props.brush_vertices[i + 1];
          sum[2] += node_props.brush_vertices[i + 2];
        }
        node_props.position[0] = static_cast<float>(sum[0] / static_cast<double>(vertex_count));
        node_props.position[1] = static_cast<float>(sum[1] / static_cast<double>(vertex_count));
        node_props.position[2] = static_cast<float>(sum[2] / static_cast<double>(vertex_count));
      }
    } else {
      // Mirror position for non-brush nodes
      node_props.position[axis_idx] = 2.0f * center[axis_idx] - node_props.position[axis_idx];
    }
  }
}
