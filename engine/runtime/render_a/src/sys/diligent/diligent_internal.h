#ifndef LTJS_DILIGENT_INTERNAL_H
#define LTJS_DILIGENT_INTERNAL_H

// Internal shared includes/types for renderer split files.
#include "diligent_state.h"
#include "ltbasedefs.h"

#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent
{
	class IDeviceContext;
	class IEngineFactoryVk;
	class IRenderDevice;
	class ISwapChain;
	class ITextureView;
}

struct RenderStruct;
struct SceneDesc;
struct ViewParams;
class ILTCommon;
class IWorldClientBSP;
class IWorldSharedBSP;

ILTCommon* diligent_get_common_client();
IWorldClientBSP* diligent_get_world_bsp_client();
IWorldSharedBSP* diligent_get_world_bsp_shared();

#endif
