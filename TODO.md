# Rendering Infrastructure Upgrade Plan (DX9 → Diligent Engine)

## Scope
- **Replace** all DX9 renderer code under:
  - `engine/runtime/ui/src/sys/d3d/`
  - `engine/runtime/model/src/sys/d3d/`
  - `engine/runtime/render_a/src/sys/d3d/`
  - `engine/runtime/render_b/src/sys/d3d/`
- **Remove** DX9 types/usage across runtime (e.g., `IDirect3DDevice9`, `D3DFORMAT`, `d3d9.h`).
- **No phased migration**: full cutover to Diligent Engine.

## Key Entry Points (current)
- Render DLL setup: `engine/runtime/render_a/src/sys/d3d/common_init.cpp` (`rdll_RenderDLLSetup`)
- Render init/term: `engine/runtime/kernel/src/sys/win/render.cpp` (`r_InitRender`, `r_TermRender`)
- RenderStruct API: `engine/runtime/shared/src/sys/win/renderstruct.h`
- Device/swapchain: `engine/runtime/render_a/src/sys/d3d/d3d_device.*`, `d3d_shell.h`
- DrawPrim: `engine/runtime/render_b/src/sys/d3d/d3ddrawprim.*`
- Video managers and tools depend on `GetD3DDevice()` and D3D types:
  - `engine/runtime/kernel/src/sys/win/binkvideomgrimpl.cpp`
  - `engine/runtime/kernel/src/sys/win/dshowvideomgrimpl.cpp`
  - `engine/runtime/kernel/src/sys/win/ltjs_ffmpeg_video_mgr_impl.cpp`

## Diligent Engine Mapping (target)
- `IDirect3DDevice9` → `IRenderDevice` + `IDeviceContext`
- Swap buffers → `ISwapChain::Present`
- Render state → `IPipelineState` (PSO)
- Texture binding → `IShaderResourceBinding` (SRB) + `CommitShaderResources`
- Buffers → `IRenderDevice::CreateBuffer` with explicit usage/bind flags

## Decisions to Lock Early
- [x] Target backends: Vulkan (Windows/Linux) + Metal (macOS; requires commercial Diligent license) with Vulkan/MoltenVK fallback; optional D3D11 for Windows debugging.
- [x] Decide Diligent integration strategy: use git submodule under `libs/diligent/` and add via CMake.
- [x] Define shader strategy: keep HLSL sources; compile via Diligent `ShaderCreateInfo` (`SHADER_SOURCE_LANGUAGE_HLSL`), add shim shaders for fixed‑function paths as needed.
- [x] Define new renderer-facing interface: replace `GetD3DDevice` with `GetRenderDevice`, `GetImmediateContext`, `GetSwapChain` returning Diligent interfaces.

---

## Phase 0 — Preparation & Inventory
- [x] Confirm all DX9 touchpoints beyond `sys/d3d` (grep for `IDirect3DDevice9`, `d3d9.h`, `D3D*`).
- [x] Identify all RenderStruct callbacks used by client/kernel and map to Diligent equivalents.
- [x] Enumerate D3D-specific data structures that must be replaced (renderstruct, formats, device caps).

### Inventory Findings (initial)
- `engine/runtime/shared/src/sys/win/renderstruct.h`: `d3d9.h` include, `GetD3DDevice`, `D3DFORMAT` return types.
- `engine/runtime/shared/src/sys/win/d3dddstructs.h`: D3D9 type definitions.
- `engine/runtime/kernel/src/sys/win/binkvideomgrimpl.cpp`: `IDirect3DDevice9`, `IDirect3DSurface9`, `D3DFORMAT`, `GetD3DDevice`.
- `engine/runtime/kernel/src/sys/win/dshowvideomgrimpl.cpp`: `IDirect3DDevice9`, `IDirect3DSurface9`, `GetD3DDevice`.
- `engine/runtime/kernel/src/sys/win/ltjs_ffmpeg_video_mgr_impl.cpp`: `GetD3DDevice` usage.
- `engine/runtime/kernel/src/sys/win/ltrendermgr_impl.cpp`: D3D surfaces + `D3DFORMAT` usage.
- `engine/runtime/client/src/sys/win/winclientde_impl.cpp`: `d3d9.h` include + device caps queries.
- `engine/runtime/shared/src/dtxmgr.cpp`: `GetTextureDDFormat2` + `ConvertTexDataToDD` usage (texture conversion path).

## Phase 1 — Build & Dependency Integration
- [x] Add Diligent Engine to the build system (CMake include paths, libs, compile defs).
- [x] Add Diligent include directories where renderer code lives.
- [x] Validate Diligent sample code compiles within the repo toolchain (built `Tutorial01_HelloTriangle`).

## Phase 2 — Renderer Interface Redesign
- [x] Replace `RenderStruct::GetD3DDevice` with new accessors (D3D handle compiled only for D3D builds; Diligent builds gate D3D-only video paths).
- [x] Remove D3D types from `renderstruct.h` and replace with engine-defined types.
- [x] Update kernel/client consumers of RenderStruct to handle the new device handle signature.

## Phase 3 — Diligent RenderDLL Skeleton
- [x] Create new renderer implementation directory (`engine/runtime/render_a/src/sys/diligent`).
- [x] Implement `rdll_RenderDLLSetup` to wire Diligent-backed callbacks.
- [x] Implement `Init/Term` using Diligent device+swapchain creation.
- [x] Implement `SwapBuffers`, `Clear`, `Start3D/End3D` using Diligent context.
- [x] Add `LTJS_USE_DILIGENT_RENDER` CMake option and conditional link.
- [x] Define device-loss / window-resize handling strategy (swapchain recreation on missing swapchain + resize on size change).

## Phase 4 — Core Rendering Pipeline
- [x] Implement PSO cache (keyed by render state + shader + input layout) with Diligent-side key+cache scaffolding.
- [x] Implement SRB creation + resource binding strategy with Diligent-side SRB cache and texture-stage resolution.
- [x] Implement texture/buffer creation + lifetime management with Diligent SharedTexture-backed GPU texture creation and release hooks.
- [ ] Implement render target management (`rendertarget.*`) using Diligent textures/views.

## Phase 5 — DrawPrim & 2D/Overlay
- [ ] Rewrite `render_b` DrawPrim to use Diligent draw calls and PSOs.
- [ ] Replace D3D fixed-function states with equivalent shader paths.
- [ ] Implement optimized 2D path (if needed) using a dedicated PSO.

## Phase 6 — World & Model Rendering
- [ ] Port `render_a` draw path: world rendering, lighting, shadowing, fog, glow.
- [ ] Translate D3D shader usage to Diligent shader compilation (HLSL).
- [ ] Replace D3D vertex declarations with Diligent input layouts.
- [ ] Port model render objects and render styles to Diligent PSO/SRB.

## Phase 7 — UI + Video Managers
- [ ] Port `ui/src/sys/d3d` UI rendering to Diligent.
- [ ] Update video managers (Bink/DirectShow/FFmpeg) to use Diligent textures and uploads.
- [ ] Remove `IDirect3DDevice9` usage from video code.

## Phase 8 — Cleanup & Removal
- [ ] Remove DX9 headers, libs, and CMake targets for D3D.
- [ ] Delete `sys/d3d` source trees after Diligent renderer is functional.
- [ ] Purge D3D-specific renderstruct fields, helpers, and wrappers.

## Phase 9 — Validation & Stabilization
- [ ] Render a minimal scene (triangle) using Diligent.
- [ ] Render a full world + models with lighting and textures.
- [ ] Validate UI overlay and video playback.
- [ ] Verify performance and memory usage regressions.
- [ ] Run Vulkan validation layers (Win/Linux) and Metal API validation (macOS) during bring-up; triage GPU validation errors.

---

## Immediate Next Actions
- [x] Choose initial backends: Vulkan (Windows/Linux) + Metal (macOS); optional D3D11 for debugging.
- [x] Define the new RenderStruct accessors and update all call sites.
- [x] Scaffold Diligent renderer with minimal swapchain + clear.
