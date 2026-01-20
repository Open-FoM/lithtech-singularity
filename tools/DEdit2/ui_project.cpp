#include "ui_project.h"

#include "ui_shared.h"

void DrawProjectPanel(
	ProjectPanelState& state,
	const std::string& project_root,
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	SelectionTarget& active_target,
	ProjectContextAction* project_action,
	UndoStack* undo_stack)
{
	if (ImGui::Begin("Project"))
	{
		if (!state.error.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", state.error.c_str());
		}

		ImGui::TextUnformatted("Project Assets");
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
		state.filter.Draw("Filter##Project", 180.0f);
		ImGui::Separator();
		ImGui::BeginChild("ProjectTree", ImVec2(0.0f, 0.0f), true);
		if (!nodes.empty())
		{
			DrawTreeNodes(
				nodes,
				0,
				state.selected_id,
				state.filter,
				expand_mode,
				state.tree_ui,
				false,
				SelectionTarget::Project,
				active_target,
				&project_root,
				&props,
				project_action,
				undo_stack);
		}
		else
		{
			ImGui::TextUnformatted("No project data loaded.");
		}
		ImGui::EndChild();

		if (ImGui::BeginPopupContextWindow("ProjectContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Create Folder"))
			{
				state.tree_ui.pending_create_parent = 0;
				state.tree_ui.pending_create_folder = true;
				active_target = SelectionTarget::Project;
			}
			if (ImGui::MenuItem("Create Asset"))
			{
				state.tree_ui.pending_create_parent = 0;
				state.tree_ui.pending_create_folder = false;
				active_target = SelectionTarget::Project;
			}
			ImGui::EndPopup();
		}
	}
	ImGui::End();

	HandlePendingCreate(
		state.tree_ui,
		nodes,
		props,
		state.selected_id,
		"New Folder",
		"New Asset",
		"Folder",
		"Asset",
		UndoTarget::Project,
		undo_stack);
	HandleRenamePopup(state.tree_ui, nodes, "Rename Project Item", UndoTarget::Project, undo_stack);
}
