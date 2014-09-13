#include "stdafx.h"
#include "TextReader.h"
#include "MaterialSystem.h"
#include "TextureSystem.h"
#include "VertexProcessing.h"
#include "GeometrySystem.h"
#include "Hash64.h"
#include "StackAllocator.h"
#include "MemoryManager.h"
#include "macros.h"
#include "RenderObject.h"
#include "StringDictionary.h"
#include "TextureSystem.h"
#include "MaterialPhong.h"

#include "XFileParser.h"


CXFileParser::tToken CXFileParser::m_Tokens[eTotalTokens] =
{
	{"template",				strlen("template"),					eTemplate,					NULL,							NULL,								NULL},
	{"Frame",					strlen("Frame"),					eFrame,						CXFileParser::PreFrame,			CXFileParser::ProcessFrame,		    CXFileParser::PostFrame},	
	{"FrameTransformMatrix",	strlen("FrameTransformMatrix"),		eFrameTransformMatrix,		NULL,							CXFileParser::ProcessFrameTransform,NULL},
	{"Mesh",					strlen("Mesh"),						eMesh,						CXFileParser::PreMesh,			CXFileParser::ProcessMesh,			CXFileParser::PostMesh},	
	{"MeshNormals",				strlen("MeshNormals"),				eMeshNormals,				NULL,							CXFileParser::ProcessNormals,		NULL},
	{"MeshTextureCoords",		strlen("MeshTextureCoords"),		eMeshTextureCoords,			NULL,							CXFileParser::ProcessTextureCoords, NULL},
	{"VertexDuplicationIndices",strlen("VertexDuplicationIndices"), eVertexDuplicationIndices,	NULL,							NULL,								NULL},
	{"MeshMaterialList",		strlen("MeshMaterialList"),			eMeshMaterialList,			NULL,							CXFileParser::ProcessMeshMaterial,  NULL},			
	{"Material",				strlen("Material"),					eMaterial,					NULL,							CXFileParser::ProcessMaterial,		CXFileParser::PostMaterial},				
	{"TextureFilename",			strlen("TextureFilename"),			eTextureFilename,			NULL,							CXFileParser::ProcessTextureName,	NULL},					
	{"XSkinMeshHeader",			strlen("XSkinMeshHeader"),			eXSkinMeshHeader,			NULL,							NULL,								NULL},						
	{"SkinWeights",				strlen("SkinWeights"),				eSkinWeights,				NULL,							CXFileParser::ProcessSkinWeights,	NULL},							
	{"AnimationSet",			strlen("AnimationSet"),				eAnimationSet,				CXFileParser::PreAnimationSet,	CXFileParser::ProcessAnimationSet,	CXFileParser::PostAnimationSet},								
	{"Animation",				strlen("Animation"),				eAnimation,					NULL,							CXFileParser::ProcessAnimation,		CXFileParser::PostAnimation},									
	{"AnimationKey",			strlen("AnimationKey"),				eAnimationKey,				NULL,							CXFileParser::ProcessAnimationKey,	NULL},										
};


//-----------------------------------------------------------------------------
CXFileParser::CXFileParser(	CTextureSystem* pTextureSystem, 
							CMaterialSystem* pMaterialSystem, 
							CAnimationSystem* pAnimationSystem,
							CStringDictionary* pStringDictionary,
							CGeometrySystem* pGeometrySystem,
							CDrawPrimitiveSystem* pDrawPrimitiveSystem,
							CSkeletonSystem* pSkeletonSystem,
							CRenderObjectSystem* pRenderObjectSystem,
							CRenderObjectSystem::CRenderObjectCreatorBase* pCreator) : m_pTextureSystem(pTextureSystem),
																		m_pMaterialSystem(pMaterialSystem),
																		m_pAnimationSystem(pAnimationSystem),
																		m_pStringDictionary(pStringDictionary),
																		m_pGeometrySystem(pGeometrySystem),
																		m_pDrawPrimitiveSystem(pDrawPrimitiveSystem),
																		m_pSkeletonSystem(pSkeletonSystem),
																		m_pRenderObjectSystem(pRenderObjectSystem),
																		m_pCreator(pCreator)
{	
	Reset();
	m_pLocalAllocator = new (&m_LocalAllocator) CStackAllocator(m_ScratchMemory, scScratchMemory, 16);
	m_pBonesHash = new (m_pLocalAllocator->Alloc(sizeof(CStaticHashMap<unsigned short>))) CStaticHashMap<unsigned short>(m_pLocalAllocator, CSkeleton::scMaxBones);	
	m_LocalDataOffset = m_pLocalAllocator->GetOffset();
}

//-----------------------------------------------------------------------------
CXFileParser::~CXFileParser()
{
}

//-----------------------------------------------------------------------------
void CXFileParser::ResetBoneData()
{
	m_NumBones = 0;
	m_NumParentedBones = 0;
}

//---------------------------------------------------------------------
void CXFileParser::SetBoneMatrix(unsigned short Index, XMMATRIX& BoneMatrix)
{
	m_BoneMatrices[Index] = BoneMatrix;
}

//---------------------------------------------------------------------
void CXFileParser::SetModelToBoneMatrix(unsigned short Index, XMMATRIX& ModelToBoneMatrix)
{
	m_ModelToBoneMatrices[Index] = ModelToBoneMatrix;
}

//---------------------------------------------------------------------
void CXFileParser::SetBoneParent(unsigned short Index, unsigned short Parent)	
{
	m_ParentBoneIndex[Index] = Parent;
	if(Parent != 0xFFFF)
	{
		m_NumParentedBones++;
	}
}

//---------------------------------------------------------------------
void CXFileParser::IncNumBones()
{
	Assert(m_NumBones < CSkeleton::scMaxBones - 1); // bone 0 is reserved for identity
	m_NumBones++;
}


//-----------------------------------------------------------------------------
void CXFileParser::Reset()
{
	m_SpecularParameters.v = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	m_DiffuseParameters.v = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	m_MtlName[0] = 0;
	m_DiffuseTextureName[0] = 0;
	m_NormalTextureName[0] = 0;
	m_SpecularTextureName[0] = 0; 
	m_AlphaTextureName[0] = 0; 	
	m_BoneStackSize = 0;
	m_NumParams = 0;
	m_NumAnimationSets = 0;
	m_pMaterial = NULL;
	ResetBoneData();
}

//-----------------------------------------------------------------------------
bool CXFileParser::IsMatching(const char* pData, const char* pToken, unsigned int TokenLength)
{
	if(pData[TokenLength] != 0) // length must match
		return false;

	for(unsigned int Char = 0; Char < TokenLength; Char++)
	{
		if(pData[Char] != pToken[Char])
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
void CXFileParser::PreMesh(CXFileParser* pContext)				
{
	pContext->m_IntermediateDrawPrimData.Reset();
	pContext->m_RawVertexData.Reset();
	pContext->m_RawIndexData.Reset();
	pContext->m_RawSkinningData.Reset();
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessMesh(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // C4100
	unsigned int NumXYZ;
	CTextReader::ReadUInt(NumXYZ, ppData);
	(*ppData)++; // Skip the ;
	for(unsigned int XYZIndex = 0; XYZIndex < NumXYZ; XYZIndex++)
	{
		XMFLOAT3 Pos;
		CTextReader::ReadFloat(Pos.x, ppData);	
		(*ppData)++; // Skip the ;		
		CTextReader::ReadFloat(Pos.y, ppData);	
		(*ppData)++; // Skip the ;		
		CTextReader::ReadFloat(Pos.z, ppData);					
		(*ppData)++; // Skip the ;		
		(*ppData)++; // Skip the ,		
		pContext->m_RawVertexData.AddPos(Pos);		
	}
	unsigned int NumFaces;
	CTextReader::ReadUInt(NumFaces, ppData);
	(*ppData)++; // Skip the ;
	for(unsigned int FaceIndex = 0; FaceIndex < NumFaces; FaceIndex++)
	{
		unsigned int VertsPerFace;
		unsigned int Indices[4];
		CTextReader::ReadUInt(VertsPerFace, ppData);
		(*ppData)++; // Skip the ;
		for(unsigned int VertexIndex = 0; VertexIndex < VertsPerFace; VertexIndex++)
		{
			CTextReader::ReadUInt(Indices[VertexIndex], ppData);
			(*ppData)++; // Skip the ; or ,		
		}
		(*ppData)++; // Skip the ; or ,		
		pContext->m_RawIndexData.AddPosTri(Indices[0], Indices[1], Indices[2]);
		pContext->m_RawIndexData.AddUvTri(Indices[0], Indices[1], Indices[2]);
		if(VertsPerFace == 4)
		{
			pContext->m_RawIndexData.AddPosTri(Indices[0], Indices[2], Indices[3]);
			pContext->m_RawIndexData.AddUvTri(Indices[0], Indices[2], Indices[3]);			
		}
	}	
}


//-----------------------------------------------------------------------------
void CXFileParser::PostMesh(CXFileParser* pContext)				
{
	pContext->CreateDrawPrimitive();
	pContext->m_NumParams++; // Assumes that each draw primitive has  exactly 1 material which gets loaded before the primitive
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessTextureCoords(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // C4100
	unsigned int NumTexCoords;
	CTextReader::ReadUInt(NumTexCoords, ppData);
	(*ppData)++; // Skip the ;
	for(unsigned int TexCoordIndex = 0; TexCoordIndex < NumTexCoords; TexCoordIndex++)
	{
		XMFLOAT2 Uv;
		CTextReader::ReadFloat(Uv.x, ppData);	
		(*ppData)++; // Skip the ;		
		CTextReader::ReadFloat(Uv.y, ppData);					
		(*ppData)++; // Skip the ;		
		(*ppData)++; // Skip the ,
		pContext->m_RawVertexData.AddUv(Uv);				
	}
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessNormals(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // C4100
	unsigned int NumNormals;
	CTextReader::ReadUInt(NumNormals, ppData);
	(*ppData)++; // Skip the ;
	for(unsigned int NormalIndex = 0; NormalIndex < NumNormals; NormalIndex++)
	{
		XMFLOAT3 Normal;		
		CTextReader::ReadFloat(Normal.x, ppData);	
		(*ppData)++; // Skip the ;		
		CTextReader::ReadFloat(Normal.y, ppData);	
		(*ppData)++; // Skip the ;		
		CTextReader::ReadFloat(Normal.z, ppData);					
		(*ppData)++; // Skip the ;		
		(*ppData)++; // Skip the ,	
		pContext->m_RawVertexData.AddNormal(Normal);			
	}
	unsigned int NumFaces;
	CTextReader::ReadUInt(NumFaces, ppData);
	(*ppData)++; // Skip the ;
	for(unsigned int FaceIndex = 0; FaceIndex < NumFaces; FaceIndex++)
	{
		unsigned int VertsPerFace;
		unsigned int Indices[4];		
		CTextReader::ReadUInt(VertsPerFace, ppData);
		(*ppData)++; // Skip the ;
		for(unsigned int VertexIndex = 0; VertexIndex < VertsPerFace; VertexIndex++)
		{
			CTextReader::ReadUInt(Indices[VertexIndex], ppData);
			(*ppData)++; // Skip the ; or ,		
		}
		(*ppData)++; // Skip the ; or ,		
		pContext->m_RawIndexData.AddNormalTri(Indices[0], Indices[1], Indices[2]);
		if(VertsPerFace == 4)
		{
			pContext->m_RawIndexData.AddNormalTri(Indices[0], Indices[2], Indices[3]);
		}		
	}	
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessMeshMaterial(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // C4100
	pContext;
	unsigned int NumMaterials;
	CTextReader::ReadUInt(NumMaterials, ppData);
	(*ppData)++; // Skip the ;
	unsigned int NumMaterialFaces;
	CTextReader::ReadUInt(NumMaterialFaces, ppData);
	(*ppData)++; // Skip the ;
	for(unsigned int FaceIndex = 0; FaceIndex < NumMaterialFaces; FaceIndex++)
	{
		unsigned int MaterialIndex;
		CTextReader::ReadUInt(MaterialIndex, ppData);
		(*ppData)++; // Skip the ;
	}
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessMaterial(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // C4100
	float EmissiveR;
	float EmissiveG;
	float EmissiveB;	
	CTextReader::ReadFloat(pContext->m_DiffuseParameters.f[0], ppData);
	(*ppData)++; // Skip the ;
	CTextReader::ReadFloat(pContext->m_DiffuseParameters.f[1], ppData);
	(*ppData)++; // Skip the ;
	CTextReader::ReadFloat(pContext->m_DiffuseParameters.f[2], ppData);
	(*ppData)++; // Skip the ;
	CTextReader::ReadFloat(pContext->m_DiffuseParameters.f[3], ppData);
	(*ppData)++; // Skip the ;
	(*ppData)++; // Skip the ;	
	
	CTextReader::ReadFloat(pContext->m_SpecularParameters.f[3], ppData);
	(*ppData)++; // Skip the ;
	CTextReader::ReadFloat(pContext->m_SpecularParameters.f[0], ppData);
	(*ppData)++; // Skip the ;	
	CTextReader::ReadFloat(pContext->m_SpecularParameters.f[1], ppData);
	(*ppData)++; // Skip the ;
	CTextReader::ReadFloat(pContext->m_SpecularParameters.f[2], ppData);
	(*ppData)++; // Skip the ;
	(*ppData)++; // Skip the ;	

	CTextReader::ReadFloat(EmissiveR, ppData);
	(*ppData)++; // Skip the ;	
	CTextReader::ReadFloat(EmissiveG, ppData);
	(*ppData)++; // Skip the ;
	CTextReader::ReadFloat(EmissiveB, ppData);
	(*ppData)++; // Skip the ;
	(*ppData)++; // Skip the ;		
}

//-----------------------------------------------------------------------------
void CXFileParser::PostMaterial(CXFileParser* pContext)
{
	pContext->AddMaterial();		
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessTextureName(CXFileParser* pContext, const char** ppData, const char* pBlockName)						
{
	pBlockName; // C4100
	CTextReader::ReadString(pContext->m_DiffuseTextureName, ppData);	
	(*ppData)++; // Skip the ;	
}

//-----------------------------------------------------------------------------
void CXFileParser::PreFrame(CXFileParser* pContext)
{
	pContext->m_BoneStack[pContext->m_BoneStackSize] = (unsigned short)pContext->m_NumBones;
	pContext->m_BoneStackSize++;
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessFrame(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	ppData; // C4100
	unsigned long long Key;
	if(pBlockName[0] > 0)
	{
		Key = CHash64::GetHash(pBlockName);
	}
	else
	{
		char Str[256];
		sprintf_s(Str, 255, "Unnamed_bone_%d", pContext->m_NumBones);
		Key = CHash64::GetHash(Str);
	}	
	Assert(pContext->m_NumBones < CSkeleton::scMaxBones - 1); // Last bone is identity for verts that are not skinned.
	pContext->m_pBonesHash->AddEntry(Key, (unsigned short)pContext->m_NumBones);
	strcpy_s(pContext->m_BoneNames[pContext->m_NumBones], 255, pBlockName);
	if(pContext->m_BoneStackSize > 1)
	{
		pContext->SetBoneParent((unsigned short)pContext->m_NumBones, pContext->m_BoneStack[pContext->m_BoneStackSize - 2]);
	}
	else
	{
		pContext->SetBoneParent((unsigned short)pContext->m_NumBones, 0xFFFF);
	}
	pContext->IncNumBones();	
}				

//-----------------------------------------------------------------------------
void CXFileParser::ProcessFrameTransform(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // c4100
	pContext->ReadBone(ppData);
}		

//-----------------------------------------------------------------------------
void CXFileParser::ProcessSkinWeights(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // c4100
	char BoneName[256];
	CTextReader::ReadString(BoneName, ppData);
	unsigned long long Key;
	Key = CHash64::GetHash(BoneName);
	unsigned short BoneIndex = *pContext->m_pBonesHash->GetValue(Key);
	(*ppData)++; // Skip the ;
	unsigned int NumVerts;
	CTextReader::ReadUInt(NumVerts, ppData);
	(*ppData)++; // Skip the ;
	pContext->m_pLocalAllocator->SetOffset(pContext->m_LocalDataOffset);
	unsigned int* pVertexIndices = (unsigned int*)pContext->m_pLocalAllocator->Alloc(sizeof(unsigned int) * NumVerts);
	for(unsigned int i = 0; i < NumVerts; i++)
	{
		CTextReader::ReadUInt(pVertexIndices[i], ppData);
		(*ppData)++; // Skip the ,
	}
	float* pBoneInfluences = (float*)pContext->m_pLocalAllocator->Alloc(sizeof(float) * NumVerts);
	for(unsigned int i = 0; i < NumVerts; i++)
	{

		CTextReader::ReadFloat(pBoneInfluences[i], ppData);
		(*ppData)++; // Skip the ,		
	}	
	
	for(unsigned int i = 0; i < NumVerts; i++)
	{
		pContext->m_RawSkinningData.AddBoneInfluence(pVertexIndices[i], (unsigned char)BoneIndex, (unsigned char)(pBoneInfluences[i] * 255.0f + 0.5f)); // add 0.5 for rounding up
	}
	
	XMMATRIX LocalToBone;
	CTextReader::ReadFloat(LocalToBone._11, ppData);	
	(*ppData)++; // Skip the ,																		
	CTextReader::ReadFloat(LocalToBone._12, ppData);	
	(*ppData)++; // Skip the ,																						
	CTextReader::ReadFloat(LocalToBone._13, ppData);	
	(*ppData)++; // Skip the ,																			
	CTextReader::ReadFloat(LocalToBone._14, ppData);				
	(*ppData)++; // Skip the ,						
	CTextReader::ReadFloat(LocalToBone._21, ppData);	
	(*ppData)++; // Skip the ,																			
	CTextReader::ReadFloat(LocalToBone._22, ppData);	
	(*ppData)++; // Skip the ,																						
	CTextReader::ReadFloat(LocalToBone._23, ppData);	
	(*ppData)++; // Skip the ,																			
	CTextReader::ReadFloat(LocalToBone._24, ppData);				
	(*ppData)++; // Skip the ,						
	CTextReader::ReadFloat(LocalToBone._31, ppData);	
	(*ppData)++; // Skip the ,																			
	CTextReader::ReadFloat(LocalToBone._32, ppData);	
	(*ppData)++; // Skip the ,																						
	CTextReader::ReadFloat(LocalToBone._33, ppData);	
	(*ppData)++; // Skip the ,																			
	CTextReader::ReadFloat(LocalToBone._34, ppData);						
	(*ppData)++; // Skip the ,						
	CTextReader::ReadFloat(LocalToBone._41, ppData);	
	(*ppData)++; // Skip the ,																			
	CTextReader::ReadFloat(LocalToBone._42, ppData);	
	(*ppData)++; // Skip the ,																						
	CTextReader::ReadFloat(LocalToBone._43, ppData);	
	(*ppData)++; // Skip the ,																				
	CTextReader::ReadFloat(LocalToBone._44, ppData);							
	(*ppData)++; // Skip the ;																			
	(*ppData)++; // Skip the ;		
	pContext->SetModelToBoneMatrix(BoneIndex, LocalToBone);
	pContext->m_pLocalAllocator->SetOffset(pContext->m_LocalDataOffset);
}

//-----------------------------------------------------------------------------
void CXFileParser::PreAnimationSet(CXFileParser* pContext)
{
	pContext->m_NumAnimations = 0;
	pContext->m_pLocalAllocator->SetOffset(pContext->m_LocalDataOffset);
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessAnimationSet(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	ppData; // C4100
	char Name[256];
	sprintf_s(Name, 255,"%s.AnimationSet.%s", pContext->m_pName, pBlockName);
	pContext->m_pAnimationSet = &pContext->m_pAnimationSystem->CreateAnimationSet(pContext->m_pStringDictionary->AddString(Name));
}					

//-----------------------------------------------------------------------------
void CXFileParser::PostAnimationSet(CXFileParser* pContext)
{
	pContext->m_pAnimationSet->Create(pContext->m_pAnimationTable, pContext->m_NumAnimations, pContext->m_pAnimationSystem);
	pContext->m_pAnimationSetTable[pContext->m_NumAnimationSets++] = pContext->m_pAnimationSet;
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessAnimation(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // c4100
	ppData;
	pContext->m_pAnimation = new(pContext->m_pLocalAllocator->Alloc(sizeof(CAnimation))) CAnimation;
	pContext->m_pAnimationTable[pContext->m_NumAnimations] = pContext->m_pAnimation;
	pContext->m_NumAnimations++;
}					

//-----------------------------------------------------------------------------
void CXFileParser::PostAnimation(CXFileParser* pContext)
{
	char BoneName[256];
	unsigned long long Key;	
	CTextReader::ReadString(BoneName, &pContext->m_pPrevBlock);
	Key = CHash64::GetHash(BoneName);	
	pContext->m_pAnimation->m_BoneIndex = *pContext->m_pBonesHash->GetValue(Key);
}

//-----------------------------------------------------------------------------
void CXFileParser::ProcessAnimationKey(CXFileParser* pContext, const char** ppData, const char* pBlockName)
{
	pBlockName; // c4100
	int KeyType;
	unsigned int NumKeys;
	CTextReader::ReadInt(KeyType, ppData);
	(*ppData)++; // Skip the ;
	CTextReader::ReadUInt(NumKeys, ppData);
	(*ppData)++; // Skip the ;	
	switch(KeyType)
	{	
		case 0: // Rotation
		{
			pContext->m_pAnimation->m_NumRotationFrames = NumKeys;
			pContext->m_pAnimation->m_pRotations = (XMVECTORF32*)(pContext->m_pLocalAllocator->AlignedAlloc(sizeof(XMVECTORF32) * NumKeys, 16));
			pContext->m_pAnimation->m_pRotationKeys = (unsigned int*)pContext->m_pLocalAllocator->Alloc(sizeof(unsigned int) * NumKeys);
			for(unsigned int KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
			{
				unsigned int UIVal;
				CTextReader::ReadUInt(UIVal, ppData);
				pContext->m_pAnimation->m_pRotationKeys[KeyIndex] = UIVal;
				(*ppData)++; // Skip the ;		
				CTextReader::ReadUInt(UIVal, ppData);	// Number of floats					
				(*ppData)++; // Skip the ;						
				pContext->m_pAnimation->m_pRotations[KeyIndex].v = XMVectorZero();
				CTextReader::ReadFloat(pContext->m_pAnimation->m_pRotations[KeyIndex].f[3], ppData);	
				(*ppData)++; // Skip the ;									
				// Quaternion rotation values are stored negated for some reason.		
				float FVal;								
				CTextReader::ReadFloat(FVal, ppData);	
				pContext->m_pAnimation->m_pRotations[KeyIndex].f[0] = -FVal;
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(FVal, ppData);	
				pContext->m_pAnimation->m_pRotations[KeyIndex].f[1] = -FVal;
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(FVal, ppData);	
				pContext->m_pAnimation->m_pRotations[KeyIndex].f[2] = -FVal;
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ,																															
			}
			break;
		}
		case 1: // Scaling
		{
			pContext->m_pAnimation->m_NumScaleFrames = NumKeys;
			pContext->m_pAnimation->m_pScales = (XMVECTORF32*)pContext->m_pLocalAllocator->AlignedAlloc(sizeof(XMVECTORF32) * NumKeys, 16);
			pContext->m_pAnimation->m_pScaleKeys = (unsigned int*)pContext->m_pLocalAllocator->Alloc(sizeof(unsigned int) * NumKeys);							
			for(unsigned int KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
			{
				unsigned int UIVal;
				CTextReader::ReadUInt(UIVal, ppData);
				pContext->m_pAnimation->m_pScaleKeys[KeyIndex] = UIVal;
				(*ppData)++; // Skip the ;		
				CTextReader::ReadUInt(UIVal, ppData);	// Number of floats					
				(*ppData)++; // Skip the ;						
				pContext->m_pAnimation->m_pScales[KeyIndex].v = XMVectorZero();
				CTextReader::ReadFloat(pContext->m_pAnimation->m_pScales[KeyIndex].f[0], ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(pContext->m_pAnimation->m_pScales[KeyIndex].f[1], ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(pContext->m_pAnimation->m_pScales[KeyIndex].f[2], ppData);	
				pContext->m_pAnimation->m_pScales[KeyIndex].f[3] = 1.0f;
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ,																															
			}												
			break;		
		}
		case 2: // Translation
		{
			pContext->m_pAnimation->m_NumTranslationFrames = NumKeys;	
			pContext->m_pAnimation->m_pTranslations	= (XMVECTORF32*)pContext->m_pLocalAllocator->AlignedAlloc(sizeof(XMVECTORF32) * NumKeys, 16);
			pContext->m_pAnimation->m_pTranslationKeys = (unsigned int*)pContext->m_pLocalAllocator->Alloc(sizeof(unsigned int) * NumKeys);						
			for(unsigned int KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
			{
				unsigned int UIVal;
				CTextReader::ReadUInt(UIVal, ppData);
				pContext->m_pAnimation->m_pTranslationKeys[KeyIndex] = UIVal;
				(*ppData)++; // Skip the ;		
				CTextReader::ReadUInt(UIVal, ppData);	// Number of floats					
				(*ppData)++; // Skip the ;						
				pContext->m_pAnimation->m_pTranslations[KeyIndex].v = XMVectorZero();
				CTextReader::ReadFloat(pContext->m_pAnimation->m_pTranslations[KeyIndex].f[0], ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(pContext->m_pAnimation->m_pTranslations[KeyIndex].f[1], ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(pContext->m_pAnimation->m_pTranslations[KeyIndex].f[2], ppData);	
				pContext->m_pAnimation->m_pTranslations[KeyIndex].f[3] = 1.0f;
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ,																															
			}																												
			break;		
		}		
		case 4: // Matrix
		{
			// Rotation
			pContext->m_pAnimation->m_NumRotationFrames = NumKeys;
			pContext->m_pAnimation->m_pRotations = (XMVECTORF32*)pContext->m_pLocalAllocator->AlignedAlloc(sizeof(XMVECTORF32) * NumKeys, 16);
			pContext->m_pAnimation->m_pRotationKeys = (unsigned int*)pContext->m_pLocalAllocator->Alloc(sizeof(unsigned int) * NumKeys);												
			// Scaling
			pContext->m_pAnimation->m_NumScaleFrames = NumKeys;
			pContext->m_pAnimation->m_pScales = (XMVECTORF32*)pContext->m_pLocalAllocator->AlignedAlloc(sizeof(XMVECTORF32) * NumKeys, 16);
			pContext->m_pAnimation->m_pScaleKeys = (unsigned int*)pContext->m_pLocalAllocator->Alloc(sizeof(unsigned int) * NumKeys);																
			// Translation
			pContext->m_pAnimation->m_NumTranslationFrames = NumKeys;	
			pContext->m_pAnimation->m_pTranslations	= (XMVECTORF32*)pContext->m_pLocalAllocator->AlignedAlloc(sizeof(XMVECTORF32) * NumKeys, 16);
			pContext->m_pAnimation->m_pTranslationKeys = (unsigned int*)pContext->m_pLocalAllocator->Alloc(sizeof(unsigned int) * NumKeys);								
			for(unsigned int KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
			{
				unsigned int UIVal;
				CTextReader::ReadUInt(UIVal, ppData);
				pContext->m_pAnimation->m_pScaleKeys[KeyIndex] = UIVal;			
				pContext->m_pAnimation->m_pRotationKeys[KeyIndex] = UIVal;							
				pContext->m_pAnimation->m_pTranslationKeys[KeyIndex] = UIVal;											
				XMMATRIX M;
				(*ppData)++; // Skip the ;		
				CTextReader::ReadUInt(UIVal, ppData);	// Number of floats					
				(*ppData)++; // Skip the ;						
				CTextReader::ReadFloat(M._11, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._12, ppData);	
				(*ppData)++; // Skip the ;																						
				CTextReader::ReadFloat(M._13, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._14, ppData);				
				(*ppData)++; // Skip the ;						
				CTextReader::ReadFloat(M._21, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._22, ppData);	
				(*ppData)++; // Skip the ;																						
				CTextReader::ReadFloat(M._23, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._24, ppData);				
				(*ppData)++; // Skip the ;						
				CTextReader::ReadFloat(M._31, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._32, ppData);	
				(*ppData)++; // Skip the ;																						
				CTextReader::ReadFloat(M._33, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._34, ppData);							
				(*ppData)++; // Skip the ;						
				CTextReader::ReadFloat(M._41, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._42, ppData);	
				(*ppData)++; // Skip the ;																						
				CTextReader::ReadFloat(M._43, ppData);	
				(*ppData)++; // Skip the ;																			
				CTextReader::ReadFloat(M._44, ppData);							
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ;																			
				(*ppData)++; // Skip the ,																		
				XMMatrixDecompose(	&pContext->m_pAnimation->m_pScales[KeyIndex].v, 
									&pContext->m_pAnimation->m_pRotations[KeyIndex].v, 
									&pContext->m_pAnimation->m_pTranslations[KeyIndex].v, 
									M);		
			}				
			break;		
		}				
	}
}


//-----------------------------------------------------------------------------
void CXFileParser::ReadBone(const char** ppData)
{
	XMMATRIX BoneTransform;
	CTextReader::ReadFloat(BoneTransform._11, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._12, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._13, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._14, ppData);
	(*ppData)++; // Skip the ,	
	CTextReader::ReadFloat(BoneTransform._21, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._22, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._23, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._24, ppData);
	(*ppData)++; // Skip the ,	
	CTextReader::ReadFloat(BoneTransform._31, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._32, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._33, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._34, ppData);
	(*ppData)++; // Skip the ,	
	CTextReader::ReadFloat(BoneTransform._41, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._42, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._43, ppData);
	(*ppData)++; // Skip the ,
	CTextReader::ReadFloat(BoneTransform._44, ppData);
	(*ppData)++; // Skip the;	
	(*ppData)++; // Skip the;				
	unsigned short BoneIndex = m_BoneStack[m_BoneStackSize - 1];
	SetBoneMatrix(BoneIndex, BoneTransform);
}

//-----------------------------------------------------------------------------
void CXFileParser::PostFrame(CXFileParser* pContext)
{
	pContext->m_BoneStackSize--;	
}

//-----------------------------------------------------------------------------
CXFileParser::eBlockType CXFileParser::TypeToEnum(const char* pStr)
{	
	if(pStr[0] == 0)
		return eNone;
	for(unsigned int TokenIndex = 0; TokenIndex < eTotalTokens; TokenIndex++)
	{
		if(IsMatching(pStr, m_Tokens[TokenIndex].m_pTokenName, m_Tokens[TokenIndex].m_NameLength))
		{
			return m_Tokens[TokenIndex].m_Type;
		}	
	}	
	Assert(0);
	return eNone;
}

//-----------------------------------------------------------------------------
CXFileParser::eBlockType CXFileParser::ReadBlock(const char **ppData, unsigned int NestingLevel, const char* pEndData)
{
	const char* pBlockStart = *ppData;
	char BlockType[256];
	char BlockName[256];	
    if(!CTextReader::AdvanceToChar('{', ppData, pEndData)) // Opening bracket
		return eNone;
		
	m_pPrevBlock = *ppData + 1;

	CTextReader::ReadString(BlockType, &pBlockStart, *ppData);
	CTextReader::ReadString(BlockName, &pBlockStart, *ppData);	
	(*ppData)++;
	for(unsigned int i = 0; i < NestingLevel; i++)
	{
//		DebugPrintf(" ");
	}
//	DebugPrintf("%s %s\n", BlockType, BlockName);		
	for(unsigned int i = 0; i < NestingLevel; i++)
	{
//		DebugPrintf(" ");
	}
//	DebugPrintf("{\n");	
	eBlockType OutType = TypeToEnum(BlockType);
		
	if(OutType != eNone)
	{
		if(m_Tokens[OutType].m_pPreProcessor)
		{
			m_Tokens[OutType].m_pPreProcessor(this);
		}
	}
	
	if(OutType != eNone)
	{
		if(m_Tokens[OutType].m_pDataProcessor)
		{
			m_Tokens[OutType].m_pDataProcessor(this, ppData, BlockName);
		}
	}
	
	pBlockStart = *ppData;		
	while(CTextReader::AdvanceToBracket(ppData) != '}') // closing bracket, process any nested blocks inside here.
	{
		ReadBlock(&pBlockStart, NestingLevel + 1, pEndData);
		*ppData = pBlockStart;
	}		
	(*ppData)++;
	for(unsigned int i = 0; i < NestingLevel; i++)
	{
//		DebugPrintf(" ");
	}	
//	DebugPrintf("}\n");		
	
	if(OutType != eNone)
	{
		if(m_Tokens[OutType].m_pPostProcessor)
		{
			m_Tokens[OutType].m_pPostProcessor(this);
		}
	}	
	return OutType;		
}

//-----------------------------------------------------------------------------
void CXFileParser::ConvertToIntermediate()
{
	const unsigned int* pPos;
	const unsigned int* pNrm;
	const unsigned int* pUv;	
	unsigned int NumTris;
	m_RawIndexData.GetIndexData(NumTris, &pPos, &pNrm, &pUv);
	for(unsigned int TriIndex = 0; TriIndex < NumTris; TriIndex++)
	{
		CIntermediateDrawPrimData::STriangle Triangle;
		for(unsigned int VertexIndex = 0; VertexIndex < 3; VertexIndex++)
		{
			Triangle.m_Verts[VertexIndex].m_PosIndex = *pPos++;
			Triangle.m_Verts[VertexIndex].m_NrmIndex = *pNrm++;
			Triangle.m_Verts[VertexIndex].m_UvIndex = *pUv++;			
			Triangle.m_Verts[VertexIndex].m_SGIndex = 0xFFFF;
		}
		m_IntermediateDrawPrimData.AddTriangle(&Triangle, &m_RawVertexData);
	}
}

//-----------------------------------------------------------------------------
void CXFileParser::AddMaterial() 
{
	CTexture* pDiffuseTexture = m_DiffuseTextureName[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, m_DiffuseTextureName) : NULL;
	CTexture* pSpecularTexture = m_SpecularTextureName[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, m_SpecularTextureName) : NULL;
	CTexture* pAlphaTexture = m_AlphaTextureName[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, m_AlphaTextureName) : NULL;
	CTexture* pNormalTexture = m_NormalTextureName[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, m_NormalTextureName) : NULL;
	char ParamName[256];
	GetParamName(ParamName);	
	CSampler* pLinearSampler = &m_pTextureSystem->GetSampler(CTextureSystem::eSamplerLinear);
	CMaterialPhong::CParameters Params(	ParamName, 
										pDiffuseTexture, 
										pSpecularTexture, 
										pAlphaTexture, 
										pNormalTexture, 
										pLinearSampler,
										pLinearSampler,
										pLinearSampler,
										pLinearSampler,
										CMaterialPhong::eSkinned, 
										CMaterialPhong::eLocalPos, 
										m_DiffuseParameters, 
										m_SpecularParameters);		
	m_pMaterial = &m_pMaterialSystem->GetMaterial<CMaterialPhong>(&Params);
}

//-----------------------------------------------------------------------------
void CXFileParser::CreateDrawPrimitive()
{
	ConvertToIntermediate();
	m_IntermediateDrawPrimData.CalculateSmoothingGroups();
	SVertexObj* pVertices; 
	unsigned short VertexCount; 
	const unsigned short* pIndices; 
	unsigned short IndexCount;
	char Name[256];
	char ParamSubName[256];
	char ParamName[256];
	GetParamSubName(ParamSubName);
	GetParamName(ParamName);	

	m_IntermediateDrawPrimData.GetVertexAndIndexData(&pVertices, VertexCount, &pIndices, IndexCount);
	CVertexProcessing::Instance().CreateTangents(pVertices, VertexCount, pIndices, IndexCount);
		
	sprintf_s(Name,"%s.VBuffer.%s", m_pName, ParamSubName);
	CVBuffer& VBuffer = m_pGeometrySystem->CreateVBuffer(Name, sizeof(SVertexObj), pVertices, VertexCount * sizeof(SVertexObj));	
	sprintf_s(Name,"%s.IBuffer.%s", m_pName, ParamSubName);
	CIBuffer& IBuffer = m_pGeometrySystem->CreateIBuffer(Name, pIndices, IndexCount * sizeof(unsigned short));
	CVBuffer* pVBuffer[CDrawPrimitive::scMaxStreams] = { &VBuffer, NULL };
	unsigned int NumStreams = 1;
	
	// Create the skinning stream if one exists
	if(!m_RawSkinningData.IsEmpty())
	{
		m_IntermediateSkinningData.Create(m_IntermediateDrawPrimData.GetSrcIndexData(), pIndices, &m_RawSkinningData, IndexCount, VertexCount);
		SVertexSkinData* pSkinningVertices;
		m_IntermediateSkinningData.GetVertexData(&pSkinningVertices);
		sprintf_s(Name,"%s.SkinVBuffer.%s", m_pName, ParamSubName);
		CVBuffer& VSkinningBuffer = m_pGeometrySystem->CreateVBuffer(Name, sizeof(SVertexSkinData), pSkinningVertices, VertexCount * sizeof(SVertexSkinData));
		pVBuffer[1] = &VSkinningBuffer;
		NumStreams++;
	}	
			
	char MatLibName[256];
	GetLibName(MatLibName);
	sprintf_s(Name,"%s.DrawPrim.%s", m_pName, ParamSubName);																				
	CDrawPrimitive* pDrawPrimitive = &m_pDrawPrimitiveSystem->Add(Name, pVBuffer, NumStreams, &IBuffer, IndexCount, m_pMaterial);
	sprintf_s(Name,"%s.Skeleton.%s", m_pName, ParamSubName);																				
	CSkeleton& Skeleton = m_pSkeletonSystem->Add(Name, m_NumBones, m_BoneMatrices, m_ModelToBoneMatrices, m_NumParentedBones, m_ParentBoneIndex);
	m_pCreator->CreateRenderObject(m_pName, ParamSubName, pDrawPrimitive, &Skeleton, m_pRenderObjectSystem);
}

//-----------------------------------------------------------------------------
void CXFileParser::FinalizeAnimationSets()
{
	char AnimSetTableID[256];
	sprintf_s(AnimSetTableID, 255, "%s.%s", m_pName, CAnimationSetTable::GetTypeName());
	m_pAnimationSystem->CreateAnimationSetTable(AnimSetTableID, m_pAnimationSetTable, m_NumAnimationSets);
}

//-----------------------------------------------------------------------------
void CXFileParser::GetParamName(char* pParamName)
{
	sprintf_s(pParamName, 255, "%s.MatParams_%d", m_pName, m_NumParams);		
}

//-----------------------------------------------------------------------------
void CXFileParser::GetParamSubName(char* pSubName)
{
	sprintf_s(pSubName, 255, "MatParams_%d", m_NumParams);		
}

//-----------------------------------------------------------------------------
void CXFileParser::GetLibName(char* pLibName)
{
	sprintf_s(pLibName, 255, "%s.MaterialLibrary", m_pName);			
}

//-----------------------------------------------------------------------------
void CXFileParser::ParseXFileFromMemory(unsigned char* pMemory, unsigned int Size, const char* pName, CJobSystem::CJob* pJob, unsigned int ThreadID)
{
	m_ThreadID = ThreadID;
	m_pName = pName;
	m_pJob = pJob;
	const char *pStart = (const char*)pMemory; 
	const char *pEnd = &pStart[Size];
	char Str[256];
	CTextReader::ReadString(Str, &pStart); // xof 0303txt 0032
	CTextReader::ReadString(Str, &pStart); 
	CTextReader::ReadString(Str, &pStart);	
	while(pStart < pEnd)
	{
		ReadBlock(&pStart, 0, pEnd);
	}
	FinalizeAnimationSets();		
}

