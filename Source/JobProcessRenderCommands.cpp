#include "stdafx.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "RWBuffer.h"
#include "Event.h"
#include "ConstantsSystem.h"

#include "JobProcessRenderCommands.h"

//---------------------------------------------------------------------------------------------
CJobProcessRenderCommands::CJobProcessRenderCommands(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//---------------------------------------------------------------------------------------------
CJobProcessRenderCommands::~CJobProcessRenderCommands()
{
}

//---------------------------------------------------------------------------------------------
CJobProcessRenderCommands::CJobProcessRenderCommands(	CRenderer* pRenderer, 
														CRWBuffer* pRWBuffer, 
														ID3D11CommandList**	ppCommandList, 
														unsigned int NumCommands,
														CEventArray* pEventArray,
														unsigned int EventIndex,
														CConstantsSystem* pConstantsSystem) : CJobSystem::CJob(CJobSystem::eJobProcessRenderCommands),
																					m_pRenderer(pRenderer),
																					m_pRWBuffer(pRWBuffer),
																					m_ppCommandList(ppCommandList),
																					m_NumCommands(NumCommands),
																					m_pEventArray(pEventArray),
																					m_EventIndex(EventIndex),
																					m_pConstantsSystem(pConstantsSystem)

{

}

//---------------------------------------------------------------------------------------------
unsigned int CJobProcessRenderCommands::Execute(unsigned int ThreadID)
{	
	ID3D11DeviceContext* pDeviceContext = m_pRenderer->GetDeferredContext(ThreadID);
	bool EndFrame;
	m_pRenderer->InitDevice(pDeviceContext);
	m_pConstantsSystem->SetConstantBuffer(CConstantsSystem::eView, pDeviceContext);
	m_pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLights, pDeviceContext);
	for (unsigned int CommandIndex = 0; CommandIndex < m_NumCommands; CommandIndex++)
	{
		IRenderCommand* pRenderCommand;
		m_pRWBuffer->BeginRead((void**)&pRenderCommand, sizeof(IRenderCommand));		// this makes an assumption that the compiler stores the 																					// virtual function table pointer as the 1st piece of data in a class
		pRenderCommand->Execute(pDeviceContext, EndFrame);
		m_pRWBuffer->EndRead(pRenderCommand->Size());
	}
	pDeviceContext->FinishCommandList(FALSE, m_ppCommandList);
	m_pRWBuffer->SwapModes(); // Finished processing this buffer
	m_pRenderer->ReturnCommandBufferSegment(m_pRWBuffer);
	m_pEventArray->Set(m_EventIndex); // Signal completion of processing of this CL
	return 0;
}