#ifndef LTJS_DILIGENT_DEBUG_DRAW_H
#define LTJS_DILIGENT_DEBUG_DRAW_H

#include "ltbasedefs.h"
#include "renderstruct.h"

struct DiligentWorldConstants;
struct DiligentWorldVertex;

enum DiligentWorldBlendMode : uint8;
enum DiligentWorldDepthMode : uint8;

bool diligent_draw_line_vertices(
	const DiligentWorldConstants& constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	const DiligentWorldVertex* vertices,
	uint32 vertex_count);

bool diligent_draw_debug_lines();

bool diligent_world_debug_enabled();
bool diligent_debug_lines_enabled();

LTRGBColor diligent_make_debug_color(uint8 r, uint8 g, uint8 b, uint8 a = 255);
void diligent_debug_clear_lines();
void diligent_debug_add_line(const LTVector& from, const LTVector& to, const LTRGBColor& color);
void diligent_debug_add_aabb(const LTVector& center, const LTVector& dims, const LTRGBColor& color);
void diligent_debug_add_world_lines();
void diligent_debug_add_obb(const ModelOBB& obb, const LTRGBColor& color);

void diligent_debug_term();

#endif
