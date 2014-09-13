#include "stdafx.h"
#include "ShaderPipeline.h"

#include "MaterialPlainColor.h"


//-------------------------------------------Plain----------------------------------
//----------------------------------------------------------------------------------
CMaterialPlainColor::CMaterialPlainColor()
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialPlainColor::CMaterialPlainColor(CParameters* pParameters) : m_Parameters(*pParameters)
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialPlainColor::~CMaterialPlainColor()
{
}

//----------------------------------------------------------------------------------
void CMaterialPlainColor::Release()
{
	SetState(CMaterialState::eUnloaded);
}

//----------------------------------------------------------------------------------
CMaterialPlainColor::CParameters::CParameters() : CParametersBase("Uninitialized")
{
}

//----------------------------------------------------------------------------------
CMaterialPlainColor::CParameters::CParameters(const char* pName, eTransformSpace TransformSpace) : CParametersBase(pName),
m_TransformSpace(TransformSpace)
{
}

//----------------------------------------------------------------------------------
void CMaterialPlainColor::CParameters::Initialize(CConstantsSystem* pConstantsSystem)
{
	pConstantsSystem; // C4100
}

//----------------------------------------------------------------------------------
bool CMaterialPlainColor::CParameters::IsLoaded() const
{
	return true;
}

//----------------------------------------------------------------------------------
D3D11_INPUT_ELEMENT_DESC CMaterialPlainColor::m_VertexShaderLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static const char* scpTransformSpace[CMaterialPlainColor::eMaxTransformSpaces] =
{
	"WORLDSPACE",
	"VIEWSPACE"
};

//----------------------------------------------------------------------------------
void CMaterialPlainColor::InitializeShaders(CShaderPipeline* pShaderPipeline)
{
	D3D_SHADER_MACRO Defines[CMaterialSystem::scMaxDefines];
	Defines[0].Name = scpTransformSpace[m_Parameters.m_TransformSpace];
	Defines[0].Definition = "1";
	Defines[1].Name = NULL;
	Defines[1].Definition = NULL;
	m_pVertexShader = &pShaderPipeline->GetVertexShader("Utility.hlsl", "PlainColor_VS", Defines, m_VertexShaderLayout, sizeof(m_VertexShaderLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC));
	m_pPixelShader = &pShaderPipeline->GetPixelShader("Utility.hlsl", "PlainColor_PS", NULL);
}

//----------------------------------------------------------------------------------
void CMaterialPlainColor::InitializeBindings(CConstantsSystem* pConstantsSystem)
{
	pConstantsSystem; // C4100
}

bool CMaterialPlainColor::AllResourcesLoaded() const
{
	return ShadersLoaded();
}

//----------------------------------------------------------------------------------
void CMaterialPlainColor::Set(ID3D11DeviceContext* pDeviceContext) const
{
	m_pVertexShader->Set(pDeviceContext);
	m_pPixelShader->Set(pDeviceContext);
}
