#include "stdafx.h"
#include "Macros.h"

#include "DrawData.h"

//-----------------------------------------------------------------------------
CIntermediateDrawPrimData::CIntermediateDrawPrimData()
{
	Reset();
	m_InUseCount = 2;	
}

//-----------------------------------------------------------------------------		
void CIntermediateDrawPrimData::Reset()
{
	m_VertexCount = 0;		
	m_IndexCount = 0;					
}
		
//-----------------------------------------------------------------------------		
unsigned int CIntermediateDrawPrimData::DecrementInUseCount()
{
	return InterlockedDecrement(&m_InUseCount);
}

//-----------------------------------------------------------------------------
void CIntermediateDrawPrimData::AddTriangle(const STriangle* pTriangle, CRawVertexData* pRawVertexData)
{
	const XMFLOAT3* pPos;
	const XMFLOAT2* pUv;
	const XMFLOAT3* pNormal;
	pRawVertexData->GetData(&pPos, &pUv, &pNormal);
	for(unsigned int TriVIndex = 0; TriVIndex < 3; TriVIndex++)
	{
		unsigned int FoundIndex = 0xFFFFFFFF;
		for(unsigned int VIndex = 0; VIndex < m_VertexCount; VIndex++)
		{
			if(m_PosNormUvSG[VIndex] == pTriangle->m_Verts[TriVIndex])
			{
				FoundIndex = VIndex;
				break;
			}
		}
		// New vertex
		if(FoundIndex == 0xFFFFFFFF)
		{
			Assert(m_VertexCount < scMaxVertsPerPrimitive);
			FoundIndex = m_VertexCount;
			m_PosNormUvSG[FoundIndex] = pTriangle->m_Verts[TriVIndex];
			m_Vertices[FoundIndex].m_Normal = pNormal[pTriangle->m_Verts[TriVIndex].m_NrmIndex];
			m_Vertices[FoundIndex].m_Pos = pPos[pTriangle->m_Verts[TriVIndex].m_PosIndex];
			m_Vertices[FoundIndex].m_Uv = pUv[pTriangle->m_Verts[TriVIndex].m_UvIndex];
			m_Vertices[FoundIndex].m_Tangent = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			m_VertexCount++;
		}
		Assert(m_IndexCount < 65535);
		m_Indices[m_IndexCount] = (unsigned short)FoundIndex;
		m_IndexCount++;				
	}
}

//-----------------------------------------------------------------------------
void CIntermediateDrawPrimData::CalculateSmoothingGroups()
{
	memset(m_SGCalculated, 0, sizeof(m_SGCalculated));
	bool AllVerticesScanned = false;
	unsigned int PivotVertex = 0;
	while(!AllVerticesScanned)
	{
		AllVerticesScanned = true;	
		for(unsigned int VIndex = PivotVertex; VIndex < m_VertexCount; VIndex++)
		{
			if(!m_SGCalculated[VIndex]) // has this vertex already participated in smoothing group calculation?
			{
				m_SGCalculated[VIndex] = true;
				AllVerticesScanned = false;		
				m_SGNormalIndices[0] = (unsigned short)VIndex;	// indices of normals that are going to be factored in
				m_SGVertexIndices[0] = (unsigned short)VIndex;	// indices of vertices that are going to receive the final smoothed out normal												
				m_NumSGNormalIndices = 1;
				m_NumSGVertexIndices = 1;						
				for(unsigned int VIndex2 = VIndex + 1; VIndex2 < m_VertexCount; VIndex2++)
				{
					// Find a vertex that is in the same smoothing group and shares the position with the pivot vertex
					if(	(m_PosNormUvSG[VIndex2].m_PosIndex == m_PosNormUvSG[VIndex].m_PosIndex) &&
						(m_PosNormUvSG[VIndex2].m_SGIndex != 0xFFFF) &&
						(m_PosNormUvSG[VIndex2].m_SGIndex == m_PosNormUvSG[VIndex].m_SGIndex))
					{
						m_SGCalculated[VIndex2] = true;
						m_SGVertexIndices[m_NumSGVertexIndices] = (unsigned short)VIndex2;	// This vertex will receive the smoothed out normal												
						m_NumSGVertexIndices++;
						unsigned int NormalIndex; 
						// Has the normal for this vertex been factored in already?
						for(NormalIndex = 0; NormalIndex < m_NumSGNormalIndices; NormalIndex++)
						{
							if(m_PosNormUvSG[m_SGNormalIndices[NormalIndex]].m_NrmIndex == m_PosNormUvSG[VIndex2].m_NrmIndex)
							{
								break;
							}
						}
						// The normal for this vertex hasnt been factored in yet, add it to the list
						if(NormalIndex == m_NumSGNormalIndices)
						{
							m_SGNormalIndices[m_NumSGNormalIndices] = (unsigned short)VIndex2;
							m_NumSGNormalIndices++;
						}
					}
				}				
				// Calculate the smoothed out normal		
				XMVECTORF32 Normal;
				Normal.v = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
				for(unsigned int SharedNormal = 0; SharedNormal < m_NumSGNormalIndices; SharedNormal++)
				{
					Normal.f[0] += m_Vertices[m_SGNormalIndices[SharedNormal]].m_Normal.x;
					Normal.f[1] += m_Vertices[m_SGNormalIndices[SharedNormal]].m_Normal.y;
					Normal.f[2] += m_Vertices[m_SGNormalIndices[SharedNormal]].m_Normal.z;														
				}
				// Replicate the normal to all the affected vertices 
				Normal.v = XMVector3Normalize(Normal.v);
				for(unsigned int SharedVertex = 0; SharedVertex < m_NumSGVertexIndices; SharedVertex++)
				{
					m_Vertices[m_SGVertexIndices[SharedVertex]].m_Normal.x = Normal.f[0]; 
					m_Vertices[m_SGVertexIndices[SharedVertex]].m_Normal.y = Normal.f[1]; 
					m_Vertices[m_SGVertexIndices[SharedVertex]].m_Normal.z = Normal.f[2]; 														
				}						
			}
		}
		PivotVertex++;
	}			
}

//-----------------------------------------------------------------------------				
void CIntermediateDrawPrimData::GetVertexAndIndexData(SVertexObj** ppVertices, unsigned short& VertexCount, const unsigned short** ppIndices, unsigned short &IndexCount)
{
	(*ppVertices) = m_Vertices;
	VertexCount	  = m_VertexCount;
	(*ppIndices)  = m_Indices;
	IndexCount    = m_IndexCount;
}

//-----------------------------------------------------------------------------				
const CIntermediateDrawPrimData::SPosNormUvSG* CIntermediateDrawPrimData::GetSrcIndexData()
{
	return m_PosNormUvSG;
}

//-----------------------------------------------------------------------------				
bool CIntermediateDrawPrimData::IsEmpty()
{
	return (m_IndexCount == 0);
}

//-----------------------------------------------------------------------------				
CIntermediateSkinningData::CIntermediateSkinningData()
{
	m_VertexCount = 0;
}

//-----------------------------------------------------------------------------				
void CIntermediateSkinningData::Create(const CIntermediateDrawPrimData::SPosNormUvSG* pSrcVertexIndices, const unsigned short* pIndices, CRawSkinningData* pRawSkinningData, unsigned short IndexCount, unsigned short VertexCount)
{
	m_VertexCount = VertexCount;
	for(unsigned short Index = 0; Index < IndexCount; Index++)
	{
		unsigned short VertIndex = pIndices[Index];
		unsigned short SrcIndex = (unsigned short)pSrcVertexIndices[VertIndex].m_PosIndex;
		unsigned int BoneIndices;
		unsigned int BoneWeights;		
		unsigned int BoneCount;			
		pRawSkinningData->GetBoneInfluence(BoneIndices, BoneWeights, BoneCount, SrcIndex);
		BoneIndices += 0x01010101; // Offset by 1 bone since bone 0 will be identity
		if(BoneCount == 0)
		{
			BoneWeights = 0xFF;	// influence of (1,0,0,0)
			BoneIndices = 0;	// bone (0, 0, 0, 0) special case identity transform at bone 0
		}
		
		m_Vertices[VertIndex].m_BoneIndices = BoneIndices;
		m_Vertices[VertIndex].m_Weights = BoneWeights;		
	}
}

//-----------------------------------------------------------------------------		
void CIntermediateSkinningData::GetVertexData(SVertexSkinData** ppVertices)
{
	*ppVertices = m_Vertices;
}

//-----------------------------------------------------------------------------		
void CRawVertexData::Reset()
{
	m_PosCount = 0;
	m_NrmCount = 0;
	m_UvCount = 0;		
}

//-----------------------------------------------------------------------------		
CRawVertexData::CRawVertexData()
{
	Reset();
}

//-----------------------------------------------------------------------------
void CRawVertexData::AddPos(XMFLOAT3& Pos)
{
	Assert(m_PosCount < scMaxVertsPerObj);
	m_Pos[m_PosCount] = Pos;
	m_PosCount++;
}

//-----------------------------------------------------------------------------
void CRawVertexData::AddUv(XMFLOAT2& Uv)
{
	Assert(m_UvCount < scMaxVertsPerObj);
	m_Uv[m_UvCount] = Uv;
	m_UvCount++;
}

//-----------------------------------------------------------------------------
void CRawVertexData::AddNormal(XMFLOAT3& Normal)
{
	Assert(m_NrmCount < scMaxVertsPerObj);
	m_Normal[m_NrmCount] = Normal;
	m_NrmCount++;
}
		
//-----------------------------------------------------------------------------
void CRawVertexData::GetData(const XMFLOAT3** ppPos, const XMFLOAT2** ppUv, const XMFLOAT3** ppNormal)
{
	*ppPos    = m_Pos;
	*ppUv     = m_Uv;
	*ppNormal = m_Normal;	
}

//-----------------------------------------------------------------------------		
CRawSkinningData::CRawSkinningData()
{
	Reset();
}

//-----------------------------------------------------------------------------		
void CRawSkinningData::Reset()
{
	memset(m_BoneIndices, 0, sizeof(m_BoneIndices));	
	memset(m_BoneWeights, 0, sizeof(m_BoneWeights));	
	memset(m_BoneCounts, 0, sizeof(m_BoneCounts));	
	m_NumVerts = 0;
}

//-----------------------------------------------------------------------------		
void CRawSkinningData::AddBoneInfluence(unsigned int VertexIndex, unsigned char BoneIndex, unsigned char BoneWeight)
{
	Assert(VertexIndex < scMaxVertsPerObj);
	Assert(m_BoneCounts[VertexIndex] < 4);
	if(m_BoneCounts[VertexIndex] == 0) // only if no bone influences have been added for this vertex increment the count
	{
		m_NumVerts++;
	}
	unsigned char* pBoneData = (unsigned char*)&m_BoneIndices[VertexIndex];	
	pBoneData[m_BoneCounts[VertexIndex]] = BoneIndex;
	pBoneData = (unsigned char*)&m_BoneWeights[VertexIndex];
	pBoneData[m_BoneCounts[VertexIndex]] = BoneWeight;	
	m_BoneCounts[VertexIndex]++;
}

//-----------------------------------------------------------------------------		
void CRawSkinningData::GetBoneInfluence(unsigned int& BoneIndices, unsigned int& BoneWeights, unsigned int& BoneCounts, unsigned int VertexIndex)
{
	Assert(VertexIndex < scMaxVertsPerObj);
	BoneIndices = m_BoneIndices[VertexIndex];
	BoneWeights = m_BoneWeights[VertexIndex];
	BoneCounts = m_BoneCounts[VertexIndex];		
}

//-----------------------------------------------------------------------------		
bool CRawSkinningData::IsEmpty()
{
	return (m_NumVerts == 0);
}

//-----------------------------------------------------------------------------		
CRawIndexData::CRawIndexData()
{
	Reset();
}

//-----------------------------------------------------------------------------		
void CRawIndexData::Reset()
{
	m_PosTriCount = 0;
	m_NrmTriCount = 0;
	m_UvTriCount = 0;		
}

//-----------------------------------------------------------------------------		
void CRawIndexData::AddPosTri(unsigned int Index0, unsigned int Index1, unsigned int Index2)
{
	Assert(m_PosTriCount < scMaxVertsPerPrimitive * 3);
	m_PosTris[m_PosTriCount][0] = Index0;
	m_PosTris[m_PosTriCount][1] = Index1;
	m_PosTris[m_PosTriCount][2] = Index2;		
	m_PosTriCount++;
}

//-----------------------------------------------------------------------------				
void CRawIndexData::AddUvTri(unsigned int Index0, unsigned int Index1, unsigned int Index2)
{
	Assert(m_UvTriCount < scMaxVertsPerPrimitive * 3);
	m_UvTris[m_UvTriCount][0] = Index0;
	m_UvTris[m_UvTriCount][1] = Index1;
	m_UvTris[m_UvTriCount][2] = Index2;			
	m_UvTriCount++;
}

//-----------------------------------------------------------------------------				
void CRawIndexData::AddNormalTri(unsigned int Index0, unsigned int Index1, unsigned int Index2)
{
	Assert(m_NrmTriCount < scMaxVertsPerPrimitive * 3);
	m_NormalTris[m_NrmTriCount][0] = Index0;
	m_NormalTris[m_NrmTriCount][1] = Index1;
	m_NormalTris[m_NrmTriCount][2] = Index2;		
	m_NrmTriCount++;
}

//-----------------------------------------------------------------------------				
void CRawIndexData::GetIndexData(unsigned int& NumTris, const unsigned int** ppPosTris, const unsigned int** ppNrmTris, const unsigned int** ppUvTris)
{
	Assert((m_PosTriCount == m_UvTriCount) && (m_PosTriCount == m_NrmTriCount));
	NumTris = m_PosTriCount;
	*ppPosTris = m_PosTris[0];
	*ppNrmTris = m_NormalTris[0];
	*ppUvTris = m_UvTris[0];
}
		
