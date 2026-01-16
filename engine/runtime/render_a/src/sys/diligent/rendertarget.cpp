#include "bdefs.h"
#include "rendertarget.h"
#include "render.h"

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"

#include <cstring>

Diligent::TEXTURE_FORMAT GetDiligentFormatFromRTFormat(ERenderTargetFormat eFormat)
{
	switch (eFormat)
	{
		case RTFMT_A8R8G8B8:
			return Diligent::TEX_FORMAT_RGBA8_UNORM;
		case RTFMT_X8R8G8B8:
			return Diligent::TEX_FORMAT_BGRA8_UNORM;
		case RTFMT_R16F:
			return Diligent::TEX_FORMAT_R16_FLOAT;
		case RTFMT_R32F:
			return Diligent::TEX_FORMAT_R32_FLOAT;
		default:
			return Diligent::TEX_FORMAT_RGBA8_UNORM;
	}
}

Diligent::TEXTURE_FORMAT GetDiligentFormatFromDSFormat(EStencilBufferFormat eFormat)
{
	switch (eFormat)
	{
		case STFMT_D24S8:
			return Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;
		case STFMT_D24X8:
			return Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;
		case STFMT_D24X4S4:
			return Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;
		case STFMT_D32:
			return Diligent::TEX_FORMAT_D32_FLOAT;
		case STFMT_D15S1:
			return Diligent::TEX_FORMAT_D16_UNORM;
		case STFMT_D16:
			return Diligent::TEX_FORMAT_D16_UNORM;
		default:
			return Diligent::TEX_FORMAT_UNKNOWN;
	}
}

CRenderTarget::CRenderTarget()
	: m_render_target(nullptr)
	, m_render_target_view(nullptr)
	, m_depth_stencil(nullptr)
	, m_depth_stencil_view(nullptr)
{
	std::memset(&m_RenderTargetParams, 0, sizeof(RenderTargetParams));
}

CRenderTarget::~CRenderTarget()
{
	Term();
}

LTRESULT CRenderTarget::Init(int nWidth, int nHeight, ERenderTargetFormat eRTFormat, EStencilBufferFormat eDSFormat)
{
	m_RenderTargetParams.Width = nWidth;
	m_RenderTargetParams.Height = nHeight;
	m_RenderTargetParams.RT_Format = eRTFormat;
	m_RenderTargetParams.DS_Format = eDSFormat;

	return Recreate();
}

LTRESULT CRenderTarget::Recreate()
{
	Term();

	auto* render_device = r_GetRenderDevice();
	auto* immediate_context = r_GetImmediateContext();
	if (!render_device || !immediate_context)
	{
		return LT_ERROR;
	}

	const auto rt_format = GetDiligentFormatFromRTFormat(m_RenderTargetParams.RT_Format);
	Diligent::TextureDesc rt_desc;
	rt_desc.Name = "ltjs_render_target";
	rt_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	rt_desc.Width = static_cast<Diligent::Uint32>(m_RenderTargetParams.Width);
	rt_desc.Height = static_cast<Diligent::Uint32>(m_RenderTargetParams.Height);
	rt_desc.MipLevels = 1;
	rt_desc.Format = rt_format;
	rt_desc.Usage = Diligent::USAGE_DEFAULT;
	rt_desc.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;

	render_device->CreateTexture(rt_desc, nullptr, &m_render_target);
	if (!m_render_target)
	{
		dsi_ConsolePrint("Error creating %dx%d rendertarget texture: CLTRenderMgr::CreateRenderTarget() with RenderTargetFormat (%d)", m_RenderTargetParams.Width, m_RenderTargetParams.Height, m_RenderTargetParams.RT_Format);
		return LT_ERROR;
	}

	m_render_target_view = m_render_target->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
	if (!m_render_target_view)
	{
		dsi_ConsolePrint("Failed to create render target view: CRenderTarget::Recreate()");
		return LT_ERROR;
	}

	m_render_target_srv = m_render_target->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
	if (!m_render_target_srv)
	{
		dsi_ConsolePrint("Failed to create render target SRV: CRenderTarget::Recreate()");
		return LT_ERROR;
	}

	auto ds_format = GetDiligentFormatFromDSFormat(m_RenderTargetParams.DS_Format);
	if (ds_format == Diligent::TEX_FORMAT_UNKNOWN)
	{
		ds_format = Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;
	}

	Diligent::TextureDesc ds_desc;
	ds_desc.Name = "ltjs_depth_stencil";
	ds_desc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	ds_desc.Width = static_cast<Diligent::Uint32>(m_RenderTargetParams.Width);
	ds_desc.Height = static_cast<Diligent::Uint32>(m_RenderTargetParams.Height);
	ds_desc.MipLevels = 1;
	ds_desc.Format = ds_format;
	ds_desc.Usage = Diligent::USAGE_DEFAULT;
	ds_desc.BindFlags = Diligent::BIND_DEPTH_STENCIL;

	render_device->CreateTexture(ds_desc, nullptr, &m_depth_stencil);
	if (!m_depth_stencil)
	{
		dsi_ConsolePrint("Failed to create depth stencil: CRenderTarget::Recreate()");
		return LT_ERROR;
	}

	m_depth_stencil_view = m_depth_stencil->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
	if (!m_depth_stencil_view)
	{
		dsi_ConsolePrint("Failed to create depth stencil view: CRenderTarget::Recreate()");
		return LT_ERROR;
	}

	return LT_OK;
}

LTRESULT CRenderTarget::Term()
{
	m_render_target_view.Release();
	m_render_target_srv.Release();
	m_render_target.Release();
	m_depth_stencil_view.Release();
	m_depth_stencil.Release();

	return LT_OK;
}

LTRESULT CRenderTarget::InstallOnDevice()
{
	if (!m_render_target_view || !m_depth_stencil_view)
	{
		return LT_ERROR;
	}

	auto* immediate_context = r_GetImmediateContext();
	if (!immediate_context)
	{
		return LT_ERROR;
	}

	Diligent::ITextureView* render_targets[] = { m_render_target_view };
	immediate_context->SetRenderTargets(1, render_targets, m_depth_stencil_view, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	const float clear_rgba[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	immediate_context->ClearRenderTarget(m_render_target_view, clear_rgba, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	immediate_context->ClearDepthStencil(m_depth_stencil_view, Diligent::CLEAR_DEPTH_FLAG | Diligent::CLEAR_STENCIL_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	return LT_OK;
}

LTRESULT CRenderTarget::StretchRectToSurface(Diligent::ITextureView* dest_view)
{
	if (!m_render_target || !dest_view)
	{
		return LT_ERROR;
	}

	auto* immediate_context = r_GetImmediateContext();
	if (!immediate_context)
	{
		return LT_ERROR;
	}

	auto* dest_texture = dest_view->GetTexture();
	if (!dest_texture)
	{
		return LT_ERROR;
	}

	const auto src_desc = m_render_target->GetDesc();
	const auto dst_desc = dest_texture->GetDesc();
	if (src_desc.Width != dst_desc.Width || src_desc.Height != dst_desc.Height)
	{
		dsi_ConsolePrint("Render target size mismatch for copy: %ux%u -> %ux%u", src_desc.Width, src_desc.Height, dst_desc.Width, dst_desc.Height);
		return LT_ERROR;
	}

	Diligent::CopyTextureAttribs copy_attribs;
	copy_attribs.pSrcTexture = m_render_target;
	copy_attribs.pDstTexture = dest_texture;
	copy_attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	copy_attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
	immediate_context->CopyTexture(copy_attribs);
	return LT_OK;
}
