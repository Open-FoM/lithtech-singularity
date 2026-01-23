#include "ui_properties.h"

#include "ui_shared.h"
#include "engine_render.h"
#include "diligent_drawprim_api.h"
#include "diligent_render_debug.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

namespace
{
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

void DrawTextureProperties(NodeProperties& props)
{
	ImGui::InputText("Texture", &props.resource);
	ImGui::Checkbox("sRGB", &props.srgb);
	ImGui::SameLine();
	ImGui::Checkbox("Mipmaps", &props.mipmaps);
	const char* compression_modes[] = {"None", "BC1", "BC3", "BC7"};
	ImGui::Combo("Compression", &props.compression_mode, compression_modes, 4);
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

void DrawProjectProperties(
	std::vector<TreeNode>& nodes,
	std::vector<NodeProperties>& props,
	int selected_id)
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
	ImGui::Checkbox("Locked", &node_props.locked);
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
		DrawTextureProperties(node_props);
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
	int selected_id)
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
	ImGui::Checkbox("Locked", &node_props.locked);
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
	int scene_selected_id)
{
	if (ImGui::Begin("Properties"))
	{
		if (active_target == SelectionTarget::Project)
		{
			DrawProjectProperties(project_nodes, project_props, project_selected_id);
		}
		else
		{
			DrawSceneProperties(scene_nodes, scene_props, scene_selected_id);
		}
	}
	ImGui::End();
}
