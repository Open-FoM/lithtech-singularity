/**
 * diligent_device.h
 *
 * This header defines the Device portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_DEVICE_H
#define LTJS_DILIGENT_DEVICE_H

#include "ltbasedefs.h"

namespace Diligent
{
	class ITextureView;
}

struct RenderStructInit;

/// \brief Returns the currently active render target (swapchain or override).
/// \details If an override is set via diligent_SetOutputTargets, it is returned.
Diligent::ITextureView* diligent_get_active_render_target();
/// \brief Returns the currently active depth target (swapchain or override).
/// \details If an override is set via diligent_SetOutputTargets, it is returned.
Diligent::ITextureView* diligent_get_active_depth_target();
/// \brief Returns sample count for the active render target (defaults to 1).
uint32 diligent_get_active_sample_count();

/// \brief Updates renderer window handles based on render init data.
/// \details Required before creating or resizing the swap chain.
void diligent_UpdateWindowHandles(const RenderStructInit* init);
/// \brief Queries the current drawable window size.
bool diligent_QueryWindowSize(uint32& width, uint32& height);
/// \brief Updates cached screen size and notifies dependent resources.
void diligent_UpdateScreenSize(uint32 width, uint32 height);

/// \brief Ensures the Diligent device is created and ready for use.
bool diligent_EnsureDevice();
/// \brief Creates the swap chain for the given dimensions.
bool diligent_CreateSwapChain(uint32 width, uint32 height);
/// \brief Ensures the swap chain is created and ready for use.
bool diligent_EnsureSwapChain();
/// \brief Returns 1.0f if the swap chain output format is sRGB, otherwise 0.0f.
float diligent_get_swapchain_output_is_srgb();

#endif
