#ifndef _JOBLOADOBJFILE_H_
#define _JOBLOADOBJFILE_H_


class CObjFileParser;
class CMaterialSystem;
class CLoadingSystem;
class CGeometrySystem;
class CDrawPrimitiveSystem;
class CRenderObjectSystem;
class CStringDictionary;
class CTextureSystem;
class CRenderObjectSystem;
class CRenderObjectCreatorBase;

class CJobLoadObjFile : public CJobSystem::CJob
{
	public:
		CJobLoadObjFile(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		~CJobLoadObjFile();		
		CJobLoadObjFile(const char* pName, 
						CMaterialSystem*		pMaterialSystem,						
						CGeometrySystem*		pGeometrySystem,
						CDrawPrimitiveSystem*	pDrawPrimitiveSystem,
						CRenderObjectSystem*	pRenderObjectSystem,
						CStringDictionary*		pStringDictionary,
						CTextureSystem*			pTextureSystem,
						CRenderObjectSystem::CRenderObjectCreatorBase* pCreator,
						CLoadingSystem*			pLoadingSystem);
		virtual unsigned int Execute(unsigned int ThreadID); 

	private:
		char m_Name[256];
		CObjFileParser*  m_pObjFileParser;
		CMaterialSystem*		m_pMaterialSystem;
		CGeometrySystem*		m_pGeometrySystem;
		CDrawPrimitiveSystem*	m_pDrawPrimitiveSystem;
		CRenderObjectSystem*	m_pRenderObjectSystem;
		CStringDictionary*		m_pStringDictionary;
		CTextureSystem*			m_pTextureSystem;
		CRenderObjectSystem::CRenderObjectCreatorBase* m_pCreator;
		CLoadingSystem*			m_pLoadingSystem;
};

#endif