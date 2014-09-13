#include "stdafx.h"
#include "Allocator.h"
#include "TextureSystem.h"

#include "RenderTargets.h"

//--------------------------------------------------------------------------
CRenderTargets::CRenderTargets()
{
}

//--------------------------------------------------------------------------
void CRenderTargets::Startup(unsigned int BackBufferWidth, unsigned int BackBufferHeight, CTextureSystem* pTextureSystem, IDXGISwapChain* pSwapChain)
{
	m_RenderTargets[eBackBuffer].m_pTexture = &pTextureSystem->CreateBackBuffer(pSwapChain, 0);
	m_RenderTargets[eBackBuffer].m_pTexture->GetRTV((ID3D11RenderTargetView**)&m_RenderTargets[eBackBuffer].m_pRTV, CTexture::eNONE, 0); 

	m_RenderTargets[eFrameBuffer].m_pTexture = &pTextureSystem->CreateTexture(DXGI_FORMAT_R16G16B16A16_TYPELESS,
		BackBufferWidth,
		BackBufferHeight,
		1,
		1, 
		D3D11_BIND_RENDER_TARGET, 
		"FrameBuffer");
	m_RenderTargets[eFrameBuffer].m_pTexture->GetRTV((ID3D11RenderTargetView**)&m_RenderTargets[eFrameBuffer].m_pRTV, CTexture::eFLOAT, 0);

	m_RenderTargets[eDepthStencil].m_pTexture = &pTextureSystem->CreateTexture(DXGI_FORMAT_R24G8_TYPELESS,
		BackBufferWidth,
		BackBufferHeight,
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL,
		"DepthStencil"); 
	m_RenderTargets[eDepthStencil].m_pTexture->GetDSV((ID3D11DepthStencilView**)&m_RenderTargets[eDepthStencil].m_pRTV, CTexture::eD24_UNORM_S8_UINT, 0);

	m_RenderTargets[eDynamicEnvironmentMap].m_pTexture = &pTextureSystem->CreateCubeTexture(DXGI_FORMAT_R8G8B8A8_TYPELESS,
		sc_DynamicEnvironmentMapWidth,
		sc_DynamicEnvironmentMapWidth,
		9,
		6,
		(D3D11_BIND_FLAG)(D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS),
		"DynamicEnvironmentMap");
	for (unsigned int FaceIndex = eDynamicEnvironmentMapFace0; FaceIndex <= eDynamicEnvironmentMapFace5; FaceIndex++)
	{
		m_RenderTargets[FaceIndex].m_pTexture = m_RenderTargets[eDynamicEnvironmentMap].m_pTexture;
		m_RenderTargets[FaceIndex].m_pTexture->GetCubeRTV((ID3D11RenderTargetView**)&m_RenderTargets[FaceIndex].m_pRTV, CTexture::eUNORM, 0, FaceIndex - eDynamicEnvironmentMapFace0);
	}

	m_RenderTargets[eDynamicEnvironmentMapDepth].m_pTexture = &pTextureSystem->CreateTexture(DXGI_FORMAT_R24G8_TYPELESS,
		sc_DynamicEnvironmentMapWidth,
		sc_DynamicEnvironmentMapHeight,
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL,
		"DynamicEnvironmentMapDepth");
	m_RenderTargets[eDynamicEnvironmentMapDepth].m_pTexture->GetDSV((ID3D11DepthStencilView**)&m_RenderTargets[eDynamicEnvironmentMapDepth].m_pRTV, CTexture::eD24_UNORM_S8_UINT, 0);
}

//--------------------------------------------------------------------------
ID3D11View* CRenderTargets::GetRTV(eRenderTargetTypes Type)
{
	return m_RenderTargets[Type].m_pRTV;
}

//--------------------------------------------------------------------------
void CRenderTargets::Shutdown()
{
	for (unsigned int RTIndex = 0; RTIndex < eMaxRenderTargets; RTIndex++)
	{
		if (m_RenderTargets[RTIndex].m_pRTV)
		{
			m_RenderTargets[RTIndex].m_pRTV->Release();
			m_RenderTargets[RTIndex].m_pRTV = NULL;
		}
	}
}



