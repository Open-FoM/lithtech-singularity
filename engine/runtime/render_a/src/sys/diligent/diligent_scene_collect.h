#ifndef LTJS_DILIGENT_SCENE_COLLECT_H
#define LTJS_DILIGENT_SCENE_COLLECT_H

struct DiligentRenderWorld;
struct SceneDesc;
struct ViewParams;

class LTObject;
class WorldModelInstance;

DiligentRenderWorld* diligent_find_world_model(const WorldModelInstance* instance);
bool diligent_should_process_model(LTObject* object);

bool diligent_draw_models(SceneDesc* desc);
void diligent_collect_world_models(SceneDesc* desc);
void diligent_collect_visible_render_blocks(const ViewParams& params);
void diligent_collect_scene_dynamic_lights(SceneDesc* desc, const ViewParams& params);

#endif
