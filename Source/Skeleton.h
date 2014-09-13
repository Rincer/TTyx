#ifndef _SKELETON_H_
#define _SKELETON_H_

#include "ElementList.h"
#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "Reference.h"


class IAllocator;
class CSkeletonSystem;
class CConstantsSystem;
class CUtilityDraw;
class CStringDictionary;

class CSkeletonState
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

class CSkeleton : public CStateAccessor<CSkeletonState::eStateType>
{
	public:
	
		static const unsigned int scMaxBones = 256;	
		CSkeleton();
		CSkeleton(CSkeletonSystem* pSkeletonSystem);
		~CSkeleton();
		
		unsigned int GetNumBones() const;
		// calculates matrix palette in memory that can be used directly by the shader pipeline, to draw the primitive
		void CalculateMatrixPalette(XMMATRIX* pMatrices, CConstantsSystem* pConstantsSystem);
		void Draw(XMMATRIX* pMatrices, XMMATRIX* pLocalToWorld, CUtilityDraw* pUtilityDraw);
		CSkeleton &operator= (const CSkeleton& Rhs);
		CSkeleton(const CSkeleton& Rhs); 		
				
	private:

		XMMATRIX*		 m_pBoneMatrices;					// 16 byte aligned pointer to bone matrices, this is the pose that the skeleton is exported in
															// and used for reference only, each skeletal render object contains an instance of its own
															// bone matrix palette
		XMMATRIX*		 m_pModelToBoneMatrices;			// 16 byte aligned pointer to model to bone matrices
		unsigned int	 m_NumBones;
		unsigned int	 m_NumParentedBones;
		unsigned short*	 m_pParentBoneIndex;
		CSkeletonSystem* m_pSkeletonSystem;
		
		friend class CSkeletonSystem;
		friend class CMultiStageAssetContainer<CSkeleton, CSkeletonState::eMaxStages, CSkeletonState::eStateType, CSkeletonState::eUnloaded, CSkeletonState::eReference>;
};

class CSkeletonSystem
{
	public:
		static const unsigned int scSkeletonsPerSegment = 16;		
		
		CSkeletonSystem(CStringDictionary** ppStringDictionary);
		~CSkeletonSystem();
		
		CSkeleton& Add(const char* pName, unsigned int NumBones, XMMATRIX*	pBoneMatrices, XMMATRIX* pModelToBoneMatrices, unsigned int NumParentedBones, unsigned short* pParentBoneIndex);
		void Release(CSkeleton& Skeleton);
	private:
		IAllocator* m_pAllocator;
		CMultiStageAssetContainer<CSkeleton, CSkeletonState::eMaxStages, CSkeletonState::eStateType, CSkeletonState::eUnloaded, CSkeletonState::eReference> m_Skeletons;
		CReference <CStringDictionary*> m_rStringDictionary;
		CLWMutex m_Mutex;	

};

#endif