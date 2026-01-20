#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ltbasedefs.h"
#include "dtxmgr.h"
#include "de_world.h"

class TextureCache
{
public:
	struct LoadedTextureInfo
	{
		std::string name;
		std::string path;
	};

	struct TextureDebugInfo
	{
		std::string name;
		std::string path;
		uint32 width = 0;
		uint32 height = 0;
		uint32 mipmaps = 0;
		uint32 sections = 0;
		uint32 flags = 0;
		uint32 user_flags = 0;
		uint32 buffer_size = 0;
		BPPIdent bpp = BPP_32;
		bool has_data = false;
	};

	void SetSearchRoots(const std::vector<std::string>& roots);
	SharedTexture* GetSharedTexture(const char* name);
	TextureData* GetTexture(SharedTexture* texture);
	const char* GetTextureName(const SharedTexture* texture) const;
	bool GetTexturePath(const char* name, std::string& out_path) const;
	bool GetTextureDebugInfo(const char* name, TextureDebugInfo& out_info);
	void GetLoadedTextureNames(std::vector<std::string>& out_names) const;
	void GetLoadedTextureInfo(std::vector<LoadedTextureInfo>& out_info) const;
	void FreeTexture(SharedTexture* texture);
	void Clear();
	std::string ResolveResourcePath(const std::string& name, const char* extension) const;

private:
	struct TextureEntry
	{
		std::string name;
		std::string path;
		std::unique_ptr<SharedTexture> texture;
		std::unique_ptr<TextureData> data;
	};

	std::string ResolveTexturePath(const std::string& name) const;
	TextureEntry* CreateEntry(const std::string& name, const std::string& path);

	std::vector<std::string> roots_;
	std::unordered_map<std::string, std::unique_ptr<TextureEntry>> by_name_;
	std::unordered_map<const SharedTexture*, TextureEntry*> by_ptr_;
};
