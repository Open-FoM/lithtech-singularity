#include "diligent_state.h"

ILTCommon* diligent_get_common_client()
{
	return g_diligent_state.common_client;
}

IWorldClientBSP* diligent_get_world_bsp_client()
{
	return g_diligent_state.world_bsp_client;
}

IWorldSharedBSP* diligent_get_world_bsp_shared()
{
	return g_diligent_state.world_bsp_shared;
}

void diligent_SetExternalWorldBspModels(WorldBsp* const* models, uint32 count)
{
	g_diligent_state.external_world_bsp_models = models;
	g_diligent_state.external_world_bsp_model_count = count;
}
