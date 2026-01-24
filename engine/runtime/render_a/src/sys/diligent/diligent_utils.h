/**
 * diligent_utils.h
 *
 * This header defines the Utils portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_UTILS_H
#define LTJS_DILIGENT_UTILS_H

#include "ltbasedefs.h"
#include "ltrotation.h"
#include "ltvector.h"

#include <array>

struct LTMatrix;
namespace Diligent
{
	struct SamplerDesc;
}

/// Combines two values into a 64-bit hash.
uint64 diligent_hash_combine(uint64 seed, uint64 value);
/// Returns the raw bit pattern of a float.
uint32 diligent_float_bits(float value);

/// Converts pixel coordinates to clip-space [-1, 1] based on current viewport.
void diligent_to_clip_space(float x, float y, float& out_x, float& out_y);
/// Converts view coordinates to clip-space, returning false if viewport invalid.
bool diligent_to_view_clip_space(float x, float y, float& out_x, float& out_y);

/// Stores an LTMatrix into a column-major float array.
void diligent_store_matrix_from_lt(const LTMatrix& matrix, std::array<float, 16>& out);
/// Writes an identity matrix into a column-major float array.
void diligent_store_identity_matrix(std::array<float, 16>& out);

/// Builds a transform matrix from position, rotation, and scale.
LTMatrix diligent_build_transform(const LTVector& position, const LTRotation& rotation, const LTVector& scale);

/// Sets the viewport to cover the full render target.
void diligent_set_viewport(float width, float height);
/// Sets the viewport to a sub-rectangle of the render target.
void diligent_set_viewport_rect(float left, float top, float width, float height);

/// Returns the active anisotropy level (0 when disabled/unsupported).
uint8 diligent_get_anisotropy_level();
/// Applies anisotropic filtering to the sampler when enabled.
void diligent_apply_anisotropy(Diligent::SamplerDesc& desc);

#endif
