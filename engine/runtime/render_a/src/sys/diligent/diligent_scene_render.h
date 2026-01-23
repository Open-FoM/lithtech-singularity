/**
 * diligent_scene_render.h
 *
 * This header defines the Scene Render portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_SCENE_RENDER_H
#define LTJS_DILIGENT_SCENE_RENDER_H

struct SceneDesc;

/// \brief Renders a scene described by \p desc.
/// \details This is the main per-frame render entry point. It performs
///          visibility collection, world/model draw passes, post effects,
///          and returns a RENDER_* status code.
/// \code
/// if (diligent_RenderScene(&scene_desc) != RENDER_OK) { /* handle error */ }
/// \endcode
int diligent_RenderScene(SceneDesc* desc);

#endif
