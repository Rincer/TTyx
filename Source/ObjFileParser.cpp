#include "stdafx.h"
#include "TextReader.h"
#include "TextureSystem.h"
#include "GeometrySystem.h"
#include "VertexProcessing.h"
#include "RenderObject.h"
#include "Hash64.h"
#include "StringDictionary.h"
#include "LoadingSystem.h"
#include "JobCreateDrawPrimitive.h"
#include "JobLoadMaterialLibrary.h"
#include "JobSystem.h"

#include "ObjFileParser.h"


//-----------------------------------------------------------------------------
CObjFileParser::CObjFileParser(	CJobSystem*				pJobSystem, 
								CMaterialSystem*		pMaterialSystem, 
								CLoadingSystem*			pLoadingSystem,
								CGeometrySystem*		pGeometrySystem,
								CDrawPrimitiveSystem*	pDrawPrimitiveSystem,
								CRenderObjectSystem*	pRenderObjectSystem,
								CStringDictionary*		pStringDictionary,
								CTextureSystem*			pTextureSystem,
								CRenderObjectSystem::CRenderObjectCreatorBase* pCreator) : m_pIntermediateDrawPrimData(NULL),
																			m_pJobSystem(pJobSystem), 
																			m_pMaterialSystem(pMaterialSystem), 
																			m_pLoadingSystem(pLoadingSystem),
																			m_pGeometrySystem(pGeometrySystem),
																			m_pDrawPrimitiveSystem(pDrawPrimitiveSystem),
																			m_pRenderObjectSystem(pRenderObjectSystem),
																			m_pStringDictionary(pStringDictionary),
																			m_pTextureSystem(pTextureSystem),
																			m_pCreator(pCreator),
																			m_PrimitivesCount(0)
{
	m_GroupName[0] = '\0';
}

//-----------------------------------------------------------------------------
CObjFileParser::~CObjFileParser()
{
}

			
//-----------------------------------------------------------------------------
void CObjFileParser::ReadSmoothingGroup(int& i, const char** ppData)
{
	CTextReader::SkipSpacesEOLAndTab(ppData);
	if((*ppData)[0] == 'o')
	{
		(*ppData)++;
		Assert((*ppData)[0] == 'f');
		(*ppData)++;
		Assert((*ppData)[0] == 'f');				
		(*ppData)++;
		i = 0xFFFF;
	}
	else
	{
		CTextReader::ReadInt(i, ppData);
	}
}


//-----------------------------------------------------------------------------
// returns 1 or 2 depending if this was a tri or a quad
unsigned int CObjFileParser::ReadTriOrQuad(CIntermediateDrawPrimData::STriangle *pTriangles, const char** ppData, unsigned int PosOffset, unsigned int NrmOffset, unsigned int UvOffset)
{
	// needs to have at least 3 vertices for the 1st face
	unsigned int TriangleCount = 1;
	int i;
	for(unsigned int VertexIndex = 0; VertexIndex < 3; VertexIndex++)
	{				
		CTextReader::ReadInt(i, ppData);
		pTriangles[0].m_Verts[VertexIndex].m_PosIndex = i - 1 - PosOffset;
		Assert(*ppData[0] == '/');
		(*ppData)++;
		CTextReader::ReadInt(i, ppData);
		pTriangles[0].m_Verts[VertexIndex].m_UvIndex = i - 1 - UvOffset;
		Assert(*ppData[0] == '/');
		(*ppData)++;
		CTextReader::ReadInt(i, ppData);
		pTriangles[0].m_Verts[VertexIndex].m_NrmIndex = i - 1 - NrmOffset;			
		pTriangles[0].m_Verts[VertexIndex].m_SGIndex = m_SmoothingGroup;
	}	
	
	if(CTextReader::ReadInt(i, ppData)) // Successfully read another index so must be a quad
	{
		pTriangles[1].m_Verts[2].m_PosIndex = i - 1 - PosOffset;
		Assert(*ppData[0] == '/');
		(*ppData)++;
		CTextReader::ReadInt(i, ppData);
		pTriangles[1].m_Verts[2].m_UvIndex = i - 1 - UvOffset;
		Assert(*ppData[0] == '/');
		(*ppData)++;
		CTextReader::ReadInt(i, ppData);
		pTriangles[1].m_Verts[2].m_NrmIndex = i - 1 - NrmOffset;		
		pTriangles[1].m_Verts[2].m_SGIndex = m_SmoothingGroup;	
			
		// Complete the 2nd tri	of the quad				
		pTriangles[1].m_Verts[0] = pTriangles[0].m_Verts[0];
		pTriangles[1].m_Verts[1] = pTriangles[0].m_Verts[2];				
		TriangleCount = 2;				
	}				
	//   0----3
	//   | \  |
	//   |   \|
	//   1----2
	// Tri1 is 0/1/2
	// Tri2 is 0/2/3
	return TriangleCount;
}
				
//-----------------------------------------------------------------------------
bool CObjFileParser::CreateDrawPrimitive()
{
	CJobCreateDrawPrimitive* pJobCreateDrawPrimitive = m_pJobSystem->AcquireJob<CJobCreateDrawPrimitive>(CJobSystem::ePriority0);
	char PrimDesc[256];
	sprintf_s(PrimDesc, 255, "%s_%d", m_pName, m_PrimitivesCount);	
	CMaterial* pMaterial = &m_pMaterialSystem->GetMaterialReference(CMaterialSystem::eMaterialPhong, m_MtlName);
	new (pJobCreateDrawPrimitive) CJobCreateDrawPrimitive(	m_pIntermediateDrawPrimData, 
															PrimDesc, 
															pMaterial, 
															m_pLoadingSystem, 
															m_pGeometrySystem, 
															m_pDrawPrimitiveSystem, 
															m_pRenderObjectSystem, 
															m_pJobSystem, 
															m_pStringDictionary,
															m_pCreator);
	m_pJobSystem->AddJob(pJobCreateDrawPrimitive);
	m_pIntermediateDrawPrimData = NULL; // Will be cleaned up by the job
	m_PrimitivesCount++;		
	return true;			
}

//-----------------------------------------------------------------------------
void CObjFileParser::AllocateIntermediateData()
{
	Assert(m_pIntermediateDrawPrimData == NULL);
	CLoadingSystem::CBlockingCondition BlockingCondition((void**)&m_pIntermediateDrawPrimData, sizeof(CIntermediateDrawPrimData), m_pJob, m_pLoadingSystem);	
	if(BlockingCondition.IsBlocked())
	{
		m_pJobSystem->YieldExecution(m_pJob->GetId(), m_pJob->GetType(), m_ThreadID, &BlockingCondition);
	}
	new (m_pIntermediateDrawPrimData) CIntermediateDrawPrimData;	
}

//-----------------------------------------------------------------------------
void CObjFileParser::ParseObjFileFromMemory(unsigned char* pMemory, unsigned int Size, const char* pName, CJobSystem::CJob* pJob, unsigned int ThreadID)
{	
	m_pName = pName;
	m_ThreadID = ThreadID;
	m_pJob = pJob;
	const char* pData = (const char*)pMemory;
	const char* pEndData = &pData[Size];
	unsigned int PosCount = 0;
	unsigned int NrmCount = 0;
	unsigned int UvCount = 0;		

	AllocateIntermediateData();
	m_RawVertexData.Reset();	
	m_SmoothingGroup = 0;
	while((pData < pEndData)) // && (!m_ForceExit)) Todo
	{
		// Comment
		CTextReader::SkipSpacesEOLAndTab(&pData);	
		if(pData >= pEndData)
		{
			break;		
		}				

		if(pData[0] == '#')
		{
			CTextReader::SkipComments('#', &pData);
		}			
		// mtllib
		else if((pData[0] == 'm') && (pData[1] == 't') && (pData[2] =='l') && (pData[3] == 'l') && (pData[4] == 'i') && (pData[5] == 'b'))
		{
			m_State = eLibrary;
			pData += 6;
			CTextReader::ReadFilename(m_MtllibName, &pData);	
			
			CJobLoadMaterialLibrary* pJobLoadMaterialLibrary = m_pJobSystem->AcquireJob<CJobLoadMaterialLibrary>(CJobSystem::ePriority0);
			new (pJobLoadMaterialLibrary)CJobLoadMaterialLibrary(m_MtllibName, m_pLoadingSystem, m_pTextureSystem, m_pMaterialSystem, m_pCreator->GetTransformType());
			m_pJobSystem->AddJob(pJobLoadMaterialLibrary);
		}
		
		// xyz, uv or normal
		else if(pData[0] == 'v')
		{
			if((m_State == eTriangles) && (!m_pIntermediateDrawPrimData->IsEmpty())) // We were reading face data and now back to vertex data
			{
				bool Res = CreateDrawPrimitive();
				m_pJobSystem->YieldToHigherPriority(pJob->GetPriority(), ThreadID);
				AllocateIntermediateData();
				PosCount += m_RawVertexData.m_PosCount; // update the vertex data counts and reset the raw data pool
				NrmCount += m_RawVertexData.m_NrmCount;
				UvCount += m_RawVertexData.m_UvCount;					
				m_RawVertexData.Reset();
				m_TriangleCount = 0;							
				if(!Res)
				{
					break;
				}																					
			}
			m_State = eVertexData;
			if(pData[1] == 't') // uv
			{
				pData += 2;
				XMFLOAT2 Uv;
				CTextReader::ReadFloat(Uv.x, &pData);
				CTextReader::ReadFloat(Uv.y, &pData);						
				m_RawVertexData.AddUv(Uv);
				float Dummy;
				CTextReader::ReadFloat(Dummy, &pData);				
			}		
			else if(pData[1] == 'n') // normal
			{
				pData += 2;
				XMFLOAT3 Normal;
				CTextReader::ReadFloat(Normal.x, &pData);
				CTextReader::ReadFloat(Normal.y, &pData);						
				CTextReader::ReadFloat(Normal.z, &pData);												
				m_RawVertexData.AddNormal(Normal);
			}		
			else // xyz
			{
				pData += 1;					
				XMFLOAT3 Pos;
				CTextReader::ReadFloat(Pos.x, &pData);
				CTextReader::ReadFloat(Pos.y, &pData);						
				CTextReader::ReadFloat(Pos.z, &pData);	
				m_RawVertexData.AddPos(Pos);				
			}
		}
		// Group
		else if(pData[0] == 'g')
		{
			Assert(m_State == eVertexData);
			m_State = eTriangles;	
			pData += 1;							
			CTextReader::ReadString(m_GroupName, &pData);
		}				
		
		// usemtl
		else if((pData[0] == 'u') && (pData[1] == 's') && (pData[2] == 'e') && (pData[3] == 'm') && (pData[4] == 't') && (pData[5] == 'l'))
		{
			pData += 6;	
			if (m_State == eTriangles) // If not then we got a material change while reading geometry data, so ignore previous one?
			{
				if (!m_pIntermediateDrawPrimData->IsEmpty()) // if we have any geometry loaded, flush it to a draw primitive since we have a material change
				{
					bool Res = CreateDrawPrimitive();
					AllocateIntermediateData();
					m_TriangleCount = 0;
					if (!Res)
					{
						break;
					}
				}
			}
			CTextReader::ReadString(m_MtlName, &pData);								
		}
		
		// Smoothing group
		else if(pData[0] == 's')
		{
			Assert(m_State == eTriangles);				
			pData += 1;	
			ReadSmoothingGroup(m_SmoothingGroup, &pData);
		}

		// Face
		else if(pData[0] == 'f')
		{
			Assert((m_State == eTriangles) || (m_GroupName[0] == '\0'));
			m_State = eTriangles;
			pData += 1;	
			CIntermediateDrawPrimData::STriangle Triangles[2];
			// Read the face, offset relative to this group since in obj file vertex indices are stored as if all objects share the same vetex pool
			unsigned int NumTriangles = ReadTriOrQuad(Triangles, &pData, PosCount, NrmCount, UvCount);
			m_TriangleCount += (unsigned short)NumTriangles;
			for( unsigned int TriangleIndex = 0; TriangleIndex < NumTriangles; TriangleIndex++)
			{
				m_pIntermediateDrawPrimData->AddTriangle(&Triangles[TriangleIndex], &m_RawVertexData);						
			}
		}
		else
		{
			Assert(0);
		}								
	}
//	if(m_ForceExit) Todo
//	{
//		return;
//	}
	if((m_State == eTriangles) && (!m_pIntermediateDrawPrimData->IsEmpty()))
	{
		CreateDrawPrimitive();			
	}
	else
	{
		m_pLoadingSystem->Free(m_pIntermediateDrawPrimData, sizeof(CIntermediateDrawPrimData));
	}
}

