#ifndef _RENDEROBJECT_H_
#define _RENDEROBJECT_H_

#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "Reference.h"
#include "MaterialPhong.h"

class CDrawPrimitive;
class CHeapAllocator;
class CSkeleton;
class CAnimationController;
class IAllocator;
class CRenderer;
class CRenderObjectSystem;
class CConstantsSystem;
class CStringDictionary;
class CUtilityDraw;

//--------------------------------------------------------------------------------------
class CRenderObjectState
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


//--------------------------------------------------------------------------------------
// render object with geometry stored in world coordinates
class CRenderObject : public CStateAccessor<CRenderObjectState::eStateType>
{
	public:
		CRenderObject();
		CRenderObject(CDrawPrimitive* pDrawPrimitive);
		virtual ~CRenderObject() {}
		virtual void Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem);
		static const char* GetTypeName()
		{
			return "RenderObject";
		}


	protected:
		bool			AllResourcesLoaded() const;
		CDrawPrimitive*	m_pDrawPrimitive;
		friend class CMultiStageAssetContainer<CRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference>;
};


//--------------------------------------------------------------------------------------
// render object with geometry stored in local coordinates
class CInstancedRenderObject : public CRenderObject
{
	public:
		CInstancedRenderObject();
		CInstancedRenderObject(CDrawPrimitive* pDrawPrimitive, XMFLOAT4X4* pLocalToWorld, XMFLOAT4X4* pLocalToWorldNormals);

		virtual ~CInstancedRenderObject() {}
		virtual void Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem);
		void SetLocalToWorld(XMMATRIX& LocalToWorld, XMMATRIX& LocalToWorldNormals);
		
		static const char* GetTypeName()
		{
			return "InstancedRenderObject";
		}

	protected:
		XMFLOAT4X4 m_LocalToWorld;			// Local to world coord transformation
		XMFLOAT4X4 m_LocalToWorldNormals;	// Unscaled for normals
		friend class CMultiStageAssetContainer<CInstancedRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference>;
};


//--------------------------------------------------------------------------------------
// Skeletal animation render object
class CSkeletalRenderObject : public CInstancedRenderObject
{
	public:
		CSkeletalRenderObject();
		virtual ~CSkeletalRenderObject();
		CSkeletalRenderObject(CDrawPrimitive* pDrawPrimitives, XMFLOAT4X4* pLocalToWorld, XMFLOAT4X4* pLocalToWorldNormals, CSkeleton* pSkeleton, CRenderObjectSystem* pRenderObjectSystem, CUtilityDraw* pUtilityDraw);
		virtual void Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem);
		XMMATRIX* GetMatrixPtr();		
		static const char* GetTypeName()
		{
			return "SkeletalRenderObject";
		}

		
	private:
		CSkeleton*	m_pSkeleton;	
		XMMATRIX*	m_pBoneMatrices;	
		CRenderObjectSystem* m_pRenderObjectSystem;
		CUtilityDraw* m_pUtilityDraw;
		friend class CMultiStageAssetContainer<CSkeletalRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference>;

};

//--------------------------------------------------------------------------------------
class CRenderObjectSystem
{
	public:
		CRenderObjectSystem(CStringDictionary** ppStringDictionary, CUtilityDraw** ppUtilityDraw);
		~CRenderObjectSystem();
		
		void CreateRenderObject(const char* pName, CDrawPrimitive* pDrawPrimitive);
		void CreateInstancedRenderObject(const char* pName, CDrawPrimitive* pDrawPrimitive, XMFLOAT4X4* pLocalToWorld, XMFLOAT4X4* pLocalToWorldNormals);
		void CreateSkeletalRenderObject(const char* pName, CDrawPrimitive* pDrawPrimitive, XMFLOAT4X4* pLocalToWorld, XMFLOAT4X4* pLocalToWorldNormals, CSkeleton* pSkeleton);

		CSkeletalRenderObject& GetSkeletalRenderObjectRef(const char* pName);
		CInstancedRenderObject& GetInstancedRenderObjectRef(const char* pName);
		CRenderObject& GetRenderObjectRef(const char* pName);	
		IAllocator* GetAllocator();
		void Tick(float DeltaSec);			
		
		// For debugging
		void DrawAllRenderObjects(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem);
	
		class CRenderObjectCreatorBase
		{
			public:
				virtual void CreateRenderObject(const char* pDesc0, const char* pDesc1, CDrawPrimitive* pDrawPrimitive, CSkeleton* pSkeleton, CRenderObjectSystem* pRenderObjectSystem) = 0;
				virtual CMaterialPhong::eLocalToWorldTransform GetTransformType() = 0;
		};

		class CRenderObjectCreator final : public CRenderObjectCreatorBase
		{
			public:
				virtual void CreateRenderObject(const char* pDesc0, const char*, CDrawPrimitive* pDrawPrimitive, CSkeleton*, CRenderObjectSystem* pRenderObjectSystem)
				{
					char Name[256];
					sprintf_s(Name, "%s.%s", pDesc0, CRenderObject::GetTypeName());
					pRenderObjectSystem->CreateRenderObject(Name, pDrawPrimitive);
				}

				virtual CMaterialPhong::eLocalToWorldTransform GetTransformType()
				{
					return CMaterialPhong::eWorldPos;
				}
		};

		class CInstancedRenderObjectCreator final : public CRenderObjectCreatorBase
		{
			public:
				virtual void CreateRenderObject(const char* pDesc0, const char*, CDrawPrimitive* pDrawPrimitive, CSkeleton*, CRenderObjectSystem* pRenderObjectSystem)
				{
					char Name[256];
					sprintf_s(Name, "%s.%s", pDesc0, CInstancedRenderObject::GetTypeName());
					XMFLOAT4X4 m = { 1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f };
					pRenderObjectSystem->CreateInstancedRenderObject(Name, pDrawPrimitive, &m ,&m);
				}

				virtual CMaterialPhong::eLocalToWorldTransform GetTransformType()
				{
					return CMaterialPhong::eLocalPos;
				}
		};

		class CSkeletalObjectCreator final : public CRenderObjectCreatorBase
		{
			public:
				virtual void CreateRenderObject(const char* pDesc0, const char* pDesc1, CDrawPrimitive* pDrawPrimitive, CSkeleton* pSkeleton, CRenderObjectSystem* pRenderObjectSystem)
				{
					char Name[256];
					sprintf_s(Name, "%s.%s.%s", pDesc0, CSkeletalRenderObject::GetTypeName(), pDesc1);
					XMFLOAT4X4 m = { 1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f };
					pRenderObjectSystem->CreateSkeletalRenderObject(Name, pDrawPrimitive, &m, &m, pSkeleton);
				}

				virtual CMaterialPhong::eLocalToWorldTransform GetTransformType()
				{
					return CMaterialPhong::eLocalPos;
				}
		};

		static CRenderObjectCreator				s_mRenderObjectCreator;
		static CInstancedRenderObjectCreator	s_mInstancedRenderObjectCreator;
		static CSkeletalObjectCreator			s_mSkeletalObjectCreator;

	private:
		CMultiStageAssetContainer<CRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference> m_RenderObjects;
		CMultiStageAssetContainer<CInstancedRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference> m_InstancedRenderObjects;
		CMultiStageAssetContainer<CSkeletalRenderObject, CRenderObjectState::eMaxStages, CRenderObjectState::eStateType, CRenderObjectState::eUnloaded, CRenderObjectState::eReference> m_SkeletalRenderObjects;
		IAllocator* m_pHeapAllocator; 		
		CReference<CStringDictionary*>	m_rStringDictionary;
		CReference<CUtilityDraw*> m_rUtilityDraw;
		CLWMutex m_Mutex;
};
#endif