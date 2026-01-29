#pragma once

#include <string>
#include <vector>

struct NodeProperties;
struct ScenePanelState;
struct TreeNode;

/// Field type for selection criteria.
enum class SelectionCriterionField
{
  Name,       ///< Node name
  Type,       ///< Node type (Brush, Light, Object, etc.)
  ClassName,  ///< Object class name (PointLight, Door, etc.)
  Property    ///< Custom property value (future)
};

/// Comparison operator for selection criteria.
enum class SelectionCriterionOp
{
  Equals,     ///< Exact match (case-insensitive)
  NotEquals,  ///< Not equal (case-insensitive)
  Contains,   ///< Substring match (case-insensitive)
  StartsWith, ///< Prefix match (case-insensitive)
  EndsWith,   ///< Suffix match (case-insensitive)
  Matches     ///< Wildcard pattern (* and ?)
};

/// A single criterion for selection queries.
struct SelectionCriterion
{
  SelectionCriterionField field = SelectionCriterionField::Name;
  SelectionCriterionOp op = SelectionCriterionOp::Contains;
  std::string value;
};

/// How to combine multiple criteria.
enum class SelectionQueryCombiner
{
  All, ///< All criteria must match (AND)
  Any  ///< At least one criterion must match (OR)
};

/// A complete selection query with multiple criteria.
struct SelectionQuery
{
  std::vector<SelectionCriterion> criteria;
  SelectionQueryCombiner combiner = SelectionQueryCombiner::All;

  /// Clear all criteria.
  void Clear() { criteria.clear(); }

  /// Returns true if the query is empty (no criteria).
  [[nodiscard]] bool IsEmpty() const { return criteria.empty(); }
};

/// Saved selection query preset.
struct SelectionQueryPreset
{
  std::string name;
  SelectionQuery query;
};

/// State for the advanced selection dialog.
struct AdvancedSelectionDialogState
{
  bool open = false;
  SelectionQuery current_query;
  std::string new_preset_name;
  std::vector<SelectionQueryPreset> presets;
};

/// Evaluate a single criterion against a node.
[[nodiscard]] bool EvaluateCriterion(
  const SelectionCriterion& criterion,
  const TreeNode& node,
  const NodeProperties& props);

/// Evaluate a complete query against a node.
[[nodiscard]] bool EvaluateQuery(
  const SelectionQuery& query,
  const TreeNode& node,
  const NodeProperties& props);

/// Count how many nodes match the query.
[[nodiscard]] int CountQueryMatches(
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);

/// Select nodes matching the query.
void SelectByQuery(
  ScenePanelState& scene_panel,
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);

/// Add nodes matching the query to existing selection.
void AddToSelectionByQuery(
  ScenePanelState& scene_panel,
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);

/// Remove nodes matching the query from existing selection.
void RemoveFromSelectionByQuery(
  ScenePanelState& scene_panel,
  const SelectionQuery& query,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);

/// Draw the advanced selection dialog.
void DrawAdvancedSelectionDialog(
  AdvancedSelectionDialogState& state,
  ScenePanelState& scene_panel,
  const std::vector<TreeNode>& nodes,
  const std::vector<NodeProperties>& props);

/// Get label for field enum.
[[nodiscard]] const char* GetFieldLabel(SelectionCriterionField field);

/// Get label for operator enum.
[[nodiscard]] const char* GetOperatorLabel(SelectionCriterionOp op);

/// Get label for combiner enum.
[[nodiscard]] const char* GetCombinerLabel(SelectionQueryCombiner combiner);
