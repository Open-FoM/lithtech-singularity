#include "texture_cache.h"

#include "dtxmgr.h"
#include "lt_stream.h"
#include "dedit2_concommand.h"
#include "path_utils.h"

extern int32 g_CV_TextureMipMapOffset;

namespace
{
struct ScopedTextureMipOffset
{
	explicit ScopedTextureMipOffset(int32 value)
		: previous(g_CV_TextureMipMapOffset)
	{
		g_CV_TextureMipMapOffset = value;
	}

	~ScopedTextureMipOffset()
	{
		g_CV_TextureMipMapOffset = previous;
	}

	int32 previous = 0;
};
} // namespace

#include <algorithm>
#include <cctype>
#include <cstring>
namespace
{
std::string NormalizeKey(std::string name)
{
	name = path_utils::NormalizePathSeparators(std::move(name));
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	if (name.size() > 4 && name.compare(name.size() - 4, 4, ".dtx") == 0)
	{
		name.resize(name.size() - 4);
	}
	return name;
}

std::string NormalizeKeyLegacy(std::string name)
{
	name = path_utils::NormalizePathSeparators(std::move(name));
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return name;
}

TextureData* CreateSolidColorTexture(uint32 rgba)
{
	TextureData* texture = dtx_Alloc(BPP_32, 1, 1, 1, nullptr, nullptr, 0);
	if (!texture || !texture->m_pDataBuffer)
	{
		if (texture)
		{
			dtx_Destroy(texture);
		}
		return nullptr;
	}

	std::memcpy(texture->m_pDataBuffer, &rgba, sizeof(uint32));
	texture->m_Mips[0].m_Width = 1;
	texture->m_Mips[0].m_Height = 1;
	texture->m_Mips[0].m_Pitch = sizeof(uint32);
	texture->m_Mips[0].m_dataSize = sizeof(uint32);
	texture->m_Mips[0].m_DataHeader = texture->m_pDataBuffer;
	texture->m_Mips[0].m_Data = texture->m_pDataBuffer;
	return texture;
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

	std::string trimmed = path_utils::TrimWhitespace(name);
	if (trimmed.empty())
	{
		return nullptr;
	}

	const std::string key = NormalizeKey(trimmed);
	auto found = by_name_.find(key);
	if (found != by_name_.end())
	{
		return found->second->texture.get();
	}
	const std::string legacy_key = NormalizeKeyLegacy(trimmed);
	if (legacy_key != key)
	{
		found = by_name_.find(legacy_key);
		if (found != by_name_.end())
		{
			return found->second->texture.get();
		}
		auto alias = by_alias_.find(legacy_key);
		if (alias != by_alias_.end())
		{
			return alias->second ? alias->second->texture.get() : nullptr;
		}
	}

	const std::string path = ResolveTexturePath(trimmed);
	TextureEntry* entry = CreateEntry(trimmed, path);
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

bool TextureCache::GetTexturePath(const char* name, std::string& out_path) const
{
	if (!name || name[0] == '\0')
	{
		return false;
	}

	const std::string key = NormalizeKey(name);
	auto found = by_name_.find(key);
	if (found != by_name_.end() && found->second)
	{
		out_path = found->second->path;
		return true;
	}
	const std::string legacy_key = NormalizeKeyLegacy(name);
	found = by_name_.find(legacy_key);
	if (found != by_name_.end() && found->second)
	{
		out_path = found->second->path;
		return true;
	}
	auto alias = by_alias_.find(legacy_key);
	if (alias != by_alias_.end() && alias->second)
	{
		out_path = alias->second->path;
		return true;
	}
	return false;
}

bool TextureCache::GetTextureDebugInfo(const char* name, TextureDebugInfo& out_info)
{
	if (!name || name[0] == '\0')
	{
		return false;
	}

	SharedTexture* texture = GetSharedTexture(name);
	if (!texture)
	{
		return false;
	}

	const std::string key = NormalizeKey(name);
	auto found = by_name_.find(key);
	const TextureEntry* entry_ptr = nullptr;
	if (found != by_name_.end())
	{
		entry_ptr = found->second.get();
	}
	else
	{
		const std::string legacy_key = NormalizeKeyLegacy(name);
		found = by_name_.find(legacy_key);
		if (found != by_name_.end())
		{
			entry_ptr = found->second.get();
		}
		else
		{
			auto alias = by_alias_.find(legacy_key);
			if (alias != by_alias_.end())
			{
				entry_ptr = alias->second;
			}
		}
	}

	if (!entry_ptr || !entry_ptr->data)
	{
		return false;
	}

	const auto& entry = *entry_ptr;
	const auto& header = entry.data->m_Header;
	out_info.name = entry.name;
	out_info.path = entry.path;
	out_info.width = header.m_BaseWidth;
	out_info.height = header.m_BaseHeight;
	out_info.mipmaps = header.m_nMipmaps;
	out_info.sections = header.m_nSections;
	out_info.flags = header.m_IFlags;
	out_info.user_flags = header.m_UserFlags;
	out_info.buffer_size = entry.data->m_bufSize;
	out_info.bpp = header.m_Extra[2] == 0 ? BPP_32 : static_cast<BPPIdent>(header.m_Extra[2]);
	out_info.has_data = entry.data->m_pDataBuffer != nullptr;
	return true;
}

void TextureCache::GetLoadedTextureNames(std::vector<std::string>& out_names) const
{
	out_names.clear();
	out_names.reserve(by_name_.size());
	for (const auto& pair : by_name_)
	{
		if (pair.second)
		{
			out_names.push_back(pair.second->name);
		}
	}
	std::sort(out_names.begin(), out_names.end());
}

void TextureCache::GetLoadedTextureInfo(std::vector<LoadedTextureInfo>& out_info) const
{
	out_info.clear();
	out_info.reserve(by_name_.size());
	for (const auto& pair : by_name_)
	{
		if (pair.second)
		{
			LoadedTextureInfo info;
			info.name = pair.second->name;
			info.path = pair.second->path;
			out_info.push_back(std::move(info));
		}
	}
	std::sort(out_info.begin(), out_info.end(),
		[](const LoadedTextureInfo& a, const LoadedTextureInfo& b)
		{
			return a.name < b.name;
		});
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
		const std::string key = NormalizeKey(found->second->name);
		const std::string legacy_key = NormalizeKeyLegacy(found->second->name);
		by_name_.erase(key);
		if (legacy_key != key)
		{
			by_name_.erase(legacy_key);
			by_alias_.erase(legacy_key);
		}
	}
	by_ptr_.erase(found);
}

void TextureCache::Clear()
{
	by_ptr_.clear();
	by_name_.clear();
	by_alias_.clear();
}

std::string TextureCache::ResolveTexturePath(const std::string& name) const
{
	return ResolveResourcePath(name, ".dtx");
}

std::string TextureCache::ResolveResourcePath(const std::string& name, const char* extension) const
{
	std::string normalized = path_utils::NormalizePathSeparators(name);
	if (!path_utils::HasExtension(normalized) && extension && extension[0] != '\0')
	{
		normalized += extension;
	}
	return normalized;
}

TextureCache::TextureEntry* TextureCache::CreateEntry(const std::string& name, const std::string& path)
{
	const std::string canonical_name = path_utils::NormalizePathSeparators(name);
	const std::string key = NormalizeKey(canonical_name);
	auto existing = by_name_.find(key);
	if (existing != by_name_.end())
	{
		return existing->second.get();
	}
	const std::string legacy_key = NormalizeKeyLegacy(canonical_name);
	if (legacy_key != key)
	{
		auto alias = by_alias_.find(legacy_key);
		if (alias != by_alias_.end())
		{
			return alias->second;
		}
	}

	std::string error;
	std::unique_ptr<TextureData> data;
	if (!path.empty())
	{
		ILTStream* stream = OpenFileStream(path);
		if (!stream)
		{
			DEdit2_Log("[TEX] NOT FOUND via file mgr: '%s' -> '%s'", name.c_str(), path.c_str());
			error = "Failed to open texture file.";
		}
		else
		{
			DEdit2_Log("[TEX] Loading: '%s' from '%s'", name.c_str(), path.c_str());
			uint32 base_width = 0;
			uint32 base_height = 0;
			TextureData* out = nullptr;
			const int32 mip_offset = std::max(0, g_DEdit2MipMapOffset);
			ScopedTextureMipOffset mip_guard(mip_offset);
			const LTRESULT result = dtx_Create(stream, &out, base_width, base_height);
			stream->Release();
			if (result == LT_OK && out)
			{
				data.reset(out);
				DEdit2_Log("[TEX] Loaded: %ux%u mips=%u bpp=%u size=%u",
					base_width, base_height, out->m_Header.m_nMipmaps,
					out->m_Header.GetBPPIdent(), out->m_bufSize);
				// Check if data is non-zero with detailed count
				uint32 non_zero_count = 0;
				if (out->m_pDataBuffer && out->m_bufSize > 0)
				{
					const uint32 check_size = out->m_bufSize < 256 ? out->m_bufSize : 256;
					for (uint32 i = 0; i < check_size; ++i)
					{
						if (out->m_pDataBuffer[i] != 0)
						{
							++non_zero_count;
						}
					}
					DEdit2_Log("[TEX] First 256 bytes: %u non-zero", non_zero_count);
				}
				if (non_zero_count == 0)
				{
					DEdit2_Log("[TEX] WARNING: Buffer appears to be all zeros!");
				}
			}
			else
			{
				DEdit2_Log("[TEX] dtx_Create FAILED: %s", error.empty() ? "unknown" : error.c_str());
				error = "dtx_Create failed.";
				if (out)
				{
					dtx_Destroy(out);
				}
			}
		}

		if (!data)
		{
			DEdit2_Log("Texture load failed: %s (%s) -> %s",
				name.c_str(),
				path.c_str(),
				error.empty() ? "unknown error" : error.c_str());
		}
	}
	else
	{
		DEdit2_Log("[TEX] Path empty, not found via file manager: %s (using fallback)", name.c_str());
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
	entry->name = canonical_name;
	entry->path = path;
	entry->data = std::move(data);
	entry->texture = std::make_unique<SharedTexture>();
	entry->texture->m_pEngineData = entry->data.get();
	entry->texture->m_pRenderData = nullptr;
	entry->texture->m_pFile = nullptr;
	entry->texture->SetRefCount(1);
	entry->texture->SetTextureInfo(
		entry->data->m_Header.m_BaseWidth,
		entry->data->m_Header.m_BaseHeight,
		entry->data->m_PFormat);
	entry->data->m_pSharedTexture = entry->texture.get();

	TextureEntry* entry_ptr = entry.get();
	by_ptr_[entry_ptr->texture.get()] = entry_ptr;
	by_name_.emplace(key, std::move(entry));
	if (legacy_key != key)
	{
		by_alias_[legacy_key] = entry_ptr;
	}
	return entry_ptr;
}
