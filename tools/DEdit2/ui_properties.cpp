#include "ui_properties.h"

#include "ui_shared.h"
#include "engine_render.h"
#include "texture_effect_group.h"
#include "diligent_drawprim_api.h"
#include "diligent_render_debug.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <filesystem>

namespace
{
constexpr uint32_t kTextureScriptChannelBase = 1;
constexpr uint32_t kRoughnessSourceVarIndex = 0;
constexpr uint32_t kRoughnessValueVarIndex = 1;
constexpr uint32_t kRoughnessOverrideVarIndex = 2;

bool IsType(const NodeProperties& props, const char* type)
{
	return props.type == type;
}

void DrawTexturePreview(const char* texture_name, float max_size = 256.0f)
{
	if (!texture_name || texture_name[0] == '\0')
	{
		return;
	}

	TextureCache* cache = GetEngineTextureCache();
	if (!cache)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Texture cache unavailable");
		return;
	}

	SharedTexture* texture = cache->GetSharedTexture(texture_name);
	if (!texture)
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Texture not loaded: %s", texture_name);
		return;
	}

	Diligent::ITextureView* view = diligent_get_drawprim_texture_view(texture, false);
	if (!view)
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Texture view unavailable");
		return;
	}

	DiligentTextureDebugInfo info;
	if (!diligent_GetTextureDebugInfo(texture, info))
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Texture info unavailable");
		return;
	}

	if (info.width == 0 || info.height == 0)
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Invalid texture dimensions");
		return;
	}

	const float aspect = static_cast<float>(info.height) / static_cast<float>(info.width);
	ImVec2 size(max_size, max_size * aspect);
	if (size.y > max_size)
	{
		size.y = max_size;
		size.x = size.y / aspect;
	}

	ImGui::Image(reinterpret_cast<ImTextureID>(view), size);
	ImGui::Text("%s (%ux%u)", texture_name, info.width, info.height);
}

enum class RoughnessSource
{
	Default,
	BaseAlpha,
	Constant
};

struct TextureRoughnessUiState
{
	std::string texture_resource;
	std::string effect_group_name;
	std::string effect_group_path;
	RoughnessSource source = RoughnessSource::Default;
	float constant = 0.5f;
	std::string status;
};

std::string BuildEffectGroupName(const std::string& texture_resource)
{
	if (texture_resource.empty())
	{
		return {};
	}

	std::filesystem::path path(texture_resource);
	if (path.has_extension())
	{
		path.replace_extension();
	}
	return path.generic_string();
}

std::string BuildEffectGroupPath(const std::string& project_root, const std::string& effect_group_name)
{
	if (project_root.empty() || effect_group_name.empty())
	{
		return {};
	}
	std::filesystem::path root(project_root);
	std::filesystem::path rel(effect_group_name + ".tfg");
	return (root / "TextureEffectGroups" / rel).string();
}

bool LoadRoughnessFromGroup(const std::string& path, TextureRoughnessUiState& state)
{
	state.status.clear();
	TextureEffectGroup group{};
	std::string error;
	if (!LoadTextureEffectGroup(path, group, error))
	{
		if (!error.empty())
		{
			state.status = error;
		}
		return false;
	}

	const TextureEffectGroupStage* base_stage = nullptr;
	for (const auto& stage : group.stages)
	{
		if (stage.enabled && stage.channel == kTextureScriptChannelBase)
		{
			base_stage = &stage;
			break;
		}
	}
	if (!base_stage)
	{
		return false;
	}

	const float use_alpha = base_stage->defaults[kRoughnessSourceVarIndex];
	const float use_constant = base_stage->defaults[kRoughnessOverrideVarIndex];
	if (use_alpha > 0.5f)
	{
		state.source = RoughnessSource::BaseAlpha;
	}
	else if (use_constant > 0.5f)
	{
		state.source = RoughnessSource::Constant;
		state.constant = std::clamp(base_stage->defaults[kRoughnessValueVarIndex], 0.0f, 1.0f);
	}
	else
	{
		state.source = RoughnessSource::Default;
	}

	return true;
}

bool ApplyRoughnessToGroup(const std::string& path, RoughnessSource source, float constant, std::string& out_status)
{
	out_status.clear();
	TextureEffectGroup group{};
	std::string error;
	const bool loaded = LoadTextureEffectGroup(path, group, error);
	if (!loaded)
	{
		group = TextureEffectGroup{};
	}

	TextureEffectGroupStage* base_stage = nullptr;
	for (auto& stage : group.stages)
	{
		if (stage.enabled && stage.channel == kTextureScriptChannelBase)
		{
			base_stage = &stage;
			break;
		}
	}

	if (!base_stage)
	{
		base_stage = &group.stages[0];
		base_stage->enabled = true;
		base_stage->overridden = false;
		base_stage->channel = kTextureScriptChannelBase;
		base_stage->script = "Identity";
		base_stage->defaults.fill(0.0f);
	}
	else if (base_stage->overridden && base_stage->override_stage < group.stages.size())
	{
		base_stage = &group.stages[base_stage->override_stage];
		base_stage->enabled = true;
		base_stage->overridden = false;
		base_stage->channel = kTextureScriptChannelBase;
		if (base_stage->script.empty())
		{
			base_stage->script = "Identity";
		}
	}

	base_stage->defaults[kRoughnessSourceVarIndex] = (source == RoughnessSource::BaseAlpha) ? 1.0f : 0.0f;
	base_stage->defaults[kRoughnessOverrideVarIndex] = (source == RoughnessSource::Constant) ? 1.0f : 0.0f;
	base_stage->defaults[kRoughnessValueVarIndex] = std::clamp(constant, 0.0f, 1.0f);

	if (!SaveTextureEffectGroup(path, group, error))
	{
		out_status = error.empty() ? "Failed to save effect group." : error;
		return false;
	}

	out_status = "Saved.";
	return true;
}

void DrawTextureProperties(NodeProperties& props, const std::string& project_root)
{
	ImGui::InputText("Texture", &props.resource);
	ImGui::Checkbox("sRGB", &props.srgb);
	ImGui::SameLine();
	ImGui::Checkbox("Mipmaps", &props.mipmaps);
	const char* compression_modes[] = {"None", "BC1", "BC3", "BC7"};
	ImGui::Combo("Compression", &props.compression_mode, compression_modes, 4);

	if (ImGui::CollapsingHeader("PBR Roughness", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static TextureRoughnessUiState roughness_state{};
		const std::string effect_group_name = BuildEffectGroupName(props.resource);
		const std::string effect_group_path = BuildEffectGroupPath(project_root, effect_group_name);
		const bool texture_changed = (roughness_state.texture_resource != props.resource) ||
			(roughness_state.effect_group_path != effect_group_path);

		if (texture_changed)
		{
			roughness_state.texture_resource = props.resource;
			roughness_state.effect_group_name = effect_group_name;
			roughness_state.effect_group_path = effect_group_path;
			roughness_state.source = RoughnessSource::Default;
			roughness_state.constant = 0.5f;
			roughness_state.status.clear();
			if (!effect_group_path.empty() && std::filesystem::exists(effect_group_path))
			{
				LoadRoughnessFromGroup(effect_group_path, roughness_state);
			}
		}

		ImGui::TextUnformatted("Effect Group");
		ImGui::TextWrapped("%s", effect_group_name.empty() ? "(none)" : effect_group_name.c_str());
		if (project_root.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Project root not set.");
		}

		const char* sources[] = {"Default", "Base Alpha", "Constant"};
		int source_index = static_cast<int>(roughness_state.source);
		if (ImGui::Combo("Roughness Source", &source_index, sources, 3))
		{
			roughness_state.source = static_cast<RoughnessSource>(source_index);
			if (!effect_group_path.empty())
			{
				ApplyRoughnessToGroup(effect_group_path, roughness_state.source, roughness_state.constant, roughness_state.status);
			}
		}

		const bool enable_constant = (roughness_state.source == RoughnessSource::Constant);
		ImGui::BeginDisabled(!enable_constant);
		if (ImGui::SliderFloat("Constant Roughness", &roughness_state.constant, 0.0f, 1.0f))
		{
			if (enable_constant && !effect_group_path.empty())
			{
				ApplyRoughnessToGroup(effect_group_path, roughness_state.source, roughness_state.constant, roughness_state.status);
			}
		}
		ImGui::EndDisabled();

		if (!roughness_state.status.empty())
		{
			ImGui::TextUnformatted(roughness_state.status.c_str());
		}
	}
}

void DrawModelProperties(NodeProperties& props)
{
	ImGui::InputText("Model", &props.resource);
	ImGui::DragFloat("Scale", &props.model_scale, 0.01f, 0.01f, 100.0f);
	ImGui::Checkbox("Cast Shadows", &props.cast_shadows);
}

void DrawSoundProperties(NodeProperties& props)
{
	ImGui::InputText("Sound", &props.resource);
	ImGui::SliderFloat("Volume", &props.volume, 0.0f, 1.0f);
	ImGui::Checkbox("Loop", &props.loop);
}

void DrawPrefabProperties(NodeProperties& props)
{
	ImGui::InputText("Prefab", &props.resource);
}

void DrawWorldProperties(NodeProperties& props)
{
	ImGui::DragFloat("Gravity", &props.gravity, 0.1f, -100.0f, 0.0f);
	ImGui::DragFloat("Far Z", &props.far_z, 10.0f, 1.0f, 1000000.0f);
	ImGui::ColorEdit3("Background", props.background_color);
	ImGui::Checkbox("Fog Enabled", &props.fog_enabled);
	ImGui::BeginDisabled(!props.fog_enabled);
	ImGui::ColorEdit3("Fog Color", props.color);
	ImGui::DragFloat("Fog Near", &props.fog_near, 1.0f, 0.0f, props.fog_far);
	ImGui::DragFloat("Fog Far", &props.fog_far, 1.0f, props.fog_near, props.far_z);
	ImGui::DragFloat("Fog Density", &props.fog_density, 0.0005f, 0.0f, 1.0f, "%.4f");
	ImGui::EndDisabled();

	if (ImGui::CollapsingHeader("Sky", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Sky Pan Enabled", &props.sky_pan_enabled);
		ImGui::InputText("Sky Pan Texture", &props.sky_pan_texture);
		if (!props.sky_pan_texture.empty())
		{
			DrawTexturePreview(props.sky_pan_texture.c_str());
		}
		ImGui::DragFloat2("Sky Pan Scale", props.sky_pan_scale, 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat2("Sky Pan AutoPan", props.sky_pan_auto_pan, 0.01f, -1000.0f, 1000.0f);
	}
}

void DrawLightProperties(NodeProperties& props)
{
	ImGui::ColorEdit3("Color", props.color);
	ImGui::DragFloat("Intensity", &props.intensity, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Range", &props.range, 0.5f, 0.0f, 500.0f);
	ImGui::Checkbox("Cast Shadows", &props.cast_shadows);
	ImGui::Checkbox("Use Temperature", &props.use_temperature);
	if (props.use_temperature)
	{
		ImGui::DragFloat("Temperature (K)", &props.temperature, 10.0f, 1000.0f, 20000.0f);
	}
}

void DrawPropProperties(NodeProperties& props)
{
	ImGui::InputText("Model", &props.resource);
	ImGui::Checkbox("Cast Shadows", &props.cast_shadows);
	ImGui::DragFloat("Scale", &props.model_scale, 0.01f, 0.01f, 100.0f);
}

void DrawRoomProperties(NodeProperties& props)
{
	ImGui::DragFloat3("Size", props.size, 0.1f, 0.0f, 1000.0f);
	ImGui::ColorEdit3("Ambient", props.ambient);
}

void DrawTerrainProperties(NodeProperties& props)
{
	ImGui::DragFloat3("Size", props.size, 1.0f, 0.0f, 10000.0f);
	ImGui::DragFloat("Height Scale", &props.height_scale, 0.1f, 0.0f, 1000.0f);
}

void DrawEntityProperties(NodeProperties& props)
{
	ImGui::InputText("Class", &props.class_name);
}

void DrawBrushProperties(NodeProperties& props, const std::string& project_root, bool* open_texture_browser)
{
	const size_t face_count = props.brush_face_textures.size();
	ImGui::Text("Faces: %zu", face_count);

	// Button to open texture browser
	if (ImGui::Button("Open Texture Browser") && open_texture_browser != nullptr)
	{
		*open_texture_browser = true;
	}

	if (face_count == 0)
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No face data available.");
		return;
	}

	ImGui::Separator();

	// Track if any changes made for marking dirty
	bool modified = false;

	// Quick buttons for all faces
	if (ImGui::Button("Reset All UVs"))
	{
		for (auto& face : props.brush_face_textures)
		{
			face.mapping = texture_ops::TextureMapping();
		}
		modified = true;
	}

	ImGui::Separator();

	// Draw each face in a collapsible section
	for (size_t i = 0; i < face_count; ++i)
	{
		BrushFaceTextureData& face = props.brush_face_textures[i];
		char header[64];
		snprintf(header, sizeof(header), "Face %zu: %s###face_%zu",
			i, face.texture_name.empty() ? "(no texture)" : face.texture_name.c_str(), i);

		if (ImGui::CollapsingHeader(header))
		{
			ImGui::PushID(static_cast<int>(i));

			// Texture name
			if (ImGui::InputText("Texture", &face.texture_name))
			{
				modified = true;
			}

			// Texture preview (small)
			if (!face.texture_name.empty())
			{
				DrawTexturePreview(face.texture_name.c_str(), 128.0f);
			}

			// UV Transform section
			if (ImGui::TreeNodeEx("UV Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::DragFloat("Offset U", &face.mapping.offset_u, 0.01f))
				{
					modified = true;
				}
				if (ImGui::DragFloat("Offset V", &face.mapping.offset_v, 0.01f))
				{
					modified = true;
				}
				if (ImGui::DragFloat("Scale U", &face.mapping.scale_u, 0.01f, 0.001f, 100.0f))
				{
					modified = true;
				}
				if (ImGui::DragFloat("Scale V", &face.mapping.scale_v, 0.01f, 0.001f, 100.0f))
				{
					modified = true;
				}
				if (ImGui::DragFloat("Rotation", &face.mapping.rotation, 1.0f, -360.0f, 360.0f, "%.1f deg"))
				{
					modified = true;
				}

				if (ImGui::Button("Reset##uv"))
				{
					face.mapping = texture_ops::TextureMapping();
					modified = true;
				}
				ImGui::TreePop();
			}

			// Surface Flags section
			if (ImGui::TreeNodeEx("Surface Flags", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto flags = static_cast<texture_ops::SurfaceFlags>(face.surface_flags);

				bool solid = texture_ops::HasFlag(flags, texture_ops::SurfaceFlags::Solid);
				if (ImGui::Checkbox("Solid", &solid))
				{
					if (solid)
						texture_ops::SetFlag(flags, texture_ops::SurfaceFlags::Solid);
					else
						texture_ops::ClearFlag(flags, texture_ops::SurfaceFlags::Solid);
					face.surface_flags = static_cast<uint32_t>(flags);
					modified = true;
				}

				bool invisible = texture_ops::HasFlag(flags, texture_ops::SurfaceFlags::Invisible);
				if (ImGui::Checkbox("Invisible", &invisible))
				{
					if (invisible)
						texture_ops::SetFlag(flags, texture_ops::SurfaceFlags::Invisible);
					else
						texture_ops::ClearFlag(flags, texture_ops::SurfaceFlags::Invisible);
					face.surface_flags = static_cast<uint32_t>(flags);
					modified = true;
				}

				bool fullbright = texture_ops::HasFlag(flags, texture_ops::SurfaceFlags::Fullbright);
				if (ImGui::Checkbox("Fullbright", &fullbright))
				{
					if (fullbright)
						texture_ops::SetFlag(flags, texture_ops::SurfaceFlags::Fullbright);
					else
						texture_ops::ClearFlag(flags, texture_ops::SurfaceFlags::Fullbright);
					face.surface_flags = static_cast<uint32_t>(flags);
					modified = true;
				}

				bool sky = texture_ops::HasFlag(flags, texture_ops::SurfaceFlags::Sky);
				if (ImGui::Checkbox("Sky", &sky))
				{
					if (sky)
						texture_ops::SetFlag(flags, texture_ops::SurfaceFlags::Sky);
					else
						texture_ops::ClearFlag(flags, texture_ops::SurfaceFlags::Sky);
					face.surface_flags = static_cast<uint32_t>(flags);
					modified = true;
				}

				bool portal = texture_ops::HasFlag(flags, texture_ops::SurfaceFlags::Portal);
				if (ImGui::Checkbox("Portal", &portal))
				{
					if (portal)
						texture_ops::SetFlag(flags, texture_ops::SurfaceFlags::Portal);
					else
						texture_ops::ClearFlag(flags, texture_ops::SurfaceFlags::Portal);
					face.surface_flags = static_cast<uint32_t>(flags);
					modified = true;
				}

				bool mirror = texture_ops::HasFlag(flags, texture_ops::SurfaceFlags::Mirror);
				if (ImGui::Checkbox("Mirror", &mirror))
				{
					if (mirror)
						texture_ops::SetFlag(flags, texture_ops::SurfaceFlags::Mirror);
					else
						texture_ops::ClearFlag(flags, texture_ops::SurfaceFlags::Mirror);
					face.surface_flags = static_cast<uint32_t>(flags);
					modified = true;
				}

				bool transparent = texture_ops::HasFlag(flags, texture_ops::SurfaceFlags::Transparent);
				if (ImGui::Checkbox("Transparent", &transparent))
				{
					if (transparent)
						texture_ops::SetFlag(flags, texture_ops::SurfaceFlags::Transparent);
					else
						texture_ops::ClearFlag(flags, texture_ops::SurfaceFlags::Transparent);
					face.surface_flags = static_cast<uint32_t>(flags);
					modified = true;
				}

				// Alpha reference slider (0-255)
				int alpha = face.alpha_ref;
				if (ImGui::SliderInt("Alpha Ref", &alpha, 0, 255))
				{
					face.alpha_ref = static_cast<uint8_t>(alpha);
					modified = true;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			ImGui::Spacing();
		}
	}

	// Note: modified flag could be used to mark document dirty in caller
	(void)modified;
}

void DrawProjectProperties(
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int selected_id,
	const std::string& project_root)
{
	if (selected_id < 0 || selected_id >= static_cast<int>(nodes.size()) || nodes[selected_id].deleted)
	{
		ImGui::TextUnformatted("No project item selected.");
		return;
	}

	TreeNode& node = nodes[selected_id];
	NodeProperties& node_props = props[selected_id];
	const bool is_folder = node.is_folder || IsType(node_props, "Folder") || IsType(node_props, "Project");
	const bool empty_name = IsNameEmptyOrWhitespace(node.name);
	const bool duplicate_name = HasDuplicateSiblingName(nodes, 0, selected_id, node.name);

	ImGui::TextUnformatted("Project Item");
	ImGui::Separator();
	ImGui::InputText("Name", &node.name);
	if (empty_name)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Name cannot be empty.");
	}
	if (duplicate_name)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Name must be unique among siblings.");
	}

	ImGui::BeginDisabled(is_folder);
	ImGui::InputText("Type", &node_props.type);
	ImGui::EndDisabled();
	ImGui::TextUnformatted("Path");
	ImGui::TextWrapped("%s", BuildNodePath(nodes, 0, selected_id).c_str());
	ImGui::Spacing();
	ImGui::Checkbox("Frozen", &node_props.frozen);
	ImGui::BeginDisabled();
	ImGui::Checkbox("Visible", &node_props.visible);
	ImGui::EndDisabled();
	ImGui::Separator();
	if (is_folder)
	{
		ImGui::TextUnformatted("Folder items do not have asset properties.");
		return;
	}

	if (IsType(node_props, "Texture"))
	{
		DrawTextureProperties(node_props, project_root);
	}
	else if (IsType(node_props, "Model"))
	{
		DrawModelProperties(node_props);
	}
	else if (IsType(node_props, "Sound"))
	{
		DrawSoundProperties(node_props);
	}
	else if (IsType(node_props, "Prefab"))
	{
		DrawPrefabProperties(node_props);
	}
	else if (IsType(node_props, "World"))
	{
		DrawWorldProperties(node_props);
	}
	else if (IsType(node_props, "Asset"))
	{
		ImGui::InputText("Resource", &node_props.resource);
	}
}

void DrawSceneProperties(
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int selected_id,
	bool* open_texture_browser)
{
	if (selected_id < 0 || selected_id >= static_cast<int>(nodes.size()) || nodes[selected_id].deleted)
	{
		ImGui::TextUnformatted("No scene object selected.");
		return;
	}

	TreeNode& node = nodes[selected_id];
	NodeProperties& node_props = props[selected_id];
	const bool empty_name = IsNameEmptyOrWhitespace(node.name);

	ImGui::TextUnformatted("Scene Object");
	ImGui::Separator();
	ImGui::InputText("Name", &node.name);
	if (empty_name)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Name cannot be empty.");
	}
	ImGui::InputText("Type", &node_props.type);
	ImGui::TextUnformatted("Path");
	ImGui::TextWrapped("%s", BuildNodePath(nodes, 0, selected_id).c_str());
	ImGui::Checkbox("Visible", &node_props.visible);
	ImGui::SameLine();
	ImGui::Checkbox("Frozen", &node_props.frozen);
	ImGui::Spacing();
	ImGui::TextUnformatted("Transform");
	ImGui::DragFloat3("Position", node_props.position, 0.1f);
	ImGui::DragFloat3("Rotation", node_props.rotation, 1.0f);
	ImGui::DragFloat3("Scale", node_props.scale, 0.01f, 0.01f, 100.0f);
	ImGui::Separator();
	if (IsType(node_props, "Light"))
	{
		DrawLightProperties(node_props);
	}
	else if (IsType(node_props, "Prop"))
	{
		DrawPropProperties(node_props);
	}
	else if (IsType(node_props, "Room"))
	{
		DrawRoomProperties(node_props);
	}
	else if (IsType(node_props, "Terrain"))
	{
		DrawTerrainProperties(node_props);
	}
	else if (IsType(node_props, "World"))
	{
		DrawWorldProperties(node_props);
	}
	else if (IsType(node_props, "Entity"))
	{
		DrawEntityProperties(node_props);
	}
	else if (IsType(node_props, "Brush"))
	{
		DrawBrushProperties(node_props, "", open_texture_browser);
	}
	else if (IsType(node_props, "Surface"))
	{
		ImGui::TextUnformatted("Material");
		ImGui::TextWrapped("%s", node_props.resource.c_str());
		DrawTexturePreview(node_props.resource.c_str());
	}

	if (ImGui::CollapsingHeader("Raw Properties"))
	{
		ImGui::BeginChild("RawProps", ImVec2(0.0f, 160.0f), true);
		if (node_props.raw_properties.empty())
		{
			ImGui::TextUnformatted("No raw properties available.");
		}
		else
		{
			for (const auto& entry : node_props.raw_properties)
			{
				ImGui::Text("%s: %s", entry.first.c_str(), entry.second.c_str());
			}
		}
		ImGui::EndChild();
	}
}
} // namespace

void DrawPropertiesPanel(
	SelectionTarget active_target,
	std::vector<TreeNode>& project_nodes,
	std::vector<NodeProperties>& project_props,
	int project_selected_id,
	std::vector<TreeNode>& scene_nodes,
	std::vector<NodeProperties>& scene_props,
	int scene_selected_id,
	const std::string& project_root,
	bool* open_texture_browser)
{
	if (ImGui::Begin("Properties"))
	{
		if (active_target == SelectionTarget::Project)
		{
			DrawProjectProperties(project_nodes, project_props, project_selected_id, project_root);
		}
		else
		{
			DrawSceneProperties(scene_nodes, scene_props, scene_selected_id, open_texture_browser);
		}
	}
	ImGui::End();
}
