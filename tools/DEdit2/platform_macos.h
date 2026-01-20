#pragma once

#include <string>

struct SDL_Window;

void* CreateMetalView(SDL_Window* window);
void DestroyMetalView(void* view);
void* GetMetalLayer(void* view);
bool OpenFolderDialog(const std::string& initial_dir, std::string& out_path);
