#ifndef LTJS_DILIGENT_WORLD_API_H
#define LTJS_DILIGENT_WORLD_API_H

#include "ltcodes.h"
#include "ltvector.h"

class ILTStream;

bool diligent_LoadWorldData(ILTStream* stream);
bool diligent_SetLightGroupColor(uint32 id, const LTVector& color);

LTRESULT diligent_SetOccluderEnabled(uint32 id, bool enabled);
LTRESULT diligent_GetOccluderEnabled(uint32 id, bool* enabled);

uint32 diligent_GetTextureEffectVarID(const char* name, uint32 stage);
bool diligent_SetTextureEffectVar(uint32 var_id, uint32 var, float value);

bool diligent_IsObjectGroupEnabled(uint32 group_id);
void diligent_SetObjectGroupEnabled(uint32 group_id, bool enabled);
void diligent_SetAllObjectGroupEnabled();

#endif
