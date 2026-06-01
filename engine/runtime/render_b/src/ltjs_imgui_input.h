#pragma once

// Engine-side bridge so the SDL event pump (kernel client.cpp) can forward
// input events to the Dear ImGui host without depending on ImGui/SDL headers.
//
// `sdl_event` is a pointer to an SDL_Event (passed as void* to keep this header
// dependency-free). Returns true if ImGui consumed the event. The engine still
// continues its normal event routing afterward so that global keys (e.g. ESC)
// reach the game shell regardless of ImGui focus.
bool ltjs_imgui_handle_sdl_event(const void *sdl_event);
