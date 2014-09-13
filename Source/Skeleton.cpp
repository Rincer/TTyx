#include "stdafx.h"
#include "Macros.h"
#include "UtilityDraw.h"
#include "MemoryManager.h"
#include "Hash64.h"
#include "StringDictionary.h"

#include "Skeleton.h"

//---------------------------------------------------------------------
CSkeleton::CSkeleton() : m_pSkeletonSystem(NULL)
{

}


//---------------------------------------------------------------------
CSkeleton::CSkeleton(CSkeletonSystem* pSkeletonSystem) : m_pSkeletonSystem(pSkeletonSystem)																							
{
	m_NumBones = 0;	
	m_NumParentedBones = 0;
	m_pBoneMatrices = NULL; 
	m_pModelToBoneMatrices = NULL; 
	m_pParentBoneIndex = NULL;
	SetState(CSkeletonState::eUnloaded);
}

//---------------------------------------------------------------------
CSkeleton::~CSkeleton()
{
	if (m_pSkeletonSystem)
	{
		m_pSkeletonSystem->Release(*this);
	}
	m_NumBones = 0;	
	m_NumParentedBones = 0;
	m_pBoneMatrices = NULL; 
	m_pModelToBoneMatrices = NULL; 
	m_pParentBoneIndex = NULL;	
}
		
//---------------------------------------------------------------------
CSkeleton& CSkeleton::operator = (const CSkeleton& Rhs)
{
	if(this != &Rhs)
	{
		m_NumBones = Rhs.m_NumBones;
		m_NumParentedBones = Rhs.m_NumParentedBones;
		memcpy(m_pBoneMatrices, Rhs.m_pBoneMatrices, sizeof(XMMATRIX) * m_NumBones);
		memcpy(m_pModelToBoneMatrices, Rhs.m_pModelToBoneMatrices, sizeof(XMMATRIX) * m_NumBones);
		for(unsigned int BoneIndex = 0; BoneIndex < m_NumBones; BoneIndex++)
		{
			m_pParentBoneIndex[BoneIndex] = Rhs.m_pParentBoneIndex[BoneIndex];
		}
	}
	return *this;
}

//---------------------------------------------------------------------
CSkeleton::CSkeleton(const CSkeleton& Rhs)
{
	*this = Rhs;
}
		
//---------------------------------------------------------------------
unsigned int CSkeleton::GetNumBones() const
{
	return m_NumBones;
}

//---------------------------------------------------------------------
void CSkeleton::CalculateMatrixPalette(XMMATRIX* pMatrices, CConstantsSystem* pConstantsSystem)
{
	XMMATRIX Matrices[scMaxBones]; // Temp storage
	Matrices[0] = XMMatrixIdentity(); // Matrix 0 is always identity for unskinned verts
	for(unsigned short BoneIndex = 0; BoneIndex < m_NumBones; BoneIndex++)
	{
		unsigned short ParentIndex = m_pParentBoneIndex[BoneIndex];
		if(ParentIndex == 0xFFFF)	// No parents
		{
			Matrices[BoneIndex + 1] = pMatrices[BoneIndex]; // +1 because bone 0 is identity
		}
		else
		{
			Matrices[BoneIndex + 1] = XMMatrixMultiply(pMatrices[BoneIndex], Matrices[ParentIndex + 1]);
		}
	}
	
	void* pMatrixPalette;
	pConstantsSystem->UpdateConstantBufferBegin(CConstantsSystem::eBones, &pMatrixPalette, (m_NumBones + 1) * 48 + 16); // 3x4 matrices + 16 byte for the last matrix so it can be transposed in place
	XMMATRIX* pMatrix = (XMMATRIX*)pMatrixPalette;
	*pMatrix = Matrices[0];						// Identity
	*pMatrix = XMMatrixTranspose(*pMatrix);
	pMatrix = (XMMATRIX*)&pMatrix->r[3];
	for(unsigned short BoneIndex = 1; BoneIndex < m_NumBones + 1; BoneIndex++)	
	{
		*pMatrix = XMMatrixMultiply(m_pModelToBoneMatrices[BoneIndex - 1], Matrices[BoneIndex]); // Add model to bone transform
		*pMatrix = XMMatrixTranspose(*pMatrix);	// Transpose to row major, to conform with shader skinning code
		pMatrix = (XMMATRIX*)&pMatrix->r[3];	// Only advance by 3 xmvectors since we store 3x4 matrices
	}	
	pConstantsSystem->UpdateConstantBufferEnd();
	pConstantsSystem->SetConstantBuffer(CConstantsSystem::eBones);
}


//---------------------------------------------------------------------
void CSkeleton::Draw(XMMATRIX* pMatrices, XMMATRIX* pLocalToWorld, CUtilityDraw* pUtilityDraw)
{
	XMMATRIX Matrices[scMaxBones];
	for(unsigned short BoneIndex = 0; BoneIndex < m_NumBones; BoneIndex++)
	{
		unsigned short ParentIndex = m_pParentBoneIndex[BoneIndex];
		if(ParentIndex == 0xFFFF)	// No parents
		{
			Matrices[BoneIndex] = pMatrices[BoneIndex]; 
		}
		else
		{
			Matrices[BoneIndex] = XMMatrixMultiply(pMatrices[BoneIndex], Matrices[ParentIndex]);
		}
	}	

	XMMATRIX Scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);	
	for(unsigned short BoneIndex = 0; BoneIndex < m_NumBones; BoneIndex++)
	{
		Matrices[BoneIndex] = XMMatrixMultiply(Matrices[BoneIndex], *pLocalToWorld);
		XMMATRIX Mat = XMMatrixMultiply(Scale, Matrices[BoneIndex]);
		pUtilityDraw->DrawCube(Mat, pUtilityDraw->GetPlainColorMaterial());
	}		
	if(m_NumParentedBones > 0)
	{
		XMMATRIX Mat = XMMatrixIdentity();
		pUtilityDraw->BeginLineList();
		for(unsigned short BoneIndex = 0; BoneIndex < m_NumBones; BoneIndex++)
		{	
			unsigned short ParentIndex = m_pParentBoneIndex[BoneIndex];
			if (ParentIndex != 0xFFFF)
			{

				CColor Color(0xff, 0xff, 0xff, 0);
				XMFLOAT3 Start(Matrices[BoneIndex]._41,Matrices[BoneIndex]._42,Matrices[BoneIndex]._43);
				XMFLOAT3 End(Matrices[ParentIndex]._41,Matrices[ParentIndex]._42,Matrices[ParentIndex]._43);
				pUtilityDraw->AddLine(Start, End, Color, Color);
			}
		}
		pUtilityDraw->EndLineList(Mat, pUtilityDraw->GetPlainColorMaterial(), CRenderer::eDepthLE, CRenderer::eBlendNone);
	}
}


//---------------------------------------------------------------------
CSkeletonSystem::CSkeletonSystem(CStringDictionary** ppStringDictionary) :	m_Skeletons(&m_Mutex),
																			m_rStringDictionary(ppStringDictionary)
{
	m_pAllocator = &CMemoryManager::GetAllocator();
	m_Skeletons.Startup(NULL, 1, scSkeletonsPerSegment);	
}

//---------------------------------------------------------------------
CSkeletonSystem::~CSkeletonSystem()
{

}

//---------------------------------------------------------------------		
CSkeleton& CSkeletonSystem::Add(const char* pName, unsigned int NumBones, XMMATRIX* pBoneMatrices, XMMATRIX* pModelToBoneMatrices, unsigned int NumParentedBones, unsigned short* pParentBoneIndex)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	bool Exists = false;
	
	// can be called from multiple threads	
	m_Mutex.Acquire();
	CSkeleton& Skeleton = m_Skeletons.AddAsset(Key, Exists);
	if(!Exists)
	{
		new(&Skeleton) CSkeleton(this);
		Skeleton.m_NumBones = NumBones;	
		Skeleton.m_NumParentedBones = NumParentedBones; 
		Skeleton.m_pBoneMatrices = (XMMATRIX*)m_pAllocator->AlignedAlloc(sizeof(XMMATRIX) * NumBones, 16);
		memcpy(Skeleton.m_pBoneMatrices, pBoneMatrices, sizeof(XMMATRIX) * NumBones);
		Skeleton.m_pModelToBoneMatrices = (XMMATRIX*)m_pAllocator->AlignedAlloc(sizeof(XMMATRIX) * NumBones, 16);	
		memcpy(Skeleton.m_pModelToBoneMatrices, pModelToBoneMatrices, sizeof(XMMATRIX) * NumBones);
		Skeleton.m_pParentBoneIndex = (unsigned short*)m_pAllocator->Alloc(sizeof(unsigned short) * NumBones);	
		memcpy(Skeleton.m_pParentBoneIndex, pParentBoneIndex, sizeof(unsigned short) * NumBones);
	}	
	m_Mutex.Release();
	return Skeleton;	
}

//---------------------------------------------------------------------		
void CSkeletonSystem::Release(CSkeleton& Skeleton)
{
	if(Skeleton.GetNumBones() > 0)
	{
		m_pAllocator->Free(Skeleton.m_pBoneMatrices);
		m_pAllocator->Free(Skeleton.m_pModelToBoneMatrices);	
		m_pAllocator->Free(Skeleton.m_pParentBoneIndex);
	}
}

//-----------------------------------------------------------------------------------				
template<>
void CMultiStageAssetContainer<CSkeleton, CSkeletonState::eMaxStages, CSkeletonState::eStateType, CSkeletonState::eUnloaded, CSkeletonState::eReference>::Tick()
{
	m_pAddAssetMutex->Acquire();
	for(CElementEntry* pElement = m_pStages[CSkeletonState::eUnloaded]->GetFirst(); pElement != NULL; )
	{
		CSkeleton* pSkeleton = (CSkeleton*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		pSkeleton->SetState(CSkeletonState::eLoaded);
		m_pStages[CSkeletonState::eUnloaded]->Move(m_pStages[CSkeletonState::eLoaded], *pElement);	
		pElement = pNextElement;
	}					
	m_pAddAssetMutex->Release();
}
