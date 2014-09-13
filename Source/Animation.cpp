#include "stdafx.h"
#include "HeapAllocator.h"
#include "MemoryManager.h"
#include "StringDictionary.h"

#include "Animation.h"

static const unsigned int scAnimationSystemMemory = 1024 * 1024;


//-----------------------------------------------------------------------------------
CAnimation::CAnimation()
{
	m_pTranslations = NULL;					
	m_pRotations = NULL;		
	m_pScales = NULL;				
	m_pTranslationKeys = NULL;
	m_pRotationKeys = NULL;		
	m_pScaleKeys = NULL;						
	m_NumTranslationFrames = 0;		
	m_NumRotationFrames = 0;				
	m_NumScaleFrames = 0;	
	m_BoneIndex = 0xFFFF;					
}

//-----------------------------------------------------------------------------------
void CAnimation::UpdateState(CAnimationSet::CAnimationState* pAnimationState, unsigned int AnimationIndex, XMMATRIX* pMatrices)
{
	unsigned int TimeOffset;
	// Wrapped around
	unsigned short ScaleIndex;	
	if(pAnimationState->m_KeyTime >= m_pScaleKeys[m_NumScaleFrames - 1])
	{
		TimeOffset = pAnimationState->m_KeyTime % m_pScaleKeys[m_NumScaleFrames - 1];
		pAnimationState->m_ScaleKey[AnimationIndex] = 0;
	}
	else
	{
		TimeOffset = pAnimationState->m_KeyTime;
	}
		
	for(ScaleIndex = pAnimationState->m_ScaleKey[AnimationIndex]; ScaleIndex < m_NumScaleFrames; ScaleIndex++)
	{
		if(TimeOffset < m_pScaleKeys[ScaleIndex])
			break;
	}		
	
	float t = (float)(TimeOffset - m_pScaleKeys[ScaleIndex - 1]) / (m_pScaleKeys[ScaleIndex] - m_pScaleKeys[ScaleIndex - 1]);
	XMVECTOR Scale = XMVectorLerp(m_pScales[ScaleIndex - 1].v, m_pScales[ScaleIndex].v, t);
	
	// Wrapped around
	unsigned short RotationIndex;	
	if(pAnimationState->m_KeyTime >= m_pRotationKeys[m_NumRotationFrames - 1])
	{
		TimeOffset = pAnimationState->m_KeyTime % m_pRotationKeys[m_NumRotationFrames - 1];
		pAnimationState->m_RotationKey[AnimationIndex] = 0;
	}
	else
	{
		TimeOffset = pAnimationState->m_KeyTime;
	}
		
	for(RotationIndex = pAnimationState->m_RotationKey[AnimationIndex]; RotationIndex < m_NumRotationFrames; RotationIndex++)
	{
		if(TimeOffset < m_pRotationKeys[RotationIndex])
			break;
	}		
	
	t = (float)(TimeOffset - m_pRotationKeys[RotationIndex - 1]) / (m_pRotationKeys[RotationIndex] - m_pRotationKeys[RotationIndex - 1]);
	XMVECTOR Rotation = XMQuaternionSlerp(m_pRotations[RotationIndex - 1].v, m_pRotations[RotationIndex].v, t);
		
	// Wrapped around
	unsigned short TranslationIndex;	
	if(pAnimationState->m_KeyTime >= m_pTranslationKeys[m_NumTranslationFrames - 1])
	{
		TimeOffset = pAnimationState->m_KeyTime % m_pTranslationKeys[m_NumTranslationFrames - 1];
		pAnimationState->m_TranslationKey[AnimationIndex] = 0;
	}
	else
	{
		TimeOffset = pAnimationState->m_KeyTime;
	}
		
	for(TranslationIndex = pAnimationState->m_TranslationKey[AnimationIndex]; TranslationIndex < m_NumTranslationFrames; TranslationIndex++)
	{
		if(TimeOffset < m_pTranslationKeys[TranslationIndex])
			break;
	}		
	t = (float)(TimeOffset - m_pTranslationKeys[TranslationIndex - 1]) / (m_pTranslationKeys[TranslationIndex] - m_pTranslationKeys[TranslationIndex - 1]);
	XMVECTOR Translation = XMQuaternionSlerp(m_pTranslations[TranslationIndex - 1].v, m_pTranslations[TranslationIndex].v, t);
	XMMATRIX ScaleM = XMMatrixScalingFromVector(Scale);
	XMMATRIX RotationM = XMMatrixRotationQuaternion(Rotation);
	XMMATRIX TranslationM = XMMatrixTranslationFromVector(Translation);
	pMatrices[m_BoneIndex] = XMMatrixMultiply(XMMatrixMultiply(ScaleM, RotationM), TranslationM);	
}

//-----------------------------------------------------------------------------------
CAnimationSet::CAnimationState::CAnimationState()
{
	Reset();
}

//-----------------------------------------------------------------------------------
void CAnimationSet::CAnimationState::Advance(float DeltaSec)
{
	m_DeltaSec += DeltaSec;
	unsigned int UnitsToAdvance = (unsigned int)(m_DeltaSec * m_ClockToKey);
	if(UnitsToAdvance)
	{
		m_KeyTime += UnitsToAdvance;
		m_DeltaSec = 0.0f;
	}
}

//-----------------------------------------------------------------------------------
void CAnimationSet::CAnimationState::Reset()
{
	m_ClockToKey = 4800.0f;
	for(unsigned int KeyIndex = 0; KeyIndex < CSkeleton::scMaxBones; KeyIndex++)
	{
		m_ScaleKey[KeyIndex] = 0;
		m_RotationKey[KeyIndex] = 0;				
		m_TranslationKey[KeyIndex] = 0;		
	}
	m_DeltaSec = 0.0f;											
	m_KeyTime = 0;									
}

//-----------------------------------------------------------------------------------
CAnimationSet::CAnimationSet()
{
	m_NumAnimations = 0;				
	m_pAnimations = NULL;
	SetState(CAnimationSetState::eUnloaded);
}	
	
//-----------------------------------------------------------------------------------		
void CAnimationSet::Create(CAnimation**  ppAnimationTable, unsigned int NumAnimations, CAnimationSystem* pAnimationSystem)
{
	m_NumAnimations = NumAnimations;	
	m_pAnimations = PlacementNew<CAnimation>(pAnimationSystem->m_pHeapAllocator->Alloc(m_NumAnimations * sizeof(CAnimation)), m_NumAnimations);
	for(unsigned int AnimationIndex = 0; AnimationIndex < NumAnimations; AnimationIndex++)
	{
		CAnimation *pSrc = ppAnimationTable[AnimationIndex];
		CAnimation *pDst = &m_pAnimations[AnimationIndex];
		
		pDst->m_NumScaleFrames = pSrc->m_NumScaleFrames;
		pDst->m_NumRotationFrames = pSrc->m_NumRotationFrames;		
		pDst->m_NumTranslationFrames = pSrc->m_NumTranslationFrames;
			
		pDst->m_pScales = (XMVECTORF32*)pAnimationSystem->m_pHeapAllocator->AlignedAlloc(pDst->m_NumScaleFrames * sizeof(XMVECTORF32), 16);
		pDst->m_pRotations = (XMVECTORF32*)pAnimationSystem->m_pHeapAllocator->AlignedAlloc(pDst->m_NumRotationFrames * sizeof(XMVECTORF32), 16);
		pDst->m_pTranslations = (XMVECTORF32*)pAnimationSystem->m_pHeapAllocator->AlignedAlloc(pDst->m_NumTranslationFrames * sizeof(XMVECTORF32), 16);
	
		pDst->m_pScaleKeys = (unsigned int*)pAnimationSystem->m_pHeapAllocator->Alloc(pDst->m_NumScaleFrames * sizeof(unsigned int));
		pDst->m_pRotationKeys = (unsigned int*)pAnimationSystem->m_pHeapAllocator->Alloc(pDst->m_NumRotationFrames * sizeof(unsigned int));
		pDst->m_pTranslationKeys = (unsigned int*)pAnimationSystem->m_pHeapAllocator->Alloc(pDst->m_NumTranslationFrames * sizeof(unsigned int));
		
		memcpy(pDst->m_pScales, pSrc->m_pScales, pDst->m_NumScaleFrames * sizeof(XMVECTORF32));									
		memcpy(pDst->m_pRotations, pSrc->m_pRotations, pDst->m_NumRotationFrames * sizeof(XMVECTORF32));									
		memcpy(pDst->m_pTranslations, pSrc->m_pTranslations, pDst->m_NumTranslationFrames * sizeof(XMVECTORF32));													
		
		memcpy(pDst->m_pScaleKeys, pSrc->m_pScaleKeys, pDst->m_NumScaleFrames * sizeof(unsigned int));									
		memcpy(pDst->m_pRotationKeys, pSrc->m_pRotationKeys, pDst->m_NumRotationFrames * sizeof(unsigned int));									
		memcpy(pDst->m_pTranslationKeys, pSrc->m_pTranslationKeys, pDst->m_NumTranslationFrames * sizeof(unsigned int));	
		
		pDst->m_BoneIndex = pSrc->m_BoneIndex;														
	}
}

//-----------------------------------------------------------------------------------
void CAnimationSet::Apply(CAnimationState& AnimationState, XMMATRIX* pMatrices)
{	
	for(unsigned short AnimationIndex = 0; AnimationIndex < m_NumAnimations; AnimationIndex++)
	{
		m_pAnimations[AnimationIndex].UpdateState(&AnimationState, AnimationIndex, pMatrices);
	}
}


//-----------------------------------------------------------------------------------		
CAnimationSystem::CAnimationSystem(CStringDictionary** ppStringDictionary) :m_AnimationSets(&m_SetMutex),
																			m_AnimationSetTables(&m_SetTableMutex),
																			m_rStringDictionary(ppStringDictionary)
{
	m_pHeapAllocator = new CHeapAllocator(scAnimationSystemMemory, false);
	m_AnimationSets.Startup(m_pHeapAllocator, 1, CAnimationSet::scMaxAnimationSets);		
	m_AnimationSetTables.Startup(m_pHeapAllocator, 1, CAnimationSet::scMaxAnimationSetTables);				
}

//-----------------------------------------------------------------------------------				
CAnimationSystem::~CAnimationSystem()
{
	delete m_pHeapAllocator;
}

//-----------------------------------------------------------------------------------				
CAnimationSet& CAnimationSystem::CreateAnimationSet(unsigned long long Key)
{
	m_SetMutex.Acquire(); //Thread safe
	bool Exists = false;
	CAnimationSet& AnimationSet = m_AnimationSets.AddAsset(Key, Exists);
	m_SetMutex.Release();
	return AnimationSet;
}

//-----------------------------------------------------------------------------------				
CAnimationSetTable::CAnimationSetTable()
{
	SetState(CAnimationSetTableState::eUnloaded);
}

//-----------------------------------------------------------------------------------				
CAnimationSetTable::CAnimationSetTable(CAnimationSet**	ppAnimationSets, unsigned int NumAnimationSets, CAnimationSystem* pAnimationSystem)
{
	m_ppAnimationSets = (CAnimationSet**)pAnimationSystem->m_pHeapAllocator->Alloc(sizeof(CAnimationSet*)* NumAnimationSets);
	m_NumAnimationSets = NumAnimationSets;
	for(unsigned int SetIndex = 0; SetIndex < m_NumAnimationSets; SetIndex++)
	{
		m_ppAnimationSets[SetIndex] = ppAnimationSets[SetIndex];
	}
	SetState(CAnimationSetTableState::eUnloaded);
}


//-----------------------------------------------------------------------------------				
CAnimationController::CAnimationController() :	m_pAnimationSetTable(NULL),
												m_CurrentSet(0)
{
}

//-----------------------------------------------------------------------------------				
void CAnimationController::Create(const char* pName, CAnimationSystem* pAnimationSystem)
{
	m_pAnimationSetTable = &pAnimationSystem->GetAnimationSetTableRef(pName);
}

//-----------------------------------------------------------------------------------				
void CAnimationController::Tick(float DeltaSec)
{
	m_AnimationState.Advance(DeltaSec);	
}

//-----------------------------------------------------------------------------------				
void CAnimationController::Apply(XMMATRIX* pBoneMatrices)
{
	m_pAnimationSetTable->m_ppAnimationSets[m_CurrentSet]->Apply(m_AnimationState, pBoneMatrices);	
}

//-----------------------------------------------------------------------------------				
bool CAnimationController::IsLoaded() const
{
	return m_pAnimationSetTable->IsLoaded();
}

//-----------------------------------------------------------------------------------						
void CAnimationSystem::Tick(float DeltaSec)
{
	m_AnimationSets.Tick();		
	m_AnimationSetTables.Tick();	
	DeltaSec; // To quiet warnings
}

//-----------------------------------------------------------------------------------				
CAnimationSetTable& CAnimationSystem::GetAnimationSetTableRef(const char* pName)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);	
	return m_AnimationSetTables.AddReference(Key);			
}

//-----------------------------------------------------------------------------------				
CAnimationSetTable& CAnimationSystem::CreateAnimationSetTable(const char* pName, CAnimationSet** ppAnimationSets, unsigned int NumAnimationSets)
{
	m_SetTableMutex.Acquire(); //Thread safe
	bool Exists = false;
	CAnimationSetTable& AnimationSetTable = m_AnimationSetTables.AddAsset(m_rStringDictionary->AddString(pName), Exists);
	if(!Exists || AnimationSetTable.IsReference())
	{
		new(&AnimationSetTable) CAnimationSetTable(ppAnimationSets, NumAnimationSets, this);
	}
	m_SetTableMutex.Release();
	return AnimationSetTable;
}

//-----------------------------------------------------------------------------------				
template<>
void CMultiStageAssetContainer<CAnimationSet, CAnimationSetState::eMaxStages, CAnimationSetState::eStateType, CAnimationSetState::eUnloaded, CAnimationSetState::eReference>::Tick()
{
	m_pAddAssetMutex->Acquire();
	for(CElementEntry* pElement = m_pStages[CAnimationSetState::eUnloaded]->GetFirst(); pElement != NULL; )
	{
		CAnimationSet* pAnimationSet = (CAnimationSet*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		pAnimationSet->SetState(CAnimationSetState::eLoaded);
		m_pStages[CAnimationSetState::eUnloaded]->Move(m_pStages[CAnimationSetState::eLoaded], *pElement);	
		pElement = pNextElement;
	}				
	m_pAddAssetMutex->Release();
}

//-----------------------------------------------------------------------------------	
template<>			
void CMultiStageAssetContainer<CAnimationSetTable, CAnimationSetTableState::eMaxStages, CAnimationSetTableState::eStateType, CAnimationSetTableState::eUnloaded, CAnimationSetTableState::eReference>::Tick()
{
	m_pAddAssetMutex->Acquire();
	for (CElementEntry* pElement = m_pStages[CAnimationSetTableState::eReference]->GetFirst(); pElement != NULL;)
	{
		CAnimationSetTable* pAnimationSetTable = (CAnimationSetTable*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pAnimationSetTable->GetState() == CAnimationSetTableState::eUnloaded)
		{
			m_pStages[CAnimationSetTableState::eReference]->Move(m_pStages[CAnimationSetTableState::eUnloaded], *pElement);
		}
		pElement = pNextElement;
	}

	for (CElementEntry* pElement = m_pStages[CAnimationSetTableState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CAnimationSetTable* pAnimationSetTable = (CAnimationSetTable*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		pAnimationSetTable->SetState(CAnimationSetTableState::eLoaded);
		m_pStages[CAnimationSetTableState::eUnloaded]->Move(m_pStages[CAnimationSetTableState::eLoaded], *pElement);
		pElement = pNextElement;
	}						
	m_pAddAssetMutex->Release();
}
