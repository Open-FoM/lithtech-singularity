#include "dedit2_concommand.h"
#include "de_objects.h"
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
#include "viewport_picking.h"
#include "ui_shared.h"
#include "undo_stack.h"
#include "engine_render.h"

#include "imgui.h"

#include "ImGuiImplSDL3.hpp"
#include "diligent_render.h"
#include "diligent_render_debug.h"
#include "rendererconsolevars.h"
#include "debuggeometry.h"
#include "ltrotation.h"
#include "ltvector.h"
#include "renderstruct.h"

#include "DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "DiligentCore/Common/interface/RefCntAutoPtr.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
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

bool ParseFloat3(const std::string& text, float out[3])
{
	std::istringstream stream(text);
	return static_cast<bool>(stream >> out[0] >> out[1] >> out[2]);
}

bool ExtractWorldBoundsFromProps(const NodeProperties& props, float out_min[3], float out_max[3])
{
	bool has_min = false;
	bool has_max = false;
	for (const auto& entry : props.raw_properties)
	{
		if (entry.first == "world_bounds_min")
		{
			has_min = ParseFloat3(entry.second, out_min);
		}
		else if (entry.first == "world_bounds_max")
		{
			has_max = ParseFloat3(entry.second, out_max);
		}
		if (has_min && has_max)
		{
			return true;
		}
	}
	return false;
}

bool TryGetWorldBounds(const std::vector<NodeProperties>& props, float out_min[3], float out_max[3])
{
	if (props.empty())
	{
		return false;
	}
	if (ExtractWorldBoundsFromProps(props.front(), out_min, out_max))
	{
		return true;
	}
	for (size_t i = 1; i < props.size(); ++i)
	{
		if (ExtractWorldBoundsFromProps(props[i], out_min, out_max))
		{
			return true;
		}
	}
	return false;
}

std::string ResolveProjectRoot(const std::string& selected_path)
{
	if (selected_path.empty())
	{
		return {};
	}

	std::error_code ec;
	fs::path chosen = fs::path(selected_path);
	if (!fs::is_directory(chosen, ec))
	{
		return selected_path;
	}

	fs::path probe = chosen;
	for (int i = 0; i < 8; ++i)
	{
		if (fs::exists(probe / "Rez", ec) || fs::exists(probe / "Worlds", ec))
		{
			return probe.string();
		}
		const fs::path parent = probe.parent_path();
		if (parent.empty() || parent == probe)
		{
			break;
		}
		probe = parent;
	}

	return chosen.string();
}

int DefaultProjectSelection(const std::vector<TreeNode>& nodes)
{
	if (nodes.empty())
	{
		return -1;
	}
	if (nodes[0].children.empty())
	{
		return -1;
	}
	return nodes[0].children.front();
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
}

struct WorldRenderSettingsCache
{
	bool valid = false;
	bool fog_enabled = false;
	float fog_near = 0.0f;
	float fog_far = 0.0f;
	float far_z = 0.0f;
	float fog_color[3] = {0.0f, 0.0f, 0.0f};
};

bool WorldSettingsDifferent(const NodeProperties& props, const WorldRenderSettingsCache& cache)
{
	return !cache.valid ||
		cache.fog_enabled != props.fog_enabled ||
		cache.fog_near != props.fog_near ||
		cache.fog_far != props.fog_far ||
		cache.far_z != props.far_z ||
		cache.fog_color[0] != props.color[0] ||
		cache.fog_color[1] != props.color[1] ||
		cache.fog_color[2] != props.color[2];
}

void UpdateWorldSettingsCache(const NodeProperties& props, WorldRenderSettingsCache& cache)
{
	cache.valid = true;
	cache.fog_enabled = props.fog_enabled;
	cache.fog_near = props.fog_near;
	cache.fog_far = props.fog_far;
	cache.far_z = props.far_z;
	cache.fog_color[0] = props.color[0];
	cache.fog_color[1] = props.color[1];
	cache.fog_color[2] = props.color[2];
}

std::string LowerCopy(std::string value)
{
	for (char& ch : value)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return value;
}

std::string TrimLine(const std::string& value);

struct DiligentContext;
void BuildSkyWorldModels(
	DiligentContext& ctx,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props);

std::string TrimQuotes(const std::string& value)
{
	std::string trimmed = TrimLine(value);
	if (trimmed.size() >= 2)
	{
		const char front = trimmed.front();
		const char back = trimmed.back();
		if ((front == '"' && back == '"') || (front == '\'' && back == '\''))
		{
			return trimmed.substr(1, trimmed.size() - 2);
		}
	}
	return trimmed;
}

bool TryGetRawPropertyString(const NodeProperties& props, const char* key, std::string& out)
{
	if (!key || key[0] == '\0')
	{
		return false;
	}
	const std::string key_lower = LowerCopy(key);
	for (const auto& entry : props.raw_properties)
	{
		if (LowerCopy(entry.first) == key_lower)
		{
			out = TrimQuotes(entry.second);
			return !out.empty();
		}
	}
	return false;
}

bool TryGetRawPropertyStringAny(
	const NodeProperties& props,
	const std::initializer_list<const char*>& keys,
	std::string& out)
{
	for (const char* key : keys)
	{
		if (TryGetRawPropertyString(props, key, out))
		{
			return true;
		}
	}
	return false;
}

bool IsSkyNode(const NodeProperties& props)
{
	const std::string type = LowerCopy(props.type);
	const std::string cls = LowerCopy(props.class_name);
	if (type == "sky" ||
		cls == "sky" ||
		type == "skyobject" ||
		cls == "skyobject" ||
		type == "demoskyworldmodel" ||
		cls == "demoskyworldmodel" ||
		type == "skypointer" ||
		cls == "skypointer")
	{
		return true;
	}

	if (type.find("sky") != std::string::npos || cls.find("sky") != std::string::npos)
	{
		return true;
	}

	std::string unused;
	return TryGetRawPropertyStringAny(props, {"objectname", "skyobject", "skyobjectname"}, unused);
}

bool IsSkyPointerNode(const NodeProperties& props)
{
	const std::string type = LowerCopy(props.type);
	const std::string cls = LowerCopy(props.class_name);
	return type == "skypointer" || cls == "skypointer";
}

bool SkyTargetMatchesWorldName(const std::string& target, const std::string& world_name)
{
	const std::string target_lower = LowerCopy(target);
	const std::string name_lower = LowerCopy(world_name);
	if (target_lower.empty() || name_lower.empty())
	{
		return false;
	}
	if (name_lower == target_lower)
	{
		return true;
	}
	if (name_lower.rfind(target_lower, 0) != 0)
	{
		return false;
	}
	const char next = name_lower.size() > target_lower.size() ? name_lower[target_lower.size()] : '\0';
	return next == '\0' || next == '_' || next == '-' || std::isspace(static_cast<unsigned char>(next));
}

std::string GuessSkyWorldModelName(
	const TreeNode& node,
	const NodeProperties& props)
{
	std::string value;
	if (TryGetRawPropertyStringAny(
			props,
			{"objectname", "worldmodel", "worldmodelname", "model", "modelname", "filename"},
			value))
	{
		return value;
	}

	if (!props.resource.empty())
	{
		return TrimQuotes(props.resource);
	}

	if (!node.name.empty() && node.name != "Node" && node.name != "World")
	{
		return TrimQuotes(node.name);
	}

	return {};
}

std::string GuessWorldModelNameForObject(
	const TreeNode& node,
	const NodeProperties& props)
{
	std::string value;
	if (TryGetRawPropertyStringAny(
			props,
			{"worldmodel", "worldmodelname", "model", "modelname", "filename"},
			value))
	{
		return value;
	}

	if (!props.resource.empty())
	{
		return TrimQuotes(props.resource);
	}

	if (!node.name.empty())
	{
		return TrimQuotes(node.name);
	}

	return {};
}

struct RenderCategoryState
{
	bool has = false;
	bool any_visible = false;
};


enum class RenderCategory
{
	World,
	WorldModel,
	Model,
	Sprite,
	PolyGrid,
	Particles,
	Volume,
	LineSystem,
	Canvas,
	Sky,
	Count
};

struct NodeCategoryFlags
{
	bool world = false;
	bool world_model = false;
	bool model = false;
	bool sprite = false;
	bool polygrid = false;
	bool particles = false;
	bool volume = false;
	bool line_system = false;
	bool canvas = false;
	bool sky = false;
};

NodeCategoryFlags ClassifyNodeCategories(const NodeProperties& props)
{
	NodeCategoryFlags flags{};
	const std::string type = LowerCopy(props.type);
	const std::string cls = LowerCopy(props.class_name);
	flags.world = (type == "world") || (cls == "world");
	flags.world_model = (type == "worldmodel") || (type == "world_model") ||
		(cls == "worldmodel") || (cls == "world_model");
	flags.model = !flags.world_model && (type == "model" || cls == "model");
	flags.sprite = (type == "sprite" || cls == "sprite");
	flags.polygrid = (type == "polygrid" || cls == "polygrid" || type == "terrain" || cls == "terrain");
	flags.particles = (type == "particlesystem" || cls == "particlesystem" ||
		type == "particles" || cls == "particles" ||
		type == "particle" || cls == "particle");
	flags.volume = (type == "volumeeffect" || cls == "volumeeffect" ||
		type == "volume" || cls == "volume");
	flags.line_system = (type == "linesystem" || cls == "linesystem" ||
		type == "line" || cls == "line");
	flags.canvas = (type == "canvas" || cls == "canvas");
	flags.sky = (type == "sky" || cls == "sky" ||
		type == "skyobject" || cls == "skyobject");
	return flags;
}

bool NodePickableByRender(const ViewportPanelState& viewport_state, const NodeProperties& props)
{
	const NodeCategoryFlags flags = ClassifyNodeCategories(props);
	if (flags.sky)
	{
		return false;
	}
	if (flags.world && !viewport_state.render_draw_world)
	{
		return false;
	}
	if (flags.world_model && !viewport_state.render_draw_world_models)
	{
		return false;
	}
	if (flags.model && !viewport_state.render_draw_models)
	{
		return false;
	}
	if (flags.sprite && !viewport_state.render_draw_sprites)
	{
		return false;
	}
	if (flags.polygrid && !viewport_state.render_draw_polygrids)
	{
		return false;
	}
	if (flags.particles && !viewport_state.render_draw_particles)
	{
		return false;
	}
	if (flags.volume && !viewport_state.render_draw_volume_effects)
	{
		return false;
	}
	if (flags.line_system && !viewport_state.render_draw_line_systems)
	{
		return false;
	}
	if (flags.canvas && !viewport_state.render_draw_canvases)
	{
		return false;
	}
	if (flags.sky && !viewport_state.render_draw_sky)
	{
		return false;
	}
	return true;
}

bool IsLightNode(const NodeProperties& props)
{
	const std::string type = LowerCopy(props.type);
	const std::string cls = LowerCopy(props.class_name);
	auto is_light = [](const std::string& value) -> bool
	{
		return value == "light" ||
			value == "dirlight" ||
			value == "directionallight" ||
			value == "spotlight" ||
			value == "pointlight";
	};
	return is_light(type) || is_light(cls);
}

bool IsDirectionalLightNode(const NodeProperties& props)
{
	const std::string type = LowerCopy(props.type);
	const std::string cls = LowerCopy(props.class_name);
	auto is_dir = [](const std::string& value) -> bool
	{
		return value == "dirlight" || value == "directionallight";
	};
	return is_dir(type) || is_dir(cls);
}

bool IsLightIconOccluded(
	const ViewportPanelState& viewport_state,
	const ScenePanelState& scene_state,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	int light_id,
	const Diligent::float3& cam_pos,
	const float light_pos[3])
{
	const Diligent::float3 light_world(light_pos[0], light_pos[1], light_pos[2]);
	const Diligent::float3 to_light = Diligent::float3(
		light_world.x - cam_pos.x,
		light_world.y - cam_pos.y,
		light_world.z - cam_pos.z);
	const float dist_sq = to_light.x * to_light.x + to_light.y * to_light.y + to_light.z * to_light.z;
	if (dist_sq <= 1.0e-4f)
	{
		return false;
	}
	const float dist = std::sqrt(dist_sq);
	const float inv_dist = 1.0f / dist;
	const Diligent::float3 dir(to_light.x * inv_dist, to_light.y * inv_dist, to_light.z * inv_dist);
	const PickRay ray{cam_pos, dir};
	const float occlusion_epsilon = 0.1f;
	const size_t count = std::min(nodes.size(), props.size());

	for (size_t i = 0; i < count; ++i)
	{
		if (static_cast<int>(i) == light_id)
		{
			continue;
		}
		const TreeNode& node = nodes[i];
		const NodeProperties& node_props = props[i];
		if (node.deleted || node.is_folder || !node_props.visible)
		{
			continue;
		}
		if (IsLightNode(node_props))
		{
			continue;
		}
		if (!SceneNodePassesFilters(scene_state, static_cast<int>(i), nodes, props))
		{
			continue;
		}
		if (!NodePickableByRender(viewport_state, node_props))
		{
			continue;
		}

		float hit_t = 0.0f;
		if (RaycastNode(node_props, ray, hit_t) && hit_t > 0.0f && hit_t < dist - occlusion_epsilon)
		{
			return true;
		}
	}

	return false;
}

void BuildViewportDynamicLights(
	const ViewportPanelState& viewport_state,
	const ScenePanelState& scene_state,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	std::vector<DynamicLight>& out_lights)
{
	out_lights.clear();
	const size_t count = std::min(nodes.size(), props.size());
	out_lights.reserve(count);

	const float intensity_scale = std::max(0.0f, g_CV_DEdit2LightIntensityScale.m_Val);
	const float radius_scale = std::max(0.0f, g_CV_DEdit2LightRadiusScale.m_Val);
	constexpr float kIntensityReference = 5.0f;

	auto to_u8 = [](float value) -> uint8
	{
		const float clamped = std::clamp(value, 0.0f, 1.0f);
		return static_cast<uint8>(clamped * 255.0f + 0.5f);
	};

	for (size_t i = 0; i < count; ++i)
	{
		const TreeNode& node = nodes[i];
		const NodeProperties& node_props = props[i];
		if (node.deleted || node.is_folder || !node_props.visible)
		{
			continue;
		}
		if (!IsLightNode(node_props))
		{
			continue;
		}
		if (IsDirectionalLightNode(node_props))
		{
			continue;
		}
		if (!SceneNodePassesFilters(scene_state, static_cast<int>(i), nodes, props))
		{
			continue;
		}
		if (!NodePickableByRender(viewport_state, node_props))
		{
			continue;
		}

		const float radius = std::max(0.0f, node_props.range * radius_scale);
		const float intensity = std::max(0.0f, node_props.intensity * intensity_scale);
		const float intensity_multiplier = (kIntensityReference > 0.0f) ? (intensity / kIntensityReference) : 0.0f;
		if (radius <= 0.0f || intensity_multiplier <= 0.0f)
		{
			continue;
		}

		float pos[3] = {node_props.position[0], node_props.position[1], node_props.position[2]};
		TryGetNodePickPosition(node_props, pos);

		DynamicLight light;
		light.SetPos(LTVector(pos[0], pos[1], pos[2]));
		light.m_LightRadius = radius;
		light.SetDims(LTVector(radius, radius, radius));
		light.m_Flags = FLAG_VISIBLE;
		light.m_Flags2 |= FLAG2_FORCEDYNAMICLIGHTWORLD;
		light.m_ColorR = to_u8(node_props.color[0]);
		light.m_ColorG = to_u8(node_props.color[1]);
		light.m_ColorB = to_u8(node_props.color[2]);
		light.m_ColorA = 255;
		light.m_Intensity = intensity_multiplier;

		out_lights.push_back(light);
	}
}

void ApplyViewportDirectionalLight(
	EngineRenderContext& ctx,
	const ViewportPanelState& viewport_state,
	const ScenePanelState& scene_state,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props)
{
	if (!ctx.render_struct)
	{
		return;
	}

	if (!viewport_state.render_dynamic_lights_world)
	{
		ctx.render_struct->m_GlobalLightColor.Init(0.0f, 0.0f, 0.0f);
		ctx.render_struct->m_GlobalLightConvertToAmbient = 0.0f;
		return;
	}

	const size_t count = std::min(nodes.size(), props.size());
	const float intensity_scale = std::max(0.0f, g_CV_DEdit2LightIntensityScale.m_Val);
	constexpr float kIntensityReference = 5.0f;

	bool found = false;
	float best_luminance = 0.0f;
	LTVector best_dir(0.0f, -1.0f, 0.0f);
	LTVector best_color(0.0f, 0.0f, 0.0f);

	for (size_t i = 0; i < count; ++i)
	{
		const TreeNode& node = nodes[i];
		const NodeProperties& node_props = props[i];
		if (node.deleted || node.is_folder || !node_props.visible)
		{
			continue;
		}
		if (!IsDirectionalLightNode(node_props))
		{
			continue;
		}
		if (!SceneNodePassesFilters(scene_state, static_cast<int>(i), nodes, props))
		{
			continue;
		}
		if (!NodePickableByRender(viewport_state, node_props))
		{
			continue;
		}

		const float intensity = std::max(0.0f, node_props.intensity * intensity_scale);
		const float intensity_multiplier = (kIntensityReference > 0.0f) ? (intensity / kIntensityReference) : 0.0f;
		if (intensity_multiplier <= 0.0f)
		{
			continue;
		}

		LTRotation rotation(node_props.rotation[0], node_props.rotation[1], node_props.rotation[2]);
		LTVector forward = rotation.Forward();
		if (forward.MagSqr() <= 1.0e-6f)
		{
			continue;
		}
		forward.Normalize();

		LTVector color(
			node_props.color[0] * intensity_multiplier,
			node_props.color[1] * intensity_multiplier,
			node_props.color[2] * intensity_multiplier);
		const float luminance = color.x + color.y + color.z;
		if (!found || luminance > best_luminance)
		{
			found = true;
			best_luminance = luminance;
			best_dir = forward;
			best_color = color;
		}
	}

	ctx.render_struct->m_GlobalLightDir = best_dir;
	ctx.render_struct->m_GlobalLightColor = best_color;
	ctx.render_struct->m_GlobalLightConvertToAmbient = 0.0f;
}

struct ViewportOverlayItem
{
	const NodeProperties* props = nullptr;
	LTRGBColor color{};
};

struct ViewportOverlayState
{
	ViewportOverlayItem items[2];
	int count = 0;
};

struct LightColor
{
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
};

LTRGBColor MakeOverlayColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	LTRGBColor color{};
	color.rgb.r = r;
	color.rgb.g = g;
	color.rgb.b = b;
	color.rgb.a = a;
	return color;
}

void AddDebugLine(const float a[3], const float b[3], const LTRGBColor& color)
{
	getDebugGeometry().addLine(
		LTVector(a[0], a[1], a[2]),
		LTVector(b[0], b[1], b[2]),
		color,
		false);
}

void AddDebugAABB(const float bounds_min[3], const float bounds_max[3], const LTRGBColor& color)
{
	const float x0 = bounds_min[0];
	const float y0 = bounds_min[1];
	const float z0 = bounds_min[2];
	const float x1 = bounds_max[0];
	const float y1 = bounds_max[1];
	const float z1 = bounds_max[2];

	const float corners[8][3] = {
		{x0, y0, z0},
		{x1, y0, z0},
		{x1, y1, z0},
		{x0, y1, z0},
		{x0, y0, z1},
		{x1, y0, z1},
		{x1, y1, z1},
		{x0, y1, z1}
	};

	static const int edges[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	for (const auto& edge : edges)
	{
		AddDebugLine(corners[edge[0]], corners[edge[1]], color);
	}
}

void AddDebugBrushWire(const NodeProperties& props, const LTRGBColor& color)
{
	const size_t vertex_count = props.brush_vertices.size() / 3;
	if (vertex_count == 0 || props.brush_indices.empty())
	{
		return;
	}

	for (size_t i = 0; i + 2 < props.brush_indices.size(); i += 3)
	{
		const uint32_t i0 = props.brush_indices[i];
		const uint32_t i1 = props.brush_indices[i + 1];
		const uint32_t i2 = props.brush_indices[i + 2];
		if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
		{
			continue;
		}
		const float v0[3] = {
			props.brush_vertices[static_cast<size_t>(i0) * 3 + 0],
			props.brush_vertices[static_cast<size_t>(i0) * 3 + 1],
			props.brush_vertices[static_cast<size_t>(i0) * 3 + 2]};
		const float v1[3] = {
			props.brush_vertices[static_cast<size_t>(i1) * 3 + 0],
			props.brush_vertices[static_cast<size_t>(i1) * 3 + 1],
			props.brush_vertices[static_cast<size_t>(i1) * 3 + 2]};
		const float v2[3] = {
			props.brush_vertices[static_cast<size_t>(i2) * 3 + 0],
			props.brush_vertices[static_cast<size_t>(i2) * 3 + 1],
			props.brush_vertices[static_cast<size_t>(i2) * 3 + 2]};

		AddDebugLine(v0, v1, color);
		AddDebugLine(v1, v2, color);
		AddDebugLine(v2, v0, color);
	}
}

void AddDebugOverlayForNode(const NodeProperties& props, const LTRGBColor& color)
{
	if (!props.brush_vertices.empty() && !props.brush_indices.empty())
	{
		AddDebugBrushWire(props, color);
		return;
	}

	float bounds_min[3];
	float bounds_max[3];
	if (TryGetNodeBounds(props, bounds_min, bounds_max))
	{
		AddDebugAABB(bounds_min, bounds_max, color);
	}
}

void RenderViewportOverlays(const ViewportOverlayState& state)
{
	CDebugGeometry& debug = getDebugGeometry();
	debug.clear();
	if (state.count <= 0)
	{
		return;
	}
	debug.setVisible(true);
	debug.setWidth(1.0f);

	for (int i = 0; i < state.count; ++i)
	{
		const ViewportOverlayItem& item = state.items[i];
		if (!item.props)
		{
			continue;
		}
		AddDebugOverlayForNode(*item.props, item.color);
	}

	drawAllDebugGeometry();
}

void DrawAABBOverlay(
	const float bounds_min[3],
	const float bounds_max[3],
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_pos,
	const ImVec2& viewport_size,
	ImDrawList* draw_list,
	ImU32 color,
	float thickness)
{
	const float x0 = bounds_min[0];
	const float y0 = bounds_min[1];
	const float z0 = bounds_min[2];
	const float x1 = bounds_max[0];
	const float y1 = bounds_max[1];
	const float z1 = bounds_max[2];

	const float corners[8][3] = {
		{x0, y0, z0},
		{x1, y0, z0},
		{x1, y1, z0},
		{x0, y1, z0},
		{x0, y0, z1},
		{x1, y0, z1},
		{x1, y1, z1},
		{x0, y1, z1}
	};

	static const int edges[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	ImVec2 screen[8];
	bool visible[8] = {};
	for (int i = 0; i < 8; ++i)
	{
		visible[i] = ProjectWorldToScreen(view_proj, corners[i], viewport_size, screen[i]);
		if (visible[i])
		{
			screen[i].x += viewport_pos.x;
			screen[i].y += viewport_pos.y;
		}
	}

	for (const auto& edge : edges)
	{
		const int a = edge[0];
		const int b = edge[1];
		if (visible[a] && visible[b])
		{
			draw_list->AddLine(screen[a], screen[b], color, thickness);
		}
	}
}

void DrawBrushOverlay(
	const NodeProperties& props,
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_pos,
	const ImVec2& viewport_size,
	ImDrawList* draw_list,
	ImU32 color,
	float thickness)
{
	const size_t vertex_count = props.brush_vertices.size() / 3;
	if (vertex_count == 0 || props.brush_indices.empty())
	{
		return;
	}

	for (size_t i = 0; i + 2 < props.brush_indices.size(); i += 3)
	{
		const uint32_t i0 = props.brush_indices[i];
		const uint32_t i1 = props.brush_indices[i + 1];
		const uint32_t i2 = props.brush_indices[i + 2];
		if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
		{
			continue;
		}
		const float v0[3] = {
			props.brush_vertices[static_cast<size_t>(i0) * 3 + 0],
			props.brush_vertices[static_cast<size_t>(i0) * 3 + 1],
			props.brush_vertices[static_cast<size_t>(i0) * 3 + 2]};
		const float v1[3] = {
			props.brush_vertices[static_cast<size_t>(i1) * 3 + 0],
			props.brush_vertices[static_cast<size_t>(i1) * 3 + 1],
			props.brush_vertices[static_cast<size_t>(i1) * 3 + 2]};
		const float v2[3] = {
			props.brush_vertices[static_cast<size_t>(i2) * 3 + 0],
			props.brush_vertices[static_cast<size_t>(i2) * 3 + 1],
			props.brush_vertices[static_cast<size_t>(i2) * 3 + 2]};

		ImVec2 s0;
		ImVec2 s1;
		ImVec2 s2;
		const bool p0 = ProjectWorldToScreen(view_proj, v0, viewport_size, s0);
		const bool p1 = ProjectWorldToScreen(view_proj, v1, viewport_size, s1);
		const bool p2 = ProjectWorldToScreen(view_proj, v2, viewport_size, s2);
		if (!(p0 || p1 || p2))
		{
			continue;
		}
		if (p0 && p1)
		{
			draw_list->AddLine(
				ImVec2(viewport_pos.x + s0.x, viewport_pos.y + s0.y),
				ImVec2(viewport_pos.x + s1.x, viewport_pos.y + s1.y),
				color,
				thickness);
		}
		if (p1 && p2)
		{
			draw_list->AddLine(
				ImVec2(viewport_pos.x + s1.x, viewport_pos.y + s1.y),
				ImVec2(viewport_pos.x + s2.x, viewport_pos.y + s2.y),
				color,
				thickness);
		}
		if (p2 && p0)
		{
			draw_list->AddLine(
				ImVec2(viewport_pos.x + s2.x, viewport_pos.y + s2.y),
				ImVec2(viewport_pos.x + s0.x, viewport_pos.y + s0.y),
				color,
				thickness);
		}
	}
}

void DrawNodeOverlay(
	const NodeProperties& props,
	const Diligent::float4x4& view_proj,
	const ImVec2& viewport_pos,
	const ImVec2& viewport_size,
	ImDrawList* draw_list,
	ImU32 color,
	float thickness)
{
	if (!props.brush_vertices.empty() && !props.brush_indices.empty())
	{
		DrawBrushOverlay(props, view_proj, viewport_pos, viewport_size, draw_list, color, thickness);
		return;
	}

	float bounds_min[3];
	float bounds_max[3];
	if (TryGetNodeBounds(props, bounds_min, bounds_max))
	{
		DrawAABBOverlay(bounds_min, bounds_max, view_proj, viewport_pos, viewport_size, draw_list, color, thickness);
		return;
	}
}

void ApplySceneVisibilityToRenderer(
	const ScenePanelState& scene_state,
	const ViewportPanelState& viewport_state,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props)
{
	std::array<RenderCategoryState, static_cast<size_t>(RenderCategory::Count)> categories{};
	const size_t count = std::min(nodes.size(), props.size());

	auto node_visible_for_render = [&](int node_id, const NodeProperties& node_props) -> bool
	{
		if (scene_state.isolate_selected && scene_state.selected_id >= 0 && node_id != scene_state.selected_id)
		{
			return false;
		}
		if (!node_props.type.empty())
		{
			auto it = scene_state.type_visibility.find(node_props.type);
			if (it != scene_state.type_visibility.end() && !it->second)
			{
				return false;
			}
		}
		if (!node_props.class_name.empty())
		{
			auto it = scene_state.class_visibility.find(node_props.class_name);
			if (it != scene_state.class_visibility.end() && !it->second)
			{
				return false;
			}
		}
		return true;
	};

	for (size_t i = 0; i < count; ++i)
	{
		const TreeNode& node = nodes[i];
		if (node.deleted)
		{
			continue;
		}

		const NodeProperties& node_props = props[i];
		const std::string type = LowerCopy(node_props.type);
		const std::string cls = LowerCopy(node_props.class_name);
		const bool is_world = (type == "world") || (cls == "world");
		if (node.is_folder && !is_world)
		{
			continue;
		}

		const bool node_visible = node_visible_for_render(static_cast<int>(i), node_props);

		auto mark = [&](RenderCategory cat)
		{
			RenderCategoryState& state = categories[static_cast<size_t>(cat)];
			state.has = true;
			state.any_visible = state.any_visible || node_visible;
		};

		const bool is_world_model = (type == "worldmodel") || (type == "world_model") ||
			(cls == "worldmodel") || (cls == "world_model");
		if (is_world)
		{
			mark(RenderCategory::World);
		}
		if (is_world_model)
		{
			mark(RenderCategory::WorldModel);
		}

		if (!is_world_model && (type == "model" || cls == "model"))
		{
			mark(RenderCategory::Model);
		}
		if (type == "sprite" || cls == "sprite")
		{
			mark(RenderCategory::Sprite);
		}
		if (type == "polygrid" || cls == "polygrid" || type == "terrain" || cls == "terrain")
		{
			mark(RenderCategory::PolyGrid);
		}
		if (type == "particlesystem" || cls == "particlesystem" ||
			type == "particles" || cls == "particles" ||
			type == "particle" || cls == "particle")
		{
			mark(RenderCategory::Particles);
		}
		if (type == "volumeeffect" || cls == "volumeeffect" ||
			type == "volume" || cls == "volume")
		{
			mark(RenderCategory::Volume);
		}
		if (type == "linesystem" || cls == "linesystem" ||
			type == "line" || cls == "line")
		{
			mark(RenderCategory::LineSystem);
		}
		if (type == "canvas" || cls == "canvas")
		{
			mark(RenderCategory::Canvas);
		}
		if (type == "sky" || cls == "sky" ||
			type == "skyobject" || cls == "skyobject")
		{
			mark(RenderCategory::Sky);
		}
	}

	auto apply = [&](RenderCategory cat, auto& cvar)
	{
		const RenderCategoryState& state = categories[static_cast<size_t>(cat)];
		const bool enabled = [&]()
		{
			switch (cat)
			{
				case RenderCategory::World:
					return viewport_state.render_draw_world;
				case RenderCategory::WorldModel:
					return viewport_state.render_draw_world_models;
				case RenderCategory::Model:
					return viewport_state.render_draw_models;
				case RenderCategory::Sprite:
					return viewport_state.render_draw_sprites;
				case RenderCategory::PolyGrid:
					return viewport_state.render_draw_polygrids;
				case RenderCategory::Particles:
					return viewport_state.render_draw_particles;
				case RenderCategory::Volume:
					return viewport_state.render_draw_volume_effects;
				case RenderCategory::LineSystem:
					return viewport_state.render_draw_line_systems;
				case RenderCategory::Canvas:
					return viewport_state.render_draw_canvases;
				case RenderCategory::Sky:
					return viewport_state.render_draw_sky;
				default:
					return true;
			}
		}();
		const bool visible = !state.has || state.any_visible;
		cvar = (enabled && visible) ? 1 : 0;
	};

	apply(RenderCategory::World, g_CV_DrawWorld);
	apply(RenderCategory::WorldModel, g_CV_DrawWorldModels);
	apply(RenderCategory::Model, g_CV_DrawModels);
	apply(RenderCategory::Sprite, g_CV_DrawSprites);
	apply(RenderCategory::PolyGrid, g_CV_DrawPolyGrids);
	apply(RenderCategory::Particles, g_CV_DrawParticles);
	apply(RenderCategory::Volume, g_CV_DrawVolumeEffects);
	apply(RenderCategory::LineSystem, g_CV_DrawLineSystems);
	apply(RenderCategory::Canvas, g_CV_DrawCanvases);
	apply(RenderCategory::Sky, g_CV_DrawSky);

	g_CV_Wireframe = viewport_state.render_wireframe_world ? 1 : 0;
	g_CV_WireframeModels = viewport_state.render_wireframe_models ? 1 : 0;
	g_CV_WorldShadingMode = viewport_state.render_world_shading_mode;
	g_CV_DrawFlat = (viewport_state.render_world_shading_mode == 1) ? 1 : 0;
	g_CV_WireframeOverlay = viewport_state.render_wireframe_overlay ? 1 : 0;
	{
		const int new_fullbright = viewport_state.render_fullbright ? 1 : 0;
		if (new_fullbright != g_CV_Fullbright.m_Val)
		{
			g_CV_Fullbright = new_fullbright;
			diligent_InvalidateWorldGeometry();
		}
	}
	int lightmap_mode = viewport_state.render_lightmap_mode;
	if (!viewport_state.render_lightmaps_available && lightmap_mode != 0)
	{
		lightmap_mode = 0;
	}
	g_CV_LightMap = (lightmap_mode != 0) ? 1 : 0;
	g_CV_LightmapsOnly = (lightmap_mode == 2) ? 1 : 0;
	g_CV_LightmapIntensity = viewport_state.render_lightmap_intensity;
	g_CV_DrawSorted = viewport_state.render_draw_sorted ? 1 : 0;
	g_CV_DrawSolidModels = viewport_state.render_draw_solid_models ? 1 : 0;
	g_CV_DrawTranslucentModels = viewport_state.render_draw_translucent_models ? 1 : 0;
	g_CV_TextureModels = viewport_state.render_texture_models ? 1 : 0;
	g_CV_LightModels = viewport_state.render_light_models ? 1 : 0;
	g_CV_ModelApplyAmbient = viewport_state.render_model_apply_ambient ? 1 : 0;
	g_CV_ModelApplySun = viewport_state.render_model_apply_sun ? 1 : 0;
	g_CV_ModelSaturation = viewport_state.render_model_saturation;
	g_CV_Saturate = (std::fabs(viewport_state.render_model_saturation - 1.0f) > 0.001f) ? 1 : 0;
	g_CV_DynamicLight = viewport_state.render_dynamic_lights ? 1 : 0;
	g_CV_DynamicLightWorld = viewport_state.render_dynamic_lights_world ? 1 : 0;
	g_CV_EnvMapPolyGrids = viewport_state.render_polygrid_env_map ? 1 : 0;
	g_CV_BumpMapPolyGrids = viewport_state.render_polygrid_bump_map ? 1 : 0;
	g_CV_ModelLODOffset = viewport_state.render_model_lod_offset;
	g_CV_ModelDebug_DrawBoxes = viewport_state.render_model_debug_boxes ? 1 : 0;
	g_CV_ModelDebug_DrawTouchingLights = viewport_state.render_model_debug_touching_lights ? 1 : 0;
	g_CV_ModelDebug_DrawSkeleton = viewport_state.render_model_debug_skeleton ? 1 : 0;
	g_CV_ModelDebug_DrawOBBS = viewport_state.render_model_debug_obbs ? 1 : 0;
	g_CV_ModelDebug_DrawVertexNormals = viewport_state.render_model_debug_vertex_normals ? 1 : 0;
	g_CV_DrawWorldTree = viewport_state.render_world_debug_nodes ? 1 : 0;
	g_CV_DrawWorldLeaves = viewport_state.render_world_debug_leaves ? 1 : 0;
	g_CV_DrawWorldPortals = viewport_state.render_world_debug_portals ? 1 : 0;
	g_CV_ScreenGlowEnable = viewport_state.render_screen_glow_enable ? 1 : 0;
	g_CV_ScreenGlowShowTexture = viewport_state.render_screen_glow_show_texture ? 1 : 0;
	g_CV_ScreenGlowShowFilter = viewport_state.render_screen_glow_show_filter ? 1 : 0;
	g_CV_ScreenGlowShowTextureScale = viewport_state.render_screen_glow_show_texture_scale;
	g_CV_ScreenGlowShowFilterScale = viewport_state.render_screen_glow_show_filter_scale;
	g_CV_ScreenGlowUVScale = viewport_state.render_screen_glow_uv_scale;
	g_CV_ScreenGlowTextureSize = viewport_state.render_screen_glow_texture_size;
	g_CV_ScreenGlowFilterSize = viewport_state.render_screen_glow_filter_size;
	g_CV_SSAOEnable = viewport_state.render_ssao_enable ? 1 : 0;
	g_CV_SSAOBackend = viewport_state.render_ssao_backend;
	g_CV_SSAOIntensity = viewport_state.render_ssao_intensity;
	g_CV_SSAOFxEffectRadius = viewport_state.render_ssao_fx_effect_radius;
	g_CV_SSAOFxEffectFalloffRange = viewport_state.render_ssao_fx_effect_falloff_range;
	g_CV_SSAOFxRadiusMultiplier = viewport_state.render_ssao_fx_radius_multiplier;
	g_CV_SSAOFxDepthMipOffset = viewport_state.render_ssao_fx_depth_mip_offset;
	g_CV_SSAOFxTemporalStability = viewport_state.render_ssao_fx_temporal_stability;
	g_CV_SSAOFxSpatialReconstructionRadius = viewport_state.render_ssao_fx_spatial_reconstruction_radius;
	g_CV_SSAOFxAlphaInterpolation = viewport_state.render_ssao_fx_alpha_interpolation;
	g_CV_SSAOFxHalfResolution = viewport_state.render_ssao_fx_half_resolution ? 1 : 0;
	g_CV_SSAOFxHalfPrecisionDepth = viewport_state.render_ssao_fx_half_precision_depth ? 1 : 0;
	g_CV_SSAOFxUniformWeighting = viewport_state.render_ssao_fx_uniform_weighting ? 1 : 0;
	g_CV_SSAOFxResetAccumulation = viewport_state.render_ssao_fx_reset_accumulation ? 1 : 0;
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
	std::vector<std::unique_ptr<WorldModelInstance>> sky_world_models;
	std::vector<LTObject*> sky_objects;
	std::vector<std::string> sky_world_model_names;
};

void BuildSkyWorldModels(
	DiligentContext& ctx,
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props,
	const std::string& selection)
{
	ctx.sky_world_models.clear();
	ctx.sky_objects.clear();
	ctx.sky_world_model_names.clear();

	if (ctx.engine.world_bsp_model_ptrs.empty())
	{
		return;
	}

	std::unordered_map<std::string, std::vector<WorldBsp*>> world_by_name;
	world_by_name.reserve(ctx.engine.world_bsp_model_ptrs.size());
	std::vector<std::string> world_model_names;
	world_model_names.reserve(ctx.engine.world_bsp_model_ptrs.size());
	for (auto* bsp : ctx.engine.world_bsp_model_ptrs)
	{
		if (!bsp)
		{
			continue;
		}
		const std::string name = TrimQuotes(bsp->m_WorldName);
		if (!name.empty())
		{
			world_by_name[LowerCopy(name)].push_back(bsp);
			world_model_names.push_back(name);
		}
	}

	std::unordered_set<WorldBsp*> used;
	used.reserve(world_by_name.size());

	std::unordered_map<std::string, std::string> object_to_world;
	object_to_world.reserve(nodes.size());
	const size_t map_count = std::min(nodes.size(), props.size());
	for (size_t i = 0; i < map_count; ++i)
	{
		if (nodes[i].deleted)
		{
			continue;
		}
		if (nodes[i].name.empty())
		{
			continue;
		}
		const std::string target = GuessWorldModelNameForObject(nodes[i], props[i]);
		if (target.empty())
		{
			continue;
		}
		object_to_world.emplace(LowerCopy(TrimQuotes(nodes[i].name)), LowerCopy(target));
	}

	auto add_world_model = [&](WorldBsp* bsp, const NodeProperties* props_source)
	{
		if (!bsp || used.find(bsp) != used.end())
		{
			return;
		}
		auto instance = std::make_unique<WorldModelInstance>();
		instance->InitWorldData(bsp, nullptr);
		instance->m_Flags |= FLAG_VISIBLE;
		instance->m_Flags2 |= FLAG2_SKYOBJECT;
		if (props_source)
		{
			instance->m_Pos.Init(props_source->position[0], props_source->position[1], props_source->position[2]);
		}
		else
		{
			instance->m_Pos.Init();
		}
		instance->m_Rotation.Init();
		obj_SetupWorldModelTransform(instance.get());
		ctx.sky_objects.push_back(static_cast<LTObject*>(instance.get()));
		ctx.sky_world_models.push_back(std::move(instance));
		used.insert(bsp);
	};

	const size_t count = std::min(nodes.size(), props.size());
	bool has_sky_pointer = false;
	std::vector<std::string> sky_pointer_targets;
	sky_pointer_targets.reserve(count);
	std::unordered_set<std::string> sky_pointer_seen;
	sky_pointer_seen.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		if (nodes[i].deleted)
		{
			continue;
		}
		if (!IsSkyPointerNode(props[i]))
		{
			continue;
		}
		has_sky_pointer = true;
		std::string target;
		if (!TryGetRawPropertyStringAny(props[i], {"objectname", "skyobject", "skyobjectname"}, target))
		{
			continue;
		}
		if (!target.empty())
		{
			const std::string normalized = LowerCopy(target);
			if (sky_pointer_seen.insert(normalized).second)
			{
				sky_pointer_targets.push_back(target);
			}
		}
	}
	if (!sky_pointer_targets.empty())
	{
		DEdit2_Log("[Sky] SkyPointer targets: %zu", sky_pointer_targets.size());
		for (const auto& target : sky_pointer_targets)
		{
			DEdit2_Log("[Sky]   target: %s", target.c_str());
		}
	}
	for (const auto& entry : world_by_name)
	{
		if (entry.second.size() > 1)
		{
			DEdit2_Log("[Sky] Duplicate world model '%s': %zu", entry.first.c_str(), entry.second.size());
		}
	}
	ctx.sky_world_model_names = sky_pointer_targets;
	if (ctx.sky_world_model_names.empty())
	{
		ctx.sky_world_model_names = world_model_names;
	}

	auto resolve_target_to_world = [&](const std::string& target) -> WorldBsp*
	{
		for (const auto& name : world_model_names)
		{
			if (SkyTargetMatchesWorldName(target, name))
			{
				auto it = world_by_name.find(LowerCopy(name));
				if (it != world_by_name.end() && !it->second.empty())
				{
					return it->second.front();
				}
			}
		}
		auto it = world_by_name.find(LowerCopy(target));
		if (it != world_by_name.end() && !it->second.empty())
		{
			return it->second.front();
		}
		return nullptr;
	};

	DEdit2_Log("[Sky] Sky objects built: %zu (selection=%s)",
		ctx.sky_objects.size(),
		selection.empty() ? "Auto" : selection.c_str());

	if (!selection.empty())
	{
		if (WorldBsp* selected = resolve_target_to_world(selection))
		{
			ctx.sky_world_models.clear();
			ctx.sky_objects.clear();
			add_world_model(selected, nullptr);
		}
		return;
	}

	if (has_sky_pointer && !sky_pointer_targets.empty())
	{
		if (WorldBsp* selected = resolve_target_to_world(sky_pointer_targets.front()))
		{
			ctx.sky_world_models.clear();
			ctx.sky_objects.clear();
			add_world_model(selected, nullptr);
		}
		return;
	}

	for (size_t i = 0; i < count; ++i)
	{
		if (nodes[i].deleted)
		{
			continue;
		}
		if (!IsSkyNode(props[i]))
		{
			continue;
		}

		std::string target = GuessSkyWorldModelName(nodes[i], props[i]);
		if (!target.empty())
		{
			const std::string target_lower = LowerCopy(target);
			if (WorldBsp* selected = resolve_target_to_world(target))
			{
				add_world_model(selected, &props[i]);
				continue;
			}
			auto ref = object_to_world.find(target_lower);
			if (ref != object_to_world.end())
			{
				if (WorldBsp* selected = resolve_target_to_world(ref->second))
				{
					add_world_model(selected, &props[i]);
					continue;
				}
			}
		}

		std::string object_ref;
		if (TryGetRawPropertyStringAny(props[i], {"objectname", "skyobject", "skyobjectname"}, object_ref))
		{
			const std::string object_lower = LowerCopy(object_ref);
			auto ref = object_to_world.find(object_lower);
			if (ref != object_to_world.end())
			{
				auto obj_it = world_by_name.find(ref->second);
				if (obj_it != world_by_name.end())
				{
					for (auto* bsp : obj_it->second)
					{
						add_world_model(bsp, &props[i]);
					}
					continue;
				}
			}
		}

		if (!target.empty())
		{
			if (WorldBsp* selected = resolve_target_to_world(target))
			{
				add_world_model(selected, &props[i]);
			}
		}
	}

	if (!ctx.sky_objects.empty())
	{
		return;
	}

	for (size_t i = 0; i < count; ++i)
	{
		if (nodes[i].deleted)
		{
			continue;
		}
		const std::string type = LowerCopy(props[i].type);
		const std::string cls = LowerCopy(props[i].class_name);
		const std::string name = LowerCopy(TrimQuotes(nodes[i].name));
		const bool is_translucent_world_model =
			type.find("translucentworldmodel") != std::string::npos ||
			cls.find("translucentworldmodel") != std::string::npos;
		const bool looks_like_sky_name = name.rfind("sl_", 0) == 0;
		if (!is_translucent_world_model && !looks_like_sky_name)
		{
			continue;
		}

		std::string target = GuessWorldModelNameForObject(nodes[i], props[i]);
		if (target.empty())
		{
			target = nodes[i].name;
		}
		if (target.empty())
		{
			continue;
		}
		if (WorldBsp* selected = resolve_target_to_world(target))
		{
			add_world_model(selected, &props[i]);
		}
	}

	if (!ctx.sky_objects.empty())
	{
		return;
	}

	for (const auto& entry : world_by_name)
	{
		if (entry.first.find("sky") == std::string::npos)
		{
			continue;
		}
		if (!entry.second.empty())
		{
			add_world_model(entry.second.front(), nullptr);
		}
	}
}

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

void RenderViewport(
	DiligentContext& ctx,
	const ViewportPanelState& viewport_state,
	const NodeProperties* world_props,
	const ViewportOverlayState& overlay_state,
	std::vector<DynamicLight>& dynamic_lights)
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
	if (ctx.engine.render_struct && ctx.engine.render_struct->Clear)
	{
		const auto to_u8 = [](float value) -> uint8
		{
			const float clamped = std::clamp(value, 0.0f, 1.0f);
			return static_cast<uint8>(clamped * 255.0f + 0.5f);
		};

		LTRGBColor clear_color_ltrgb{};
		clear_color_ltrgb.rgb.r = to_u8(clear_color[0]);
		clear_color_ltrgb.rgb.g = to_u8(clear_color[1]);
		clear_color_ltrgb.rgb.b = to_u8(clear_color[2]);
		clear_color_ltrgb.rgb.a = to_u8(clear_color[3]);
		ctx.engine.render_struct->Clear(nullptr, 0, clear_color_ltrgb);
	}
	else
	{
		ctx.engine.context->ClearRenderTarget(render_target, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		ctx.engine.context->ClearDepthStencil(
			depth_target,
			Diligent::CLEAR_DEPTH_FLAG,
			1.0f,
			0,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}

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
		scene.m_SkyObjects = ctx.sky_objects.empty() ? nullptr : ctx.sky_objects.data();
		scene.m_nSkyObjects = static_cast<int>(ctx.sky_objects.size());

		std::vector<LTObject*> object_list;
		if (!dynamic_lights.empty())
		{
			object_list.reserve(dynamic_lights.size());
			for (DynamicLight& light : dynamic_lights)
			{
				object_list.push_back(static_cast<LTObject*>(&light));
			}
		}

		if (!object_list.empty())
		{
			scene.m_DrawMode = DRAWMODE_OBJECTLIST;
			scene.m_pObjectList = object_list.data();
			scene.m_ObjectListSize = static_cast<int>(object_list.size());
		}
		else
		{
			scene.m_DrawMode = DRAWMODE_NORMAL;
			scene.m_pObjectList = nullptr;
			scene.m_ObjectListSize = 0;
		}

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

	// Ensure editor overlays render to the viewport targets even if the renderer
	// temporarily bound offscreen targets for post-processing.
	diligent_SetOutputTargets(render_target, depth_target);
	ctx.engine.context->SetRenderTargets(
		1,
		&render_target,
		depth_target,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (ctx.grid_ready)
	{
		const float aspect = ctx.viewport.height > 0 ? static_cast<float>(ctx.viewport.width) /
			static_cast<float>(ctx.viewport.height) : 1.0f;
		const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_state, aspect);
		const Diligent::float3 grid_origin{
			viewport_state.grid_origin[0],
			viewport_state.grid_origin[1],
			viewport_state.grid_origin[2]};
		UpdateViewportGridConstants(ctx.engine.context, ctx.grid_renderer, view_proj, grid_origin);

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

	if (ctx.engine.render_struct && ctx.engine.render_struct->m_bInitted)
	{
		RenderViewportOverlays(overlay_state);
	}

	diligent_SetOutputTargets(nullptr, nullptr);
	ctx.engine.context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
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
	std::string project_root = ResolveProjectRoot(paths.project_root);
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
		diligent.engine.swapchain ? diligent.engine.swapchain->GetDesc().ColorBufferFormat
			: Diligent::TEX_FORMAT_RGBA8_UNORM,
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
	project_panel.selected_id = DefaultProjectSelection(project_nodes);
	scene_panel.error = scene_error;
	scene_panel.selected_id = scene_nodes.empty() ? -1 : 0;
	SelectionTarget active_target = SelectionTarget::Project;
	WorldRenderSettingsCache world_settings_cache;
	DEdit2_SetConsoleBindings({&project_nodes, &project_props, &scene_nodes, &scene_props});

	auto load_scene_world = [&](const std::string& path)
	{
		world_file = path;
		diligent.engine.world_loaded = false;
		viewport_panel.lightmaps_autoloaded = false;
		viewport_panel.render_lightmaps_available = false;
		scene_nodes = BuildSceneTree(world_file, scene_props, scene_error);
		scene_panel.error = scene_error;
		scene_panel.selected_id = scene_nodes.empty() ? -1 : 0;
		scene_panel.tree_ui = {};
		active_target = SelectionTarget::Scene;

		if (const NodeProperties* world_props = FindWorldProperties(scene_props))
		{
			ApplyWorldSettingsToRenderer(*world_props);
			UpdateWorldSettingsCache(*world_props, world_settings_cache);
		}

		const std::string compiled_path = FindCompiledWorldPath(world_file, project_root);
		if (!compiled_path.empty())
		{
			std::string render_error;
			if (!LoadRenderWorld(diligent.engine, compiled_path, render_error))
			{
				std::fprintf(stderr, "World render load failed: %s\n", render_error.c_str());
				diligent.sky_world_models.clear();
				diligent.sky_objects.clear();
			}
			else
			{
				DiligentWorldTextureStats stats{};
				if (diligent_GetWorldTextureStats(stats))
				{
					const bool has_lightmaps = stats.lightmap_present > 0;
					viewport_panel.render_lightmaps_available = has_lightmaps;
					if (has_lightmaps)
					{
						if (!viewport_panel.lightmaps_autoloaded)
						{
							viewport_panel.render_lightmap_mode =
								static_cast<int>(ViewportPanelState::WorldLightmapMode::Baked);
							viewport_panel.lightmaps_autoloaded = true;
						}
					}
					else
					{
						viewport_panel.render_lightmap_mode =
							static_cast<int>(ViewportPanelState::WorldLightmapMode::Dynamic);
						viewport_panel.lightmaps_autoloaded = true;
					}
				}
			}
		}

		// Sync model instances with the scene tree for rendering
		BuildSkyWorldModels(diligent, scene_nodes, scene_props, viewport_panel.sky_world_model);
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

		ViewportOverlayState overlay_state{};

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
				project_root = ResolveProjectRoot(selected_path);
				project_error.clear();
				project_nodes = BuildProjectTree(project_root, project_props, project_error);
				project_panel.error = project_error;
				project_panel.selected_id = DefaultProjectSelection(project_nodes);
				project_panel.tree_ui = {};
				world_browser.refresh = true;
				SetEngineProjectRoot(diligent.engine, project_root);
				PushRecentProject(recent_projects, project_root);
			}
		}
		if (menu_actions.open_recent_project)
		{
			project_root = ResolveProjectRoot(menu_actions.recent_project_path);
			project_error.clear();
			project_nodes = BuildProjectTree(project_root, project_props, project_error);
			project_panel.error = project_error;
			project_panel.selected_id = DefaultProjectSelection(project_nodes);
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
			if (ImGui::Button("Center"))
			{
				float bounds_min[3] = {0.0f, 0.0f, 0.0f};
				float bounds_max[3] = {0.0f, 0.0f, 0.0f};
				if (TryGetWorldBounds(scene_props, bounds_min, bounds_max))
				{
					const float center[3] = {
						(bounds_min[0] + bounds_max[0]) * 0.5f,
						(bounds_min[1] + bounds_max[1]) * 0.5f,
						(bounds_min[2] + bounds_max[2]) * 0.5f};
					viewport_panel.grid_origin[0] = center[0];
					viewport_panel.grid_origin[1] = center[1];
					viewport_panel.grid_origin[2] = center[2];
					viewport_panel.target[0] = center[0];
					viewport_panel.target[1] = center[1];
					viewport_panel.target[2] = center[2];

					const float dx = bounds_max[0] - bounds_min[0];
					const float dy = bounds_max[1] - bounds_min[1];
					const float dz = bounds_max[2] - bounds_min[2];
					const float radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
					const float desired = std::max(100.0f, radius * 2.6f);
					viewport_panel.orbit_distance = std::min(desired, 20000.0f);
					if (viewport_panel.fly_mode)
					{
						SyncFlyFromOrbit(viewport_panel);
					}
				}
				else
				{
					DEdit2_Log("World bounds unavailable; cannot center view.");
				}
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
			ImGui::SameLine();
			if (ImGui::Button("Render"))
			{
				ImGui::OpenPopup("RenderSettings");
			}
			ImGui::SameLine();
			const float eye_button_size = ImGui::GetFrameHeight();
			if (ImGui::Button("##LightIconSettings", ImVec2(eye_button_size, eye_button_size)))
			{
				ImGui::OpenPopup("LightIconSettings");
			}
			{
				const ImVec2 min = ImGui::GetItemRectMin();
				const ImVec2 max = ImGui::GetItemRectMax();
				const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
				const ImVec2 radius(
					(max.x - min.x) * 0.38f,
					(max.y - min.y) * 0.24f);
				const ImU32 eye_color = ImGui::GetColorU32(
					viewport_panel.show_light_icons ? ImGuiCol_Text : ImGuiCol_TextDisabled);
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				draw_list->AddEllipse(center, radius, eye_color, 0.0f, 0, 1.2f);
				draw_list->AddCircleFilled(center, radius.y * 0.55f, eye_color);
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
			if (ImGui::BeginPopup("LightIconSettings"))
			{
				ImGui::TextUnformatted("Lightbulbs");
				ImGui::Separator();
				ImGui::Checkbox("Show Lightbulb Icons", &viewport_panel.show_light_icons);
				if (!viewport_panel.show_light_icons)
				{
					ImGui::BeginDisabled();
				}
				ImGui::Checkbox("Occlude Behind Geometry", &viewport_panel.light_icon_occlusion);
				if (!viewport_panel.show_light_icons)
				{
					ImGui::EndDisabled();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopup("RenderSettings"))
			{
				if (ImGui::BeginTabBar("RenderSettingsTabs"))
				{
					if (ImGui::BeginTabItem("Draw"))
					{
						if (ImGui::Button("All On"))
						{
							viewport_panel.render_draw_world = true;
							viewport_panel.render_draw_world_models = true;
							viewport_panel.render_draw_models = true;
							viewport_panel.render_draw_sprites = true;
							viewport_panel.render_draw_polygrids = true;
							viewport_panel.render_draw_particles = true;
							viewport_panel.render_draw_volume_effects = true;
							viewport_panel.render_draw_line_systems = true;
							viewport_panel.render_draw_canvases = true;
							viewport_panel.render_draw_sky = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("All Off"))
						{
							viewport_panel.render_draw_world = false;
							viewport_panel.render_draw_world_models = false;
							viewport_panel.render_draw_models = false;
							viewport_panel.render_draw_sprites = false;
							viewport_panel.render_draw_polygrids = false;
							viewport_panel.render_draw_particles = false;
							viewport_panel.render_draw_volume_effects = false;
							viewport_panel.render_draw_line_systems = false;
							viewport_panel.render_draw_canvases = false;
							viewport_panel.render_draw_sky = false;
						}
						ImGui::Separator();
						ImGui::Checkbox("World", &viewport_panel.render_draw_world);
						ImGui::Checkbox("World Models", &viewport_panel.render_draw_world_models);
						ImGui::Checkbox("Models", &viewport_panel.render_draw_models);
						ImGui::Checkbox("Sprites", &viewport_panel.render_draw_sprites);
						ImGui::Checkbox("PolyGrids", &viewport_panel.render_draw_polygrids);
						ImGui::Checkbox("Particles", &viewport_panel.render_draw_particles);
						ImGui::Checkbox("Volume Effects", &viewport_panel.render_draw_volume_effects);
						ImGui::Checkbox("Line Systems", &viewport_panel.render_draw_line_systems);
						ImGui::Checkbox("Canvases", &viewport_panel.render_draw_canvases);
						ImGui::Checkbox("Sky", &viewport_panel.render_draw_sky);
						if (!viewport_panel.render_draw_sky || diligent.sky_world_model_names.empty())
						{
							ImGui::BeginDisabled();
						}
						const char* current_sky = viewport_panel.sky_world_model.empty()
							? "Auto"
							: viewport_panel.sky_world_model.c_str();
						if (ImGui::BeginCombo("Sky Model", current_sky))
						{
							const bool auto_selected = viewport_panel.sky_world_model.empty();
							if (ImGui::Selectable("Auto", auto_selected))
							{
								viewport_panel.sky_world_model.clear();
								BuildSkyWorldModels(diligent, scene_nodes, scene_props, viewport_panel.sky_world_model);
							}
							if (auto_selected)
							{
								ImGui::SetItemDefaultFocus();
							}
							for (const auto& name : diligent.sky_world_model_names)
							{
								const bool selected = (viewport_panel.sky_world_model == name);
								if (ImGui::Selectable(name.c_str(), selected))
								{
									viewport_panel.sky_world_model = name;
									BuildSkyWorldModels(diligent, scene_nodes, scene_props, viewport_panel.sky_world_model);
								}
								if (selected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}
						if (!viewport_panel.render_draw_sky || diligent.sky_world_model_names.empty())
						{
							ImGui::EndDisabled();
						}
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Modes"))
					{
						ImGui::Checkbox("Wireframe World", &viewport_panel.render_wireframe_world);
						ImGui::Checkbox("Wireframe Models", &viewport_panel.render_wireframe_models);
						const char* shading_modes[] = {"Textured", "Flat", "Normals"};
						ImGui::Combo("World Shading Mode", &viewport_panel.render_world_shading_mode, shading_modes, IM_ARRAYSIZE(shading_modes));
						ImGui::Checkbox("Wireframe Overlay", &viewport_panel.render_wireframe_overlay);
						ImGui::Checkbox("Fullbright", &viewport_panel.render_fullbright);
						ImGui::Separator();
						ImGui::TextUnformatted("Texture Filtering");
						const char* anisotropy_levels[] = {"Off", "2x", "4x", "8x", "16x"};
						int anisotropy_index = 0;
						switch (g_CV_Anisotropic.m_Val)
						{
							case 16: anisotropy_index = 4; break;
							case 8: anisotropy_index = 3; break;
							case 4: anisotropy_index = 2; break;
							case 2: anisotropy_index = 1; break;
							default: anisotropy_index = 0; break;
						}
						if (ImGui::Combo("Anisotropic", &anisotropy_index, anisotropy_levels, IM_ARRAYSIZE(anisotropy_levels)))
						{
							static const int kLevels[] = {0, 2, 4, 8, 16};
							g_CV_Anisotropic.m_Val = kLevels[anisotropy_index];
						}
						ImGui::Separator();
						ImGui::TextUnformatted("Antialiasing");
						const char* msaa_levels[] = {"Off", "2x", "4x", "8x"};
						int msaa_index = 0;
						switch (g_CV_MSAA.m_Val)
						{
							case 8: msaa_index = 3; break;
							case 4: msaa_index = 2; break;
							case 2: msaa_index = 1; break;
							default: msaa_index = 0; break;
						}
						if (ImGui::Combo("MSAA", &msaa_index, msaa_levels, IM_ARRAYSIZE(msaa_levels)))
						{
							static const int kMsaaLevels[] = {0, 2, 4, 8};
							g_CV_MSAA.m_Val = kMsaaLevels[msaa_index];
						}
						const char* lightmap_modes[] = {"Dynamic", "Baked (if available)", "Lightmap Only"};
						int& lightmap_mode = viewport_panel.render_lightmap_mode;
						if (!viewport_panel.render_lightmaps_available && lightmap_mode != 0)
						{
							lightmap_mode = 0;
						}
						const char* current_lightmap_mode = lightmap_modes[lightmap_mode];
						if (ImGui::BeginCombo("World Lighting", current_lightmap_mode))
						{
							if (ImGui::Selectable(lightmap_modes[0], lightmap_mode == 0))
							{
								lightmap_mode = 0;
							}
							if (!viewport_panel.render_lightmaps_available)
							{
								ImGui::BeginDisabled();
							}
							if (ImGui::Selectable(lightmap_modes[1], lightmap_mode == 1))
							{
								lightmap_mode = 1;
							}
							if (ImGui::Selectable(lightmap_modes[2], lightmap_mode == 2))
							{
								lightmap_mode = 2;
							}
							if (!viewport_panel.render_lightmaps_available)
							{
								ImGui::EndDisabled();
							}
							ImGui::EndCombo();
						}
						ImGui::DragFloat("Lightmap Intensity", &viewport_panel.render_lightmap_intensity, 0.05f, 0.0f, 4.0f, "%.2f");
						ImGui::Checkbox("Draw Sorted", &viewport_panel.render_draw_sorted);
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Models"))
					{
						ImGui::Checkbox("Solid Models", &viewport_panel.render_draw_solid_models);
						ImGui::Checkbox("Translucent Models", &viewport_panel.render_draw_translucent_models);
						ImGui::Checkbox("Texture Models", &viewport_panel.render_texture_models);
						ImGui::DragInt("Model LOD Offset", &viewport_panel.render_model_lod_offset, 1.0f, -5, 5);
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Lighting"))
					{
						ImGui::Checkbox("Light Models", &viewport_panel.render_light_models);
						ImGui::Checkbox("Apply Ambient", &viewport_panel.render_model_apply_ambient);
						ImGui::Checkbox("Apply Sun", &viewport_panel.render_model_apply_sun);
						ImGui::SliderFloat("Saturation", &viewport_panel.render_model_saturation, 0.0f, 2.0f, "%.2f");
						ImGui::Separator();
						ImGui::TextUnformatted("World");
						ImGui::Checkbox("Dynamic Lights", &viewport_panel.render_dynamic_lights);
						ImGui::Checkbox("World Dynamic Lights", &viewport_panel.render_dynamic_lights_world);
						ImGui::Checkbox("PolyGrid Env Map", &viewport_panel.render_polygrid_env_map);
						ImGui::Checkbox("PolyGrid Bump Map", &viewport_panel.render_polygrid_bump_map);
						ImGui::Separator();
						ImGui::TextUnformatted("Editor Light Scaling");
						ImGui::DragFloat("Intensity Scale", &g_CV_DEdit2LightIntensityScale.m_Val, 0.05f, 0.0f, 10.0f);
						ImGui::DragFloat("Radius Scale", &g_CV_DEdit2LightRadiusScale.m_Val, 0.05f, 0.0f, 10.0f);
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Debug"))
					{
						ImGui::Checkbox("Draw Boxes", &viewport_panel.render_model_debug_boxes);
						ImGui::Checkbox("Draw Touching Lights", &viewport_panel.render_model_debug_touching_lights);
						ImGui::Checkbox("Draw Skeleton", &viewport_panel.render_model_debug_skeleton);
						ImGui::Checkbox("Draw OBBs", &viewport_panel.render_model_debug_obbs);
						ImGui::Checkbox("Draw Vertex Normals", &viewport_panel.render_model_debug_vertex_normals);
						ImGui::Separator();
						ImGui::TextUnformatted("World");
						ImGui::Checkbox("World Nodes", &viewport_panel.render_world_debug_nodes);
						ImGui::Checkbox("World Leaves", &viewport_panel.render_world_debug_leaves);
						ImGui::Checkbox("World Portals", &viewport_panel.render_world_debug_portals);
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Post FX"))
					{
						ImGui::Checkbox("Screen Glow", &viewport_panel.render_screen_glow_enable);
						ImGui::Checkbox("Show Glow Texture", &viewport_panel.render_screen_glow_show_texture);
						ImGui::SameLine();
						ImGui::DragFloat("Scale##GlowTex", &viewport_panel.render_screen_glow_show_texture_scale, 0.01f, 0.05f, 1.0f);
						ImGui::Checkbox("Show Glow Filter", &viewport_panel.render_screen_glow_show_filter);
						ImGui::SameLine();
						ImGui::DragFloat("Scale##GlowFilter", &viewport_panel.render_screen_glow_show_filter_scale, 0.01f, 0.05f, 1.0f);
						ImGui::DragFloat("Glow UV Scale", &viewport_panel.render_screen_glow_uv_scale, 0.01f, 0.1f, 2.0f);
						ImGui::DragInt("Glow Texture Size", &viewport_panel.render_screen_glow_texture_size, 1.0f, 64, 2048);
						ImGui::DragInt("Glow Filter Size", &viewport_panel.render_screen_glow_filter_size, 1.0f, 4, 128);
						ImGui::Separator();
						ImGui::TextUnformatted("SSAO");
						ImGui::Checkbox("Enable##SSAO", &viewport_panel.render_ssao_enable);
						static const char* ssao_backend_labels[] = {"Legacy", "DiligentFX"};
						ImGui::Combo("Backend##SSAO", &viewport_panel.render_ssao_backend, ssao_backend_labels, IM_ARRAYSIZE(ssao_backend_labels));
						const ImGuiSliderFlags ssao_clamp_flags = ImGuiSliderFlags_AlwaysClamp;
						ImGui::SliderFloat("Intensity##SSAO", &viewport_panel.render_ssao_intensity, 0.0f, 4.0f, "%.2f", ssao_clamp_flags);
						ImGui::SliderFloat("Effect Radius##SSAO", &viewport_panel.render_ssao_fx_effect_radius, 1.0f, 300.0f, "%.2f", ssao_clamp_flags);
						ImGui::SliderFloat("Falloff Range##SSAO", &viewport_panel.render_ssao_fx_effect_falloff_range, 0.1f, 10.0f, "%.2f", ssao_clamp_flags);
						ImGui::SliderFloat("Radius Multiplier##SSAO", &viewport_panel.render_ssao_fx_radius_multiplier, 0.1f, 4.0f, "%.2f", ssao_clamp_flags);
						ImGui::SliderFloat("Depth MIP Offset##SSAO", &viewport_panel.render_ssao_fx_depth_mip_offset, 0.0f, 8.0f, "%.2f", ssao_clamp_flags);
						ImGui::SliderFloat("Temporal Stability##SSAO", &viewport_panel.render_ssao_fx_temporal_stability, 0.0f, 0.999f, "%.3f", ssao_clamp_flags);
						ImGui::SliderFloat("Spatial Reconstruction##SSAO", &viewport_panel.render_ssao_fx_spatial_reconstruction_radius, 0.1f, 16.0f, "%.2f", ssao_clamp_flags);
						ImGui::SliderFloat("Alpha Interp##SSAO", &viewport_panel.render_ssao_fx_alpha_interpolation, 0.0f, 1.0f, "%.2f", ssao_clamp_flags);
						ImGui::Checkbox("Half Resolution##SSAO", &viewport_panel.render_ssao_fx_half_resolution);
						ImGui::Checkbox("Half Precision Depth##SSAO", &viewport_panel.render_ssao_fx_half_precision_depth);
						ImGui::Checkbox("Uniform Weighting##SSAO", &viewport_panel.render_ssao_fx_uniform_weighting);
						ImGui::Checkbox("Reset Accumulation##SSAO", &viewport_panel.render_ssao_fx_reset_accumulation);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Advanced"))
					{
						static char filter_text[128] = "";
						ImGui::InputText("Filter", filter_text, sizeof(filter_text));
						ImGui::SameLine();
						if (ImGui::Button("Clear"))
						{
							filter_text[0] = '\0';
						}

						std::vector<BaseConVar*> vars;
						for (BaseConVar* var = g_pConVars; var; var = var->m_pNext)
						{
							vars.push_back(var);
						}
						std::sort(vars.begin(), vars.end(), [](const BaseConVar* a, const BaseConVar* b)
						{
							return std::strcmp(a->m_pName, b->m_pName) < 0;
						});

						const std::string filter_lower = LowerCopy(filter_text);
						constexpr ImGuiTableFlags kTableFlags =
							ImGuiTableFlags_BordersInnerV |
							ImGuiTableFlags_RowBg |
							ImGuiTableFlags_ScrollY |
							ImGuiTableFlags_SizingStretchProp;
						if (ImGui::BeginTable("RenderCvars", 3, kTableFlags, ImVec2(0.0f, 240.0f)))
						{
							ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 220.0f);
							ImGui::TableSetupColumn("Value");
							ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableHeadersRow();

							for (BaseConVar* var : vars)
							{
								if (!var || !var->m_pName)
								{
									continue;
								}
								if (!filter_lower.empty())
								{
									const std::string name_lower = LowerCopy(var->m_pName);
									if (name_lower.find(filter_lower) == std::string::npos)
									{
										continue;
									}
								}

								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::TextUnformatted(var->m_pName);

								ImGui::TableSetColumnIndex(1);
								const float current = var->GetFloat();
								const bool is_bool =
									(current == 0.0f || current == 1.0f) &&
									(var->m_DefaultVal == 0.0f || var->m_DefaultVal == 1.0f);
								if (is_bool)
								{
									bool enabled = (current != 0.0f);
									if (ImGui::Checkbox(("##val_" + std::string(var->m_pName)).c_str(), &enabled))
									{
										var->SetFloat(enabled ? 1.0f : 0.0f);
									}
								}
								else
								{
									float value = current;
									if (ImGui::DragFloat(("##val_" + std::string(var->m_pName)).c_str(), &value, 0.1f))
									{
										var->SetFloat(value);
									}
								}

								ImGui::TableSetColumnIndex(2);
								if (ImGui::SmallButton(("Reset##" + std::string(var->m_pName)).c_str()))
								{
									var->SetFloat(var->m_DefaultVal);
								}
							}

							ImGui::EndTable();
						}

						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
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

			const DEdit2TexViewState& texview = DEdit2_GetTexViewState();
			if (drew_image && texview.enabled && texview.view && texview.width > 0 && texview.height > 0)
			{
				const float max_size = std::min(256.0f, avail.x * 0.35f);
				const float aspect = static_cast<float>(texview.height) / static_cast<float>(texview.width);
				ImVec2 size(max_size, max_size * aspect);
				const float max_height = avail.y * 0.35f;
				if (size.y > max_height && max_height > 0.0f)
				{
					size.y = max_height;
					size.x = size.y / aspect;
				}

				const ImVec2 pos(
					viewport_pos.x + avail.x - size.x - 8.0f,
					viewport_pos.y + 8.0f);
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				draw_list->AddImage(
					reinterpret_cast<ImTextureID>(texview.view),
					pos,
					ImVec2(pos.x + size.x, pos.y + size.y));
				draw_list->AddRect(
					pos,
					ImVec2(pos.x + size.x, pos.y + size.y),
					IM_COL32(255, 255, 255, 200));
				draw_list->AddText(
					ImVec2(pos.x, pos.y + size.y + 4.0f),
					IM_COL32(255, 255, 255, 200),
					texview.name.c_str());
			}

			const bool hovered = drew_image && ImGui::IsItemHovered();
			UpdateViewportControls(viewport_panel, viewport_pos, avail, hovered);
			bool gizmo_consumed_click = false;
			int hovered_scene_id = -1;
			Diligent::float3 hovered_hit_pos(0.0f, 0.0f, 0.0f);
			bool hovered_hit_valid = false;
			if (drew_image && hovered && active_target == SelectionTarget::Scene)
			{
				const int selected_id = scene_panel.selected_id;
				const size_t count = std::min(scene_nodes.size(), scene_props.size());
				if (selected_id >= 0 && static_cast<size_t>(selected_id) < count)
				{
					const TreeNode& node = scene_nodes[selected_id];
					NodeProperties& props = scene_props[selected_id];
						if (!node.deleted && !node.is_folder && !props.locked &&
							SceneNodePassesFilters(scene_panel, selected_id, scene_nodes, scene_props))
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
			if (hovered && !scene_nodes.empty() && !scene_props.empty())
			{
				const float aspect = avail.y > 0.0f ? (avail.x / avail.y) : 1.0f;
				const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);
				const ImVec2 mouse = ImGui::GetIO().MousePos;
				const ImVec2 local(mouse.x - viewport_pos.x, mouse.y - viewport_pos.y);
				const PickRay pick_ray = BuildPickRay(viewport_panel, avail, local);
				float best_t = 1.0e30f;
				int best_id = -1;
				const float light_pick_radius = 9.0f;
				const float light_pick_radius2 = light_pick_radius * light_pick_radius;
				float best_light_dist2 = light_pick_radius2;
				int best_light_id = -1;

				const size_t count = std::min(scene_nodes.size(), scene_props.size());
				for (size_t i = 0; i < count; ++i)
				{
					const TreeNode& node = scene_nodes[i];
					if (node.deleted || node.is_folder)
					{
						continue;
					}
					if (!SceneNodePassesFilters(
							scene_panel,
							static_cast<int>(i),
							scene_nodes,
							scene_props))
					{
						continue;
					}
					if (!NodePickableByRender(viewport_panel, scene_props[i]))
					{
						continue;
					}
					if (IsLightNode(scene_props[i]))
					{
						float pick_pos[3] = {scene_props[i].position[0], scene_props[i].position[1], scene_props[i].position[2]};
						TryGetNodePickPosition(scene_props[i], pick_pos);
						ImVec2 screen_pos;
						if (!ProjectWorldToScreen(view_proj, pick_pos, avail, screen_pos))
						{
							continue;
						}
						const float dx = screen_pos.x - local.x;
						const float dy = screen_pos.y - local.y;
						const float dist2 = dx * dx + dy * dy;
						if (dist2 <= best_light_dist2)
						{
							best_light_dist2 = dist2;
							best_light_id = static_cast<int>(i);
						}
						continue;
					}
					float hit_t = 0.0f;
					if (!RaycastNode(scene_props[i], pick_ray, hit_t))
					{
						continue;
					}
					if (hit_t < best_t)
					{
						best_t = hit_t;
						best_id = static_cast<int>(i);
					}
				}

				if (best_light_id >= 0)
				{
					hovered_scene_id = best_light_id;
					const NodeProperties& props = scene_props[best_light_id];
					hovered_hit_pos = Diligent::float3(
						props.position[0],
						props.position[1],
						props.position[2]);
					hovered_hit_valid = true;
				}
				else if (best_id >= 0)
				{
					hovered_scene_id = best_id;
					hovered_hit_pos = Diligent::float3(
						pick_ray.origin.x + pick_ray.dir.x * best_t,
						pick_ray.origin.y + pick_ray.dir.y * best_t,
						pick_ray.origin.z + pick_ray.dir.z * best_t);
					hovered_hit_valid = true;
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
					else if (hovered_scene_id >= 0)
					{
						scene_panel.selected_id = hovered_scene_id;
						active_target = SelectionTarget::Scene;
					}
				}
			}
			if (drew_image)
			{
				const float aspect = avail.y > 0.0f ? (avail.x / avail.y) : 1.0f;
				const Diligent::float4x4 view_proj = ComputeViewportViewProj(viewport_panel, aspect);
				Diligent::float3 cam_pos;
				Diligent::float3 cam_forward;
				Diligent::float3 cam_right;
				Diligent::float3 cam_up;
				ComputeCameraBasis(viewport_panel, cam_pos, cam_forward, cam_right, cam_up);
				(void)cam_forward;
				(void)cam_right;
				(void)cam_up;
				const size_t count = std::min(scene_nodes.size(), scene_props.size());
				const int selected_id = scene_panel.selected_id;
				overlay_state.count = 0;
				if (selected_id >= 0 && static_cast<size_t>(selected_id) < count)
				{
					const TreeNode& node = scene_nodes[selected_id];
					const NodeProperties& props = scene_props[selected_id];
					if (!node.deleted && !node.is_folder &&
						SceneNodePassesFilters(scene_panel, selected_id, scene_nodes, scene_props) &&
						NodePickableByRender(viewport_panel, props))
					{
						if (overlay_state.count < 2)
						{
							overlay_state.items[overlay_state.count++] =
								ViewportOverlayItem{&props, MakeOverlayColor(255, 200, 0, 255)};
						}
					}
				}

				if (hovered_scene_id >= 0 && hovered_scene_id != selected_id)
				{
					const TreeNode& node = scene_nodes[hovered_scene_id];
					const NodeProperties& props = scene_props[hovered_scene_id];
					if (!node.deleted && !node.is_folder &&
						SceneNodePassesFilters(scene_panel, hovered_scene_id, scene_nodes, scene_props) &&
						NodePickableByRender(viewport_panel, props))
					{
						if (overlay_state.count < 2)
						{
							overlay_state.items[overlay_state.count++] =
								ViewportOverlayItem{&props, MakeOverlayColor(255, 255, 255, 180)};
						}
					}
				}

				if (viewport_panel.show_light_icons)
				{
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					for (size_t i = 0; i < count; ++i)
					{
						const TreeNode& node = scene_nodes[i];
						const NodeProperties& props = scene_props[i];
						if (node.deleted || node.is_folder || !props.visible)
						{
							continue;
						}
						if (!IsLightNode(props))
						{
							continue;
						}
						if (!SceneNodePassesFilters(
								scene_panel,
								static_cast<int>(i),
								scene_nodes,
								scene_props))
						{
							continue;
						}
						if (!NodePickableByRender(viewport_panel, props))
						{
							continue;
						}

						float pick_pos[3] = {props.position[0], props.position[1], props.position[2]};
						TryGetNodePickPosition(props, pick_pos);
						ImVec2 screen_pos;
						if (!ProjectWorldToScreen(view_proj, pick_pos, avail, screen_pos))
						{
							continue;
						}
						if (viewport_panel.light_icon_occlusion &&
							IsLightIconOccluded(
								viewport_panel,
								scene_panel,
								scene_nodes,
								scene_props,
								static_cast<int>(i),
								cam_pos,
								pick_pos))
						{
							continue;
						}

						const ImVec2 center(
							viewport_pos.x + screen_pos.x,
							viewport_pos.y + screen_pos.y);
						const bool is_selected = static_cast<int>(i) == selected_id;
						const bool is_hovered = static_cast<int>(i) == hovered_scene_id;
						const float radius = is_selected ? 7.0f : (is_hovered ? 6.0f : 5.0f);
						const ImU32 bulb_color = is_selected ? IM_COL32(255, 210, 120, 240)
							: (is_hovered ? IM_COL32(255, 235, 160, 230) : IM_COL32(220, 200, 120, 210));
						const ImU32 base_color = is_selected ? IM_COL32(120, 110, 90, 230)
							: IM_COL32(90, 85, 70, 200);

						draw_list->AddCircleFilled(center, radius, bulb_color, 20);
						draw_list->AddCircle(center, radius, IM_COL32(30, 30, 30, 160), 20, 1.0f);
						draw_list->AddRectFilled(
							ImVec2(center.x - radius * 0.6f, center.y + radius * 0.4f),
							ImVec2(center.x + radius * 0.6f, center.y + radius * 1.2f),
							base_color,
							2.0f);
					}
				}

				if (hovered_scene_id >= 0)
				{
					const TreeNode& node = scene_nodes[hovered_scene_id];
					const NodeProperties& props = scene_props[hovered_scene_id];
					float pick_pos[3] = {props.position[0], props.position[1], props.position[2]};
					TryGetNodePickPosition(props, pick_pos);
					ImGui::BeginTooltip();
					ImGui::Text("%s", node.name.c_str());
					if (!props.type.empty())
					{
						ImGui::Text("Type: %s", props.type.c_str());
					}
					if (!props.class_name.empty())
					{
						ImGui::Text("Class: %s", props.class_name.c_str());
					}
					if (hovered_hit_valid)
					{
						ImGui::Text("Hit: %.1f %.1f %.1f", hovered_hit_pos.x, hovered_hit_pos.y, hovered_hit_pos.z);
					}
					ImGui::Text("Center: %.1f %.1f %.1f", pick_pos[0], pick_pos[1], pick_pos[2]);
					ImGui::EndTooltip();
				}
				DrawViewportOverlay(viewport_panel, ImGui::GetWindowDrawList(), viewport_pos, avail);
			}
	}
	ImGui::End();

	const NodeProperties* world_props = FindWorldProperties(scene_props);
	if (world_props && WorldSettingsDifferent(*world_props, world_settings_cache))
	{
		ApplyWorldSettingsToRenderer(*world_props);
		UpdateWorldSettingsCache(*world_props, world_settings_cache);
	}
	std::vector<DynamicLight> viewport_dynamic_lights;
	ApplyViewportDirectionalLight(diligent.engine, viewport_panel, scene_panel, scene_nodes, scene_props);
	BuildViewportDynamicLights(viewport_panel, scene_panel, scene_nodes, scene_props, viewport_dynamic_lights);
	ApplySceneVisibilityToRenderer(scene_panel, viewport_panel, scene_nodes, scene_props);
	RenderViewport(diligent, viewport_panel, world_props, overlay_state, viewport_dynamic_lights);

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
