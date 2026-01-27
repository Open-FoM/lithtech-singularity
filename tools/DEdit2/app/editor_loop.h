#pragma once

struct DiligentContext;
struct EditorSession;
struct SDL_Window;

void RunEditorLoop(SDL_Window* window, DiligentContext& diligent, EditorSession& session);
