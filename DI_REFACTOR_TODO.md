# LithTech Engine: Dependency Injection Refactor

## Overview

This document outlines a comprehensive refactor to introduce dependency injection (DI) throughout the LithTech engine. The goal is to decouple tightly-coupled components, eliminate global state where practical, and enable tools like DEdit2 to share code paths with the runtime engine.

### Current Problems

1. **Global state everywhere** - `g_pClientMgr`, `g_pServerMgr`, `g_ModelMgr` are accessed directly throughout the codebase
2. **Monolithic managers** - `CClientMgr` is a god-object with 50+ responsibilities
3. **Untestable code** - Can't unit test components in isolation
4. **Tool limitations** - DEdit2 can't use engine object creation because it requires full runtime
5. **Hidden dependencies** - Functions reach into globals making dependency chains invisible

### Target Architecture

```
Before:  Function() → g_pClientMgr->DoThing()     [hidden global dependency]
After:   Function(IContext* ctx) → ctx->GetService()->DoThing()  [explicit dependency]
```

---

## Phase 1: Interface Definitions

### 1.1 Core Service Interfaces

Create focused interfaces for each major service category.

**Files to create in `engine/sdk/inc/`:**

| File | Interface | Wraps |
|------|-----------|-------|
| `imodelloader.h` | `IModelLoader` | Model loading, caching |
| `itextureloader.h` | `ITextureLoader` | Texture loading, SharedTexture management |
| `ispriteloader.h` | `ISpriteLoader` | Sprite loading |
| `isoundloader.h` | `ISoundLoader` | Sound/music loading |
| `ifilemanager.h` | `IFileManager` | File resolution, streaming |
| `iobjectmanager.h` | `IObjectManager` | Object allocation, lifecycle |
| `iworldmanager.h` | `IWorldManager` | World/BSP operations |
| `irenderer.h` | `IRenderer` | Render operations |
| `iinputmanager.h` | `IInputManager` | Input handling |
| `inetworkmanager.h` | `INetworkManager` | Network operations |
| `iconsole.h` | `IConsole` | Console commands, cvars |
| `itimer.h` | `ITimer` | Time/frame management |

### 1.2 Context Bundles

Create context interfaces that bundle related services.

**Files to create:**

| File | Interface | Contains |
|------|-----------|----------|
| `iobjectcontext.h` | `IObjectContext` | IModelLoader, ITextureLoader, ISpriteLoader, IObjectManager |
| `irendercontext.h` | `IRenderContext` | IRenderer, ITextureLoader, IWorldManager |
| `igamecontext.h` | `IGameContext` | Full bundle for game code |
| `ieditorcontext.h` | `IEditorContext` | Subset for editor tools |

### 1.3 Interface Template

```cpp
// engine/sdk/inc/imodelloader.h
#ifndef __IMODELLOADER_H__
#define __IMODELLOADER_H__

#include "ltmodule.h"
#include "ltbasedefs.h"

class Model;
struct FileRef;

class IModelLoader : public IBase {
public:
    interface_version(IModelLoader, 0);

    // Core operations
    virtual LTRESULT LoadModel(const char* filename, Model*& pModel) = 0;
    virtual LTRESULT LoadModel(const FileRef& fileRef, Model*& pModel) = 0;
    virtual void UnloadModel(Model* pModel) = 0;

    // Cache management
    virtual bool IsModelCached(const char* filename) const = 0;
    virtual void FlushModelCache() = 0;
    virtual size_t GetCacheSize() const = 0;

    // Fallback
    virtual LTRESULT LoadDefaultModel(Model*& pModel) = 0;
};

#endif // __IMODELLOADER_H__
```

---

## Phase 2: Adapter Layer

### 2.1 CClientMgr Adapters

Create adapter classes that implement new interfaces by delegating to existing `g_pClientMgr` methods.

**Files to create in `engine/runtime/client/src/`:**

| File | Classes |
|------|---------|
| `clientmgr_adapters.h` | Adapter declarations |
| `clientmgr_adapters.cpp` | Adapter implementations |

**Adapters to implement:**

```cpp
// Wraps g_pClientMgr model loading
class CClientMgrModelLoader : public IModelLoader {
public:
    LTRESULT LoadModel(const char* filename, Model*& pModel) override {
        if (!g_pClientMgr) return LT_NOTINITIALIZED;
        return g_pClientMgr->LoadModel(filename, pModel);
    }
    // ... other methods
};

// Wraps g_pClientMgr texture loading
class CClientMgrTextureLoader : public ITextureLoader { ... };

// Wraps g_pClientMgr sprite loading
class CClientMgrSpriteLoader : public ISpriteLoader { ... };

// Wraps ObjectMgr operations
class CClientMgrObjectManager : public IObjectManager { ... };

// Bundle context
class CClientMgrObjectContext : public IObjectContext {
    CClientMgrModelLoader m_modelLoader;
    CClientMgrTextureLoader m_textureLoader;
    CClientMgrSpriteLoader m_spriteLoader;
    CClientMgrObjectManager m_objectManager;
public:
    IModelLoader* GetModelLoader() override { return &m_modelLoader; }
    ITextureLoader* GetTextureLoader() override { return &m_textureLoader; }
    // ...
};
```

### 2.2 CServerMgr Adapters

Similar adapters for server-side operations.

**Files to create in `engine/runtime/server/src/`:**

| File | Classes |
|------|---------|
| `servermgr_adapters.h` | Server adapter declarations |
| `servermgr_adapters.cpp` | Server adapter implementations |

### 2.3 Registration with Holder System

Use existing `ltmodule.h` holder system for interface registration:

```cpp
// In clientmgr_adapters.cpp
static CClientMgrObjectContext s_clientObjectContext;
define_holder(IObjectContext, s_pObjectContext);

void InitClientAdapters() {
    s_pObjectContext = &s_clientObjectContext;
}
```

---

## Phase 3: Core System Refactoring

### 3.1 Object Creation System

**Target:** `engine/runtime/client/src/setupobject.cpp`

**Current state:**
- `so_ExtraInit()` dispatches to type-specific init functions
- `ModelExtraInit()` calls `g_pClientMgr->LoadModel()` directly
- `SpriteExtraInit()` calls `g_pClientMgr` methods directly

**Refactored state:**
```cpp
// Add context parameter
LTRESULT so_ExtraInit(LTObject* pObject, InternalObjectSetup* pSetup,
                      bool bLocalFromServer, IObjectContext* pContext);

// Legacy overload for backward compatibility
LTRESULT so_ExtraInit(LTObject* pObject, InternalObjectSetup* pSetup,
                      bool bLocalFromServer) {
    return so_ExtraInit(pObject, pSetup, bLocalFromServer, GetDefaultObjectContext());
}

// Refactored ModelExtraInit
static LTRESULT ModelExtraInit(LTObject* pObject, InternalObjectSetup* pSetup,
                               bool bLocalFromServer, IObjectContext* pContext) {
    IModelLoader* loader = pContext->GetModelLoader();
    LTRESULT result = loader->LoadModel(pSetup->m_Filename[0], pModel);
    // ...
}
```

**Files to modify:**
- `engine/runtime/client/src/setupobject.h`
- `engine/runtime/client/src/setupobject.cpp`

### 3.2 Object Manager

**Target:** `engine/runtime/shared/src/objectmgr.cpp`

**Current state:**
- `om_CreateObject()` allocates from object banks
- Used by both client and server
- No external dependencies (already clean)

**Refactored state:**
- Wrap in `IObjectManager` interface
- Allow injection of custom allocators

**Files to modify:**
- `engine/runtime/shared/src/objectmgr.h`
- `engine/runtime/shared/src/objectmgr.cpp`

### 3.3 Model Manager

**Target:** `engine/runtime/model/src/`

**Current state:**
- `g_ModelMgr` global for model caching
- Direct file access

**Refactored state:**
- Implement `IModelLoader` interface
- Accept `IFileManager` for file operations

**Files to modify:**
- `engine/runtime/model/src/modelmgr.h`
- `engine/runtime/model/src/modelmgr.cpp`

### 3.4 Texture System

**Target:** `engine/runtime/client/src/` texture handling

**Current state:**
- `g_pClientMgr->AddSharedTexture2()` for loading
- `SharedTextureBank` for allocation
- Tightly coupled to CClientMgr

**Refactored state:**
- `ITextureLoader` interface
- Separate `TextureCache` class
- Can be used by editor without CClientMgr

**Files to modify:**
- `engine/runtime/client/src/cutil.cpp`
- `engine/runtime/shared/src/texture.cpp`

---

## Phase 4: Client Manager Decomposition

### 4.1 Extract Services from CClientMgr

`CClientMgr` currently handles too many responsibilities. Extract into focused classes:

| Responsibility | New Class | Interface |
|----------------|-----------|-----------|
| Model loading/caching | `ModelService` | `IModelLoader` |
| Texture loading/caching | `TextureService` | `ITextureLoader` |
| Sprite management | `SpriteService` | `ISpriteLoader` |
| Object lifecycle | `ObjectService` | `IObjectManager` |
| World management | `WorldService` | `IWorldManager` |
| Sound/music | `AudioService` | `ISoundLoader` |
| Input handling | `InputService` | `IInputManager` |
| Network | `NetworkService` | `INetworkManager` |

### 4.2 CClientMgr as Facade

After extraction, `CClientMgr` becomes a facade:

```cpp
class CClientMgr {
private:
    ModelService m_modelService;
    TextureService m_textureService;
    ObjectService m_objectService;
    // ...

public:
    // Legacy methods delegate to services
    LTRESULT LoadModel(const char* filename, Model*& pModel) {
        return m_modelService.LoadModel(filename, pModel);
    }

    // Expose services for DI
    IModelLoader* GetModelLoader() { return &m_modelService; }
    ITextureLoader* GetTextureLoader() { return &m_textureService; }
};
```

### 4.3 Files to Create/Modify

**New files:**
- `engine/runtime/client/src/model_service.h/.cpp`
- `engine/runtime/client/src/texture_service.h/.cpp`
- `engine/runtime/client/src/sprite_service.h/.cpp`
- `engine/runtime/client/src/object_service.h/.cpp`
- `engine/runtime/client/src/world_service.h/.cpp`
- `engine/runtime/client/src/audio_service.h/.cpp`

**Modified files:**
- `engine/runtime/client/src/clientmgr.h`
- `engine/runtime/client/src/clientmgr.cpp`

---

## Phase 5: Server Manager Decomposition

Similar decomposition for `CServerMgr`:

| Responsibility | New Class | Interface |
|----------------|-----------|-----------|
| Object management | `ServerObjectService` | `IObjectManager` |
| World management | `ServerWorldService` | `IWorldManager` |
| Network handling | `ServerNetworkService` | `INetworkManager` |
| Game logic dispatch | `GameLogicService` | `IGameLogic` |

---

## Phase 6: Tool Integration

### 6.1 DEdit2 Context

Create editor-specific implementations of interfaces.

**Files to create in `tools/DEdit2/`:**

| File | Purpose |
|------|---------|
| `editor_model_service.h/.cpp` | IModelLoader for editor |
| `editor_texture_service.h/.cpp` | ITextureLoader for editor |
| `editor_object_context.h/.cpp` | IObjectContext bundle |

**Implementation:**
```cpp
class EditorModelService : public IModelLoader {
    std::unordered_map<std::string, Model*> m_cache;

public:
    LTRESULT LoadModel(const char* filename, Model*& pModel) override {
        // Use DEdit2's file manager
        // Cache loaded models
        // Return loaded model
    }
};

class EditorObjectContext : public IObjectContext {
    EditorModelService m_modelService;
    EditorTextureService m_textureService;
    // ...
public:
    EditorObjectContext(/* DEdit2 dependencies */);
    IModelLoader* GetModelLoader() override { return &m_modelService; }
};
```

### 6.2 Enable Shared Code Paths

With DI in place, DEdit2 can use `so_ExtraInit()`:

```cpp
// In DEdit2's model instance creation
ModelInstance* CreateModelInstance(const char* filename, ...) {
    // Use om_CreateObject (shared with runtime)
    ObjectCreateStruct createStruct = {...};
    LTObject* pObject;
    om_CreateObject(&m_objectMgr, &createStruct, &pObject);

    // Use so_ExtraInit with editor context (shared code path!)
    InternalObjectSetup setup = {...};
    so_ExtraInit(pObject, &setup, false, &m_editorContext);

    return ToModel(pObject);
}
```

### 6.3 Future Tools

The DI architecture enables future tools:

- **Model Viewer** - Uses `IModelLoader`, `IRenderContext`
- **Texture Browser** - Uses `ITextureLoader`, `IFileManager`
- **Animation Editor** - Uses `IModelLoader`, animation interfaces
- **Test Harness** - Mock implementations for unit testing

---

## Phase 7: Testing Infrastructure

### 7.1 Mock Implementations

Create mock implementations for testing:

**Files to create in `engine/test/mocks/`:**

| File | Purpose |
|------|---------|
| `mock_model_loader.h` | Returns pre-defined models |
| `mock_texture_loader.h` | Returns placeholder textures |
| `mock_file_manager.h` | In-memory file system |
| `mock_object_context.h` | Bundle of mocks |

### 7.2 Example Test

```cpp
TEST(ModelExtraInit, LoadsModelFromContext) {
    MockModelLoader mockLoader;
    Model* expectedModel = CreateTestModel();
    mockLoader.SetNextModel(expectedModel);

    MockObjectContext context;
    context.SetModelLoader(&mockLoader);

    ModelInstance instance;
    InternalObjectSetup setup = {...};

    LTRESULT result = ModelExtraInit(&instance, &setup, false, &context);

    EXPECT_EQ(LT_OK, result);
    EXPECT_EQ(expectedModel, instance.GetModelDB());
}
```

---

## Migration Strategy

### Backward Compatibility Rules

1. **Never break existing code** - Legacy functions remain, delegate to new ones
2. **Incremental rollout** - One system at a time
3. **Feature flags** - `#ifdef USE_DI_INTERFACES` for gradual adoption
4. **Regression testing** - Full game must work after each phase

### Migration Order

```
Phase 1: Interfaces          [Non-breaking - just adding headers]
    ↓
Phase 2: Adapters           [Non-breaking - wraps existing globals]
    ↓
Phase 3.1: Object Creation  [Backward compatible - legacy overloads]
    ↓
Phase 3.2-3.4: Other core   [Backward compatible]
    ↓
Phase 4: Client decomp      [Internal refactor, API unchanged]
    ↓
Phase 5: Server decomp      [Internal refactor, API unchanged]
    ↓
Phase 6: Tool integration   [New capability]
    ↓
Phase 7: Testing            [New capability]
```

### Deprecation Path

After full rollout:
1. Mark legacy functions as `[[deprecated]]`
2. Update all call sites over time
3. Eventually remove legacy functions (major version bump)

---

## File Summary

### New Files to Create

**Interfaces (engine/sdk/inc/):**
- `imodelloader.h`
- `itextureloader.h`
- `ispriteloader.h`
- `isoundloader.h`
- `ifilemanager.h`
- `iobjectmanager.h`
- `iworldmanager.h`
- `irenderer.h`
- `iinputmanager.h`
- `inetworkmanager.h`
- `iconsole.h`
- `itimer.h`
- `iobjectcontext.h`
- `irendercontext.h`
- `igamecontext.h`

**Client Adapters (engine/runtime/client/src/):**
- `clientmgr_adapters.h`
- `clientmgr_adapters.cpp`
- `model_service.h/.cpp`
- `texture_service.h/.cpp`
- `sprite_service.h/.cpp`
- `object_service.h/.cpp`
- `world_service.h/.cpp`
- `audio_service.h/.cpp`

**Server Adapters (engine/runtime/server/src/):**
- `servermgr_adapters.h`
- `servermgr_adapters.cpp`

**Editor (tools/DEdit2/):**
- `editor_model_service.h/.cpp`
- `editor_texture_service.h/.cpp`
- `editor_object_context.h/.cpp`

**Testing (engine/test/mocks/):**
- `mock_model_loader.h`
- `mock_texture_loader.h`
- `mock_file_manager.h`
- `mock_object_context.h`

### Existing Files to Modify

**Core:**
- `engine/runtime/client/src/setupobject.h/.cpp`
- `engine/runtime/client/src/clientmgr.h/.cpp`
- `engine/runtime/server/src/servermgr.h/.cpp`
- `engine/runtime/shared/src/objectmgr.h/.cpp`
- `engine/runtime/model/src/modelmgr.h/.cpp`

**Build:**
- `engine/runtime/client/src/CMakeLists.txt`
- `engine/runtime/server/src/CMakeLists.txt`
- `tools/DEdit2/CMakeLists.txt`

---

## Success Criteria

1. **Game works unchanged** - Full backward compatibility
2. **DEdit2 renders models** - Using shared `so_ExtraInit()` path
3. **Unit tests pass** - With mock implementations
4. **No global access in new code** - All dependencies injected
5. **Clear dependency graph** - Can trace what each function needs

---

## Open Questions

- [ ] Should we use smart pointers for service lifetime management?
- [ ] How to handle circular dependencies between services?
- [ ] Should contexts be thread-safe for future multi-threading?
- [ ] How to version interfaces for future expansion?
- [ ] Should we adopt a DI container library or keep it manual?

---

## References

- `engine/sdk/inc/ltmodule.h` - Existing interface pattern
- `engine/runtime/client/src/clientmgr.h` - Current CClientMgr structure
- `engine/runtime/client/src/setupobject.cpp` - Object creation to refactor
- Modern engine examples: Unreal's subsystems, Unity's dependency injection
