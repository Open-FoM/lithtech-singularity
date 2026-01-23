/**
 * diligent_shadow_draw.h
 *
 * This header defines the Shadow Draw portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_SHADOW_DRAW_H
#define LTJS_DILIGENT_SHADOW_DRAW_H

struct ViewParams;
class ModelInstance;

/// Updates the list of shadow textures based on current scene state.
void diligent_update_shadow_texture_list();
/// Clears the queued model shadow list.
void diligent_clear_shadow_queue();
/// Queues a model instance for shadow rendering.
void diligent_queue_model_shadows(ModelInstance* instance);
/// Renders queued model shadows for the current view.
bool diligent_render_queued_model_shadows(const ViewParams& params);
/// Renders shadow projections for translucent world models.
bool diligent_draw_translucent_world_model_shadow_projection(const ViewParams& params);
/// Releases cached shadow textures.
void diligent_release_shadow_textures();

#endif
