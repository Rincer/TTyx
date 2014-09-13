#ifndef _OBJFILEPARSER_H_
#define _OBJFILEPARSER_H_

#include "DrawData.h"
#include "JobSystem.h"

class CMaterialSystem;
class CLoadingSystem;
class CGeometrySystem;
class CDrawPrimitiveSystem;
class CRenderObjectSystem;
class CStringDictionary;
class CTextureSystem;

class CObjFileParser
{
	public:		
		enum eParseState
		{
			eNone,
			eLibrary,
			eVertexData,
			eTriangles
		};
		
	
		CObjFileParser(	CJobSystem*				pJobSystem, 
						CMaterialSystem*		pMaterialSystem, 
						CLoadingSystem*			pLoadingSystem, 
						CGeometrySystem*		pGeometrySystem,
						CDrawPrimitiveSystem*	pDrawPrimitiveSystem,
						CRenderObjectSystem*	pRenderObjectSystem,
						CStringDictionary*		pStringDictionary,
						CTextureSystem*			pTextureSystem,
						CRenderObjectSystem::CRenderObjectCreatorBase* pCreator);
		~CObjFileParser();
		void ParseObjFileFromMemory(unsigned char* pMemory, unsigned int Size, const char* pName, CJobSystem::CJob* pJob, unsigned int ThreadID); 

		void ReadSmoothingGroup(int& i, const char** ppData);
		
		//-----------------------------------------------------------------------------
		// returns 1 or 2 depending if this was a tri or a quad, offsets relative to this group
		unsigned int ReadTriOrQuad(CIntermediateDrawPrimData::STriangle *pTriangles, const char** ppData, unsigned int PosOffset, unsigned int NrmPosOffset, unsigned int UvPosOffset);				
		bool CreateDrawPrimitive();
	private:			
		void AllocateIntermediateData();
		volatile eParseState	m_State;	
		const char*				m_pName;	
		unsigned int			m_ThreadID;
		char					m_GroupName[256];	
																
		int				m_SmoothingGroup;		
		unsigned short	m_TriangleCount;	
		unsigned int    m_PrimitivesCount;		
		
		CRawVertexData			   m_RawVertexData;	
		CIntermediateDrawPrimData* m_pIntermediateDrawPrimData;
		CJobSystem::CJob*		   m_pJob;
				
		char			m_MtllibName[256];						
		char			m_MtlName[256];				

		CJobSystem*				m_pJobSystem;
		CMaterialSystem*		m_pMaterialSystem;
		CLoadingSystem*			m_pLoadingSystem;
		CGeometrySystem*		m_pGeometrySystem;
		CDrawPrimitiveSystem*	m_pDrawPrimitiveSystem;
		CRenderObjectSystem*	m_pRenderObjectSystem;
		CStringDictionary*		m_pStringDictionary;
		CTextureSystem*			m_pTextureSystem;
		CRenderObjectSystem::CRenderObjectCreatorBase* m_pCreator;
};


#endif