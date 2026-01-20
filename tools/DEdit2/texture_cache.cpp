#include "texture_cache.h"

#include "dtx_loader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{
std::string NormalizeKey(std::string name)
{
	std::replace(name.begin(), name.end(), '\\', '/');
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return name;
}

bool HasExtension(const fs::path& path)
{
	const auto ext = path.extension().string();
	return !ext.empty();
}
} // namespace

void TextureCache::SetSearchRoots(const std::vector<std::string>& roots)
{
	roots_.clear();
	roots_.reserve(roots.size());
	for (const auto& root : roots)
	{
		if (!root.empty())
		{
			roots_.push_back(root);
		}
	}
}

SharedTexture* TextureCache::GetSharedTexture(const char* name)
{
	if (!name || name[0] == '\0')
	{
		return nullptr;
	}

	const std::string key = NormalizeKey(name);
	auto found = by_name_.find(key);
	if (found != by_name_.end())
	{
		return found->second->texture.get();
	}

	const std::string path = ResolveTexturePath(name);
	TextureEntry* entry = CreateEntry(name, path);
	return entry ? entry->texture.get() : nullptr;
}

TextureData* TextureCache::GetTexture(SharedTexture* texture)
{
	if (!texture)
	{
		return nullptr;
	}

	auto found = by_ptr_.find(texture);
	if (found == by_ptr_.end())
	{
		return nullptr;
	}

	return found->second->data.get();
}

const char* TextureCache::GetTextureName(const SharedTexture* texture) const
{
	if (!texture)
	{
		return nullptr;
	}

	auto found = by_ptr_.find(texture);
	if (found == by_ptr_.end() || !found->second)
	{
		return nullptr;
	}

	return found->second->name.c_str();
}

void TextureCache::FreeTexture(SharedTexture* texture)
{
	if (!texture)
	{
		return;
	}

	auto found = by_ptr_.find(texture);
	if (found == by_ptr_.end())
	{
		return;
	}

	if (found->second)
	{
		by_name_.erase(NormalizeKey(found->second->name));
	}
	by_ptr_.erase(found);
}

void TextureCache::Clear()
{
	by_ptr_.clear();
	by_name_.clear();
}

std::string TextureCache::ResolveTexturePath(const std::string& name) const
{
	return ResolveResourcePath(name, ".dtx");
}

std::string TextureCache::ResolveResourcePath(const std::string& name, const char* extension) const
{
	fs::path input(name);
	if (input.is_absolute() && fs::exists(input))
	{
		return input.string();
	}

	const bool has_ext = HasExtension(input);
	std::vector<fs::path> candidates;
	candidates.reserve(roots_.size() * 6);

	for (const auto& root : roots_)
	{
		fs::path base(root);
		candidates.push_back(base / input);
		candidates.push_back(base / "Textures" / input);
		candidates.push_back(base / "Sprites" / input);
		candidates.push_back(base / "Rez" / input);
		candidates.push_back(base / "Rez" / "Textures" / input);
		candidates.push_back(base / "Rez" / "Sprites" / input);
	}

	for (auto& candidate : candidates)
	{
		if (!has_ext && extension && extension[0] != '\0')
		{
			candidate.replace_extension(extension);
		}
		if (fs::exists(candidate))
		{
			return candidate.string();
		}
	}

	return std::string();
}

TextureCache::TextureEntry* TextureCache::CreateEntry(const std::string& name, const std::string& path)
{
	std::string error;
	std::unique_ptr<TextureData> data;
	if (!path.empty())
	{
		data.reset(LoadDtxTexture(path, &error));
	}

	if (!data)
	{
		data.reset(CreateSolidColorTexture(0xFFCCCCCCu));
	}

	if (!data)
	{
		return nullptr;
	}

	auto entry = std::make_unique<TextureEntry>();
	entry->name = name;
	entry->path = path;
	entry->data = std::move(data);
	entry->texture = std::make_unique<SharedTexture>();
	entry->texture->m_pEngineData = entry->data.get();
	entry->texture->m_pRenderData = nullptr;
	entry->texture->m_pFile = nullptr;
	entry->texture->SetRefCount(1);
	entry->data->m_pSharedTexture = entry->texture.get();

	TextureEntry* entry_ptr = entry.get();
	by_ptr_[entry_ptr->texture.get()] = entry_ptr;
	by_name_.emplace(NormalizeKey(name), std::move(entry));
	return entry_ptr;
}
