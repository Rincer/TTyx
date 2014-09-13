#include "stdafx.h"
#include "MaterialSystem.h"
#include "DrawPrimitive.h"
#include "MaterialPostprocess.h"
#include "TextureSystem.h"
#include "RenderTargets.h"
#include "VertexFormats.h"
#include "GeometrySystem.h"
#include "Renderer.h"

#include "Postprocessor.h"

// Create vertex buffer
static SPosTex sFSQuadVerts[] =
{
	{ XMFLOAT3( -1.0f, 1.0f, 0.5f ), XMFLOAT2(0.0f, 0.0f )},
    { XMFLOAT3( -1.0f,-1.0f, 0.5f ), XMFLOAT2(0.0f, 1.0f )},
    { XMFLOAT3(  1.0f,-1.0f, 0.5f ), XMFLOAT2(1.0f, 1.0f )},
    { XMFLOAT3(  1.0f, 1.0f, 0.5f ), XMFLOAT2(1.0f, 0.0f )},
};

static unsigned short sFSQuadIndices[] =
{
	0, 2, 1,
	0, 3, 2
};

//---------------------------------------------------------------------------------------------
CPostprocessor::CPostprocessor() : m_pPostprocessPrimitive(NULL),
m_pPostprocessDMaterial(NULL)
{
}


//---------------------------------------------------------------------------------------------
CPostprocessor::~CPostprocessor()
{
}

//---------------------------------------------------------------------------------------------
void CPostprocessor::Startup(CTextureSystem* pTextureSystem,
								CRenderTargets* pRenderTargets,
								CDrawPrimitiveSystem* pDrawPrimitiveSystem,
								CMaterialSystem* pMaterialSystem,
								CGeometrySystem* pGeometrySystem,
								CRenderer* pRenderer)
{
	CMaterialPostprocess::CParameters ParamsPostprocess("Postprocess", 
		pRenderTargets->GetTexture(CRenderTargets::eFrameBuffer), 
		&pTextureSystem->GetSampler(CTextureSystem::eSamplerPoint),
		pRenderer->GetDepthStencilState(CRenderer::eDepthNone),
		pRenderer->GetDepthStencilState(CRenderer::eDepthLE));
	m_pPostprocessDMaterial = &pMaterialSystem->GetMaterial<CMaterialPostprocess>(&ParamsPostprocess);	
	CVBuffer& VBuffer = pGeometrySystem->CreateVBuffer("Postprocess.VBuffer", sizeof(SPosTex), sFSQuadVerts, sizeof(sFSQuadVerts));
	CIBuffer& IBuffer = pGeometrySystem->CreateIBuffer("Postprocess.IBuffer", sFSQuadIndices, sizeof(sFSQuadIndices));
	CVBuffer* pVBuffer[CDrawPrimitive::scMaxStreams] = { &VBuffer, NULL };
	m_pPostprocessPrimitive = &pDrawPrimitiveSystem->Add("Postprocess", pVBuffer, 1, &IBuffer, sizeof(sFSQuadIndices) / sizeof(unsigned short), m_pPostprocessDMaterial);
}

//---------------------------------------------------------------------------------------------
CDrawPrimitive*	CPostprocessor::GetPostprocessPrimitive()
{
	return m_pPostprocessPrimitive;
}