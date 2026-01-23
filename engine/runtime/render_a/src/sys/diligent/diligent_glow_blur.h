/**
 * diligent_glow_blur.h
 *
 * This header defines the Glow Blur portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_GLOW_BLUR_H
#define LTJS_DILIGENT_GLOW_BLUR_H

#include "ltbasedefs.h"
#include "renderstruct.h"

#include <array>

namespace Diligent
{
	class ITextureView;
}

/// Number of tap samples packed per blur batch.
constexpr uint32 kDiligentGlowBlurBatchSize = 8;

/// Constants buffer for glow blur passes.
struct DiligentGlowBlurConstants
{
	std::array<std::array<float, 4>, kDiligentGlowBlurBatchSize> taps{};
	float params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct DiligentGlowBlurPipeline;

/// Retrieves or creates the glow blur pipeline for the given blend mode.
DiligentGlowBlurPipeline* diligent_get_glow_blur_pipeline(LTSurfaceBlend blend);
/// Uploads glow blur constants to the GPU.
bool diligent_update_glow_blur_constants(const DiligentGlowBlurConstants& constants);
/// \brief Draws a fullscreen quad with glow blur shaders.
/// \details The caller is responsible for setting up targets and constants.
bool diligent_draw_glow_blur_quad(
	DiligentGlowBlurPipeline* pipeline,
	Diligent::ITextureView* texture_view,
	Diligent::ITextureView* render_target,
	Diligent::ITextureView* depth_target);

#endif
