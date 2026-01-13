# LithTech Rendering Backend Abstraction Plan

CRITICAL: PICK THE FIRST UNFINISHED TASK FROM RENDER_BACKEND_TODO.md

## Purpose
Abstract the existing LithTech D3D9 rendering stack to support multiple backends (Vulkan, Metal, newer DirectX) while preserving current behavior and interfaces.

## Success Criteria
- **Functional**: D3D9 backend behavior remains unchanged; new backend can initialize and render a minimal scene via existing entry points.
- **Observable**: Frame snapshots match D3D9 baseline for the same scene; no init/teardown regressions.
- **Pass/Fail**: Backend selection produces a working device and a rendered frame without crashes.

## Current Integration Anchors (Baseline)
- `External/LithTech/engine/runtime/shared/src/sys/win/renderstruct.h` — engine↔renderer contract, lifecycle hooks, device accessors.
- `External/LithTech/engine/runtime/kernel/src/sys/win/render.cpp` — `r_InitRender` and renderer DLL setup.
- `External/LithTech/engine/runtime/render_a/src/sys/d3d/d3d_init.cpp` — D3D init and RenderStruct wiring.
- `External/LithTech/engine/runtime/render_a/src/sys/d3d/d3d_device.h` — `CD3D_Device` (core D3D device wrapper).
- `External/LithTech/engine/runtime/render_a/src/sys/d3d/d3d_renderworld.h` — `CD3D_RenderWorld` (world rendering pipeline).
- `External/LithTech/engine/runtime/render_a/src/sys/d3d/d3d_renderstyle.h` — `CD3DRenderStyle` (render style pipeline).
- SDK interfaces: `iltrendermgr.h`, `iltdrawprim.h`, `ilttexinterface.h`, `iltmodel.h`, `iltrenderstyles.h`.

## Phased Roadmap

see @RENDER_BACKEND_TODO.md

## Risks & Mitigations
- **RenderStruct D3D coupling**: Keep `GetD3DDevice` for legacy; add neutral accessor.
- **Render style D3D options**: Keep D3D options as backend extensions.
- **Render world tight coupling**: Introduce an interface wrapper; keep D3D as reference implementation.
- **ILT* interface expectations**: Preserve signatures; route to backend-neutral implementations.

## External Reference Patterns (Design Guidance)
- Godot `RenderingDeviceDriver` — driver layer separation.
- Diligent Engine — device/context split and shader resource binding model.
- O3DE RHI — factory-based backend selection.
- bgfx/wgpu — backend enum selection and adapter model.

