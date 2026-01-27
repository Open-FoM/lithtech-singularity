#include "ui_scene.h"

#include "ui_shared.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace
{
void CollectSceneFilterEntries(
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	std::vector<std::string>& out_types,
	std::vector<std::string>& out_classes)
{
	std::unordered_set<std::string> types;
	std::unordered_set<std::string> classes;
	const size_t count = std::min(nodes.size(), props.size());
	for (size_t i = 0; i < count; ++i)
	{
		const TreeNode& node = nodes[i];
		if (node.deleted || node.is_folder)
		{
			continue;
		}
		const NodeProperties& node_props = props[i];
		if (!node_props.type.empty())
		{
			types.insert(node_props.type);
		}
		if (!node_props.class_name.empty())
		{
			classes.insert(node_props.class_name);
		}
	}
	out_types.assign(types.begin(), types.end());
	out_classes.assign(classes.begin(), classes.end());
	std::sort(out_types.begin(), out_types.end());
	std::sort(out_classes.begin(), out_classes.end());
}

void SyncFilterMap(std::unordered_map<std::string, bool>& map, const std::vector<std::string>& entries)
{
	for (const auto& entry : entries)
	{
		if (map.find(entry) == map.end())
		{
			map.emplace(entry, true);
		}
	}
}

bool SceneNodeVisibleRecursive(
	const ScenePanelState& state,
	int node_id,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	bool respect_node_visibility)
{
	if (node_id < 0 || static_cast<size_t>(node_id) >= nodes.size())
	{
		return false;
	}

	const TreeNode& node = nodes[node_id];
	if (node.deleted)
	{
		return false;
	}

	if (node.is_folder)
	{
		for (int child_id : node.children)
		{
			if (SceneNodeVisibleRecursive(state, child_id, nodes, props, respect_node_visibility))
			{
				return true;
			}
		}
		return false;
	}

	if (static_cast<size_t>(node_id) >= props.size())
	{
		return false;
	}

	const NodeProperties& node_props = props[node_id];
	if (respect_node_visibility && !node_props.visible)
	{
		return false;
	}

	if (state.isolate_selected && !state.selected_ids.empty() &&
	    state.selected_ids.find(node_id) == state.selected_ids.end())
	{
		return false;
	}

	if (!node_props.type.empty())
	{
		auto it = state.type_visibility.find(node_props.type);
		if (it != state.type_visibility.end() && !it->second)
		{
			return false;
		}
	}

	if (!node_props.class_name.empty())
	{
		auto it = state.class_visibility.find(node_props.class_name);
		if (it != state.class_visibility.end() && !it->second)
		{
			return false;
		}
	}

	return true;
}

bool SceneNodeFilterCallback(
	int node_id,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>* props,
	void* user)
{
	if (!user || !props)
	{
		return true;
	}
	const ScenePanelState* state = static_cast<const ScenePanelState*>(user);
	return SceneNodeVisibleRecursive(*state, node_id, nodes, *props, false);
}
} // namespace

bool SceneNodePassesFilters(
	const ScenePanelState& state,
	int node_id,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props)
{
	return SceneNodeVisibleRecursive(state, node_id, nodes, props, true);
}

void DrawScenePanel(
	ScenePanelState& state,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	SelectionTarget& active_target,
	UndoStack* undo_stack)
{
	if (ImGui::Begin("Scene"))
	{
		if (!state.error.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", state.error.c_str());
		}
		ImGui::TextUnformatted("Scene Hierarchy");
		ImGui::SameLine();
		ExpandMode expand_mode = ExpandMode::None;
		if (ImGui::SmallButton("Expand All"))
		{
			expand_mode = ExpandMode::ExpandAll;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Collapse All"))
		{
			expand_mode = ExpandMode::CollapseAll;
		}
		ImGui::SameLine();
		state.filter.Draw("Filter##Scene", 180.0f);
		ImGui::Separator();

		std::vector<std::string> types;
		std::vector<std::string> classes;
		CollectSceneFilterEntries(nodes, props, types, classes);
		SyncFilterMap(state.type_visibility, types);
		SyncFilterMap(state.class_visibility, classes);

		if (ImGui::CollapsingHeader("Visibility Filters"))
		{
			const bool has_selection = HasSelection(state);
			ImGui::BeginDisabled(!has_selection);
			ImGui::Checkbox("Isolate Selected", &state.isolate_selected);
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Show All"))
			{
				state.isolate_selected = false;
				for (auto& entry : state.type_visibility)
				{
					entry.second = true;
				}
				for (auto& entry : state.class_visibility)
				{
					entry.second = true;
				}
			}

			if (!types.empty())
			{
				ImGui::Separator();
				ImGui::TextUnformatted("Types");
				ImGui::PushID("TypeFilters");
				for (const auto& type : types)
				{
					bool& visible = state.type_visibility[type];
					ImGui::PushID(type.c_str());
					ImGui::Checkbox("##TypeVisible", &visible);
					ImGui::SameLine();
					ImGui::TextUnformatted(type.c_str());
					ImGui::PopID();
				}
				ImGui::PopID();
			}

			if (!classes.empty())
			{
				ImGui::Separator();
				ImGui::TextUnformatted("Classes");
				ImGui::PushID("ClassFilters");
				for (const auto& class_name : classes)
				{
					bool& visible = state.class_visibility[class_name];
					ImGui::PushID(class_name.c_str());
					ImGui::Checkbox("##ClassVisible", &visible);
					ImGui::SameLine();
					ImGui::TextUnformatted(class_name.c_str());
					ImGui::PopID();
				}
				ImGui::PopID();
			}
		}

		ImGui::Separator();
		ImGui::BeginChild("SceneTree", ImVec2(0.0f, 0.0f), true);
		if (!nodes.empty())
		{
			DrawTreeNodes(
				nodes,
				0,
				state.primary_selection,
				state.filter,
				expand_mode,
				state.tree_ui,
				true,
				SelectionTarget::Scene,
				active_target,
				nullptr,
				&props,
				nullptr,
				undo_stack,
				SceneNodeFilterCallback,
				&state);
		}
		else
		{
			ImGui::TextUnformatted("No scene loaded.");
		}
		ImGui::EndChild();

		if (ImGui::BeginPopupContextWindow("SceneContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Create Object"))
			{
				state.tree_ui.pending_create_parent = 0;
				state.tree_ui.pending_create_folder = false;
				active_target = SelectionTarget::Scene;
			}
			ImGui::EndPopup();
		}
	}
	ImGui::End();

	HandlePendingCreate(
		state.tree_ui,
		nodes,
		props,
		state.primary_selection,
		"New Folder",
		"New Object",
		"Folder",
		"Entity",
		UndoTarget::Scene,
		undo_stack);
	HandleRenamePopup(state.tree_ui, nodes, "Rename Scene Item", UndoTarget::Scene, undo_stack);

	// Sync primary_selection with selected_ids set
	if (state.primary_selection >= 0 && state.selected_ids.find(state.primary_selection) == state.selected_ids.end())
	{
		// Primary was changed by tree widget, update the set
		state.selected_ids.clear();
		state.selected_ids.insert(state.primary_selection);
	}
	else if (state.primary_selection < 0 && !state.selected_ids.empty())
	{
		// Primary was cleared (e.g., by deletion), clear selection set to maintain invariants
		state.selected_ids.clear();
	}
}

void ClearSelection(ScenePanelState& state)
{
	state.selected_ids.clear();
	state.primary_selection = -1;
}

void SelectNode(ScenePanelState& state, int node_id)
{
	state.selected_ids.clear();
	if (node_id >= 0)
	{
		state.selected_ids.insert(node_id);
	}
	state.primary_selection = node_id;
}

void ModifySelection(ScenePanelState& state, int node_id, SelectionMode mode)
{
	if (node_id < 0)
	{
		return;
	}

	switch (mode)
	{
	case SelectionMode::Replace:
		state.selected_ids.clear();
		state.selected_ids.insert(node_id);
		state.primary_selection = node_id;
		break;

	case SelectionMode::Add:
		state.selected_ids.insert(node_id);
		state.primary_selection = node_id;
		break;

	case SelectionMode::Remove:
		state.selected_ids.erase(node_id);
		if (state.primary_selection == node_id)
		{
			state.primary_selection = state.selected_ids.empty() ? -1 : *state.selected_ids.begin();
		}
		break;

	case SelectionMode::Toggle:
		if (state.selected_ids.find(node_id) != state.selected_ids.end())
		{
			state.selected_ids.erase(node_id);
			if (state.primary_selection == node_id)
			{
				state.primary_selection = state.selected_ids.empty() ? -1 : *state.selected_ids.begin();
			}
		}
		else
		{
			state.selected_ids.insert(node_id);
			state.primary_selection = node_id;
		}
		break;
	}
}

bool IsNodeSelected(const ScenePanelState& state, int node_id)
{
	return state.selected_ids.find(node_id) != state.selected_ids.end();
}

bool HasSelection(const ScenePanelState& state)
{
	return !state.selected_ids.empty();
}

size_t SelectionCount(const ScenePanelState& state)
{
	return state.selected_ids.size();
}

void SelectAll(
	ScenePanelState& state,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props)
{
	state.selected_ids.clear();
	const size_t count = std::min(nodes.size(), props.size());
	for (size_t i = 0; i < count; ++i)
	{
		const TreeNode& node = nodes[i];
		if (node.deleted || node.is_folder)
		{
			continue;
		}
		if (!SceneNodePassesFilters(state, static_cast<int>(i), nodes, props))
		{
			continue;
		}
		state.selected_ids.insert(static_cast<int>(i));
	}
	if (!state.selected_ids.empty())
	{
		state.primary_selection = *state.selected_ids.begin();
	}
	else
	{
		state.primary_selection = -1;
	}
}

void SelectNone(ScenePanelState& state)
{
	ClearSelection(state);
}

void SelectInverse(
	ScenePanelState& state,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props)
{
	std::unordered_set<int> new_selection;
	const size_t count = std::min(nodes.size(), props.size());
	for (size_t i = 0; i < count; ++i)
	{
		const TreeNode& node = nodes[i];
		if (node.deleted || node.is_folder)
		{
			continue;
		}
		if (!SceneNodePassesFilters(state, static_cast<int>(i), nodes, props))
		{
			continue;
		}
		if (state.selected_ids.find(static_cast<int>(i)) == state.selected_ids.end())
		{
			new_selection.insert(static_cast<int>(i));
		}
	}
	state.selected_ids = std::move(new_selection);
	if (!state.selected_ids.empty())
	{
		state.primary_selection = *state.selected_ids.begin();
	}
	else
	{
		state.primary_selection = -1;
	}
}

std::array<float, 3> ComputeSelectionCenter(
	const ScenePanelState& state,
	const std::vector<NodeProperties>& props)
{
	if (state.selected_ids.empty())
	{
		return {0.0f, 0.0f, 0.0f};
	}

	float sum_x = 0.0f;
	float sum_y = 0.0f;
	float sum_z = 0.0f;
	int valid_count = 0;

	for (int id : state.selected_ids)
	{
		if (id < 0 || static_cast<size_t>(id) >= props.size())
		{
			continue;
		}
		const NodeProperties& p = props[id];
		sum_x += p.position[0];
		sum_y += p.position[1];
		sum_z += p.position[2];
		++valid_count;
	}

	if (valid_count == 0)
	{
		return {0.0f, 0.0f, 0.0f};
	}

	return {sum_x / valid_count, sum_y / valid_count, sum_z / valid_count};
}

bool ComputeSelectionBounds(
	const ScenePanelState& state,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	float out_min[3],
	float out_max[3])
{
	bool has_valid = false;
	float min_x = 1.0e30f, min_y = 1.0e30f, min_z = 1.0e30f;
	float max_x = -1.0e30f, max_y = -1.0e30f, max_z = -1.0e30f;

	for (int id : state.selected_ids)
	{
		if (id < 0 || static_cast<size_t>(id) >= props.size())
		{
			continue;
		}
		if (static_cast<size_t>(id) >= nodes.size() || nodes[id].deleted)
		{
			continue;
		}
		const NodeProperties& p = props[id];
		min_x = std::min(min_x, p.position[0]);
		min_y = std::min(min_y, p.position[1]);
		min_z = std::min(min_z, p.position[2]);
		max_x = std::max(max_x, p.position[0]);
		max_y = std::max(max_y, p.position[1]);
		max_z = std::max(max_z, p.position[2]);
		has_valid = true;
	}

	if (has_valid)
	{
		out_min[0] = min_x;
		out_min[1] = min_y;
		out_min[2] = min_z;
		out_max[0] = max_x;
		out_max[1] = max_y;
		out_max[2] = max_z;
	}
	return has_valid;
}

void HideSelected(
	const ScenePanelState& state,
	std::vector<NodeProperties>& props)
{
	for (int id : state.selected_ids)
	{
		if (id >= 0 && static_cast<size_t>(id) < props.size())
		{
			props[id].visible = false;
		}
	}
}

void UnhideAll(std::vector<NodeProperties>& props)
{
	for (NodeProperties& p : props)
	{
		p.visible = true;
	}
}

void HideUnselected(
	const ScenePanelState& state,
	const std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props)
{
	const size_t count = std::min(nodes.size(), props.size());
	for (size_t i = 0; i < count; ++i)
	{
		const TreeNode& node = nodes[i];
		if (node.deleted || node.is_folder)
		{
			continue;
		}
		if (state.selected_ids.find(static_cast<int>(i)) == state.selected_ids.end())
		{
			props[i].visible = false;
		}
	}
}

void FreezeSelected(
	const ScenePanelState& state,
	std::vector<NodeProperties>& props)
{
	for (int id : state.selected_ids)
	{
		if (id >= 0 && static_cast<size_t>(id) < props.size())
		{
			props[id].frozen = true;
		}
	}
}

void UnfreezeAll(std::vector<NodeProperties>& props)
{
	for (NodeProperties& p : props)
	{
		p.frozen = false;
	}
}

bool IsNodePickable(const NodeProperties& props)
{
	return props.visible && !props.frozen;
}
