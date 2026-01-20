#include "engine_render.h"

#include "lt_stream.h"

#include "ltjs_main_window_descriptor.h"

#include "render.h"
#include "dedit2_concommand.h"
#include "world_shared_bsp.h"
#include "ltvector.h"
#include "diligent_render.h"

#include <SDL3/SDL.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace
{
TextureCache* g_texture_cache = nullptr;
uint32 g_object_frame_code = 0;
uint16 g_texture_frame_code = 1;

LTObject* Engine_ProcessAttachment(LTObject*, Attachment*)
{
	return nullptr;
}

SharedTexture* Engine_GetSharedTexture(const char* filename)
{
	return g_texture_cache ? g_texture_cache->GetSharedTexture(filename) : nullptr;
}

TextureData* Engine_GetTexture(SharedTexture* texture)
{
	return g_texture_cache ? g_texture_cache->GetTexture(texture) : nullptr;
}

const char* Engine_GetTextureName(const SharedTexture* texture)
{
	return g_texture_cache ? g_texture_cache->GetTextureName(texture) : nullptr;
}

void Engine_FreeTexture(SharedTexture* texture)
{
	if (g_texture_cache)
	{
		g_texture_cache->FreeTexture(texture);
	}
}

void Engine_RunConsoleString(const char*)
{
}

void Engine_ConsolePrint(const char* msg, ...)
{
	if (!msg)
	{
		return;
	}

	va_list args;
	va_start(args, msg);
	std::vfprintf(stderr, msg, args);
	std::fprintf(stderr, "\n");
	va_end(args);
}

HLTPARAM Engine_GetParameter(const char*)
{
	return nullptr;
}

float Engine_GetParameterValueFloat(HLTPARAM)
{
	return 0.0f;
}

char* Engine_GetParameterValueString(HLTPARAM)
{
	return nullptr;
}

uint32 Engine_IncObjectFrameCode()
{
	return ++g_object_frame_code;
}

uint32 Engine_GetObjectFrameCode()
{
	return g_object_frame_code;
}

uint16 Engine_IncCurTextureFrameCode()
{
	return ++g_texture_frame_code;
}

void InitRenderStruct(RenderStruct& render_struct)
{
	std::memset(&render_struct, 0, sizeof(RenderStruct));
	render_struct.ProcessAttachment = Engine_ProcessAttachment;
	render_struct.GetSharedTexture = Engine_GetSharedTexture;
	render_struct.GetTexture = Engine_GetTexture;
	render_struct.GetTextureName = Engine_GetTextureName;
	render_struct.FreeTexture = Engine_FreeTexture;
	render_struct.RunConsoleString = Engine_RunConsoleString;
	render_struct.ConsolePrint = Engine_ConsolePrint;
	render_struct.GetParameter = Engine_GetParameter;
	render_struct.GetParameterValueFloat = Engine_GetParameterValueFloat;
	render_struct.GetParameterValueString = Engine_GetParameterValueString;
	render_struct.IncObjectFrameCode = Engine_IncObjectFrameCode;
	render_struct.GetObjectFrameCode = Engine_GetObjectFrameCode;
	render_struct.IncCurTextureFrameCode = Engine_IncCurTextureFrameCode;
	render_struct.m_GlobalLightDir.Init(0.0f, -2.0f, -1.0f);
	render_struct.m_GlobalLightDir.Norm();
}
} // namespace

RenderStruct g_Render;
RMode g_RMode;

bool InitEngineRenderer(SDL_Window* window, void* native_handle, EngineRenderContext& ctx, std::string& error)
{
	if (!window)
	{
		error = "Renderer init failed: missing SDL window.";
		return false;
	}

	g_texture_cache = &ctx.textures;
	ctx.render_struct = &g_Render;
	InitRenderStruct(*ctx.render_struct);

	rdll_RenderDLLSetup(ctx.render_struct);

	int width = 0;
	int height = 0;
	if (!SDL_GetWindowSizeInPixels(window, &width, &height))
	{
		error = "Renderer init failed: unable to query window size.";
		return false;
	}

	ltjs::MainWindowDescriptor window_desc{};
	window_desc.sdl_window = window;
	window_desc.native_handle = native_handle;

	RenderStructInit init{};
	std::memset(&init, 0, sizeof(init));
	init.m_Mode.m_Width = static_cast<uint32>(width);
	init.m_Mode.m_Height = static_cast<uint32>(height);
	init.m_Mode.m_BitDepth = 32;
	init.m_Mode.m_bHWTnL = true;
	init.m_hWnd = &window_desc;

	const int init_result = ctx.render_struct->Init(&init);
	if (init_result != RENDER_OK)
	{
		error = "Renderer init failed: renderer returned error.";
		return false;
	}

	ctx.render_struct->m_bInitted = true;
	ctx.render_struct->m_bLoaded = true;
	ctx.render_struct->m_Width = init.m_Mode.m_Width;
	ctx.render_struct->m_Height = init.m_Mode.m_Height;

	Diligent::IRenderDevice* raw_device = ctx.render_struct->GetRenderDevice ?
		ctx.render_struct->GetRenderDevice() : nullptr;
	Diligent::IDeviceContext* raw_context = ctx.render_struct->GetImmediateContext ?
		ctx.render_struct->GetImmediateContext() : nullptr;
	Diligent::ISwapChain* raw_swapchain = ctx.render_struct->GetSwapChain ?
		ctx.render_struct->GetSwapChain() : nullptr;
	if (!raw_device || !raw_context || !raw_swapchain)
	{
		char message[256];
		std::snprintf(
			message,
			sizeof(message),
			"Renderer init failed: device=%p context=%p swapchain=%p.",
			static_cast<void*>(raw_device),
			static_cast<void*>(raw_context),
			static_cast<void*>(raw_swapchain));
		error = message;
		return false;
	}

	ctx.device = raw_device;
	ctx.context = raw_context;
	ctx.swapchain = raw_swapchain;

	return true;
}

void ShutdownEngineRenderer(EngineRenderContext& ctx)
{
	if (ctx.render_struct && ctx.render_struct->Term)
	{
		ctx.render_struct->Term(true);
	}

	ctx.textures.Clear();
	ctx.swapchain.Release();
	ctx.context.Release();
	ctx.device.Release();
	ctx.render_struct = nullptr;
	ctx.world_loaded = false;
	g_texture_cache = nullptr;
	TermClientFileMgr();
}

std::vector<std::string> BuildFileMgrTrees(const std::string& project_root, const std::string& world_path)
{
	std::vector<std::string> roots;
	if (!project_root.empty())
	{
		const std::filesystem::path root(project_root);
		roots.push_back(root.string());
		roots.push_back((root / "Rez").string());
		roots.push_back((root / "Resources").string());
		roots.push_back((root / "Worlds").string());
		roots.push_back((root / "Resources" / "Rez").string());
		roots.push_back((root / "Resources" / "Worlds").string());
	}

	if (!world_path.empty())
	{
		const std::filesystem::path world_dir = std::filesystem::path(world_path).parent_path();
		if (!world_dir.empty())
		{
			roots.push_back(world_dir.string());
			const std::filesystem::path world_parent = world_dir.parent_path();
			if (!world_parent.empty())
			{
				roots.push_back(world_parent.string());
				roots.push_back((world_parent / "Rez").string());
			}
		}
	}

	return roots;
}

void SetEngineProjectRoot(EngineRenderContext& ctx, const std::string& root)
{
	ctx.project_root = root;
	ctx.textures.Clear();

	SetClientFileMgrTrees(BuildFileMgrTrees(root, {}));
}

TextureCache* GetEngineTextureCache()
{
	return g_texture_cache;
}

bool LoadRenderWorld(EngineRenderContext& ctx, const std::string& world_path, std::string& error)
{
	if (!ctx.render_struct || !ctx.render_struct->LoadWorldData)
	{
		error = "Renderer not initialized.";
		return false;
	}

	diligent_SetWorldOffset(LTVector(0.0f, 0.0f, 0.0f));

	const auto trees = BuildFileMgrTrees(ctx.project_root, world_path);
	SetClientFileMgrTrees(trees);
	DEdit2_Log("File manager trees:");
	for (const auto& root : trees)
	{
		DEdit2_Log("  %s", root.c_str());
	}

	ILTStream* stream = OpenFileStream(world_path);
	if (!stream)
	{
		error = "Failed to open world file.";
		return false;
	}

	uint32 version = 0;
	uint32 object_data_pos = 0;
	uint32 blind_object_data_pos = 0;
	uint32 lightgrid_pos = 0;
	uint32 collision_pos = 0;
	uint32 particle_pos = 0;
	uint32 render_pos = 0;

	if (!IWorldSharedBSP::ReadWorldHeader(
			stream,
			version,
			object_data_pos,
			blind_object_data_pos,
			lightgrid_pos,
			collision_pos,
			particle_pos,
			render_pos))
	{
		error = "World header invalid or unsupported.";
		stream->Release();
		return false;
	}

	uint32 info_len = 0;
	stream->Read(&info_len, sizeof(info_len));
	if (stream->ErrorStatus() != LT_OK)
	{
		error = "World header invalid or unsupported.";
		stream->Release();
		return false;
	}

	if (info_len > 0)
	{
		const uint32 skip_to = stream->GetPos() + info_len;
		if (stream->SeekTo(skip_to) != LT_OK)
		{
			error = "World header invalid or unsupported.";
			stream->Release();
			return false;
		}
	}

	LTVector world_min{};
	LTVector world_max{};
	LTVector world_offset{};
	*stream >> world_min >> world_max >> world_offset;
	if (stream->ErrorStatus() != LT_OK)
	{
		error = "World header invalid or unsupported.";
		stream->Release();
		return false;
	}

	diligent_SetWorldOffset(world_offset);

	if (stream->SeekTo(render_pos) != LT_OK)
	{
		error = "World file seek failed.";
		stream->Release();
		return false;
	}

	if (!ctx.render_struct->LoadWorldData(stream))
	{
		error = "Renderer failed to load world render data.";
		stream->Release();
		return false;
	}

	ctx.world_loaded = true;
	stream->Release();
	return true;
}
