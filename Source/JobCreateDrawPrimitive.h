#ifndef _JOBCREATEDRAWPRIMITIVE_H_
#define _JOBCREATEDRAWPRIMITIVE_H_

#include "RenderObject.h"

class CIntermediateDrawPrimData;
class CMaterial;
class CLoadingSystem;
class CGeometrySystem;
class CDrawPrimitiveSystem;
class CRenderObjectSystem;
class CJobSystem;
class CStringDictionary;

class CJobCreateDrawPrimitive : public CJobSystem::CJob
{
	public:
		CJobCreateDrawPrimitive(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		CJobCreateDrawPrimitive(CIntermediateDrawPrimData* pIntermediateDrawPrimData, 
								const char* pPrimDesc,
								const CMaterial* pMaterial,
								CLoadingSystem* pLoadingSystem,
								CGeometrySystem* pGeometrySystem,
								CDrawPrimitiveSystem* pDrawPrimitiveSystem,
								CRenderObjectSystem* pRenderObjectSystem,
								CJobSystem* pJobSystem,
								CStringDictionary* pStringDictionary,
								CRenderObjectSystem::CRenderObjectCreatorBase* pCreator);
								
		virtual ~CJobCreateDrawPrimitive();
		virtual unsigned int Execute(unsigned int ThreadID); 
		void SignalResourceCreation();

	private:
		CIntermediateDrawPrimData* m_pIntermediateDrawPrimData;
		char m_PrimDesc[256];
		const CMaterial* m_pMaterial;
		CLoadingSystem* m_pLoadingSystem;
		CGeometrySystem* m_pGeometrySystem;
		CDrawPrimitiveSystem* m_pDrawPrimitiveSystem;
		CRenderObjectSystem* m_pRenderObjectSystem;
		CJobSystem* m_pJobSystem;
		CStringDictionary* m_pStringDictionary;
		CRenderObjectSystem::CRenderObjectCreatorBase* m_pCreator;
};


#endif