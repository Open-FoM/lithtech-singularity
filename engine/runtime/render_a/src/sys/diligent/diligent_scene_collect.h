/**
 * diligent_scene_collect.h
 *
 * This header defines the Scene Collect portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_SCENE_COLLECT_H
#define LTJS_DILIGENT_SCENE_COLLECT_H

struct DiligentRenderWorld;
struct SceneDesc;
struct ViewParams;

class LTObject;
class WorldModelInstance;

/// Resolves the render-world for a world-model instance.
DiligentRenderWorld* diligent_find_world_model(const WorldModelInstance* instance);
/// Returns true if the given object should be processed for rendering.
bool diligent_should_process_model(LTObject* object);

/// Draws all scene models for the current view.
bool diligent_draw_models(SceneDesc* desc);
/// Collects world-model instances from the scene.
void diligent_collect_world_models(SceneDesc* desc);
/// \brief Collects visible world render blocks for the current view.
/// \details Call this after updating view params and before drawing world blocks.
/// \code
/// diligent_collect_visible_render_blocks(g_diligent_state.view_params);
/// \endcode
void diligent_collect_visible_render_blocks(const ViewParams& params);
/// Collects dynamic lights affecting the scene for the current view.
void diligent_collect_scene_dynamic_lights(SceneDesc* desc, const ViewParams& params);

#endif
