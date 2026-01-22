#ifndef LTJS_DILIGENT_RENDER_API_H
#define LTJS_DILIGENT_RENDER_API_H

#include "ltbasedefs.h"
#include "renderstruct.h"

class SharedTexture;

namespace Diligent
{
	class IDeviceContext;
	class IRenderDevice;
	class ISwapChain;
	class ITextureView;
}

void diligent_Clear(LTRect* rect, uint32 flags, LTRGBColor& clear_color);
bool diligent_Start3D();
bool diligent_End3D();
bool diligent_IsIn3D();

void diligent_RenderCommand(int argc, const char** argv);
void diligent_SwapBuffers(uint32 flags);
bool diligent_LockScreen(int left, int top, int right, int bottom, void** out_buffer, long* out_pitch);
void diligent_UnlockScreen();
void diligent_MakeScreenShot(const char* filename);
void diligent_MakeCubicEnvMap(const char* filename, uint32 size, const SceneDesc& scene);
void diligent_ReadConsoleVariables();
void diligent_GetRenderInfo(RenderInfoStruct* info);

HRENDERCONTEXT diligent_CreateContext();
void diligent_DeleteContext(HRENDERCONTEXT context);

void diligent_BindTexture(SharedTexture* texture, bool texture_changed);
void diligent_UnbindTexture(SharedTexture* texture);
uint32 diligent_GetTextureFormat1(BPPIdent format, uint32 flags);
bool diligent_QueryDDSupport(PFormat* format);
bool diligent_GetTextureFormat2(BPPIdent format, uint32 flags, PFormat* out_format);
bool diligent_ConvertTexDataToDD(
	uint8* src_data,
	PFormat* src_format,
	uint32 src_width,
	uint32 src_height,
	uint8* dest_data,
	PFormat* dest_format,
	BPPIdent dest_format_id,
	uint32 dest_width,
	uint32 dest_height,
	uint32 flags);

void diligent_DrawPrimSetTexture(SharedTexture* texture);
void diligent_DrawPrimDisableTextures();

CRenderObject* diligent_CreateRenderObject(CRenderObject::RENDER_OBJECT_TYPES object_type);
bool diligent_DestroyRenderObject(CRenderObject* object);

#if !defined(LTJS_USE_DILIGENT_RENDER)
void* diligent_GetD3DDevice();
#endif
Diligent::IRenderDevice* diligent_GetRenderDevice();
Diligent::IDeviceContext* diligent_GetImmediateContext();
Diligent::ISwapChain* diligent_GetSwapChain();

RMode* rdll_GetSupportedModes();
void rdll_FreeModeList(RMode* modes);

void diligent_render_api_term();

#endif
