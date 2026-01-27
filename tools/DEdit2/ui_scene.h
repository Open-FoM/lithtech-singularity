#pragma once

#include "editor_state.h"

#include "imgui.h"

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <string>

/// Selection mode determines how clicks modify the selection set.
enum class SelectionMode {
  Replace, ///< Clear existing selection, select clicked item
  Add,     ///< Add clicked item to selection (Shift+Click)
  Remove,  ///< Remove clicked item from selection
  Toggle   ///< Toggle clicked item in selection (Ctrl+Click)
};

struct ScenePanelState {
  /// Set of currently selected node IDs
  std::unordered_set<int> selected_ids;

  /// Primary selection ID for property editing and gizmo anchor.
  /// -1 if nothing selected, otherwise must be in selected_ids.
  int primary_selection = -1;

  /// Legacy single-selection accessor for gradual migration.
  /// @deprecated Use selected_ids and primary_selection instead.
  [[nodiscard]] int selected_id_compat() const { return primary_selection; }

  ImGuiTextFilter filter;
  TreeUiState tree_ui;
  std::string error;
  bool isolate_selected = false;
  std::unordered_map<std::string, bool> type_visibility;
  std::unordered_map<std::string, bool> class_visibility;
};

class UndoStack;

bool SceneNodePassesFilters(
	const ScenePanelState& state,
	int node_id,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props);

void DrawScenePanel(
	ScenePanelState& state,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	SelectionTarget& active_target,
	UndoStack* undo_stack);

/// Selection manipulation functions.
/// @{

/// Clears all selected nodes.
void ClearSelection(ScenePanelState& state);

/// Selects a single node, clearing previous selection.
void SelectNode(ScenePanelState& state, int node_id);

/// Modifies selection based on mode and clicked node.
void ModifySelection(ScenePanelState& state, int node_id, SelectionMode mode);

/// Returns true if the given node is currently selected.
[[nodiscard]] bool IsNodeSelected(const ScenePanelState& state, int node_id);

/// Returns true if any nodes are selected.
[[nodiscard]] bool HasSelection(const ScenePanelState& state);

/// Returns the number of selected nodes.
[[nodiscard]] size_t SelectionCount(const ScenePanelState& state);

/// Select all visible, non-folder nodes.
void SelectAll(
  ScenePanelState& state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);

/// Deselect all nodes (alias for ClearSelection).
void SelectNone(ScenePanelState& state);

/// Invert selection: selected become unselected, and vice versa.
void SelectInverse(
  ScenePanelState& state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);

/// Compute center position of all selected nodes.
/// Returns (0,0,0) if no nodes are selected.
[[nodiscard]] std::array<float, 3> ComputeSelectionCenter(
  const ScenePanelState& state,
  const std::vector<NodeProperties>& props);

/// Compute axis-aligned bounding box of selection.
/// Returns false if no valid selection exists.
[[nodiscard]] bool ComputeSelectionBounds(
  const ScenePanelState& state,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props,
  float out_min[3],
  float out_max[3]);

/// @}

/// Visibility/Freeze commands.
/// @{

/// Hide all currently selected nodes (sets visible = false).
void HideSelected(
  const ScenePanelState& state,
  std::vector<NodeProperties>& props);

/// Unhide all nodes (sets visible = true for all).
void UnhideAll(std::vector<NodeProperties>& props);

/// Hide all nodes except those currently selected.
void HideUnselected(
  const ScenePanelState& state,
  const std::vector<TreeNode>& nodes,
  std::vector<NodeProperties>& props);

/// Freeze all currently selected nodes (sets frozen = true).
void FreezeSelected(
  const ScenePanelState& state,
  std::vector<NodeProperties>& props);

/// Unfreeze all nodes (sets frozen = false for all).
void UnfreezeAll(std::vector<NodeProperties>& props);

/// Returns true if the node can be picked/selected (not hidden or frozen).
[[nodiscard]] bool IsNodePickable(
  const NodeProperties& props);

/// @}
