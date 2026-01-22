#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class Model;
class ModelInstance;
class LTObject;

// Forward declarations for scene tree types
struct TreeNode;
struct NodeProperties;

// EditorModelManager handles loading and managing model instances for rendering
// in the DEdit2 viewport. It maintains a cache of loaded Model databases and
// creates ModelInstance objects that can be passed to the renderer.
class EditorModelManager
{
public:
	EditorModelManager();
	~EditorModelManager();

	// Load a Model database from file (returns cached if already loaded)
	// path: Relative or absolute path to .ltb model file
	Model* LoadModel(const std::string& path);

	// Create a ModelInstance for rendering
	// node_id: Scene tree node ID (used for tracking)
	// model_path: Path to the model file
	// pos: World position [x, y, z]
	// rot: Euler rotation in radians [pitch, yaw, roll]
	// scale: Scale factors [x, y, z]
	ModelInstance* CreateInstance(
		int node_id,
		const std::string& model_path,
		const float pos[3],
		const float rot[3],
		const float scale[3]);

	// Update an existing instance's transform
	void UpdateInstanceTransform(
		ModelInstance* inst,
		const float pos[3],
		const float rot[3],
		const float scale[3]);

	// Destroy a specific instance
	void DestroyInstance(ModelInstance* inst);

	// Get the instance associated with a scene node
	ModelInstance* GetInstanceForNode(int node_id) const;

	// Collect all instances as LTObject pointers for rendering
	std::vector<LTObject*> CollectObjectList() const;

	// Get the number of loaded instances
	size_t GetInstanceCount() const { return m_instances.size(); }

	// Get the number of cached models
	size_t GetModelCount() const { return m_model_cache.size(); }

	// Clear all instances and cached models
	void Clear();

private:
	// Resolve a model path relative to project/rez directories
	std::string ResolveModelPath(const std::string& path) const;

	// Load textures for a model instance
	void LoadModelTextures(ModelInstance* inst, Model* model);

	// Cache of loaded Model databases (path -> Model*)
	std::unordered_map<std::string, Model*> m_model_cache;

	// All created instances
	std::vector<ModelInstance*> m_instances;

	// Map from scene node ID to instance
	std::unordered_map<int, ModelInstance*> m_node_to_instance;
};

// Global accessor for the singleton EditorModelManager instance
EditorModelManager& GetEditorModelManager();

// Sync model instances with the current scene tree
// Creates ModelInstance objects for all model/prop nodes in the scene
void SyncModelsWithSceneTree(
	const std::vector<TreeNode>& nodes,
	const std::vector<NodeProperties>& props);
