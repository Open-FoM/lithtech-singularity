#ifndef LTJS_DILIGENT_UTILS_H
#define LTJS_DILIGENT_UTILS_H

#include "ltbasedefs.h"
#include "ltrotation.h"
#include "ltvector.h"

#include <array>

struct LTMatrix;

uint64 diligent_hash_combine(uint64 seed, uint64 value);
uint32 diligent_float_bits(float value);

void diligent_to_clip_space(float x, float y, float& out_x, float& out_y);
bool diligent_to_view_clip_space(float x, float y, float& out_x, float& out_y);

void diligent_store_matrix_from_lt(const LTMatrix& matrix, std::array<float, 16>& out);
void diligent_store_identity_matrix(std::array<float, 16>& out);

LTMatrix diligent_build_transform(const LTVector& position, const LTRotation& rotation, const LTVector& scale);

void diligent_set_viewport(float width, float height);
void diligent_set_viewport_rect(float left, float top, float width, float height);

#endif
