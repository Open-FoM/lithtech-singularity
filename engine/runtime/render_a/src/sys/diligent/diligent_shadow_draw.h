#ifndef LTJS_DILIGENT_SHADOW_DRAW_H
#define LTJS_DILIGENT_SHADOW_DRAW_H

struct ViewParams;
class ModelInstance;

void diligent_update_shadow_texture_list();
void diligent_clear_shadow_queue();
void diligent_queue_model_shadows(ModelInstance* instance);
bool diligent_render_queued_model_shadows(const ViewParams& params);
bool diligent_draw_translucent_world_model_shadow_projection(const ViewParams& params);
void diligent_release_shadow_textures();

#endif
