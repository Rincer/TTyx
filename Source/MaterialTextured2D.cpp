#include "stdafx.h"
#include "TextureSystem.h"
#include "ShaderPipeline.h"

#include "MaterialTextured2D.h"


// 2D textured material
//----------------------------------------------------------------------------------
CMaterialTextured2D::CParameters::CParameters(const char* pName,
	CTexture* pTexture,
	CSampler* pSampler,
	eTextureChannels TextureChannelsInterface) : CParametersBase(pName),
	m_pTexture(pTexture),
	m_pSampler(pSampler),
	m_TextureChannelsInterface(TextureChannelsInterface)
{
}


//----------------------------------------------------------------------------------
CMaterialTextured2D::CParameters::CParameters() : CParametersBase("Uninitialized"),
m_pTexture(NULL)
{
}

//----------------------------------------------------------------------------------
void CMaterialTextured2D::CParameters::Initialize(CConstantsSystem* pConstantsSystem)
{
	pConstantsSystem; // C4100
}

//----------------------------------------------------------------------------------
bool CMaterialTextured2D::CParameters::IsLoaded() const
{
	return m_pTexture && m_pTexture->IsLoaded();
}

//----------------------------------------------------------------------------------
D3D11_INPUT_ELEMENT_DESC CMaterialTextured2D::m_VertexShaderLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//----------------------------------------------------------------------------------
CMaterialTextured2D::CMaterialTextured2D()
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialTextured2D::CMaterialTextured2D(CParameters* pParameters) : m_Parameters(*pParameters),
m_pTextureView(NULL)
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialTextured2D::~CMaterialTextured2D()
{
}


static const char* scpTextureChannelsDefines[CMaterialTextured2D::eMaxTextureChannelInterfaces] =
{
	"TEXTURE_RGBA",
	"TEXTURE_A"
};

//----------------------------------------------------------------------------------
void CMaterialTextured2D::InitializeShaders(CShaderPipeline* pShaderPipeline)
{
	m_pVertexShader = &pShaderPipeline->GetVertexShader("Utility.hlsl", "Textured2D_VS", NULL, m_VertexShaderLayout, sizeof(m_VertexShaderLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC));
	D3D_SHADER_MACRO Defines[CMaterialSystem::scMaxDefines];
	Defines[0].Name = scpTextureChannelsDefines[m_Parameters.m_TextureChannelsInterface];
	Defines[0].Definition = "1";
	Defines[1].Name = NULL;
	Defines[1].Definition = NULL;
	m_pPixelShader = &pShaderPipeline->GetPixelShader("Utility.hlsl", "Textured_PS", Defines);
}

//----------------------------------------------------------------------------------
void CMaterialTextured2D::InitializeBindings(CConstantsSystem* pConstantsSystem)
{
	pConstantsSystem; // C4100
	BindResource(m_Texture, "Texture");	// Texture                          
	BindResource(m_Sampler, "Sampler");	// Sampler                         
}

//----------------------------------------------------------------------------------
void CMaterialTextured2D::Set(ID3D11DeviceContext* pDeviceContext) const
{
	SetSRV(pDeviceContext, m_Texture, m_pTextureView);
	m_Parameters.m_pSampler->Set(pDeviceContext, &m_Sampler);

	m_pVertexShader->Set(pDeviceContext);
	m_pPixelShader->Set(pDeviceContext);
}

//----------------------------------------------------------------------------------
bool CMaterialTextured2D::AllResourcesLoaded() const
{
	return m_Parameters.IsLoaded() && ShadersLoaded() && (m_Texture.m_ResourceInfo[ePixelShader].m_NumViews > 0) && (m_Sampler.m_ResourceInfo[ePixelShader].m_NumViews > 0);
}

//----------------------------------------------------------------------------------
void CMaterialTextured2D::CreateViews()
{
	m_Parameters.m_pTexture->GetSRV(&m_pTextureView, CTexture::eUNORM, -1);
}

//----------------------------------------------------------------------------------
void CMaterialTextured2D::Release()
{
	if(m_pTextureView)
	{
		m_pTextureView->Release();
		m_pTextureView = NULL;
	}
	SetState(CMaterialState::eUnloaded);
}
