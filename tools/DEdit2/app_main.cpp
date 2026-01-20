#include "dedit2_concommand.h"
#include "editor_state.h"
#include "editor_transfer.h"
#include "platform_macos.h"
#include "ui_console.h"
#include "ui_dock.h"
#include "ui_project.h"
#include "ui_worlds.h"
#include "ui_properties.h"
#include "ui_scene.h"
#include "ui_viewport.h"
#include "viewport_render.h"
#include "viewport_gizmo.h"
#include "ui_shared.h"
#include "undo_stack.h"
#include "engine_render.h"

#include "imgui.h"

#include "ImGuiImplSDL3.hpp"
#include "diligent_render.h"
#include "rendererconsolevars.h"
#include "ltrotation.h"
#include "ltvector.h"
#include "renderstruct.h"

#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace
{
namespace fs = std::filesystem;

struct EditorPaths
{
	std::string project_root;
	std::string world_file;
};

void SaveRecentProjects(const std::vector<std::string>& recent_projects);

EditorPaths ParseEditorPaths(int argc, char** argv)
{
	EditorPaths paths{};
	if (const char* env_root = std::getenv("LTJS_PROJECT_ROOT"))
	{
		paths.project_root = env_root;
	}
	if (const char* env_world = std::getenv("LTJS_WORLD_FILE"))
	{
		paths.world_file = env_world;
	}

	for (int i = 1; i < argc; ++i)
	{
		const std::string arg = argv[i];
		if ((arg == "--project-root" || arg == "--project") && (i + 1) < argc)
		{
			paths.project_root = argv[++i];
		}
		else if ((arg == "--world" || arg == "--map") && (i + 1) < argc)
		{
			paths.world_file = argv[++i];
		}
	}

	if (!paths.world_file.empty())
	{
		fs::path world_path = fs::path(paths.world_file);
		if (world_path.is_relative() && !paths.project_root.empty())
		{
			world_path = fs::path(paths.project_root) / world_path;
		}
		paths.world_file = world_path.string();
	}

	return paths;
}

std::string FindCompiledWorldPath(const std::string& world_file, const std::string& project_root)
{
	if (world_file.empty())
	{
		return std::string();
	}

	fs::path world_path(world_file);
	if (world_path.is_relative() && !project_root.empty())
	{
		world_path = fs::path(project_root) / world_path;
	}
	if (world_path.extension() == ".dat")
	{
		return world_path.string();
	}

	fs::path candidate = world_path;
	candidate.replace_extension(".dat");
	if (fs::exists(candidate))
	{
		return candidate.string();
	}

	if (!project_root.empty())
	{
		const fs::path root(project_root);
		const fs::path basename = world_path.stem();
		const fs::path worlds_dat = root / "Worlds" / (basename.string() + ".dat");
		if (fs::exists(worlds_dat))
		{
			return worlds_dat.string();
		}
		const fs::path rez_worlds_dat = root / "Rez" / "Worlds" / (basename.string() + ".dat");
		if (fs::exists(rez_worlds_dat))
		{
			return rez_worlds_dat.string();
		}
	}

	return std::string();
}

const NodeProperties* FindWorldProperties(const std::vector<NodeProperties>& props)
{
	for (const auto& prop : props)
	{
		if (prop.type == "World")
		{
			return &prop;
		}
	}

	return nullptr;
}

void ApplyWorldSettingsToRenderer(const NodeProperties& world_props)
{
	g_CV_FogEnable = world_props.fog_enabled ? 1 : 0;
	g_CV_FogNearZ = world_props.fog_near;
	g_CV_FogFarZ = world_props.fog_far;
	g_CV_FarZ = world_props.far_z;

	g_CV_FogColorR = static_cast<int>(std::clamp(world_props.color[0], 0.0f, 1.0f) * 255.0f);
	g_CV_FogColorG = static_cast<int>(std::clamp(world_props.color[1], 0.0f, 1.0f) * 255.0f);
	g_CV_FogColorB = static_cast<int>(std::clamp(world_props.color[2], 0.0f, 1.0f) * 255.0f);

	g_CV_DrawSky = 0;
}

std::string TrimLine(const std::string& value)
{
	size_t start = 0;
	while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
	{
		++start;
	}
	size_t end = value.size();
	while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
	{
		--end;
	}
	return value.substr(start, end - start);
}

std::string GetRecentProjectsPath()
{
	char* pref_path = SDL_GetPrefPath("LithTech", "DEdit2");
	if (!pref_path)
	{
		return {};
	}
	std::string path = pref_path;
	SDL_free(pref_path);
	if (!path.empty() && path.back() != '/' && path.back() != '\\')
	{
		path.push_back('/');
	}
	path += "recent_projects.txt";
	return path;
}

bool IsValidRecentProjectPath(const std::string& path)
{
	if (path.empty())
	{
		return false;
	}
	std::error_code ec;
	return fs::exists(path, ec) && fs::is_directory(path, ec);
}

void LoadRecentProjects(std::vector<std::string>& recent_projects)
{
	recent_projects.clear();
	const std::string path = GetRecentProjectsPath();
	if (path.empty())
	{
		return;
	}

	std::ifstream input(path);
	if (!input.is_open())
	{
		return;
	}

	std::unordered_set<std::string> seen;
	std::string line;
	while (std::getline(input, line))
	{
		std::string trimmed = TrimLine(line);
		if (trimmed.empty())
		{
			continue;
		}
		if (seen.insert(trimmed).second)
		{
			recent_projects.push_back(trimmed);
		}
	}
}

void PruneRecentProjects(std::vector<std::string>& recent_projects)
{
	const size_t before = recent_projects.size();
	recent_projects.erase(
		std::remove_if(
			recent_projects.begin(),
			recent_projects.end(),
			[](const std::string& path)
			{
				return !IsValidRecentProjectPath(path);
			}),
		recent_projects.end());
	if (recent_projects.size() != before)
	{
		SaveRecentProjects(recent_projects);
	}
}

void SaveRecentProjects(const std::vector<std::string>& recent_projects)
{
	const std::string path = GetRecentProjectsPath();
	if (path.empty())
	{
		return;
	}

	std::ofstream output(path, std::ios::trunc);
	if (!output.is_open())
	{
		return;
	}

	for (const auto& entry : recent_projects)
	{
		output << entry << '\n';
	}
}

void PushRecentProject(std::vector<std::string>& recent_projects, const std::string& path)
{
	if (!IsValidRecentProjectPath(path))
	{
		return;
	}

	auto existing = std::find(recent_projects.begin(), recent_projects.end(), path);
	if (existing != recent_projects.end())
	{
		recent_projects.erase(existing);
	}
	recent_projects.insert(recent_projects.begin(), path);
	constexpr size_t kMaxRecent = 10;
	if (recent_projects.size() > kMaxRecent)
	{
		recent_projects.resize(kMaxRecent);
	}
	SaveRecentProjects(recent_projects);
}

Diligent::float3 Cross(const Diligent::float3& a, const Diligent::float3& b)
{
	return Diligent::float3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

Diligent::float3 Normalize(const Diligent::float3& v)
{
	const float len_sq = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len_sq <= 0.0f)
	{
		return Diligent::float3(0.0f, 0.0f, 0.0f);
	}
	const float inv_len = 1.0f / std::sqrt(len_sq);
	return Diligent::float3(v.x * inv_len, v.y * inv_len, v.z * inv_len);
}

void ComputeCameraBasis(
	const ViewportPanelState& state,
	Diligent::float3& out_pos,
	Diligent::float3& out_forward,
	Diligent::float3& out_right,
	Diligent::float3& out_up)
{
	const float cp = std::cos(state.orbit_pitch);
	const float sp = std::sin(state.orbit_pitch);
	const float cy = std::cos(state.orbit_yaw);
	const float sy = std::sin(state.orbit_yaw);

	out_forward = Normalize(Diligent::float3(sy * cp, sp, cy * cp));
	const Diligent::float3 world_up(0.0f, 1.0f, 0.0f);
	out_right = Normalize(Cross(world_up, out_forward));
	out_up = Cross(out_forward, out_right);

	if (state.fly_mode)
	{
		out_pos = Diligent::float3(state.fly_pos[0], state.fly_pos[1], state.fly_pos[2]);
	}
	else
	{
		const Diligent::float3 target(state.target[0], state.target[1], state.target[2]);
		out_pos = Diligent::float3(
			target.x - out_forward.x * state.orbit_distance,
			target.y - out_forward.y * state.orbit_distance,
			target.z - out_forward.z * state.orbit_distance);
	}
}
bool EnsureVulkanLoaderAvailable()
{
	void* handle = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
	if (handle == nullptr)
	{
		std::fprintf(
			stderr,
			"Vulkan loader (libvulkan.dylib) not found. Install via Homebrew "
			"(brew install vulkan-loader molten-vk) and ensure /opt/homebrew/lib is "
			"on the loader path or set VULKAN_SDK=/opt/homebrew.\n");
		return false;
	}

	dlclose(handle);
	return true;
}

struct ViewportRenderResources
{
	Diligent::RefCntAutoPtr<Diligent::ITexture> color;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> color_rtv;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> color_srv;
	Diligent::RefCntAutoPtr<Diligent::ITexture> depth;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> depth_dsv;
	uint32_t width = 0;
	uint32_t height = 0;
};

struct DiligentContext
{
	EngineRenderContext engine;
	std::unique_ptr<Diligent::ImGuiImplSDL3> imgui;
	void* metal_view = nullptr;
	ViewportRenderResources viewport;
	bool viewport_visible = false;
	ViewportGridRenderer grid_renderer;
	bool grid_ready = false;
};

bool CreateViewportTargets(DiligentContext& ctx, uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
	{
		return false;
	}

	if (ctx.viewport.width == width && ctx.viewport.height == height &&
		ctx.viewport.color && ctx.viewport.depth)
	{
		return true;
	}

	ctx.viewport = {};
	ctx.viewport.width = width;
	ctx.viewport.height = height;

	Diligent::TextureDesc color_desc;
	color_desc.Name = "DEdit2 Viewport Color";
	color_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	color_desc.Width = width;
	color_desc.Height = height;
	color_desc.MipLevels = 1;
	color_desc.ArraySize = 1;
	color_desc.Format = ctx.engine.swapchain
		? ctx.engine.swapchain->GetDesc().ColorBufferFormat
		: Diligent::TEX_FORMAT_RGBA8_UNORM;
	color_desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

	ctx.engine.device->CreateTexture(color_desc, nullptr, &ctx.viewport.color);
	if (!ctx.viewport.color)
	{
		return false;
	}

	ctx.viewport.color_rtv = ctx.viewport.color->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	ctx.viewport.color_srv = ctx.viewport.color->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);

	Diligent::TextureDesc depth_desc;
	depth_desc.Name = "DEdit2 Viewport Depth";
	depth_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	depth_desc.Width = width;
	depth_desc.Height = height;
	depth_desc.MipLevels = 1;
	depth_desc.ArraySize = 1;
	depth_desc.Format = Diligent::TEX_FORMAT_D32_FLOAT;
	depth_desc.BindFlags = Diligent::BIND_DEPTH_STENCIL;

	ctx.engine.device->CreateTexture(depth_desc, nullptr, &ctx.viewport.depth);
	if (!ctx.viewport.depth)
	{
		return false;
	}

	ctx.viewport.depth_dsv = ctx.viewport.depth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
	return true;
}

void RenderViewport(DiligentContext& ctx, const ViewportPanelState& viewport_state, const NodeProperties* world_props)
{
	if (!ctx.viewport_visible || !ctx.viewport.color_rtv || !ctx.viewport.depth_dsv || !ctx.engine.context)
	{
		return;
	}

	Diligent::ITextureView* render_target = ctx.viewport.color_rtv;
	Diligent::ITextureView* depth_target = ctx.viewport.depth_dsv;
	diligent_SetOutputTargets(render_target, depth_target);

	ctx.engine.context->SetRenderTargets(
		1,
		&render_target,
		depth_target,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	float clear_color[] = {0.08f, 0.10f, 0.14f, 1.0f};
	if (world_props)
	{
		clear_color[0] = world_props->background_color[0];
		clear_color[1] = world_props->background_color[1];
		clear_color[2] = world_props->background_color[2];
	}

	ctx.engine.context->ClearRenderTarget(render_target, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	ctx.engine.context->ClearDepthStencil(
		depth_target,
		Diligent::CLEAR_DEPTH_FLAG,
		1.0f,
		0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (ctx.engine.world_loaded && ctx.engine.render_struct && ctx.engine.render_struct->RenderScene)
	{
		const float aspect = ctx.viewport.height > 0 ? static_cast<float>(ctx.viewport.width) /
			static_cast<float>(ctx.viewport.height) : 1.0f;
		const float fov_y = Diligent::PI_F / 4.0f;
		const float fov_x = 2.0f * std::atan(std::tan(fov_y * 0.5f) * aspect);

		Diligent::float3 cam_pos{};
		Diligent::float3 cam_forward{};
		Diligent::float3 cam_right{};
		Diligent::float3 cam_up{};
		ComputeCameraBasis(viewport_state, cam_pos, cam_forward, cam_right, cam_up);

		SceneDesc scene{};
		scene.m_DrawMode = DRAWMODE_NORMAL;
		scene.m_FrameTime = ImGui::GetIO().DeltaTime;
		scene.m_fActualFrameTime = scene.m_FrameTime;
		scene.m_GlobalLightScale.Init(1.0f, 1.0f, 1.0f);
		scene.m_GlobalLightAdd.Init(0.0f, 0.0f, 0.0f);
		scene.m_GlobalModelLightAdd.Init(0.0f, 0.0f, 0.0f);
		scene.m_Rect.left = 0;
		scene.m_Rect.top = 0;
		scene.m_Rect.right = static_cast<int>(ctx.viewport.width);
		scene.m_Rect.bottom = static_cast<int>(ctx.viewport.height);
		scene.m_xFov = fov_x;
		scene.m_yFov = fov_y;
		scene.m_Pos.Init(cam_pos.x, cam_pos.y, cam_pos.z);
		scene.m_Rotation = LTRotation(
			LTVector(cam_forward.x, cam_forward.y, cam_forward.z),
			LTVector(cam_up.x, cam_up.y, cam_up.z));
		scene.m_SkyObjects = nullptr;
		scene.m_nSkyObjects = 0;
		scene.m_pObjectList = nullptr;
		scene.m_ObjectListSize = 0;

		if (ctx.engine.render_struct->Start3D)
		{
			ctx.engine.render_struct->Start3D();
		}
		ctx.engine.render_struct->RenderScene(&scene);
		if (ctx.engine.render_struct->End3D)
		{
			ctx.engine.render_struct->End3D();
		}
	}

	if (ctx.grid_ready)
	{
		const float aspect = ctx.viewport.height > 0 ? static_cast<float>(ctx.viewport.width) /
			static_cast<float>(ctx.viewport.height) : 1.0f;
		const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_state, aspect);
		UpdateViewportGridConstants(ctx.engine.context, ctx.grid_renderer, view_proj);

		Diligent::Viewport vp;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<float>(ctx.viewport.width);
		vp.Height = static_cast<float>(ctx.viewport.height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		ctx.engine.context->SetViewports(1, &vp, 0, 0);

		DrawViewportGrid(ctx.engine.context, ctx.grid_renderer, viewport_state.show_grid, viewport_state.show_axes);
	}

	diligent_SetOutputTargets(nullptr, nullptr);
}

bool InitDiligent(SDL_Window* window, DiligentContext& ctx)
{
#if defined(__APPLE__)
	if (!EnsureVulkanLoaderAvailable())
	{
		return false;
	}
#endif

	ctx.metal_view = CreateMetalView(window);
	if (ctx.metal_view == nullptr)
	{
		std::fprintf(stderr, "Diligent: failed to create Metal view for SDL window.\n");
		return false;
	}

	std::string error;
	if (!InitEngineRenderer(window, ctx.metal_view, ctx.engine, error))
	{
		std::fprintf(stderr, "Diligent: %s\n", error.c_str());
		DestroyMetalView(ctx.metal_view);
		ctx.metal_view = nullptr;
		return false;
	}

	Diligent::ImGuiDiligentCreateInfo imgui_ci{ctx.engine.device, ctx.engine.swapchain->GetDesc()};
	ctx.imgui = Diligent::ImGuiImplSDL3::Create(imgui_ci, window);
	if (!ctx.imgui)
	{
		std::fprintf(stderr, "Diligent: failed to create ImGui renderer.\n");
		ShutdownEngineRenderer(ctx.engine);
		DestroyMetalView(ctx.metal_view);
		ctx.metal_view = nullptr;
		return false;
	}

	return true;
}
} // namespace

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	RegisterEditorTransferFormat();
	DEdit2_InitConsoleCommands();

	const float display_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags =
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
#if defined(__APPLE__)
	window_flags |= SDL_WINDOW_METAL;
#else
	window_flags |= SDL_WINDOW_VULKAN;
#endif
	SDL_Window* window = SDL_CreateWindow(
		"DEdit2 (SDL/ImGui)",
		static_cast<int>(1280 * display_scale),
		static_cast<int>(800 * display_scale),
		window_flags);
	if (window == nullptr)
	{
		std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(display_scale);
	style.FontScaleDpi = display_scale;

	EditorPaths paths = ParseEditorPaths(argc, argv);
	std::string project_root = paths.project_root;
	std::string world_file = paths.world_file;
	std::string project_error;
	std::string scene_error;
	std::vector<std::string> recent_projects;
	LoadRecentProjects(recent_projects);
	PruneRecentProjects(recent_projects);
	UndoStack undo_stack;

	DiligentContext diligent{};
	if (!InitDiligent(window, diligent))
	{
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	diligent.grid_ready = InitViewportGridRenderer(
		diligent.engine.device,
		Diligent::TEX_FORMAT_RGBA8_UNORM,
		Diligent::TEX_FORMAT_D32_FLOAT,
		diligent.grid_renderer);
	SetEngineProjectRoot(diligent.engine, project_root);

	std::vector<NodeProperties> project_props;
	std::vector<NodeProperties> scene_props;
	std::vector<TreeNode> project_nodes;
	std::vector<TreeNode> scene_nodes;
	if (!project_root.empty())
	{
		project_nodes = BuildProjectTree(project_root, project_props, project_error);
	}
	ProjectPanelState project_panel;
	WorldBrowserState world_browser;
	ScenePanelState scene_panel;
	ConsolePanelState console_panel;
	ViewportPanelState viewport_panel;
	project_panel.error = project_error;
	project_panel.selected_id = project_nodes.empty() ? -1 : 0;
	scene_panel.error = scene_error;
	scene_panel.selected_id = scene_nodes.empty() ? -1 : 0;
	SelectionTarget active_target = SelectionTarget::Project;
	DEdit2_SetConsoleBindings({&project_nodes, &project_props, &scene_nodes, &scene_props});

	auto load_scene_world = [&](const std::string& path)
	{
		world_file = path;
		diligent.engine.world_loaded = false;
		scene_nodes = BuildSceneTree(world_file, scene_props, scene_error);
		scene_panel.error = scene_error;
		scene_panel.selected_id = scene_nodes.empty() ? -1 : 0;
		scene_panel.tree_ui = {};
		active_target = SelectionTarget::Scene;

		if (const NodeProperties* world_props = FindWorldProperties(scene_props))
		{
			ApplyWorldSettingsToRenderer(*world_props);
		}

		const std::string compiled_path = FindCompiledWorldPath(world_file, project_root);
		if (!compiled_path.empty())
		{
			std::string render_error;
			if (!LoadRenderWorld(diligent.engine, compiled_path, render_error))
			{
				std::fprintf(stderr, "World render load failed: %s\n", render_error.c_str());
			}
		}
	};

	if (!world_file.empty())
	{
		load_scene_world(world_file);
	}

	bool done = false;
	bool request_reset_layout = true;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			diligent.imgui->HandleSDLEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
			{
				done = true;
			}
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
				event.window.windowID == SDL_GetWindowID(window))
			{
				done = true;
			}
			if (event.type == SDL_EVENT_WINDOW_RESIZED ||
				event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
			{
				int w = 0;
				int h = 0;
				if (SDL_GetWindowSizeInPixels(window, &w, &h))
				{
					diligent.engine.swapchain->Resize(
						static_cast<Diligent::Uint32>(w),
						static_cast<Diligent::Uint32>(h),
						Diligent::SURFACE_TRANSFORM_OPTIMAL);
				}
			}
		}

		if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		const auto& sc_desc = diligent.engine.swapchain->GetDesc();
		diligent.imgui->NewFrame(sc_desc.Width, sc_desc.Height, sc_desc.PreTransform);

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGuiID dockspace_id = ImGui::GetID("DEditDockSpace");
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		EnsureDockLayout(dockspace_id, viewport, request_reset_layout);
		request_reset_layout = false;
		ImGui::DockSpaceOverViewport(dockspace_id, viewport, dockspace_flags);

		MainMenuActions menu_actions{};
		DrawMainMenuBar(
			request_reset_layout,
			menu_actions,
			recent_projects,
			undo_stack.CanUndo(),
			undo_stack.CanRedo());
		ImGuiIO& io = ImGui::GetIO();
		bool trigger_undo = menu_actions.undo;
		bool trigger_redo = menu_actions.redo;
		if (!io.WantCaptureKeyboard)
		{
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
			{
				if (io.KeyShift)
				{
					trigger_redo = true;
				}
				else
				{
					trigger_undo = true;
				}
			}
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
			{
				trigger_redo = true;
			}
		}
		if (trigger_undo)
		{
			undo_stack.Undo(project_nodes, scene_nodes);
		}
		if (trigger_redo)
		{
			undo_stack.Redo(project_nodes, scene_nodes);
		}
		if (menu_actions.open_project_folder)
		{
			std::string selected_path;
			const std::string initial_path = project_root.empty() ? fs::current_path().string() : project_root;
			if (OpenFolderDialog(initial_path, selected_path))
			{
				project_root = selected_path;
				project_error.clear();
				project_nodes = BuildProjectTree(project_root, project_props, project_error);
				project_panel.error = project_error;
				project_panel.selected_id = project_nodes.empty() ? -1 : 0;
				project_panel.tree_ui = {};
				world_browser.refresh = true;
				SetEngineProjectRoot(diligent.engine, project_root);
				PushRecentProject(recent_projects, project_root);
			}
		}
		if (menu_actions.open_recent_project)
		{
			project_root = menu_actions.recent_project_path;
			project_error.clear();
			project_nodes = BuildProjectTree(project_root, project_props, project_error);
			project_panel.error = project_error;
			project_panel.selected_id = project_nodes.empty() ? -1 : 0;
			project_panel.tree_ui = {};
			world_browser.refresh = true;
			SetEngineProjectRoot(diligent.engine, project_root);
			PushRecentProject(recent_projects, project_root);
		}

		ProjectContextAction project_action{};
		DrawProjectPanel(
			project_panel,
			project_root,
			project_nodes,
			project_props,
			active_target,
			&project_action,
			&undo_stack);
		if (project_panel.error != project_error)
		{
			project_error = project_panel.error;
		}
		if (project_action.load_world)
		{
			load_scene_world(project_action.world_path);
		}
		WorldBrowserAction world_action{};
		DrawWorldBrowserPanel(world_browser, project_root, world_action);
		if (world_action.load_world)
		{
			load_scene_world(world_action.world_path);
		}
		DrawScenePanel(scene_panel, scene_nodes, scene_props, active_target, &undo_stack);
		if (scene_panel.error != scene_error)
		{
			scene_error = scene_panel.error;
		}
		DrawPropertiesPanel(
			active_target,
			project_nodes,
			project_props,
			project_panel.selected_id,
			scene_nodes,
			scene_props,
			scene_panel.selected_id);

		DrawConsolePanel(console_panel);
		diligent.viewport_visible = false;
		if (ImGui::Begin("Viewport"))
		{
			if (ImGui::Checkbox("Grid", &viewport_panel.show_grid))
			{
				// toggle only
			}
			ImGui::SameLine();
			if (ImGui::Checkbox("Axes", &viewport_panel.show_axes))
			{
				// toggle only
			}
			ImGui::SameLine();
			bool previous_fly_mode = viewport_panel.fly_mode;
			ImGui::Checkbox("Fly", &viewport_panel.fly_mode);
			if (viewport_panel.fly_mode != previous_fly_mode)
			{
				if (viewport_panel.fly_mode)
				{
					SyncFlyFromOrbit(viewport_panel);
				}
				else
				{
					SyncOrbitFromFly(viewport_panel);
				}
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Gizmo:");
			ImGui::SameLine();
			bool gizmo_move = viewport_panel.gizmo_mode == ViewportPanelState::GizmoMode::Translate;
			bool gizmo_rotate = viewport_panel.gizmo_mode == ViewportPanelState::GizmoMode::Rotate;
			bool gizmo_scale = viewport_panel.gizmo_mode == ViewportPanelState::GizmoMode::Scale;
			if (ImGui::RadioButton("Move", gizmo_move))
			{
				viewport_panel.gizmo_mode = ViewportPanelState::GizmoMode::Translate;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Rotate", gizmo_rotate))
			{
				viewport_panel.gizmo_mode = ViewportPanelState::GizmoMode::Rotate;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Scale", gizmo_scale))
			{
				viewport_panel.gizmo_mode = ViewportPanelState::GizmoMode::Scale;
			}
			ImGui::SameLine();
			if (ImGui::Button("Gizmo Settings"))
			{
				ImGui::OpenPopup("GizmoSettings");
			}
			if (ImGui::BeginPopup("GizmoSettings"))
			{
				ImGui::TextUnformatted("Space");
				ImGui::Separator();
				ImGui::TextUnformatted("Gizmo");
				ImGui::SameLine();
				const bool gizmo_world = viewport_panel.gizmo_space == ViewportPanelState::GizmoSpace::World;
				const bool gizmo_local = viewport_panel.gizmo_space == ViewportPanelState::GizmoSpace::Local;
				if (ImGui::RadioButton("World##GizmoSpace", gizmo_world))
				{
					viewport_panel.gizmo_space = ViewportPanelState::GizmoSpace::World;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Local##GizmoSpace", gizmo_local))
				{
					viewport_panel.gizmo_space = ViewportPanelState::GizmoSpace::Local;
				}
				ImGui::TextUnformatted("Rotation");
				ImGui::SameLine();
				const bool rot_world = viewport_panel.rotation_space == ViewportPanelState::GizmoSpace::World;
				const bool rot_local = viewport_panel.rotation_space == ViewportPanelState::GizmoSpace::Local;
				if (ImGui::RadioButton("World##RotSpace", rot_world))
				{
					viewport_panel.rotation_space = ViewportPanelState::GizmoSpace::World;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Local##RotSpace", rot_local))
				{
					viewport_panel.rotation_space = ViewportPanelState::GizmoSpace::Local;
				}

				ImGui::Separator();
				ImGui::TextUnformatted("Snapping");
				ImGui::Separator();
				ImGui::Checkbox("Snap Move", &viewport_panel.snap_translate);
				ImGui::SameLine();
				ImGui::DragFloat("Move Step", &viewport_panel.snap_translate_step, 0.1f, 0.01f, 1000.0f);
				ImGui::TextUnformatted("Move Axes");
				ImGui::SameLine();
				ImGui::Checkbox("X##MoveAxis", &viewport_panel.snap_translate_axis[0]);
				ImGui::SameLine();
				ImGui::Checkbox("Y##MoveAxis", &viewport_panel.snap_translate_axis[1]);
				ImGui::SameLine();
				ImGui::Checkbox("Z##MoveAxis", &viewport_panel.snap_translate_axis[2]);

				ImGui::Separator();
				ImGui::Checkbox("Snap Rotate", &viewport_panel.snap_rotate);
				ImGui::SameLine();
				ImGui::DragFloat("Rotate Step", &viewport_panel.snap_rotate_step, 1.0f, 1.0f, 180.0f);
				ImGui::TextUnformatted("Rotate Axes");
				ImGui::SameLine();
				ImGui::Checkbox("X##RotAxis", &viewport_panel.snap_rotate_axis[0]);
				ImGui::SameLine();
				ImGui::Checkbox("Y##RotAxis", &viewport_panel.snap_rotate_axis[1]);
				ImGui::SameLine();
				ImGui::Checkbox("Z##RotAxis", &viewport_panel.snap_rotate_axis[2]);

				ImGui::Separator();
				ImGui::Checkbox("Snap Scale", &viewport_panel.snap_scale);
				ImGui::SameLine();
				ImGui::DragFloat("Scale Step", &viewport_panel.snap_scale_step, 0.01f, 0.01f, 10.0f);
				ImGui::TextUnformatted("Scale Axes");
				ImGui::SameLine();
				ImGui::Checkbox("X##ScaleAxis", &viewport_panel.snap_scale_axis[0]);
				ImGui::SameLine();
				ImGui::Checkbox("Y##ScaleAxis", &viewport_panel.snap_scale_axis[1]);
				ImGui::SameLine();
				ImGui::Checkbox("Z##ScaleAxis", &viewport_panel.snap_scale_axis[2]);

				ImGui::Separator();
				ImGui::TextUnformatted("Sizing");
				ImGui::Separator();
				ImGui::SliderFloat("Overall", &viewport_panel.gizmo_size, 0.25f, 3.0f, "%.2f");
				ImGui::DragFloat3(
					"Axis Scale",
					viewport_panel.gizmo_axis_scale,
					0.05f,
					0.1f,
					5.0f,
					"%.2f");
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset View"))
			{
				viewport_panel.initialized = false;
			}

			ImVec2 avail = ImGui::GetContentRegionAvail();
			const uint32_t target_width = static_cast<uint32_t>(avail.x > 0.0f ? avail.x : 0.0f);
			const uint32_t target_height = static_cast<uint32_t>(avail.y > 0.0f ? avail.y : 0.0f);
			const ImVec2 viewport_pos = ImGui::GetCursorScreenPos();

			diligent.viewport_visible = (target_width > 0 && target_height > 0);
			bool drew_image = false;
			if (diligent.viewport_visible && CreateViewportTargets(diligent, target_width, target_height))
			{
				ImTextureID view_id = reinterpret_cast<ImTextureID>(diligent.viewport.color_srv.RawPtr());
				ImGui::Image(view_id, avail);
				drew_image = true;
			}
			else
			{
				ImGui::TextUnformatted("Viewport inactive.");
			}

			const bool hovered = drew_image && ImGui::IsItemHovered();
			UpdateViewportControls(viewport_panel, viewport_pos, avail, hovered);
			bool gizmo_consumed_click = false;
			if (drew_image && hovered && active_target == SelectionTarget::Scene)
			{
				const int selected_id = scene_panel.selected_id;
				const size_t count = std::min(scene_nodes.size(), scene_props.size());
				if (selected_id >= 0 && static_cast<size_t>(selected_id) < count)
				{
					const TreeNode& node = scene_nodes[selected_id];
					NodeProperties& props = scene_props[selected_id];
					if (!node.deleted && !node.is_folder && !props.locked)
					{
						const float aspect = avail.y > 0.0f ? (avail.x / avail.y) : 1.0f;
						const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);

						Diligent::float3 cam_pos;
						Diligent::float3 cam_forward;
						Diligent::float3 cam_right;
						Diligent::float3 cam_up;
						ComputeCameraBasis(viewport_panel, cam_pos, cam_forward, cam_right, cam_up);

						GizmoDrawState gizmo_state{};
						if (BuildGizmoDrawState(viewport_panel, props, cam_pos, view_proj, avail, gizmo_state))
						{
							DrawGizmo(viewport_panel, gizmo_state, viewport_pos, ImGui::GetWindowDrawList());
							const ImVec2 mouse_local(ImGui::GetIO().MousePos.x - viewport_pos.x,
								ImGui::GetIO().MousePos.y - viewport_pos.y);
							const bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
							const bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
							if (UpdateGizmoInteraction(viewport_panel, gizmo_state, props, mouse_local, mouse_down, mouse_clicked))
							{
								gizmo_consumed_click = true;
							}
						}
					}
				}
			}
			if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (!scene_nodes.empty() && !scene_props.empty())
				{
					if (gizmo_consumed_click)
					{
						// Gizmo took this click.
					}
					else
					{
						const float aspect = avail.y > 0.0f ? (avail.x / avail.y) : 1.0f;
						const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);
						const ImVec2 mouse = ImGui::GetIO().MousePos;
						const ImVec2 local(mouse.x - viewport_pos.x, mouse.y - viewport_pos.y);

						const float pick_radius = 10.0f;
						const float max_dist2 = pick_radius * pick_radius;
						float best_dist2 = max_dist2;
						int best_id = -1;

						const size_t count = std::min(scene_nodes.size(), scene_props.size());
						for (size_t i = 0; i < count; ++i)
						{
							const TreeNode& node = scene_nodes[i];
							if (node.deleted || node.is_folder)
							{
								continue;
							}
							ImVec2 screen_pos;
							if (!ProjectWorldToScreen(view_proj, scene_props[i].position, avail, screen_pos))
							{
								continue;
							}
							const float dx = screen_pos.x - local.x;
							const float dy = screen_pos.y - local.y;
							const float dist2 = dx * dx + dy * dy;
							if (dist2 < best_dist2)
							{
								best_dist2 = dist2;
								best_id = static_cast<int>(i);
							}
						}

						if (best_id >= 0)
						{
							scene_panel.selected_id = best_id;
							active_target = SelectionTarget::Scene;
						}
					}
				}
			}
			if (drew_image)
			{
				DrawViewportOverlay(viewport_panel, ImGui::GetWindowDrawList(), viewport_pos, avail);
			}
		}
		ImGui::End();

		const NodeProperties* world_props = FindWorldProperties(scene_props);
		RenderViewport(diligent, viewport_panel, world_props);

		Diligent::ITextureView* back_rtv = diligent.engine.swapchain->GetCurrentBackBufferRTV();
		Diligent::ITextureView* back_dsv = diligent.engine.swapchain->GetDepthBufferDSV();
		diligent.engine.context->SetRenderTargets(
			1,
			&back_rtv,
			back_dsv,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		const float clear_color[] = {0.10f, 0.11f, 0.12f, 1.0f};
		diligent.engine.context->ClearRenderTarget(back_rtv, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		if (back_dsv)
		{
			diligent.engine.context->ClearDepthStencil(
				back_dsv,
				Diligent::CLEAR_DEPTH_FLAG,
				1.0f,
				0,
				Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		}

		diligent.imgui->Render(diligent.engine.context);
		diligent.engine.swapchain->Present();
	}

	diligent.imgui.reset();
	ShutdownEngineRenderer(diligent.engine);
	if (diligent.metal_view)
	{
		DestroyMetalView(diligent.metal_view);
		diligent.metal_view = nullptr;
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	DEdit2_TermConsoleCommands();

	return 0;
}
