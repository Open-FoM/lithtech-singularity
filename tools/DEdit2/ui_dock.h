#pragma once

#include "multi_viewport.h"

#include "imgui.h"

#include <optional>
#include <string>
#include <vector>

/// Type of brush primitive to create.
enum class PrimitiveType {
  None,
  Box,
  Cylinder,
  Pyramid,
  Sphere,
  Dome,
  Plane
};

/// View mode for viewport. Matches ViewportPanelState::ViewMode.
enum class ViewModeAction {
	None = -1,
	Perspective = 0,
	Top,
	Bottom,
	Front,
	Back,
	Left,
	Right,
	ToggleOrthoPerspective  ///< Toggle between persp and last ortho view
};

struct MainMenuActions
{
	bool new_world = false;
	bool open_world = false;
	bool save_world = false;
	bool save_world_as = false;
	bool open_project_folder = false;
	bool open_recent_project = false;
	std::string recent_project_path;
	bool undo = false;
	bool redo = false;
	PrimitiveType create_primitive = PrimitiveType::None;
	bool enter_polygon_mode = false;
	bool select_all = false;
	bool select_none = false;
	bool select_inverse = false;
	bool select_brushes = false;
	bool select_lights = false;
	bool select_objects = false;
	bool select_world_models = false;
	bool select_by_class = false;
	std::string select_class_name;
	bool hide_selected = false;
	bool unhide_all = false;
	bool freeze_selected = false;
	bool unfreeze_all = false;
	bool mirror_x = false;
	bool mirror_y = false;
	bool mirror_z = false;
	bool rotate_selection = false;
	bool mirror_selection_dialog = false;
	bool open_marker_dialog = false;
	bool reset_marker = false;
	bool open_selection_filter = false;
	bool open_advanced_selection = false;
	ViewModeAction view_mode = ViewModeAction::None;
	std::optional<ViewportLayout> layout_change;
	bool cycle_viewport = false;
	bool toggle_fps = false;
	bool reset_panel_visibility = false;
	bool reset_splitters = false;
};

/// FPS display state pointer for View menu.
struct ViewportDisplayFlags {
	bool* show_fps = nullptr;
};

/// Panel visibility flags for the main menu.
struct PanelVisibilityFlags {
  bool* show_project = nullptr;
  bool* show_worlds = nullptr;
  bool* show_scene = nullptr;
  bool* show_properties = nullptr;
  bool* show_console = nullptr;
  bool* show_tools = nullptr;
};

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
	PanelVisibilityFlags* panels = nullptr,
	ViewportDisplayFlags* viewport_display = nullptr);
void EnsureDockLayout(ImGuiID dockspace_id, const ImGuiViewport* viewport, bool force_reset);
