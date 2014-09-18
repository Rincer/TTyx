#include "stdafx.h"
#include "ShaderPipeline.h"
#include "TextureSystem.h"
#include "ConstantsSystem.h"

#include "MaterialExperimental.h"


//-------------------------------------------Experimental---------------------------
//----------------------------------------------------------------------------------
CMaterialExperimental::CMaterialExperimental()
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialExperimental::CMaterialExperimental(CParameters* pParameters) : m_Parameters(*pParameters),
m_pDynEnvMapSRV(NULL)
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialExperimental::~CMaterialExperimental()
{
}

//----------------------------------------------------------------------------------
void CMaterialExperimental::Release()
{
	if(m_pDynEnvMapSRV)
	{
		m_pDynEnvMapSRV->Release();
		m_pDynEnvMapSRV = NULL;
	}
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialExperimental::CParameters::CParameters() : CParametersBase("Uninitialized"),
m_pDynEnvMap(NULL),
m_pDynEnvMapSampler(NULL),
m_pCBuffer(NULL)
{
}

//----------------------------------------------------------------------------------
CMaterialExperimental::CParameters::CParameters(const char* pName, CTexture* DynEnvMap, CSampler* DynEnvMapSampler, XMFLOAT4 BaseColor, XMFLOAT4 RSM) : CParametersBase(pName),
m_pDynEnvMap(DynEnvMap),
m_pDynEnvMapSampler(DynEnvMapSampler),
m_pCBuffer(NULL)
{
	m_BrdfParams.BaseColor = BaseColor;
	m_BrdfParams.RSM = RSM;
}

//----------------------------------------------------------------------------------
void CMaterialExperimental::CParameters::Initialize(CConstantsSystem* pConstantsSystem)
{
	char ParamName[256];
	sprintf_s(ParamName, 255, "CommandBuffer.MaterialExperimental.%s", m_Name);
	m_pCBuffer = pConstantsSystem->CreateCBuffer(&m_BrdfParams, sizeof(cbBrdfParams), ParamName);
}

//----------------------------------------------------------------------------------
bool CMaterialExperimental::CParameters::IsLoaded() const
{
	return m_pDynEnvMap && m_pDynEnvMap->IsLoaded() && m_pCBuffer && m_pCBuffer->IsLoaded();	
}

//----------------------------------------------------------------------------------
D3D11_INPUT_ELEMENT_DESC CMaterialExperimental::m_VertexShaderLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//----------------------------------------------------------------------------------
void CMaterialExperimental::InitializeShaders(CShaderPipeline* pShaderPipeline)
{
	D3D_SHADER_MACRO Defines[CMaterialSystem::scMaxDefines];
	Defines[0].Name = NULL;
	Defines[0].Definition = NULL;
	m_pVertexShader = &pShaderPipeline->GetVertexShader("Experimental.hlsl", "Main", Defines, m_VertexShaderLayout, sizeof(m_VertexShaderLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC));
	m_pPixelShader = &pShaderPipeline->GetPixelShader("Experimental.hlsl", "Main", NULL);
}

//----------------------------------------------------------------------------------
void CMaterialExperimental::InitializeBindings(CConstantsSystem* pConstantsSystem)
{
	BindResource(m_ShaderConstants, "cbBrdfParams"); // brdf parameters
	BindResource(m_DynEnvMap, "DynEnvMap");	// Diffuse map                           
	BindResource(m_DynEnvMapSampler, "SamplerDynEnvMap");	// Diffuse map      
	m_Parameters.Initialize(pConstantsSystem);
}

//----------------------------------------------------------------------------------
bool CMaterialExperimental::AllResourcesLoaded() const
{
	return ShadersLoaded() && m_Parameters.IsLoaded();
}

//----------------------------------------------------------------------------------
void CMaterialExperimental::CreateViews()
{
	m_Parameters.m_pDynEnvMap->GetCubeSRV(&m_pDynEnvMapSRV, CTexture::eUNORM, -1, 0);
}

//----------------------------------------------------------------------------------
void CMaterialExperimental::Set(ID3D11DeviceContext* pDeviceContext) const
{		
	SetSRV(pDeviceContext, m_DynEnvMap, m_pDynEnvMapSRV);
	m_Parameters.m_pCBuffer->Set(pDeviceContext, &m_ShaderConstants);
	m_Parameters.m_pDynEnvMapSampler->Set(pDeviceContext, &m_DynEnvMapSampler);
	m_pVertexShader->Set(pDeviceContext);
	m_pPixelShader->Set(pDeviceContext);
}

//----------------------------------------------------------------------------------
void CMaterialExperimental::Unset(ID3D11DeviceContext* pDeviceContext) const
{
	ID3D11ShaderResourceView* pSRV[1] = { NULL };
	pDeviceContext->PSSetShaderResources(m_DynEnvMap.m_ResourceInfo[ePixelShader].m_StartSlot, m_DynEnvMap.m_ResourceInfo[ePixelShader].m_NumViews, pSRV);
}