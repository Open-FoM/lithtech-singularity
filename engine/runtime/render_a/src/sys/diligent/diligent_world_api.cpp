#include "diligent_world_api.h"

#include "diligent_world_data.h"
#include "texturescriptvarmgr.h"

#include <memory>

bool diligent_LoadWorldData(ILTStream* stream)
{
	if (!stream)
	{
		return false;
	}

	g_render_world.reset();

	auto world = std::unique_ptr<DiligentRenderWorld>(new DiligentRenderWorld());
	if (!world->Load(stream))
	{
		return false;
	}

	g_render_world = std::move(world);
	return true;
}

bool diligent_SetLightGroupColor(uint32 id, const LTVector& color)
{
	if (!g_render_world)
	{
		return false;
	}

	g_render_world->SetLightGroupColor(id, color);
	return true;
}

LTRESULT diligent_SetOccluderEnabled(uint32, bool)
{
	return LT_NOTFOUND;
}

LTRESULT diligent_GetOccluderEnabled(uint32, bool*)
{
	return LT_NOTFOUND;
}

uint32 diligent_GetTextureEffectVarID(const char* name, uint32 stage)
{
	if (!name)
	{
		return 0;
	}

	return CTextureScriptVarMgr::GetID(name, stage);
}

bool diligent_SetTextureEffectVar(uint32 var_id, uint32 var, float value)
{
	return CTextureScriptVarMgr::GetSingleton().SetVar(var_id, var, value);
}

bool diligent_IsObjectGroupEnabled(uint32)
{
	return false;
}

void diligent_SetObjectGroupEnabled(uint32, bool)
{
}

void diligent_SetAllObjectGroupEnabled()
{
}
