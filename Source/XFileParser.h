#ifndef _XFILEPARSER_H_
#define _XFILEPARSER_H_

#include "DrawData.h"
#include "Skeleton.h"
#include "HashMap.h"
#include "Animation.h"
#include "StackAllocator.h"
#include "JobSystem.h"

class CMaterial;
class CStackAllocator;
class CTextureSystem;
class CMaterialSystem;
class CAnimationSystem;
class CStringDictionary;
class CGeometrySystem;
class CDrawPrimitiveSystem;
class CSkeletonSystem;
class CRenderObjectSystem;
class CRenderObjectCreatorBase;

class CXFileParser 
{
	public:
		static const unsigned int scScratchMemory = 512 * 1024;
		CXFileParser(CTextureSystem* pTextureSystem, 
					 CMaterialSystem* pMaterialSystem, 
					 CAnimationSystem* pAnimationSystem, 
					 CStringDictionary* pStringDictionary,
					 CGeometrySystem* pGeometrySystem,
					 CDrawPrimitiveSystem* pDrawPrimitiveSystem,
					 CSkeletonSystem* pSkeletonSystem,
					 CRenderObjectSystem* pRenderObjectSystem,
					 CRenderObjectSystem::CRenderObjectCreatorBase* pCreator);
		~CXFileParser();
		void ParseXFileFromMemory(unsigned char* pMemory, unsigned int Size, const char* pName, CJobSystem::CJob* pJob, unsigned int ThreadID);

	private:
			
		void Reset();						
		enum eBlockType
		{
			eNone = -1,
			eTemplate,
			eFrame,
			eFrameTransformMatrix,
			eMesh,
			eMeshNormals,		
			eMeshTextureCoords,
			eVertexDuplicationIndices,
			eMeshMaterialList,
			eMaterial,
			eTextureFilename,
			eXSkinMeshHeader,
			eSkinWeights,
			eAnimationSet,
			eAnimation,
			eAnimationKey,
			eTotalTokens,
		};

		typedef void (*PreProcessor)(CXFileParser* pContext);						
		typedef void (*DataProcessor)(CXFileParser* pContext, const char** ppData, const char* pBlockName);			
		typedef void (*PostProcessor)(CXFileParser* pContext);						
		
		struct tToken
		{
			const char*		m_pTokenName;
			unsigned int	m_NameLength;
			eBlockType		m_Type;
			PreProcessor	m_pPreProcessor;		
			DataProcessor	m_pDataProcessor;
			PostProcessor	m_pPostProcessor;		
		};
			
		eBlockType TypeToEnum(const char* pStr);			
		bool IsMatching(const char* pData, const char* pToken, unsigned int TokenLength);		
		
		static void PreMesh(CXFileParser* pContext);				
		static void ProcessMesh(CXFileParser* pContext, const char** ppData, const char* pBlockName);
		static void PostMesh(CXFileParser* pContext);				
		
		static void ProcessTextureCoords(CXFileParser* pContext, const char** ppData, const char* pBlockName);
		static void ProcessNormals(CXFileParser* pContext, const char** ppData, const char* pBlockName);
		static void ProcessMeshMaterial(CXFileParser* pContext, const char** ppData, const char* pBlockName);
		
		static void ProcessMaterial(CXFileParser* pContext, const char** ppData, const char* pBlockName);
		static void PostMaterial(CXFileParser* pContext);					
					
		static void ProcessTextureName(CXFileParser* pContext, const char** ppData, const char* pBlockName);						
		static void PreFrame(CXFileParser* pContext);				
		static void ProcessFrame(CXFileParser* pContext, const char** ppData, const char* pBlockName);	
		static void PostFrame(CXFileParser* pContext);		
		static void ProcessFrameTransform(CXFileParser* pContext, const char** ppData, const char* pBlockName);			
		
		static void PreAnimationSet(CXFileParser* pContext);		
		static void ProcessAnimationSet(CXFileParser* pContext, const char** ppData, const char* pBlockName);					
		static void PostAnimationSet(CXFileParser* pContext);		
		
		static void ProcessAnimation(CXFileParser* pContext, const char** ppData, const char* pBlockName);					
		static void PostAnimation(CXFileParser* pContext);	
					
		static void ProcessAnimationKey(CXFileParser* pContext, const char** ppData, const char* pBlockName);							
		
		static void ProcessSkinWeights(CXFileParser* pContext, const char** ppData, const char* pBlockName);					
											
		eBlockType ReadBlock(const char **ppData, unsigned int NestingLevel, const char* pEndData);
		void ConvertToIntermediate();
		void AddMaterial();
		void CreateDrawPrimitive();		
		void ReadBone(const char** ppData);		
		
		void ResetBoneData();
		void SetBoneMatrix(unsigned short Index, XMMATRIX& BoneMatrix);
		void SetModelToBoneMatrix(unsigned short Index, XMMATRIX& ModelToBoneMatrix);
		void SetBoneParent(unsigned short Index, unsigned short Parent);
		void IncNumBones();
		void FinalizeAnimationSets();
		void GetParamSubName(char* pSubName);		
		void GetParamName(char* pParamName);
		void GetLibName(char* pLibName);		
		
		// Everything requiring 16 byte alignment needs to be declared first
		XMVECTORF32					m_DiffuseParameters;
		XMVECTORF32					m_SpecularParameters;
		XMMATRIX					m_BoneMatrices[CSkeleton::scMaxBones];			
		XMMATRIX					m_ModelToBoneMatrices[CSkeleton::scMaxBones];				
		//----------------------------------------------------------------------------------------
				
		const char*					m_pPrevBlock;				
		CRawVertexData				m_RawVertexData;	
		CRawIndexData				m_RawIndexData;
		CRawSkinningData			m_RawSkinningData;
		CIntermediateDrawPrimData	m_IntermediateDrawPrimData;
		CIntermediateSkinningData	m_IntermediateSkinningData;
		CStackAllocator*			m_pLocalAllocator;
		unsigned int				m_LocalDataOffset;
		
		char						m_MtlName[256];
		char						m_DiffuseTextureName[256];
		char						m_SpecularTextureName[256]; 			
		char						m_AlphaTextureName[256]; 			
		char						m_NormalTextureName[256];
		unsigned int				m_NumParams;
		CMaterial*					m_pMaterial;								
		
		unsigned short				m_BoneStack[CSkeleton::scMaxBones];
		int							m_BoneStackSize;		

		unsigned int				m_NumBones;
		unsigned int				m_NumParentedBones;
		unsigned short				m_ParentBoneIndex[CSkeleton::scMaxBones];	
		
		char						m_BoneNames[CSkeleton::scMaxBones][256];
		CAnimation*					m_pAnimationTable[CSkeleton::scMaxBones]; // cant have more animations than bones
		unsigned int				m_NumAnimations;	
		CAnimationSet*				m_pAnimationSet;
		CAnimationSet*				m_pAnimationSetTable[CAnimationSet::scMaxAnimationSets];		
		unsigned int				m_NumAnimationSets;		
		
	
		CStaticHashMap<unsigned short>* m_pBonesHash;
		CAnimation*					m_pAnimation;
		CStackAllocator				m_LocalAllocator; // The actual memory for the allocator
		const char*					m_pName;	// Name of asset being parsed
		unsigned int				m_ThreadID; // thread pool id of executing thread
		CJobSystem::CJob*			m_pJob;
		CTextureSystem*				m_pTextureSystem;
		CMaterialSystem*			m_pMaterialSystem; 
		CAnimationSystem*			m_pAnimationSystem;
		CStringDictionary*			m_pStringDictionary;
		CGeometrySystem*			m_pGeometrySystem;
		CDrawPrimitiveSystem*		m_pDrawPrimitiveSystem;
		CSkeletonSystem*			m_pSkeletonSystem;
		CRenderObjectSystem*		m_pRenderObjectSystem;
		CRenderObjectSystem::CRenderObjectCreatorBase* m_pCreator;
		unsigned char				m_ScratchMemory[1]; // Needs to be last non-static
															
		static tToken m_Tokens[eTotalTokens];
};

#endif
