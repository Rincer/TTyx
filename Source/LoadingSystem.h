#ifndef _LOADINGSYSTEM_H_
#define _LOADINGSYSTEM_H_

#include "MultiStageAssetContainer.h"
#include "MemoryManager.h"
#include "LWMutex.h"
#include "JobSystem.h"
#include "Reference.h"
#include "RenderObject.h"

class CTextureSystem;
class CMaterialSystem;
class CAnimationSystem;
class CStringDictionary;
class CGeometrySystem;
class CDrawPrimitiveSystem;
class CSkeletonSystem;
class CRenderObjectSystem;
class CJobSystem;



class CLoadingSystem
{
	public:
	
		class CBlockingCondition : public CJobSystem::IBlockingCondition
		{
			public:
				CBlockingCondition(void** ppData, unsigned int Size, CJobSystem::CJob* pJob, CLoadingSystem* pLoadingSystem) :	m_ppData(ppData),
																																m_Size(Size),
																																m_pJob(pJob),
																																m_pLoadingSystem(pLoadingSystem)
				{
				}
				
				virtual bool IsBlocked()
				{
					*m_ppData = m_pLoadingSystem->Alloc(m_Size);
					return (*m_ppData == NULL);
				}	
				
			private:
				void** m_ppData;
				unsigned int m_Size;
				CJobSystem::CJob* m_pJob;
				CLoadingSystem* m_pLoadingSystem;
		};
	
		// Loading system needs to know about a lot of other systems!!
		CLoadingSystem(	CTextureSystem**		ppTextureSystem,
						CMaterialSystem**		ppMaterialSystem,
						CAnimationSystem**		ppAnimationSystem,
						CStringDictionary**		ppStringDictionary,
						CGeometrySystem**		ppGeometrySystem,
						CDrawPrimitiveSystem**	ppDrawPrimitiveSystem,
						CSkeletonSystem**		ppSkeletonSystem,
						CRenderObjectSystem**	ppRenderObjectSystem,
						CJobSystem**			ppJobSystem);
		~CLoadingSystem();
								
		void LoadFromFile(const char* pFileName, CRenderObjectSystem::CRenderObjectCreatorBase* pCreator);
		bool IsShutdown();
		void* Alloc(unsigned int Size);
		void Free(const void* pMemory, unsigned int Size);
		
	private:	
		CMemoryCapper m_FileReadMemory;
		CReference<CTextureSystem*>			m_rTextureSystem;
		CReference<CMaterialSystem*>		m_rMaterialSystem;
		CReference<CAnimationSystem*>		m_rAnimationSystem;
		CReference<CStringDictionary*>		m_rStringDictionary;
		CReference<CGeometrySystem*>		m_rGeometrySystem;
		CReference<CDrawPrimitiveSystem*>	m_rDrawPrimitiveSystem;
		CReference<CSkeletonSystem*>		m_rSkeletonSystem;
		CReference<CRenderObjectSystem*>	m_rRenderObjectSystem;
		CReference<CJobSystem*>				m_rJobSystem;
};

#endif