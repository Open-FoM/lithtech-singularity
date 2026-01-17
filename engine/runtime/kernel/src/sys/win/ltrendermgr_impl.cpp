#include "bdefs.h"
#include "ltrendermgr_impl.h"

#include "render.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"
#include "rendertargetmgr.h"
#include "rendertarget.h"

//instantiate our implementation class
define_interface(CLTRenderMgr, ILTRenderMgr);

namespace
{
Diligent::RefCntAutoPtr<Diligent::ITextureView> g_default_render_target;
Diligent::RefCntAutoPtr<Diligent::ITextureView> g_default_depth_stencil;
}

void CLTRenderMgr::Init()
{

}

void CLTRenderMgr::Term()
{
	g_default_render_target.Release();
	g_default_depth_stencil.Release();
}



LTRESULT CLTRenderMgr::CreateRenderTarget(uint32 nWidth, uint32 nHeight, ERenderTargetFormat eRenderTargetFormat, EStencilBufferFormat eStencilBufferFormat, HRENDERTARGET hRenderTarget)
{
	LTRESULT hr = CRenderTargetMgr::GetSingleton().AddRenderTarget(nWidth, nHeight, eRenderTargetFormat, eStencilBufferFormat, hRenderTarget);
	return hr;
}

LTRESULT CLTRenderMgr::InstallRenderTarget(HRENDERTARGET hRenderTarget)
{
	CRenderTarget* pRenderTarget = CRenderTargetMgr::GetSingleton().GetRenderTarget(hRenderTarget);
	if(pRenderTarget)
	{
		if(pRenderTarget->InstallOnDevice() == LT_OK)
		{
			CRenderTargetMgr::GetSingleton().SetCurrentRenderTargetHandle(hRenderTarget);
			return LT_OK;
		}
	}

	return LT_ERROR;
}

LTRESULT CLTRenderMgr::RemoveRenderTarget(HRENDERTARGET hRenderTarget)
{
	return CRenderTargetMgr::GetSingleton().RemoveRenderTarget(hRenderTarget);
}

LTRESULT CLTRenderMgr::StretchRectRenderTargetToBackBuffer(HRENDERTARGET hRenderTarget)
{
	CRenderTarget* pRenderTarget = CRenderTargetMgr::GetSingleton().GetRenderTarget(hRenderTarget);
	if(pRenderTarget)
	{
		auto* swap_chain = r_GetSwapChain();
		if (!swap_chain)
		{
			return LT_ERROR;
		}

		auto* back_buffer_view = swap_chain->GetCurrentBackBufferRTV();
		if (!back_buffer_view)
		{
			return LT_ERROR;
		}

		return pRenderTarget->StretchRectToSurface(back_buffer_view);
	}

	return LT_ERROR;
}

LTRESULT CLTRenderMgr::GetRenderTargetDims(HRENDERTARGET hRenderTarget, uint32& nWidth, uint32 nHeight)
{
	CRenderTarget* pRenderTarget = CRenderTargetMgr::GetSingleton().GetRenderTarget(hRenderTarget);
	if(pRenderTarget)
	{
		const RenderTargetParams params = pRenderTarget->GetRenderTargetParams();
		nWidth = params.Width;
		nHeight = params.Height;

		return LT_OK;
	}

	return LT_ERROR;
}

LTRESULT CLTRenderMgr::StoreDefaultRenderTarget()
{
	auto* swap_chain = r_GetSwapChain();
	if (!swap_chain)
	{
		return LT_ERROR;
	}

	g_default_render_target = swap_chain->GetCurrentBackBufferRTV();
	g_default_depth_stencil = swap_chain->GetDepthBufferDSV();
	return g_default_render_target ? LT_OK : LT_ERROR;
}

LTRESULT CLTRenderMgr::RestoreDefaultRenderTarget()
{
	auto* immediate_context = r_GetImmediateContext();
	auto* swap_chain = r_GetSwapChain();
	if (!immediate_context || !swap_chain)
	{
		return LT_ERROR;
	}

	auto* current_back_buffer = swap_chain->GetCurrentBackBufferRTV();
	auto* current_depth_buffer = swap_chain->GetDepthBufferDSV();
	auto* render_target = g_default_render_target ? g_default_render_target.RawPtr() : current_back_buffer;
	auto* depth_target = g_default_depth_stencil ? g_default_depth_stencil.RawPtr() : current_depth_buffer;

	// If the swap chain was resized, cached views may be stale; prefer current views.
	if (render_target && current_back_buffer && render_target->GetTexture() != current_back_buffer->GetTexture())
	{
		render_target = current_back_buffer;
	}
	if (depth_target && current_depth_buffer && depth_target->GetTexture() != current_depth_buffer->GetTexture())
	{
		depth_target = current_depth_buffer;
	}
	if (!render_target)
	{
		return LT_ERROR;
	}

	Diligent::ITextureView* render_targets[] = { render_target };
	immediate_context->SetRenderTargets(1, render_targets, depth_target, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	g_default_render_target.Release();
	g_default_depth_stencil.Release();

	CRenderTargetMgr::GetSingleton().SetCurrentRenderTargetHandle(INVALID_RENDER_TARGET);

	return LT_OK;
}


LTRESULT CLTRenderMgr::SnapshotCurrentFrame()
{
	return LT_ERROR;
}

LTRESULT CLTRenderMgr::SaveCurrentFrameToPrevious()
{
	return LT_ERROR;
}
