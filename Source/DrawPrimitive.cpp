#include "stdafx.h"
#include "MaterialSystem.h"
#include "GeometrySystem.h"
#include "Hash64.h"
#include "StringDictionary.h"
#include "Macros.h"

#include "DrawPrimitive.h"

static const unsigned int scMaxDrawPrimitives = 1024; 
static const unsigned int scMaxComputePrimitives = 16;

//----------------------------------------------------------------------------------
void CBasePrimitive::Clone(void* pMemory) const
{
	unsigned int MaterialSize = m_pMaterial->GetSize();
	unsigned int Size = GetSizeWithMaterial() - MaterialSize;
	memcpy(pMemory, this, Size);

	CBasePrimitive* pPrimitive = (CBasePrimitive*)pMemory;
	unsigned char* pMem = (unsigned char*)pMemory + Size;

	m_pMaterial->Clone(pMem);
	CMaterial* pMaterial = (CMaterial*)pMem;

	pPrimitive->m_pMaterial = pMaterial;
}

//----------------------------------------------------------------------------------
void CDrawPrimitive::Draw(ID3D11DeviceContext* pDeviceContext)
{
	m_pMaterial->Set(pDeviceContext);	
	for(unsigned int Stream = 0; Stream < m_NumStreams; Stream++)
	{
		m_pVBuffer[Stream]->Set(Stream, pDeviceContext);
	}
	m_pIBuffer->Set(pDeviceContext);  
    pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );	
    {
//		CTimeLine::CScopedEvent Event(RenderThreadEvents::eCommandDrawPrim);	       
		pDeviceContext->DrawIndexed(m_IndexCount, 0, 0 );    
	}
	m_pMaterial->Unset(pDeviceContext);
}


//--------------------------------------------------------------------------------
bool CDrawPrimitive::AllResourcesLoaded() const
{
	for(unsigned int StreamIndex = 0; StreamIndex < scMaxStreams; StreamIndex++)
	{
		if(m_pVBuffer[StreamIndex] && !m_pVBuffer[StreamIndex]->IsLoaded())
			return false;
	}
	return (m_pIBuffer->IsLoaded() && m_pMaterial->IsLoaded());
}

//----------------------------------------------------------------------------------
void CComputePrimitive::Draw(ID3D11DeviceContext* pDeviceContext)
{
	m_pMaterial->Set(pDeviceContext);
	pDeviceContext->Dispatch(m_DispatchX, m_DispatchY, m_DispatchZ);
	m_pMaterial->Unset(pDeviceContext);
}

//--------------------------------------------------------------------------------
bool CComputePrimitive::AllResourcesLoaded() const
{
	return (m_pMaterial->IsLoaded());
}

//--------------------------------------------------------------------------------
void CComputePrimitive::SetThreadGroupCount(unsigned int DispatchX, unsigned int DispatchY, unsigned int DispatchZ)
{
	m_DispatchX = DispatchX;
	m_DispatchY = DispatchY;
	m_DispatchZ = DispatchZ;
}

//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CDrawPrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference>::Shutdown()
{
}

//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CDrawPrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference>::Tick()
{
	// Draw primitives could be added to the list from other threads
	m_pAddAssetMutex->Acquire();
	for (CElementEntry* pElement = m_pStages[CBasePrimitiveState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CDrawPrimitive* pDrawPrimitive = (CDrawPrimitive*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if(pDrawPrimitive->AllResourcesLoaded())
		{
			pDrawPrimitive->SetState(CBasePrimitiveState::eLoaded);
			m_pStages[CBasePrimitiveState::eUnloaded]->Move(m_pStages[CBasePrimitiveState::eLoaded], *pElement);
		}
		pElement = pNextElement;
	}				
	m_pAddAssetMutex->Release();
}


//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CComputePrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference>::Shutdown()
{
}

//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CComputePrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference>::Tick()
{
	// Draw primitives could be added to the list from other threads
	m_pAddAssetMutex->Acquire();
	for (CElementEntry* pElement = m_pStages[CBasePrimitiveState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CComputePrimitive* pComputePrimitive = (CComputePrimitive*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pComputePrimitive->AllResourcesLoaded())
		{
			pComputePrimitive->SetState(CBasePrimitiveState::eLoaded);
			m_pStages[CBasePrimitiveState::eUnloaded]->Move(m_pStages[CBasePrimitiveState::eLoaded], *pElement);
		}
		pElement = pNextElement;
	}
	m_pAddAssetMutex->Release();
}

//----------------------------------------------------------------------------------
CDrawPrimitive& CDrawPrimitiveSystem::Add(const char* pName, CVBuffer* pVBuffer[CDrawPrimitive::scMaxStreams], unsigned int NumStreams, CIBuffer* pIBuffer, unsigned int IndexCount, const CMaterial* pMaterial)
{
	m_Mutex.Acquire(); // Thread safe	
	bool Exists;
	CDrawPrimitive& DrawPrimitive = m_DrawPrimitives.AddAsset(m_rStringDictionary->AddString(pName), Exists);
	new (&DrawPrimitive) CDrawPrimitive(pVBuffer, NumStreams, pIBuffer, IndexCount, pMaterial);	
	m_Mutex.Release();	
	return DrawPrimitive;
}

//----------------------------------------------------------------------------------
CComputePrimitive& CDrawPrimitiveSystem::Add(const char* pName, unsigned int DispatchX, unsigned int DispatchY, unsigned int DispatchZ, const CMaterial* pMaterial)
{
	m_Mutex.Acquire(); // Thread safe	
	bool Exists;
	CComputePrimitive& ComputePrimitive = m_ComputePrimitives.AddAsset(m_rStringDictionary->AddString(pName), Exists);
	new (&ComputePrimitive) CComputePrimitive(pMaterial, DispatchX, DispatchY, DispatchZ);
	m_Mutex.Release();
	return ComputePrimitive;
}

//----------------------------------------------------------------------------------
void CDrawPrimitiveSystem::AcquireMutex()
{
	m_Mutex.Acquire();
}

//----------------------------------------------------------------------------------
void CDrawPrimitiveSystem::ReleaseMutex()
{
	m_Mutex.Release();
}

//----------------------------------------------------------------------------------
void CDrawPrimitiveSystem::Tick(float DeltaSec)
{
	m_DrawPrimitives.Tick();
	m_ComputePrimitives.Tick();
	DeltaSec; // To quiet warnings
}

//----------------------------------------------------------------------------------
CDrawPrimitiveSystem::CDrawPrimitiveSystem(CStringDictionary** ppStringDictionary) : m_DrawPrimitives(&m_Mutex),
																					 m_ComputePrimitives(&m_Mutex),
																				     m_rStringDictionary(ppStringDictionary)
{
	m_DrawPrimitives.Startup(NULL, 1, scMaxDrawPrimitives);
	m_ComputePrimitives.Startup(NULL, 1, scMaxComputePrimitives);
}

//----------------------------------------------------------------------------------
CDrawPrimitiveSystem::~CDrawPrimitiveSystem()
{
	m_DrawPrimitives.Shutdown();
	m_ComputePrimitives.Shutdown();
}

