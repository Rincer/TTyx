#include "stdafx.h"
#include "HeapAllocator.h"
#include "VertexFormats.h"

#include "VertexProcessing.h"

static const unsigned int scMaxVertsPerBuffer = 16 * 1024; // Arbitrary number
static const unsigned int scVertexProcessingHeapSize = scMaxVertsPerBuffer * sizeof(XMFLOAT3);


//--------------------------------------------------------------------------------
CVertexProcessing::CVertexProcessing()
{
	m_pHeapAllocator = new CHeapAllocator(scVertexProcessingHeapSize, false);
}

//--------------------------------------------------------------------------------
CVertexProcessing::~CVertexProcessing()
{
	delete m_pHeapAllocator;
}

//--------------------------------------------------------------------------------
void CVertexProcessing::CreateTangents(SVertexObj* pVertices, unsigned short NumVertices, const unsigned short* pIndices, unsigned int NumIndices)
{
	XMFLOAT3* pBiTangent = (XMFLOAT3*)m_pHeapAllocator->Alloc(NumVertices * sizeof(XMFLOAT3));
	memset(pBiTangent, 0, NumVertices * sizeof(XMFLOAT3));
	for (unsigned int Index = 0; Index < NumIndices / 3; Index ++)
    {
        unsigned int i1 = pIndices[Index * 3 + 0];
        unsigned int i2 = pIndices[Index * 3 + 1];
        unsigned int i3 = pIndices[Index * 3 + 2];
                       
        const XMFLOAT3& Pos1 = pVertices[i1].m_Pos;
        const XMFLOAT3& Pos2 = pVertices[i2].m_Pos;
        const XMFLOAT3& Pos3 = pVertices[i3].m_Pos;
        
        const XMFLOAT2& Uv1 = pVertices[i1].m_Uv;
        const XMFLOAT2& Uv2 = pVertices[i2].m_Uv;
        const XMFLOAT2& Uv3 = pVertices[i3].m_Uv;
        
        float x1 = Pos2.x - Pos1.x;
        float x2 = Pos3.x - Pos1.x;
        float y1 = Pos2.y - Pos1.y;
        float y2 = Pos3.y - Pos1.y;
        float z1 = Pos2.z - Pos1.z;
        float z2 = Pos3.z - Pos1.z;
        
        float s1 = Uv2.x - Uv1.x;
        float s2 = Uv3.x - Uv1.x;
        float t1 = Uv2.y - Uv1.y;
        float t2 = Uv3.y - Uv1.y;
        
        float r = 1.0F / (s1 * t2 - s2 * t1);
        XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
        XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
        
        pVertices[i1].m_Tangent.x += sdir.x; pVertices[i1].m_Tangent.y += sdir.y; pVertices[i1].m_Tangent.z += sdir.z;
        pVertices[i2].m_Tangent.x += sdir.x; pVertices[i2].m_Tangent.y += sdir.y; pVertices[i2].m_Tangent.z += sdir.z;
        pVertices[i3].m_Tangent.x += sdir.x; pVertices[i3].m_Tangent.y += sdir.y; pVertices[i3].m_Tangent.z += sdir.z;                
        
        pBiTangent[i1].x += tdir.x; pBiTangent[i1].y += tdir.y; pBiTangent[i1].z += tdir.z;
        pBiTangent[i2].x += tdir.x; pBiTangent[i2].y += tdir.y; pBiTangent[i2].z += tdir.z;
        pBiTangent[i3].x += tdir.x; pBiTangent[i3].y += tdir.y; pBiTangent[i3].z += tdir.z;                
    }
    
    for (unsigned int Vertex = 0; Vertex < NumVertices; Vertex++)
    {
        const XMVECTOR n = XMVectorSet(pVertices[Vertex].m_Normal.x, pVertices[Vertex].m_Normal.y, pVertices[Vertex].m_Normal.z, 0.0f);
			  XMVECTOR t = XMVectorSet(pVertices[Vertex].m_Tangent.x, pVertices[Vertex].m_Tangent.y, pVertices[Vertex].m_Tangent.z, 0.0f); 
        const XMVECTOR b = XMVector3Normalize(XMVectorSet(pBiTangent[Vertex].x, pBiTangent[Vertex].y, pBiTangent[Vertex].z, 0.0f)); 
        // Gram-Schmidt orthogonalize
        t = XMVector3Normalize(t - n * XMVector3Dot(n, t));
		pVertices[Vertex].m_Tangent.x = t.m128_f32[0];
		pVertices[Vertex].m_Tangent.y = t.m128_f32[1];
		pVertices[Vertex].m_Tangent.z = t.m128_f32[2];        

		// Calculate handedness        
        pVertices[Vertex].m_Tangent.w = XMVector3Dot(XMVector3Cross(n, t), b).m128_f32[0] < 0.0f ? -1.0f : 1.0f;        
    }	
	m_pHeapAllocator->Free(pBiTangent);
}

//--------------------------------------------------------------------------------
void CVertexProcessing::CreateSphere(SPos* pVertices, unsigned short* pIndices, unsigned short Segments)
{
	unsigned short TotalVerts = (Segments * 2 - 1) * Segments * 4 + 2;
	unsigned short NumTris = Segments * 4 * 2 + (Segments * 2 - 2) * (Segments * 4) * 2;
	unsigned short TotalIndices = NumTris * 3;
	unsigned short VertexCount = 0;
	SPos* pVert = pVertices;
	pVert->m_Pos.x = 0.0f;
	pVert->m_Pos.y = 1.0f;
	pVert->m_Pos.z = 0.0f;
	pVert++;
	VertexCount++;
	float AngleIncrement = XM_PI / (Segments * 2);
	for (unsigned short ZSegment = 1; ZSegment < Segments * 2; ZSegment++)
	{
		float ZRotation = ZSegment * AngleIncrement;
		float CosZ = cos(ZRotation);
		float SinZ = sin(ZRotation);

		for (unsigned short YSegment = 0; YSegment < Segments * 4; YSegment++)
		{
			float YRotation = YSegment * AngleIncrement;
			pVert->m_Pos.x = SinZ * cos(YRotation);
			pVert->m_Pos.y = CosZ;
			pVert->m_Pos.z = SinZ * sin(YRotation);
			pVert++;
			VertexCount++;
		}
	}
	pVert->m_Pos.x = 0.0f;
	pVert->m_Pos.y = -1.0f;
	pVert->m_Pos.z = 0.0f;
	VertexCount++;
	Assert(VertexCount == TotalVerts);

	unsigned short* pIndex = pIndices;
	unsigned short Offset = 1;
	unsigned short IndexCount = 0;
	for (unsigned short YSegment = 0; YSegment < Segments * 4; YSegment++)
	{
		pIndex[0] = 0;
		pIndex[1] = (YSegment == Segments * 4 - 1) ? 1 : Offset + 1; // wrap back for last triangle
		pIndex[2] = Offset; 
		pIndex += 3;
		IndexCount += 3;
		Offset++;
	}

	for (unsigned short ZSegment = 0; ZSegment < Segments * 2 - 2; ZSegment++)
	{
		for (unsigned short YSegment = 0; YSegment < Segments * 4; YSegment++)
		{
			unsigned short BotLeft = Offset;
			unsigned short TopLeft = Offset - Segments * 4;
			unsigned short BotRight = BotLeft + 1;
			unsigned short TopRight = TopLeft + 1;
			if (YSegment == Segments * 4 - 1) // wrap back for the last quad
			{
				TopRight -= Segments * 4; 
				BotRight -= Segments * 4;
			}

			pIndex[0] = TopLeft;
			pIndex[1] = TopRight;
			pIndex[2] = BotLeft; 
			pIndex += 3;
			IndexCount += 3;

			pIndex[0] = TopRight;
			pIndex[1] = BotRight; 
			pIndex[2] = BotLeft;
			pIndex += 3;
			IndexCount += 3;
			Offset++;
		}
	}

	unsigned short LastVertex = TotalVerts - 1;
	Offset -= Segments * 4;
	unsigned short LastLayerOffest = Offset;
	for (unsigned short YSegment = 0; YSegment < Segments * 4; YSegment++)
	{
		pIndex[0] = LastVertex;
		pIndex[1] = Offset;
		pIndex[2] = (YSegment == Segments * 4 - 1) ? LastLayerOffest : Offset + 1; 
		pIndex += 3;
		IndexCount += 3;
		Offset++;
	}
	Assert(IndexCount == TotalIndices);
}