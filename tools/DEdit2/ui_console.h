#pragma once

#include <string>

struct ConsolePanelState
{
	char input[256] = {};
	bool auto_scroll = true;
	bool scroll_to_bottom = false;
	size_t log_line_count = 0;
	std::string log_last_line;
	std::string log_buffer;
	int scroll_frames = 0;
};

void DrawConsolePanel(ConsolePanelState& state);
