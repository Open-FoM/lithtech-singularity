#pragma once

struct ConsolePanelState
{
	char input[256] = {};
	bool auto_scroll = true;
	bool scroll_to_bottom = false;
};

void DrawConsolePanel(ConsolePanelState& state);
