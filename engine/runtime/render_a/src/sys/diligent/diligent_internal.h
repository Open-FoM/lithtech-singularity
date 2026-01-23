/**
 * diligent_internal.h
 *
 * This header defines the Internal portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_INTERNAL_H
#define LTJS_DILIGENT_INTERNAL_H

/// Internal shared includes/types for renderer split files.
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

/// Returns the common client interface (if available).
ILTCommon* diligent_get_common_client();
/// Returns the world client BSP interface (if available).
IWorldClientBSP* diligent_get_world_bsp_client();
/// Returns the shared BSP interface (if available).
IWorldSharedBSP* diligent_get_world_bsp_shared();

#endif
