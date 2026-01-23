/**
 * diligent_debug_draw.h
 *
 * This header defines the Debug Draw portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef LTJS_DILIGENT_DEBUG_DRAW_H
#define LTJS_DILIGENT_DEBUG_DRAW_H

#include "ltbasedefs.h"
#include "renderstruct.h"

struct DiligentWorldConstants;
struct DiligentWorldVertex;

enum DiligentWorldBlendMode : uint8;
enum DiligentWorldDepthMode : uint8;

/// Draws line-list vertices using world constants and blend/depth modes.
bool diligent_draw_line_vertices(
	const DiligentWorldConstants& constants,
	DiligentWorldBlendMode blend_mode,
	DiligentWorldDepthMode depth_mode,
	const DiligentWorldVertex* vertices,
	uint32 vertex_count);

/// Draws all queued debug lines.
bool diligent_draw_debug_lines();

/// Returns true if world debug drawing is enabled.
bool diligent_world_debug_enabled();
/// Returns true if line debug drawing is enabled.
bool diligent_debug_lines_enabled();

/// Builds an LTRGBColor suitable for debug drawing.
LTRGBColor diligent_make_debug_color(uint8 r, uint8 g, uint8 b, uint8 a = 255);
/// Clears all queued debug lines.
void diligent_debug_clear_lines();
/// \brief Adds a single line segment to the debug draw list.
/// \code
/// diligent_debug_add_line({0,0,0}, {100,0,0}, diligent_make_debug_color(255,0,0));
/// \endcode
void diligent_debug_add_line(const LTVector& from, const LTVector& to, const LTRGBColor& color);
/// Adds an axis-aligned bounding box to the debug draw list.
void diligent_debug_add_aabb(const LTVector& center, const LTVector& dims, const LTRGBColor& color);
/// Adds world debug lines (from world geometry).
void diligent_debug_add_world_lines();
/// Adds an oriented bounding box to the debug draw list.
void diligent_debug_add_obb(const ModelOBB& obb, const LTRGBColor& color);

/// Releases debug draw resources and clears state.
void diligent_debug_term();

#endif
