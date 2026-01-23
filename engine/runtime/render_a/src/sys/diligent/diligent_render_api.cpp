#include "diligent_render_api.h"

#include "bdefs.h"
#include "diligent_device.h"
#include "diligent_postfx.h"
#include "diligent_state.h"
#include "diligent_model_draw.h"
#include "diligent_texture_cache.h"
#include "debuggeometry.h"
#include "strtools.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

namespace
{

SharedTexture* g_drawprim_texture = nullptr;
CRenderObject* g_render_object_list_head = nullptr;

} // namespace

void diligent_Clear(LTRect*, uint32, LTRGBColor& clear_color)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	diligent_ssao_set_clear_color(clear_color);

	const float clear_rgba[4] = {
		static_cast<float>(clear_color.rgb.r) / 255.0f,
		static_cast<float>(clear_color.rgb.g) / 255.0f,
		static_cast<float>(clear_color.rgb.b) / 255.0f,
		static_cast<float>(clear_color.rgb.a) / 255.0f};

	auto* render_target = diligent_get_active_render_target();
	auto* depth_target = diligent_get_active_depth_target();

	if (!render_target)
	{
		return;
	}

	g_diligent_state.immediate_context->ClearRenderTarget(
		render_target,
		clear_rgba,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	if (depth_target)
	{
		g_diligent_state.immediate_context->ClearDepthStencil(
			depth_target,
			Diligent::CLEAR_DEPTH_FLAG,
			1.0f,
			0,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	}
}

bool diligent_Start3D()
{
	g_diligent_state.is_in_3d = true;
	if (g_diligent_state.render_struct)
	{
		++g_diligent_state.render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_End3D()
{
	g_diligent_state.is_in_3d = false;
	if (g_diligent_state.render_struct && g_diligent_state.render_struct->m_nIn3D > 0)
	{
		--g_diligent_state.render_struct->m_nIn3D;
	}
	return true;
}

bool diligent_IsIn3D()
{
	return g_diligent_state.is_in_3d;
}

void diligent_RenderCommand(int, const char**)
{
}

void diligent_SwapBuffers(uint32)
{
	if (!diligent_EnsureSwapChain())
	{
		return;
	}

	g_diligent_state.swap_chain->Present(1);
}

bool diligent_LockScreen(int, int, int, int, void**, long*)
{
	return false;
}

void diligent_UnlockScreen()
{
}

void diligent_MakeScreenShot(const char*)
{
}

void diligent_MakeCubicEnvMap(const char*, uint32, const SceneDesc&)
{
}

void diligent_ReadConsoleVariables()
{
}

void diligent_GetRenderInfo(RenderInfoStruct*)
{
}

HRENDERCONTEXT diligent_CreateContext()
{
	void* context = nullptr;
	LT_MEM_TRACK_ALLOC(context = LTMemAlloc(1), LT_MEM_TYPE_RENDERER);
	return static_cast<HRENDERCONTEXT>(context);
}

void diligent_DeleteContext(HRENDERCONTEXT context)
{
	if (context)
	{
		LTMemFree(context);
	}
}

void diligent_BindTexture(SharedTexture* texture, bool texture_changed)
{
	diligent_get_texture_view(texture, texture_changed);
}

void diligent_UnbindTexture(SharedTexture* texture)
{
	diligent_release_render_texture(texture);
}

uint32 diligent_GetTextureFormat1(BPPIdent, uint32)
{
	return 0;
}

bool diligent_QueryDDSupport(PFormat*)
{
	return false;
}

bool diligent_GetTextureFormat2(BPPIdent, uint32, PFormat*)
{
	return false;
}

bool diligent_ConvertTexDataToDD(uint8*, PFormat*, uint32, uint32, uint8*, PFormat*, BPPIdent, uint32, uint32, uint32)
{
	return false;
}

void diligent_DrawPrimSetTexture(SharedTexture* texture)
{
	g_drawprim_texture = texture;
	diligent_get_texture_view(texture, false);
}

void diligent_DrawPrimDisableTextures()
{
}

CRenderObject* diligent_CreateRenderObject(CRenderObject::RENDER_OBJECT_TYPES object_type)
{
	CRenderObject* new_object = nullptr;
	switch (object_type)
	{
		case CRenderObject::eDebugLine:
			LT_MEM_TRACK_ALLOC(new_object = new CDIDebugLine(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eDebugPolygon:
			LT_MEM_TRACK_ALLOC(new_object = new CDIDebugPolygon(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eDebugText:
			LT_MEM_TRACK_ALLOC(new_object = new CDIDebugText(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eRigidMesh:
			LT_MEM_TRACK_ALLOC(new_object = new DiligentRigidMesh(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eSkelMesh:
			LT_MEM_TRACK_ALLOC(new_object = new DiligentSkelMesh(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eVAMesh:
			LT_MEM_TRACK_ALLOC(new_object = new DiligentVAMesh(), LT_MEM_TYPE_RENDERER);
			break;
		case CRenderObject::eNullMesh:
			LT_MEM_TRACK_ALLOC(new_object = new CDIModelDrawable(), LT_MEM_TYPE_RENDERER);
			break;
		default:
			return nullptr;
	}

	if (new_object)
	{
		new_object->SetNext(g_render_object_list_head);
		g_render_object_list_head = new_object;
	}
	return new_object;
}

bool diligent_DestroyRenderObject(CRenderObject* object)
{
	if (!object)
	{
		return false;
	}

	if (object == g_render_object_list_head)
	{
		g_render_object_list_head = object->GetNext();
	}
	else
	{
		if (!g_render_object_list_head)
		{
			return false;
		}
		CRenderObject* previous = g_render_object_list_head;
		while (previous->GetNext() && previous->GetNext() != object)
		{
			previous = previous->GetNext();
		}
		if (previous->GetNext())
		{
			previous->SetNext(previous->GetNext()->GetNext());
		}
		else
		{
			return false;
		}
	}

	delete object;
	return true;
}

#if !defined(LTJS_USE_DILIGENT_RENDER)
void* diligent_GetD3DDevice()
{
	return nullptr;
}
#endif

Diligent::IRenderDevice* diligent_GetRenderDevice()
{
	return g_diligent_state.render_device.RawPtr();
}

Diligent::IDeviceContext* diligent_GetImmediateContext()
{
	return g_diligent_state.immediate_context.RawPtr();
}

Diligent::ISwapChain* diligent_GetSwapChain()
{
	return g_diligent_state.swap_chain.RawPtr();
}

RMode* rdll_GetSupportedModes()
{
	RMode* mode = nullptr;
	LT_MEM_TRACK_ALLOC(mode = new RMode, LT_MEM_TYPE_RENDERER);
	if (!mode)
	{
		return nullptr;
	}

	LTStrCpy(mode->m_Description, "Diligent (Vulkan)", sizeof(mode->m_Description));
	LTStrCpy(mode->m_InternalName, "DiligentVulkan", sizeof(mode->m_InternalName));
	mode->m_Width = 1280;
	mode->m_Height = 720;
	mode->m_BitDepth = 32;
	mode->m_bHWTnL = true;
	mode->m_pNext = LTNULL;
	return mode;
}

void rdll_FreeModeList(RMode* modes)
{
	auto* current = modes;
	while (current)
	{
		auto* next = current->m_pNext;
		LTMemFree(current);
		current = next;
	}
}

Diligent::ITextureView* diligent_get_drawprim_texture_view(SharedTexture* shared_texture, bool texture_changed)
{
	return diligent_get_texture_view(shared_texture, texture_changed);
}

void diligent_render_api_term()
{
	g_drawprim_texture = nullptr;
	while (g_render_object_list_head)
	{
		CRenderObject* next = g_render_object_list_head->GetNext();
		delete g_render_object_list_head;
		g_render_object_list_head = next;
	}
}
