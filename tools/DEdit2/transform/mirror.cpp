#include "transform/mirror.h"

#include "editor_state.h"
#include "ui_scene.h"

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

}  // namespace

void MirrorSelection(
    ScenePanelState& scene_panel,
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    MirrorAxis axis) {
  if (!HasSelection(scene_panel)) {
    return;
  }

  // Compute selection bounding box center
  float bounds_min[3];
  float bounds_max[3];
  if (!ComputeSelectionBounds(scene_panel, nodes, props, bounds_min, bounds_max)) {
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
    if (id < 0 || static_cast<size_t>(id) >= props.size()) {
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
