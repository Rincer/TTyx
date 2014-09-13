#ifndef _UTILITYDRAW_H_
#define _UTILITYDRAW_H_

#include "ConstantsSystem.h"
#include "Color.h"
#include "DoubleBuffer.h"
#include "MaterialSystem.h"
#include "Renderer.h"
#include "SegmentedBuffer.h"
#include "DrawPrimitive.h"
#include "Reference.h"

class	CVBuffer;
class	CIBuffer;
class	CCBuffer;
class	CMaterial;
class	IAllocator;
class	CRWBuffer;
class	CGeometrySystem;
class	CDrawPrimitive;

//-----------------------------------------------------------------------------------------------------
// Utility class for drawing basic shapes, not thread safe!
class CUtilityDraw
{
	public:
		CUtilityDraw(	CRenderer** ppRenderer, 
						CConstantsSystem** ppConstantsSystem, 
						CGeometrySystem** ppGeometrySystem, 
						CStringDictionary** ppStringDictionary,
						CMaterialSystem**	ppMaterialSystem,
						CDrawPrimitiveSystem** ppDrawPrimitiveSystem);
		~CUtilityDraw();
				
		void Tick();
		
		// Lines
		void BeginLineList();
		void AddLine(XMFLOAT3& Pt0, XMFLOAT3& Pt1, CColor& Color0, CColor& Color1);
		void EndLineList(XMMATRIX& LocalToWorld, const CMaterial* pMaterial, CRenderer::eDepthStates DepthState, CRenderer::eBlendStates BlendState);		
		
		// Triangles
		void BeginTriangleList();
		void AddTriangle(XMFLOAT3& Pt0, XMFLOAT3& Pt1, XMFLOAT3& Pt2, XMFLOAT2& Uv0, XMFLOAT2& Uv1, XMFLOAT2& Uv2, const CColor& Color0, const CColor& Color1, const CColor& Color2);
		void EndTriangleList(XMMATRIX& LocalToWorld, const CMaterial* pMaterial, CRenderer::eDepthStates DepthState, CRenderer::eBlendStates BlendState);	

		// Cube					
		void DrawCube(XMMATRIX& LocalToWorld, const CMaterial* pMaterial);
		
		// Sphere
		void DrawSphere(XMMATRIX& LocalToWorld, const CMaterial* pMaterial);

		void Render(ID3D11DeviceContext* pDeviceContext, ID3D11CommandList** ppCommandList, unsigned int VertexBuffer);
		
		const CMaterial* GetPlainColorMaterial();

	private:		
	
		enum eState
		{
			eNone,
			eTriangleList,
			eLineList
		};
		
		ID3D11CommandList*		m_pCommandList;
		CVBuffer*				m_pCubeVBuffer;
		CIBuffer*				m_pCubeIBuffer;		
		CVBuffer*				m_pSphereVBuffer;
		CIBuffer*				m_pSphereIBuffer;
		CVBuffer*				m_pVBuffers[3];	
		CVBuffer*				m_pVBuffersDeferred[3];
		CDrawPrimitive*			m_pSphereDrawPrimitive;

		// deferred contex rendering
		ID3D11DeviceContext*	m_pDeviceContext[3]; // For each buffer

		CMaterial*				m_pMaterialPlainColor;
		CSegmentedBuffer		m_StaticSegmentedBuffer;	
		CSegmentedBuffer		m_DynamicSegmentedBuffer;
		CRWBuffer*				m_pStaticRWBuffer;
		CRWBuffer*				m_pDynamicRWBuffer;
		unsigned int			m_CurrentVertexBuffer;				
		unsigned int			m_VertexOffset;
		unsigned int			m_StartVertexOffset;
		unsigned int			m_TotalVerts;	
		unsigned int			m_NumVerts;
		IAllocator*				m_pAllocator;
		eState					m_State;
		CReference<CRenderer*>				m_rRenderer;
		CReference<CConstantsSystem*>		m_rConstantsSystem;
		CReference<CGeometrySystem*>		m_rGeometrySystem;
		CReference<CStringDictionary*>		m_rStringDictionary;
		CReference<CMaterialSystem*>		m_rMaterialSystem;
		CReference<CDrawPrimitiveSystem*>	m_rDrawPrimitiveSystem;
		friend class CUtilityDrawCube;
};


#endif
