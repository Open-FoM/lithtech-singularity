#include "dedit2_concommand.h"

#include "engine_render.h"
#include "diligent_drawprim_api.h"
#include "diligent_render_debug.h"
#include "rendererconsolevars.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

static DEdit2TexViewState g_TexView;
extern int32 g_CV_TextureMipMapOffset;

namespace
{
struct ConsoleVar
{
	enum class Type
	{
		Int,
		Float,
		String
	};

	std::string name;
	Type type = Type::String;
	void* target = nullptr;
	std::string value;
};

using CommandFn = void (*)(int argc, const char* argv[]);

struct ConsoleCommand
{
	std::string name;
	CommandFn fn = nullptr;
};

bool ParseInt(const char* text, int& out);
bool ParseFloat(const char* text, float& out);

std::unordered_map<std::string, ConsoleVar> g_Vars;
std::unordered_map<std::string, ConsoleCommand> g_Commands;
std::vector<std::string> g_CommandOrder;
std::vector<std::string> g_Log;
DEdit2ConsoleBindings g_Bindings;

std::string ToLower(const std::string& value)
{
	std::string out = value;
	for (char& ch : out)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return out;
}

bool StringEqualsIgnoreCase(const char* left, const char* right)
{
	if (!left || !right)
	{
		return false;
	}
	while (*left != '\0' && *right != '\0')
	{
		const unsigned char lch = static_cast<unsigned char>(*left++);
		const unsigned char rch = static_cast<unsigned char>(*right++);
		if (std::tolower(lch) != std::tolower(rch))
		{
			return false;
		}
	}
	return *left == *right;
}

const char* BppToString(BPPIdent bpp)
{
	switch (bpp)
	{
		case BPP_8P:
			return "BPP_8P";
		case BPP_8:
			return "BPP_8";
		case BPP_16:
			return "BPP_16";
		case BPP_24:
			return "BPP_24";
		case BPP_32:
			return "BPP_32";
		case BPP_32P:
			return "BPP_32P";
		case BPP_S3TC_DXT1:
			return "BPP_S3TC_DXT1";
		case BPP_S3TC_DXT3:
			return "BPP_S3TC_DXT3";
		case BPP_S3TC_DXT5:
			return "BPP_S3TC_DXT5";
		default:
			return "BPP_UNKNOWN";
	}
}

bool IsLightClassName(const std::string& value)
{
	const std::string lower = ToLower(value);
	return lower == "light" ||
		lower == "dirlight" ||
		lower == "directionallight" ||
		lower == "spotlight" ||
		lower == "pointlight";
}

std::vector<std::string> Tokenize(const char* command)
{
	std::vector<std::string> tokens;
	if (!command)
	{
		return tokens;
	}

	std::string current;
	bool in_quotes = false;
	bool escaped = false;
	for (const char* ptr = command; *ptr; ++ptr)
	{
		char ch = *ptr;
		if (escaped)
		{
			current.push_back(ch);
			escaped = false;
			continue;
		}

		if (in_quotes && ch == '\\')
		{
			escaped = true;
			continue;
		}

		if (ch == '"')
		{
			in_quotes = !in_quotes;
			continue;
		}

		if (!in_quotes && std::isspace(static_cast<unsigned char>(ch)))
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
			continue;
		}

		current.push_back(ch);
	}

	if (!current.empty())
	{
		tokens.push_back(current);
	}

	return tokens;
}

void AddCommand(const char* name, CommandFn fn)
{
	if (!name || !fn)
	{
		return;
	}
	const std::string key = ToLower(name);
	ConsoleCommand cmd{std::string(name), fn};
	g_Commands[key] = cmd;
	g_CommandOrder.push_back(cmd.name);
}

void AddVar(const char* name, ConsoleVar::Type type, void* target)
{
	if (!name)
	{
		return;
	}
	const std::string key = ToLower(name);
	ConsoleVar var;
	var.name = name;
	var.type = type;
	var.target = target;
	g_Vars[key] = var;
}

void CmdTexProbe(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: texprobe <texture_name>");
		return;
	}

	TextureCache* cache = GetEngineTextureCache();
	if (!cache)
	{
		DEdit2_Log("Texture cache not initialized.");
		return;
	}

	const char* name = argv[0];
	TextureCache::TextureDebugInfo info;
	if (!cache->GetTextureDebugInfo(name, info))
	{
		DEdit2_Log("Texture '%s' not loaded.", name);
		return;
	}

	DEdit2_Log("Texture probe:");
	DEdit2_Log(" Name: %s", info.name.c_str());
	DEdit2_Log(" Path: %s", info.path.empty() ? "<none>" : info.path.c_str());
	DEdit2_Log(" Size: %ux%u Mips: %u BPP: %s Buffer: %u HasData: %s Flags: 0x%08X User: 0x%08X Sections: %u",
		info.width,
		info.height,
		info.mipmaps,
		BppToString(info.bpp),
		info.buffer_size,
		info.has_data ? "yes" : "no",
		info.flags,
		info.user_flags,
		info.sections);

	SharedTexture* texture = cache->GetSharedTexture(name);
	DiligentTextureDebugInfo gpu_info;
	if (texture && diligent_GetTextureDebugInfo(texture, gpu_info))
	{
		DEdit2_Log(" GPU: %s Converted: %s Size: %ux%u Format: %u",
			gpu_info.has_render_texture ? "yes" : "no",
			gpu_info.used_conversion ? "yes" : "no",
			gpu_info.width,
			gpu_info.height,
			gpu_info.format);
		if (gpu_info.has_cpu_pixel)
		{
			const uint32 pixel = gpu_info.first_pixel;
			const uint8 a = static_cast<uint8>((pixel >> 24) & 0xFF);
			const uint8 r = static_cast<uint8>((pixel >> 16) & 0xFF);
			const uint8 g = static_cast<uint8>((pixel >> 8) & 0xFF);
			const uint8 b = static_cast<uint8>(pixel & 0xFF);
			DEdit2_Log(" CPU pixel[0]: 0x%08X (A=%u R=%u G=%u B=%u)", pixel, a, r, g, b);
		}
	}
	else
	{
		DEdit2_Log(" GPU: unavailable");
	}

	if (info.bpp == BPP_32)
	{
		SharedTexture* shared = cache->GetSharedTexture(name);
		TextureData* data = shared ? cache->GetTexture(shared) : nullptr;
		if (data && data->m_Mips[0].m_Data)
		{
			DEdit2_Log(" Mip0: size=%ux%u pitch=%d dataSize=%d",
				data->m_Mips[0].m_Width,
				data->m_Mips[0].m_Height,
				data->m_Mips[0].m_Pitch,
				data->m_Mips[0].m_dataSize);
			DEdit2_Log(" Masks: A=0x%08X R=0x%08X G=0x%08X B=0x%08X",
				data->m_PFormat.m_Masks[0],
				data->m_PFormat.m_Masks[1],
				data->m_PFormat.m_Masks[2],
				data->m_PFormat.m_Masks[3]);
			const uint32 argb = *reinterpret_cast<const uint32*>(data->m_Mips[0].m_Data);
			const uint8 a = static_cast<uint8>((argb >> 24) & 0xFF);
			const uint8 r = static_cast<uint8>((argb >> 16) & 0xFF);
			const uint8 g = static_cast<uint8>((argb >> 8) & 0xFF);
			const uint8 b = static_cast<uint8>(argb & 0xFF);
			DEdit2_Log(" CPU pixel[0] (ARGB): 0x%08X (A=%u R=%u G=%u B=%u)", argb, a, r, g, b);

			const uint32 bgra = (a << 24) | (b << 16) | (g << 8) | r;
			const uint8 ba = static_cast<uint8>((bgra >> 24) & 0xFF);
			const uint8 br = static_cast<uint8>((bgra >> 16) & 0xFF);
			const uint8 bg = static_cast<uint8>((bgra >> 8) & 0xFF);
			const uint8 bb = static_cast<uint8>(bgra & 0xFF);
			DEdit2_Log(" CPU pixel[0] (BGRA): 0x%08X (A=%u R=%u G=%u B=%u)", bgra, ba, br, bg, bb);
		}
	}
}

void CmdWorldColorStats(int, const char*[])
{
	DiligentWorldColorStats stats;
	if (!diligent_GetWorldColorStats(stats))
	{
		DEdit2_Log("World color stats unavailable.");
		return;
	}

	DEdit2_Log("World vertex colors: count=%llu zero=%llu min=(%u,%u,%u,%u) max=(%u,%u,%u,%u)",
		static_cast<unsigned long long>(stats.vertex_count),
		static_cast<unsigned long long>(stats.zero_color_count),
		stats.min_a, stats.min_r, stats.min_g, stats.min_b,
		stats.max_a, stats.max_r, stats.max_g, stats.max_b);
}

void CmdWorldTextureStats(int, const char*[])
{
	DiligentWorldTextureStats stats;
	if (!diligent_GetWorldTextureStats(stats))
	{
		DEdit2_Log("World texture stats unavailable.");
		return;
	}

	DEdit2_Log("World textures: sections=%llu tex0=%llu view0=%llu tex1=%llu view1=%llu lm=%llu lmview=%llu",
		static_cast<unsigned long long>(stats.section_count),
		static_cast<unsigned long long>(stats.texture0_present),
		static_cast<unsigned long long>(stats.texture0_view),
		static_cast<unsigned long long>(stats.texture1_present),
		static_cast<unsigned long long>(stats.texture1_view),
		static_cast<unsigned long long>(stats.lightmap_present),
		static_cast<unsigned long long>(stats.lightmap_view));
}

void CmdWorldUvStats(int, const char*[])
{
	DiligentWorldUvStats stats;
	if (!diligent_GetWorldUvStats(stats) || !stats.has_range)
	{
		DEdit2_Log("World UV stats unavailable.");
		return;
	}

	DEdit2_Log("World UVs: count=%llu nan0=%llu nan1=%llu",
		static_cast<unsigned long long>(stats.vertex_count),
		static_cast<unsigned long long>(stats.nan_uv0),
		static_cast<unsigned long long>(stats.nan_uv1));
	DEdit2_Log(" UV0: min=(%.3f, %.3f) max=(%.3f, %.3f)",
		stats.min_u0, stats.min_v0, stats.max_u0, stats.max_v0);
	DEdit2_Log(" UV1: min=(%.3f, %.3f) max=(%.3f, %.3f)",
		stats.min_u1, stats.min_v1, stats.max_u1, stats.max_v1);
}

void CmdWorldUvDebug(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: worlduvdebug <0|1>");
		return;
	}

	int enabled = 0;
	if (!ParseInt(argv[0], enabled))
	{
		DEdit2_Log("Usage: worlduvdebug <0|1>");
		return;
	}

	g_CV_WorldUvDebug.m_Val = enabled != 0 ? 1 : 0;
	DEdit2_Log("worlduvdebug = %d", g_CV_WorldUvDebug.m_Val);
}

void CmdWorldTexturedDebug(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: worldtextureddebug <0|1>");
		return;
	}

	int enabled = 0;
	if (!ParseInt(argv[0], enabled))
	{
		DEdit2_Log("Usage: worldtextureddebug <0|1>");
		return;
	}

	g_CV_WorldUvDebug.m_Val = enabled != 0 ? 1 : 0;
	DEdit2_Log("worldtextureddebug = %d", g_CV_WorldUvDebug.m_Val);
}

void CmdWorldPipelineStats(int, const char*[])
{
	DiligentWorldPipelineStats stats;
	if (!diligent_GetWorldPipelineStats(stats))
	{
		DEdit2_Log("World pipeline stats unavailable.");
		return;
	}

	DEdit2_Log("World pipelines: total=%llu lm_data=%llu lm_view=%llu lm_black=%llu light_anim=%llu skip=%llu solid=%llu tex=%llu lm=%llu lm_only=%llu dual=%llu lm_dual=%llu bump=%llu glow=%llu dyn=%llu vol=%llu shadow=%llu flat=%llu normals=%llu",
		static_cast<unsigned long long>(stats.total_sections),
		static_cast<unsigned long long>(stats.sections_lightmap_data),
		static_cast<unsigned long long>(stats.sections_lightmap_view),
		static_cast<unsigned long long>(stats.sections_lightmap_black),
		static_cast<unsigned long long>(stats.sections_light_anim),
		static_cast<unsigned long long>(stats.mode_skip),
		static_cast<unsigned long long>(stats.mode_solid),
		static_cast<unsigned long long>(stats.mode_textured),
		static_cast<unsigned long long>(stats.mode_lightmap),
		static_cast<unsigned long long>(stats.mode_lightmap_only),
		static_cast<unsigned long long>(stats.mode_dual),
		static_cast<unsigned long long>(stats.mode_lightmap_dual),
		static_cast<unsigned long long>(stats.mode_bump),
		static_cast<unsigned long long>(stats.mode_glow),
		static_cast<unsigned long long>(stats.mode_dynamic_light),
		static_cast<unsigned long long>(stats.mode_volume),
		static_cast<unsigned long long>(stats.mode_shadow_project),
		static_cast<unsigned long long>(stats.mode_flat),
		static_cast<unsigned long long>(stats.mode_normals));
}

void CmdWorldPipelineReset(int, const char*[])
{
	diligent_ResetWorldPipelineStats();
	DEdit2_Log("World pipeline stats reset.");
}

void CmdWorldPsDebug(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: worldpsdebug <0|1|2>");
		return;
	}

	int enabled = 0;
	if (!ParseInt(argv[0], enabled))
	{
		DEdit2_Log("Usage: worldpsdebug <0|1|2>");
		return;
	}

	g_CV_WorldPsDebug.m_Val = enabled;
	DEdit2_Log("worldpsdebug = %d", g_CV_WorldPsDebug.m_Val);
}

void CmdWorldTexDump(int argc, const char* argv[])
{
	int limit = 10;
	if (argc >= 1)
	{
		if (!ParseInt(argv[0], limit) || limit < 0)
		{
			DEdit2_Log("Usage: worldtexdump [limit]");
			return;
		}
	}
	diligent_DumpWorldTextureBindings(static_cast<uint32_t>(limit));
}

void CmdWorldSurfaceDump(int argc, const char* argv[])
{
	int limit = 1;
	if (argc >= 1)
	{
		if (!ParseInt(argv[0], limit) || limit < 0)
		{
			DEdit2_Log("Usage: worldsurfacedump [limit]");
			return;
		}
	}
	diligent_DumpWorldSurfaceDebug(static_cast<uint32_t>(limit));
}

void CmdWorldBaseVertex(int argc, const char* argv[])
{
	int enabled = g_CV_WorldUseBaseVertex.m_Val;
	if (argc >= 1)
	{
		int parsed = 0;
		if (!ParseInt(argv[0], parsed) || (parsed != 0 && parsed != 1))
		{
			DEdit2_Log("Usage: worldbasevertex <0|1>");
			return;
		}
		enabled = parsed;
	}
	else
	{
		enabled = enabled ? 0 : 1;
	}

	g_CV_WorldUseBaseVertex.m_Val = enabled;
	DEdit2_Log("worldbasevertex = %d", enabled);
}

void CmdWorldShaderReset(int, const char*[])
{
	diligent_ResetWorldShaders();
	DEdit2_Log("World shaders reset.");
}

void CmdFogDebug(int, const char*[])
{
	DEdit2_Log("Fog: enabled=%d near=%.2f far=%.2f color=(%d,%d,%d)",
		g_CV_FogEnable.m_Val != 0 ? 1 : 0,
		g_CV_FogNearZ.m_Val,
		g_CV_FogFarZ.m_Val,
		g_CV_FogColorR.m_Val,
		g_CV_FogColorG.m_Val,
		g_CV_FogColorB.m_Val);
}

void CmdFogEnable(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: fogenable <0|1>");
		return;
	}

	int enabled = 0;
	if (!ParseInt(argv[0], enabled))
	{
		DEdit2_Log("Usage: fogenable <0|1>");
		return;
	}

	g_CV_FogEnable.m_Val = enabled != 0 ? 1 : 0;
	DEdit2_Log("fogenable = %d", g_CV_FogEnable.m_Val != 0 ? 1 : 0);
}

void CmdFogRange(int argc, const char* argv[])
{
	if (argc < 2)
	{
		DEdit2_Log("Usage: fogrange <near> <far>");
		return;
	}

	float near_z = 0.0f;
	float far_z = 0.0f;
	if (!ParseFloat(argv[0], near_z) || !ParseFloat(argv[1], far_z))
	{
		DEdit2_Log("Usage: fogrange <near> <far>");
		return;
	}

	g_CV_FogNearZ.m_Val = near_z;
	g_CV_FogFarZ.m_Val = far_z;
	DEdit2_Log("fogrange = %.2f %.2f", near_z, far_z);
}

void CmdWorldForceTextured(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: worldforcetexture <0|1>");
		return;
	}

	int enabled = 0;
	if (!ParseInt(argv[0], enabled))
	{
		DEdit2_Log("Usage: worldforcetexture <0|1>");
		return;
	}

	g_CV_WorldForceTexture.m_Val = enabled != 0 ? 1 : 0;
	DEdit2_Log("worldforcetexture = %d", g_CV_WorldForceTexture.m_Val);
}

bool ParseInt(const char* text, int& out)
{
	if (!text)
	{
		return false;
	}
	char* end = nullptr;
	long value = std::strtol(text, &end, 10);
	if (end == text)
	{
		return false;
	}
	out = static_cast<int>(value);
	return true;
}

bool ParseFloat(const char* text, float& out)
{
	if (!text)
	{
		return false;
	}
	char* end = nullptr;
	float value = std::strtof(text, &end);
	if (end == text)
	{
		return false;
	}
	out = value;
	return true;
}

const char* VarToString(const ConsoleVar& var)
{
	static char buffer[128];
	buffer[0] = '\0';
	if (var.type == ConsoleVar::Type::Int)
	{
		int value = 0;
		if (var.target)
		{
			value = *static_cast<int*>(var.target);
		}
		std::snprintf(buffer, sizeof(buffer), "%d", value);
		return buffer;
	}
	if (var.type == ConsoleVar::Type::Float)
	{
		float value = 0.0f;
		if (var.target)
		{
			value = *static_cast<float*>(var.target);
		}
		std::snprintf(buffer, sizeof(buffer), "%0.6f", value);
		return buffer;
	}
	if (var.target)
	{
		return static_cast<std::string*>(var.target)->c_str();
	}
	return var.value.c_str();
}

void CmdClear(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_ClearLog();
	DEdit2_Log("Console cleared.");
}

void CmdEcho(int argc, const char* argv[])
{
	if (argc <= 0)
	{
		DEdit2_Log("(no args)");
		return;
	}
	std::string message;
	for (int i = 0; i < argc; ++i)
	{
		if (i > 0)
		{
			message += ' ';
		}
		message += argv[i];
	}
	DEdit2_Log("%s", message.c_str());
}

void CmdHelp(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_Log("Commands:");
	for (const auto& name : g_CommandOrder)
	{
		DEdit2_Log("  %s", name.c_str());
	}
	DEdit2_Log("Variables:");
	for (const auto& pair : g_Vars)
	{
		const ConsoleVar& var = pair.second;
		DEdit2_Log("  %s = %s", var.name.c_str(), VarToString(var));
	}
}

void DumpNodes(const std::vector<TreeNode>& nodes, int node_id, int indent)
{
	if (node_id < 0 || static_cast<size_t>(node_id) >= nodes.size())
	{
		return;
	}
	const TreeNode& node = nodes[node_id];
	std::string padding(static_cast<size_t>(indent) * 2, ' ');
	DEdit2_Log("%s- %s", padding.c_str(), node.name.c_str());
	for (int child_id : node.children)
	{
		DumpNodes(nodes, child_id, indent + 1);
	}
}

void CmdTexView(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: texview <texture_name|off>");
		return;
	}

	const std::string name = argv[0];
	if (ToLower(name) == "off")
	{
		g_TexView = {};
		DEdit2_Log("texview cleared.");
		return;
	}

	TextureCache* cache = GetEngineTextureCache();
	if (!cache)
	{
		DEdit2_Log("Texture cache not initialized.");
		return;
	}

	SharedTexture* texture = cache->GetSharedTexture(name.c_str());
	if (!texture)
	{
		DEdit2_Log("Texture '%s' not loaded.", name.c_str());
		return;
	}

	Diligent::ITextureView* view = diligent_get_drawprim_texture_view(texture, false);
	if (!view)
	{
		DEdit2_Log("Texture '%s' has no GPU view.", name.c_str());
		return;
	}

	DiligentTextureDebugInfo info;
	if (!diligent_GetTextureDebugInfo(texture, info))
	{
		DEdit2_Log("Texture '%s' debug info unavailable.", name.c_str());
		return;
	}

	g_TexView.enabled = true;
	g_TexView.name = name;
	g_TexView.view = view;
	g_TexView.width = info.width;
	g_TexView.height = info.height;

	TextureData* texture_data = cache->GetTexture(texture);
	uint32 cpu_pixel = 0;
	bool has_cpu_pixel = false;
	BPPIdent cpu_bpp = BPP_32;
	const void* mip_data = nullptr;
	uint32 mip_size = 0;
	uint32 buf_size = 0;
	uint32 mip_w = 0;
	uint32 mip_h = 0;
	if (texture_data)
	{
		cpu_bpp = texture_data->m_Header.GetBPPIdent();
		buf_size = static_cast<uint32>(texture_data->m_bufSize);
		mip_data = texture_data->m_Mips[0].m_Data;
		mip_size = static_cast<uint32>(texture_data->m_Mips[0].m_dataSize);
		mip_w = static_cast<uint32>(texture_data->m_Mips[0].m_Width);
		mip_h = static_cast<uint32>(texture_data->m_Mips[0].m_Height);
		if (cpu_bpp == BPP_32 && texture_data->m_Mips[0].m_Data)
		{
			std::memcpy(&cpu_pixel, texture_data->m_Mips[0].m_Data, sizeof(uint32));
			has_cpu_pixel = true;
		}
	}

	DEdit2_Log("texview = %s (%ux%u) view=%p tex=%p engine=%p render=%p cpu_bpp=%u cpu_pixel=0x%08X buf=%u mip0=%ux%u data=%p size=%u",
		g_TexView.name.c_str(),
		g_TexView.width,
		g_TexView.height,
		static_cast<void*>(view),
		static_cast<void*>(texture),
		texture ? texture->m_pEngineData : nullptr,
		texture ? texture->m_pRenderData : nullptr,
		static_cast<unsigned>(cpu_bpp),
		has_cpu_pixel ? cpu_pixel : 0u,
		buf_size,
		mip_w,
		mip_h,
		mip_data,
		mip_size);
}

void CmdListNodes(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	if (g_Bindings.project_nodes && !g_Bindings.project_nodes->empty())
	{
		DEdit2_Log("Project nodes:");
		DumpNodes(*g_Bindings.project_nodes, 0, 0);
	}
	else
	{
		DEdit2_Log("Project nodes unavailable.");
	}

	if (g_Bindings.scene_nodes && !g_Bindings.scene_nodes->empty())
	{
		DEdit2_Log("Scene nodes:");
		DumpNodes(*g_Bindings.scene_nodes, 0, 0);
	}
	else
	{
		DEdit2_Log("Scene nodes unavailable.");
	}
}

void CmdFreeTextures(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_Log("freetextures is not implemented in DEdit2 yet.");
}

void CmdReplaceTex(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	DEdit2_Log("replacetex is not implemented in DEdit2 yet.");
}

void CmdShaders(int argc, const char* argv[])
{
	int enabled = g_CV_ShadersEnabled.m_Val;
	if (argc >= 1)
	{
		int value = 0;
		if (!ParseInt(argv[0], value) || (value != 0 && value != 1))
		{
			DEdit2_Log("Usage: shaders [0/1]");
			return;
		}
		enabled = value;
	}
	else
	{
		enabled = enabled ? 0 : 1;
	}

	g_CV_ShadersEnabled.m_Val = enabled;
	DEdit2_Log("Shaders %s.", enabled ? "enabled" : "disabled");
}

void CmdListTextures(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	TextureCache* cache = GetEngineTextureCache();
	if (!cache)
	{
		DEdit2_Log("Texture cache unavailable.");
		return;
	}

	std::vector<std::string> names;
	cache->GetLoadedTextureNames(names);
	if (names.empty())
	{
		DEdit2_Log("No textures loaded.");
		return;
	}

	DEdit2_Log("Loaded textures (%zu):", names.size());
	for (const auto& name : names)
	{
		DEdit2_Log("  %s", name.c_str());
	}
}

void CmdListTexturePaths(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;
	TextureCache* cache = GetEngineTextureCache();
	if (!cache)
	{
		DEdit2_Log("Texture cache unavailable.");
		return;
	}

	std::vector<TextureCache::LoadedTextureInfo> info;
	cache->GetLoadedTextureInfo(info);
	if (info.empty())
	{
		DEdit2_Log("No textures loaded.");
		return;
	}

	DEdit2_Log("Loaded texture paths (%zu):", info.size());
	for (const auto& entry : info)
	{
		const char* path = entry.path.empty() ? "(missing)" : entry.path.c_str();
		DEdit2_Log("  %s -> %s", entry.name.c_str(), path);
	}
}

void CmdTexInfo(int argc, const char* argv[])
{
	if (argc < 1)
	{
		DEdit2_Log("Usage: texinfo <texture_name>");
		return;
	}

	TextureCache* cache = GetEngineTextureCache();
	if (!cache)
	{
		DEdit2_Log("Texture cache unavailable.");
		return;
	}

	TextureCache::TextureDebugInfo info;
	if (!cache->GetTextureDebugInfo(argv[0], info))
	{
		DEdit2_Log("Texture '%s' not found or failed to load.", argv[0]);
		return;
	}

	SharedTexture* shared = cache->GetSharedTexture(argv[0]);
	TextureData* data = shared ? cache->GetTexture(shared) : nullptr;

	DEdit2_Log("Texture info:");
	DEdit2_Log("  Name: %s", info.name.c_str());
	DEdit2_Log("  Path: %s", info.path.empty() ? "(missing)" : info.path.c_str());
	DEdit2_Log("  Size: %ux%u", info.width, info.height);
	DEdit2_Log("  Mips: %u", info.mipmaps);
	DEdit2_Log("  BPP: %s", BppToString(info.bpp));
	DEdit2_Log("  Flags: 0x%08x UserFlags: 0x%08x Sections: %u", info.flags, info.user_flags, info.sections);
	DEdit2_Log("  Buffer: %u bytes (data=%s)", info.buffer_size, info.has_data ? "yes" : "no");

	if (data)
	{
		const auto& pf = data->m_PFormat;
		DEdit2_Log("  PFormat: type=%u bpp=%u masks=%08x/%08x/%08x/%08x firstbits=%u/%u/%u/%u",
			static_cast<unsigned>(pf.m_eType),
			static_cast<unsigned>(pf.m_nBPP),
			pf.m_Masks[0], pf.m_Masks[1], pf.m_Masks[2], pf.m_Masks[3],
			pf.m_FirstBits[0], pf.m_FirstBits[1], pf.m_FirstBits[2], pf.m_FirstBits[3]);
	}
}

} // namespace

const DEdit2TexViewState& DEdit2_GetTexViewState()
{
	return g_TexView;
}

int g_DEdit2MipMapOffset = 0;
int g_DEdit2Bilinear = 1;
int g_DEdit2MaxTextureMemory = 128 * 1024 * 1024;
void DEdit2_InitConsoleCommands()
{
	g_Commands.clear();
	g_CommandOrder.clear();
	g_Vars.clear();

	AddCommand("clear", CmdClear);
	AddCommand("echo", CmdEcho);
	AddCommand("help", CmdHelp);
	AddCommand("listnodes", CmdListNodes);
	AddCommand("freetextures", CmdFreeTextures);
	AddCommand("replacetex", CmdReplaceTex);
	AddCommand("shaders", CmdShaders);
	AddCommand("listtextures", CmdListTextures);
	AddCommand("listtexturepaths", CmdListTexturePaths);
	AddCommand("texinfo", CmdTexInfo);
	AddCommand("texprobe", CmdTexProbe);
	AddCommand("texview", CmdTexView);
	AddCommand("worldcolorstats", CmdWorldColorStats);
	AddCommand("worldtexstats", CmdWorldTextureStats);
	AddCommand("worlduvstats", CmdWorldUvStats);
	AddCommand("worlduvdebug", CmdWorldUvDebug);
	AddCommand("worldtextureddebug", CmdWorldTexturedDebug);
	AddCommand("worldpipestats", CmdWorldPipelineStats);
	AddCommand("worldpipereset", CmdWorldPipelineReset);
	AddCommand("worldpsdebug", CmdWorldPsDebug);
	AddCommand("worldtexdump", CmdWorldTexDump);
	AddCommand("worldsurfacedump", CmdWorldSurfaceDump);
	AddCommand("worldbasevertex", CmdWorldBaseVertex);
	AddCommand("worldshaderreset", CmdWorldShaderReset);
	AddCommand("shaders", CmdShaders);
	AddCommand("fogdebug", CmdFogDebug);
	AddCommand("fogenable", CmdFogEnable);
	AddCommand("fogrange", CmdFogRange);
	AddCommand("worldforcetexture", CmdWorldForceTextured);

	AddVar("mipmapoffset", ConsoleVar::Type::Int, &g_DEdit2MipMapOffset);
	AddVar("mipmapbias", ConsoleVar::Type::Float, &g_CV_MipMapBias.m_Val);
	AddVar("bilinear", ConsoleVar::Type::Int, &g_DEdit2Bilinear);
	AddVar("MaxTextureMemory", ConsoleVar::Type::Int, &g_DEdit2MaxTextureMemory);
	AddVar("worldforcelegacyverts", ConsoleVar::Type::Int, &g_CV_WorldForceLegacyVerts.m_Val);

	DEdit2_Log("DEdit2 console initialized. Type 'help' for commands.");
}

void DEdit2_TermConsoleCommands()
{
	g_Commands.clear();
	g_CommandOrder.clear();
	g_Vars.clear();
	g_Log.clear();
}

void DEdit2_CommandHandler(const char* command)
{
	if (!command)
	{
		return;
	}
	const std::string raw(command);
	if (raw.find_first_not_of(" \t\n\r") == std::string::npos)
	{
		return;
	}

	DEdit2_Log("> %s", raw.c_str());
	std::vector<std::string> tokens = Tokenize(command);
	if (tokens.empty())
	{
		return;
	}

	const std::string key = ToLower(tokens[0]);
	auto cmd_it = g_Commands.find(key);
	if (cmd_it != g_Commands.end())
	{
		std::vector<const char*> args;
		for (size_t i = 1; i < tokens.size(); ++i)
		{
			args.push_back(tokens[i].c_str());
		}
		cmd_it->second.fn(static_cast<int>(args.size()), args.data());
		return;
	}

	auto var_it = g_Vars.find(key);
	if (tokens.size() == 1)
	{
		if (var_it != g_Vars.end())
		{
			DEdit2_Log("%s = %s", var_it->second.name.c_str(), VarToString(var_it->second));
		}
		else
		{
			DEdit2_Log("Unknown command or variable '%s'.", tokens[0].c_str());
		}
		return;
	}

	std::string value = tokens[1];
	for (size_t i = 2; i < tokens.size(); ++i)
	{
		value += ' ';
		value += tokens[i];
	}

	if (var_it == g_Vars.end())
	{
		ConsoleVar var;
		var.name = tokens[0];
		var.type = ConsoleVar::Type::String;
		var.value = value;
		g_Vars[key] = var;
		DEdit2_Log("%s = %s", var.name.c_str(), var.value.c_str());
		return;
	}

	ConsoleVar& var = var_it->second;
	bool updated = false;
	switch (var.type)
	{
		case ConsoleVar::Type::Int:
		{
			int parsed = 0;
			if (ParseInt(value.c_str(), parsed))
			{
				if (var.target)
				{
					*static_cast<int*>(var.target) = parsed;
				}
				if (var.target == &g_DEdit2MipMapOffset)
				{
					g_CV_TextureMipMapOffset = parsed;
				}
				updated = true;
			}
			break;
		}
		case ConsoleVar::Type::Float:
		{
			float parsed = 0.0f;
			if (ParseFloat(value.c_str(), parsed))
			{
				if (var.target)
				{
					*static_cast<float*>(var.target) = parsed;
				}
				if (var.target == &g_CV_MipMapBias.m_Val)
				{
					diligent_ResetWorldShaders();
				}
				updated = true;
			}
			break;
		}
		case ConsoleVar::Type::String:
		{
			if (var.target)
			{
				*static_cast<std::string*>(var.target) = value;
			}
			else
			{
				var.value = value;
			}
			updated = true;
			break;
		}
	}

	if (!updated)
	{
		DEdit2_Log("Invalid value for %s: %s", var.name.c_str(), value.c_str());
		return;
	}

	DEdit2_Log("%s = %s", var.name.c_str(), VarToString(var));
}

void DEdit2_SetConsoleBindings(const DEdit2ConsoleBindings& bindings)
{
	g_Bindings = bindings;
}

void DEdit2_LogV(const char* fmt, va_list args)
{
	if (!fmt)
	{
		return;
	}

	char buffer[1024];
	std::vsnprintf(buffer, sizeof(buffer), fmt, args);

	g_Log.emplace_back(buffer);
	std::fprintf(stderr, "%s\n", buffer);
}

void DEdit2_Log(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	DEdit2_LogV(fmt, args);
	va_end(args);
}

void DEdit2_ClearLog()
{
	g_Log.clear();
}

const std::vector<std::string>& DEdit2_GetLog()
{
	return g_Log;
}
