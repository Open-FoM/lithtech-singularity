#ifndef LTJS_DILIGENT_POSTFX_H
#define LTJS_DILIGENT_POSTFX_H

struct SceneDesc;

float diligent_get_tonemap_enabled();

bool diligent_render_screen_glow(SceneDesc* desc);

bool diligent_AddGlowRenderStyleMapping(const char* source, const char* map_to);
bool diligent_SetGlowDefaultRenderStyle(const char* filename);
bool diligent_SetNoGlowRenderStyle(const char* filename);

void diligent_postfx_term();

#endif
