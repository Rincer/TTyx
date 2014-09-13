#include "stdafx.h"
#include "Macros.h"
#include "ShaderPipeline.h"
#include "HeapAllocator.h"
#include "HashMap.h"
#include "Hash64.h"
#include "TextureSystem.h"
#include "ConstantsSystem.h"
#include "Renderer.h"
#include "MaterialPlainColor.h"
#include "MaterialTextured2D.h"
#include "MaterialPhong.h"
#include "MaterialTextureOps.h"
#include "MaterialExperimental.h"
#include "MaterialPostprocess.h"

#include "MaterialSystem.h"

static const unsigned int scMaterialSystemMemorySize = 512 * 1024;
static const unsigned int scPhongMaterialsSegmentSize = 512;
static const unsigned int scPlainColorMaterialsSegmentSize = 3;
static const unsigned int scTextured2DMaterialsSegmentSize = 2;
static const unsigned int scMipGeneratorMaterialsSegmentSize = 1;
static const unsigned int scLinearTo2DMaterialsSegmentSize = 3;
static const unsigned int scExperimentalMaterialsSegmentSize = 1;
static const unsigned int scPostprocessMaterialsSegmentSize = 1;

static XMVECTOR sPink = XMVectorSet(1.0f, 0.0f, 1.0f, 1.0f);
static XMVECTOR sBlack = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);


//----------------------------------------------------------------------------------
void CMaterial::SetSRV(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo& ResourceInfo, ID3D11ShaderResourceView* pSRV) const
{
	ID3D11ShaderResourceView* ppSRV[1] = { pSRV };
	if (ResourceInfo.m_ResourceInfo[eVertexShader].m_NumViews > 0)
	{
		pDeviceContext->VSSetShaderResources(ResourceInfo.m_ResourceInfo[eVertexShader].m_StartSlot, ResourceInfo.m_ResourceInfo[eVertexShader].m_NumViews, ppSRV);
	}

	if (ResourceInfo.m_ResourceInfo[ePixelShader].m_NumViews > 0)
	{
		pDeviceContext->PSSetShaderResources(ResourceInfo.m_ResourceInfo[ePixelShader].m_StartSlot, ResourceInfo.m_ResourceInfo[ePixelShader].m_NumViews, ppSRV);
	}

	if (ResourceInfo.m_ResourceInfo[eComputeShader].m_NumViews > 0)
	{
		pDeviceContext->CSSetShaderResources(ResourceInfo.m_ResourceInfo[eComputeShader].m_StartSlot, ResourceInfo.m_ResourceInfo[eComputeShader].m_NumViews, ppSRV);
	}
}

//----------------------------------------------------------------------------------
void CMaterial::SetUAV(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo& ResourceInfo, ID3D11UnorderedAccessView* pUAV) const
{
	ID3D11UnorderedAccessView* ppUAV[1] = { pUAV };
	if (ResourceInfo.m_ResourceInfo[eComputeShader].m_NumViews > 0)
	{
		pDeviceContext->CSSetUnorderedAccessViews(ResourceInfo.m_ResourceInfo[eComputeShader].m_StartSlot, ResourceInfo.m_ResourceInfo[eComputeShader].m_NumViews, ppUAV, NULL);
	}
}

// Class that handles all material operations
//----------------------------------------------------------------------------------
CMaterialSystem::CMaterialSystem(CStringDictionary**	ppStringDictionary,
								 CJobSystem**			ppJobSystem,
								 CShaderPipeline**		ppShaderPipeline,
								 CConstantsSystem**		ppConstantsSystem) :m_PhongMaterials(&m_AddMaterialMutex),
																			m_PlainColorMaterials(&m_AddMaterialMutex),
																			m_Textured2DMaterials(&m_AddMaterialMutex),
																			m_TextureOpsMaterials(&m_AddMaterialMutex),
																			m_ExperimentalMaterials(&m_AddMaterialMutex),
																			m_PostprocessMaterials(&m_AddMaterialMutex),
																			m_rStringDictionary(ppStringDictionary),
																			m_rJobSystem(ppJobSystem),
																			m_rShaderPipeline(ppShaderPipeline),
																			m_rConstantsSystem(ppConstantsSystem)
{
	m_pHeapAllocator = new CHeapAllocator(scMaterialSystemMemorySize, false);	
	m_PhongMaterials.Startup(m_pHeapAllocator, 1, scPhongMaterialsSegmentSize);
	m_PlainColorMaterials.Startup(m_pHeapAllocator, 1, scPlainColorMaterialsSegmentSize);
	m_Textured2DMaterials.Startup(m_pHeapAllocator, 1, scTextured2DMaterialsSegmentSize);	
	m_TextureOpsMaterials.Startup(m_pHeapAllocator, 1, scLinearTo2DMaterialsSegmentSize);
	m_ExperimentalMaterials.Startup(m_pHeapAllocator, 1, scExperimentalMaterialsSegmentSize);
	m_PostprocessMaterials.Startup(m_pHeapAllocator, 1, scExperimentalMaterialsSegmentSize);
}

//----------------------------------------------------------------------------------
CMaterialSystem::~CMaterialSystem()
{
	delete m_pHeapAllocator;
}

//----------------------------------------------------------------------------------
CMaterial& CMaterialSystem::GetMaterialReference(eMaterials Type, const char* pName)
{
	m_AddMaterialMutex.Acquire();
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	CMaterial* pMaterial = NULL;
	if(Type == eMaterialPhong)
	{
		pMaterial = &(m_PhongMaterials.AddReference(Key));
	}
	else 
	{
		Assert(0);
	}		
	m_AddMaterialMutex.Release();
	return *pMaterial;	
}


//----------------------------------------------------------------------------------
void CMaterialSystem::Tick(float DeltaSec)
{
	DeltaSec;
	m_PhongMaterials.Tick();
	m_PlainColorMaterials.Tick();
	m_Textured2DMaterials.Tick();
	m_TextureOpsMaterials.Tick();
	m_ExperimentalMaterials.Tick();
	m_PostprocessMaterials.Tick();
}

//----------------------------------------------------------------------------------
void CMaterialSystem::Shutdown()
{
	m_PhongMaterials.Shutdown();
	m_PlainColorMaterials.Shutdown();
	m_Textured2DMaterials.Shutdown();
	m_TextureOpsMaterials.Shutdown();
	m_ExperimentalMaterials.Shutdown();
	m_PostprocessMaterials.Shutdown();
}

//----------------------------------------------------------------------------------
bool CMaterialSystem::IsShutdown()
{
	return m_PhongMaterials.IsShutdown() && 
		m_PlainColorMaterials.IsShutdown() && 
		m_Textured2DMaterials.IsShutdown() && 
		m_TextureOpsMaterials.IsShutdown() &&
		m_ExperimentalMaterials.IsShutdown() &&
		m_PostprocessMaterials.IsShutdown();
}


// common code for all templated material containers
#define MATERIAL_TICK	m_pAddAssetMutex->Acquire(); \
						for (CElementEntry* pElement = m_pStages[CMaterialState::eReference]->GetFirst(); pElement != NULL;) \
						{ \
							CMaterial* pMaterial = (CMaterial*)pElement->m_pData; \
							CElementEntry* pNextElement = pElement->m_pNext; \
							if (pMaterial->GetState() == CMaterialState::eUnloaded) \
							{ \
								m_pStages[CMaterialState::eReference]->Move(m_pStages[CMaterialState::eUnloaded], *pElement); \
							} \
							pElement = pNextElement; \
						} \
						for (CElementEntry* pElement = m_pStages[CMaterialState::eUnloaded]->GetFirst(); pElement != NULL;) \
						{ \
							CMaterial* pMaterial = (CMaterial*)pElement->m_pData; \
							CElementEntry* pNextElement = pElement->m_pNext; \
							if(pMaterial->AllResourcesLoaded()) \
							{ \
								pMaterial->CreateViews(); \
								pMaterial->SetState(CMaterialState::eLoaded); \
								m_pStages[CMaterialState::eUnloaded]->Move(m_pStages[CMaterialState::eLoaded], *pElement); \
							} \
							pElement = pNextElement; \
						} \
						m_pAddAssetMutex->Release(); \
						for (CElementEntry* pElement = m_pStages[CMaterialState::eDeviceRelease]->GetFirst(); pElement != NULL;) \
						{ \
							CMaterial* pMaterial = (CMaterial*)pElement->m_pData; \
							CElementEntry* pNextElement = pElement->m_pNext; \
							pMaterial->Release(); \
							m_pStages[CMaterialState::eDeviceRelease]->Remove(*pElement); \
							pElement = pNextElement; \
						}				


// common code for all material containers
#define MATERIAL_SHUTDOWN	Tick(); \
							for(CElementEntry* pElement = m_pStages[CMaterialState::eLoaded]->GetFirst(); pElement != NULL; ) \
							{ \
								CMaterial* pMaterial = (CMaterial*)pElement->m_pData; \
								CElementEntry* pNextElement = pElement->m_pNext; \
								pMaterial->SetState(CMaterialState::eDeviceRelease); \
								m_pStages[CMaterialState::eLoaded]->Move(m_pStages[CMaterialState::eDeviceRelease], *pElement); \
								pElement = pNextElement; \
							}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialPhong, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Tick()
{
	MATERIAL_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialPlainColor, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Tick()
{
	MATERIAL_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialTextured2D, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Tick()
{
	MATERIAL_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialTextureOps, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Tick()
{
	MATERIAL_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialExperimental, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Tick()
{
	MATERIAL_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialPostprocess, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Tick()
{
	MATERIAL_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialPhong, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Shutdown()
{
	MATERIAL_SHUTDOWN
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialPlainColor, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Shutdown()
{
	MATERIAL_SHUTDOWN
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialTextured2D, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Shutdown()
{
	MATERIAL_SHUTDOWN
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialTextureOps, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Shutdown()
{
	MATERIAL_SHUTDOWN
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialExperimental, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Shutdown()
{
	MATERIAL_SHUTDOWN
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CMaterialPostprocess, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference>::Shutdown()
{
	MATERIAL_SHUTDOWN
}






