#include "diligent_state.h"
#include "iltcommon.h"
#include "world_client_bsp.h"
#include "world_shared_bsp.h"

DiligentRenderState g_diligent_state;

define_holder_to_instance(ILTCommon, g_diligent_state.common_client, Client);
define_holder(IWorldClientBSP, g_diligent_state.world_bsp_client);
define_holder(IWorldSharedBSP, g_diligent_state.world_bsp_shared);
