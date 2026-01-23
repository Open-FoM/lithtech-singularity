/**
 * diligent_renderer_lifecycle.h
 *
 * This header defines the Renderer Lifecycle portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_RENDERER_LIFECYCLE_H
#define LTJS_DILIGENT_RENDERER_LIFECYCLE_H

struct RenderStructInit;

/// \brief Initializes the renderer and creates device/swapchain state.
/// \details Called from the engine render DLL entry points during startup.
/// \code
/// RenderStructInit init{};
/// // ... fill init ...
/// int result = diligent_Init(&init);
/// \endcode
int diligent_Init(RenderStructInit* init);
/// \brief Shuts down the renderer.
/// \details If \p full is true, all cached resources are released; otherwise
///          only per-session resources are torn down.
void diligent_Term(bool full);

#endif
