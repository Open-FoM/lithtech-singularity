#ifndef __DILIGENT_DRAWPRIM_H__
#define __DILIGENT_DRAWPRIM_H__

#ifndef __GENDRAWPRIM_H__
#include "gendrawprim.h"
#endif

#include <memory>
#include <vector>

#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h"

struct DiligentDrawPrimResources;

class CDiligentDrawPrim : public CGenDrawPrim
{
public:
	declare_interface(CDiligentDrawPrim);

	CDiligentDrawPrim();

	LTRESULT BeginDrawPrim() override;
	LTRESULT EndDrawPrim() override;

	LTRESULT SetCamera(const HOBJECT hCamera) override;
	LTRESULT SetTexture(const HTEXTURE hTexture) override;
	LTRESULT SetTransformType(const ELTTransformType eType) override;
	LTRESULT SetColorOp(const ELTColorOp eColorOp) override;
	LTRESULT SetAlphaBlendMode(const ELTBlendMode eBlendMode) override;
	LTRESULT SetZBufferMode(const ELTZBufferMode eZBufferMode) override;
	LTRESULT SetAlphaTestMode(const ELTTestMode eTestMode) override;
	LTRESULT SetClipMode(const ELTClipMode eClipMode) override;
	LTRESULT SetFillMode(ELTDPFillMode eFillMode) override;
	LTRESULT SetCullMode(ELTDPCullMode eCullMode) override;
	LTRESULT SetFogEnable(bool bFogEnable) override;
	LTRESULT SetReallyClose(bool bReallyClose) override;
	LTRESULT SetEffectShaderID(uint32 nEffectShaderID) override;
	void SaveViewport() override;
	void RestoreViewport() override;

	void SetUVWH(LT_POLYGT4* pPrim, HTEXTURE pTex, float u, float v, float w, float h) override;

	LTRESULT DrawPrim(LT_POLYGT3* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYFT3* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYG3* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYF3* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYGT4* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYGT4** ppPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYFT4* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYG4* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_POLYF4* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_LINEGT* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_LINEFT* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_LINEG* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrim(LT_LINEF* pPrim, const uint32 nCount = 1) override;
	LTRESULT DrawPrimPoint(LT_VERTGT* pVerts, const uint32 nCount = 1) override;
	LTRESULT DrawPrimPoint(LT_VERTG* pVerts, const uint32 nCount = 1) override;
	LTRESULT DrawPrimFan(LT_VERTGT* pVerts, const uint32 nCount) override;
	LTRESULT DrawPrimFan(LT_VERTFT* pVerts, const uint32 nCount, LT_VERTRGBA rgba) override;
	LTRESULT DrawPrimFan(LT_VERTG* pVerts, const uint32 nCount) override;
	LTRESULT DrawPrimFan(LT_VERTF* pVerts, const uint32 nCount, LT_VERTRGBA rgba) override;
	LTRESULT DrawPrimStrip(LT_VERTGT* pVerts, const uint32 nCount) override;
	LTRESULT DrawPrimStrip(LT_VERTFT* pVerts, const uint32 nCount, LT_VERTRGBA rgba) override;
	LTRESULT DrawPrimStrip(LT_VERTG* pVerts, const uint32 nCount) override;
	LTRESULT DrawPrimStrip(LT_VERTF* pVerts, const uint32 nCount, LT_VERTRGBA rgba) override;

public:
	struct DrawPrimVertex
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 0.0f;
		float u = 0.0f;
		float v = 0.0f;
	};

private:
	static DrawPrimVertex MakeVertex(float x, float y, float z, const LT_VERTRGBA& rgba, float u, float v);
	static void AppendTriangle(std::vector<DrawPrimVertex>& vertices, const DrawPrimVertex& first, const DrawPrimVertex& second, const DrawPrimVertex& third);

	bool VerifyValid() const;
	bool UpdateTransformMatrix();
	void BuildViewport(Diligent::Viewport& viewport) const;
	void ApplyViewport();
	LTRESULT SubmitDraw(const std::vector<DrawPrimVertex>& vertices, Diligent::PRIMITIVE_TOPOLOGY topology, bool textured);

	std::unique_ptr<DiligentDrawPrimResources> m_resources;
	int32 m_block_count = 0;
	bool m_transform_dirty = true;
	Diligent::Viewport m_saved_viewport{};
	bool m_has_saved_viewport = false;
};

#endif
