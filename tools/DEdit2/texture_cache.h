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
	void SetSearchRoots(const std::vector<std::string>& roots);
	SharedTexture* GetSharedTexture(const char* name);
	TextureData* GetTexture(SharedTexture* texture);
	const char* GetTextureName(const SharedTexture* texture) const;
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
