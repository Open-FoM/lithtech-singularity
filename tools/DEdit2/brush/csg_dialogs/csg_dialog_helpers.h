#pragma once

#include "editor_state.h"
#include "ui_scene.h"
#include "undo_stack.h"

#include <string>
#include <vector>

/// State for the CSG error popup.
struct CSGErrorPopupState {
  bool show = false;
  std::string message;
};

/// Extract brush geometry from a node's properties.
/// @param props The node properties.
/// @param out_vertices Output vertex positions (XYZ triplets).
/// @param out_indices Output triangle indices.
/// @return true if brush has valid geometry.
[[nodiscard]] bool ExtractBrushGeometry(const NodeProperties& props, std::vector<float>& out_vertices,
                                        std::vector<uint32_t>& out_indices);

/// Check if a node is a valid brush for CSG operations.
/// @param node The tree node.
/// @param props The node properties.
/// @return true if the node is a non-deleted, non-folder brush with geometry.
[[nodiscard]] bool IsValidCSGBrush(const TreeNode& node, const NodeProperties& props);

/// Get all selected brush node IDs that are valid for CSG operations.
/// @param state Scene panel state with selection.
/// @param nodes Scene nodes.
/// @param props Scene properties.
/// @return Vector of valid brush node IDs.
[[nodiscard]] std::vector<int> GetSelectedCSGBrushIds(const ScenePanelState& state, const std::vector<TreeNode>& nodes,
                                                      const std::vector<NodeProperties>& props);

/// Create a new brush node from CSG result and add to scene.
/// @param scene_nodes Scene node list (modified).
/// @param scene_props Scene properties list (modified).
/// @param vertices Triangle mesh vertices (XYZ triplets).
/// @param indices Triangle mesh indices.
/// @param name Name for the new node.
/// @return New node ID, or -1 on failure.
int CreateBrushFromCSGResult(std::vector<TreeNode>& scene_nodes, std::vector<NodeProperties>& scene_props,
                             const std::vector<float>& vertices, const std::vector<uint32_t>& indices,
                             const char* name);

/// Delete brush nodes (mark as deleted) with undo support.
/// @param node_ids IDs of nodes to delete.
/// @param scene_nodes Scene nodes (modified).
/// @param undo_stack Undo stack for recording deletions (may be nullptr).
void DeleteBrushNodes(const std::vector<int>& node_ids, std::vector<TreeNode>& scene_nodes, UndoStack* undo_stack);

/// Draw the CSG error popup.
/// @param error_state The error popup state.
void DrawCSGErrorPopup(CSGErrorPopupState& error_state);
