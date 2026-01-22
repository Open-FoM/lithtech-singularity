## Diligent Renderer Refactor Tracking

### Goals
- Split `diligent_render.cpp` into focused units by concern.
- Keep public API (`diligent_render.h`) stable.
- Maintain buildability at each phase.

### Legend
- [ ] pending
- [~] in progress
- [x] done

### Phases
- [x] Phase 0: Scaffolding (tracking file, internal headers, render state stub)
- [x] Phase 1: Core utils + device/swapchain/buffers
- [x] Phase 2: Pipeline/cache + texture cache/conversion
- [x] Phase 3: World data/load + world draw paths
- [x] Phase 4: Models + object renderers + shadows/glow
- [x] Phase 5: API entry points + wiring cleanup

### Move Ledger
| Symbol / Area | From | To | Status |
| --- | --- | --- | --- |
| `diligent_get_active_render_target` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| `diligent_get_active_depth_target` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| `diligent_UpdateWindowHandles` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| `diligent_QueryWindowSize` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| `diligent_UpdateScreenSize` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| `diligent_EnsureDevice` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| `diligent_CreateSwapChain` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| `diligent_EnsureSwapChain` | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| Mesh layout structs + builders | `diligent_render.cpp` | `diligent_mesh_layout.h/.cpp` | [x] |
| Core utils (hash/float/clip/matrix/viewport) | `diligent_render.cpp` | `diligent_utils.h/.cpp` | [x] |
| Buffer helpers (`diligent_create_buffer`, `diligent_create_dynamic_vertex_buffer`) | `diligent_render.cpp` | `diligent_buffers.h/.cpp` | [x] |
| Pipeline cache structs + hashing + PSO/SRB caches | `diligent_render.cpp` | `diligent_pipeline_cache.h/.cpp` | [x] |
| Texture cache + conversion + FormatMgr | `diligent_render.cpp` | `diligent_texture_cache.h/.cpp` | [x] |
| World data structs + load/lightmap helpers | `diligent_render.cpp` | `diligent_world_data.h/.cpp` | [x] |
| World pipeline + draw helpers | `diligent_render.cpp` | `diligent_world_draw.h/.cpp` | [x] |
| Model pipeline + draw + mesh implementations | `diligent_render.cpp` | `diligent_model_draw.h/.cpp` | [x] |
| Optimized 2D/surfaces + blit path | `diligent_render.cpp` | `diligent_2d_draw.h/.cpp` | [x] |
| Object renderers (sprites/particles/lines/volumes/polygrids/canvases/sky) | `diligent_render.cpp` | `diligent_object_draw.h/.cpp` | [x] |
| Debug line pipeline + debug draw helpers | `diligent_render.cpp` | `diligent_debug_draw.h/.cpp` | [x] |
| Scene/world collection + model traversal | `diligent_render.cpp` | `diligent_scene_collect.h/.cpp` | [x] |
| World API (load/light groups/texture effect vars) | `diligent_render.cpp` | `diligent_world_api.cpp` | [x] |
| Renderer API entry points (Clear/Start3D/SwapBuffers/etc.) | `diligent_render.cpp` | `diligent_render_api.h/.cpp` | [x] |
| Output target overrides (SetOutputTargets) | `diligent_render.cpp` | `diligent_output_targets.cpp` | [x] |
| Scene render orchestration (RenderScene) | `diligent_render.cpp` | `diligent_scene_render.h/.cpp` | [x] |
| Renderer lifecycle (Init/Term) | `diligent_render.cpp` | `diligent_renderer_lifecycle.h/.cpp` | [x] |
| Fog/shader controls | `diligent_render.cpp` | `rendererconsolevars.h` (direct convars) | [x] |
| Swapchain sRGB helper | `diligent_render.cpp` | `diligent_device.cpp` | [x] |
| Texture debug/test wrappers | `diligent_render.cpp` | `diligent_texture_cache.cpp` | [x] |
| Render DLL setup wiring | `diligent_render.cpp` | `diligent_render_entry.cpp` | [x] |

### Notes / Checkpoints
- 2026-01-21: Initialized tracking file + added `diligent_state.h/.cpp` and `diligent_internal.h`.
- 2026-01-21: Extracted device/window/swapchain helpers into `diligent_device.cpp`.
- 2026-01-21: Extracted mesh layout + model vertex shader builder into `diligent_mesh_layout.cpp`.
- 2026-01-21: Extracted core utils (hash/float/clip/matrix/viewport) into `diligent_utils.cpp`.
- 2026-01-21: Extracted buffer helpers into `diligent_buffers.cpp`.
- 2026-01-21: Extracted pipeline cache (PSO/SRB keying + caches) into `diligent_pipeline_cache.cpp`.
- 2026-01-21: Extracted texture cache/conversion + `FormatMgr` into `diligent_texture_cache.cpp`.
- 2026-01-21: Extracted world data/load + lightmap helpers into `diligent_world_data.cpp`.
- 2026-01-21: Extracted world pipeline/draw logic into `diligent_world_draw.cpp`.
- 2026-01-22: Repaired world draw extraction (sorting helper, glow draw move) and fixed sky extents signature.
- 2026-01-22: Extracted model pipeline/draw + mesh implementations into `diligent_model_draw.cpp`.
- 2026-01-22: Extracted optimized 2D/surface/blit helpers into `diligent_2d_draw.cpp`.
- 2026-01-22: Extracted object draw + collection (sprites/particles/lines/volumes/polygrids/canvases/sky) into `diligent_object_draw.cpp`.
- 2026-01-22: Moved `diligent_build_transform` into `diligent_utils.cpp`, added shared BSP accessor, and localized `ScreenGlow_DrawCanvases` + line color packing for object draw.
- 2026-01-22: Extracted model shadow rendering/queueing/projection into `diligent_shadow_draw.cpp` and shared glow blur constants via `diligent_glow_blur.h`.
- 2026-01-22: Extracted screen glow/tonemap/postfx into `diligent_postfx.cpp` with public helpers in `diligent_postfx.h`.
- 2026-01-22: Extracted debug line pipeline + world/model debug drawing into `diligent_debug_draw.cpp`.
- 2026-01-22: Extracted scene collection (visible blocks, dynamic lights, model/world traversal) into `diligent_scene_collect.cpp`.
- 2026-01-22: Extracted world load/light-group/texture-effect APIs into `diligent_world_api.cpp`.
- 2026-01-22: Extracted renderer API entry points into `diligent_render_api.cpp`.
- 2026-01-22: Extracted output target override state into `diligent_output_targets.cpp`.
- 2026-01-22: Extracted RenderScene orchestration into `diligent_scene_render.cpp`.
- 2026-01-22: Extracted renderer lifecycle (Init/Term) into `diligent_renderer_lifecycle.cpp`.
- 2026-01-22: Moved fog/shader controls to `diligent_render_controls.cpp`, swapchain sRGB helper to `diligent_device.cpp`, and texture debug wrappers to `diligent_texture_cache.cpp`.
- 2026-01-22: Removed fog/shader/world debug wrappers in favor of direct renderer convars; deleted `diligent_render_controls.cpp`.
- 2026-01-22: Moved core renderer globals into `DiligentRenderState` with reference aliases for existing call sites.
- 2026-01-22: Migrated call sites to `g_diligent_state.*` and removed legacy global aliases.
- 2026-01-22: Moved `rdll_RenderDLLSetup` wiring into `diligent_render_entry.cpp`.
- 2026-01-22: Split debug/stat APIs into `diligent_render_debug.h` and trimmed `diligent_render.h` to core entry points.
- 2026-01-22: Extracted drawprim texture view helper into `diligent_drawprim_api.h`.
