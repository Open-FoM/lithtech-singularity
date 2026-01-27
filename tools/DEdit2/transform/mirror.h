#pragma once

#include <vector>

struct NodeProperties;
struct ScenePanelState;
struct TreeNode;

/// Axis for mirror operation.
enum class MirrorAxis { X, Y, Z };

/// Mirrors all selected nodes around the selection center on the specified axis.
/// For brushes, vertices are mirrored and triangle winding is reversed.
/// For other nodes, position is mirrored.
void MirrorSelection(
    ScenePanelState& scene_panel,
    std::vector<TreeNode>& nodes,
    std::vector<NodeProperties>& props,
    MirrorAxis axis);
