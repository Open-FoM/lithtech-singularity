# Debug Cleanup Notes (DEdit2/Diligent)

Purpose: catalog debug tooling wired into the DEdit2 console and the Diligent world renderer so we can later move this out of the main render path (planned `diligent_debug.h/.cpp` split).

## Console Commands (DEdit2 -> Diligent)
All commands below are registered in `tools/DEdit2/dedit2_concommand.cpp` and call into Diligent APIs or update renderer convars in `engine/runtime/render_a/src/sys/rendererconsolevars.h`.

### Texture inspection
- `listtextures`
- `texprobe <texture_name>` -> `TextureCache::GetTextureDebugInfo`, `diligent_GetTextureDebugInfo`
- `texview <texture_name|off>` -> texture view preview (global `g_TexView`)

### Fog debug
- `fogdebug` -> reads `g_CV_FogEnable`, `g_CV_FogNearZ`, `g_CV_FogFarZ`, `g_CV_FogColor*`
- `fogenable <0|1>` -> sets `g_CV_FogEnable`
- `fogrange <near> <far>` -> sets `g_CV_FogNearZ`/`g_CV_FogFarZ`

### World stats and dumps
- `worldcolorstats` -> `diligent_GetWorldColorStats`
- `worldtexstats` -> `diligent_GetWorldTextureStats`
- `worlduvstats` -> `diligent_GetWorldUvStats`
- `worldpipestats` -> `diligent_GetWorldPipelineStats`
- `worldpipereset` -> `diligent_ResetWorldPipelineStats`
- `worldtexdump [limit]` -> `diligent_DumpWorldTextureBindings`
- `worldshaderreset` -> `diligent_ResetWorldShaders`

### World debug toggles
- `worldforcewhite <0|1>` -> `diligent_SetForceWhiteVertexColor` + `diligent_InvalidateWorldGeometry`
- `worlduvdebug <0|1>` -> `g_CV_WorldUvDebug`
- `worldtextureddebug <0|1>` -> alias of `worlduvdebug`
- `worldtexdebug <0|1|2|3|4>` -> `g_CV_WorldTexDebugMode`
- `worldpsdebug <0|1|2>` -> `g_CV_WorldPsDebug`
- `worldtexeluv <0|1|2>` -> `g_CV_WorldTexelUV`
- `worldbasevertex <0|1>` -> `g_CV_WorldUseBaseVertex`
- `worldforcetexture <0|1>` -> `g_CV_WorldForceTexture`

## Diligent Render Touchpoints
These are the main areas in `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp` where debug state touches the render path.

### Debug flags / globals
- World debug flags now live primarily in renderer convars (`g_CV_World*`), plus remaining Diligent globals (force white/checker textures, lightmap overrides, bind dump index, etc.).

### Shader/pipeline selection
- `diligent_get_world_tex_debug_value()` controls debug shading via `fog_params[3]`.
- `diligent_get_world_pixel_shader(...)` uses `g_diligent_world_ps_debug`.
- `diligent_get_world_pipeline(...)` uses debug shader modes + immutable sampler toggles.

### Texture binding overrides / debug textures
- `diligent_get_debug_white_texture_view()` + `diligent_get_debug_checker_texture_view()`.
- Per-section binding logic uses:
  - `g_diligent_world_force_white_texture`
  - `g_diligent_world_force_checker_texture`
  - `g_diligent_world_force_no_lightmap`
  - `g_diligent_world_force_white_lightmap`
  - `g_diligent_world_fallback_fullbright`

### World bind dumps and section info
- Bind dump output gated by `g_diligent_world_bind_dump` + `g_diligent_world_bind_dump_index`.
- `diligent_DumpWorldSectionInfo` prints per-section shader/texture/lightmap info.

### Texture/UV stats and inspection helpers
- `diligent_GetWorldTextureStats`, `diligent_GetWorldUvStats`, `diligent_GetWorldColorStats`.
- `diligent_GetTextureDebugInfo` (paired with `texprobe`).

## Files to refactor later
- `tools/DEdit2/dedit2_concommand.cpp` (console wiring)
- `engine/runtime/render_a/src/sys/diligent/diligent_render.h` (debug API surface)
- `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp` (flags, debug shaders, bind dumps, overrides)
