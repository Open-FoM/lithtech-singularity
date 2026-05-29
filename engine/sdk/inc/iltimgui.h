//-------------------------------------------------------------------
//
//   MODULE    : ILTIMGUI.H
//
//   PURPOSE   : Engine-provided Dear ImGui host interface.
//
//   CREATED   : FoM reconstruction - Dear ImGui GUI system
//
//-------------------------------------------------------------------
//
// The engine (LithTech Singularity) hosts a single Dear ImGui context and the
// Diligent + SDL3 ImGui backend, because only the engine owns the graphics
// device, swap chain, OS window and input event pump. Game modules (CShell)
// drive the UI by calling ImGui:: functions each frame; they obtain the shared
// context + allocator through this interface so that ImGui's per-module globals
// resolve to the one engine-owned context across the DLL boundary.
//
// Typical game-module usage (see CShell CGuiManager):
//   static ILTImGui *g_pLTImGui;
//   define_holder(ILTImGui, g_pLTImGui);
//   ...
//   ImGui::SetCurrentContext(g_pLTImGui->GetContext());
//   ImGuiMemAllocFunc a; ImGuiMemFreeFunc f; void *u;
//   g_pLTImGui->GetAllocatorFunctions(&a, &f, &u);
//   ImGui::SetAllocatorFunctions(a, f, u);
//   ...
//   g_pLTImGui->BeginFrame();      // once per frame, before any ImGui:: call
//   ImGui::Begin(...); ...; ImGui::End();
//   g_pLTImGui->EndFrameAndRender(); // before FlipScreen / present
//
//-------------------------------------------------------------------

#ifndef __ILTIMGUI_H__
#define __ILTIMGUI_H__

#ifndef __LTMODULE_H__
#include "ltmodule.h"
#endif

#ifndef __LTBASEDEFS_H__
#include "ltbasedefs.h" // HTEXTURE
#endif

#include "imgui.h" // ImGuiContext, ImGuiMemAllocFunc, ImGuiMemFreeFunc, ImTextureID

/*!
The ILTImGui interface exposes the engine-owned Dear ImGui host to game modules.

Define a holder to get this interface like this:
\code
define_holder(ILTImGui, your_var);
\endcode
*/
class ILTImGui : public IBase
{
public:
    interface_version(ILTImGui, 0);
    virtual ~ILTImGui() {}

    /*!
    Returns the engine-owned ImGui context. A consuming module must call
    ImGui::SetCurrentContext() with this value before issuing any ImGui:: calls.
    Returns nullptr if the host has not been initialized yet (no device).
    */
    virtual ImGuiContext *GetContext() = 0;

    /*!
    Returns the allocator functions used by the engine-owned ImGui context so a
    consuming module can call ImGui::SetAllocatorFunctions() with matching
    functions (required for cross-module allocation/free correctness).
    */
    virtual void GetAllocatorFunctions(ImGuiMemAllocFunc *pAllocFunc,
                                       ImGuiMemFreeFunc *pFreeFunc,
                                       void **ppUserData) = 0;

    /*!
    Begins a new ImGui frame (lazily initializes the host on first use once the
    render device + swap chain + window exist). Must be called once per frame
    before any ImGui:: widget calls. Safe to call before the device exists.

    \return true if a frame was started and ImGui:: widget calls are valid this
            frame; false if the host is not ready yet (no device) - in which case
            the caller must not issue ImGui:: calls and EndFrameAndRender() is a no-op.
    */
    virtual bool BeginFrame() = 0;

    /*!
    Ends the ImGui frame and renders the accumulated draw data to the active back
    buffer. Must be called after all ImGui:: widget calls and before the frame is
    presented (FlipScreen). No-op if BeginFrame() did nothing this frame.
    */
    virtual void EndFrameAndRender() = 0;

    /*!
    Converts a LithTech HTEXTURE into an ImTextureID usable with ImGui::Image().
    Returns 0 if the texture cannot be resolved.
    */
    virtual ImTextureID GetTextureId(HTEXTURE hTexture) = 0;

    /*!
    True if ImGui currently wants to capture mouse / keyboard input. Game input
    handling should suppress world interaction when these return true.
    */
    virtual bool WantCaptureMouse() = 0;
    virtual bool WantCaptureKeyboard() = 0;
};

#endif // __ILTIMGUI_H__
