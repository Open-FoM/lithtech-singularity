#include "editor_model_mgr.h"

#include "bdefs.h"
#include "de_objects.h"
#include "editor_state.h"
#include "animtracker.h"
#include "model.h"
#include "lteulerangles.h"
#include "lt_stream.h"
#include "texture_cache.h"
#include "engine_render.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace
{
namespace fs = std::filesystem;

std::string ToLower(std::string value)
{
	for (char& ch : value)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return value;
}

// Convert euler angles (in radians) to LTRotation quaternion
// DEdit stores rotation as Euler angles in radians (pitch, yaw, roll)
LTRotation EulerToLTRotation(float pitch, float yaw, float roll)
{
	// LithTech uses YXZ order for euler angles
	EulerAngles ea = Eul_(yaw, pitch, roll, EulOrdYXZs);
	return Eul_ToQuat(ea);
}

// Check if a path points to a model file
bool IsModelFile(const std::string& path)
{
	if (path.empty())
	{
		return false;
	}
	std::string ext = ToLower(fs::path(path).extension().string());
	return ext == ".ltb" || ext == ".abc" || ext == ".lmo";
}

// Check if a node type represents a renderable model
bool IsModelNodeType(const std::string& type, const std::string& class_name)
{
	// Convert to lowercase for comparison
	std::string type_lower = ToLower(type);
	std::string class_lower = ToLower(class_name);

	// Check type
	if (type_lower == "model" || type_lower == "prop")
	{
		return true;
	}

	// Check class name for common model classes
	if (class_lower == "prop" || class_lower == "door" || class_lower == "pickup" ||
	    class_lower == "worlditem" || class_lower == "weapon" || class_lower == "character" ||
	    class_lower == "body" || class_lower == "debris" || class_lower == "gadgettarget")
	{
		return true;
	}

	return false;
}

} // namespace

EditorModelManager::EditorModelManager()
{
}

EditorModelManager::~EditorModelManager()
{
	Clear();
}

std::string EditorModelManager::ResolveModelPath(const std::string& path) const
{
	if (path.empty())
	{
		return {};
	}

	// If absolute path and exists, use it directly
	const fs::path p(path);
	if (p.is_absolute())
	{
		std::error_code ec;
		if (fs::exists(p, ec))
		{
			return p.string();
		}
	}

	// Search through file manager trees
	const auto& trees = GetClientFileMgrTrees();
	for (const auto& root : trees)
	{
		if (root.empty())
		{
			continue;
		}

		// Try exact path
		fs::path candidate = fs::path(root) / path;
		std::error_code ec;
		if (fs::exists(candidate, ec))
		{
			return candidate.string();
		}

		// Try with different common subdirectories
		const char* subdirs[] = {"Models", "models", "Props", "props", "Characters", "characters", ""};
		for (const char* subdir : subdirs)
		{
			candidate = fs::path(root) / subdir / path;
			if (fs::exists(candidate, ec))
			{
				return candidate.string();
			}

			// Also try with lowercase extension
			std::string lower_path = path;
			size_t dot_pos = lower_path.rfind('.');
			if (dot_pos != std::string::npos)
			{
				for (size_t i = dot_pos; i < lower_path.size(); ++i)
				{
					lower_path[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower_path[i])));
				}
				candidate = fs::path(root) / subdir / lower_path;
				if (fs::exists(candidate, ec))
				{
					return candidate.string();
				}
			}
		}
	}

	// If using texture cache, try its resolver
	TextureCache* tex_cache = GetEngineTextureCache();
	if (tex_cache)
	{
		std::string resolved = tex_cache->ResolveResourcePath(path, ".ltb");
		if (!resolved.empty())
		{
			return resolved;
		}
		// Try other extensions
		resolved = tex_cache->ResolveResourcePath(path, ".abc");
		if (!resolved.empty())
		{
			return resolved;
		}
	}

	return {};
}

Model* EditorModelManager::LoadModel(const std::string& path)
{
	if (path.empty())
	{
		return nullptr;
	}

	// Check cache first
	auto it = m_model_cache.find(path);
	if (it != m_model_cache.end())
	{
		return it->second;
	}

	// Resolve the path
	std::string resolved = ResolveModelPath(path);
	if (resolved.empty())
	{
		// Try again with just the filename
		std::string filename = fs::path(path).filename().string();
		if (filename != path)
		{
			resolved = ResolveModelPath(filename);
		}
	}

	if (resolved.empty())
	{
		std::fprintf(stderr, "EditorModelManager: Could not resolve model path: %s\n", path.c_str());
		return nullptr;
	}

	// Open file stream
	ILTStream* stream = OpenFileStream(resolved);
	if (!stream)
	{
		std::fprintf(stderr, "EditorModelManager: Could not open model file: %s\n", resolved.c_str());
		return nullptr;
	}

	// Create and load model
	Model* model = new Model;
	ModelLoadRequest request;
	request.m_pFile = stream;
	request.m_bLoadChildModels = false;  // Don't load child models for now

	LTRESULT result = model->Load(&request, resolved.c_str());
	stream->Release();

	if (result != LT_OK)
	{
		std::fprintf(stderr, "EditorModelManager: Failed to load model: %s (error %d)\n",
			resolved.c_str(), static_cast<int>(result));
		delete model;
		return nullptr;
	}

	// Cache it
	m_model_cache[path] = model;

	std::fprintf(stderr, "EditorModelManager: Loaded model: %s\n", resolved.c_str());
	return model;
}

void EditorModelManager::LoadModelTextures(ModelInstance* inst, Model* model)
{
	if (!inst || !model)
	{
		return;
	}

	TextureCache* tex_cache = GetEngineTextureCache();
	if (!tex_cache)
	{
		return;
	}

	// Load textures for each skin slot
	for (uint32 i = 0; i < MAX_MODEL_TEXTURES; ++i)
	{
		const char* tex_name = model->GetFilename();  // Model might have embedded texture refs
		// For now, we'll rely on the model's internal texture references
		// which are set up during rendering through the render struct callbacks
	}
}

ModelInstance* EditorModelManager::CreateInstance(
	int node_id,
	const std::string& model_path,
	const float pos[3],
	const float rot[3],
	const float scale[3])
{
	if (!IsModelFile(model_path))
	{
		return nullptr;
	}

	// Load the model
	Model* model = LoadModel(model_path);
	if (!model)
	{
		return nullptr;
	}

	// Create ModelInstance
	ModelInstance* inst = new ModelInstance;

	// Set object type and flags
	inst->m_ObjectType = OT_MODEL;
	inst->m_Flags = FLAG_VISIBLE;
	inst->m_Flags2 = 0;

	// Set position
	inst->m_Pos.Init(pos[0], pos[1], pos[2]);

	// Set scale
	inst->m_Scale.Init(scale[0], scale[1], scale[2]);

	// Set rotation (convert euler angles to quaternion)
	inst->m_Rotation = EulerToLTRotation(rot[0], rot[1], rot[2]);

	// Initialize dims from model radius
	float radius = model->m_VisRadius;
	if (radius <= 0.0f)
	{
		radius = 50.0f;  // Default radius if model doesn't specify
	}
	inst->SetDims(LTVector(radius, radius, radius));

	// Bind the model database
	inst->BindModelDB(model);

	// Initialize animation trackers with default flags
	// AT_ALLOWINVALID allows rendering even without valid animation
	inst->InitAnimTrackers(AT_ALLOWINVALID);

	// Load textures
	LoadModelTextures(inst, model);

	// Track the instance
	m_instances.push_back(inst);
	m_node_to_instance[node_id] = inst;

	return inst;
}

void EditorModelManager::UpdateInstanceTransform(
	ModelInstance* inst,
	const float pos[3],
	const float rot[3],
	const float scale[3])
{
	if (!inst)
	{
		return;
	}

	// Update position
	inst->m_Pos.Init(pos[0], pos[1], pos[2]);

	// Update scale
	inst->m_Scale.Init(scale[0], scale[1], scale[2]);

	// Update rotation
	inst->m_Rotation = EulerToLTRotation(rot[0], rot[1], rot[2]);

	// Mark cached transforms as needing re-evaluation
	inst->ResetCachedTransformNodeStates();
}

void EditorModelManager::DestroyInstance(ModelInstance* inst)
{
	if (!inst)
	{
		return;
	}

	// Remove from instances vector
	auto it = std::find(m_instances.begin(), m_instances.end(), inst);
	if (it != m_instances.end())
	{
		m_instances.erase(it);
	}

	// Remove from node map
	for (auto map_it = m_node_to_instance.begin(); map_it != m_node_to_instance.end(); )
	{
		if (map_it->second == inst)
		{
			map_it = m_node_to_instance.erase(map_it);
		}
		else
		{
			++map_it;
		}
	}

	// Delete the instance
	inst->UnbindModelDB();
	delete inst;
}

ModelInstance* EditorModelManager::GetInstanceForNode(int node_id) const
{
	auto it = m_node_to_instance.find(node_id);
	if (it != m_node_to_instance.end())
	{
		return it->second;
	}
	return nullptr;
}

std::vector<LTObject*> EditorModelManager::CollectObjectList() const
{
	std::vector<LTObject*> objects;
	objects.reserve(m_instances.size());
	for (ModelInstance* inst : m_instances)
	{
		if (inst && (inst->m_Flags & FLAG_VISIBLE))
		{
			objects.push_back(static_cast<LTObject*>(inst));
		}
	}
	return objects;
}

void EditorModelManager::Clear()
{
	// Delete all instances
	for (ModelInstance* inst : m_instances)
	{
		if (inst)
		{
			inst->UnbindModelDB();
		}
		delete inst;
	}
	m_instances.clear();
	m_node_to_instance.clear();

	// Release all cached models
	for (auto& pair : m_model_cache)
	{
		if (pair.second)
		{
			pair.second->Release();
		}
	}
	m_model_cache.clear();
}

// Static singleton instance
static EditorModelManager s_editor_model_mgr;

EditorModelManager& GetEditorModelManager()
{
	return s_editor_model_mgr;
}

void SyncModelsWithSceneTree(
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props)
{
	EditorModelManager& mgr = GetEditorModelManager();
	mgr.Clear();

	const size_t count = std::min(nodes.size(), props.size());
	for (size_t i = 0; i < count; ++i)
	{
		const NodeProperties& p = props[i];

		// Skip deleted nodes
		if (nodes[i].deleted)
		{
			continue;
		}

		// Check if this node represents a model
		if (!IsModelNodeType(p.type, p.class_name))
		{
			continue;
		}

		// Must have a resource path
		if (p.resource.empty())
		{
			continue;
		}

		// Check if the resource looks like a model file
		if (!IsModelFile(p.resource))
		{
			continue;
		}

		// Calculate final scale
		float scale[3] = {
			p.scale[0] * p.model_scale,
			p.scale[1] * p.model_scale,
			p.scale[2] * p.model_scale
		};

		// Create the model instance
		mgr.CreateInstance(
			static_cast<int>(i),
			p.resource,
			p.position,
			p.rotation,
			scale);
	}

	std::fprintf(stderr, "SyncModelsWithSceneTree: Created %zu model instances\n",
		mgr.GetInstanceCount());
}
