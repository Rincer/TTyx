#include "stdafx.h"
#include "JobSystem.h"
#include "LoadingSystem.h"
#include "DrawData.h"
#include "VertexProcessing.h"
#include "GeometrySystem.h"
#include "StringDictionary.h"
#include "RenderObject.h"

#include "JobCreateDrawPrimitive.h"

//---------------------------------------------------------------------------------------------
CJobCreateDrawPrimitive::CJobCreateDrawPrimitive(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//---------------------------------------------------------------------------------------------
CJobCreateDrawPrimitive::CJobCreateDrawPrimitive(CIntermediateDrawPrimData* pIntermediateDrawPrimData, 
												 const char* pPrimDesc,
												 const CMaterial* pMaterial,
												 CLoadingSystem* pLoadingSystem,
												 CGeometrySystem* pGeometrySystem,
												 CDrawPrimitiveSystem* pDrawPrimitiveSystem,
												 CRenderObjectSystem* pRenderObjectSystem,
												 CJobSystem* pJobSystem,
												 CStringDictionary* pStringDictionary,
												 CRenderObjectSystem::CRenderObjectCreatorBase* pCreator) : CJobSystem::CJob(CJobSystem::eJobCreateDrawPrimitive),
																			m_pIntermediateDrawPrimData(pIntermediateDrawPrimData),
																			m_pMaterial(pMaterial),
																			m_pLoadingSystem(pLoadingSystem),
																			m_pGeometrySystem(pGeometrySystem),
																			m_pDrawPrimitiveSystem(pDrawPrimitiveSystem),
																			m_pRenderObjectSystem(pRenderObjectSystem),
																			m_pJobSystem(pJobSystem),
																			m_pStringDictionary(pStringDictionary),
																			m_pCreator(pCreator)

{
	strcpy_s(m_PrimDesc, 255, pPrimDesc);
} 


//---------------------------------------------------------------------------------------------
CJobCreateDrawPrimitive::~CJobCreateDrawPrimitive()
{
	m_pLoadingSystem->Free(m_pIntermediateDrawPrimData, sizeof(CIntermediateDrawPrimData));
}

//---------------------------------------------------------------------------------------------
unsigned int CJobCreateDrawPrimitive::Execute(unsigned int ThreadID)
{
	ThreadID;
	m_pIntermediateDrawPrimData->CalculateSmoothingGroups();
	SVertexObj* pVertices; 
	unsigned short VertexCount; 
	const unsigned short* pIndices; 
	unsigned short IndexCount;
	char Name[256];	
	m_pIntermediateDrawPrimData->GetVertexAndIndexData(&pVertices, VertexCount, &pIndices, IndexCount);
	CVertexProcessing::Instance().CreateTangents(pVertices, VertexCount, pIndices, IndexCount);
	sprintf_s(Name,"%s.VBuffer", m_PrimDesc);
	CVBuffer& VBuffer = m_pGeometrySystem->CreateVBuffer(Name, sizeof(SVertexObj), pVertices, VertexCount * sizeof(SVertexObj));
	sprintf_s(Name,"%s.IBuffer", m_PrimDesc);	
	CIBuffer& IBuffer = m_pGeometrySystem->CreateIBuffer(Name, pIndices, IndexCount * sizeof(unsigned short));
	CVBuffer* pVBuffer[CDrawPrimitive::scMaxStreams] = { &VBuffer, NULL };	
	sprintf_s(Name,"%s.DrawPrim", m_PrimDesc);		
	CDrawPrimitive* pDrawPrimitive = &m_pDrawPrimitiveSystem->Add(Name, pVBuffer, 1, &IBuffer, IndexCount, m_pMaterial);
	m_pCreator->CreateRenderObject(m_PrimDesc, NULL, pDrawPrimitive, NULL, m_pRenderObjectSystem);
	return 0;
}

//---------------------------------------------------------------------------------------------
void CJobCreateDrawPrimitive::SignalResourceCreation()
{
	if(m_pIntermediateDrawPrimData->DecrementInUseCount() == 0)
	{
		m_pJobSystem->ReleaseJob(this);
	}
}