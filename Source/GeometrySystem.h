#ifndef _GEOMETRYSYSTEM_H_
#define _GEOMETRYSYSTEM_H_
#include "ElementList.h"
#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "JobSystem.h"
#include "Reference.h"

class CElementList;		
class CElementPool;
class CStackAllocator;
class CHashMap;
class CStringDictionary;

class CBufferState
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
class CBuffer  : public CStateAccessor<CBufferState::eStateType>
{
	public:
	
		CBuffer();
		virtual ~CBuffer();
		virtual void CreateFromMemory(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size) = 0;		
		virtual void Set(ID3D11DeviceContext* m_pDeviceContext) = 0;
		void MapResourceData(ID3D11DeviceContext* pDeviceContext);	
		void UnMapResourceData(ID3D11DeviceContext* pDeviceContext);
		void* GetResourceData();					
		void Release();

	protected:
		ID3D11Buffer*	m_pBuffer;	
		void*			m_pResourceData; // ptr to resource data If resource is in a mapped state, null otherwise
};

//--------------------------------------------------------------------------------
class CVBuffer : public CBuffer
{	
	public:
		virtual void CreateFromMemory(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size);				
		virtual void Set(ID3D11DeviceContext* pDeviceContext) { pDeviceContext;}; // Todo, maybe this should not be pure virtual?
		virtual void Set(unsigned int Stream, ID3D11DeviceContext* pDeviceContext);		
		
	private:	
		unsigned int	m_Stride;	
				
	friend class CGeometrySystem;
	friend CMultiStageAssetContainer<CVBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference>;
};

//--------------------------------------------------------------------------------
class CIBuffer : public CBuffer
{	
	public:
		virtual void CreateFromMemory(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size);						
		virtual void Set(ID3D11DeviceContext* pDeviceContext);				

	private:				
		DXGI_FORMAT		m_Format;
		
	friend class CGeometrySystem;		
	friend CMultiStageAssetContainer<CIBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference>;
};

//--------------------------------------------------------------------------------
class CGeometrySystem
{
	public:
		CGeometrySystem(CRenderer** ppRenderer, CStringDictionary** ppStringDictionary);
		~CGeometrySystem();		
		CVBuffer& CreateVBuffer(const char* pName, unsigned int Stride, const void* pData, unsigned int Size);
		CIBuffer& CreateIBuffer(const char* pName, const void* pData, unsigned int Size);				
		void Tick(float DeltaSec);
		void Shutdown();
		bool IsShutdown();
		
	private:		
		void Startup();		
		CMultiStageAssetContainer<CVBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference> m_VBufferAssets;
		CMultiStageAssetContainer<CIBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference> m_IBufferAssets;
		CStackAllocator*				m_pStackAllocator; // Allocator used by the Geometry System
		CLWMutex						m_VBufferCreateMutex;
		CLWMutex						m_IBufferCreateMutex;
		CReference<CRenderer*>			m_rRenderer;
		CReference<CStringDictionary*>	m_rStringDictionary;
};


#endif