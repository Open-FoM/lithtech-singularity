/**
 * rendertargetmgr.h
 *
 * This header defines the Rendertargetmgr portion of the Diligent renderer.
 * It declares the primary types and functions used by other renderer units
 * and documents the responsibilities and expectations at this interface.
 * Implementations live in the corresponding .cpp unless noted otherwise.
 */
#ifndef _RENDERTARGETMGR_H_
#define _RENDERTARGETMGR_H_

#include "ltbasedefs.h"
#include "iltrendermgr.h"
#include <map>

class CRenderTarget;
typedef std::map<HRENDERTARGET, CRenderTarget*> LTRenderTargetMap;

class CRenderTargetMgr
{
public:
	~CRenderTargetMgr();

	static CRenderTargetMgr&	GetSingleton();

	LTRESULT Init();
	LTRESULT Term();

	LTRESULT		AddRenderTarget(int Width, int Height, ERenderTargetFormat eRTFormat, EStencilBufferFormat eDSFormat, HRENDERTARGET hRenderTarget);
	CRenderTarget*	GetRenderTarget(HRENDERTARGET hRenderTarget);
	LTRESULT		RemoveRenderTarget(HRENDERTARGET hRenderTarget);
	void			SetCurrentRenderTargetHandle(HRENDERTARGET hRenderTarget){m_hRenderTarget = hRenderTarget;}
	HRENDERTARGET	GetCurrentRenderTargetHandle(){return m_hRenderTarget;}

	void						FreeDeviceObjects();
	void						RecreateRenderTargets();

protected:

	LTRenderTargetMap m_RenderTargets;
	HRENDERTARGET		m_hRenderTarget;
};

#endif