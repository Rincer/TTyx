#ifndef _ANIMATION_H_
#define _ANIMATION_H_
#include "HashMap.h"
#include "Skeleton.h"
#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "Reference.h"

class CHeapAllocator;
class CAnimation;
class CAnimationSystem;
class CStringDictionary;

//-----------------------------------------------------------------------------
class CAnimationSetState 
{
	public:
		enum eStateType
		{
			eUnloaded,
			eReference,
			eLoaded,
			eMaxStages
		};
};

//-----------------------------------------------------------------------------
class CAnimationSet : public CStateAccessor<CAnimationSetState::eStateType>
{
	public:
	
		static const unsigned int scMaxAnimationSets = 64;
		static const unsigned int scMaxAnimationSetTables = 16;		
	
		class CAnimationState
		{	
			public:
				CAnimationState();
				void Advance(float DeltaSec);
				void Reset();
				float		   m_ClockToKey;
				float		   m_DeltaSec;				
				unsigned int   m_KeyTime;													
				unsigned short m_ScaleKey[CSkeleton::scMaxBones];
				unsigned short m_RotationKey[CSkeleton::scMaxBones];				
				unsigned short m_TranslationKey[CSkeleton::scMaxBones];								

		};
	
		CAnimationSet();		
		
		// Creates an animation set from a table of animations
		void Create(CAnimation**  ppAnimationTable, unsigned int NumAnimations, CAnimationSystem* pAnimationSystem);
		
		// Applies and animation state to an animation set to generate an array of animation matrices
		void Apply(CAnimationState& AnimationState,  XMMATRIX* pMatrices);
		
	protected:			
		unsigned int m_NumAnimations;				
		CAnimation*  m_pAnimations;		
		friend class CMultiStageAssetContainer<CAnimationSet, CAnimationSetState::eMaxStages, CAnimationSetState::eStateType, CAnimationSetState::eUnloaded, CAnimationSetState::eReference>;
};


//-----------------------------------------------------------------------------
class CAnimation
{
	public:	
		CAnimation();
		
		XMVECTORF32*	m_pTranslations;					
		XMVECTORF32*	m_pRotations;		
		XMVECTORF32*	m_pScales;				
		unsigned int*	m_pTranslationKeys;
		unsigned int*	m_pRotationKeys;		
		unsigned int*	m_pScaleKeys;						
		unsigned int	m_NumTranslationFrames;		
		unsigned int	m_NumRotationFrames;				
		unsigned int	m_NumScaleFrames;	
		unsigned short	m_BoneIndex;					
		void UpdateState(CAnimationSet::CAnimationState* pAnimationState, unsigned int AnimationIndex, XMMATRIX* pMatrices);
};

//-----------------------------------------------------------------------------
class CAnimationSetTableState
{
	public:
		enum eStateType
		{
			eUnloaded,
			eReference,
			eLoaded,
			eMaxStages
		};
};


//-----------------------------------------------------------------------------
class CAnimationSetTable : public CStateAccessor<CAnimationSetTableState::eStateType>
{
	public:
		CAnimationSetTable();					
		CAnimationSetTable(	CAnimationSet**	ppAnimationSets,
							unsigned int	NumAnimationSets,
							CAnimationSystem* pAnimationSystem);
		static const char* GetTypeName()
		{
			return "AnimationSetTable";
		}
									
		CAnimationSet**	m_ppAnimationSets;
		unsigned int	m_NumAnimationSets;	
};

//-----------------------------------------------------------------------------
class CAnimationController
{
	public:
		CAnimationController();
		void Create(const char* pName, CAnimationSystem* pAnimationSystem);
		bool IsLoaded() const;
		void Tick(float DeltaSec);
		void Apply(XMMATRIX* pBoneMatrices);
		CAnimationSetTable*				m_pAnimationSetTable;
		CAnimationSet::CAnimationState	m_AnimationState;
		unsigned int m_CurrentSet;
};

//-----------------------------------------------------------------------------
class CAnimationSystem
{
	public:	
		CAnimationSystem(CStringDictionary** ppStringDictionary);
		~CAnimationSystem();		
		
		CAnimationSet& CreateAnimationSet(unsigned long long Key);
		CAnimationSetTable& GetAnimationSetTableRef(const char* pName);				
		CAnimationSetTable& CreateAnimationSetTable(const char* pName, CAnimationSet**	ppAnimationSets, unsigned int NumAnimationSets);
		void AcquireSetMutex();
		void ReleaseSetMutex();		
		void AcquireSetTableMutex();
		void ReleaseSetTableMutex();	
		void Tick(float DeltaSec);	
		
	private:
	
		CMultiStageAssetContainer<CAnimationSet, CAnimationSetState::eMaxStages, CAnimationSetState::eStateType, CAnimationSetState::eUnloaded, CAnimationSetState::eReference> m_AnimationSets;
		CMultiStageAssetContainer<CAnimationSetTable, CAnimationSetTableState::eMaxStages, CAnimationSetTableState::eStateType, CAnimationSetTableState::eUnloaded, CAnimationSetTableState::eReference> m_AnimationSetTables;
		CHeapAllocator* m_pHeapAllocator; // Allocates memory for all animation related data
		CLWMutex m_SetMutex;
		CLWMutex m_SetTableMutex;
		CReference<CStringDictionary*>	m_rStringDictionary;
		friend class CAnimationSet;
		friend class CAnimationSetTable;		
};


#endif