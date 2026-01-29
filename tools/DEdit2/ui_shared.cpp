#include "ui_shared.h"

#include "grouping/node_grouping.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <spawn.h>
#include <vector>

extern char **environ;

namespace
{
namespace fs = std::filesystem;

bool OpenPathInFileManager(const std::string& path)
{
#if defined(__APPLE__)
	if (path.empty())
	{
		return false;
	}
	std::vector<const char*> args;
	args.push_back("open");
	args.push_back(path.c_str());
	args.push_back(nullptr);
	pid_t pid = 0;
	const int result = posix_spawnp(
		&pid,
		"open",
		nullptr,
		nullptr,
		const_cast<char* const*>(args.data()),
		environ);
	return result == 0;
#else
	(void)path;
	return false;
#endif
}

fs::path ResolveProjectItemPath(
	const std::string& project_root,
	const std::vector<NodeProperties>& props,
	const TreeNode& node,
	int node_id)
{
	fs::path root = project_root.empty() ? fs::path(".") : fs::path(project_root);
	if (node_id < 0 || node_id >= static_cast<int>(props.size()))
	{
		return root;
	}

	const std::string& resource = props[node_id].resource;
	fs::path item_path;
	if (resource.empty() || resource == "." || resource == "./")
	{
		item_path = root;
	}
	else
	{
		item_path = fs::path(resource);
		if (item_path.is_relative())
		{
			item_path = root / item_path;
		}
	}

	if (!node.is_folder)
	{
		fs::path parent = item_path.parent_path();
		if (!parent.empty())
		{
			item_path = parent;
		}
	}

	return item_path;
}

fs::path ResolveProjectItemFilePath(
	const std::string& project_root,
	const std::vector<NodeProperties>& props,
	int node_id)
{
	fs::path root = project_root.empty() ? fs::path(".") : fs::path(project_root);
	if (node_id < 0 || node_id >= static_cast<int>(props.size()))
	{
		return {};
	}

	const std::string& resource = props[node_id].resource;
	if (resource.empty())
	{
		return {};
	}

	fs::path item_path = fs::path(resource);
	if (item_path.is_relative())
	{
		item_path = root / item_path;
	}

	return item_path;
}

std::string ToLowerAscii(std::string value)
{
	for (char& ch : value)
	{
		const unsigned char uch = static_cast<unsigned char>(ch);
		ch = static_cast<char>(std::tolower(uch));
	}
	return value;
}

bool IsWorldFilePath(const fs::path& path)
{
	const std::string ext = ToLowerAscii(path.extension().string());
	return ext == ".ltc" || ext == ".lta" || ext == ".tbw" || ext == ".dat";
}

bool NodeMatchesFilter(const std::vector<TreeNode>& nodes, int node_id, const ImGuiTextFilter& filter)
{
	if (node_id < 0 || node_id >= static_cast<int>(nodes.size()))
	{
		return false;
	}

	if (!filter.IsActive())
	{
		return !nodes[node_id].deleted;
	}

	const TreeNode& node = nodes[node_id];
	if (node.deleted)
	{
		return false;
	}
	if (filter.PassFilter(node.name.c_str()))
	{
		return true;
	}

	for (int child_id : node.children)
	{
		if (NodeMatchesFilter(nodes, child_id, filter))
		{
			return true;
		}
	}

	return false;
}

bool BuildNodePathRecursive(
	const std::vector<TreeNode>& nodes,
	int node_id,
	int target_id,
	std::vector<std::string>& out_parts)
{
	if (node_id < 0 || node_id >= static_cast<int>(nodes.size()))
	{
		return false;
	}

	const TreeNode& node = nodes[node_id];
	if (node.deleted)
	{
		return false;
	}

	out_parts.push_back(node.name);
	if (node_id == target_id)
	{
		return true;
	}

	for (int child_id : node.children)
	{
		if (BuildNodePathRecursive(nodes, child_id, target_id, out_parts))
		{
			return true;
		}
	}

	out_parts.pop_back();
	return false;
}

UndoTarget ToUndoTarget(SelectionTarget target)
{
	return target == SelectionTarget::Project ? UndoTarget::Project : UndoTarget::Scene;
}

} // namespace

bool IsNameEmptyOrWhitespace(const std::string& name)
{
	for (char ch : name)
	{
		if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
		{
			return false;
		}
	}
	return true;
}

bool HasDuplicateSiblingName(
	const std::vector<TreeNode>& nodes,
	int root_id,
	int node_id,
	const std::string& name)
{
	const int parent_id = FindParentId(nodes, root_id, node_id);
	if (parent_id < 0)
	{
		return false;
	}
	for (int child_id : nodes[parent_id].children)
	{
		if (child_id == node_id)
		{
			continue;
		}
		if (child_id < 0 || child_id >= static_cast<int>(nodes.size()))
		{
			continue;
		}
		const TreeNode& child = nodes[child_id];
		if (child.deleted)
		{
			continue;
		}
		if (child.name == name)
		{
			return true;
		}
	}
	return false;
}

std::string BuildNodePath(const std::vector<TreeNode>& nodes, int root_id, int target_id)
{
	std::vector<std::string> parts;
	if (!BuildNodePathRecursive(nodes, root_id, target_id, parts))
	{
		return {};
	}

	std::string path;
	for (size_t i = 0; i < parts.size(); ++i)
	{
		if (i > 0)
		{
			path += "/";
		}
		path += parts[i];
	}
	return path;
}

void DrawTreeNodes(
	std::vector<TreeNode>& nodes,
	int node_id,
	int& selected_id,
	const ImGuiTextFilter& filter,
	ExpandMode expand_mode,
	TreeUiState& ui_state,
	bool is_scene,
	SelectionTarget target,
	SelectionTarget& active_target,
	const std::string* project_root,
	const std::vector<NodeProperties>* props,
	ProjectContextAction* project_action,
	UndoStack* undo_stack,
	TreeNodeFilter node_filter,
	void* node_filter_user)
{
	if (node_filter && !node_filter(node_id, nodes, props, node_filter_user))
	{
		return;
	}
	if (!NodeMatchesFilter(nodes, node_id, filter))
	{
		return;
	}

	const TreeNode& node = nodes[node_id];
	if (node.deleted)
	{
		return;
	}
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
	const bool has_children = !node.children.empty();
	if (!has_children)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (node_id == selected_id)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool any_child_match = false;
	if (filter.IsActive() && has_children)
	{
		for (int child_id : node.children)
		{
			if (NodeMatchesFilter(nodes, child_id, filter))
			{
				any_child_match = true;
				break;
			}
		}
	}

	if (has_children)
	{
		if (expand_mode == ExpandMode::ExpandAll)
		{
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}
		else if (expand_mode == ExpandMode::CollapseAll)
		{
			ImGui::SetNextItemOpen(false, ImGuiCond_Always);
		}
		else if (filter.IsActive() && any_child_match)
		{
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}
	}

	ImGui::PushID(node_id);
	if (filter.IsActive() && !filter.PassFilter(node.name.c_str()))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
	}
	const bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		// Block selection of frozen scene nodes (consistent with viewport picking)
		const bool is_frozen = is_scene && props != nullptr && node_id >= 0 &&
			static_cast<size_t>(node_id) < props->size() && (*props)[node_id].frozen;
		if (!is_frozen)
		{
			selected_id = node_id;
			active_target = target;
		}
	}
	// Double-click to request viewport focus on this node
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		ui_state.focus_request_id = node_id;
	}

	// Drag-drop for scene nodes (reparenting)
	if (is_scene && node_id != 0)
	{
		// Drag source: allow dragging non-root scene nodes
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload("SCENE_NODE", &node_id, sizeof(int));
			ImGui::Text("Move: %s", node.name.c_str());
			ImGui::EndDragDropSource();
		}

		// Drop target: only folders can accept drops
		if (node.is_folder && ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
			{
				const int dropped_id = *static_cast<const int*>(payload->Data);
				// Validate: can't drop onto self, can't drop ancestor onto descendant
				if (dropped_id != node_id && !IsDescendantOf(nodes, node_id, dropped_id))
				{
					// Find current parent of dropped node
					const int old_parent = FindParentId(nodes, 0, dropped_id);
					if (old_parent >= 0 && old_parent != node_id)
					{
						// Remove from old parent
						auto& old_children = nodes[old_parent].children;
						old_children.erase(
							std::remove(old_children.begin(), old_children.end(), dropped_id),
							old_children.end());

						// Add to new parent (this node)
						nodes[node_id].children.push_back(dropped_id);

						// Record undo
						if (undo_stack != nullptr)
						{
							undo_stack->PushMove(ToUndoTarget(target), dropped_id, old_parent, node_id);
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	// Draw visibility/freeze icons for scene nodes
	if (is_scene && props != nullptr && node_id >= 0 && static_cast<size_t>(node_id) < props->size())
	{
		const NodeProperties& node_props = (*props)[node_id];
		if (!node.is_folder)
		{
			ImGui::SameLine();
			// Eye icon for visibility (gray = hidden)
			if (node_props.visible)
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.6f), "[V]");
			}
			else
			{
				ImGui::TextColored(ImVec4(0.5f, 0.3f, 0.3f, 0.8f), "[H]");
			}
			ImGui::SameLine();
			// Lock icon for freeze (red = frozen)
			if (node_props.frozen)
			{
				ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.1f, 1.0f), "[F]");
			}
		}
	}

	if (filter.IsActive() && !filter.PassFilter(node.name.c_str()))
	{
		ImGui::PopStyleColor();
	}

	if (ImGui::BeginPopupContextItem())
	{
		selected_id = node_id;
		active_target = target;
		if (!is_scene && project_root != nullptr && props != nullptr)
		{
			bool added_project_action = false;
			if (project_action != nullptr && !node.is_folder)
			{
				const fs::path item_path = ResolveProjectItemFilePath(*project_root, *props, node_id);
				if (!item_path.empty() && IsWorldFilePath(item_path))
				{
					if (ImGui::MenuItem("Open World"))
					{
						project_action->load_world = true;
						project_action->world_path = item_path.string();
					}
					added_project_action = true;
				}
			}
			if (ImGui::MenuItem("Open Folder"))
			{
				const fs::path folder_path = ResolveProjectItemPath(*project_root, *props, node, node_id);
				OpenPathInFileManager(folder_path.string());
			}
			added_project_action = true;
			if (added_project_action)
			{
				ImGui::Separator();
			}
		}
		if (ImGui::MenuItem("Rename..."))
		{
			ui_state.rename_node_id = node_id;
			std::snprintf(ui_state.rename_buffer, sizeof(ui_state.rename_buffer), "%s", node.name.c_str());
			ui_state.open_rename_popup = true;
		}

		if (node_id != 0 && ImGui::MenuItem("Delete"))
		{
			const bool prev_deleted = nodes[node_id].deleted;
			nodes[node_id].deleted = true;
			if (undo_stack)
			{
				undo_stack->PushDelete(ToUndoTarget(target), node_id, prev_deleted);
			}
			if (selected_id == node_id)
			{
				selected_id = -1;
			}
		}

		if (is_scene)
		{
			if (ImGui::MenuItem("Create Object"))
			{
				ui_state.pending_create_parent = node_id;
				ui_state.pending_create_folder = false;
			}
		}
		else
		{
			if (ImGui::MenuItem("Create Folder"))
			{
				ui_state.pending_create_parent = node_id;
				ui_state.pending_create_folder = true;
			}
			if (ImGui::MenuItem("Create Asset"))
			{
				ui_state.pending_create_parent = node_id;
				ui_state.pending_create_folder = false;
			}
		}

		ImGui::EndPopup();
	}

	if (has_children && open)
	{
		for (int child_id : node.children)
		{
			DrawTreeNodes(
				nodes,
				child_id,
				selected_id,
				filter,
				expand_mode,
				ui_state,
				is_scene,
				target,
				active_target,
				project_root,
				props,
				project_action,
				undo_stack,
				node_filter,
				node_filter_user);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void HandlePendingCreate(
	TreeUiState& ui_state,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int& selected_id,
	const char* default_folder,
	const char* default_object,
	const char* folder_type,
	const char* object_type,
	UndoTarget target,
	UndoStack* undo_stack)
{
	if (ui_state.pending_create_parent < 0)
	{
		return;
	}

	const char* name = ui_state.pending_create_folder ? default_folder : default_object;
	const char* type_name = ui_state.pending_create_folder ? folder_type : object_type;
	const int new_id = AddChildNode(nodes, props, ui_state.pending_create_parent, name, ui_state.pending_create_folder, type_name);
	if (undo_stack)
	{
		undo_stack->PushCreate(target, new_id);
	}
	selected_id = new_id;
	ui_state.rename_node_id = new_id;
	std::snprintf(ui_state.rename_buffer, sizeof(ui_state.rename_buffer), "%s", name);
	ui_state.open_rename_popup = true;
	ui_state.pending_create_parent = -1;
}

void HandleRenamePopup(
	TreeUiState& ui_state,
	std::vector<TreeNode>& nodes,
	const char* popup_id,
	UndoTarget target,
	UndoStack* undo_stack)
{
	if (ui_state.open_rename_popup)
	{
		ImGui::OpenPopup(popup_id);
		ui_state.open_rename_popup = false;
	}

	if (ImGui::BeginPopupModal(popup_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Name", ui_state.rename_buffer, sizeof(ui_state.rename_buffer));
		if (ImGui::Button("OK"))
		{
			if (ui_state.rename_node_id >= 0 && ui_state.rename_node_id < static_cast<int>(nodes.size()))
			{
				const std::string previous = nodes[ui_state.rename_node_id].name;
				const std::string next = ui_state.rename_buffer;
				if (previous != next)
				{
					nodes[ui_state.rename_node_id].name = next;
					if (undo_stack)
					{
						undo_stack->PushRename(target, ui_state.rename_node_id, previous, next);
					}
				}
			}
			ui_state.rename_node_id = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ui_state.rename_node_id = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
