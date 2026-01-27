#pragma once

#include "imgui.h"

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
	bool open_project_folder = false;
	bool open_recent_project = false;
	std::string recent_project_path;
	bool undo = false;
	bool redo = false;
	PrimitiveType create_primitive = PrimitiveType::None;
	bool select_all = false;
	bool select_none = false;
	bool select_inverse = false;
	bool hide_selected = false;
	bool unhide_all = false;
	bool freeze_selected = false;
	bool unfreeze_all = false;
	bool mirror_x = false;
	bool mirror_y = false;
	bool mirror_z = false;
	ViewModeAction view_mode = ViewModeAction::None;
};

void DrawMainMenuBar(
	bool& request_reset_layout,
	MainMenuActions& actions,
	const std::vector<std::string>& recent_projects,
	bool can_undo,
	bool can_redo);
void EnsureDockLayout(ImGuiID dockspace_id, const ImGuiViewport* viewport, bool force_reset);
