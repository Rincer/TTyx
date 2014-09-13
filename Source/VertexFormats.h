#ifndef _VERTEXFORMATS_H_
#define _VERTEXFORMATS_H_


// Vertex used by the Obj material (static geometry)
struct SVertexObj
{
	XMFLOAT3 m_Pos;
	XMFLOAT2 m_Uv;
	XMFLOAT3 m_Normal;
	XMFLOAT4 m_Tangent;
};

// Vertex used by the plain color
struct SPosTexColor
{
	XMFLOAT3		m_Pos;
	XMFLOAT2		m_Tex;
	unsigned int	m_Col;
};


// Position only
struct SPos
{
	XMFLOAT3		m_Pos;
};

// Position and texture
struct SPosTex
{
	XMFLOAT3		m_Pos;
	XMFLOAT2		m_Tex;
};

// Skinning data 
struct SVertexSkinData
{
	unsigned int m_BoneIndices;
	unsigned int m_Weights;
};


#endif