#include "ui_dock.h"

#include "imgui_internal.h"

void DrawMainMenuBar(
	bool& request_reset_layout,
	MainMenuActions& actions,
	const std::vector<std::string>& recent_projects,
	bool can_undo,
	bool can_redo)
{
	if (!ImGui::BeginMainMenuBar())
	{
		return;
	}

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Open Folder..."))
		{
			actions.open_project_folder = true;
		}
		if (ImGui::BeginMenu("Recent"))
		{
			if (recent_projects.empty())
			{
				ImGui::MenuItem("(empty)", nullptr, false, false);
			}
			else
			{
				for (const auto& path : recent_projects)
				{
					if (ImGui::MenuItem(path.c_str()))
					{
						actions.open_recent_project = true;
						actions.recent_project_path = path;
					}
				}
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		ImGui::MenuItem("Exit", nullptr, false, false);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Cmd+Z", false, can_undo))
		{
			actions.undo = true;
		}
		if (ImGui::MenuItem("Redo", "Cmd+Shift+Z", false, can_redo))
		{
			actions.redo = true;
		}
		ImGui::Separator();
		ImGui::MenuItem("Preferences...", nullptr, false, false);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Reset Layout"))
		{
			request_reset_layout = true;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		ImGui::MenuItem("About DEdit", nullptr, false, false);
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void EnsureDockLayout(ImGuiID dockspace_id, const ImGuiViewport* viewport, bool force_reset)
{
	ImGuiDockNode* dockspace_node = ImGui::DockBuilderGetNode(dockspace_id);
	if (!force_reset && dockspace_node != nullptr)
	{
		return;
	}

	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

	ImGuiID dock_main = dockspace_id;
	ImGuiID dock_left = 0;
	ImGuiID dock_right = 0;
	ImGuiID dock_bottom = 0;

	ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.22f, &dock_left, &dock_main);
	ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right, &dock_main);
	ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_main);

	ImGuiID dock_left_top = 0;
	ImGuiID dock_left_bottom = 0;
	ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, 0.5f, &dock_left_bottom, &dock_left_top);

	ImGui::DockBuilderDockWindow("Project", dock_left_top);
	ImGui::DockBuilderDockWindow("Worlds", dock_left_bottom);
	ImGui::DockBuilderDockWindow("Scene", dock_left_bottom);
	ImGui::DockBuilderDockWindow("Properties", dock_right);
	ImGui::DockBuilderDockWindow("Console", dock_bottom);
	ImGui::DockBuilderDockWindow("Viewport", dock_main);

	ImGui::DockBuilderFinish(dockspace_id);
}
