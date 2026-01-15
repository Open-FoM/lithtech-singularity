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
- [x] Target backends: Vulkan (Windows/Linux) + Metal (macOS); optional D3D11 for Windows debugging.
- [ ] Decide Diligent integration strategy (submodule vs vendored vs external build).
- [ ] Define shader strategy: reuse existing HLSL + add translations where needed.
- [ ] Define new renderer-facing interface (replace `GetD3DDevice`).

---

## Phase 0 — Preparation & Inventory
- [ ] Confirm all DX9 touchpoints beyond `sys/d3d` (grep for `IDirect3DDevice9`, `d3d9.h`, `D3D*`).
- [ ] Identify all RenderStruct callbacks used by client/kernel and map to Diligent equivalents.
- [ ] Enumerate D3D-specific data structures that must be replaced (renderstruct, formats, device caps).

## Phase 1 — Build & Dependency Integration
- [ ] Add Diligent Engine to the build system (CMake include paths, libs, compile defs).
- [ ] Add Diligent include directories where renderer code lives.
- [ ] Validate Diligent sample code compiles within the repo toolchain.

## Phase 2 — Renderer Interface Redesign
- [ ] Replace `RenderStruct::GetD3DDevice` with new accessors (e.g., `GetRenderDevice`, `GetDeviceContext`, `GetSwapChain`).
- [ ] Remove D3D types from `renderstruct.h` and replace with engine-defined types.
- [ ] Update kernel/client consumers of RenderStruct to use new accessors.

## Phase 3 — Diligent RenderDLL Skeleton
- [ ] Create new renderer implementation directory (e.g., `sys/diligent/` or replace `sys/d3d`).
- [ ] Implement `rdll_RenderDLLSetup` to wire Diligent-backed callbacks.
- [ ] Implement `Init/Term` using Diligent device+swapchain creation.
- [ ] Implement `SwapBuffers`, `Clear`, `Start3D/End3D` using Diligent context.
- [ ] Define device-loss / window-resize handling strategy (swapchain recreation).

## Phase 4 — Core Rendering Pipeline
- [ ] Implement PSO cache (keyed by render state + shader + input layout).
- [ ] Implement SRB creation + resource binding strategy.
- [ ] Implement texture/buffer creation + lifetime management.
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
- [ ] Define the new RenderStruct accessors and update all call sites.
- [ ] Scaffold Diligent renderer with minimal swapchain + clear.
