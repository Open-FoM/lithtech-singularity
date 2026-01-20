#include "lt_stream.h"

#include "client_filemgr.h"
#include "dedit2_concommand.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

namespace
{
namespace fs = std::filesystem;

IClientFileMgr* g_client_file_mgr = nullptr;
define_holder(IClientFileMgr, g_client_file_mgr);

bool g_file_mgr_ready = false;
std::vector<std::string> g_file_mgr_trees;
std::vector<fs::path> g_file_mgr_tree_paths;

std::string NormalizePath(std::string path)
{
	std::replace(path.begin(), path.end(), '\\', '/');
	return path;
}

std::string NormalizeLower(std::string path)
{
	path = NormalizePath(std::move(path));
	std::transform(path.begin(), path.end(), path.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return path;
}

std::string MakeRelativeToTrees(const std::string& path)
{
	if (path.empty())
	{
		return path;
	}

	fs::path input(path);
	if (!input.is_absolute())
	{
		return NormalizePath(path);
	}

	std::error_code ec;
	fs::path normalized = fs::weakly_canonical(input, ec);
	if (ec)
	{
		normalized = input.lexically_normal();
	}
	const std::string normalized_str = NormalizeLower(normalized.string());

	for (const auto& tree : g_file_mgr_tree_paths)
	{
		if (tree.empty())
		{
			continue;
		}

		fs::path tree_norm = fs::weakly_canonical(tree, ec);
		if (ec)
		{
			tree_norm = tree.lexically_normal();
		}

		const std::string tree_str = NormalizeLower(tree_norm.string());
		if (!tree_str.empty() && normalized_str.rfind(tree_str, 0) == 0)
		{
			fs::path rel = fs::relative(normalized, tree_norm, ec);
			if (!ec)
			{
				return NormalizePath(rel.string());
			}
		}
	}

	return NormalizePath(path);
}
} // namespace

ILTStream* OpenFileStream(const std::string& path)
{
	if (!g_client_file_mgr)
	{
		DEdit2_Log("File manager unavailable; cannot open '%s'.", path.c_str());
		return nullptr;
	}

	if (!g_file_mgr_ready)
	{
		g_client_file_mgr->Init();
		g_file_mgr_ready = true;
	}

	const std::string relative = MakeRelativeToTrees(path);
	FileRef ref;
	ref.m_FileType = FILE_ANYFILE;
	ref.m_pFilename = relative.c_str();
	return g_client_file_mgr->OpenFile(&ref);
}

bool InitClientFileMgr()
{
	if (!g_client_file_mgr)
	{
		DEdit2_Log("File manager unavailable; cannot initialize.");
		return false;
	}

	if (!g_file_mgr_ready)
	{
		g_client_file_mgr->Init();
		g_file_mgr_ready = true;
	}

	return true;
}

void TermClientFileMgr()
{
	if (g_client_file_mgr && g_file_mgr_ready)
	{
		g_client_file_mgr->Term();
	}
	g_file_mgr_ready = false;
	g_file_mgr_trees.clear();
	g_file_mgr_tree_paths.clear();
}

void SetClientFileMgrTrees(const std::vector<std::string>& trees)
{
	if (!InitClientFileMgr())
	{
		return;
	}

	g_client_file_mgr->Term();
	g_client_file_mgr->Init();
	g_file_mgr_ready = true;

	g_file_mgr_trees = trees;
	g_file_mgr_tree_paths.clear();
	g_file_mgr_tree_paths.reserve(trees.size());
	for (const auto& tree : trees)
	{
		g_file_mgr_tree_paths.emplace_back(tree);
	}

	if (!trees.empty())
	{
		std::vector<const char*> names;
		names.reserve(trees.size());
		for (const auto& tree : trees)
		{
			names.push_back(tree.c_str());
		}

		int loaded = 0;
		g_client_file_mgr->AddResourceTrees(names.data(), static_cast<int>(names.size()), nullptr, &loaded);
		DEdit2_Log("File manager trees loaded: %d/%zu", loaded, trees.size());
	}
}

const std::vector<std::string>& GetClientFileMgrTrees()
{
	return g_file_mgr_trees;
}

bool ReadFileToBuffer(const std::string& path, std::vector<uint8_t>& out_data, std::string& error)
{
	out_data.clear();
	error.clear();

	ILTStream* stream = OpenFileStream(path);
	if (!stream)
	{
		error = "Failed to open file.";
		return false;
	}

	uint32 length = 0;
	if (stream->GetLen(&length) != LT_OK || length == 0)
	{
		error = "File is empty or length unknown.";
		stream->Release();
		return false;
	}

	out_data.resize(length);
	if (stream->Read(out_data.data(), length) != LT_OK)
	{
		error = "Failed to read file.";
		out_data.clear();
		stream->Release();
		return false;
	}

	stream->Release();
	return true;
}
