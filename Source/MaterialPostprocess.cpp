#include "stdafx.h"
#include "ShaderPipeline.h"
#include "Macros.h"
#include "TextureSystem.h"
#include "ConstantsSystem.h"
#include "Renderer.h"

#include "MaterialPostprocess.h"

//--------------------------------------------------------------------------------------
CMaterialPostprocess::CParameters::CParameters(const char* pName, 
	CTexture* pHDRTexture, 
	CSampler* pSampler,
	CDepthStencilState* pSetDepthStencilState,
	CDepthStencilState* pUnsetDepthStencilState) : CParametersBase(pName),
m_pHDRTexture(pHDRTexture),
m_pSampler(pSampler),
m_pSetDepthStencilState(pSetDepthStencilState),
m_pUnsetDepthStencilState(pUnsetDepthStencilState)
{
}

//--------------------------------------------------------------------------------------
CMaterialPostprocess::CParameters::CParameters() : CParametersBase("Uninitialized"),
m_pHDRTexture(NULL),
m_pSampler(NULL),
m_pSetDepthStencilState(NULL),
m_pUnsetDepthStencilState(NULL)
{
}

//--------------------------------------------------------------------------------------
void CMaterialPostprocess::CParameters::Initialize(CConstantsSystem* /*pConstantsSystem*/)
{

}

//--------------------------------------------------------------------------------------
bool CMaterialPostprocess::CParameters::IsLoaded() const
{

	return true;
}

//--------------------------------------------------------------------------------------
void CMaterialPostprocess::CParameters::Update(CConstantsSystem* /*pConstantsSystem*/, CParametersBase* /*pParameters*/)
{
}

//--------------------------------------------------------------------------------------
CMaterialPostprocess::CMaterialPostprocess()
{
	SetState(CMaterialState::eUnloaded);

}

//--------------------------------------------------------------------------------------
CMaterialPostprocess::CMaterialPostprocess(CParameters* pParameters) : m_pHDRTextureSRV(NULL)
{
	m_Parameters = *pParameters;
	SetState(CMaterialState::eUnloaded);
}

//--------------------------------------------------------------------------------------
CMaterialPostprocess::~CMaterialPostprocess()
{
}

//--------------------------------------------------------------------------------------
bool CMaterialPostprocess::AllResourcesLoaded() const
{
	return m_Parameters.IsLoaded() && ShadersLoaded();
}

//----------------------------------------------------------------------------------
D3D11_INPUT_ELEMENT_DESC CMaterialPostprocess::m_VertexShaderLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//--------------------------------------------------------------------------------------
void CMaterialPostprocess::InitializeShaders(CShaderPipeline* pShaderPipeline)
{
	D3D_SHADER_MACRO Defines[CMaterialSystem::scMaxDefines];
	Defines[0].Name = NULL;
	Defines[0].Definition = NULL;
	m_pVertexShader = &pShaderPipeline->GetVertexShader("Postprocess.hlsl", "Main", Defines, m_VertexShaderLayout, sizeof(m_VertexShaderLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC));
	m_pPixelShader = &pShaderPipeline->GetPixelShader("Postprocess.hlsl", "Main", Defines);
}

//--------------------------------------------------------------------------------------
void CMaterialPostprocess::InitializeBindings(CConstantsSystem* pConstantsSystem)
{
	BindResource(m_HDRTexture, "HDRTexture");
	BindResource(m_HDRSampler, "HDRSampler");
	m_Parameters.Initialize(pConstantsSystem);
}

//--------------------------------------------------------------------------------------
void CMaterialPostprocess::Set(ID3D11DeviceContext* pDeviceContext) const
{
	m_Parameters.m_pSampler->Set(pDeviceContext, &m_HDRSampler);
	SetSRV(pDeviceContext, m_HDRTexture, m_pHDRTextureSRV);
	m_Parameters.m_pSetDepthStencilState->Set(pDeviceContext);
	m_pVertexShader->Set(pDeviceContext);
	m_pPixelShader->Set(pDeviceContext);
}

//--------------------------------------------------------------------------------------	
void CMaterialPostprocess::Unset(ID3D11DeviceContext* pDeviceContext) const
{
	m_Parameters.m_pUnsetDepthStencilState->Set(pDeviceContext);
	SetSRV(pDeviceContext, m_HDRTexture, NULL);
}

//--------------------------------------------------------------------------------------	
void CMaterialPostprocess::CreateViews()
{
	m_Parameters.m_pHDRTexture->GetSRV(&m_pHDRTextureSRV, CTexture::eFLOAT, -1);
}

//--------------------------------------------------------------------------------------	
void CMaterialPostprocess::Release()
{
	if(m_pHDRTextureSRV)
	{
		m_pHDRTextureSRV->Release();
		m_pHDRTextureSRV = NULL;
	}
	SetState(CMaterialState::eUnloaded);
}

