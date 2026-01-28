#include "ui_dock.h"

#include "imgui_internal.h"

void DrawMainMenuBar(
	bool& request_reset_layout,
	MainMenuActions& actions,
	const std::vector<std::string>& recent_projects,
	bool can_undo,
	bool can_redo,
	bool* show_tools_panel)
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
		if (ImGui::MenuItem("Select All", "Cmd+A"))
		{
			actions.select_all = true;
		}
		if (ImGui::MenuItem("Select None", "Cmd+D"))
		{
			actions.select_none = true;
		}
		if (ImGui::MenuItem("Select Inverse", "Cmd+I"))
		{
			actions.select_inverse = true;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Hide Selected", "H"))
		{
			actions.hide_selected = true;
		}
		if (ImGui::MenuItem("Unhide All", "Shift+H"))
		{
			actions.unhide_all = true;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Freeze Selected", "F"))
		{
			actions.freeze_selected = true;
		}
		if (ImGui::MenuItem("Unfreeze All", "Shift+F"))
		{
			actions.unfreeze_all = true;
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("Mirror"))
		{
			if (ImGui::MenuItem("Mirror X", "Ctrl+Shift+X"))
			{
				actions.mirror_x = true;
			}
			if (ImGui::MenuItem("Mirror Y", "Ctrl+Shift+Y"))
			{
				actions.mirror_y = true;
			}
			if (ImGui::MenuItem("Mirror Z", "Ctrl+Shift+Z"))
			{
				actions.mirror_z = true;
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		ImGui::MenuItem("Preferences...", nullptr, false, false);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Brush"))
	{
		if (ImGui::BeginMenu("Create Primitive"))
		{
			if (ImGui::MenuItem("Box...", "Ctrl+Shift+B"))
			{
				actions.create_primitive = PrimitiveType::Box;
			}
			if (ImGui::MenuItem("Cylinder...", "Ctrl+Shift+C"))
			{
				actions.create_primitive = PrimitiveType::Cylinder;
			}
			if (ImGui::MenuItem("Pyramid...", "Ctrl+Shift+Y"))
			{
				actions.create_primitive = PrimitiveType::Pyramid;
			}
			if (ImGui::MenuItem("Sphere...", "Ctrl+Shift+S"))
			{
				actions.create_primitive = PrimitiveType::Sphere;
			}
			if (ImGui::MenuItem("Dome...", "Ctrl+Shift+D"))
			{
				actions.create_primitive = PrimitiveType::Dome;
			}
			if (ImGui::MenuItem("Plane...", "Ctrl+Shift+P"))
			{
				actions.create_primitive = PrimitiveType::Plane;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::BeginMenu("Viewport"))
		{
			if (ImGui::MenuItem("Perspective", "Numpad 5"))
			{
				actions.view_mode = ViewModeAction::Perspective;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Top", "Numpad 7"))
			{
				actions.view_mode = ViewModeAction::Top;
			}
			if (ImGui::MenuItem("Bottom", "Ctrl+Numpad 7"))
			{
				actions.view_mode = ViewModeAction::Bottom;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Front", "Numpad 1"))
			{
				actions.view_mode = ViewModeAction::Front;
			}
			if (ImGui::MenuItem("Back", "Ctrl+Numpad 1"))
			{
				actions.view_mode = ViewModeAction::Back;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Right", "Numpad 3"))
			{
				actions.view_mode = ViewModeAction::Right;
			}
			if (ImGui::MenuItem("Left", "Ctrl+Numpad 3"))
			{
				actions.view_mode = ViewModeAction::Left;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Layout"))
		{
			if (ImGui::MenuItem("Single Viewport"))
			{
				actions.layout_change = ViewportLayout::Single;
			}
			if (ImGui::MenuItem("Two Vertical (Left/Right)"))
			{
				actions.layout_change = ViewportLayout::TwoVertical;
			}
			if (ImGui::MenuItem("Two Horizontal (Top/Bottom)"))
			{
				actions.layout_change = ViewportLayout::TwoHorizontal;
			}
			if (ImGui::MenuItem("Three (Large Left)"))
			{
				actions.layout_change = ViewportLayout::ThreeLeft;
			}
			if (ImGui::MenuItem("Three (Large Top)"))
			{
				actions.layout_change = ViewportLayout::ThreeTop;
			}
			if (ImGui::MenuItem("Quad (2x2)", "Ctrl+Shift+4"))
			{
				actions.layout_change = ViewportLayout::Quad;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Cycle Active Viewport", "Tab"))
			{
				actions.cycle_viewport = true;
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Reset Layout"))
		{
			request_reset_layout = true;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		if (show_tools_panel != nullptr)
		{
			if (ImGui::MenuItem("Tools", nullptr, *show_tools_panel))
			{
				*show_tools_panel = !*show_tools_panel;
			}
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
	ImGui::DockBuilderDockWindow("Tools", dock_left_top);
	ImGui::DockBuilderDockWindow("Properties", dock_right);
	ImGui::DockBuilderDockWindow("Console", dock_bottom);
	ImGui::DockBuilderDockWindow("Viewport", dock_main);

	ImGui::DockBuilderFinish(dockspace_id);
}
