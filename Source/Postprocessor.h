#ifndef _POSTPROCESSOR_H_
#define _POSTPROCESSOR_H_

class CDrawPrimitive;
class CMaterial;
class CTextureSystem;
class CRenderTargets;
class CDrawPrimitiveSystem;
class CMaterialSystem;
class CGeometrySystem;
class CRenderer;

class CPostprocessor
{
	public:
		CPostprocessor();
		~CPostprocessor();
		void Startup(CTextureSystem* pTextureSystem,
						CRenderTargets* pRenderTargets,
						CDrawPrimitiveSystem* pDrawPrimitiveSystem,
						CMaterialSystem* pMaterialSystem,
						CGeometrySystem* pGeometrySystem,
						CRenderer* pRenderer);
		CDrawPrimitive*	GetPostprocessPrimitive();

	private:
		CDrawPrimitive*	m_pPostprocessPrimitive;
		CMaterial*		m_pPostprocessDMaterial;
};

#endif