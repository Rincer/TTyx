#include "stdafx.h"
#include "JobSystem.h"
#include "FileSystem.h"
#include "LoadingSystem.h"
#include "ObjFileParser.h"
#include "Macros.h"

#include "JobLoadObjFile.h"

static const char* scObjPath = "Obj\\";	

//---------------------------------------------------------------------------------------------
CJobLoadObjFile::CJobLoadObjFile(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//---------------------------------------------------------------------------------------------
CJobLoadObjFile::~CJobLoadObjFile()
{
}

//---------------------------------------------------------------------------------------------
CJobLoadObjFile::CJobLoadObjFile(const char* pName,
								CMaterialSystem*		pMaterialSystem,								
								CGeometrySystem*		pGeometrySystem,
								CDrawPrimitiveSystem*	pDrawPrimitiveSystem,
								CRenderObjectSystem*	pRenderObjectSystem,
								CStringDictionary*		pStringDictionary,
								CTextureSystem*			pTextureSystem,
								CRenderObjectSystem::CRenderObjectCreatorBase* pCreator,
								CLoadingSystem*			pLoadingSystem) : CJobSystem::CJob(CJobSystem::eJobLoadObjFile),
																		  m_pMaterialSystem(pMaterialSystem),																		 
																		  m_pGeometrySystem(pGeometrySystem),
																		  m_pDrawPrimitiveSystem(pDrawPrimitiveSystem),
																		  m_pRenderObjectSystem(pRenderObjectSystem),
																		  m_pStringDictionary(pStringDictionary),
																		  m_pTextureSystem(pTextureSystem),
																		  m_pCreator(pCreator),
																		  m_pLoadingSystem(pLoadingSystem)
{
	strcpy_s(m_Name, 255, pName);
}

//---------------------------------------------------------------------------------------------
unsigned int CJobLoadObjFile::Execute(unsigned int ThreadID)
{
	char FullPathName[256];	
	strcpy_s(FullPathName, 255, scObjPath);	
	strcat_s(FullPathName, 255, m_Name); 
	HANDLE File = CFileSystem::Open(FullPathName);
	unsigned int FileSize = GetFileSize(File, NULL); 
	unsigned int Size = FileSize + sizeof(CObjFileParser);
	unsigned char* pData;
	CLoadingSystem::CBlockingCondition BlockingCondition((void**)&pData, Size, this, m_pLoadingSystem);
	if(BlockingCondition.IsBlocked())
	{
		m_pJobSystem->YieldExecution(m_Id, m_Type, ThreadID, &BlockingCondition);
	}	
	m_pObjFileParser = new (pData) CObjFileParser(m_pJobSystem, m_pMaterialSystem, m_pLoadingSystem, m_pGeometrySystem, m_pDrawPrimitiveSystem, m_pRenderObjectSystem, m_pStringDictionary, m_pTextureSystem, m_pCreator);	
	CFileSystem::Read(&pData[sizeof(CObjFileParser)], FileSize, File);
	CFileSystem::Close(File);
		
	m_pObjFileParser->ParseObjFileFromMemory(&pData[sizeof(CObjFileParser)], FileSize, m_Name, this, ThreadID);	
	m_pLoadingSystem->Free(pData, Size);
	return 0;
}
