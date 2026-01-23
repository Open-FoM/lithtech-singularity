/**
 * diligent_world_api.h
 *
 * This header defines the World API portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_WORLD_API_H
#define LTJS_DILIGENT_WORLD_API_H

#include "ltcodes.h"
#include "ltvector.h"

class ILTStream;

/// \brief Loads world rendering data from a stream.
/// \details This populates the render-world data structures used by the
///          Diligent backend for world geometry, lightmaps, and occluders.
/// \code
/// std::unique_ptr<ILTStream> stream = OpenWorldStream();
/// if (!diligent_LoadWorldData(stream.get())) { /* handle error */ }
/// \endcode
bool diligent_LoadWorldData(ILTStream* stream);
/// \brief Overrides the color for a light group by ID.
/// \details This is typically driven by dynamic lighting or editor overrides.
bool diligent_SetLightGroupColor(uint32 id, const LTVector& color);

/// \brief Enables/disables a world occluder by ID.
LTRESULT diligent_SetOccluderEnabled(uint32 id, bool enabled);
/// \brief Queries whether a world occluder is enabled.
LTRESULT diligent_GetOccluderEnabled(uint32 id, bool* enabled);

/// \brief Resolves a texture-effect variable ID by name and stage.
/// \details Use the returned ID when updating texture-effect variables at runtime.
uint32 diligent_GetTextureEffectVarID(const char* name, uint32 stage);
/// \brief Sets a texture-effect variable value.
bool diligent_SetTextureEffectVar(uint32 var_id, uint32 var, float value);

/// \brief Returns true if the object group is enabled for rendering.
bool diligent_IsObjectGroupEnabled(uint32 group_id);
/// \brief Enables/disables rendering for a single object group.
void diligent_SetObjectGroupEnabled(uint32 group_id, bool enabled);
/// \brief Enables rendering for all object groups.
void diligent_SetAllObjectGroupEnabled();

#endif
