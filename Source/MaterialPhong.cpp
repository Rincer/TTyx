#include "stdafx.h"
#include "TextureSystem.h"
#include "ConstantsSystem.h"
#include "Macros.h"
#include "ShaderPipeline.h"

#include "MaterialPhong.h"


//-------------------------------------------Phong----------------------------------
//----------------------------------------------------------------------------------
CMaterialPhong::CParameters::CParameters() : CParametersBase("Uninitialized"),
m_pmap_Kd(NULL),
m_pmap_Ks(NULL),
m_pmap_d(NULL),
m_pmap_bump(NULL),
m_pCBuffer(NULL)

{
}

//----------------------------------------------------------------------------------
CMaterialPhong::CParameters::CParameters(const char* pName,
	CTexture* pmap_Kd,
	CTexture* pmap_Ks,
	CTexture* pmap_d,
	CTexture* pmap_bump,
	CSampler* pKdSampler,
	CSampler* pKsSampler,
	CSampler* pdSampler,
	CSampler* pBumpSampler,
	eTransform TransformInterface,
	eLocalToWorldTransform LocalToWorldInterface,
	XMVECTOR DiffuseParams,
	XMVECTOR SpecularParams) :
	CParametersBase(pName),
	m_pmap_Kd(pmap_Kd),
	m_pmap_Ks(pmap_Ks),
	m_pmap_d(pmap_d),
	m_pmap_bump(pmap_bump),
	m_pKdSampler(pKdSampler),
	m_pKsSampler(pKsSampler),
	m_pdSampler(pdSampler),
	m_pBumpSampler(pBumpSampler),
	m_pCBuffer(NULL),
	m_TransformInterface(TransformInterface),
	m_LocalToWorldInterface(LocalToWorldInterface)
{
	m_PhongParams.m_DiffuseParams = DiffuseParams.m128_f32;
	m_PhongParams.m_SpecularParams = SpecularParams.m128_f32;
	m_DiffuseInterface = pmap_Kd ? CMaterialPhong::eTexturedDiffuse : CMaterialPhong::eUnTexturedDiffuse;
	m_OpacityInterface = pmap_d ? CMaterialPhong::eTexturedOpacity : CMaterialPhong::eUnTexturedOpacity;
	m_NormalInterface = pmap_bump ? CMaterialPhong::eNormalMapped : CMaterialPhong::eVertexNormal;
	XMVECTORF32 Spec;
	Spec.v = SpecularParams;
	if (((Spec.f[0] == 0.0f) && (Spec.f[1] == 0.0f) && (Spec.f[2] == 0.0f)) || (Spec.f[3] == 0.0f))
	{
		m_SpecularInterface = CMaterialPhong::eZeroSpecular;
	}
	else
	{
		m_SpecularInterface = pmap_Ks ? CMaterialPhong::eTexturedSpecular : CMaterialPhong::eUnTexturedSpecular;
	}
}


D3D11_INPUT_ELEMENT_DESC  CMaterialPhong::m_VertexShaderObjLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONEWEIGHTS", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 4, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

//----------------------------------------------------------------------------------
void CMaterialPhong::CParameters::Initialize(CConstantsSystem* pConstantsSystem)
{
	char ParamName[256];
	sprintf_s(ParamName, 255, "CommandBuffer.MaterialPhong.%s", m_Name);
	m_pCBuffer = pConstantsSystem->CreateCBuffer(&m_PhongParams, sizeof(cbObjParams), ParamName);
}

//----------------------------------------------------------------------------------
bool CMaterialPhong::CParameters::IsLoaded() const
{
	if (!m_pCBuffer || !m_pCBuffer->IsLoaded())
	{
		return false;
	}
	if (m_pmap_Kd && !m_pmap_Kd->IsLoaded())
	{
		return false;
	}
	if (m_pmap_Ks && !m_pmap_Ks->IsLoaded())
	{
		return false;
	}
	if (m_pmap_d && !m_pmap_d->IsLoaded())
	{
		return false;
	}
	if (m_pmap_bump && !m_pmap_bump->IsLoaded())
	{
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------
CMaterialPhong::CMaterialPhong()
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialPhong::CMaterialPhong(CParameters* pParameters) : 	m_pDiffuse(NULL),
m_pSpecular(NULL),
m_pAlpha(NULL),
m_pNormal(NULL),
m_UseDynamicLinkage(false)

{
	m_Parameters = *pParameters;
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
bool CMaterialPhong::AllResourcesLoaded() const
{
	return m_Parameters.IsLoaded() && ShadersLoaded();
}

//----------------------------------------------------------------------------------
CMaterialPhong::~CMaterialPhong()
{
}

//----------------------------------------------------------------------------------
void CMaterialPhong::CreateViews()
{
	if (m_Parameters.m_pmap_Kd)
		m_Parameters.m_pmap_Kd->GetSRV(&m_pDiffuse, CTexture::eUNORM_SRGB, -1);
	if (m_Parameters.m_pmap_Ks)
		m_Parameters.m_pmap_Ks->GetSRV(&m_pSpecular, CTexture::eUNORM_SRGB, -1);
	if (m_Parameters.m_pmap_d)
		m_Parameters.m_pmap_d->GetSRV(&m_pAlpha, CTexture::eUNORM, -1);
	if (m_Parameters.m_pmap_bump)
		m_Parameters.m_pmap_bump->GetSRV(&m_pNormal, CTexture::eUNORM, -1);
}

//----------------------------------------------------------------------------------
void CMaterialPhong::Release()
{
	if (m_UseDynamicLinkage)
	{
		for (unsigned int TransformIndex = 0; TransformIndex < eMaxTransforms; TransformIndex++)
		{
			m_TransformInstances[TransformIndex].m_pClassInstance->Release();
		}
		for (unsigned int TransformIndex = 0; TransformIndex < eMaxLocalToWorldTransforms; TransformIndex++)
		{
			m_LocalToWorldInstances[TransformIndex].m_pClassInstance->Release();
		}
		for (unsigned int DiffuseIndex = 0; DiffuseIndex < eMaxDiffuseInstances; DiffuseIndex++)
		{
			m_DiffuseInstances[DiffuseIndex].m_pClassInstance->Release();
		}
		for (unsigned int OpacityIndex = 0; OpacityIndex < eMaxOpacityInstances; OpacityIndex++)
		{
			m_OpacityInstances[OpacityIndex].m_pClassInstance->Release();
		}
		for (unsigned int NormalIndex = 0; NormalIndex < eMaxNormalInstances; NormalIndex++)
		{
			m_NormalInstances[NormalIndex].m_pClassInstance->Release();
		}
		for (unsigned int SpecularIndex = 0; SpecularIndex < eMaxSpecularInstances; SpecularIndex++)
		{
			m_SpecularInstances[SpecularIndex].m_pClassInstance->Release();
		}
	}
	if(m_pDiffuse)
	{
		m_pDiffuse->Release();
		m_pDiffuse = NULL;
	}
	if(m_pSpecular)
	{
		m_pSpecular->Release();
		m_pSpecular = NULL;
	}
	if(m_pAlpha)
	{
		m_pAlpha->Release();
		m_pAlpha = NULL;
	}
	if(m_pNormal)
	{
		m_pNormal->Release();
		m_pNormal = NULL;
	}
	SetState(CMaterialState::eUnloaded);
}


static const char* scpDynamicLinkageDefine = "USE_DYNAMIC_LINKAGE";


static const char* scpTransformDefines[CMaterialPhong::eMaxTransforms] =
{
	"UNSKINNED",
	"SKINNED",
};

static const char* scpLocalToWorldDefines[CMaterialPhong::eMaxLocalToWorldTransforms] =
{
	"WORLDSPACE",
	"LOCALSPACE",
};

static const char* scpDiffuseDefines[CMaterialPhong::eMaxDiffuseInstances] =
{
	"DIFFUSE_TEXTURED",
	"DIFFUSE_UNTEXTURED",
};

static const char* scpOpacityDefines[CMaterialPhong::eMaxOpacityInstances] =
{
	"OPACITY_TEXTURED",
	"OPACITY_UNTEXTURED",
};

static const char* scpNormalDefines[CMaterialPhong::eMaxNormalInstances] =
{
	"NORMALMAPPED",
	"VERTEXNORMAL",
};

static const char* scpSpecularDefines[CMaterialPhong::eMaxSpecularInstances] =
{
	"SPECULAR_ZERO",
	"SPECULAR_TEXTURED",
	"SPECULAR_UNTEXTURED"
};

//----------------------------------------------------------------------------------
void CMaterialPhong::InitializeShaders(CShaderPipeline* pShaderPipeline)
{
	D3D_SHADER_MACRO Defines[CMaterialSystem::scMaxDefines];
	if (m_UseDynamicLinkage)
	{
		Defines[0].Name = scpDynamicLinkageDefine;
		Defines[0].Definition = "1";
		Defines[1].Name = NULL;
		Defines[1].Definition = NULL;
	}
	else
	{
		Defines[0].Name = scpDynamicLinkageDefine;
		Defines[0].Definition = "0";
		Defines[1].Name = scpTransformDefines[m_Parameters.m_TransformInterface];
		Defines[1].Definition = "1";
		Defines[2].Name = scpLocalToWorldDefines[m_Parameters.m_LocalToWorldInterface];
		Defines[2].Definition = "1";
		Defines[3].Name = NULL;
		Defines[3].Definition = NULL;
	}
	m_pVertexShader = &pShaderPipeline->GetVertexShader("Phong.hlsl", "VS", Defines, m_VertexShaderObjLayout, sizeof(m_VertexShaderObjLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC));

	if (m_UseDynamicLinkage)
	{
		Defines[0].Name = scpDynamicLinkageDefine;
		Defines[0].Definition = "1";
		Defines[1].Name = NULL;
		Defines[1].Definition = NULL;
	}
	else
	{
		Defines[0].Name = scpDynamicLinkageDefine;
		Defines[0].Definition = "0";
		Defines[1].Name = scpDiffuseDefines[m_Parameters.m_DiffuseInterface];
		Defines[1].Definition = "1";
		Defines[2].Name = scpOpacityDefines[m_Parameters.m_OpacityInterface];
		Defines[2].Definition = "1";
		Defines[3].Name = scpNormalDefines[m_Parameters.m_NormalInterface];
		Defines[3].Definition = "1";
		Defines[4].Name = scpSpecularDefines[m_Parameters.m_SpecularInterface];
		Defines[4].Definition = "1";
		Defines[5].Name = NULL;
		Defines[5].Definition = NULL;
	}
	m_pPixelShader = &pShaderPipeline->GetPixelShader("Phong.hlsl", "PS", Defines);
}

//----------------------------------------------------------------------------------
void CMaterialPhong::InitializeBindings(CConstantsSystem* pConstantsSystem)
{
	BindResource(m_map_Kd, "map_Kd");	// Diffuse map                           
	BindResource(m_map_Ks, "map_Ks");	// Specular map
	BindResource(m_map_d, "map_d");	// Alpha map
	BindResource(m_map_bump, "map_bump");// Bump	map

	BindResource(m_sam_Kd, "sam_Kd");	// Diffuse sampler                           
	BindResource(m_sam_Ks, "sam_Ks");	// Specular sampler
	BindResource(m_sam_d, "sam_d");	// Alpha sampler
	BindResource(m_sam_bump, "sam_bump");// Bump	sampler	

	BindResource(m_ShaderConstants, "cbObjParams"); // Obj constant parameters

	if (m_UseDynamicLinkage)
	{
		m_TransformInstances[eUnskinned].m_InterfaceSlot = m_pVertexShader->GetInterfaceSlot("gBaseTransform");
		m_pVertexShader->GetClassInstance(&m_TransformInstances[eUnskinned].m_pClassInstance, "gUnSkinnedTransform");
		m_TransformInstances[eSkinned].m_InterfaceSlot = m_pVertexShader->GetInterfaceSlot("gBaseTransform");
		m_pVertexShader->GetClassInstance(&m_TransformInstances[eSkinned].m_pClassInstance, "gSkinnedTransform");

		m_LocalToWorldInstances[eWorldPos].m_InterfaceSlot = m_pVertexShader->GetInterfaceSlot("gLocalToWorldTransform");
		m_pVertexShader->GetClassInstance(&m_LocalToWorldInstances[eWorldPos].m_pClassInstance, "gWorldTransform");
		m_LocalToWorldInstances[eLocalPos].m_InterfaceSlot = m_pVertexShader->GetInterfaceSlot("gLocalToWorldTransform");
		m_pVertexShader->GetClassInstance(&m_LocalToWorldInstances[eLocalPos].m_pClassInstance, "gLocalTransform");

		m_DiffuseInstances[eUnTexturedDiffuse].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseDiffuse");
		m_pPixelShader->GetClassInstance(&m_DiffuseInstances[eUnTexturedDiffuse].m_pClassInstance, "gUnTexturedDiffuse");
		m_DiffuseInstances[eTexturedDiffuse].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseDiffuse");
		m_pPixelShader->GetClassInstance(&m_DiffuseInstances[eTexturedDiffuse].m_pClassInstance, "gTexturedDiffuse");

		m_OpacityInstances[eUnTexturedOpacity].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseOpacity");
		m_pPixelShader->GetClassInstance(&m_OpacityInstances[eUnTexturedOpacity].m_pClassInstance, "gUnTexturedOpacity");
		m_OpacityInstances[eTexturedOpacity].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseOpacity");
		m_pPixelShader->GetClassInstance(&m_OpacityInstances[eTexturedOpacity].m_pClassInstance, "gTexturedOpacity");

		m_NormalInstances[eNormalMapped].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseNormal");
		m_pPixelShader->GetClassInstance(&m_NormalInstances[eNormalMapped].m_pClassInstance, "gNormalMapped");
		m_NormalInstances[eVertexNormal].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseNormal");
		m_pPixelShader->GetClassInstance(&m_NormalInstances[eVertexNormal].m_pClassInstance, "gVertexNormal");

		m_SpecularInstances[eZeroSpecular].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseSpecular");
		m_pPixelShader->GetClassInstance(&m_SpecularInstances[eZeroSpecular].m_pClassInstance, "gZeroSpecular");
		m_SpecularInstances[eTexturedSpecular].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseSpecular");
		m_pPixelShader->GetClassInstance(&m_SpecularInstances[eTexturedSpecular].m_pClassInstance, "gTexturedSpecular");
		m_SpecularInstances[eUnTexturedSpecular].m_InterfaceSlot = m_pPixelShader->GetInterfaceSlot("gBaseSpecular");
		m_pPixelShader->GetClassInstance(&m_SpecularInstances[eUnTexturedSpecular].m_pClassInstance, "gUnTexturedSpecular");
	}
	m_Parameters.Initialize(pConstantsSystem);
}

//----------------------------------------------------------------------------------
void CMaterialPhong::Set(ID3D11DeviceContext* pDeviceContext) const
{
	if (m_Parameters.m_pmap_Kd)
		SetSRV(pDeviceContext, m_map_Kd, m_pDiffuse);
	if (m_Parameters.m_pmap_Ks)
		SetSRV(pDeviceContext, m_map_Ks, m_pSpecular);
	if (m_Parameters.m_pmap_d)
		SetSRV(pDeviceContext, m_map_d, m_pAlpha);
	if (m_Parameters.m_pmap_bump)
		SetSRV(pDeviceContext, m_map_bump, m_pNormal);

	m_Parameters.m_pKdSampler->Set(pDeviceContext, &m_sam_Kd);
	m_Parameters.m_pKsSampler->Set(pDeviceContext, &m_sam_Ks);
	m_Parameters.m_pdSampler->Set(pDeviceContext, &m_sam_d);
	m_Parameters.m_pBumpSampler->Set(pDeviceContext, &m_sam_bump);

	m_Parameters.m_pCBuffer->Set(pDeviceContext, &m_ShaderConstants);

	if (m_UseDynamicLinkage)
	{
		const CClassInstanceInfo* pClassInstanceInfo = &m_TransformInstances[m_Parameters.m_TransformInterface];
		m_pVertexShader->SetClassInstance(pClassInstanceInfo->m_InterfaceSlot, pClassInstanceInfo->m_pClassInstance);
		pClassInstanceInfo = &m_LocalToWorldInstances[m_Parameters.m_LocalToWorldInterface];
		m_pVertexShader->SetClassInstance(pClassInstanceInfo->m_InterfaceSlot, pClassInstanceInfo->m_pClassInstance);

		pClassInstanceInfo = &m_DiffuseInstances[m_Parameters.m_DiffuseInterface];
		m_pPixelShader->SetClassInstance(pClassInstanceInfo->m_InterfaceSlot, pClassInstanceInfo->m_pClassInstance);
		pClassInstanceInfo = &m_OpacityInstances[m_Parameters.m_OpacityInterface];
		m_pPixelShader->SetClassInstance(pClassInstanceInfo->m_InterfaceSlot, pClassInstanceInfo->m_pClassInstance);
		pClassInstanceInfo = &m_NormalInstances[m_Parameters.m_NormalInterface];
		m_pPixelShader->SetClassInstance(pClassInstanceInfo->m_InterfaceSlot, pClassInstanceInfo->m_pClassInstance);
		pClassInstanceInfo = &m_SpecularInstances[m_Parameters.m_SpecularInterface];
		m_pPixelShader->SetClassInstance(pClassInstanceInfo->m_InterfaceSlot, pClassInstanceInfo->m_pClassInstance);
	}
	m_pVertexShader->Set(pDeviceContext);
	m_pPixelShader->Set(pDeviceContext);
}
