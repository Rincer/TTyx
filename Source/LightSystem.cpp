#include "stdafx.h"
#include "StackAllocator.h"
#include "Macros.h"
#include "ShaderPipeline.h"
#include "Renderer.h"
#include "LiveEditTree.h"

#include "LightSystem.h"

static const unsigned int scLightSystemMemory = 128 * 1024;
static const unsigned int scMaxLights = 1024; 


//----------------------------------------------------------------------------------
CLightSystem::CLightSystem()
{
	m_pStackAllocator = new CStackAllocator(scLightSystemMemory);	
	m_pLightsElementArrayPool = new(m_pStackAllocator->Alloc(sizeof(CElementArrayPool<CLight>))) CElementArrayPool<CLight>(m_pStackAllocator, 1, scMaxLights);	
	m_pLights = new(m_pStackAllocator->Alloc(sizeof(CElementArray<CLight>))) CElementArray<CLight>(m_pLightsElementArrayPool);	
	m_NumDrawnLights = 0;
	m_TotalLights = 0;
}

//----------------------------------------------------------------------------------
CLightSystem::~CLightSystem()
{
	delete m_pStackAllocator;
}

//----------------------------------------------------------------------------------
CLight& CLightSystem::CreateLight(CColor& Color, float Brightness, XMFLOAT3& Position, float Radius)
{
	CLight* pLight;	
	pLight = &(m_pLights->Add());
	pLight = new(pLight) CLight(Color, Brightness, Position, Radius);
	char LightName[256];
	sprintf_s(LightName, 255, "Graphics/Lights/Light_%d", m_TotalLights);
	CLiveEditTree::Instance().AddColor(LightName, "Color", &pLight->m_Color, NULL);		
	CLiveEditTree::Instance().AddFloat(LightName, "Brightness", 1.0f, 100.0f, 0.1f, &pLight->m_Brightness, NULL);		
	CLiveEditTree::Instance().AddFloat(LightName, "X", -100000.0f, 100000.0f, 0.1f, &pLight->m_Position.x, NULL);			
	CLiveEditTree::Instance().AddFloat(LightName, "Y", -100000.0f, 100000.0f, 0.1f, &pLight->m_Position.y, NULL);				
	CLiveEditTree::Instance().AddFloat(LightName, "Z", -100000.0f, 100000.0f, 0.1f, &pLight->m_Position.z, NULL);					
	CLiveEditTree::Instance().AddFloat(LightName, "Radius", -100000.0f, 100000.0f, 0.1f, &pLight->m_Radius, NULL);						
	m_TotalLights++;		
	return *pLight;
}

//----------------------------------------------------------------------------------
void CLightSystem::CalculateDrawnLights()	
{
	m_NumDrawnLights = 0;
	for( CElementEntry* pElement = m_pLights->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
	{
		m_pDrawnLights[m_NumDrawnLights] = (CLight*)pElement->m_pData;
		m_NumDrawnLights++;
	}
	Assert(m_NumDrawnLights <= scMaxDrawnLights);
}

//----------------------------------------------------------------------------------
void CLightSystem::SetDrawnLights(CConstantsSystem* pConstantsSystem)
{
	CConstantsSystem::cbLights* pcbLights;    
	pConstantsSystem->UpdateConstantBufferBegin(CConstantsSystem::eLights, (void**)&pcbLights, sizeof(CConstantsSystem::cbLights));
	for(unsigned int LightIndex = 0; LightIndex < m_NumDrawnLights; LightIndex++)
	{
		CLight* pLight = m_pDrawnLights[LightIndex];
		float RadiusSq = pLight->m_Radius * pLight->m_Radius;														
		pcbLights->m_ColorSqR = XMVectorSet(pLight->m_Brightness * (float)pLight->m_Color.m_Color.m_Channels.m_R / 255.0f,
											pLight->m_Brightness * (float)pLight->m_Color.m_Color.m_Channels.m_G / 255.0f,
											pLight->m_Brightness * (float)pLight->m_Color.m_Color.m_Channels.m_B / 255.0f,												
											RadiusSq);
		pcbLights->m_PosInvSqR = XMVectorSet(pLight->m_Position.x, pLight->m_Position.y, pLight->m_Position.z, 1.0f / RadiusSq);

		pcbLights++;
	}	
	pConstantsSystem->UpdateConstantBufferEnd();
	pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLights);
}

