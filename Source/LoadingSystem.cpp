#include "stdafx.h"
#include "HeapAllocator.h"
#include "Hash64.h"
#include "HashMap.h"
#include "StringDictionary.h"
#include "JobSystem.h"
#include "JobLoadXFile.h"
#include "JobLoadObjFile.h"

#include "LoadingSystem.h"

static const unsigned int scLoadingSystemMemory = 2 * 1024 * 1024;
static const unsigned int scTotalFileIOMemory = 20 * 1024 * 1024;


//--------------------------------------------------------------------------------
CLoadingSystem::CLoadingSystem(	CTextureSystem**		ppTextureSystem,
								CMaterialSystem**		ppMaterialSystem,
								CAnimationSystem**		ppAnimationSystem,
								CStringDictionary**		ppStringDictionary,
								CGeometrySystem**		ppGeometrySystem,
								CDrawPrimitiveSystem**	ppDrawPrimitiveSystem,
								CSkeletonSystem**		ppSkeletonSystem,
								CRenderObjectSystem**	ppRenderObjectSystem,
								CJobSystem**			ppJobSystem) :  m_FileReadMemory(100 * 1024 * 1024), // 100 megs!
																		m_rTextureSystem(ppTextureSystem),
																		m_rMaterialSystem(ppMaterialSystem),
																		m_rAnimationSystem(ppAnimationSystem),
																		m_rStringDictionary(ppStringDictionary),
																		m_rGeometrySystem(ppGeometrySystem),
																		m_rDrawPrimitiveSystem(ppDrawPrimitiveSystem),
																		m_rSkeletonSystem(ppSkeletonSystem),
																		m_rRenderObjectSystem(ppRenderObjectSystem),
																		m_rJobSystem(ppJobSystem)
{

}

//--------------------------------------------------------------------------------
CLoadingSystem::~CLoadingSystem()
{
}

//--------------------------------------------------------------------------------
void* CLoadingSystem::Alloc(unsigned int Size)
{
	return m_FileReadMemory.Alloc(Size);
}

//--------------------------------------------------------------------------------
void CLoadingSystem::Free(const void* pMemory, unsigned int Size)
{
	m_FileReadMemory.Free(pMemory, Size);
}

//--------------------------------------------------------------------------------
void CLoadingSystem::LoadFromFile(const char* pFileName, CRenderObjectSystem::CRenderObjectCreatorBase* pCreator)
{
	const char* pExt = strchr(pFileName, '.');
	if(pExt[1] == 'x')
	{
		CJobLoadXFile* pJobLoadXFile = m_rJobSystem->AcquireJob<CJobLoadXFile>(CJobSystem::ePriority0);
		new (pJobLoadXFile) CJobLoadXFile(	pFileName,
											*m_rTextureSystem,
											*m_rMaterialSystem,
											*m_rAnimationSystem,
											*m_rStringDictionary,
											*m_rGeometrySystem,
											*m_rDrawPrimitiveSystem,
											*m_rSkeletonSystem,
											*m_rRenderObjectSystem,
											pCreator,
											this);
		m_rJobSystem->AddJob(pJobLoadXFile);
		return;
	
	} else if ((pExt[1] == 'o') && (pExt[2] == 'b') && (pExt[3] == 'j'))
	{
		CJobLoadObjFile* pJobLoadObjFile = m_rJobSystem->AcquireJob<CJobLoadObjFile>(CJobSystem::ePriority0);
		new (pJobLoadObjFile) CJobLoadObjFile(pFileName, 
											  *m_rMaterialSystem, 
											  *m_rGeometrySystem, 
											  *m_rDrawPrimitiveSystem , 
											  *m_rRenderObjectSystem, 
											  *m_rStringDictionary, 
											  *m_rTextureSystem,
											  pCreator,
											  this);
		m_rJobSystem->AddJob(pJobLoadObjFile);
		return;	
	}		
}


//--------------------------------------------------------------------------------
bool CLoadingSystem::IsShutdown()
{
	return true;
}

