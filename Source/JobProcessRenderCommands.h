#ifndef _JOBPROCESSRENDERCOMMANDS_H_
#define _JOBPROCESSRENDERCOMMANDS_H_

#include "JobSystem.h"

class CRWBuffer;
class CRenderer;
class CEventArray;
class CConstantsSystem;
struct ID3D11CommandList;

// Processes a buffered list of render commands on deferred context and converts them to directx command list

class CJobProcessRenderCommands : public CJobSystem::CJob
{
public:
	CJobProcessRenderCommands(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
	CJobProcessRenderCommands(	CRenderer* pRenderer, 
								CRWBuffer* pRWBuffer, 
								ID3D11CommandList** ppCommandList, 
								unsigned int NumCommands, 
								CEventArray* pEventArray, 
								unsigned int EventIndex,
								CConstantsSystem* pConstantsSystem);

	virtual ~CJobProcessRenderCommands();
	virtual unsigned int Execute(unsigned int ThreadID);

private:
	CRenderer*			m_pRenderer;
	CRWBuffer*			m_pRWBuffer;
	ID3D11CommandList**	m_ppCommandList;
	unsigned int		m_NumCommands;
	CEventArray*		m_pEventArray;
	unsigned int		m_EventIndex;
	CConstantsSystem*	m_pConstantsSystem;
};

#endif

