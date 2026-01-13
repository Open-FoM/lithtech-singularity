# LithTech Rendering Backend Abstraction TODO

## Overview
This TODO tracks the implementation of a multi-backend rendering abstraction layer for LithTech, enabling support for Vulkan, Metal, and modern DirectX while preserving existing D3D9 behavior.

**Reference Document**: `/RENDER_BACKEND_PLAN.md`

**Success Criteria**:
- D3D9 backend behavior remains unchanged
- New backend can initialize and render a minimal scene via existing entry points
- Frame snapshots match D3D9 baseline for the same scene
- Backend selection produces a working device and rendered frame without crashes

---

## Phase 0 — Architecture Inventory (No Behavior Changes)

**Goal**: Build a precise map of entry points, dependencies, and lifecycle.

### Tasks

- [ ] **Map RenderStruct Function Pointers to D3D Implementations**
  - Locate and analyze `renderstruct.h` in shared runtime
  - Document each RenderStruct function pointer and its D3D implementation
  - Create mapping table: RenderStruct function → D3D implementation file/function
  - Touch points: `./engine/runtime/shared/src/sys/win/renderstruct.h`, `./engine/runtime/render_a/src/sys/d3d/`
  - Deliverable: `Docs/./RENDER_ARCHITECTURE_INVENTORY.md`

- [ ] **Trace Renderer Init Flow**
  - Document complete initialization flow: `r_InitRender` → `rdll_RenderDLLSetup` → `d3d_Init`
  - Identify all DLL loading, device creation, and interface setup steps
  - Document error handling paths and failure modes
  - Touch points: `./engine/runtime/kernel/src/sys/win/render.cpp`, `./engine/runtime/render_a/src/sys/d3d/d3d_init.cpp`
  - Deliverable: Add init/teardown call graph to inventory document

- [ ] **Produce D3D Module Dependency Matrix**
  - Analyze all files in `./engine/runtime/render_a/src/sys/d3d/`
  - Map each module by responsibility (device management, world rendering, styles, textures, etc.)
  - Document inter-module dependencies (include files, function calls, data structures)
  - Touch points: All D3D source files and headers
  - Deliverable: Dependency matrix in inventory document

- [ ] **Document SDK Interface Contract**
  - Catalog all SDK interfaces: `iltrendermgr.h`, `iltdrawprim.h`, `ilttexinterface.h`, `iltmodel.h`, `iltrenderstyles.h`
  - Document current implementation expectations and return types
  - Touch points: `./engine/sdk/inc/`
  - Deliverable: Interface contract documentation

**Phase 0 Milestone**: Architecture inventory and init flow map (M1)

---

## Phase 1 — Backend-Neutral Renderer Contract

**Goal**: Introduce a backend-neutral interface without changing public APIs.

### Tasks

- [ ] **Define Backend-Neutral Renderer Interface**
  - Create `./engine/runtime/shared/src/sys/win/backend_interface.h`
  - Define abstract interface mirroring RenderStruct responsibilities
  - Include: device lifecycle, resource creation, rendering commands, state management
  - Preserve all existing signatures and behaviors
  - Deliverable: Backend interface header with pure virtual methods

- [ ] **Create Adapter Layer**
  - Implement `./engine/runtime/kernel/src/sys/win/backend_adapter.cpp`
  - Modify `render.cpp` to route RenderStruct calls through backend adapter
  - Keep RenderStruct stable (no signature changes)
  - Add backend-neutral native handle accessor alongside `GetD3DDevice`
  - Touch points: `./engine/runtime/kernel/src/sys/win/render.cpp`, `./engine/runtime/shared/src/sys/win/renderstruct.h`
  - Deliverable: Functional adapter layer routing to D3D

- [ ] **Add Backend Selection Mechanism**
  - Implement backend factory or registry in adapter layer
  - Add console variable or config option for backend selection
  - Default to D3D9 (current behavior)
  - Deliverable: Backend selection logic with default D3D9

- [ ] **Update Build System**
  - Modify CMakeLists to include new interface and adapter files
  - Ensure D3D9 backend is still linked by default
  - Touch points: `./CMakeLists.txt`, runtime CMake files
  - Deliverable: Updated build configuration

**Phase 1 Milestone**: Backend-neutral interface + adapter (M2)

---

## Phase 2 — Modularize D3D9 Backend

**Goal**: Wrap D3D9 implementation behind the new interface with no behavior change.

### Tasks

- [ ] **Create D3D9 Backend Implementation**
  - Create `./engine/runtime/render_a/src/sys/d3d/d3d9_backend.h` and `.cpp`
  - Implement backend interface for all required methods
  - Wrap `CD3D_Device` calls behind interface
  - Ensure zero behavior change (pass-through wrappers initially)
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_device.h`, `d3d_init.cpp`
  - Deliverable: D3D9 backend module implementing new interface

- [ ] **Isolate D3D Render World**
  - Wrap `CD3D_RenderWorld` behind backend-neutral interface methods
  - Create world rendering abstraction in backend interface
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_renderworld.h`, `d3d_renderworld.cpp`
  - Deliverable: D3D world rendering behind interface

- [ ] **Isolate D3D Render Style Pipeline**
  - Wrap `CD3DRenderStyle` and render style operations behind interface
  - Introduce D3D-only extension points for D3DX hooks
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_renderstyle.h`, `d3d_renderstyle.cpp`
  - Deliverable: D3D render styles behind interface

- [ ] **Isolate Texture and Resource Management**
  - Wrap D3D texture creation, update, and management behind interface
  - Ensure `ILTTexInterface` routes through backend-neutral implementation
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_texture.h`, `d3d_texture.cpp`
  - Deliverable: D3D texture management behind interface

- [ ] **Register D3D9 Backend**
  - Register D3D9 backend implementation in factory/registry
  - Ensure default selection works
  - Deliverable: D3D9 backend auto-registered

- [ ] **Validate D3D9 Pass-Through**
  - Run existing D3D9 test scenarios
  - Verify identical behavior to baseline
  - Check all touch points: init, rendering, resource management, teardown
  - Deliverable: Regression test confirming identical D3D9 behavior

**Phase 2 Milestone**: D3D9 backend modularized (M3)

---

## Phase 3 — Shared Render Data Layer

**Goal**: Create backend-neutral types for core render data.

### Tasks

- [ ] **Define Backend-Neutral Type System**
  - Create `./engine/runtime/shared/src/sys/win/render_types.h`
  - Define enums for: pixel formats, texture types, primitive topologies, render target formats
  - Create structs for: render target description, texture description, render pass ops
  - Deliverable: Backend-neutral type definitions

- [ ] **Create Format Mapping Tables**
  - Create mapping from backend-neutral formats to D3D9 formats
  - Extend mapping tables for future backends (Vulkan, Metal, DX12)
  - Touch points: All format conversion points in D3D backend
  - Deliverable: Format mapping tables per backend

- [ ] **Abstract Render Target Types**
  - Define backend-neutral render target creation parameters
  - Map D3D render target creation to neutral types
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/rendertarget.h`, `rendertarget.cpp`
  - Deliverable: Neutral render target abstraction

- [ ] **Abstract Texture Creation/Update**
  - Define neutral texture upload and update interfaces
  - Map D3D texture operations to neutral types
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_texture.cpp`
  - Deliverable: Neutral texture operations

- [ ] **Route ILT Interfaces Through Neutral Layer**
  - Update `ILTDrawPrim` implementations to use backend-neutral types
  - Update `ILTTexInterface` to use backend-neutral types
  - Update `ILTRenderStyles` to use backend-neutral render pass ops
  - Touch points: `./engine/sdk/inc/iltrendermgr.h`, `iltdrawprim.h`, `ilttexinterface.h`, `iltrenderstyles.h`
  - Deliverable: All SDK interfaces using neutral types

- [ ] **Create Render Pass Abstraction**
  - Define `RenderPassOp` structure (clear, load, store operations)
  - Map D3D render states to neutral render pass ops
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_renderstyle.cpp`
  - Deliverable: Render pass abstraction with D3D mapping

**Phase 3 Milestone**: Shared render data layer (M4)

---

## Phase 4 — New Backend Bring-Up

**Goal**: Minimal functional backends wired through `RenderStruct`.

### Tasks

- [x] **Choose First Backend Target**
  - Decision: Vulkan as first bring-up
  - Rationale: Vulkan provides explicit control, cross-platform support, and good tooling
  - Deliverable: Backend selection document (Vulkan selected)

- [ ] **Create Backend Skeleton**
  - Create `./engine/runtime/render_a/src/sys/vulkan/` directory
  - Implement minimal Vulkan backend interface stubs
  - Add build configuration for Vulkan backend (Vulkan SDK, dxc)
  - Deliverable: Skeleton Vulkan backend building and linking

- [ ] **Implement Device Initialization**
  - Implement backend device creation and validation
  - Handle swapchain creation and surface setup
  - Implement `SwapBuffers` semantics for present
  - Touch points: Backend init files, adapter layer
  - Deliverable: Backend can create device and swapchain

- [ ] **Implement Render Target Creation**
  - Create neutral render target → backend render target mapping
  - Implement texture creation aligned with `ILTRenderMgr` expectations
  - Touch points: Backend texture management files
  - Deliverable: Basic render targets working

- [ ] **Implement Basic ILTDrawPrim Path**
  - Implement minimal state setup (viewport, scissor, blend modes)
  - Implement triangle list draw calls
  - Map vertex buffers to backend format
  - Touch points: Backend draw implementation
  - Deliverable: Basic geometry rendering

- [ ] **Implement Texture Upload**
  - Implement texture data upload from neutral types
  - Handle format conversion as needed
  - Touch points: Backend texture implementation
  - Deliverable: Textures upload and render correctly

- [ ] **Setup HLSL → SPIR-V Shader Pipeline (via dxc)**
  - Integrate DirectXShaderCompiler (dxc) into build system
  - Create HLSL shader source directory for Vulkan backend
  - Implement build-time shader compilation: HLSL → SPIR-V using dxc
  - Create simple vertex/pixel shaders for minimal scene test
  - Handle descriptor set layouts and push constants mapping
  - Touch points: CMakeLists.txt, Vulkan shader compiler script, shader sources
  - Deliverable: Working HLSL → SPIR-V compilation pipeline with test shaders

- [ ] **Create Minimal Test Scene**
  - Build test that renders basic colored triangle using Vulkan shaders
  - Verify swapchain present works
  - Compare to D3D9 baseline snapshot
  - Deliverable: Test scene and validation results

**Phase 4 Milestone**: Minimal Vulkan backend bring-up (M5)

---

## Phase 5 — World Rendering + Render Style Parity

**Goal**: Backend-neutral render world and render style pipeline parity.

### Tasks

- [ ] **Abstract World Rendering Interface**
  - Define backend-neutral world renderer interface
  - Extract common world rendering operations from D3D implementation
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_renderworld.h`, `d3d_renderworld.cpp`
  - Deliverable: World rendering abstraction

- [ ] **Implement World Rendering in New Backend**
  - Port world rendering logic to new backend
  - Handle visibility culling, model rendering, world brushes
  - Touch points: Backend world rendering files
  - Deliverable: World rendering in new backend

- [ ] **Abstract Render Style Pipeline**
  - Define backend-neutral render style operations
  - Extract common render style state management
  - Map `RenderPassOp` to backend render pass equivalents
  - Touch points: `./engine/runtime/render_a/src/sys/d3d/d3d_renderstyle.h`, `d3d_renderstyle.cpp`
  - Deliverable: Render style abstraction

- [ ] **Implement Render Styles in New Backend**
  - Port render style pipeline to new backend
  - Handle lighting, shading, material effects
  - Map D3D render style options to backend equivalents
  - Touch points: Backend render style implementation
  - Deliverable: Render styles working in new backend

- [ ] **Handle D3DX Hooks and Extensions**
  - Keep D3DX-specific functionality in D3D backend extensions
  - Document which render styles require D3D-specific behavior
  - Touch points: D3D backend render style extensions
  - Deliverable: D3DX extensions documented and isolated

- [ ] **Validate World + Style Parity**
  - Run complex world rendering tests
  - Compare render output to D3D9 baseline
  - Verify render styles produce matching results
  - Deliverable: Parity validation report

**Phase 5 Milestone**: Render world + render style parity (M6)

---

## Phase 6 — Validation & Performance Hardening

**Goal**: Regression suite and performance baseline vs D3D9.

### Tasks

- [ ] **Create Snapshot Regression Suite**
  - Implement automated screenshot capture using `MakeScreenShot`
  - Create test scenes covering: basic geometry, complex world, render styles, textures
  - Compare new backend snapshots to D3D9 baseline
  - Touch points: `./engine/runtime/kernel/src/sys/win/render.cpp`
  - Deliverable: Automated snapshot test suite

- [ ] **Build Test Matrix**
  - Create tests for: render targets, texture upload, draw prims, render styles
  - Test corner cases: multiple render targets, texture formats, blend modes
  - Deliverable: Comprehensive test matrix document

- [ ] **Performance Baseline**
  - Profile D3D9 backend for GPU timing where available
  - Profile new backend for same scenarios
  - Identify performance regressions
  - Deliverable: Performance baseline report

- [ ] **Memory Usage Analysis**
  - Track memory allocation patterns in D3D9 backend
  - Compare to new backend memory usage
  - Identify and fix memory regressions
  - Deliverable: Memory usage report

- [ ] **Stress Testing**
  - Run extended duration tests for stability
  - Test resource allocation/deallocation cycles
  - Test rapid backend switching if supported
  - Deliverable: Stability test results

- [ ] **Create Validation Checklist**
  - Document all validation criteria from plan
  - Mark each criterion as pass/fail
  - Create remediation plan for any failures
  - Deliverable: Validation checklist

- [ ] **Final Documentation**
  - Update `RENDER_BACKEND_PLAN.md` with implementation notes
  - Document backend selection and configuration
  - Document known limitations and future work
  - Deliverable: Final documentation

**Phase 6 Milestone**: Validation and performance baselines (M7)

---

## Open Questions (Resolve Before Phase 4)

- [x] Determine preferred backend order → **Vulkan first, then Metal, then DX12**
- [x] Decide first bring-up target → **Minimal scene**
- [x] Evaluate if multi-backend runtime switching is needed → **Build-time selection only**
- [x] Determine shader translation strategy → **HLSL → SPIR-V via dxc (DirectXShaderCompiler)**

---

## Risk Tracking

### Identified Risks
1. **RenderStruct D3D Coupling**: Tight coupling to D3D device handle
   - *Mitigation*: Keep `GetD3DDevice` for legacy, add neutral accessor

2. **Render Style D3D Options**: D3DX-specific render style options
   - *Mitigation*: Keep D3D options as backend extensions, document exclusivity

3. **Render World Tight Coupling**: `CD3D_RenderWorld` deeply integrated
   - *Mitigation*: Introduce interface wrapper, keep D3D as reference implementation

4. **ILT* Interface Expectations**: SDK interfaces may assume D3D behavior
   - *Mitigation*: Preserve signatures, route to backend-neutral implementations, add compatibility layer if needed

---

## Shader Translation Strategy

### Approach: HLSL → SPIR-V via DirectXShaderCompiler (dxc)

**Decision**: Use DirectXShaderCompiler (dxc) to compile HLSL shaders directly to SPIR-V for Vulkan backend.

### Rationale

1. **Direct HLSL support**: LithTech shaders are in HLSL source format
2. **Mature tooling**: dxc has robust HLSL → SPIR-V conversion with good HLSL feature coverage
3. **Build-time only**: Shader compilation happens at build time, avoiding runtime complexity
4. **Debuggable**: Source-level debugging and error messages maintained
5. **Active development**: Microsoft actively maintains dxc with Vulkan support

### Implementation Plan

#### Phase 4 (Minimal Scene)
- Integrate dxc into build system via CMake
- Create simple HLSL vertex/pixel shaders for triangle test
- Build-time compilation: `dxc -spirv -T vs_6_0 -E main -Fo output.spv input.hlsl`
- Load compiled SPIR-V at runtime via Vulkan pipeline

#### Phase 5 (World + Render Styles)
- Expand shader library for full world rendering
- Handle D3DX-specific shader intrinsics via dxc compatibility
- Create descriptor set layout mapping for shader resources
- Implement push constants for uniform data

### Technical Details

**dxc Integration Requirements**:
- Vulkan SDK (validation layers, loader)
- DirectXShaderCompiler (dxc executable/libraries)
- CMake custom command for shader compilation
- Shader source directory: `./engine/runtime/render_a/src/sys/vulkan/shaders/`

**HLSL Compatibility**:
- Target profile: `vs_6_0` / `ps_6_0` (SPIR-V generation)
- Supported features: HLSL 2021 with SPIR-V extensions
- Resource binding: Use `register()` declarations mapped to descriptor sets
- Semantics: `SV_Position`, `SV_Target`, etc. for Vulkan compatibility

**Limitations**:
- Some D3D9-specific intrinsics may need manual replacement
- Fixed-function pipeline must be emulated via shaders
- Render state blocks require manual shader constant management

---

## Progress Tracking

- [ ] Phase 0: Architecture Inventory (M1)
- [ ] Phase 1: Backend-Neutral Renderer Contract (M2)
- [ ] Phase 2: Modularize D3D9 Backend (M3)
- [ ] Phase 3: Shared Render Data Layer (M4)
- [ ] Phase 4: New Backend Bring-Up (M5)
- [ ] Phase 5: World Rendering + Render Style Parity (M6)
- [ ] Phase 6: Validation & Performance Hardening (M7)

---

## Reference Documentation

- **Architecture Inventory**: `Docs/./RENDER_ARCHITECTURE_INVENTORY.md` (to be created)
- **API Reference**: `Docs/./LITHTECH_API_REFERENCE.md`
- **Comprehensive Reference**: `Docs/./LITHTECH_COMPREHENSIVE_REFERENCE.md`
- **Main Plan**: `/RENDER_BACKEND_PLAN.md`
- **External Reference Patterns**: Godot RenderingDeviceDriver, Diligent Engine, O3DE RHI, bgfx/wgpu

---

## Notes

- This TODO should be updated incrementally as tasks are completed
- Track blockers and dependencies in task descriptions
- Use git commits with descriptive messages for each milestone
- Update `.gitignore` to exclude build artifacts and logs
- Consider adding CI pipeline for regression testing once Phase 6 begins
