# Debug Cleanup Notes (DEdit2/Diligent)

Purpose: catalog debug tooling wired into the DEdit2 console and the Diligent world renderer so we can later move this out of the main render path (planned `diligent_debug.h/.cpp` split).

## Console Commands (DEdit2 -> Diligent)
All commands below are registered in `tools/DEdit2/dedit2_concommand.cpp` and call into Diligent APIs in `engine/runtime/render_a/src/sys/diligent/diligent_render.h/.cpp`.

### Texture inspection
- `listtextures`
- `texprobe <texture_name>` -> `TextureCache::GetTextureDebugInfo`, `diligent_GetTextureDebugInfo`
- `texview <texture_name|off>` -> texture view preview (global `g_TexView`)

### Fog debug
- `fogdebug` -> `diligent_GetFogDebugInfo`
- `fogenable <0|1>` -> `diligent_SetFogEnabled`

### World stats and dumps
- `worldcolorstats` -> `diligent_GetWorldColorStats`
- `worldtexstats` -> `diligent_GetWorldTextureStats`
- `worlduvstats` -> `diligent_GetWorldUvStats`
- `worldpipestats` -> `diligent_GetWorldPipelineStats`
- `worldpipereset` -> `diligent_ResetWorldPipelineStats`
- `worldshaderdump <0=textured|1=lightmap|2=lightmap_only|3=dual|4=lightmap_dual>` -> `diligent_DumpWorldShaderResources`
- `worldtexdump [limit]` -> `diligent_DumpWorldTextureBindings`
- `worldbinddump [section_index]` -> `diligent_RequestWorldBindDump` / `diligent_RequestWorldBindDumpForSection`
- `worldsection <index>` -> `diligent_DumpWorldSectionInfo`
- `worlddebugstate` -> `diligent_DumpWorldDebugState`
- `worldshaderreset` -> `diligent_ResetWorldShaders`
- `worldtexrefresh` -> `diligent_RefreshWorldTextures`

### World debug toggles
- `worldforcewhite <0|1>` -> `diligent_SetForceWhiteVertexColor` + `diligent_InvalidateWorldGeometry`
- `worlduvdebug <0|1>` -> `diligent_SetWorldUvDebug`
- `worlduv1debug <0|1>` -> `diligent_SetWorldUv1Debug`
- `worldtextureddebug <0|1>` -> alias of `worlduvdebug`
- `worldbaseuv <0|1>` -> `diligent_SetWorldBaseUvDebug`
- `worldfullbright <0|1>` -> `diligent_SetWorldFullbright`
- `worldlightdebug <0|1|2|3>` -> `diligent_SetWorldLightDebugMode`
- `worldtexdebug <0|1|2|3|4>` -> `diligent_SetWorldTexDebugMode`
- `worldpsdebug <0|1|2>` -> `diligent_SetWorldPsDebug`
- `worldtexeluv <0|1>` -> `diligent_SetWorldTexelUV`
- `worldtexfx <0|1>` -> `diligent_SetWorldTexEffectsEnabled`
- `worldimsamplers <0|1>` -> `diligent_SetWorldUseImmutableSamplers`
- `worldcombsamplers <0|1>` -> `diligent_SetWorldUseCombinedSamplers`
- `worldbasevertex <0|1>` -> `diligent_SetWorldUseBaseVertex`
- `worldtexoverride <texture_name|off>` -> `diligent_SetWorldTextureOverride`
- `worldforcetexture <0|1>` -> `diligent_SetForceTexturedWorld`
- `worldforcewhitetex <0|1>` -> `diligent_SetWorldForceWhiteTexture`
- `worldforcechecker <0|1>` -> `diligent_SetWorldForceCheckerTexture`
- `worldforcelm <0|1>` -> `diligent_SetWorldForceNoLightmap`
- `worldforcewhitelm <0|1>` -> `diligent_SetWorldForceWhiteLightmap`
- `worldfallbackfullbright <0|1>` -> `diligent_SetWorldFallbackFullbright`

## Diligent Render Touchpoints
These are the main areas in `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp` where debug state touches the render path.

### Debug flags / globals
- `g_diligent_world_*` flags (UV debug, tex debug, fullbright, light debug, immutable samplers, force textures, force lightmaps, fallback fullbright, bind dump index, etc.).

### Shader/pipeline selection
- `diligent_get_world_tex_debug_value()` controls debug shading via `fog_params[3]`.
- `diligent_get_world_pixel_shader(...)` uses `g_diligent_world_ps_debug`.
- `diligent_get_world_pipeline(...)` uses debug shader modes + immutable sampler toggles.

### Texture binding overrides / debug textures
- `diligent_get_debug_white_texture_view()` + `diligent_get_debug_checker_texture_view()`.
- Per-section binding logic uses:
  - `g_diligent_world_force_white_texture`
  - `g_diligent_world_force_checker_texture`
  - `g_diligent_world_texture_override`
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
