#ifndef _JOBLOADXFILE_H_
#define _JOBLOADXFILE_H_

#include "RenderObject.h"

class CTextureSystem;
class CMaterialSystem;
class CAnimationSystem;
class CStringDictionary;
class CGeometrySystem;
class CDrawPrimitiveSystem;
class CSkeletonSystem;
class CRenderObjectSystem;
class CLoadingSystem;
class CJobSystem;
class CRenderObjectCreatorBase;


class CJobLoadXFile : public CJobSystem::CJob
{
	public:
		CJobLoadXFile(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		~CJobLoadXFile();		
		CJobLoadXFile(	const char* pName,
						CTextureSystem*			pTextureSystem,
						CMaterialSystem*		pMaterialSystem,
						CAnimationSystem*		pAnimationSystem,
						CStringDictionary*		pStringDictionary,
						CGeometrySystem*		pGeometrySystem,
						CDrawPrimitiveSystem*	pDrawPrimitiveSystem,
						CSkeletonSystem*		pSkeletonSystem,
						CRenderObjectSystem*	pRenderObjectSystem,
						CRenderObjectSystem::CRenderObjectCreatorBase* pCreator,
						CLoadingSystem*			pLoadingSystem);
		virtual unsigned int Execute(unsigned int ThreadID); 

	private:
		char											m_Name[256];
		unsigned char*									m_pData;
		unsigned int									m_Size;		
		CTextureSystem*									m_pTextureSystem;
		CMaterialSystem*								m_pMaterialSystem;
		CAnimationSystem*								m_pAnimationSystem;
		CStringDictionary*								m_pStringDictionary;
		CGeometrySystem*								m_pGeometrySystem;
		CDrawPrimitiveSystem*							m_pDrawPrimitiveSystem;
		CSkeletonSystem*								m_pSkeletonSystem;
		CRenderObjectSystem*							m_pRenderObjectSystem;
		CLoadingSystem*									m_pLoadingSystem;
		CRenderObjectSystem::CRenderObjectCreatorBase*	m_pCreator;
};


#endif