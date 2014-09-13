#ifndef _JOBUTILITYDRAW_H_
#define _JOBUTILITYDRAW_H_

#include "JobSystem.h"

class  CUtilityDraw;
struct ID3D11CommandList;

// Processes a buffered list of render commands on deferred context and converts them to directx command list

class CJobUtilityDraw : public CJobSystem::CJob
{
	public:
		CJobUtilityDraw(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		CJobUtilityDraw(CUtilityDraw* pUtilityDraw,
						ID3D11DeviceContext* pDeviceContext,
						ID3D11CommandList** ppCommandList,
						unsigned int VertexBuffer,
						CEventArray* pEventArray,
						unsigned int EventIndex);

		virtual ~CJobUtilityDraw();
		virtual unsigned int Execute(unsigned int ThreadID);

	private:
		CUtilityDraw*		 m_pUtilityDraw;
		ID3D11DeviceContext* m_pDeviceContext;
		ID3D11CommandList**	 m_ppCommandList;
		unsigned int		 m_VertexBuffer;
		CEventArray*		 m_pEventArray;
		unsigned int		 m_EventIndex;
};

#endif

