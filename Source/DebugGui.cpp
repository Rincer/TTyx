#include "stdafx.h"
#include "TextureSystem.h"
#include "MaterialSystem.h"
#include "UtilityDraw.h"
#include "Renderer.h"
#include "MaterialTextured2D.h"

#include "DebugGui.h"

//------------------------------------------------------------------------------------
CDebugGUI::CDebugGUI(CTextureSystem** ppTextureSystem, CUtilityDraw** ppUtilityDraw, CMaterialSystem**ppMaterialSystem, CRenderer** ppRenderer) : m_rTextureSystem(ppTextureSystem),
																																				  m_rUtilityDraw(ppUtilityDraw),
																																				  m_rMaterialSystem(ppMaterialSystem),
																																				  m_rRenderer(ppRenderer)
{
	CTexture* pTexture = &m_rTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8_TYPELESS, 1, "Font.tga");
	CMaterialTextured2D::CParameters Params("FontMaterial", pTexture, &m_rTextureSystem->GetSampler(CTextureSystem::eSamplerLinear), CMaterialTextured2D::eA);
	m_pFontMaterial = &m_rMaterialSystem->GetMaterial<CMaterialTextured2D>(&Params);
	unsigned int Width, Height;
	m_rRenderer->GetViewportSize(Width, Height);
	m_ScreenPixelWidth = 2.0f / (float)Width;
	m_ScreenPixelHeight = 2.0f / (float)Height;	
	m_FontPixelWidth = 1.0f / 768.0f; // Hardcoded from font.tga
	m_FontPixelHeight = 1.0f / 16.0f;			
}

//-------------------------------------------------------------------------------------
CDebugGUI::~CDebugGUI()
{
}

//-------------------------------------------------------------------------------------
void CDebugGUI::DrawString(float x, float y, const char* pString, const CColor& Color)
{
	XMMATRIX LocalToWorld = XMMatrixIdentity();
	m_rUtilityDraw->BeginTriangleList();
	float X0 = -1.0f + x * m_ScreenPixelWidth;
	float X1 = X0 + 7.0f * m_ScreenPixelWidth;	
	float Y0 = 1.0f - y * m_ScreenPixelHeight;
	float Y1 = Y0 - 16.0f * m_ScreenPixelHeight;
	for(unsigned int CharIndex = 0; CharIndex < strlen(pString); CharIndex++)
	{
		float U0 = (float)(pString[CharIndex] - ' ') * 8.0f * m_FontPixelWidth;
		float U1 = U0 + 7.0f * m_FontPixelWidth;		
		XMFLOAT3 Pt0(X0, Y0, 0.999f);
		XMFLOAT3 Pt1(X0, Y1, 0.999f);		
		XMFLOAT3 Pt2(X1, Y0, 0.999f);				
		XMFLOAT3 Pt3(X1, Y1, 0.999f);						
		
		XMFLOAT2 Uv0(U0, 0.0f);
		XMFLOAT2 Uv1(U0, 1.0f);		
		XMFLOAT2 Uv2(U1, 0.0f);				
		XMFLOAT2 Uv3(U1, 1.0f);		
		
		m_rUtilityDraw->AddTriangle(Pt0, Pt2, Pt1, Uv0, Uv2, Uv1, Color, Color, Color);
		m_rUtilityDraw->AddTriangle(Pt1, Pt2, Pt3, Uv1, Uv2, Uv3, Color, Color, Color);
		X1 += (1.0f * m_ScreenPixelWidth);
		X0 = X1;
		X1 += (7.0f * m_ScreenPixelWidth);
	}
	m_rUtilityDraw->EndTriangleList(LocalToWorld, m_pFontMaterial, CRenderer::eDepthNone, CRenderer::eBlendModulate);
}