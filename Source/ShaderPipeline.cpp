#include "stdafx.h"
#include "HeapAllocator.h"
#include "HashMap.h"
#include "Hash64.h"
#include "MaterialSystem.h"
#include "StringDictionary.h"
#include "JobCreateShader.h"
#include "Renderer.h"

#include "ShaderPipeline.h"

static const char scVertexShaderPath[] = "Source\\ShaderPipeline\\Shaders\\Vertex\\";
static const char scPixelShaderPath[] = "Source\\ShaderPipeline\\Shaders\\Pixel\\";
static const char scComputeShaderPath[] = "Source\\ShaderPipeline\\Shaders\\Compute\\";

static const unsigned int scShaderPipelineHeapSize = 512 * 1024;
static const unsigned int scVertexShadersSegmentSize = 32;
static const unsigned int scPixelShadersSegmentSize = 128;
static const unsigned int scComputeShadersSegmentSize = 16;

//----------------------------------------------------------------------------------------------
CShaderPipeline::CShaderPipeline(CStringDictionary** ppStringDictionary,
								 CJobSystem** ppJobSystem,
								 CRenderer** ppRenderer) :	m_VertexShaders(&m_AddVShaderMutex),
															 m_PixelShaders(&m_AddPShaderMutex),
															 m_ComputeShaders(&m_AddCShaderMutex),
															 m_rStringDictionary(ppStringDictionary),
															 m_rJobSystem(ppJobSystem),
															 m_rRenderer(ppRenderer)
{
	m_pHeapAllocator = new CHeapAllocator(scShaderPipelineHeapSize, false);	
	m_VertexShaders.Startup(m_pHeapAllocator, 1, scVertexShadersSegmentSize);
	m_PixelShaders.Startup(m_pHeapAllocator, 1, scPixelShadersSegmentSize);
	m_ComputeShaders.Startup(m_pHeapAllocator, 1, scComputeShadersSegmentSize);
}

//----------------------------------------------------------------------------------------------
CShaderPipeline::~CShaderPipeline()
{
	delete m_pHeapAllocator;
}

//----------------------------------------------------------------------------------------------
void CShaderPipeline::Tick(float DeltaSec)
{
	DeltaSec;
	m_VertexShaders.Tick();
	m_PixelShaders.Tick();
	m_ComputeShaders.Tick();
}

//----------------------------------------------------------------------------------------------
void CShaderPipeline::Shutdown()
{
	m_VertexShaders.Shutdown();
	m_PixelShaders.Shutdown();
	m_ComputeShaders.Shutdown();
}

//----------------------------------------------------------------------------------------------
bool CShaderPipeline::IsShutdown()
{
	return m_VertexShaders.IsShutdown() && m_PixelShaders.IsShutdown() && m_ComputeShaders.IsShutdown();
}

//----------------------------------------------------------------------------------------------
CHeapAllocator* CShaderPipeline::GetHeapAllocator()
{
	return m_pHeapAllocator;
}

//----------------------------------------------------------------------------------------------
void CShaderPipeline::GetUniqueName(char* pUniqueName, const char* pFileName, const char* pMainName, const D3D_SHADER_MACRO* pDefines)
{
	strcpy_s(pUniqueName, 255, pFileName);
	strcat_s(pUniqueName, 255, "-");
	strcat_s(pUniqueName, 255, pMainName);	
	if(pDefines)
	{
		while(pDefines->Name && pDefines->Definition)
		{
			strcat_s(pUniqueName, 255, "-");
			strcat_s(pUniqueName, 255, pDefines->Name);
			strcat_s(pUniqueName, 255, "_");
			strcat_s(pUniqueName, 255, pDefines->Definition);
			pDefines++;
		}
	}
}

//----------------------------------------------------------------------------------------------
CVertexShader& CShaderPipeline::GetVertexShader(const char* pFileName, const char* pMain, const D3D_SHADER_MACRO* pDefines, D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout, unsigned int LayoutSize)
{
	char UniqueShaderName[256];
	GetUniqueName(UniqueShaderName, pFileName, pMain, pDefines);
	m_AddVShaderMutex.Acquire();
	bool Exists = false;
	unsigned long long Key = m_rStringDictionary->AddString(UniqueShaderName);
	CVertexShader* pShader = &(m_VertexShaders.AddAsset(Key, Exists));
	m_AddVShaderMutex.Release();

	if(!Exists)
	{
		CJobCreateShader* pJobCreateShader = m_rJobSystem->AcquireJob<CJobCreateShader>(CJobSystem::ePriority0);
		new (pJobCreateShader)CJobCreateShader(pShader, pFileName, pMain, pDefines, pVertexShaderLayout, LayoutSize, *m_rRenderer, m_pHeapAllocator);
		m_rJobSystem->AddJob(pJobCreateShader);
	}
	return *pShader;
}

//----------------------------------------------------------------------------------------------
CPixelShader& CShaderPipeline::GetPixelShader(const char* pFileName, const char* pMain, const D3D_SHADER_MACRO* pDefines)
{
	char UniqueShaderName[256];
	GetUniqueName(UniqueShaderName, pFileName, pMain, pDefines);
	m_AddPShaderMutex.Acquire();
	bool Exists = false;
	unsigned long long Key = m_rStringDictionary->AddString(UniqueShaderName);
	CPixelShader* pShader = &(m_PixelShaders.AddAsset(Key, Exists));
	m_AddPShaderMutex.Release();

	if(!Exists)
	{
		CJobCreateShader* pJobCreateShader = m_rJobSystem->AcquireJob<CJobCreateShader>(CJobSystem::ePriority0);
		new (pJobCreateShader)CJobCreateShader(pShader, pFileName, pMain, pDefines, NULL, 0, *m_rRenderer, m_pHeapAllocator);
		m_rJobSystem->AddJob(pJobCreateShader);
	}

	return *pShader;
}

//----------------------------------------------------------------------------------------------
CComputeShader& CShaderPipeline::GetComputeShader(const char* pFileName, const char* pMain, const D3D_SHADER_MACRO* pDefines)
{
	char UniqueShaderName[256];
	GetUniqueName(UniqueShaderName, pFileName, pMain, pDefines);
	m_AddPShaderMutex.Acquire();
	bool Exists = false;
	unsigned long long Key = m_rStringDictionary->AddString(UniqueShaderName);
	CComputeShader* pShader = &(m_ComputeShaders.AddAsset(Key, Exists));
	m_AddPShaderMutex.Release();

	if (!Exists)
	{
		CJobCreateShader* pJobCreateShader = m_rJobSystem->AcquireJob<CJobCreateShader>(CJobSystem::ePriority0);
		new (pJobCreateShader)CJobCreateShader(pShader, pFileName, pMain, pDefines, NULL, 0, *m_rRenderer, m_pHeapAllocator);
		m_rJobSystem->AddJob(pJobCreateShader);
	}

	return *pShader;
}

//----------------------------------------------------------------------------------------------
// Vertex shaders start
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
CVertexShader::CVertexShader()
{
	m_pVertexLayout = NULL;
	m_pVertexShader = NULL;
	m_pVSClassLinkage = NULL;	
	m_pConstantBuffersMap = NULL;
	m_pReflector = NULL;
	m_ppDynamicLinkageArray = NULL; 
	m_NumVSInterfaces = 0;
}

//----------------------------------------------------------------------------------------------
CVertexShader::~CVertexShader()
{
}

//----------------------------------------------------------------------------------------------
void CVertexShader::Release()
{
	if(m_pVertexShader)
	{
		m_pVertexShader->Release();
		m_pVertexShader = NULL;
	}
	if(m_pVertexLayout)
	{
		m_pVertexLayout->Release();
		m_pVertexLayout = NULL;		
	}	
	if(m_pVSClassLinkage)
	{
		m_pVSClassLinkage->Release();
		m_pVSClassLinkage = NULL;
	}			
	if(m_pReflector)
	{
		m_pReflector->Release();
		m_pReflector = NULL;
	}	
	if (m_ppDynamicLinkageArray)
	{
		delete m_ppDynamicLinkageArray;
		m_ppDynamicLinkageArray = NULL;
	}
	SetState(CShaderState::eUnloaded);
}


//----------------------------------------------------------------------------------------------
void CVertexShader::Set(ID3D11DeviceContext* pDeviceContext)
{
	pDeviceContext->IASetInputLayout( m_pVertexLayout );
	pDeviceContext->VSSetShader( m_pVertexShader, m_ppDynamicLinkageArray, m_NumVSInterfaces);
}


//----------------------------------------------------------------------------------------------
const CResourceInfo* CVertexShader::GetResourceInfo(const char* pName) const
{
	if(m_pConstantBuffersMap == NULL)
		return NULL;
	unsigned long long Key = CHash64::GetHash(pName);	
	const CResourceInfo* pRes = (const CResourceInfo*)m_pConstantBuffersMap->GetValue(Key);
	return pRes;
}

//----------------------------------------------------------------------------------------------
void CVertexShader::Build(const char* pFileName, 
						  const char* pMain, 
						  const D3D_SHADER_MACRO* pDefines, 
						  const D3D11_INPUT_ELEMENT_DESC* 
						  pVertexShaderLayout, 
						  unsigned int LayoutSize, 
						  CRenderer* pRenderer,
						  CHeapAllocator* pHeapAllocator)
{
	ID3DBlob* pVShaderBlob = NULL;
	ID3DBlob* pErrorsBlob = NULL;
	ID3D11Device* pd3dDevice = pRenderer->GetDevice();
	char FullPathName[256];
	strcpy_s(FullPathName, 255, scVertexShaderPath);
	strcat_s(FullPathName, 255, pFileName);
		
	unsigned int Flags = D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if _DEBUG 
	Flags |= D3DCOMPILE_DEBUG;
#endif
	HRESULT Res = D3DX11CompileFromFileA(
		FullPathName,
		pDefines,
		NULL,
		pMain,
		"vs_5_0", 
		Flags,
		0,
		0,
		&pVShaderBlob,
		&pErrorsBlob,
		0);
		
	if(Res != S_OK)
	{
		char* pError = (char*)pErrorsBlob->GetBufferPointer();
        if ( pErrorsBlob )
        {
            OutputDebugStringA( pError );
		}
		Assert(0);
	}
    // Create the vertex shader
	Res = pd3dDevice->CreateClassLinkage(&m_pVSClassLinkage);
	Assert(Res == S_OK);	
	m_pVSClassLinkage->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(FullPathName), FullPathName);
	// Create the vertex shader
	Res = pd3dDevice->CreateVertexShader( pVShaderBlob->GetBufferPointer(), pVShaderBlob->GetBufferSize(), m_pVSClassLinkage, &m_pVertexShader);
	Assert(Res == S_OK);
	m_pVertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(FullPathName), FullPathName);
    // Create the input layout
    Res = pd3dDevice->CreateInputLayout( pVertexShaderLayout, LayoutSize, pVShaderBlob->GetBufferPointer(), pVShaderBlob->GetBufferSize(), &m_pVertexLayout);
    Assert(Res == S_OK);
	m_pVertexLayout->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(FullPathName), FullPathName);
    
	D3DReflect( pVShaderBlob->GetBufferPointer(), pVShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**) &m_pReflector);
	D3D11_SHADER_DESC Desc;
	m_pReflector->GetDesc(&Desc);	
    if(Desc.ConstantBuffers)
    {		
		m_pConstantBuffersMap = new(pHeapAllocator->Alloc(sizeof(CHashMap))) CHashMap(pHeapAllocator, Desc.ConstantBuffers);    
			
		for(unsigned int ResourceIndex = 0; ResourceIndex < Desc.BoundResources; ResourceIndex++)
		{
			D3D11_SHADER_INPUT_BIND_DESC ResourceDesc;
			m_pReflector->GetResourceBindingDesc(ResourceIndex, &ResourceDesc);
			if(ResourceDesc.Type == D3D10_SIT_CBUFFER)
			{
				unsigned long long Key = CHash64::GetHash(ResourceDesc.Name);
				CResourceInfo ResourceInfo(ResourceDesc.BindPoint, 1);
				m_pConstantBuffersMap->AddEntry(Key, &ResourceInfo, sizeof(CResourceInfo));
			}		
		}		
	}
		
    m_NumVSInterfaces = m_pReflector->GetNumInterfaceSlots(); 
    if(m_NumVSInterfaces)
    {
		m_ppDynamicLinkageArray = (ID3D11ClassInstance**)pHeapAllocator->Alloc(sizeof(ID3D11ClassInstance*) * m_NumVSInterfaces); 
    }
    else
    {
		m_ppDynamicLinkageArray = NULL;
    }
		
    pVShaderBlob->Release();
	SetState(CShaderState::eLoaded);
}

//----------------------------------------------------------------------------------------------
void CVertexShader::SetClassInstance(unsigned int InterfaceSlot, ID3D11ClassInstance* pClassInstance)
{
	if(m_NumVSInterfaces == 0)
		return;
	if(InterfaceSlot == 0xFFFFFFFF)
		return;
	Assert(InterfaceSlot < m_NumVSInterfaces);
	m_ppDynamicLinkageArray[InterfaceSlot] = pClassInstance;
}


//----------------------------------------------------------------------------------------------
void CVertexShader::GetClassInstance(ID3D11ClassInstance** ppClassInstance, const char* pName)
{
	HRESULT Res = m_pVSClassLinkage->GetClassInstance(pName, 0, ppClassInstance);
	Assert(Res == S_OK);
	(*ppClassInstance)->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(pName), pName);
}

//----------------------------------------------------------------------------------------------
unsigned int CVertexShader::GetInterfaceSlot(const char* pName)
{
	ID3D11ShaderReflectionVariable* pVar = m_pReflector->GetVariableByName(pName);
	return pVar->GetInterfaceSlot(0);	
}


//----------------------------------------------------------------------------------------------
// Pixel shaders start
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
CPixelShader::CPixelShader() :	m_pPixelShader(NULL),
								m_pPSClassLinkage(NULL),
								m_pReflector(NULL),
								m_pResourceMap(NULL)

{
}

//----------------------------------------------------------------------------------------------
CPixelShader::~CPixelShader()
{
}
				
//----------------------------------------------------------------------------------------------				
void CPixelShader::Release()
{
	if(m_pPixelShader)
	{
		m_pPixelShader->Release();
		m_pPixelShader = NULL;
	}
	if(m_pPSClassLinkage)
	{
		m_pPSClassLinkage->Release();
		m_pPSClassLinkage = NULL;
	}	
	if(m_pReflector)
	{
		m_pReflector->Release();
		m_pReflector = NULL;
	}
	SetState(CShaderState::eUnloaded);
}
				
//----------------------------------------------------------------------------------------------
void CPixelShader::Set(ID3D11DeviceContext* pDeviceContext)
{
	pDeviceContext->PSSetShader( m_pPixelShader, m_ppDynamicLinkageArray,  m_NumPSInterfaces );
}
		

//----------------------------------------------------------------------------------------------
void CPixelShader::Build(	const char* pFileName, 
							const char* pMain, 
							const D3D_SHADER_MACRO* pDefines, 
							const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout,
							unsigned int LayoutSize, 
							CRenderer* pRenderer, 
							CHeapAllocator* pHeapAllocator)
{	
	(void)pVertexShaderLayout;
	(void)LayoutSize;

    // Compile the pixel shader
    ID3DBlob* pPShaderBlob = NULL;
	ID3DBlob* pErrorsBlob = NULL;
   
	char FullPathName[256];
	strcpy_s(FullPathName, 255, scPixelShaderPath);
	strcat_s(FullPathName, 255, pFileName);
	ID3D11Device* pd3dDevice = pRenderer->GetDevice();

	unsigned int Flags = D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if _DEBUG 
	Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
#else
	Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	HRESULT Res = D3DX11CompileFromFileA(	
		FullPathName,		
		pDefines,
		NULL,
		pMain,
		"ps_5_0", 
		Flags,
		0,
		0,
		&pPShaderBlob,
		&pErrorsBlob,
		0);
	if(Res != S_OK)
	{
        if ( pErrorsBlob )
        {
            OutputDebugStringA( (char*)pErrorsBlob->GetBufferPointer() );
		}
		Assert(0);
	}

    // Create the pixel shader
	Res = pd3dDevice->CreateClassLinkage(&m_pPSClassLinkage);
	Assert(Res == S_OK);
	m_pPSClassLinkage->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(FullPathName), FullPathName);
    Res = pd3dDevice->CreatePixelShader(pPShaderBlob->GetBufferPointer(), pPShaderBlob->GetBufferSize(), m_pPSClassLinkage, &m_pPixelShader );
    Assert(Res == S_OK);
	m_pPixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(FullPathName), FullPathName);
    
	D3DReflect( pPShaderBlob->GetBufferPointer(), pPShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**) &m_pReflector);
	D3D11_SHADER_DESC Desc;
	m_pReflector->GetDesc(&Desc);	
	if(Desc.BoundResources)
	{
		m_pResourceMap = new(pHeapAllocator->Alloc(sizeof(CHashMap))) CHashMap(pHeapAllocator, Desc.BoundResources);

		for(unsigned int ResourceIndex = 0; ResourceIndex < Desc.BoundResources; ResourceIndex++)
		{
			D3D11_SHADER_INPUT_BIND_DESC ResourceDesc;
			m_pReflector->GetResourceBindingDesc(ResourceIndex, &ResourceDesc);
			unsigned long long Key = CHash64::GetHash(ResourceDesc.Name);
			CResourceInfo ResourceInfo(ResourceDesc.BindPoint, 1);
			m_pResourceMap->AddEntry(Key, &ResourceInfo, sizeof(CResourceInfo));
		}		
	}
	
    m_NumPSInterfaces = m_pReflector->GetNumInterfaceSlots(); 
    if(m_NumPSInterfaces)
    {
		m_ppDynamicLinkageArray = (ID3D11ClassInstance**)pHeapAllocator->Alloc(sizeof(ID3D11ClassInstance*) * m_NumPSInterfaces); 
    }
    else
    {
		m_ppDynamicLinkageArray = NULL;
    }
    pPShaderBlob->Release();
	SetState(CShaderState::eLoaded);
}

//----------------------------------------------------------------------------------------------
const CResourceInfo* CPixelShader::GetResourceInfo(const char* pName) const
{
	if(m_pResourceMap == NULL)
		return NULL;
	unsigned long long Key = CHash64::GetHash(pName);	
	const CResourceInfo* pRes = (const CResourceInfo*)m_pResourceMap->GetValue(Key);
	return pRes;
}

//----------------------------------------------------------------------------------------------
void CPixelShader::SetClassInstance(unsigned int InterfaceSlot, ID3D11ClassInstance* pClassInstance)
{
	if(m_NumPSInterfaces == 0)
		return;
	if(InterfaceSlot == 0xFFFFFFFF)
		return;
	Assert(InterfaceSlot < m_NumPSInterfaces);
	m_ppDynamicLinkageArray[InterfaceSlot] = pClassInstance;
}


//----------------------------------------------------------------------------------------------
void CPixelShader::GetClassInstance(ID3D11ClassInstance** ppClassInstance, const char* pName)
{
	HRESULT Res = m_pPSClassLinkage->GetClassInstance(pName, 0, ppClassInstance);
	Assert(Res == S_OK);
	(*ppClassInstance)->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(pName), pName);
}

//----------------------------------------------------------------------------------------------
unsigned int CPixelShader::GetInterfaceSlot(const char* pName)
{
	ID3D11ShaderReflectionVariable* pVar = m_pReflector->GetVariableByName(pName);
	return pVar->GetInterfaceSlot(0);	
}

// Compute shader code
//----------------------------------------------------------------------------------------------
CComputeShader::CComputeShader() :	m_pShader(NULL),
									m_pReflector(NULL),
									m_pFileName(NULL),
									m_pEntryFunction(NULL),
									m_pResourceMap(NULL)
{
}

//----------------------------------------------------------------------------------------------
CComputeShader::~CComputeShader()
{
}

//----------------------------------------------------------------------------------------------
void CComputeShader::Build(	const char* pFileName, 
							const char* pMain, 
							const D3D_SHADER_MACRO* pDefines,
							const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout,
							unsigned int LayoutSize,
							CRenderer* pRenderer, 
							CHeapAllocator* pHeapAllocator)
{
	(void)pVertexShaderLayout;
	(void)LayoutSize;

	// Compile the pixel shader
	ID3DBlob* pPShaderBlob = NULL;
	ID3DBlob* pErrorsBlob = NULL;

	char FullPathName[256];
	strcpy_s(FullPathName, 255, scComputeShaderPath);
	strcat_s(FullPathName, 255, pFileName);
	ID3D11Device* pd3dDevice = pRenderer->GetDevice();

	unsigned int Flags = D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if _DEBUG 
	Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
#else
	Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	HRESULT Res = D3DX11CompileFromFileA(
		FullPathName,
		pDefines,
		NULL,
		pMain,
		"cs_5_0",
		Flags,
		0,
		0,
		&pPShaderBlob,
		&pErrorsBlob,
		0);
	if (Res != S_OK)
	{
		if (pErrorsBlob)
		{
			OutputDebugStringA((char*)pErrorsBlob->GetBufferPointer());
		}
		Assert(0);
	}

	// Create the shader
	Res = pd3dDevice->CreateComputeShader(pPShaderBlob->GetBufferPointer(), pPShaderBlob->GetBufferSize(), NULL, &m_pShader);
	Assert(Res == S_OK);
	m_pShader->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(FullPathName), FullPathName);
	D3DReflect(pPShaderBlob->GetBufferPointer(), pPShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&m_pReflector);
	D3D11_SHADER_DESC Desc;
	m_pReflector->GetDesc(&Desc);
	if (Desc.BoundResources)
	{
		m_pResourceMap = new(pHeapAllocator->Alloc(sizeof(CHashMap))) CHashMap(pHeapAllocator, Desc.BoundResources);
		for (unsigned int ResourceIndex = 0; ResourceIndex < Desc.BoundResources; ResourceIndex++)
		{
			D3D11_SHADER_INPUT_BIND_DESC ResourceDesc;
			m_pReflector->GetResourceBindingDesc(ResourceIndex, &ResourceDesc);
			unsigned long long Key = CHash64::GetHash(ResourceDesc.Name);
			CResourceInfo ResourceInfo(ResourceDesc.BindPoint, 1);
			m_pResourceMap->AddEntry(Key, &ResourceInfo, sizeof(CResourceInfo));
		}
	}
	pPShaderBlob->Release();
	SetState(CShaderState::eLoaded);
}

//----------------------------------------------------------------------------------------------
const CResourceInfo* CComputeShader::GetResourceInfo(const char* pName) const
{
	if (m_pResourceMap == NULL)
		return NULL;
	unsigned long long Key = CHash64::GetHash(pName);
	const CResourceInfo* pRes = (const CResourceInfo*)m_pResourceMap->GetValue(Key);
	return pRes;
}

//----------------------------------------------------------------------------------------------
void CComputeShader::Set(ID3D11DeviceContext* pDeviceContext)
{
	pDeviceContext->CSSetShader(m_pShader, NULL, 0);
}

//----------------------------------------------------------------------------------------------
void CComputeShader::Release()
{
	if (m_pShader)
	{
		m_pShader->Release();
		m_pShader = NULL;
	}
	if (m_pReflector)
	{
		m_pReflector->Release();
		m_pReflector = NULL;
	}
}



// common code for all shader containers
#define SHADERS_TICK	m_pAddAssetMutex->Acquire(); \
						for(CElementEntry* pElement = m_pStages[CShaderState::eUnloaded]->GetFirst(); pElement != NULL; ) \
						{ \
							CShaderBase* pShader = (CShaderBase*)pElement->m_pData; \
							CElementEntry* pNextElement = pElement->m_pNext; \
							if(pShader->IsLoaded()) \
							{ \
								m_pStages[CShaderState::eUnloaded]->Move(m_pStages[CShaderState::eLoaded], *pElement); \
							} \
							pElement = pNextElement; \
						} \
						m_pAddAssetMutex->Release(); \
						for (CElementEntry* pElement = m_pStages[CShaderState::eDeviceRelease]->GetFirst(); pElement != NULL;) \
						{ \
							CShaderBase* pShader = (CShaderBase*)pElement->m_pData; \
							CElementEntry* pNextElement = pElement->m_pNext; \
							pShader->Release(); \
							m_pStages[CShaderState::eDeviceRelease]->Remove(*pElement); \
							pElement = pNextElement; \
						}				
						

// common code for all shader containers
#define SHADERS_SHUTDOWN	Tick(); \
							for(CElementEntry* pElement = m_pStages[CShaderState::eLoaded]->GetFirst(); pElement != NULL; ) \
							{ \
								CShaderBase* pShader = (CShaderBase*)pElement->m_pData; \
								CElementEntry* pNextElement = pElement->m_pNext; \
								pShader->SetState(CShaderState::eDeviceRelease); \
								m_pStages[CShaderState::eLoaded]->Move(m_pStages[CShaderState::eDeviceRelease], *pElement); \
								pElement = pNextElement; \
							}	


//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CVertexShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>::Tick()
{
	SHADERS_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CPixelShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>::Tick()
{
	SHADERS_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CComputeShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>::Tick()
{
	SHADERS_TICK
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CVertexShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>::Shutdown()
{
	SHADERS_SHUTDOWN
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CPixelShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>::Shutdown()
{
	SHADERS_SHUTDOWN
}

//----------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CComputeShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>::Shutdown()
{
	SHADERS_SHUTDOWN
}
