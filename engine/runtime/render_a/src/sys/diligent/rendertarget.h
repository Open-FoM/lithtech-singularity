#ifndef _RENDERTARGET_H_
#define _RENDERTARGET_H_

#include "ltbasedefs.h"
#include "iltrendermgr.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"

struct RenderTargetParams
{
	int					Width;
	int					Height;
	ERenderTargetFormat		RT_Format;
	EStencilBufferFormat	DS_Format;
};

class CRenderTarget
{
public:
	CRenderTarget();
	~CRenderTarget();

	LTRESULT	Init(int nWidth, int nHeight, ERenderTargetFormat eRTFormat, EStencilBufferFormat eDSFormat);
	LTRESULT	Term();
	LTRESULT	Recreate();
	LTRESULT	InstallOnDevice();
	LTRESULT	StretchRectToSurface(Diligent::ITextureView* dest_view);
	Diligent::ITexture* GetRenderTargetTexture() { return m_render_target; }
	const RenderTargetParams& GetRenderTargetParams() { return m_RenderTargetParams; }

protected:
	RenderTargetParams			m_RenderTargetParams;
	Diligent::RefCntAutoPtr<Diligent::ITexture> m_render_target;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> m_render_target_view;
	Diligent::RefCntAutoPtr<Diligent::ITexture> m_depth_stencil;
	Diligent::RefCntAutoPtr<Diligent::ITextureView> m_depth_stencil_view;
};

#endif