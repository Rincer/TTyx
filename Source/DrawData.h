#ifndef _DRAWDATA_H_
#define _DRAWDATA_H_

#include "VertexFormats.h"
#include "DrawPrimitive.h"

static const unsigned int scMaxVertsPerObj = 64 * 1024;	// 16 bit indices

//-----------------------------------------------------------------------------		
class CRawVertexData
{
	public:
		void Reset();
		CRawVertexData();
		void AddPos(XMFLOAT3& Pos);
		void AddUv(XMFLOAT2& Uv);
		void AddNormal(XMFLOAT3& Normal);
		void GetData(const XMFLOAT3** ppPos, const XMFLOAT2** ppUv, const XMFLOAT3** ppNormal);		
		unsigned int m_PosCount;
		unsigned int m_NrmCount;
		unsigned int m_UvCount;		
		
	private:		
		XMFLOAT3	 m_Pos[scMaxVertsPerObj];
		XMFLOAT2	 m_Uv[scMaxVertsPerObj];
		XMFLOAT3	 m_Normal[scMaxVertsPerObj];		
};


//-----------------------------------------------------------------------------		
class CRawSkinningData
{
	public:
		void Reset();
		CRawSkinningData();
		void AddBoneInfluence(unsigned int VertexIndex, unsigned char BoneIndex, unsigned char BoneWeight);	
		void GetBoneInfluence(unsigned int& BoneIndices, unsigned int& BoneWeights, unsigned int& BoneCounts, unsigned int VertexIndex);
		bool IsEmpty();

	private:
		unsigned int  m_BoneIndices[scMaxVertsPerObj];
		unsigned int  m_BoneWeights[scMaxVertsPerObj];
		unsigned char m_BoneCounts[scMaxVertsPerObj];		
		unsigned int  m_NumVerts;
};


//-----------------------------------------------------------------------------		
class CRawIndexData
{
	public:
		void Reset();
		CRawIndexData();
		void AddPosTri(unsigned int Index0, unsigned int Index1, unsigned int Index2);
		void AddUvTri(unsigned int Index0, unsigned int Index1, unsigned int Index2);
		void AddNormalTri(unsigned int Index0, unsigned int Index1, unsigned int Index2);
		void GetIndexData(unsigned int& NumTris, const unsigned int** ppPosTris, const unsigned int** ppNrmTris, const unsigned int** ppUvTris);
		
	private:
		unsigned int m_PosTriCount;
		unsigned int m_NrmTriCount;
		unsigned int m_UvTriCount;		
		unsigned int m_PosTris[scMaxVertsPerPrimitive * 3][3];
		unsigned int m_UvTris[scMaxVertsPerPrimitive * 3][3];
		unsigned int m_NormalTris[scMaxVertsPerPrimitive * 3][3];		
};

//-----------------------------------------------------------------------------		
class CIntermediateDrawPrimData
{
	public:		
		struct SPosNormUvSG
		{
			unsigned int m_PosIndex;
			unsigned int m_UvIndex;			
			unsigned int m_NrmIndex;
			unsigned int m_SGIndex;
			bool operator == (const SPosNormUvSG& RHS) const 
			{
				return (m_PosIndex == RHS.m_PosIndex) && (m_UvIndex == RHS.m_UvIndex) && (m_NrmIndex == RHS.m_NrmIndex);
			}			
		};
	
		struct STriangle
		{
			SPosNormUvSG m_Verts[3];
		};
	
		CIntermediateDrawPrimData();
		void Reset();		
		void AddTriangle(const STriangle* pTriangle, CRawVertexData* pRawVertexData);		
		void CalculateSmoothingGroups();		
		void GetVertexAndIndexData(SVertexObj** ppVertices, unsigned short& VertexCount, const unsigned short** ppIndices, unsigned short &IndexCount);
		bool IsEmpty();
		const SPosNormUvSG* GetSrcIndexData();
		unsigned int DecrementInUseCount();
		
	private:	
		SPosNormUvSG	m_PosNormUvSG[scMaxVertsPerPrimitive];			
		SVertexObj		m_Vertices[scMaxVertsPerPrimitive];
		unsigned short  m_Indices[scMaxVertsPerPrimitive * 3];			
		unsigned short	m_VertexCount;		
		unsigned short	m_IndexCount;	
		
		// Smoothing group calculation data
		unsigned short  m_SGNormalIndices[scMaxVertsPerPrimitive];	
		unsigned short  m_NumSGNormalIndices;
		unsigned short  m_SGVertexIndices[scMaxVertsPerPrimitive];	
		unsigned short  m_NumSGVertexIndices;								
		long			m_InUseCount;	
		bool			m_SGCalculated[scMaxVertsPerPrimitive];	
};

//-----------------------------------------------------------------------------		
class CIntermediateSkinningData
{
	public:
		CIntermediateSkinningData();
		void Create(const CIntermediateDrawPrimData::SPosNormUvSG* pSrcVertexIndices, const unsigned short* pIndices, CRawSkinningData* pRawSkinningData, unsigned short IndexCount, unsigned short VertexCount);
		void GetVertexData(SVertexSkinData** ppVertices);
	private:
		SVertexSkinData m_Vertices[scMaxVertsPerPrimitive];
		unsigned short	m_VertexCount;
};

#endif