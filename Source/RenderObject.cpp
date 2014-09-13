#include "stdafx.h"
#include "HeapAllocator.h"
#include "LWMutex.h"
#include "Hash64.h"
#include "StringDictionary.h"
#include "Skeleton.h"
#include "Renderer.h"
#include "DrawPrimitive.h"
#include "ConstantsSystem.h"

#include "RenderObject.h"

static const unsigned int scRenderObjectMemory = 512 * 1024;
static const unsigned int scMaxRenderObjects = 1024;
static const unsigned int scMaxInstancedRenderObjects = 1024;
static const unsigned int scMaxSkeletalRenderObjects = 128;

CRenderObjectSystem::CRenderObjectCreator			CRenderObjectSystem::s_mRenderObjectCreator;
CRenderObjectSystem::CInstancedRenderObjectCreator	CRenderObjectSystem::s_mInstancedRenderObjectCreator;
CRenderObjectSystem::CSkeletalObjectCreator			CRenderObjectSystem::s_mSkeletalObjectCreator;


//--------------------------------------------------------------------------------------
CRenderObject::CRenderObject() : m_pDrawPrimitive(NULL)
{	
	SetState(CRenderObjectState::eUnloaded);
}

//--------------------------------------------------------------------------------------
CRenderObject::CRenderObject(CDrawPrimitive *pDrawPrimitive) : m_pDrawPrimitive(pDrawPrimitive)
{
	SetState(CRenderObjectState::eUnloaded);
}

bool CRenderObject::AllResourcesLoaded() const
{
	return m_pDrawPrimitive->IsLoaded();
}

//--------------------------------------------------------------------------------------
void CRenderObject::Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{
	pConstantsSystem; // C4100
	pRenderer->DrawIndexedPrimitive(m_pDrawPrimitive);
}


//--------------------------------------------------------------------------------------
CInstancedRenderObject::CInstancedRenderObject() : CRenderObject()
{	
}

//--------------------------------------------------------------------------------------
CInstancedRenderObject::CInstancedRenderObject(	CDrawPrimitive* pDrawPrimitive, 
												XMFLOAT4X4* pLocalToWorld, 
												XMFLOAT4X4* pLocalToWorldNormals) : CRenderObject(pDrawPrimitive),
																					 m_LocalToWorld(*pLocalToWorld),
																					 m_LocalToWorldNormals(*pLocalToWorldNormals)
{		
}

//--------------------------------------------------------------------------------------
void CInstancedRenderObject::Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{ 
	XMMATRIX* pLocalToWorld;
	pConstantsSystem->UpdateConstantBufferBegin(CConstantsSystem::eLocalToWorld, (void**)&pLocalToWorld, sizeof(CConstantsSystem::cbLocalToWorld));
	*pLocalToWorld = XMMatrixSet(m_LocalToWorld._11, m_LocalToWorld._12, m_LocalToWorld._13, m_LocalToWorld._14,
		m_LocalToWorld._21, m_LocalToWorld._22, m_LocalToWorld._23, m_LocalToWorld._24,
		m_LocalToWorld._31, m_LocalToWorld._32, m_LocalToWorld._33, m_LocalToWorld._34,
		m_LocalToWorld._41, m_LocalToWorld._42, m_LocalToWorld._43, m_LocalToWorld._44);
	pLocalToWorld++;
	*pLocalToWorld = XMMatrixSet(m_LocalToWorldNormals._11, m_LocalToWorldNormals._12, m_LocalToWorldNormals._13, m_LocalToWorldNormals._14,
		m_LocalToWorldNormals._21, m_LocalToWorldNormals._22, m_LocalToWorldNormals._23, m_LocalToWorldNormals._24,
		m_LocalToWorldNormals._31, m_LocalToWorldNormals._32, m_LocalToWorldNormals._33, m_LocalToWorldNormals._34,
		m_LocalToWorldNormals._41, m_LocalToWorldNormals._42, m_LocalToWorldNormals._43, m_LocalToWorldNormals._44);
	pConstantsSystem->UpdateConstantBufferEnd();
	pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLocalToWorld);
	pRenderer->DrawIndexedPrimitive(m_pDrawPrimitive);

} 

//--------------------------------------------------------------------------------------
void CInstancedRenderObject::SetLocalToWorld(XMMATRIX& LocalToWorld, XMMATRIX& LocalToWorldNormals)
{
	m_LocalToWorld = XMFLOAT4X4(&LocalToWorld._11);
	m_LocalToWorldNormals = XMFLOAT4X4(&LocalToWorldNormals._11);
}

//--------------------------------------------------------------------------------------
CSkeletalRenderObject::CSkeletalRenderObject() : CInstancedRenderObject(),
												 m_pSkeleton(NULL)
{
}

//--------------------------------------------------------------------------------------
CSkeletalRenderObject::CSkeletalRenderObject(	CDrawPrimitive* pDrawPrimitive, 
												XMFLOAT4X4* pLocalToWorld, 
												XMFLOAT4X4* pLocalToWorldNormals,
												CSkeleton* pSkeleton,
												CRenderObjectSystem* pRenderObjectSystem,
												CUtilityDraw* pUtilityDraw) : CInstancedRenderObject(pDrawPrimitive, pLocalToWorld, pLocalToWorldNormals),
																							m_pSkeleton(pSkeleton),
																							m_pRenderObjectSystem(pRenderObjectSystem),
																							m_pUtilityDraw(pUtilityDraw)


										
{
	m_pBoneMatrices = (XMMATRIX*)m_pRenderObjectSystem->GetAllocator()->AlignedAlloc(pSkeleton->GetNumBones() * sizeof(XMMATRIX), 16);
	for(unsigned int BoneIndex = 0; BoneIndex < pSkeleton->GetNumBones(); BoneIndex++)
	{
		m_pBoneMatrices[BoneIndex] = XMMatrixIdentity();
	}
}

//--------------------------------------------------------------------------------------
CSkeletalRenderObject::~CSkeletalRenderObject()
{
	m_pRenderObjectSystem->GetAllocator()->Free(m_pBoneMatrices);
}

//--------------------------------------------------------------------------------------
XMMATRIX* CSkeletalRenderObject::GetMatrixPtr()		
{
	return m_pBoneMatrices;
}

//--------------------------------------------------------------------------------------
void CSkeletalRenderObject::Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{
#if 0
//  Test code to draw the skeleton
	pRenderer; // C4100
	pConstantsSystem;
	XMMATRIX LocalToWorld =	XMMatrixSet(m_LocalToWorld._11, m_LocalToWorld._12, m_LocalToWorld._13, m_LocalToWorld._14,
										m_LocalToWorld._21, m_LocalToWorld._22, m_LocalToWorld._23, m_LocalToWorld._24,
										m_LocalToWorld._31, m_LocalToWorld._32, m_LocalToWorld._33, m_LocalToWorld._34,
										m_LocalToWorld._41, m_LocalToWorld._42, m_LocalToWorld._43, m_LocalToWorld._44);
	m_pSkeleton->Draw(m_pBoneMatrices, &LocalToWorld, m_pUtilityDraw);
	return;
#else
	XMMATRIX* pLocalToWorld;
	pConstantsSystem->UpdateConstantBufferBegin(CConstantsSystem::eLocalToWorld, (void**)&pLocalToWorld, sizeof(CConstantsSystem::cbLocalToWorld));
	*pLocalToWorld = XMMatrixSet(	m_LocalToWorld._11, m_LocalToWorld._12, m_LocalToWorld._13, m_LocalToWorld._14,
									m_LocalToWorld._21, m_LocalToWorld._22, m_LocalToWorld._23, m_LocalToWorld._24,
									m_LocalToWorld._31, m_LocalToWorld._32, m_LocalToWorld._33, m_LocalToWorld._34,
									m_LocalToWorld._41, m_LocalToWorld._42, m_LocalToWorld._43, m_LocalToWorld._44);
	pLocalToWorld++;
	*pLocalToWorld = XMMatrixSet(	m_LocalToWorldNormals._11, m_LocalToWorldNormals._12, m_LocalToWorldNormals._13, m_LocalToWorldNormals._14,
									m_LocalToWorldNormals._21, m_LocalToWorldNormals._22, m_LocalToWorldNormals._23, m_LocalToWorldNormals._24,
									m_LocalToWorldNormals._31, m_LocalToWorldNormals._32, m_LocalToWorldNormals._33, m_LocalToWorldNormals._34,
									m_LocalToWorldNormals._41, m_LocalToWorldNormals._42, m_LocalToWorldNormals._43, m_LocalToWorldNormals._44);	
	pConstantsSystem->UpdateConstantBufferEnd();
	pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLocalToWorld);
	
	m_pSkeleton->CalculateMatrixPalette(m_pBoneMatrices, pConstantsSystem); // This will also set the bones in the constant buffer	
	pRenderer->DrawIndexedPrimitive(m_pDrawPrimitive);
#endif
}

//--------------------------------------------------------------------------------------
CRenderObjectSystem::CRenderObjectSystem(CStringDictionary** ppStringDictionary, CUtilityDraw** ppUtilityDraw) : m_RenderObjects(&m_Mutex),
																												m_InstancedRenderObjects(&m_Mutex),
																												m_SkeletalRenderObjects(&m_Mutex),
																												m_rStringDictionary(ppStringDictionary),
																												m_rUtilityDraw(ppUtilityDraw)
{
	m_pHeapAllocator = new CHeapAllocator(scRenderObjectMemory, false);	
	m_RenderObjects.Startup(m_pHeapAllocator, 1, scMaxRenderObjects);			
	m_InstancedRenderObjects.Startup(m_pHeapAllocator, 1, scMaxInstancedRenderObjects);
	m_SkeletalRenderObjects.Startup(m_pHeapAllocator, 1, scMaxSkeletalRenderObjects);	
}

//--------------------------------------------------------------------------------------
CRenderObjectSystem::~CRenderObjectSystem()
{
}

//--------------------------------------------------------------------------------------
IAllocator* CRenderObjectSystem::GetAllocator()
{
	return m_pHeapAllocator;
}

//--------------------------------------------------------------------------------------		
CSkeletalRenderObject& CRenderObjectSystem::GetSkeletalRenderObjectRef(const char* pName)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	return m_SkeletalRenderObjects.AddReference(Key);		
}

//--------------------------------------------------------------------------------------		
CInstancedRenderObject& CRenderObjectSystem::GetInstancedRenderObjectRef(const char* pName)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	return m_InstancedRenderObjects.AddReference(Key);		
}

//--------------------------------------------------------------------------------------		
CRenderObject& CRenderObjectSystem::GetRenderObjectRef(const char* pName)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	return m_RenderObjects.AddReference(Key);		
}

//--------------------------------------------------------------------------------------		
void CRenderObjectSystem::CreateRenderObject(const char* pName, CDrawPrimitive* pDrawPrimitive)
{		
	// thread safe	
	m_Mutex.Acquire();
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	bool Exists = false;	
	CRenderObject& RenderObject = m_RenderObjects.AddAsset(Key, Exists);	
	if ((!Exists) || RenderObject.IsReference())
	{			
		new(&RenderObject) CRenderObject(pDrawPrimitive);	
	}
	m_Mutex.Release();
}

//--------------------------------------------------------------------------------------		
void CRenderObjectSystem::CreateInstancedRenderObject(const char* pName, CDrawPrimitive* pDrawPrimitive, XMFLOAT4X4* pLocalToWorld, XMFLOAT4X4* pLocalToWorldNormals)
{
	// thread safe	
	m_Mutex.Acquire();
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	bool Exists = false;
	Assert(pLocalToWorldNormals);
	CRenderObject& RenderObject = m_InstancedRenderObjects.AddAsset(Key, Exists);
	if ((!Exists) || RenderObject.IsReference())
	{
		new(&RenderObject) CInstancedRenderObject(pDrawPrimitive, pLocalToWorld, pLocalToWorldNormals);
	}
	m_Mutex.Release();
}

//--------------------------------------------------------------------------------------		
void CRenderObjectSystem::CreateSkeletalRenderObject(const char* pName, CDrawPrimitive* pDrawPrimitive, XMFLOAT4X4* pLocalToWorld, XMFLOAT4X4* pLocalToWorldNormals, CSkeleton* pSkeleton)
{
	// thread safe	
	m_Mutex.Acquire();
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	bool Exists = false;
	CRenderObject& RenderObject = m_SkeletalRenderObjects.AddAsset(Key, Exists);
	if ((!Exists) || RenderObject.IsReference())
	{
		new(&RenderObject) CSkeletalRenderObject(pDrawPrimitive, pLocalToWorld, pLocalToWorldNormals, pSkeleton, this, *m_rUtilityDraw);
	}
	m_Mutex.Release();
}

//--------------------------------------------------------------------------------------		
void CRenderObjectSystem::Tick(float DeltaSec)
{
	DeltaSec;
	m_RenderObjects.Tick();			
	m_InstancedRenderObjects.Tick();					
	m_SkeletalRenderObjects.Tick();		
}

//--------------------------------------------------------------------------------------		
void CRenderObjectSystem::DrawAllRenderObjects(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{
	m_RenderObjects.Draw(pRenderer, pConstantsSystem);
}

//--------------------------------------------------------------------------------------		
template<>
void CMultiStageAssetContainer<CRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference>::Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{
	for (CElementEntry* pElement = m_pStages[CRenderObjectState::eLoaded]->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
	{
		CRenderObject* pRenderObject = (CRenderObject*)pElement->m_pData;
		pRenderObject->Draw(pRenderer, pConstantsSystem);
	}				
}
 
//--------------------------------------------------------------------------------------		
template<>
void CMultiStageAssetContainer<CRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference>::Tick()
{
	// Render objects could be added to the list from other threads
	m_pAddAssetMutex->Acquire();

	for (CElementEntry* pElement = m_pStages[CRenderObjectState::eReference]->GetFirst(); pElement != NULL;)
	{
		CRenderObject* pRenderObject = (CRenderObject*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pRenderObject->GetState() == CRenderObjectState::eUnloaded)
		{
			pRenderObject->SetState(CRenderObjectState::eUnloaded);
			m_pStages[CRenderObjectState::eReference]->Move(m_pStages[CRenderObjectState::eUnloaded], *pElement);
		}
		pElement = pNextElement;
	}

	for (CElementEntry* pElement = m_pStages[CRenderObjectState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CRenderObject* pRenderObject = (CRenderObject*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if(pRenderObject->AllResourcesLoaded())
		{
			pRenderObject->SetState(CRenderObjectState::eLoaded);
			m_pStages[CRenderObjectState::eUnloaded]->Move(m_pStages[CRenderObjectState::eLoaded], *pElement);	
		}
		pElement = pNextElement;
	}				
	m_pAddAssetMutex->Release();
}

//--------------------------------------------------------------------------------------		
template<>
void CMultiStageAssetContainer<CInstancedRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference>::Tick()
{
	// Render objects could be added to the list from other threads
	m_pAddAssetMutex->Acquire();

	for (CElementEntry* pElement = m_pStages[CRenderObjectState::eReference]->GetFirst(); pElement != NULL;)
	{
		CInstancedRenderObject* pRenderObject = (CInstancedRenderObject*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pRenderObject->GetState() == CRenderObjectState::eUnloaded)
		{
			pRenderObject->SetState(CRenderObjectState::eUnloaded);
			m_pStages[CRenderObjectState::eReference]->Move(m_pStages[CRenderObjectState::eUnloaded], *pElement);
		}
		pElement = pNextElement;
	}

	for (CElementEntry* pElement = m_pStages[CRenderObjectState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CInstancedRenderObject* pRenderObject = (CInstancedRenderObject*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if(pRenderObject->AllResourcesLoaded())
		{
			pRenderObject->SetState(CRenderObjectState::eLoaded);
			m_pStages[CRenderObjectState::eUnloaded]->Move(m_pStages[CRenderObjectState::eLoaded], *pElement);
		}
		pElement = pNextElement;
	}				
	m_pAddAssetMutex->Release();
}

//--------------------------------------------------------------------------------------		
template<>
void CMultiStageAssetContainer<CSkeletalRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference>::Tick()
{
	// Render objects could be added to the list from other threads
	m_pAddAssetMutex->Acquire();

	for (CElementEntry* pElement = m_pStages[CRenderObjectState::eReference]->GetFirst(); pElement != NULL;)
	{
		CSkeletalRenderObject* pRenderObject = (CSkeletalRenderObject*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pRenderObject->GetState() == CRenderObjectState::eUnloaded)
		{
			pRenderObject->SetState(CRenderObjectState::eUnloaded);
			m_pStages[CRenderObjectState::eReference]->Move(m_pStages[CRenderObjectState::eUnloaded], *pElement);
		}
		pElement = pNextElement;
	}

	for(CElementEntry* pElement = m_pStages[CRenderObjectState::eUnloaded]->GetFirst(); pElement != NULL; )
	{
		CSkeletalRenderObject* pRenderObject = (CSkeletalRenderObject*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if(pRenderObject->AllResourcesLoaded())
		{
			pRenderObject->SetState(CRenderObjectState::eLoaded);
			m_pStages[CRenderObjectState::eUnloaded]->Move(m_pStages[CRenderObjectState::eLoaded], *pElement);	
		}
		pElement = pNextElement;
	}				
	m_pAddAssetMutex->Release();
}
