#include "ui_scene.h"

#include "ui_shared.h"


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
		ImGui::BeginChild("SceneTree", ImVec2(0.0f, 0.0f), true);
		if (!nodes.empty())
		{
			DrawTreeNodes(
				nodes,
				0,
				state.selected_id,
				state.filter,
				expand_mode,
				state.tree_ui,
				true,
				SelectionTarget::Scene,
				active_target,
				nullptr,
				nullptr,
				nullptr,
				undo_stack);
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
		state.selected_id,
		"New Folder",
		"New Object",
		"Folder",
		"Entity",
		UndoTarget::Scene,
		undo_stack);
	HandleRenamePopup(state.tree_ui, nodes, "Rename Scene Item", UndoTarget::Scene, undo_stack);
}
