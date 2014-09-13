#include "stdafx.h"
#include "GeometrySystem.h"
#include "Renderer.h"
#include "ConstantsSystem.h"
#include "DoubleBuffer.h"
#include "VertexFormats.h"
#include "MemoryStream.h"
#include "MaterialSystem.h"
#include "Hash64.h"
#include "StringDictionary.h"
#include "StackAllocator.h"
#include "RWBuffer.h"
#include "Macros.h"
#include "Time.h"
#include "MaterialPlainColor.h"
#include "VertexProcessing.h"
#include "DrawPrimitive.h"

#include "UtilityDraw.h"


static const unsigned int scUtilityDrawCommandBufferSize = 128 * 1024;
static const unsigned int scMaxNumVerices = 12 * 1024;


// Create vertex buffer
static SPosTexColor sCubeVerts[] =
{
	{ XMFLOAT3( -0.5f, 0.5f, -0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x000000FF},
    { XMFLOAT3( -0.5f,-0.5f, -0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x0000FF00},
    { XMFLOAT3(  0.5f,-0.5f, -0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x00FF0000},
    { XMFLOAT3(  0.5f, 0.5f, -0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x0000FFFF},
	{ XMFLOAT3( -0.5f, 0.5f,  0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x000000FF},
    { XMFLOAT3( -0.5f,-0.5f,  0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x0000FF00},
    { XMFLOAT3(  0.5f,-0.5f,  0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x00FF0000},
    { XMFLOAT3(  0.5f, 0.5f,  0.5f ), XMFLOAT2(0.0f, 0.0f ), 0x0000FFFF}    
};

static unsigned short sCubeIndices[] =
{
	0, 3, 1,
	3, 2, 1,
	3, 7, 2,
	7, 6, 2,
	7, 4, 6,
	4, 5, 6,
	4, 0, 5,
	0, 1, 5,
	4, 7, 0,
	7, 3, 0,
	1, 2, 5,
	2, 6, 5
};

static unsigned int scSphereSegments = 8;

//------------------------------------------------------------------------------------
class IUtilityDrawCommand
{
	public:			
		IUtilityDrawCommand() : m_pMaterial(NULL)
		{
		}	
		
		IUtilityDrawCommand(const CMaterial* pMaterial) : m_pMaterial(pMaterial)
		{
		}
		
		virtual void Execute(ID3D11DeviceContext* pDeviceContext, bool& bEndFrame, const CMaterial** ppCurrMaterial) = 0;		
		virtual int Size() = 0;
		bool SetMaterial(ID3D11DeviceContext* pDeviceContext, const CMaterial** ppCurrMaterial)
		{
			if(!m_pMaterial->IsLoaded())
				return false;
				
			if(*ppCurrMaterial != m_pMaterial)
			{
				*ppCurrMaterial = m_pMaterial;
				m_pMaterial->Set(pDeviceContext);
			}			
			return true;
		}
		
	protected:
		const CMaterial* m_pMaterial; 
};

//------------------------------------------------------------------------------------
class CUtilityDrawCommandTriangleList : public IUtilityDrawCommand
{
	public:
		CUtilityDrawCommandTriangleList(XMMATRIX& LocalToWorld,
										unsigned int VertexOffset,
										unsigned int NumVertices,
										const CMaterial* pMaterial,
										CRenderer* pRenderer,
										CConstantsSystem* pConstantsSystem,
										CRenderer::eDepthStates DepthState,
										CRenderer::eBlendStates BlendState) :	IUtilityDrawCommand(pMaterial),	
																				m_VertexOffset(VertexOffset),
																				m_NumVertices(NumVertices),
																				m_pRenderer(pRenderer),
																				m_pConstantsSystem(pConstantsSystem),
																				m_DepthState(DepthState),
																				m_BlendState(BlendState)
																												
		{
			memcpy(&m_LocalToWorld, &LocalToWorld, sizeof(XMMATRIX));
		}

		virtual void Execute(ID3D11DeviceContext* pDeviceContext, bool& bEndFrame, const CMaterial** ppCurrMaterial)
		{			
			bEndFrame;
			if(!SetMaterial(pDeviceContext, ppCurrMaterial))
				return;
			m_pConstantsSystem->UpdateConstantBuffer(CConstantsSystem::eLocalToWorld, pDeviceContext, &m_LocalToWorld, sizeof(m_LocalToWorld));
			m_pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLocalToWorld, pDeviceContext);
			pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );	
			m_pRenderer->SetDepthState(pDeviceContext, m_DepthState);
			m_pRenderer->SetBlendState(pDeviceContext, m_BlendState);
			pDeviceContext->Draw(m_NumVertices, m_VertexOffset);    		
		}	
		
		int Size()
		{
			return sizeof(*this);
		}		
	private:					
		XMMATRIX			m_LocalToWorld;
		unsigned int		m_VertexOffset;
		unsigned int		m_NumVertices;	
		CRenderer*			m_pRenderer;
		CConstantsSystem*	m_pConstantsSystem;
		CRenderer::eDepthStates m_DepthState;	
		CRenderer::eBlendStates m_BlendState;			
};

//------------------------------------------------------------------------------------
class CUtilityDrawCommandLineList : public IUtilityDrawCommand
{
	public:
		CUtilityDrawCommandLineList(XMMATRIX& LocalToWorld, 
									unsigned int VertexOffset, 
									unsigned int NumVertices, 
									const CMaterial* pMaterial,
									CRenderer*	pRenderer,
									CConstantsSystem*	pConstantsSystem,
									CRenderer::eDepthStates DepthState,
									CRenderer::eBlendStates BlendState) :	IUtilityDrawCommand(pMaterial),
																			m_VertexOffset(VertexOffset),
																			m_NumVertices(NumVertices),
																			m_pRenderer(pRenderer),
																			m_pConstantsSystem(pConstantsSystem),
																			m_DepthState(DepthState),
																			m_BlendState(BlendState)
																																		
																											
		{
			memcpy(&m_LocalToWorld, &LocalToWorld, sizeof(XMMATRIX));		
		}
		
		virtual void Execute(ID3D11DeviceContext* pDeviceContext, bool& bEndFrame, const CMaterial** ppCurrMaterial)
		{		
			bEndFrame;
			if(!SetMaterial(pDeviceContext, ppCurrMaterial))
				return;		
			m_pConstantsSystem->UpdateConstantBuffer(CConstantsSystem::eLocalToWorld, pDeviceContext, &m_LocalToWorld, sizeof(m_LocalToWorld));
			m_pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLocalToWorld, pDeviceContext);
			pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
			m_pRenderer->SetDepthState(pDeviceContext, m_DepthState);
			m_pRenderer->SetBlendState(pDeviceContext, m_BlendState);
			pDeviceContext->Draw(m_NumVertices, m_VertexOffset);   
		}				
		
		int Size()
		{
			return sizeof(*this);
		}		
		
		
	private:						
		XMMATRIX		m_LocalToWorld;
		unsigned int	m_VertexOffset;		
		unsigned int	m_NumVertices;	
		CRenderer*			m_pRenderer;
		CConstantsSystem*	m_pConstantsSystem;
		CRenderer::eDepthStates m_DepthState;	
		CRenderer::eBlendStates m_BlendState;							
};


//------------------------------------------------------------------------------------
class CUtilityDrawCube : public IUtilityDrawCommand
{
	public:
		CUtilityDrawCube(	XMMATRIX& LocalToWorld, 
							const CMaterial*	pMaterial, 
							CUtilityDraw*		pUtilityDraw,
							CConstantsSystem*	pConstantsSystem,
							CRenderer*			pRenderer) :IUtilityDrawCommand(pMaterial),
															m_pUtilityDraw(pUtilityDraw),
															m_pConstantsSystem(pConstantsSystem),
															m_pRenderer(pRenderer)


		{
			memcpy(&m_LocalToWorld, &LocalToWorld, sizeof(XMMATRIX));				
		}
		
		virtual void Execute(ID3D11DeviceContext* pDeviceContext, bool& bEndFrame, const CMaterial** ppCurrMaterial)
		{			
			bEndFrame;
			if(!SetMaterial(pDeviceContext, ppCurrMaterial))
				return;
			m_pConstantsSystem->UpdateConstantBuffer(CConstantsSystem::eLocalToWorld, pDeviceContext, &m_LocalToWorld, sizeof(m_LocalToWorld));
			m_pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLocalToWorld, pDeviceContext);
			m_pUtilityDraw->m_pCubeVBuffer->Set(0, pDeviceContext);
			m_pUtilityDraw->m_pCubeIBuffer->Set(pDeviceContext);
			pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );		 
			m_pRenderer->SetDepthState(pDeviceContext, CRenderer::eDepthLE);
			m_pRenderer->SetBlendState(pDeviceContext, CRenderer::eBlendNone);
			pDeviceContext->DrawIndexed(sizeof(sCubeIndices) / sizeof(unsigned short), 0, 0 );    		
		}				
		
		int Size()
		{
			return sizeof(*this);
		}		
		
		
	private:						
		XMMATRIX			m_LocalToWorld;
		CUtilityDraw*		m_pUtilityDraw;
		CConstantsSystem*	m_pConstantsSystem;
		CRenderer*			m_pRenderer;
};

//------------------------------------------------------------------------------------
class CUtilityDrawEndFrame : public IUtilityDrawCommand
{
	public:
		CUtilityDrawEndFrame() 																				
		{
		}
		
		virtual void Execute(ID3D11DeviceContext* pDeviceContext, bool& bEndFrame, const CMaterial** ppCurrMaterial)
		{			
			ppCurrMaterial;
			pDeviceContext;
			bEndFrame = true;
		}				
		
		int Size()
		{
			return sizeof(*this);
		}						
};

//------------------------------------------------------------------------------------
CUtilityDraw::CUtilityDraw(	CRenderer** ppRenderer, 
							CConstantsSystem** ppConstantsSystem, 
							CGeometrySystem** ppGeometrySystem, 
							CStringDictionary** ppStringDictionary,
							CMaterialSystem**	ppMaterialSystem,
							CDrawPrimitiveSystem** ppDrawPrimitiveSystem) : m_rRenderer(ppRenderer),
																			m_rConstantsSystem(ppConstantsSystem),
																			m_rGeometrySystem(ppGeometrySystem),
																			m_rStringDictionary(ppStringDictionary),
																			m_rMaterialSystem(ppMaterialSystem),
																			m_rDrawPrimitiveSystem(ppDrawPrimitiveSystem)

{
	m_State = eNone;		
	m_CurrentVertexBuffer = 0;	

// deferred contex rendering
	m_rRenderer->GetDevice()->CreateDeferredContext(0, &m_pDeviceContext[0]);
	m_rRenderer->GetDevice()->CreateDeferredContext(0, &m_pDeviceContext[1]);
	m_rRenderer->GetDevice()->CreateDeferredContext(0, &m_pDeviceContext[2]);

	m_pVBuffers[0] = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.VBuffer0", sizeof(SPosTexColor), NULL, scMaxNumVerices * sizeof(SVertexObj));
	m_pVBuffers[1] = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.VBuffer1", sizeof(SPosTexColor), NULL, scMaxNumVerices * sizeof(SVertexObj));
	m_pVBuffers[2] = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.VBuffer2", sizeof(SPosTexColor), NULL, scMaxNumVerices * sizeof(SVertexObj));

// deferred context rendering
	m_pVBuffersDeferred[0] = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.VBuffeDfrd0", sizeof(SPosTexColor), NULL, scMaxNumVerices * sizeof(SVertexObj));
	m_pVBuffersDeferred[1] = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.VBuffeDfrd1", sizeof(SPosTexColor), NULL, scMaxNumVerices * sizeof(SVertexObj));
	m_pVBuffersDeferred[2] = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.VBuffeDfrd2", sizeof(SPosTexColor), NULL, scMaxNumVerices * sizeof(SVertexObj));

	m_pVBuffersDeferred[0]->MapResourceData(m_pDeviceContext[0]);
	m_pVBuffersDeferred[1]->MapResourceData(m_pDeviceContext[1]);

	m_rRenderer->MapBufferImmediate(m_pVBuffers[0]);
	m_rRenderer->MapBufferImmediate(m_pVBuffers[1]);

	m_pCubeVBuffer = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.CubeVBuffer", sizeof(SPosTexColor), sCubeVerts, sizeof(sCubeVerts));
	m_pCubeIBuffer = &m_rGeometrySystem->CreateIBuffer("UtilityDraw.CubeIBuffer", sCubeIndices, sizeof(sCubeIndices));

	unsigned int Size = POW2ALIGN(POW2ALIGN(3 * scUtilityDrawCommandBufferSize, 16) + 3 * sizeof(CRWBuffer), 16);
	m_pAllocator = new CStackAllocator(2 * Size);

	unsigned short TotalVerts = (unsigned short)((scSphereSegments * 2 - 1) * scSphereSegments * 4 + 2);
	unsigned short NumTris = (unsigned short)(scSphereSegments * 4 * 2 + (scSphereSegments * 2 - 2) * (scSphereSegments * 4) * 2);
	unsigned short TotalIndices = NumTris * 3;

	SPos* pVerts = (SPos*)m_pAllocator->Alloc(TotalVerts * sizeof(SPos));
	unsigned short* pInds = (unsigned short*)m_pAllocator->Alloc(TotalIndices * sizeof(unsigned short));
	CVertexProcessing::Instance().CreateSphere(pVerts, pInds, (unsigned short)scSphereSegments);

	m_pSphereVBuffer = &m_rGeometrySystem->CreateVBuffer("UtilityDraw.SphereVBuffer", sizeof(SPos), pVerts, TotalVerts * sizeof(SPos));
	m_pSphereIBuffer = &m_rGeometrySystem->CreateIBuffer("UtilityDraw.SphereIBuffer", pInds, TotalIndices * sizeof(unsigned short));
	m_pAllocator->Reset();

	CMaterialPlainColor::CParameters Params("CUtilityDraw.PlainColorParams", CMaterialPlainColor::eWorldSpace);
	m_pMaterialPlainColor = &m_rMaterialSystem->GetMaterial<CMaterialPlainColor>(&Params);

	CVBuffer* pVBuffers[2] = { m_pSphereVBuffer, NULL };
	m_pSphereDrawPrimitive = &m_rDrawPrimitiveSystem->Add("UtilityDraw.SpherePrimitive", pVBuffers, 1, m_pSphereIBuffer, TotalIndices, m_pMaterialPlainColor);

	m_StaticSegmentedBuffer.Initialize(3, scUtilityDrawCommandBufferSize, m_pAllocator);
	m_DynamicSegmentedBuffer.Initialize(3, scUtilityDrawCommandBufferSize, m_pAllocator);
	m_pStaticRWBuffer = m_StaticSegmentedBuffer.GetWrite();
	m_pDynamicRWBuffer = m_DynamicSegmentedBuffer.GetWrite();
	
}

//------------------------------------------------------------------------------------
CUtilityDraw::~CUtilityDraw()
{
// deferred context rendering
	m_pDeviceContext[0]->Release();
	m_pDeviceContext[1]->Release();
	m_pDeviceContext[2]->Release();
}

//------------------------------------------------------------------------------------
void CUtilityDraw::BeginLineList()
{
	Assert(m_State == eNone);
	m_State = eLineList;		
	m_NumVerts = 0;
	m_StartVertexOffset = m_VertexOffset;
}

//------------------------------------------------------------------------------------
void CUtilityDraw::AddLine(XMFLOAT3& Pt0, XMFLOAT3& Pt1, CColor& Color0, CColor& Color1)
{
	Assert(m_State == eLineList);
	Assert(m_TotalVerts + m_NumVerts + 2 < scMaxNumVerices);	
	CVBuffer* pVBuffer = m_rRenderer->GetExecutionModel() == CRenderer::eExecutionModel::eMultithreadedDeferred ? m_pVBuffersDeferred[m_CurrentVertexBuffer] : m_pVBuffers[m_CurrentVertexBuffer];
	SPosTexColor* pVertexData = (SPosTexColor*)pVBuffer->GetResourceData();
	Assert(pVertexData);
	pVertexData[m_VertexOffset].m_Pos = Pt0;
	pVertexData[m_VertexOffset].m_Col = Color0.m_Color.m_Argb;
	m_VertexOffset++;
	m_NumVerts++;
	pVertexData[m_VertexOffset].m_Pos = Pt1;
	pVertexData[m_VertexOffset].m_Col = Color1.m_Color.m_Argb;
	m_VertexOffset++;
	m_NumVerts++;	
}

//------------------------------------------------------------------------------------
void CUtilityDraw::EndLineList(XMMATRIX& LocalToWorld, const CMaterial* pMaterial, CRenderer::eDepthStates DepthState, CRenderer::eBlendStates BlendState)
{
	Assert(m_State == eLineList);
	void* pData;
	m_pDynamicRWBuffer->BeginWrite(&pData, sizeof(CUtilityDrawCommandLineList));
	new(pData) CUtilityDrawCommandLineList(LocalToWorld, m_StartVertexOffset, m_NumVerts, pMaterial, *m_rRenderer, *m_rConstantsSystem, DepthState, BlendState);	
	m_pDynamicRWBuffer->EndWrite(sizeof(CUtilityDrawCommandLineList));
	m_State = eNone;
}

//------------------------------------------------------------------------------------
void CUtilityDraw::BeginTriangleList()
{
	Assert(m_State == eNone);
	m_State = eTriangleList;	
	m_NumVerts = 0;
	m_StartVertexOffset = m_VertexOffset;
}

//------------------------------------------------------------------------------------
void CUtilityDraw::AddTriangle(XMFLOAT3& Pt0, XMFLOAT3& Pt1, XMFLOAT3& Pt2, XMFLOAT2& Uv0, XMFLOAT2& Uv1, XMFLOAT2& Uv2, const CColor& Color0, const CColor& Color1, const CColor& Color2)
{
	Assert(m_State == eTriangleList);
	Assert(m_TotalVerts + m_NumVerts + 3 < scMaxNumVerices);	
	CVBuffer* pVBuffer = m_rRenderer->GetExecutionModel() == CRenderer::eExecutionModel::eMultithreadedDeferred ? m_pVBuffersDeferred[m_CurrentVertexBuffer] : m_pVBuffers[m_CurrentVertexBuffer];
	SPosTexColor* pVertexData = (SPosTexColor*)pVBuffer->GetResourceData();
	Assert(pVertexData);
	pVertexData[m_VertexOffset].m_Pos = Pt0;
	pVertexData[m_VertexOffset].m_Tex = Uv0;
	pVertexData[m_VertexOffset].m_Col = Color0.m_Color.m_Argb;
	m_VertexOffset++;
	m_NumVerts++;	
	pVertexData[m_VertexOffset].m_Pos = Pt1;
	pVertexData[m_VertexOffset].m_Tex = Uv1;	
	pVertexData[m_VertexOffset].m_Col = Color1.m_Color.m_Argb;
	m_VertexOffset++;
	m_NumVerts++;	
	pVertexData[m_VertexOffset].m_Pos = Pt2;
	pVertexData[m_VertexOffset].m_Tex = Uv2;	
	pVertexData[m_VertexOffset].m_Col = Color2.m_Color.m_Argb;
	m_VertexOffset++;	
	m_NumVerts++;	
}

//------------------------------------------------------------------------------------
void CUtilityDraw::EndTriangleList(XMMATRIX& LocalToWorld, const CMaterial* pMaterial, CRenderer::eDepthStates DepthState, CRenderer::eBlendStates BlendState)
{
	Assert(m_State == eTriangleList);
	void* pData;
	m_pDynamicRWBuffer->BeginWrite(&pData, sizeof(CUtilityDrawCommandTriangleList));
	new(pData)CUtilityDrawCommandTriangleList(LocalToWorld, m_StartVertexOffset, m_NumVerts, pMaterial, *m_rRenderer, *m_rConstantsSystem, DepthState, BlendState);
	m_pDynamicRWBuffer->EndWrite(sizeof(CUtilityDrawCommandTriangleList));
	m_State = eNone;
}

//------------------------------------------------------------------------------------
void CUtilityDraw::DrawCube(XMMATRIX& LocalToWorld, const CMaterial* pMaterial)
{
	void* pData;
	m_pStaticRWBuffer->BeginWrite(&pData, sizeof(CUtilityDrawCube));
	new(pData)CUtilityDrawCube(LocalToWorld, pMaterial, this, *m_rConstantsSystem, *m_rRenderer);
	m_pStaticRWBuffer->EndWrite(sizeof(CUtilityDrawCube));
}

//------------------------------------------------------------------------------------
void CUtilityDraw::DrawSphere(XMMATRIX& LocalToWorld, const CMaterial* pMaterial)
{
	if (!pMaterial->IsLoaded())
		return;
	XMMATRIX* pData;
	m_rConstantsSystem->UpdateConstantBufferBegin(CConstantsSystem::eLocalToWorld, (void**)&pData, sizeof(XMMATRIX));
	*pData = LocalToWorld;
	m_rConstantsSystem->UpdateConstantBufferEnd();
	m_rConstantsSystem->SetConstantBuffer(CConstantsSystem::eLocalToWorld);
	m_pSphereDrawPrimitive->SetMaterial(pMaterial);
	m_rRenderer->MakeCopyAndDrawPrimitive(m_pSphereDrawPrimitive);
}

//------------------------------------------------------------------------------------
const CMaterial* CUtilityDraw::GetPlainColorMaterial()
{
	return m_pMaterialPlainColor;
}

// This comes in from the job
//------------------------------------------------------------------------------------
void CUtilityDraw::Render(ID3D11DeviceContext* pDeviceContext, ID3D11CommandList** ppCommandList, unsigned int VertexBuffer)
{
	bool bEndFrame = false;		
	const CMaterial* pCurrMaterial = NULL;
	CRWBuffer* pDynamicRWBuffer = m_DynamicSegmentedBuffer.GetRead();
	bool MultithreadedDeferred = (m_rRenderer->GetExecutionModel() == CRenderer::eExecutionModel::eMultithreadedDeferred);
	if(MultithreadedDeferred)
	{
		m_rRenderer->InitDevice(pDeviceContext);
		m_rConstantsSystem->SetConstantBuffer(CConstantsSystem::eView, pDeviceContext);
	}
	else
	{
		pDeviceContext = m_rRenderer->GetDeviceContext();
	}
	// Uses global dynamic vertex buffer
	if (pDynamicRWBuffer)
	{
		if(MultithreadedDeferred)
		{
			m_pVBuffersDeferred[VertexBuffer]->Set(0, pDeviceContext);
		}
		else
		{
			m_pVBuffers[VertexBuffer]->Set(0, pDeviceContext);
		}
		while(!bEndFrame)
		{
			IUtilityDrawCommand* pCommand;
			pDynamicRWBuffer->BeginRead((void**)&pCommand, sizeof(IUtilityDrawCommand));
			pCommand->Execute(pDeviceContext, bEndFrame, &pCurrMaterial);
			pDynamicRWBuffer->EndRead(pCommand->Size());
		}
		pDynamicRWBuffer->SwapModes();
		m_DynamicSegmentedBuffer.PutWrite(pDynamicRWBuffer);
	}
	// Draw all static geometry, each call sets its own vertex buffer
	CRWBuffer* pStaticRWBuffer = m_StaticSegmentedBuffer.GetRead();
	if (pStaticRWBuffer)
	{
		bEndFrame = false;	
		while(!bEndFrame)
		{
			IUtilityDrawCommand* pCommand;
			pStaticRWBuffer->BeginRead((void**)&pCommand, sizeof(IUtilityDrawCommand));
			pCommand->Execute(pDeviceContext, bEndFrame, &pCurrMaterial);
			pStaticRWBuffer->EndRead(pCommand->Size());
		}
		pStaticRWBuffer->SwapModes();
		m_StaticSegmentedBuffer.PutWrite(pStaticRWBuffer);
	}	
	if(MultithreadedDeferred)
	{
		pDeviceContext->FinishCommandList(FALSE, ppCommandList);
	}
}

//------------------------------------------------------------------------------------
void CUtilityDraw::Tick()
{
	unsigned int VertexBufferIndex = 0xFFFFFFFF;		
	bool IsNoData = true;
	// Any dynamic geometry to draw?
	ID3D11DeviceContext* pDeviceContext = NULL;
	if(!m_pDynamicRWBuffer->IsEmpty())
	{
		// Add the end draw command
		void* pData;
		m_pDynamicRWBuffer->BeginWrite(&pData, sizeof(CUtilityDrawEndFrame));
		new(pData) CUtilityDrawEndFrame();	
		m_pDynamicRWBuffer->EndWrite(sizeof(CUtilityDrawEndFrame));
		m_pDynamicRWBuffer->SwapModes();		
		m_DynamicSegmentedBuffer.PutRead(m_pDynamicRWBuffer);
		m_pDynamicRWBuffer = m_DynamicSegmentedBuffer.GetWrite();

		// deferred context
		bool MultithreadedDeferred = (m_rRenderer->GetExecutionModel() == CRenderer::eExecutionModel::eMultithreadedDeferred);
		if(MultithreadedDeferred)
		{
			m_pVBuffersDeferred[m_CurrentVertexBuffer]->UnMapResourceData(m_pDeviceContext[m_CurrentVertexBuffer]);						// Unmap the current buffer so that GPU can render from it
			m_pVBuffersDeferred[(m_CurrentVertexBuffer + 2) % 3]->MapResourceData(m_pDeviceContext[(m_CurrentVertexBuffer + 2) % 3]);	// Map the next buffer so that we can update it. Triple buffer, because GPU might still be using m_CurrentVertexBuffer + 1		
			pDeviceContext = m_pDeviceContext[m_CurrentVertexBuffer];
		}
		m_rRenderer->UnMapBuffer(m_pVBuffers[m_CurrentVertexBuffer]);						// Unmap the current buffer so that GPU can render from it
		m_rRenderer->MapBuffer(m_pVBuffers[(m_CurrentVertexBuffer + 2) % 3]);				// Map the next buffer so that we can update it. Triple buffer, because GPU might still be using m_CurrentVertexBuffer + 1		

		VertexBufferIndex = m_CurrentVertexBuffer;
		m_CurrentVertexBuffer = (m_CurrentVertexBuffer + 1) % 3;
		m_TotalVerts = 0;
		m_VertexOffset = 0;		
		IsNoData = false;
	}
	
	// Static geometry
	if (!m_pStaticRWBuffer->IsEmpty())
	{
		// Add the end draw command
		void* pData;
		m_pStaticRWBuffer->BeginWrite(&pData, sizeof(CUtilityDrawEndFrame));
		new(pData) CUtilityDrawEndFrame();	
		m_pStaticRWBuffer->EndWrite(sizeof(CUtilityDrawEndFrame));
		m_pStaticRWBuffer->SwapModes();
		m_StaticSegmentedBuffer.PutRead(m_pStaticRWBuffer);
		m_pStaticRWBuffer = m_StaticSegmentedBuffer.GetWrite();
		IsNoData = false;
	}

	if (!IsNoData) 
	{
		m_rRenderer->UtilityDrawRenderer(this, pDeviceContext, VertexBufferIndex);	// Queue up rendering with the render thread	
	}
}