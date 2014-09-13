#include "stdafx.h"
#include "ShaderPipeline.h"
#include "Macros.h"
#include "TextureSystem.h"
#include "ConstantsSystem.h"

#include "MaterialTextureOps.h"

//--------------------------------------------------------------------------------------
CMaterialTextureOps::CParameters::CParameters(const char* pName, CTexture* pTexture, CSampler* pSampler, eSamplerOp SamplerOp, unsigned int Width, unsigned int Height, unsigned int Face, unsigned int DstMipLevel) : CParametersBase(pName),
m_pTexture(pTexture),
m_pSampler(pSampler),
m_SamplerOp(SamplerOp),
m_Face(Face),
m_DstMipLevel(DstMipLevel),
m_pCBuffer(NULL)
{
	m_TextureParams.m_PixelsToUVs[0] = 1.0f / Width;	
	m_TextureParams.m_PixelsToUVs[1] = 1.0f / Height;
	m_TextureParams.m_PixelsToUVs[2] = (float)Width;
}

//--------------------------------------------------------------------------------------
CMaterialTextureOps::CParameters::CParameters() : CParametersBase("Uninitialized"),
m_pTexture(NULL),
m_pCBuffer(NULL)
{
}

//--------------------------------------------------------------------------------------
void CMaterialTextureOps::CParameters::Initialize(CConstantsSystem* pConstantsSystem)
{
	char ParamName[256];
	sprintf_s(ParamName, 255, "CommandBuffer.MaterialTextureOps.%s", m_Name);
	m_pCBuffer = pConstantsSystem->CreateCBuffer(&m_TextureParams, sizeof(cbTextureParams), ParamName);	
}

//--------------------------------------------------------------------------------------
bool CMaterialTextureOps::CParameters::IsLoaded() const
{
	if ((!m_pTexture) || (!m_pTexture->IsLoaded()))
	{
		return false;
	}
	if ((!m_pCBuffer) || (!m_pCBuffer->IsLoaded()))
	{
		return false;
	}
	return true;
}

//--------------------------------------------------------------------------------------
void CMaterialTextureOps::CParameters::Update(CConstantsSystem* pConstantsSystem, CParametersBase* pParameters)
{
	CMaterialTextureOps::CParameters* pParams = (CMaterialTextureOps::CParameters*)pParameters;
	Assert(m_SamplerOp == pParams->m_SamplerOp);
	m_TextureParams = pParams->m_TextureParams;
	m_pTexture = pParams->m_pTexture;
	m_DstMipLevel = pParams->m_DstMipLevel;
	m_Face = pParams->m_Face;
	pConstantsSystem->UpdateConstantBuffer(m_pCBuffer, &m_TextureParams.m_PixelsToUVs, sizeof(m_TextureParams.m_PixelsToUVs));
}

//--------------------------------------------------------------------------------------
CMaterialTextureOps::CMaterialTextureOps()
{
	SetState(CMaterialState::eUnloaded);

}

//--------------------------------------------------------------------------------------
CMaterialTextureOps::CMaterialTextureOps(CParameters* pParameters)
{
	m_Parameters = *pParameters;
	SetState(CMaterialState::eUnloaded);
}

//--------------------------------------------------------------------------------------
CMaterialTextureOps::~CMaterialTextureOps()
{
}

//--------------------------------------------------------------------------------------
bool CMaterialTextureOps::AllResourcesLoaded() const
{
	return m_Parameters.IsLoaded() && ShadersLoaded();
}

//--------------------------------------------------------------------------------------
static const char* scpSamplerOpDefines[CMaterialTextureOps::eMaxSamplerOp] =
{
	"BYTEADDRESS_TO_TEXTURE",
	"DOWNSAMPLE_2x2BOX",
	"DOWNSAMPLECUBE_2x2BOX"
};

//--------------------------------------------------------------------------------------
void CMaterialTextureOps::InitializeShaders(CShaderPipeline* pShaderPipeline)
{
	D3D_SHADER_MACRO Defines[CMaterialSystem::scMaxDefines];
	Defines[0].Name = scpSamplerOpDefines[m_Parameters.m_SamplerOp];
	Defines[0].Definition = "1";
	Defines[1].Name = NULL;
	Defines[1].Definition = NULL;
	m_pComputeShader = &pShaderPipeline->GetComputeShader("TextureOps.hlsl", "CSMain", Defines);
}

//--------------------------------------------------------------------------------------
void CMaterialTextureOps::InitializeBindings(CConstantsSystem* pConstantsSystem)
{
	BindResource(m_TextureParams, "TextureParams");
	BindResource(m_SrcBuffer,  "LinearData");
	BindResource(m_SrcSampler, "InSampler");
	BindResource(m_DstTexture, "OutTexture");
	BindResource(m_SrcTexture, "InTexture");
	BindResource(m_DstTextureCube, "OutTextureCube");
	BindResource(m_SrcTextureCube, "InTextureCube");
	m_Parameters.Initialize(pConstantsSystem);
}

//--------------------------------------------------------------------------------------
void CMaterialTextureOps::Set(ID3D11DeviceContext* pDeviceContext) const
{
	m_Parameters.m_pCBuffer->Set(pDeviceContext, &m_TextureParams);
	ID3D11ShaderResourceView* pSrcSRV[1] = { NULL };
	ID3D11UnorderedAccessView* pDstUAV[1] = { NULL };
	switch(m_Parameters.m_SamplerOp)
	{
		case eByteAddressBuffer:
		{
			m_Parameters.m_pTexture->GetRawDataSRV(&pSrcSRV[0]);
			m_Parameters.m_pTexture->GetUAV(&pDstUAV[0], CTexture::eUNORM, m_Parameters.m_DstMipLevel);
			pDeviceContext->CSSetShaderResources(	m_SrcBuffer.m_ResourceInfo[eComputeShader].m_StartSlot, 
													m_SrcBuffer.m_ResourceInfo[eComputeShader].m_NumViews, 
													pSrcSRV);
			pDeviceContext->CSSetUnorderedAccessViews(	m_DstTexture.m_ResourceInfo[eComputeShader].m_StartSlot,
														m_DstTexture.m_ResourceInfo[eComputeShader].m_NumViews,
														pDstUAV,
														NULL);
			pSrcSRV[0]->Release();
			pDstUAV[0]->Release();
			break;
		}

		case eDownsampleBox2x2:
		{
			m_Parameters.m_pSampler->Set(pDeviceContext, &m_SrcSampler);
			m_Parameters.m_pTexture->GetSRV(&pSrcSRV[0], CTexture::eUNORM, m_Parameters.m_DstMipLevel - 1);
			m_Parameters.m_pTexture->GetUAV(&pDstUAV[0], CTexture::eUNORM, m_Parameters.m_DstMipLevel);
			pDeviceContext->CSSetShaderResources(m_SrcTexture.m_ResourceInfo[eComputeShader].m_StartSlot,
				m_SrcTexture.m_ResourceInfo[eComputeShader].m_NumViews,
				pSrcSRV);
			pDeviceContext->CSSetUnorderedAccessViews(m_DstTexture.m_ResourceInfo[eComputeShader].m_StartSlot,
				m_DstTexture.m_ResourceInfo[eComputeShader].m_NumViews,
				pDstUAV,
				NULL);
			pSrcSRV[0]->Release();
			pDstUAV[0]->Release();
			break;
		}


		case eDownsampleBox2x2Cube:
		{
			m_Parameters.m_pSampler->Set(pDeviceContext, &m_SrcSampler);
			m_Parameters.m_pTexture->GetCubeSRV(&pSrcSRV[0], CTexture::eUNORM, m_Parameters.m_DstMipLevel - 1, m_Parameters.m_Face);
			m_Parameters.m_pTexture->GetCubeUAV(&pDstUAV[0], CTexture::eUNORM, m_Parameters.m_DstMipLevel, m_Parameters.m_Face);
			pDeviceContext->CSSetShaderResources(m_SrcTextureCube.m_ResourceInfo[eComputeShader].m_StartSlot,
				m_SrcTextureCube.m_ResourceInfo[eComputeShader].m_NumViews,
				pSrcSRV);
			pDeviceContext->CSSetUnorderedAccessViews(m_DstTextureCube.m_ResourceInfo[eComputeShader].m_StartSlot,
				m_DstTextureCube.m_ResourceInfo[eComputeShader].m_NumViews,
				pDstUAV,
				NULL);
			pSrcSRV[0]->Release();
			pDstUAV[0]->Release();

			break;
		}

		default:
		{
			Assert(0);
			break;
		}
	}
	m_pComputeShader->Set(pDeviceContext);
}

//--------------------------------------------------------------------------------------	
void CMaterialTextureOps::Unset(ID3D11DeviceContext* pDeviceContext) const
{
	ID3D11UnorderedAccessView*	pUAVs[1] = { NULL };
	ID3D11ShaderResourceView*	pSRVs[1] = { NULL };
	switch (m_Parameters.m_SamplerOp)
	{
		case eByteAddressBuffer:
		case eDownsampleBox2x2:
		{
			pDeviceContext->CSSetUnorderedAccessViews(m_DstTexture.m_ResourceInfo[eComputeShader].m_StartSlot,
				m_DstTexture.m_ResourceInfo[eComputeShader].m_NumViews,
				pUAVs,
				NULL);
			break;
		}

		case eDownsampleBox2x2Cube:
		{
			pDeviceContext->CSSetUnorderedAccessViews(m_DstTextureCube.m_ResourceInfo[eComputeShader].m_StartSlot,
				m_DstTextureCube.m_ResourceInfo[eComputeShader].m_NumViews,
				pUAVs,
				NULL);

			pDeviceContext->CSSetShaderResources(m_SrcTextureCube.m_ResourceInfo[eComputeShader].m_StartSlot,
				m_SrcTextureCube.m_ResourceInfo[eComputeShader].m_NumViews,
				pSRVs);
			break;
		}
		default:
			Assert(0);
			break;
	}
}


//--------------------------------------------------------------------------------------	
void CMaterialTextureOps::Release()
{
	SetState(CMaterialState::eUnloaded);
}

//--------------------------------------------------------------------------------------	
bool CMaterialTextureOps::ShadersLoaded() const
{
	return m_pComputeShader && m_pComputeShader->IsLoaded();
}