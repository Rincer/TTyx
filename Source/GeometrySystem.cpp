#include "stdafx.h"
#include "StackAllocator.h"
#include "HashMap.h"
#include "Hash64.h"
#include "Renderer.h"
#include "JobSystem.h"
#include "JobCreateDrawPrimitive.h"
#include "StringDictionary.h"

#include "GeometrySystem.h"

static const unsigned int scMaxVBuffers = 1024; 
static const unsigned int scMaxIBuffers = 1024; 
static const unsigned int scGeometrySystemMemory = 256 * 1024; // 256K


//--------------------------------------------------------------------------------
CBuffer::CBuffer()
{
	m_pBuffer = NULL;
	SetState(CBufferState::eUnloaded);
	m_pResourceData = NULL;
}

//--------------------------------------------------------------------------------
CBuffer::~CBuffer()
{
}

//--------------------------------------------------------------------------------
void CBuffer::Release()
{
	Assert(GetState() == CBufferState::eDeviceRelease);
	if(m_pBuffer)
	{
		m_pBuffer->Release();
		m_pBuffer = NULL;
	}
	SetState(CBufferState::eUnloaded);
}


//--------------------------------------------------------------------------------
void CBuffer::MapResourceData(ID3D11DeviceContext* pDeviceContext)
{
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT Res = pDeviceContext->Map( m_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
	Assert(Res == S_OK);
	m_pResourceData = MappedResource.pData;
}

//--------------------------------------------------------------------------------
void CBuffer::UnMapResourceData(ID3D11DeviceContext* pDeviceContext)
{
	pDeviceContext->Unmap( m_pBuffer, 0 );   
	m_pResourceData = NULL; 			
}

//--------------------------------------------------------------------------------
void* CBuffer::GetResourceData()
{
	return m_pResourceData;
}


//--------------------------------------------------------------------------------
void CVBuffer::CreateFromMemory(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size)
{
	Assert(GetState() == CBufferState::eUnloaded);
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.ByteWidth = Size;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;    
	HRESULT Res;
	if (pData == NULL)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Res = pd3dDevice->CreateBuffer(&bd, NULL, &m_pBuffer);
	}
	else
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = pData;
		Res = pd3dDevice->CreateBuffer(&bd, &InitData, &m_pBuffer);
	}
	Assert(Res == S_OK);
	m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, strlen("Vertex Buffer"), "Vertex Buffer");
	SetState(CBufferState::eLoaded);
}


//--------------------------------------------------------------------------------
void CVBuffer::Set(unsigned int Stream, ID3D11DeviceContext* pDeviceContext)
{
	unsigned int Offset = 0;
	pDeviceContext->IASetVertexBuffers( Stream, 1, &m_pBuffer, &m_Stride, &Offset );
}



//--------------------------------------------------------------------------------
void CIBuffer::CreateFromMemory(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size)
{
	Assert(GetState() == CBufferState::eUnloaded);
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.ByteWidth = Size;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	HRESULT Res;
	if (pData == NULL)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Res = pd3dDevice->CreateBuffer(&bd, NULL, &m_pBuffer);
	}
	else
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = pData;
		Res = pd3dDevice->CreateBuffer(&bd, &InitData, &m_pBuffer);
	}
	Assert(Res == S_OK);
	m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, strlen("Index Buffer"), "Index Buffer");
	SetState(CBufferState::eLoaded);
}

//--------------------------------------------------------------------------------
void CIBuffer::Set(ID3D11DeviceContext* pDeviceContext)
{
	Assert(m_pBuffer != NULL);
	pDeviceContext->IASetIndexBuffer( m_pBuffer, m_Format, 0 );
}

//--------------------------------------------------------------------------------
CGeometrySystem::CGeometrySystem(CRenderer** ppRenderer,
								 CStringDictionary** ppStringDictionary) : m_VBufferAssets(&m_VBufferCreateMutex),
m_IBufferAssets(&m_IBufferCreateMutex),
m_rRenderer(ppRenderer),
m_rStringDictionary(ppStringDictionary)
{
	m_pStackAllocator = new CStackAllocator(scGeometrySystemMemory);	
	Startup();
}

//--------------------------------------------------------------------------------
CGeometrySystem::~CGeometrySystem()
{
	delete m_pStackAllocator;
}

//--------------------------------------------------------------------------------
CVBuffer& CGeometrySystem::CreateVBuffer(const char* pName, unsigned int Stride, const void* pData, unsigned int Size)
{
	CVBuffer* pVBuffer;	
	m_VBufferCreateMutex.Acquire();
	bool Exists;
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	pVBuffer = &(m_VBufferAssets.AddAsset(Key, Exists));
	if(!Exists)
	{
		pVBuffer->m_Stride = Stride;				
		pVBuffer->CreateFromMemory(m_rRenderer->GetDevice(), pData, Size);
	}
	m_VBufferCreateMutex.Release();
	return *pVBuffer;
}

//--------------------------------------------------------------------------------
CIBuffer& CGeometrySystem::CreateIBuffer(const char* pName, const void* pData, unsigned int Size)
{
	CIBuffer* pIBuffer;	
	m_IBufferCreateMutex.Acquire();
	bool Exists;
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	pIBuffer = &(m_IBufferAssets.AddAsset(Key, Exists));
	if(!Exists)
	{
		pIBuffer->m_Format = DXGI_FORMAT_R16_UINT;
		pIBuffer->CreateFromMemory(m_rRenderer->GetDevice(), pData, Size);
	}
	m_IBufferCreateMutex.Release();
	return *pIBuffer;
}


//--------------------------------------------------------------------------------
void CGeometrySystem::Startup()
{
	m_VBufferAssets.Startup(m_pStackAllocator, 1, scMaxVBuffers);	
	m_IBufferAssets.Startup(m_pStackAllocator, 1, scMaxIBuffers);		
}


//--------------------------------------------------------------------------------
void CMultiStageAssetContainer<CVBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference>::Tick()
{
	m_pAddAssetMutex->Acquire();

	// vertex buffers could be added to device create stage from other threads
	for (CElementEntry* pElement = m_pStages[CBufferState::eReference]->GetFirst(); pElement != NULL;)
	{
		CVBuffer* pVBuffer = (CVBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pVBuffer->GetState() == CBufferState::eUnloaded)
		{
			m_pStages[CBufferState::eReference]->Move(m_pStages[CBufferState::eUnloaded], *pElement);
		}
		pElement = pNextElement;
	}

	for(CElementEntry* pElement = m_pStages[CBufferState::eUnloaded]->GetFirst(); pElement != NULL; )
	{
		CVBuffer* pVBuffer = (CVBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if (pVBuffer->IsLoaded())
		{
			m_pStages[CBufferState::eUnloaded]->Move(m_pStages[CBufferState::eLoaded], *pElement);
		}
		pElement = pNextElement;
		
	}	
	m_pAddAssetMutex->Release();
		
	for(CElementEntry* pElement = m_pStages[CBufferState::eDeviceRelease]->GetFirst(); pElement != NULL; )
	{
		CVBuffer* pVBuffer = (CVBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		pVBuffer->Release();
		m_pStages[CBufferState::eDeviceRelease]->Remove(*pElement);
		pElement = pNextElement;
	}					
}

//--------------------------------------------------------------------------------
void CMultiStageAssetContainer<CIBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference>::Tick()
{
	m_pAddAssetMutex->Acquire(); // index buffers could be added to device create stage from other threads

	for (CElementEntry* pElement = m_pStages[CBufferState::eReference]->GetFirst(); pElement != NULL;)
	{
		CIBuffer* pIBuffer = (CIBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pIBuffer->GetState() == CBufferState::eUnloaded)
		{
			m_pStages[CBufferState::eReference]->Move(m_pStages[CBufferState::eUnloaded], *pElement);
		}
		pElement = pNextElement;
	}
	m_pAddAssetMutex->Release();

	for(CElementEntry* pElement = m_pStages[CBufferState::eUnloaded]->GetFirst(); pElement != NULL; )
	{
		CIBuffer* pIBuffer = (CIBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if(pIBuffer->IsLoaded())
		{
			m_pStages[CBufferState::eUnloaded]->Move(m_pStages[CBufferState::eLoaded], *pElement);
		}					
		pElement = pNextElement;
	}	
	
	for(CElementEntry* pElement = m_pStages[CBufferState::eDeviceRelease]->GetFirst(); pElement != NULL; )
	{
		CIBuffer* pIBuffer = (CIBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		pIBuffer->Release();
		m_pStages[CBufferState::eDeviceRelease]->Remove(*pElement);
		pElement = pNextElement;
	}					
}

//--------------------------------------------------------------------------------
void CMultiStageAssetContainer<CVBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference>::Shutdown()
{
	Tick();
	for(CElementEntry* pElement = m_pStages[CBufferState::eLoaded]->GetFirst(); pElement != NULL; )
	{
		CVBuffer* pVBuffer = (CVBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		if(pVBuffer->IsLoaded())
		{
			m_pStages[CBufferState::eLoaded]->Move(m_pStages[CBufferState::eDeviceRelease], *pElement);
			pVBuffer->SetState(CBufferState::eDeviceRelease);
		}
		pElement = pNextElement;
	}			
}

//--------------------------------------------------------------------------------
void CMultiStageAssetContainer<CIBuffer, CBufferState::eMaxBufferStates, CBufferState::eStateType, CBufferState::eUnloaded, CBufferState::eReference>::Shutdown()
{
	Tick();
	for(CElementEntry* pElement = m_pStages[CBufferState::eLoaded]->GetFirst(); pElement != NULL; )
	{
		CIBuffer* pIBuffer = (CIBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		if(pIBuffer->IsLoaded())
		{
			m_pStages[CBufferState::eLoaded]->Move(m_pStages[CBufferState::eDeviceRelease], *pElement);
			pIBuffer->SetState(CBufferState::eDeviceRelease);
		}
		pElement = pNextElement;
	}			
}

//--------------------------------------------------------------------------------
void CGeometrySystem::Tick(float DeltaSec)
{			
	m_VBufferAssets.Tick();
	m_IBufferAssets.Tick();
	DeltaSec;
}

//--------------------------------------------------------------------------------
void CGeometrySystem::Shutdown()
{
	m_VBufferAssets.Shutdown();
	m_IBufferAssets.Shutdown();
}

//--------------------------------------------------------------------------------
bool CGeometrySystem::IsShutdown()
{
	return m_VBufferAssets.IsShutdown() && m_IBufferAssets.IsShutdown();
}
