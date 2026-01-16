# Diligent Screen Glow Handoff

## Summary

Screen glow is fully implemented in the Diligent renderer. The pipeline renders glow content to an offscreen target, applies a separable Gaussian blur, and composites the result additively over the main scene. The implementation includes model render-style remapping with `MHF_NOGLOW` support and debug overlays for the glow texture and blur kernel.

This document is intended to hand off the current state, key implementation details, and remaining gaps.

## Scope of Work Completed

- Offscreen glow render targets with SRV access for blur/composite.
- Gaussian filter kernel generation driven by console variables.
- World glow rendering (fullbright sections only).
- Model glow rendering with render-style remapping and `MHF_NOGLOW` handling.
- Blur passes (horizontal and vertical) using additive quads.
- Additive composite to the main backbuffer.
- Debug overlays for glow texture and filter visualization.

## Key Files Touched

- `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp`
  - Main glow pipeline, model glow path, debug overlays, model hook handling, and helper utilities.
- `engine/runtime/render_a/src/sys/diligent/rendertarget.h`
  - Added SRV accessor so render targets can be sampled during blur/composite.
- `engine/runtime/render_a/src/sys/diligent/rendertarget.cpp`
  - SRV creation for render target textures.
- `engine/runtime/render_a/src/sys/d3d/renderstylemap.h`
  - `CRenderStyleMap` interface used by glow render-style mapping.
- `engine/runtime/render_a/src/sys/d3d/renderstylemap.cpp`
  - D3D render-style map implementation (no-glow style, mapping rules).
- `engine/runtime/render_a/src/sys/d3d/rendererconsolevars.h`
  - Glow-related console variables (shared by Diligent).
- `engine/sdk/inc/ltbasedefs.h`
  - `MHF_NOGLOW` flag definition.
- `TODO.md`
  - Tracking for glow work completion and documentation.

## Implementation Details

### 1) Render Targets and State

- `diligent_glow_ensure_targets()` creates or recreates glow/blur `CRenderTarget` instances.
- Render target sizes are truncated to a power of two and clamped to `[16, 512]`.
- Render targets are created with `RTFMT_X8R8G8B8` color and `STFMT_D24S8` depth.
- `CRenderTarget` now exposes `GetShaderResourceView()` so the blur and composite passes can sample the rendered glow.

### 2) Gaussian Filter

- `diligent_glow_update_filter()` builds the kernel using:
  - `ScreenGlowFilterSize`
  - `ScreenGlowGaussAmp0/1`
  - `ScreenGlowGaussRadius0/1`
  - `ScreenGlowPixelShift`
- Elements with weight below `0.01f` are dropped.
- The filter list is padded to a multiple of four elements to mirror D3D batching alignment.

### 3) Glow Render Pass Flow

`diligent_render_screen_glow()` orchestrates the effect:

1. **Setup + Validation**
   - Early-out when glow is disabled.
   - Ensure targets and filter are valid.
   - Capture `g_ViewParams` and visible render blocks.

2. **Glow View Setup**
   - Builds a glow-specific frustum via `d3d_InitFrustum` with `ViewParams::eRenderMode_Glow`.
   - Uses glow fog CVars (`ScreenGlowFogEnable/NearZ/FarZ`) and black fog color.

3. **World Glow**
   - `diligent_draw_world_glow()` renders fullbright world sections to the glow target.

4. **Model Glow**
   - `diligent_draw_models_glow()` renders models using render-style remapping.
   - `g_diligent_glow_mode` toggles fog overrides in model constants.

5. **Blur Passes**
   - `diligent_glow_render_blur_pass()` runs a horizontal pass to `blur_target` then a vertical pass back to `glow_target`.
   - Default path batches 8 taps per fullscreen quad via a dedicated blur shader (`ScreenGlowBlurMultiTap 1`).
   - The legacy path draws one quad per kernel element (`ScreenGlowBlurMultiTap 0`).

6. **Composite**
   - The glow texture is additively blended onto the backbuffer.
   - UVs are offset by one texel to match D3D’s pixel center bias.

7. **Debug Overlays**
   - Optional overlays are drawn for the glow texture and filter chart.

### 4) Model Render-Style Mapping + `MHF_NOGLOW`

- `diligent_get_model_hook_data()` mirrors D3D’s `CallModelHook` pattern using the current `SceneDesc`.
- `diligent_draw_model_instance_with_render_style_map()` checks `MHF_NOGLOW` via hook data:
  - If set, uses `render_style_map.GetNoGlowRenderStyle()`.
  - Otherwise uses `render_style_map.MapRenderStyle()`.
- This ensures no-glow models render with the no-glow render style even during the glow pass.

### 5) Debug Overlays

- `ScreenGlowShowTexture` displays the unblurred glow target in the top-left.
- `ScreenGlowShowFilter` draws a bar chart of the current Gaussian kernel.
- Helpers:
  - `diligent_glow_draw_debug_quad()` renders arbitrary debug rectangles.
  - `diligent_glow_draw_debug_texture()` draws the glow texture overlay.
  - `diligent_glow_draw_debug_filter()` draws the kernel visualization.
- `diligent_get_debug_white_texture_view()` lazily creates a 1×1 white texture for untextured overlay fills.

### 6) SceneDesc Scope

- `DiligentSceneDescScope` tracks the active `SceneDesc` in `g_diligent_scene_desc` during glow rendering.
- This enables model hook callbacks (`SceneDesc::m_ModelHookFn`) and access to `m_GlobalModelLightAdd` for glow models.

## Console Variables (Glow)

### Enable / Debug
- `ScreenGlowEnable`
- `ScreenGlowShowTexture`
- `ScreenGlowShowTextureScale`
- `ScreenGlowShowFilter`
- `ScreenGlowShowFilterScale`
- `ScreenGlowShowFilterRange`

### Texture / Filter
- `ScreenGlowTextureSize`
- `ScreenGlowUVScale`
- `ScreenGlowFilterSize`
- `ScreenGlowGaussAmp0`
- `ScreenGlowGaussRadius0`
- `ScreenGlowGaussAmp1`
- `ScreenGlowGaussRadius1`
- `ScreenGlowPixelShift`
- `ScreenGlowBlurMultiTap`

### Fog
- `ScreenGlowFogEnable`
- `ScreenGlowFogNearZ`
- `ScreenGlowFogFarZ`

### D3D-only (not used by Diligent)
- `ScreenGlowEnablePS`

## Verification Status

- No automated tests were run.
- `lsp_diagnostics` cannot run for `.cpp` because no C++ LSP is configured in this environment.

## Known Gaps / Next Steps

1. **Shader Parity**
   - D3D’s `CRenderShader_Glow` does not have a direct Diligent equivalent. Confirm world glow appearance matches D3D’s fullbright-only path.

2. **Canvas Glow**
   - D3D has `ScreenGlow_DrawCanvases` (local CVar in D3D screen glow manager). Diligent does not implement this path yet.

3. **Performance**
   - The blur pass draws a quad per kernel element. D3D batches via fixed texture limits; consider batching if performance becomes an issue.

4. **Debug Overlay Placement**
   - Overlays are drawn relative to `g_ViewParams.m_fScreenWidth/Height`. If multiple viewports are used, confirm positioning is correct.

## Quick Manual Test Checklist

1. Enable glow:
   - `ScreenGlowEnable 1`
2. Show the glow texture overlay:
   - `ScreenGlowShowTexture 1`
   - Adjust `ScreenGlowShowTextureScale`
3. Show the filter chart:
   - `ScreenGlowShowFilter 1`
   - Adjust `ScreenGlowShowFilterScale` and `ScreenGlowShowFilterRange`
4. Verify `MHF_NOGLOW`:
   - Set model hook flag and confirm render-style mapping uses no-glow style.

## Change Tracking

- Completion recorded in `TODO.md` under Phase 6 (World & Model Rendering).
