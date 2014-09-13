#include "stdafx.h"
#include "StackAllocator.h"
#include "HashMap.h"
#include "Hash64.h"
#include "Renderer.h"
#include "MaterialSystem.h"
#include "ImageProcessing.h"
#include "StringDictionary.h"
#include "JobLoadTexture.h"
#include "Math.h"
#include "Macros.h"
#include "MaterialTextureOps.h"
#include "DrawPrimitive.h"

#include "TextureSystem.h"

static const unsigned int scMaxTextures = 1024; 
static const unsigned int scTextureSystemMemory = 256 * 1024; // 256K
static const char* scTexturePath = "Textures\\";

//--------------------------------------------------------------------------------
CTexture::CTexture() :	m_pTexture(NULL),
						m_pRawTextureData(NULL)
{
	memset(&m_Desc, 0, sizeof(m_Desc));
	SetState(CTextureState::eUnloaded);
}

//--------------------------------------------------------------------------------
CTexture::~CTexture()
{
}

//--------------------------------------------------------------------------------
void CTexture::CreateRawDataResource(ID3D11Device* pd3dDevice, const void* pRawData, unsigned int Size)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = Size;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	HRESULT Res;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = pRawData;
	Res = pd3dDevice->CreateBuffer(&bd, &InitData, &m_pRawTextureData);
	Assert(Res == S_OK);
}

//--------------------------------------------------------------------------------
void CTexture::Create2D(ID3D11Device* pd3dDevice, const void* pData, const char* pName)
{
	HRESULT Res;
	D3D11_TEXTURE2D_DESC Desc = m_Desc;
	if (pData)
	{
		D3D11_SUBRESOURCE_DATA SubResourceData[CTexture::sc_MaxMipLevel];
		unsigned int Bpp = CTextureSystem::GetFormatBPP(m_Desc.Format);
		unsigned int MipLevelSize = m_Desc.Width * m_Desc.Height * Bpp;
		unsigned int SysMemPitch = m_Desc.Width * Bpp;
		const unsigned char* pSysMem = (const unsigned char*)pData;
		for (unsigned int MipLevel = 0; MipLevel < m_Desc.MipLevels; MipLevel++)
		{
			SubResourceData[MipLevel].pSysMem = pSysMem;
			SubResourceData[MipLevel].SysMemPitch = SysMemPitch;
			pSysMem += MipLevelSize;
			SysMemPitch /= 2;
			MipLevelSize /= 4;
		}
		Res = pd3dDevice->CreateTexture2D(&Desc, SubResourceData, &m_pTexture);
	}
	else
	{
		Res = pd3dDevice->CreateTexture2D(&Desc, NULL, &m_pTexture);
	}
	Assert(Res == S_OK);
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(pName), pName);
}

//--------------------------------------------------------------------------------
void CTexture::Create2DCube(ID3D11Device* pd3dDevice, const char* pName)
{
	HRESULT Res;
	Res = pd3dDevice->CreateTexture2D(&m_Desc, NULL, &m_pTexture);	
	Assert(Res == S_OK);
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(pName), pName);
}

//--------------------------------------------------------------------------------
void CTexture::CreateTexture2D(ID3D11Device* pd3dDevice, const void* pData, const char* pName)
{
	Create2D(pd3dDevice, pData, pName);
	SetState(CTextureState::eLoaded);
}

//--------------------------------------------------------------------------------
void CTexture::CreateTexture2DCube(ID3D11Device* pd3dDevice, const char* pName)
{
	Create2DCube(pd3dDevice, pName);
	SetState(CTextureState::eLoaded);
}

//--------------------------------------------------------------------------------
// uses compute shaders to create the texture from raw data
void CTexture::CreateTexture2D_Compute(ID3D11Device* pd3dDevice, const void* pData, const char* pName)
{
	Create2D(pd3dDevice, NULL, pName);
	CreateRawDataResource(pd3dDevice, pData, GetSubresourceDataSize(0, 0));
	SetState(CTextureState::eRawDataLoaded);
}

// gets a SRV of specifed mip level
//--------------------------------------------------------------------------------
void CTexture::GetSRV(ID3D11ShaderResourceView** ppSRV, eFormatModifiers FormatModifier, int MipLevel) const
{
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = CTexture::ApplyModifier(m_Desc.Format, FormatModifier);
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	if(MipLevel < 0)
	{
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = m_Desc.MipLevels;	
	}
	else
	{
		SRVDesc.Texture2D.MostDetailedMip = (unsigned int)MipLevel;
		SRVDesc.Texture2D.MipLevels = 1;	
	}
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateShaderResourceView(m_pTexture, &SRVDesc, ppSRV);
	Assert(Res == S_OK);
}

// gets an RTV of specifed mip level
void CTexture::GetRTV(ID3D11RenderTargetView** ppRTV, eFormatModifiers FormatModifier, int MipLevel) const
{
	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = CTexture::ApplyModifier(m_Desc.Format, FormatModifier);
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = (unsigned int)MipLevel;
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateRenderTargetView(m_pTexture, &RTVDesc, ppRTV);
	Assert(Res == S_OK);
}

// gets a UAV of specifed mip level
//--------------------------------------------------------------------------------
void CTexture::GetUAV(ID3D11UnorderedAccessView** ppUAV, eFormatModifiers FormatModifier, int MipLevel) const
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = CTexture::ApplyModifier(m_Desc.Format, FormatModifier);
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	UAVDesc.Texture2D.MipSlice = (unsigned int)MipLevel;
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateUnorderedAccessView(m_pTexture, &UAVDesc, ppUAV);
	Assert(Res == S_OK);
}

// gets a shader resource view of specified mip level of a cube map face
void CTexture::GetCubeSRV(ID3D11ShaderResourceView** ppSRV, eFormatModifiers FormatModifier, int MipLevel, unsigned int Face) const
{
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = CTexture::ApplyModifier(m_Desc.Format, FormatModifier);
	if(MipLevel < 0)
	{
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		SRVDesc.TextureCube.MostDetailedMip = 0;
		SRVDesc.TextureCube.MipLevels = m_Desc.MipLevels;
	}
	else
	{
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		SRVDesc.Texture2DArray.ArraySize = 1;
		SRVDesc.Texture2DArray.FirstArraySlice = Face;
		SRVDesc.Texture2DArray.MostDetailedMip = MipLevel;
		SRVDesc.Texture2DArray.MipLevels = 1;
	}
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateShaderResourceView(m_pTexture, &SRVDesc, ppSRV);
	Assert(Res == S_OK);
}

// gets a render target view of specified mip level of a cube map face
void CTexture::GetCubeRTV(ID3D11RenderTargetView** ppRTV, eFormatModifiers FormatModifier, int MipLevel, unsigned int Face) const
{
	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = CTexture::ApplyModifier(m_Desc.Format, FormatModifier);
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	RTVDesc.Texture2DArray.ArraySize = 1;
	RTVDesc.Texture2DArray.FirstArraySlice = Face;
	RTVDesc.Texture2DArray.MipSlice = (unsigned int)MipLevel;
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateRenderTargetView(m_pTexture, &RTVDesc, ppRTV);
	Assert(Res == S_OK);
}


// gets an unordered access view of specified mip level of a cube map face
void CTexture::GetCubeUAV(ID3D11UnorderedAccessView** ppUAV, eFormatModifiers FormatModifier, int MipLevel, unsigned int Face) const
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = CTexture::ApplyModifier(m_Desc.Format, FormatModifier);
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	UAVDesc.Texture2DArray.ArraySize = 1;
	UAVDesc.Texture2DArray.FirstArraySlice = Face;
	UAVDesc.Texture2DArray.MipSlice = (unsigned int)MipLevel;
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateUnorderedAccessView(m_pTexture, &UAVDesc, ppUAV);
	Assert(Res == S_OK);
}

// gets a depth stencil view of specified mip level
void CTexture::GetDSV(ID3D11DepthStencilView** ppRTV, eFormatModifiers FormatModifier, int MipLevel) const
{
	D3D11_DEPTH_STENCIL_VIEW_DESC  DSVDesc;
	DSVDesc.Format = CTexture::ApplyModifier(m_Desc.Format, FormatModifier);
	DSVDesc.Flags = 0;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Texture2D.MipSlice = (unsigned int)MipLevel;
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateDepthStencilView(m_pTexture, &DSVDesc, ppRTV);
	Assert(Res == S_OK);
}



// gets an SRV into buffer holding the raw texture data
//--------------------------------------------------------------------------------
void CTexture::GetRawDataSRV(ID3D11ShaderResourceView** ppSRV) const
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	desc.BufferEx.FirstElement = 0;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	desc.BufferEx.NumElements = GetSubresourceDataSize(0, 0) / 4;
	HRESULT Res = m_pOwnerSystem->GetDevice()->CreateShaderResourceView(m_pRawTextureData, &desc, ppSRV);
	Assert(Res == S_OK);
}

//--------------------------------------------------------------------------------
void CTexture::Release()
{
	Assert(GetState() == CTextureState::eDeviceRelease);
	if(m_pTexture)
	{
		m_pTexture->Release();
		m_pTexture = NULL;
	}

	if (m_pRawTextureData)
	{
		m_pRawTextureData->Release();
		m_pRawTextureData = NULL;
	}

	SetState(CTextureState::eUnloaded);
}

//--------------------------------------------------------------------------------
unsigned int CTexture::GetSubresourceDataSize(unsigned int StartMip, unsigned int EndMip) const
{
	unsigned int Bpp = CTextureSystem::GetFormatBPP(m_Desc.Format);
	unsigned int TotalSize = 0;  
	unsigned int Width = m_Desc.Width;
	unsigned int Height = m_Desc.Height;

	if (m_Desc.MipLevels == 1)
	{
		TotalSize = Width * Height;
	}
	else
	{
		for (unsigned int MipLevel = (MAX(0, StartMip)); MipLevel <= (MIN(m_Desc.MipLevels - 1, EndMip)); MipLevel++)
		{
			TotalSize += Width * Height;
			Width = (Width == 1) ? Width : Width / 2;
			Height = (Height == 1) ? Height : Height / 2;
		}
	}
	TotalSize *= Bpp;
	return TotalSize;
}

//--------------------------------------------------------------------------------
bool CTexture::GenerateMipmaps()
{
	if (m_pOwnerSystem->GenerateMipmaps(this))
	{
		SetState(CTextureState::eCreateMips);
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------------
DXGI_FORMAT CTexture::ApplyModifier(DXGI_FORMAT SrcFormat, eFormatModifiers FormatModifier)
{
	if(FormatModifier == eNONE)
		return SrcFormat;

	switch (SrcFormat)
	{
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		{
			switch(FormatModifier)
			{
				case eFLOAT:
					return DXGI_FORMAT_R16G16B16A16_FLOAT;
			}
			break;
		}

		case DXGI_FORMAT_R8G8B8A8_UNORM: // special case for back buffer
		{
			switch(FormatModifier)
			{
				case eUNORM_SRGB:
					return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			}
			break;
		}

		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		{
			switch(FormatModifier)
			{
				case eUNORM:
					return DXGI_FORMAT_R8G8B8A8_UNORM;
				case eUNORM_SRGB:
					return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			}
			break;
		}

		case DXGI_FORMAT_R24G8_TYPELESS:
		{
			switch(FormatModifier)
			{
				case eD24_UNORM_S8_UINT:
					return DXGI_FORMAT_D24_UNORM_S8_UINT;
			}
			break;
		}

		case DXGI_FORMAT_R8_TYPELESS:
		{
			switch(FormatModifier)
			{
				case eUNORM:
					return DXGI_FORMAT_R8_UNORM;
			}
			break;
		}
	}
	Assert(0);
	return DXGI_FORMAT_UNKNOWN;
}

//--------------------------------------------------------------------------------
void CSampler::Initialize(ID3D11SamplerState* pSampler)
{
	m_pSampler = pSampler;
}

//--------------------------------------------------------------------------------
void CSampler::Set(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo* pResourceInfo)
{
	if(pResourceInfo->m_ResourceInfo[ePixelShader].m_NumViews)
	{
		pDeviceContext->PSSetSamplers(pResourceInfo->m_ResourceInfo[ePixelShader].m_StartSlot, pResourceInfo->m_ResourceInfo[ePixelShader].m_NumViews, &m_pSampler);
	}
	if (pResourceInfo->m_ResourceInfo[eComputeShader].m_NumViews)
	{
		pDeviceContext->CSSetSamplers(pResourceInfo->m_ResourceInfo[eComputeShader].m_StartSlot, pResourceInfo->m_ResourceInfo[eComputeShader].m_NumViews, &m_pSampler);
	}
}

//--------------------------------------------------------------------------------
void CSampler::Release()
{
	if(m_pSampler)
	{
		m_pSampler->Release();
		m_pSampler = NULL;
	}
}

//--------------------------------------------------------------------------------
CTextureSystem::CTextureSystem( CStringDictionary**  ppDictionary,
								CJobSystem** ppJobSystem,
								CRenderer**	ppRenderer,
								CLoadingSystem** ppLoadingSystem,
								CDrawPrimitiveSystem** ppDrawPrimitiveSystem,
								CMaterialSystem** ppMaterialSystem,
								CConstantsSystem** ppConstantsSystem) : m_pBlackTexture(NULL),
																		m_rStringDictionary(ppDictionary),
																		m_rJobSystem(ppJobSystem),
																		m_rRenderer(ppRenderer),
																		m_rLoadingSystem(ppLoadingSystem),
																		m_rDrawPrimitiveSystem(ppDrawPrimitiveSystem),
																		m_rMaterialSystem(ppMaterialSystem),
																		m_rConstantsSystem(ppConstantsSystem),
																		m_TextureAssets(&m_AddTextureMutex),
																		m_pLinearTo2DPrimitive(NULL),
																		m_pLinearTo2DMaterial(NULL),
																		m_pDownsampleBox2x2Primitive(NULL),
																		m_pDownsampleBox2x2Material(NULL),
																		m_pDownsampleCubeBox2x2Primitive(NULL),
																		m_pDownsampleCubeBox2x2Material(NULL)

{
	m_pStackAllocator = new CStackAllocator(scTextureSystemMemory);
	m_TextureAssets.Startup(m_pStackAllocator, 1, scMaxTextures);
	m_SamplersCreated = false;
}

//--------------------------------------------------------------------------------
CTextureSystem::~CTextureSystem()
{
	delete m_pStackAllocator;
}

//--------------------------------------------------------------------------------
CTexture& CTextureSystem::GetTextureRef(const char* pName)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	return m_TextureAssets.AddReference(Key);

}

//--------------------------------------------------------------------------------
CTexture& CTextureSystem::CreateTextureFromFile(DXGI_FORMAT Format, unsigned int MipLevels, const char* pName) // 0 MipLevels = entire chain
{
	char FullPathName[256];
	strcpy_s(FullPathName, 255, scTexturePath);
	strcat_s(FullPathName, 255, pName);
	unsigned long long Key = m_rStringDictionary->AddString(FullPathName);
	// can be called from multiple threads	
	m_AddTextureMutex.Acquire();
	bool Exists = false;
	CTexture* pTexture = &(m_TextureAssets.AddAsset(Key, Exists));
	if (!Exists || pTexture->IsReference())
	{
		pTexture->m_Desc.Format = Format;
		pTexture->m_Desc.MipLevels = MipLevels;
		pTexture->m_pOwnerSystem = this;
		CJobLoadTexture* pJobLoadTexture = m_rJobSystem->AcquireJob<CJobLoadTexture>(CJobSystem::ePriority0);
		new (pJobLoadTexture)CJobLoadTexture(pTexture, FullPathName, *m_rRenderer, *m_rLoadingSystem);
		m_rJobSystem->AddJob(pJobLoadTexture);
	}
	m_AddTextureMutex.Release();
	return *pTexture;
}

//--------------------------------------------------------------------------------
CTexture& CTextureSystem::CreateTexture(DXGI_FORMAT Format, unsigned int Width, unsigned int Height, unsigned int MipLevels, unsigned int ArraySize, D3D11_BIND_FLAG BindFlags, const char* pName)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	// can be called from multiple threads	
	m_AddTextureMutex.Acquire();
	bool Exists = false;
	CTexture* pTexture = &(m_TextureAssets.AddAsset(Key, Exists));
	if (!Exists || pTexture->IsReference())
	{
		pTexture->m_Desc.Format = Format;
		pTexture->m_Desc.MipLevels = MipLevels;
		pTexture->m_Desc.ArraySize = ArraySize;
		pTexture->m_Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | BindFlags;
		pTexture->m_Desc.Width = Width;
		pTexture->m_Desc.Height = Height;
		pTexture->m_Desc.SampleDesc.Count = 1;
		pTexture->m_Desc.SampleDesc.Quality = 0;
		pTexture->m_pOwnerSystem = this;
		pTexture->Create2D(GetDevice(), NULL, pName);
	}
	// render targets or depth textures are considered to be loaded by default
	if (BindFlags & (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_RENDER_TARGET))
	{
		pTexture->SetState(CTextureState::eLoaded);
	}
	m_AddTextureMutex.Release();
	return *pTexture;
}

//--------------------------------------------------------------------------------
CTexture& CTextureSystem::CreateCubeTexture(DXGI_FORMAT Format, unsigned int Width, unsigned int Height, unsigned int MipLevels, unsigned int ArraySize, D3D11_BIND_FLAG BindFlags, const char* pName)
{
	unsigned long long Key = m_rStringDictionary->AddString(pName);
	// can be called from multiple threads	
	m_AddTextureMutex.Acquire();
	bool Exists = false;
	CTexture* pTexture = &(m_TextureAssets.AddAsset(Key, Exists));
	if (!Exists || pTexture->IsReference())
	{
		pTexture->m_Desc.Format = Format;
		pTexture->m_Desc.MipLevels = MipLevels;
		pTexture->m_Desc.ArraySize = ArraySize;
		pTexture->m_Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | BindFlags;
		pTexture->m_Desc.Width = Width;
		pTexture->m_Desc.Height = Height;
		pTexture->m_Desc.SampleDesc.Count = 1;
		pTexture->m_Desc.SampleDesc.Quality = 0;
		pTexture->m_Desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		pTexture->m_pOwnerSystem = this;
		pTexture->Create2DCube(GetDevice(), pName);
	}
	// render targets or depth textures are considered to be loaded by default
	if (BindFlags & (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_RENDER_TARGET))
	{
		pTexture->SetState(CTextureState::eLoaded);
	}
	m_AddTextureMutex.Release();
	return *pTexture;
}


//--------------------------------------------------------------------------------
CTexture& CTextureSystem::CreateBackBuffer(IDXGISwapChain* pSwapChain, unsigned int BackBufferIndex)
{
	char Name[256];
	sprintf_s(Name, "BackBuffer_%d", BackBufferIndex);
	unsigned long long Key = m_rStringDictionary->AddString(Name);
	m_AddTextureMutex.Acquire();
	bool Exists = false;
	CTexture* pTexture = &(m_TextureAssets.AddAsset(Key, Exists));
	if (!Exists || pTexture->IsReference())
	{
		HRESULT Res;
		Res = pSwapChain->GetBuffer(BackBufferIndex, __uuidof(ID3D11Texture2D), (LPVOID*)&pTexture->m_pTexture);
		pTexture->m_pTexture->GetDesc(&pTexture->m_Desc);
		pTexture->m_pOwnerSystem = this;
	}
	m_AddTextureMutex.Release();
	pTexture->SetState(CTextureState::eLoaded);
	return *pTexture;

}

//--------------------------------------------------------------------------------
void CTextureSystem::Tick(float DeltaSec)
{	
	// C4100
	DeltaSec;
	m_TextureAssets.Tick();
}

//--------------------------------------------------------------------------------
void CTextureSystem::Startup()
{
	unsigned long long Key = m_rStringDictionary->AddString("TextureSystem.BlackTexture");
	bool Exists = false;
	m_pBlackTexture = &(m_TextureAssets.AddAsset(Key, Exists));
	m_BlackTextureData = 0;
	ZeroMemory(&m_pBlackTexture->m_Desc, sizeof(m_pBlackTexture->m_Desc));
	m_pBlackTexture->m_pOwnerSystem = this;
	m_pBlackTexture->m_Desc.ArraySize = 1;
	m_pBlackTexture->m_Desc.Width = 1;
	m_pBlackTexture->m_Desc.Height = 1;
	m_pBlackTexture->m_Desc.MipLevels = 1;
	m_pBlackTexture->m_Desc.SampleDesc.Count = 1;
	m_pBlackTexture->m_Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	m_pBlackTexture->m_Desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	m_pBlackTexture->CreateTexture2D(m_rRenderer->GetDevice(), &m_BlackTextureData, "TextureSystem.BlackTexture");

	CMaterialTextureOps::CParameters ParamsLinearTo2D("LinearTo2D", m_pBlackTexture, &GetSampler(eSamplerPoint), CMaterialTextureOps::eByteAddressBuffer, 1, 1, 0, 0);
	m_pLinearTo2DMaterial = &m_rMaterialSystem->GetMaterial<CMaterialTextureOps>(&ParamsLinearTo2D);
	m_pLinearTo2DPrimitive = &m_rDrawPrimitiveSystem->Add("LinearTo2D", 1, 1, 1, m_pLinearTo2DMaterial);

	CMaterialTextureOps::CParameters ParamsDownsampleBox2x2("DownsampleBox2x2", m_pBlackTexture, &GetSampler(eSamplerPoint), CMaterialTextureOps::eDownsampleBox2x2, 1, 1, 0, 0);
	m_pDownsampleBox2x2Material = &m_rMaterialSystem->GetMaterial<CMaterialTextureOps>(&ParamsDownsampleBox2x2);
	m_pDownsampleBox2x2Primitive = &m_rDrawPrimitiveSystem->Add("DownsampleBox2x2", 1, 1, 1, m_pDownsampleBox2x2Material);

	CMaterialTextureOps::CParameters ParamsDownsampleCubeBox2x2("DownsampleCubeBox2x2", m_pBlackTexture, &GetSampler(eSamplerPoint), CMaterialTextureOps::eDownsampleBox2x2Cube, 1, 1, 0, 0);
	m_pDownsampleCubeBox2x2Material = &m_rMaterialSystem->GetMaterial<CMaterialTextureOps>(&ParamsDownsampleCubeBox2x2);
	m_pDownsampleCubeBox2x2Primitive = &m_rDrawPrimitiveSystem->Add("DownsampleCubeBox2x2", 1, 1, 1, m_pDownsampleCubeBox2x2Material);

}

//--------------------------------------------------------------------------------
void CTextureSystem::Shutdown()
{
	m_TextureAssets.Shutdown();
}

//--------------------------------------------------------------------------------
bool CTextureSystem::IsShutdown()
{
	return	m_TextureAssets.IsShutdown();
}

//--------------------------------------------------------------------------------
void CTextureSystem::CreateSamplers(ID3D11Device* pd3dDevice)
{
// Linear sampler
	HRESULT Res = S_OK;
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11SamplerState* pSampler;
	Res = pd3dDevice->CreateSamplerState( &sampDesc, &pSampler);
	Assert(Res == S_OK);
	m_Samplers[eSamplerLinear].Initialize(pSampler);
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	Res = pd3dDevice->CreateSamplerState(&sampDesc, &pSampler);
	Assert(Res == S_OK);
	m_Samplers[eSamplerPoint].Initialize(pSampler);
	m_SamplersCreated = true;
}

//--------------------------------------------------------------------------------
CSampler& CTextureSystem::GetSampler(eSamplerTypes Type)
{
	return m_Samplers[Type];
}

//--------------------------------------------------------------------------------
void CTextureSystem::ReleaseSamplers()
{
	for(unsigned int SamplerIndex = 0; SamplerIndex < eMaxSamplers; SamplerIndex++)
	{
		m_Samplers[SamplerIndex].Release();
	}
	m_SamplersCreated = false;
}

//--------------------------------------------------------------------------------
void CTextureSystem::AcquireAddMutex()
{
	m_AddTextureMutex.Acquire();
}

//--------------------------------------------------------------------------------
void CTextureSystem::ReleaseAddMutex()
{
	m_AddTextureMutex.Release();
}

//--------------------------------------------------------------------------------
unsigned int CTextureSystem::GetFormatBPP(DXGI_FORMAT Format)
{
	switch (Format)
	{
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			return 4;
		case DXGI_FORMAT_R8_TYPELESS:
			return 1;
		default:
			Assert(0); // unknown format
	}
	return 0;
}

//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CTexture, CTextureState::eMaxTextureLoadStates, CTextureState::eStateType, CTextureState::eUnloaded, CTextureState::eReference>::Tick()
{
	// Textures could be added to the list from other threads
	m_pAddAssetMutex->Acquire();
	for (CElementEntry* pElement = m_pStages[CTextureState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CTexture* pTexture = (CTexture*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;	
		if(pTexture->IsLoaded())
		{
			m_pStages[CTextureState::eUnloaded]->Move(m_pStages[CTextureState::eLoaded], *pElement);
		}
		else if (pTexture->GetState() == CTextureState::eRawDataLoaded)
		{
			m_pStages[CTextureState::eUnloaded]->Move(m_pStages[CTextureState::eRawDataLoaded], *pElement);
		}
		pElement = pNextElement;
	}				
	m_pAddAssetMutex->Release();


	for (CElementEntry* pElement = m_pStages[CTextureState::eRawDataLoaded]->GetFirst(); pElement != NULL;)
	{
		CTexture* pTexture = (CTexture*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		if (pTexture->GenerateMipmaps())
		{
			m_pStages[CTextureState::eRawDataLoaded]->Move(m_pStages[CTextureState::eCreateMips], *pElement);
		}		
		pElement = pNextElement;
	}

	for (CElementEntry* pElement = m_pStages[CTextureState::eCreateMips]->GetFirst(); pElement != NULL;)
	{
		CTexture* pTexture = (CTexture*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		m_pStages[CTextureState::eCreateMips]->Move(m_pStages[CTextureState::eLoaded], *pElement);
		pTexture->SetState(CTextureState::eLoaded);
		pElement = pNextElement;
	}

	for(CElementEntry* pElement = m_pStages[CTextureState::eDeviceRelease]->GetFirst(); pElement != NULL; )
	{
		CTexture* pTexture = (CTexture*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		pTexture->Release();
		m_pStages[CTextureState::eDeviceRelease]->Remove(*pElement);
		pElement = pNextElement;
	}				
}

//--------------------------------------------------------------------------------
template<>
void CMultiStageAssetContainer<CTexture, CTextureState::eMaxTextureLoadStates, CTextureState::eStateType, CTextureState::eUnloaded, CTextureState::eReference>::Shutdown()
{
	Tick();
	for (CElementEntry* pElement = m_pStages[CTextureState::eUnloaded]->GetFirst(); pElement != NULL;)
	{
		CTexture* pTexture = (CTexture*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;
		m_pStages[CTextureState::eUnloaded]->Move(m_pStages[CTextureState::eDeviceRelease], *pElement);
		pTexture->SetState(CTextureState::eDeviceRelease);
		pElement = pNextElement;
	}

	for(CElementEntry* pElement = m_pStages[CTextureState::eLoaded]->GetFirst(); pElement != NULL; )
	{
		CTexture* pTexture = (CTexture*)pElement->m_pData;
		CElementEntry* pNextElement = pElement->m_pNext;		
		m_pStages[CTextureState::eLoaded]->Move(m_pStages[CTextureState::eDeviceRelease], *pElement);
		pTexture->SetState(CTextureState::eDeviceRelease);
		pElement = pNextElement;
	}			
}

//--------------------------------------------------------------------------------
bool CTextureSystem::GenerateMipmaps(CTexture* pTexture)
{
	if (m_pLinearTo2DPrimitive->IsLoaded() && m_pDownsampleBox2x2Primitive->IsLoaded())
	{		
		CMaterialTextureOps::CParameters ParamsLinearTo2D("LinearTo2D", pTexture, &GetSampler(eSamplerPoint), CMaterialTextureOps::eByteAddressBuffer, pTexture->m_Desc.Width, pTexture->m_Desc.Height, 0, 0);
		m_pLinearTo2DMaterial->Update(*m_rConstantsSystem, &ParamsLinearTo2D);
		m_pLinearTo2DPrimitive->SetThreadGroupCount(pTexture->m_Desc.Width, pTexture->m_Desc.Height, 1);
		m_rRenderer->MakeCopyAndDrawPrimitive(m_pLinearTo2DPrimitive);
		
		unsigned int Width = pTexture->m_Desc.Width;
		unsigned int Height = pTexture->m_Desc.Height;
		for (unsigned int MipLevel = 1; MipLevel < pTexture->m_Desc.MipLevels; MipLevel++)
		{
			CMaterialTextureOps::CParameters ParamsDownsampleBox2x2("DownsampleBox2x2", pTexture, &GetSampler(eSamplerPoint), CMaterialTextureOps::eDownsampleBox2x2, Width, Height, 0, MipLevel);
			Width = (Width == 1) ? Width : Width / 2;
			Height = (Height == 1) ? Height : Height / 2;

			m_pDownsampleBox2x2Material->Update(*m_rConstantsSystem, &ParamsDownsampleBox2x2);
			m_pDownsampleBox2x2Primitive->SetThreadGroupCount(Width, Height, 1);
			m_rRenderer->MakeCopyAndDrawPrimitive(m_pDownsampleBox2x2Primitive);
		}
		return true;
	}
	else
	{
		return false;	// failed because material is not ready yet.
	}
}

//--------------------------------------------------------------------------------
bool CTextureSystem::GenerateDynEnvMapMipmaps(CTexture* pTexture)
{
	if (m_pDownsampleCubeBox2x2Primitive->IsLoaded())
	{
		for (unsigned int Face = 0; Face < 6; Face++)
		{
			unsigned int Width = pTexture->m_Desc.Width;
			unsigned int Height = pTexture->m_Desc.Height;
			for (unsigned int MipLevel = 1; MipLevel < pTexture->m_Desc.MipLevels; MipLevel++)
			{
				CMaterialTextureOps::CParameters ParamsDownsampleBox2x2Cube("DownsampleBox2x2Cube", pTexture, &GetSampler(eSamplerPoint), CMaterialTextureOps::eDownsampleBox2x2Cube, Width, Height, Face, MipLevel);
				Width = (Width == 1) ? Width : Width / 2;
				Height = (Height == 1) ? Height : Height / 2;

				m_pDownsampleCubeBox2x2Material->Update(*m_rConstantsSystem, &ParamsDownsampleBox2x2Cube);
				m_pDownsampleCubeBox2x2Primitive->SetThreadGroupCount(Width, Height, 1);
				m_rRenderer->MakeCopyAndDrawPrimitive(m_pDownsampleCubeBox2x2Primitive);
			}
		}
		return true;
	}
	else
	{
		return false;	// failed because material is not ready yet.
	}
}

//--------------------------------------------------------------------------------
ID3D11Device* CTextureSystem::GetDevice()
{
	return m_rRenderer->GetDevice();
}


