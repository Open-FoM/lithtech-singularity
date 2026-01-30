#include "ui_dock.h"

#include "imgui_internal.h"

namespace {

const char* PrimaryShortcutLabel()
{
#if defined(__APPLE__)
	return "Cmd";
#else
	return "Ctrl";
#endif
}

}

void DrawMainMenuBar(
	bool& request_reset_layout,
	MainMenuActions& actions,
	const std::vector<std::string>& recent_projects,
	const std::vector<std::string>& scene_classes,
	bool can_undo,
	bool can_redo,
	bool has_selection,
	bool has_world,
	bool world_dirty,
	PanelVisibilityFlags* panels,
	ViewportDisplayFlags* viewport_display)
{
	if (!ImGui::BeginMainMenuBar())
	{
		return;
	}

	const char* primary_label = PrimaryShortcutLabel();
	auto primary = [primary_label](const char* key) { return std::string(primary_label) + "+" + key; };
	auto primary_shift = [primary_label](const char* key) { return std::string(primary_label) + "+Shift+" + key; };
	auto primary_numpad = [primary_label](const char* key) { return std::string(primary_label) + "+Numpad " + key; };

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New World", primary("N").c_str()))
		{
			actions.new_world = true;
		}
		if (ImGui::MenuItem("Open World...", primary("O").c_str()))
		{
			actions.open_world = true;
		}
		ImGui::Separator();
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
		if (ImGui::MenuItem("Save", primary("S").c_str(), false, has_world))
		{
			actions.save_world = true;
		}
		if (ImGui::MenuItem("Save As...", primary_shift("S").c_str(), false, has_world))
		{
			actions.save_world_as = true;
		}
		ImGui::Separator();
		ImGui::MenuItem("Exit", nullptr, false, false);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", primary("Z").c_str(), false, can_undo))
		{
			actions.undo = true;
		}
		if (ImGui::MenuItem("Redo", primary_shift("Z").c_str(), false, can_redo))
		{
			actions.redo = true;
		}
		ImGui::Separator();
		// Clipboard operations not yet implemented - keep disabled
		ImGui::MenuItem("Cut", primary("X").c_str(), false, false);
		ImGui::MenuItem("Copy", primary("C").c_str(), false, false);
		ImGui::MenuItem("Paste", primary("V").c_str(), false, false);
		ImGui::MenuItem("Delete", "Del", false, false);
		ImGui::Separator();
		if (ImGui::MenuItem("Select All", primary("A").c_str()))
		{
			actions.select_all = true;
		}
		if (ImGui::MenuItem("Select None", primary("D").c_str()))
		{
			actions.select_none = true;
		}
		if (ImGui::MenuItem("Select Inverse", primary("I").c_str()))
		{
			actions.select_inverse = true;
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("Select by Type"))
		{
			if (ImGui::MenuItem("All Brushes"))
			{
				actions.select_brushes = true;
			}
			if (ImGui::MenuItem("All Lights"))
			{
				actions.select_lights = true;
			}
			if (ImGui::MenuItem("All Objects"))
			{
				actions.select_objects = true;
			}
			if (ImGui::MenuItem("All World Models"))
			{
				actions.select_world_models = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Select by Class"))
		{
			if (scene_classes.empty())
			{
				ImGui::MenuItem("(no classes)", nullptr, false, false);
			}
			else
			{
				for (const auto& class_name : scene_classes)
				{
					if (ImGui::MenuItem(class_name.c_str()))
					{
						actions.select_by_class = true;
						actions.select_class_name = class_name;
					}
				}
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Selection Filter..."))
		{
			actions.open_selection_filter = true;
		}
		if (ImGui::MenuItem("Advanced Selection..."))
		{
			actions.open_advanced_selection = true;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Hide Selected", "H"))
		{
			actions.hide_selected = true;
		}
		if (ImGui::MenuItem("Unhide All", primary("H").c_str()))
		{
			actions.unhide_all = true;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Freeze Selected", "F"))
		{
			actions.freeze_selected = true;
		}
		if (ImGui::MenuItem("Unfreeze All", primary("F").c_str()))
		{
			actions.unfreeze_all = true;
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("Mirror"))
		{
			if (ImGui::MenuItem("Mirror X", primary_shift("X").c_str()))
			{
				actions.mirror_x = true;
			}
			if (ImGui::MenuItem("Mirror Y", primary_shift("Y").c_str()))
			{
				actions.mirror_y = true;
			}
			if (ImGui::MenuItem("Mirror Z", primary_shift("Z").c_str()))
			{
				actions.mirror_z = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Rotate Selection...", primary_shift("R").c_str()))
		{
			actions.rotate_selection = true;
		}
		if (ImGui::MenuItem("Mirror Selection...", nullptr))
		{
			actions.mirror_selection_dialog = true;
		}
		ImGui::Separator();
		ImGui::MenuItem("Preferences...", nullptr, false, false);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Brush"))
	{
		if (ImGui::BeginMenu("Create Primitive"))
		{
			if (ImGui::MenuItem("Box...", primary_shift("B").c_str()))
			{
				actions.create_primitive = PrimitiveType::Box;
			}
			if (ImGui::MenuItem("Cylinder...", primary_shift("C").c_str()))
			{
				actions.create_primitive = PrimitiveType::Cylinder;
			}
			if (ImGui::MenuItem("Pyramid...", primary_shift("Y").c_str()))
			{
				actions.create_primitive = PrimitiveType::Pyramid;
			}
			if (ImGui::MenuItem("Sphere...", primary_shift("S").c_str()))
			{
				actions.create_primitive = PrimitiveType::Sphere;
			}
			if (ImGui::MenuItem("Dome...", primary_shift("D").c_str()))
			{
				actions.create_primitive = PrimitiveType::Dome;
			}
			if (ImGui::MenuItem("Plane...", primary_shift("P").c_str()))
			{
				actions.create_primitive = PrimitiveType::Plane;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Draw Polygon...", "P"))
			{
				actions.enter_polygon_mode = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Operations", has_selection))
		{
			if (ImGui::MenuItem("Hollow...", nullptr, false, has_selection))
			{
				actions.csg_hollow = true;
			}
			if (ImGui::MenuItem("Carve...", nullptr, false, has_selection))
			{
				actions.csg_carve = true;
			}
			if (ImGui::MenuItem("Split...", nullptr, false, has_selection))
			{
				actions.csg_split = true;
			}
			if (ImGui::MenuItem("Join...", nullptr, false, has_selection))
			{
				actions.csg_join = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Triangulate", nullptr, false, has_selection))
			{
				actions.csg_triangulate = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Geometry", has_selection))
		{
			if (ImGui::MenuItem("Flip Normals", "N", false, has_selection))
			{
				actions.geometry_flip = true;
			}
			if (ImGui::MenuItem("Weld Vertices...", nullptr, false, has_selection))
			{
				actions.geometry_weld = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Extrude Faces...", nullptr, false, has_selection))
			{
				actions.geometry_extrude = true;
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
			if (ImGui::MenuItem("Bottom", primary_numpad("7").c_str()))
			{
				actions.view_mode = ViewModeAction::Bottom;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Front", "Numpad 1"))
			{
				actions.view_mode = ViewModeAction::Front;
			}
			if (ImGui::MenuItem("Back", primary_numpad("1").c_str()))
			{
				actions.view_mode = ViewModeAction::Back;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Right", "Numpad 3"))
			{
				actions.view_mode = ViewModeAction::Right;
			}
			if (ImGui::MenuItem("Left", primary_numpad("3").c_str()))
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
			if (ImGui::MenuItem("Quad (2x2)", primary_shift("4").c_str()))
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
		if (ImGui::BeginMenu("Marker"))
		{
			if (ImGui::MenuItem("Set Marker Position...", "Shift+M"))
			{
				actions.open_marker_dialog = true;
			}
			if (ImGui::MenuItem("Reset Marker to Origin", primary("M").c_str()))
			{
				actions.reset_marker = true;
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (viewport_display != nullptr && viewport_display->show_fps != nullptr)
		{
			if (ImGui::MenuItem("Show FPS", nullptr, *viewport_display->show_fps))
			{
				*viewport_display->show_fps = !*viewport_display->show_fps;
			}
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Reset Splitters"))
		{
			actions.reset_splitters = true;
		}
		if (ImGui::MenuItem("Reset Layout"))
		{
			request_reset_layout = true;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		if (ImGui::BeginMenu("Panels"))
		{
			if (panels != nullptr)
			{
				if (panels->show_project != nullptr)
				{
					if (ImGui::MenuItem("Project", nullptr, *panels->show_project))
					{
						*panels->show_project = !*panels->show_project;
					}
				}
				if (panels->show_worlds != nullptr)
				{
					if (ImGui::MenuItem("Worlds", nullptr, *panels->show_worlds))
					{
						*panels->show_worlds = !*panels->show_worlds;
					}
				}
				if (panels->show_scene != nullptr)
				{
					if (ImGui::MenuItem("Scene", nullptr, *panels->show_scene))
					{
						*panels->show_scene = !*panels->show_scene;
					}
				}
				if (panels->show_properties != nullptr)
				{
					if (ImGui::MenuItem("Properties", nullptr, *panels->show_properties))
					{
						*panels->show_properties = !*panels->show_properties;
					}
				}
				if (panels->show_console != nullptr)
				{
					if (ImGui::MenuItem("Console", nullptr, *panels->show_console))
					{
						*panels->show_console = !*panels->show_console;
					}
				}
				if (panels->show_tools != nullptr)
				{
					if (ImGui::MenuItem("Tools", nullptr, *panels->show_tools))
					{
						*panels->show_tools = !*panels->show_tools;
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Reset Panel Visibility"))
				{
					actions.reset_panel_visibility = true;
				}
			}
			ImGui::EndMenu();
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
