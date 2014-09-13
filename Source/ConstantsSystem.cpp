#include "stdafx.h"
#include "MaterialSystem.h"
#include "StackAllocator.h"
#include "Renderer.h"
#include "Hash64.h"
#include "StringDictionary.h"

#include "ConstantsSystem.h"

static const unsigned int scMaxCBuffers = 1024; 
static const unsigned int scConstantsSystemMemory = 128 * 1024;

//--------------------------------------------------------------------------------
CConstantBufferBase::CConstantBufferBase()
{
	m_pBuffer = NULL;
	SetState(CConstantBufferState::eUnloaded);
}

//--------------------------------------------------------------------------------
CConstantBufferBase::~CConstantBufferBase()
{
}

//--------------------------------------------------------------------------------
void CConstantBufferBase::Release()
{
	Assert(GetState() == CConstantBufferState::eDeviceRelease);
	if(m_pBuffer)
	{
		m_pBuffer->Release();
		m_pBuffer = NULL;
	}
	SetState(CConstantBufferState::eUnloaded);
}

//--------------------------------------------------------------------------------
ID3D11Buffer* CConstantBufferBase::GetBuffer() const
{
	return ( GetState() == CConstantBufferState::eLoaded) ? m_pBuffer : NULL;
}


//--------------------------------------------------------------------------------
void CCBuffer::Create(ID3D11Device* pd3dDevice, const void* pData, unsigned int Size, const char* pName)
{
	Assert(GetState() == CConstantBufferState::eUnloaded);
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = Size;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA InitData;
    if(pData)
    {
		ZeroMemory( &InitData, sizeof(InitData) );
		InitData.pSysMem = pData;
	}
    HRESULT Res = pd3dDevice->CreateBuffer( &bd, pData ? &InitData : NULL, &m_pBuffer);	
	Assert(Res == S_OK);
	m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(pName), pName);
	SetState(CConstantBufferState::eLoaded);
}

//--------------------------------------------------------------------------------
void CCBuffer::Set(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo* pResourceInfo)
{
	if (GetState() != CConstantBufferState::eLoaded)
		return; // How to handle this if the buffer doesnt exist yet?
								
	if(pResourceInfo->m_ResourceInfo[eVertexShader].m_NumViews > 0)
	{
		pDeviceContext->VSSetConstantBuffers(pResourceInfo->m_ResourceInfo[eVertexShader].m_StartSlot, pResourceInfo->m_ResourceInfo[eVertexShader].m_NumViews, &m_pBuffer);
	}
	if (pResourceInfo->m_ResourceInfo[ePixelShader].m_NumViews > 0)
	{
		pDeviceContext->PSSetConstantBuffers(pResourceInfo->m_ResourceInfo[ePixelShader].m_StartSlot, pResourceInfo->m_ResourceInfo[ePixelShader].m_NumViews, &m_pBuffer);
	}
	if (pResourceInfo->m_ResourceInfo[eComputeShader].m_NumViews > 0)
	{
		pDeviceContext->CSSetConstantBuffers(pResourceInfo->m_ResourceInfo[eComputeShader].m_StartSlot, pResourceInfo->m_ResourceInfo[eComputeShader].m_NumViews, &m_pBuffer);
	}
}

//--------------------------------------------------------------------------------
void CCBuffer::Update(ID3D11DeviceContext* pDeviceContext, const void* pData, unsigned int Size)
{
	if (GetState() != CConstantBufferState::eLoaded)
		return; // How to handle this if the buffer doesnt exist yet?
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT Res = pDeviceContext->Map( m_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
	Assert(Res == S_OK);
	memcpy(MappedResource.pData, pData, Size);
	pDeviceContext->Unmap( m_pBuffer, 0 );    			
}


//--------------------------------------------------------------------------------
CConstantsSystem::CConstantsSystem(CRenderer** ppRenderer, CStringDictionary** ppStringDictionary) : m_AssetConstantBuffers(&m_DeviceCreateMutex),
																									 m_rRenderer(ppRenderer),
																									 m_rStringDictionary(ppStringDictionary)
{
	m_pStackAllocator = new CStackAllocator(scConstantsSystemMemory);	
	StartupCBuffers();
	
	m_BufferSizes[eView] = sizeof(cbView);
	m_BufferSizes[eLights] = sizeof(cbLights);	
	m_BufferSizes[eLocalToWorld] = sizeof(cbLocalToWorld);				
	m_BufferSizes[eBones] = sizeof(cbBones);	
		
	m_CBufferInfo[eView].m_ResourceInfo[eVertexShader].m_StartSlot = eView;
	m_CBufferInfo[eView].m_ResourceInfo[eVertexShader].m_NumViews = 1;
	m_CBufferInfo[eView].m_ResourceInfo[ePixelShader].m_StartSlot = eView;
	m_CBufferInfo[eView].m_ResourceInfo[ePixelShader].m_NumViews = 1;
	m_pCBuffer[eView] = CreateCBuffer(NULL, m_BufferSizes[eView], "CommandBuffer.View");

	m_CBufferInfo[eLights].m_ResourceInfo[ePixelShader].m_StartSlot = eLights;
	m_CBufferInfo[eLights].m_ResourceInfo[ePixelShader].m_NumViews = 1;
	m_pCBuffer[eLights] = CreateCBuffer(NULL, m_BufferSizes[eLights], "CommandBuffer.Lights");
	
	m_CBufferInfo[eLocalToWorld].m_ResourceInfo[eVertexShader].m_StartSlot = eLocalToWorld;
	m_CBufferInfo[eLocalToWorld].m_ResourceInfo[eVertexShader].m_NumViews = 1;
	m_pCBuffer[eLocalToWorld] = CreateCBuffer(NULL, m_BufferSizes[eLocalToWorld], "CommandBuffer.LocalToWorld");

	m_CBufferInfo[eBones].m_ResourceInfo[eVertexShader].m_StartSlot = eBones;
	m_CBufferInfo[eBones].m_ResourceInfo[eVertexShader].m_NumViews = 1;
	m_pCBuffer[eBones] = CreateCBuffer(NULL, m_BufferSizes[eBones], "CommandBuffer.Bones");
}

//--------------------------------------------------------------------------------
CConstantsSystem::~CConstantsSystem()
{
	delete m_pStackAllocator;
}

//--------------------------------------------------------------------------------
CCBuffer* CConstantsSystem::CreateCBuffer(const void* pData, unsigned int Size, const char* pName)
{
	CCBuffer* pCBuffer;	
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	AcquireDeviceCreateMutex();
	bool Exists;
	pCBuffer = &(m_AssetConstantBuffers.AddAsset(Key, Exists));
	if(!Exists)
	{
		pCBuffer->Create(m_rRenderer->GetDevice(), pData, Size, pName);
	}
	ReleaseDeviceCreateMutex();
	return pCBuffer;
}


//--------------------------------------------------------------------------------
void CConstantsSystem::StartupCBuffers()
{
	m_AssetConstantBuffers.Startup(m_pStackAllocator, 1, scMaxCBuffers);
}

//--------------------------------------------------------------------------------
void CConstantsSystem::UpdateConstantBuffer(CConstantBufferBase* pConstantBuffer, void* pData, unsigned int Size)
{
	void* pDstData;
	m_rRenderer->UpdateConstantBufferBegin(pConstantBuffer, &pDstData, Size);
	memcpy(pDstData, pData, Size);
	m_rRenderer->UpdateConstantBufferEnd();
}

//--------------------------------------------------------------------------------
void CConstantsSystem::UpdateConstantBufferBegin(eBindSlot BindSlot, void** ppData, unsigned int Size)
{
	Assert(Size <= m_BufferSizes[BindSlot]);
	m_rRenderer->UpdateConstantBufferBegin(m_pCBuffer[BindSlot], ppData, Size);	
}

//--------------------------------------------------------------------------------
void CConstantsSystem::UpdateConstantBufferEnd()
{
	m_rRenderer->UpdateConstantBufferEnd();
}

//--------------------------------------------------------------------------------		
void CConstantsSystem::SetConstantBuffer(eBindSlot BindSlot)
{
	m_rRenderer->SetConstantBuffer(m_pCBuffer[BindSlot], &m_CBufferInfo[BindSlot]);
}

void CConstantsSystem::UpdateConstantBuffer(eBindSlot BindSlot,ID3D11DeviceContext* pDeviceContext, void* pData, unsigned int Size)
{
	Assert(Size <= m_BufferSizes[BindSlot]);
	m_pCBuffer[BindSlot]->Update(pDeviceContext, pData, Size);
}

//--------------------------------------------------------------------------------		
void CConstantsSystem::SetConstantBuffer(eBindSlot BindSlot, ID3D11DeviceContext* pDeviceContext)
{
	m_pCBuffer[BindSlot]->Set(pDeviceContext, &m_CBufferInfo[BindSlot]);
}

//--------------------------------------------------------------------------------
void CConstantsSystem::AcquireDeviceCreateMutex()
{
	m_DeviceCreateMutex.Acquire();
}

//--------------------------------------------------------------------------------
void CConstantsSystem::ReleaseDeviceCreateMutex()
{
	m_DeviceCreateMutex.Release();
}

//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CCBuffer, CConstantBufferState::eMaxBufferStates, CConstantBufferState::eStateType, CConstantBufferState::eUnloaded, CConstantBufferState::eReference>::Tick()
{
	m_pAddAssetMutex->Acquire();	// Constan buffer device create elements could be getting added from other threads
	for (CElementEntry* pElement = m_pStages[CConstantBufferState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CCBuffer* pCBuffer = (CCBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if(pCBuffer->IsLoaded())
		{		
			m_pStages[CConstantBufferState::eUnloaded]->Move(m_pStages[CConstantBufferState::eLoaded], *pElement);
		}
		pElement = pNextElement;
	}	
	m_pAddAssetMutex->Release();

	for (CElementEntry* pElement = m_pStages[CConstantBufferState::eDeviceRelease]->GetFirst(); pElement != NULL;)
	{
		CCBuffer* pCBuffer = (CCBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		pCBuffer->Release();
		m_pStages[CConstantBufferState::eDeviceRelease]->Remove(*pElement);
		pElement = pNextElement;
	}					
}

//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CCBuffer, CConstantBufferState::eMaxBufferStates, CConstantBufferState::eStateType, CConstantBufferState::eUnloaded, CConstantBufferState::eReference>::Shutdown()
{
	Tick();
	for (CElementEntry* pElement = m_pStages[CConstantBufferState::eLoaded]->GetFirst(); pElement != NULL;)
	{
		CCBuffer* pCBuffer = (CCBuffer*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		if (pCBuffer->GetState() == CConstantBufferState::eLoaded)
		{
			m_pStages[CConstantBufferState::eLoaded]->Move(m_pStages[CConstantBufferState::eDeviceRelease], *pElement);
			pCBuffer->SetState(CConstantBufferState::eDeviceRelease);
		}
		pElement = pNextElement;
	}				
}

//--------------------------------------------------------------------------------
void CConstantsSystem::Tick(float DeltaSec)
{
	m_AssetConstantBuffers.Tick();
	DeltaSec; // To quiet warnings
}

//--------------------------------------------------------------------------------
void CConstantsSystem::Shutdown()
{
	m_AssetConstantBuffers.Shutdown();
}

//--------------------------------------------------------------------------------
bool CConstantsSystem::IsShutdown()
{
	return m_AssetConstantBuffers.IsShutdown();
}


