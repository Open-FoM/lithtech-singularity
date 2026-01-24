# Client-Like Model Setup in DEdit2 (Recommended Plan)

This document outlines the recommended plan to make DEdit2 create and initialize model objects using the same core engine code paths as the runtime client. The goal is to eliminate transform/offset drift and other mismatches caused by DEdit2’s prior “manual ModelInstance” construction.

## Why this

DEdit2 previously created `ModelInstance` objects directly, bypassing the client object pipeline. That skipped several important steps (object setup, extra init, skin/renderstyle setup, world offset handling, dims update, etc.). The result was visible divergence from in‑game placement and behavior (e.g., floating props, inconsistent offsets, wrong render state assumptions).

Using the client‑like path aligns editor behavior with the runtime’s object creation semantics.

## Target Engine Path (Runtime Analogy)

Runtime uses roughly:

- `om_CreateObject` (object manager creates `LTObject` from `ObjectCreateStruct`)
- `so_ExtraInit` (type‑specific extra init)
- `MoveObject` / `RotateObject` / `ScaleObject` as needed

Client entry point example:
`CClientMgr::AddObjectToClientWorld` (engine/runtime/client/src/clientmgr.cpp)

## Current State

- The manual `EditorModelManager` path has been removed.
- Model rendering is currently disabled in DEdit2 (no model objects are pushed to the renderer).
- The renderer-side transform fix remains in place.

## Recommended Plan (Option 1)

### 1) Introduce a DEdit2 “client‑like” setup helper

Create a new helper (module name suggestion: `editor_client_like_setup.{h,cpp}`) that:

- Builds an `ObjectCreateStruct` for each model node
- Wraps it in `InternalObjectSetup`
- Calls the engine’s object creation and init functions

Minimal flow:

```
ObjectCreateStruct ocs;
ocs.m_ObjectType = OT_MODEL;
ocs.m_Pos = <node pos, adjusted with world offset if compiled world>;
ocs.m_Rotation = <node rotation, converted to LTRotation>;
ocs.m_Scale = <node scale>;
ocs.m_Filename = <model path>;
ocs.m_SkinNames[] = <Skin / SkinN props>;
ocs.m_RenderStyleNames[] = <RenderStyle props if available>;

InternalObjectSetup setup;
setup.m_pSetup = &ocs;

LTObject* obj = nullptr;
om_CreateObject(&g_ObjectMgr, &ocs, &obj);
so_ExtraInit(obj, &setup, /*bLocalFromServer=*/false);

// optional: Move/Rotate/Scale if needed to mirror runtime sequencing
```

### 2) Ensure world offset handling matches client

When loading compiled worlds (`.dat`), positions from BSP object data should match runtime placement.

- Confirm whether `object.Pos` in BSP is source-world or local-world
- Use the same offset application as `world_shared_bsp` and client object setup

The goal is that `obj.position` in DEdit2 matches what runtime would use.

### 3) Retain renderer fixes

Even with client‑like setup, the renderer still needs correct model transform usage:

- Keep the Diligent fix that removes bind‑pose global transform for render‑space (skinned meshes)
- Use raw cached transforms for rigid meshes

## Implementation Notes

- The Object Manager and client helpers already exist; prefer calling those rather than duplicating logic.
- Use DEdit2’s existing file manager tree resolution for model/skin paths.
- For models that don’t specify skins, allow the default to stand (runtime will leave skin slots null).

## Rollout Strategy

1) Introduce new helper + integrate for model nodes only.
2) Validate placement against runtime (floor alignment and rotation).
3) Expand to additional object classes if needed.

## Validation Checklist

- Props sit on the floor correctly for known scenes
- Rotation matches runtime
- Skins/textures appear correctly
- No crashes when unloading/reloading scenes
- Performance acceptable for typical scene sizes

## References

- `engine/runtime/client/src/clientmgr.cpp` (AddObjectToClientWorld)
- `engine/runtime/client/src/setupobject.cpp` (model extra init/skins)
- `engine/runtime/shared/src/objectmgr.cpp` (object creation + transforms)
- `engine/runtime/render_a/src/sys/diligent/diligent_model_draw.cpp` (render transforms)
