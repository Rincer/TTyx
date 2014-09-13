#include "stdafx.h"
#include "JobSystem.h"
#include "FileSystem.h"
#include "LoadingSystem.h"
#include "XFileParser.h"
#include "Macros.h"

#include "JobLoadXFile.h"

static const unsigned int scXFileParserScratchMemory = 128 * 1024;
static const char* scXPath = "X\\";	

//---------------------------------------------------------------------------------------------
CJobLoadXFile::CJobLoadXFile(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//---------------------------------------------------------------------------------------------
CJobLoadXFile::~CJobLoadXFile()
{
	m_pLoadingSystem->Free(m_pData, m_Size);
}

//---------------------------------------------------------------------------------------------
CJobLoadXFile::CJobLoadXFile(const char* pName,
							 CTextureSystem*		pTextureSystem,
							 CMaterialSystem*		pMaterialSystem,
							 CAnimationSystem*		pAnimationSystem,
						     CStringDictionary*		pStringDictionary,
							 CGeometrySystem*		pGeometrySystem,
							 CDrawPrimitiveSystem*	pDrawPrimitiveSystem,
							 CSkeletonSystem*		pSkeletonSystem,
							 CRenderObjectSystem*	pRenderObjectSystem,
							 CRenderObjectSystem::CRenderObjectCreatorBase* pCreator,
							 CLoadingSystem*		pLoadingSystem) : CJobSystem::CJob(CJobSystem::eJobLoadXFile),
																		   m_pTextureSystem(pTextureSystem),
																		   m_pMaterialSystem(pMaterialSystem),
																		   m_pAnimationSystem(pAnimationSystem),
																		   m_pStringDictionary(pStringDictionary),
																		   m_pGeometrySystem(pGeometrySystem),
																		   m_pDrawPrimitiveSystem(pDrawPrimitiveSystem),
																		   m_pSkeletonSystem(pSkeletonSystem),
																		   m_pRenderObjectSystem(pRenderObjectSystem),
																		   m_pCreator(pCreator),
																		   m_pLoadingSystem(pLoadingSystem)
{
	strcpy_s(m_Name, 255, pName);
}

//---------------------------------------------------------------------------------------------
unsigned int CJobLoadXFile::Execute(unsigned int ThreadID)
{
	char FullPathName[256];	
	strcpy_s(FullPathName, 255, scXPath);	
	strcat_s(FullPathName, 255, m_Name); 
	HANDLE File = CFileSystem::Open(FullPathName);
	unsigned int FileSize = GetFileSize(File, NULL); 
	
	m_Size = FileSize + 16 + sizeof(CXFileParser) + CXFileParser::scScratchMemory; 	// CXFileParser needs to be 16 byte aligned, because it contains some vector types
	CLoadingSystem::CBlockingCondition BlockingCondition((void**)&m_pData, m_Size, this, m_pLoadingSystem);	
	if(BlockingCondition.IsBlocked())
	{
		m_pJobSystem->YieldExecution(m_Id, m_Type, ThreadID, &BlockingCondition);
	}
	CFileSystem::Read(m_pData, FileSize, File);
	CFileSystem::Close(File);
	unsigned char* pXFileParserMemory = (unsigned char*)(POW2ALIGN((unsigned long long)&m_pData[FileSize], 16)); // make sure CXFileParser is 16 byte aligned 
	CXFileParser* pXFileParser = new (pXFileParserMemory)CXFileParser(	m_pTextureSystem,
																		m_pMaterialSystem,
																		m_pAnimationSystem,
																		m_pStringDictionary,
																		m_pGeometrySystem,
																		m_pDrawPrimitiveSystem,
																		m_pSkeletonSystem,
																		m_pRenderObjectSystem,
																		m_pCreator);
	pXFileParser->ParseXFileFromMemory(m_pData, FileSize, m_Name, this, ThreadID);	
	return 0;
}

