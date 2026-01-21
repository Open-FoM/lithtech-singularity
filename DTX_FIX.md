# DTX_FIX.md - Essential Texture Fixes

## Overview
These are the REQUIRED fixes for texture rendering in DEdit2 using the Diligent graphics engine.

---

## 1. dtxmgr.cpp - Header Dimension Restoration

**File**: `engine/runtime/shared/src/dtxmgr.cpp`
**Location**: Line ~389 (non-D3D path)

**Problem**: After memcpy of file header, dimensions don't match allocated data.

**Fix**:
```cpp
pRet->m_pSharedTexture = LTNULL;
memcpy(&pRet->m_Header, &hdr, sizeof(DtxHeader));

// Restore dimensions to match allocated data (like D3D path)
pRet->m_Header.m_BaseWidth = static_cast<uint16>(nTexWidth);
pRet->m_Header.m_BaseHeight = static_cast<uint16>(nTexHeight);
pRet->m_Header.m_nMipmaps = static_cast<uint16>(nNumMips);
```

---

## 2. diligent_render.cpp - Stride=0 Fallback

**File**: `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp`
**Location**: Line ~11140 (texture upload)

**Problem**: Uncompressed textures with stride=0 upload incorrectly.

**Fix**:
```cpp
uint32 stride = static_cast<uint32>(mip.m_Pitch);
if (stride == 0)
{
    if (IsFormatCompressed(texture_data->m_Header.GetBPPIdent()))
    {
        stride = diligent_calc_compressed_stride(texture_data->m_Header.GetBPPIdent(), mip.m_Width);
    }
    else
    {
        // Uncompressed RGBA8/BGRA8: 4 bytes per pixel
        stride = mip.m_Width * 4;
    }
}
```

---

## 3. diligent_render.cpp - BC Texture Alpha Fix

**File**: `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp`
**Location**: Line ~11047 (BC decompression)

**Problem**: BC-compressed textures may have all-zero alpha after decompression (invisible).

**Fix**:
```cpp
// Check if decompressed texture has all-zero alpha (would be invisible)
const bool bc_all_alpha_zero = !diligent_texture_has_visible_alpha(converted.get());

// Force alpha opaque if:
// 1. No alpha wanted (!wants_alpha), OR
// 2. All alpha is zero (texture would be invisible regardless of wants_alpha)
if (!wants_alpha || bc_all_alpha_zero)
{
    diligent_force_texture_alpha_opaque(converted.get());
}
```

---

## 4. diligent_render.cpp - BPP_32 Alpha Fix

**File**: `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp`
**Location**: Line ~11077 (BPP_32 handling block)

**Problem**: Uncompressed BPP_32 textures with all-zero alpha are invisible.

**Fix**:
```cpp
if (is_bpp32 && !converted)
{
    // Check if all alpha is zero - texture would be invisible
    const bool all_alpha_zero = !diligent_texture_has_visible_alpha(texture_data);

    converted = diligent_clone_texture_data(texture_data, mip_count);
    if (!converted) { return nullptr; }

    // Force alpha opaque if all alpha is zero
    if (all_alpha_zero)
    {
        diligent_force_texture_alpha_opaque(converted.get());
    }
    diligent_swap_bgra_to_rgba(converted.get(), mip_count);
    texture_data = converted.get();
}
```

---

## 5. diligent_render.cpp - LightAnim Section Skip

**File**: `engine/runtime/render_a/src/sys/diligent/diligent_render.cpp`

**Problem**: LightAnim placeholder sections render AFTER textured sections and overwrite them with lightmap-only mode.

**Fix (4 parts)**:

### A. Add flag to struct (Line ~224)
```cpp
struct DiligentRBSection {
    // ... existing members ...
    bool is_lightanim = false;
};
```

### B. Initialize in constructor (Line ~1469)
```cpp
DiligentRBSection::DiligentRBSection()
    : // ... existing initializers ...
      is_lightanim(false),
```

### C. Detect LightAnim textures (Line ~1872)
```cpp
// Detect LightAnim placeholder textures
if (section.textures[0] && texture_names[0][0])
{
    if (strstr(texture_names[0], "LightAnim") != nullptr)
    {
        section.is_lightanim = true;
        section.textures[0]->SetRefCount(section.textures[0]->GetRefCount() - 1);
        section.textures[0] = nullptr;
    }
}
```

### D. Skip during rendering (Line ~5721)
```cpp
for (const auto& section_ptr : block->sections)
{
    if (!section_ptr) { continue; }
    auto& section = *section_ptr;

    // Skip LightAnim sections - they would overwrite textured sections
    if (section.is_lightanim) { continue; }

    // ... rest of rendering code ...
}
```

---

## Summary

| Fix | File | Line | Purpose |
|-----|------|------|---------|
| 1 | dtxmgr.cpp | ~391 | Textures load with correct dimensions |
| 2 | diligent_render.cpp | ~11140 | Uncompressed textures upload correctly |
| 3 | diligent_render.cpp | ~11047 | BC textures visible (not transparent) |
| 4 | diligent_render.cpp | ~11077 | BPP_32 textures visible |
| 5A | diligent_render.cpp | ~224 | LightAnim flag in struct |
| 5B | diligent_render.cpp | ~1469 | LightAnim flag initialization |
| 5C | diligent_render.cpp | ~1872 | LightAnim detection |
| 5D | diligent_render.cpp | ~5721 | LightAnim sections skipped |

All 5 fixes are required for correct texture rendering.

**Status**: All fixes have been implemented.
