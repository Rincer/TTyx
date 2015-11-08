#ifndef _CONSTANTSSYSTEM_H_
#define _CONSTANTSSYSTEM_H_
#include "ElementList.h"
#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "Skeleton.h"
#include "MaterialSystem.h"
#include "Reference.h"

class CElementList;		
class CElementPool;
class CStackAllocator;
class CHashMap;
class CResourceInfo;
class CRenderer;
class CStringDictionary;


//--------------------------------------------------------------------------------
class CConstantBufferState
{
	public:
		enum eStateType
		{
			eUnloaded,			// not loaded
			eReference,
			eLoaded,			// buffer is ready
			eDeviceRelease,		// graphics device is releasing the buffer
			eMaxBufferStates
		};
};


//--------------------------------------------------------------------------------
// Have a base class in case in the future this will be extended to support texture buffers
class CConstantBufferBase : public CStateAccessor<CConstantBufferState::eStateType>
{
	public:
	
		CConstantBufferBase();
		virtual ~CConstantBufferBase();
		virtual void Create(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size, const char* pName) = 0;
		virtual void Set(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo* pResourceInfo) = 0;
		virtual void Update(ID3D11DeviceContext* pDeviceContext, const void* pData, unsigned int Size) = 0;
		void Release();
		ID3D11Buffer* GetBuffer() const;
		

	protected:
		ID3D11Buffer*	m_pBuffer;		
};

//--------------------------------------------------------------------------------
class CCBuffer : public CConstantBufferBase
{
	public:
		CCBuffer()
		{
		}
		virtual void Create(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size, const char* pName);		
		virtual void Set(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo* pResourceInfo);	
		virtual void Update(ID3D11DeviceContext* pDeviceContext, const void* pData, unsigned int Size);	
			
	friend class CConstantsSystem;		
	friend class CMultiStageAssetContainer<CCBuffer, CConstantBufferState::eMaxBufferStates, CConstantBufferState::eStateType, CConstantBufferState::eUnloaded, CConstantBufferState::eReference>;
};

//--------------------------------------------------------------------------------
class CConstantsSystem
{
	public:
	
		enum eBindSlot	// Needs to correspond to the bind registers, because these are shared by both vetex and pixel shaders  
		{				// a bind slot used for a constant buffer in a vertex shader only, will not be available in the pixel shader unless we want to use it for
						// the same constant data.
			eView,
			eLights,
			eLocalToWorld,
			eBones,
			eMaxSlots
		};

		struct cbView
		{
			XMMATRIX	m_InvViewProjection;
			XMVECTORF32 m_CameraPos;
		};

		struct cbLights
		{
			// point
			XMVECTOR m_ColorSqR;	// r, g, b, sqr(Radius)
			XMVECTOR m_PosInvSqR;	// x, y, z, 1 / sqr(Radius)
			// directional
			XMVECTOR m_DirectionalDir;	 
			XMVECTOR m_DirectionalCol;	
		};

		struct cbLocalToWorld
		{
			XMMATRIX m_LocalToWorld;
			XMMATRIX m_LocalToWorldNormals;			
		};
		
		struct cbBones
		{
			XMVECTOR m_Bones[CSkeleton::scMaxBones][3]; 
		};

		CConstantsSystem(CRenderer** ppRenderer,
						 CStringDictionary** ppStringDictionary);
		~CConstantsSystem();
				
		CCBuffer* CreateCBuffer(const void* pData, unsigned int Size, const char* pName);
		
		void Tick(float DeltaSec);
		void Shutdown();
		bool IsShutdown();		
		void AcquireDeviceCreateMutex();
		void ReleaseDeviceCreateMutex();	
		void UpdateConstantBuffer(CConstantBufferBase* pConstantBuffer, void* pData, unsigned int Size);

		// These 2 come from a thread other than render and cause queueing up of render thread commands
		void UpdateConstantBufferBegin(eBindSlot BindSlot, void** ppData, unsigned int Size);	
		void UpdateConstantBufferEnd();

		void SetConstantBuffer(eBindSlot BindSlot);	
				
		// These 2 come from a render thread and are executed directly
		void UpdateConstantBuffer(eBindSlot BindSlot,ID3D11DeviceContext* pDeviceContext, void* pData, unsigned int Size);	
		void SetConstantBuffer(eBindSlot BindSlot, ID3D11DeviceContext* pDeviceContext);			
		
	private:		
		void StartupCBuffers();
		void TickCBuffers();
		bool AreCBuffersShutDown();
		
		CMultiStageAssetContainer<CCBuffer, CConstantBufferState::eMaxBufferStates, CConstantBufferState::eStateType, CConstantBufferState::eUnloaded, CConstantBufferState::eReference> m_AssetConstantBuffers;
		CStackAllocator*	m_pStackAllocator; // Allocator used by the Constant buffers System		
		CLWMutex			m_DeviceCreateMutex;
		
		CShaderResourceInfo	m_CBufferInfo[eMaxSlots];
		CCBuffer*			m_pCBuffer[eMaxSlots];
		unsigned int		m_BufferSizes[eMaxSlots];		
		CReference<CRenderer*>			m_rRenderer;
		CReference<CStringDictionary*>	m_rStringDictionary;

};

#endif