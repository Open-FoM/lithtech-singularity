#pragma once

#include <string>
#include <vector>

struct WorldBrowserEntry
{
	std::string name;
	std::string path;
	std::string size_label;
	std::string modified_label;
};

struct WorldBrowserState
{
	std::vector<WorldBrowserEntry> entries;
	std::string error;
	std::string last_root;
	bool refresh = true;
	int selected_index = -1;
};

struct WorldBrowserAction
{
	bool load_world = false;
	std::string world_path;
};

void DrawWorldBrowserPanel(
	WorldBrowserState& state,
	const std::string& project_root,
	WorldBrowserAction& action);
