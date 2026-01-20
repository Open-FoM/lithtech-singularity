#pragma once

#include "imgui.h"

#include <string>
#include <vector>

struct MainMenuActions
{
	bool open_project_folder = false;
	bool open_recent_project = false;
	std::string recent_project_path;
	bool undo = false;
	bool redo = false;
};

void DrawMainMenuBar(
	bool& request_reset_layout,
	MainMenuActions& actions,
	const std::vector<std::string>& recent_projects,
	bool can_undo,
	bool can_redo);
void EnsureDockLayout(ImGuiID dockspace_id, const ImGuiViewport* viewport, bool force_reset);
