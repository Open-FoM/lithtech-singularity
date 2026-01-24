#include "bdefs.h"
#include "diligent_2d_draw.h"
#include "diligent_postfx.h"
#include "diligent_render_api.h"
#include "diligent_renderer_lifecycle.h"
#include "diligent_scene_render.h"
#include "diligent_state.h"
#include "diligent_world_api.h"
#include "renderstruct.h"

void rdll_RenderDLLSetup(RenderStruct *render_struct) {
  g_diligent_state.render_struct = render_struct;

  render_struct->Init = diligent_Init;
  render_struct->Term = diligent_Term;
#if !defined(LTJS_USE_DILIGENT_RENDER)
  render_struct->GetD3DDevice = diligent_GetD3DDevice;
#endif
  render_struct->GetRenderDevice = diligent_GetRenderDevice;
  render_struct->GetImmediateContext = diligent_GetImmediateContext;
  render_struct->GetSwapChain = diligent_GetSwapChain;
  render_struct->BindTexture = diligent_BindTexture;
  render_struct->UnbindTexture = diligent_UnbindTexture;
  render_struct->GetTextureDDFormat1 = diligent_GetTextureFormat1;
  render_struct->QueryDDSupport = diligent_QueryDDSupport;
  render_struct->GetTextureDDFormat2 = diligent_GetTextureFormat2;
  render_struct->ConvertTexDataToDD = diligent_ConvertTexDataToDD;
  render_struct->DrawPrimSetTexture = diligent_DrawPrimSetTexture;
  render_struct->DrawPrimDisableTextures = diligent_DrawPrimDisableTextures;
  render_struct->CreateContext = diligent_CreateContext;
  render_struct->DeleteContext = diligent_DeleteContext;
  render_struct->Clear = diligent_Clear;
  render_struct->Start3D = diligent_Start3D;
  render_struct->End3D = diligent_End3D;
  render_struct->IsIn3D = diligent_IsIn3D;
  render_struct->StartOptimized2D = diligent_StartOptimized2D;
  render_struct->EndOptimized2D = diligent_EndOptimized2D;
  render_struct->IsInOptimized2D = diligent_IsInOptimized2D;
  render_struct->SetOptimized2DBlend = diligent_SetOptimized2DBlend;
  render_struct->GetOptimized2DBlend = diligent_GetOptimized2DBlend;
  render_struct->SetOptimized2DColor = diligent_SetOptimized2DColor;
  render_struct->GetOptimized2DColor = diligent_GetOptimized2DColor;
  render_struct->RenderScene = diligent_RenderScene;
  render_struct->RenderCommand = diligent_RenderCommand;
  render_struct->SwapBuffers = diligent_SwapBuffers;
  render_struct->GetScreenFormat = diligent_GetScreenFormat;
  render_struct->CreateSurface = diligent_CreateSurface;
  render_struct->DeleteSurface = diligent_DeleteSurface;
  render_struct->GetSurfaceInfo = diligent_GetSurfaceInfo;
  render_struct->LockSurface = diligent_LockSurface;
  render_struct->UnlockSurface = diligent_UnlockSurface;
  render_struct->OptimizeSurface = diligent_OptimizeSurface;
  render_struct->UnoptimizeSurface = diligent_UnoptimizeSurface;
  render_struct->LockScreen = diligent_LockScreen;
  render_struct->UnlockScreen = diligent_UnlockScreen;
  render_struct->BlitToScreen = diligent_BlitToScreen;
  render_struct->BlitFromScreen = diligent_BlitFromScreen;
  render_struct->WarpToScreen = diligent_WarpToScreen;
  render_struct->MakeScreenShot = diligent_MakeScreenShot;
  render_struct->MakeCubicEnvMap = diligent_MakeCubicEnvMap;
  render_struct->ReadConsoleVariables = diligent_ReadConsoleVariables;
  render_struct->GetRenderInfo = diligent_GetRenderInfo;
  render_struct->CreateRenderObject = diligent_CreateRenderObject;
  render_struct->DestroyRenderObject = diligent_DestroyRenderObject;
  render_struct->LoadWorldData = diligent_LoadWorldData;
  render_struct->SetLightGroupColor = diligent_SetLightGroupColor;
  render_struct->SetOccluderEnabled = diligent_SetOccluderEnabled;
  render_struct->GetOccluderEnabled = diligent_GetOccluderEnabled;
  render_struct->GetTextureEffectVarID = diligent_GetTextureEffectVarID;
  render_struct->SetTextureEffectVar = diligent_SetTextureEffectVar;
  render_struct->IsObjectGroupEnabled = diligent_IsObjectGroupEnabled;
  render_struct->SetObjectGroupEnabled = diligent_SetObjectGroupEnabled;
  render_struct->SetAllObjectGroupEnabled = diligent_SetAllObjectGroupEnabled;
  render_struct->AddGlowRenderStyleMapping = diligent_AddGlowRenderStyleMapping;
  render_struct->SetGlowDefaultRenderStyle = diligent_SetGlowDefaultRenderStyle;
  render_struct->SetNoGlowRenderStyle = diligent_SetNoGlowRenderStyle;
}
