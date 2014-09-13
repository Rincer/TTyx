#include "stdafx.h"
#include "UtilityDraw.h"
#include "JobSystem.h"

#include "JobUtilityDraw.h"


CJobUtilityDraw::CJobUtilityDraw(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}


CJobUtilityDraw::CJobUtilityDraw(CUtilityDraw* pUtilityDraw,
								 ID3D11DeviceContext* pDeviceContext,
								 ID3D11CommandList** ppCommandList,
								 unsigned int VertexBuffer,
								 CEventArray* pEventArray,
								 unsigned int EventIndex) :	CJobSystem::CJob(CJobSystem::eJobUtilityDraw),
																		m_pUtilityDraw(pUtilityDraw),
																		m_pDeviceContext(pDeviceContext),
																		m_ppCommandList(ppCommandList),
																		m_VertexBuffer(VertexBuffer),
																		m_pEventArray(pEventArray),
																		m_EventIndex(EventIndex)


{
}

CJobUtilityDraw::~CJobUtilityDraw()
{
}
 
unsigned int CJobUtilityDraw::Execute(unsigned int ThreadID)
{
	// C4100
	ThreadID;
	m_pUtilityDraw->Render(m_pDeviceContext, m_ppCommandList, m_VertexBuffer);
	m_pEventArray->Set(m_EventIndex); // Signal completion of processing of this CL
	return 0;
}

