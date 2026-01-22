#ifndef LTJS_DILIGENT_GLOW_BLUR_H
#define LTJS_DILIGENT_GLOW_BLUR_H

#include "ltbasedefs.h"
#include "renderstruct.h"

#include <array>

namespace Diligent
{
	class ITextureView;
}

constexpr uint32 kDiligentGlowBlurBatchSize = 8;

struct DiligentGlowBlurConstants
{
	std::array<std::array<float, 4>, kDiligentGlowBlurBatchSize> taps{};
	float params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DiligentGlowBlurPipeline;

DiligentGlowBlurPipeline* diligent_get_glow_blur_pipeline(LTSurfaceBlend blend);
bool diligent_update_glow_blur_constants(const DiligentGlowBlurConstants& constants);
bool diligent_draw_glow_blur_quad(
	DiligentGlowBlurPipeline* pipeline,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target);

#endif
