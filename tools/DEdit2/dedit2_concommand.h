#pragma once

#include "editor_state.h"

#include <cstdarg>
#include <string>
#include <vector>

struct DEdit2ConsoleBindings
{
	const std::vector<TreeNode>* project_nodes = nullptr;
	const std::vector<NodeProperties>* project_props = nullptr;
	const std::vector<TreeNode>* scene_nodes = nullptr;
	const std::vector<NodeProperties>* scene_props = nullptr;
};

namespace Diligent
{
	class ITextureView;
}

struct DEdit2TexViewState
{
	bool enabled = false;
	std::string name;
	Diligent::ITextureView* view = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
};

void DEdit2_InitConsoleCommands();
void DEdit2_TermConsoleCommands();
void DEdit2_CommandHandler(const char* command);

void DEdit2_SetConsoleBindings(const DEdit2ConsoleBindings& bindings);
const DEdit2TexViewState& DEdit2_GetTexViewState();

void DEdit2_Log(const char* fmt, ...);
void DEdit2_LogV(const char* fmt, va_list args);
void DEdit2_ClearLog();
const std::vector<std::string>& DEdit2_GetLog();

// Console variables kept for parity with the legacy DEdit console.
extern int g_DEdit2MipMapOffset;
extern int g_DEdit2Bilinear;
extern int g_DEdit2MaxTextureMemory;
extern int g_DEdit2WorldTexelUVAuto;
