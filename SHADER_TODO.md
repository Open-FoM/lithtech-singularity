# Shader TODO (Diligent)

## Summary
The current Diligent world shaders are intentionally minimal: they apply texture script UV transforms, sample textures, and apply basic fog/modulation. This is enough to unblock rendering with texture effects, but it does **not** fully match the legacy D3D fixed‑function pipeline behavior. If we want visual parity with the original renderer, each shader path needs to be refined to emulate the original stage states, blend modes, and per‑pass quirks.

## What is in place now
- Texture script transforms are applied per stage in the vertex shader.
- UV selection (UV0 vs UV1) is wired per pipeline mode.
- Reflection/normal/position inputs are generated in view space and can drive texgen.
- Pixel shaders do simple modulate chains (base * lightmap * dual) with fog.
- Bump, volume effect, and dynamic light paths are stubbed to basic texture modulation.

## Known gaps vs fixed‑function D3D
These are the main differences that will affect parity with the D3D renderer:

1) **Exact stage ops and args**
   - D3D uses per‑stage ColorOp/AlphaOp and Arg1/Arg2 (e.g., SELECTARG1, MODULATE, ADDSIGNED).
   - Many paths use stage‑specific state in D3D (e.g., detail add vs modulate, env map blend). Our shaders currently do a single multiply chain.

2) **Alpha test and texture factor usage**
   - D3D paths use alpha testing, alpha ref, and texture factor (TextureFactor) in multiple passes.
   - These are not modeled in the current shaders.

3) **Multiple pass behavior and stateful blending**
   - Some D3D renderers rely on multi‑pass blending and render‑state changes between passes.
   - Current shaders assume single‑pass blending per pipeline.

4) **Texcoord index routing**
   - D3D sometimes overrides TEXCOORDINDEX (e.g., detail/dual uses UV1 even for stage 0 in some passes).
   - We map UV1 for lightmap/dual stages, but not all D3D corner cases are represented.

5) **Bump/env map behavior**
   - D3D uses BUMPENVMAP and environment map blend ops.
   - Current bump shader just modulates base and bump textures.

6) **Dynamic lighting pass**
   - D3D dynamic light path uses additive lighting with specific blend behavior.
   - Current shader approximates with a simple distance falloff.

7) **Shadow projection and volume effects**
   - D3D shadow projection uses specific projection and blend settings.
   - Volume effects have multiple lighting modes and particle behaviors not fully represented.

## Concrete follow‑ups
To reach parity, do the following per pipeline mode:

- **Textured / GlowTextured**
  - Match D3D color/alpha ops and any alpha test behavior.

- **Lightmap / LightmapOnly / LightmapDual**
  - Match D3D lightmap blending, fog fixes, and any saturation adjustments.

- **DualTexture / Detail**
  - Implement D3D detail texture add vs modulate and use correct UV routing.

- **Bump / EnvMap**
  - Implement bump env mapping and env map blend ops.

- **DynamicLight**
  - Match D3D light pass blending and attenuation.

- **Shadow projection**
  - Implement the proper projective texture and blending that D3D uses.

## Suggested reference points in the legacy D3D renderer
Look at the following for authoritative behavior:
- `d3d_rendershader_gouraud.cpp`
- `d3d_rendershader_lightmap.cpp`
- `d3d_rendershader_glow.cpp`
- `d3d_rendershader_dynamiclight.cpp`

These files define the exact stage states and blend operations that the Diligent shaders should mimic.
