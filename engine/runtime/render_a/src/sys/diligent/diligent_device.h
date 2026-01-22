#ifndef LTJS_DILIGENT_DEVICE_H
#define LTJS_DILIGENT_DEVICE_H

#include "ltbasedefs.h"

namespace Diligent
{
	class ITextureView;
}

struct RenderStructInit;

Diligent::ITextureView* diligent_get_active_render_target();
Diligent::ITextureView* diligent_get_active_depth_target();

void diligent_UpdateWindowHandles(const RenderStructInit* init);
bool diligent_QueryWindowSize(uint32& width, uint32& height);
void diligent_UpdateScreenSize(uint32 width, uint32 height);

bool diligent_EnsureDevice();
bool diligent_CreateSwapChain(uint32 width, uint32 height);
bool diligent_EnsureSwapChain();
float diligent_get_swapchain_output_is_srgb();

#endif
