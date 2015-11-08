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
	m_pPointLightsElementArrayPool = new(m_pStackAllocator->Alloc(sizeof(CElementArrayPool<CPointLight>))) CElementArrayPool<CPointLight>(m_pStackAllocator, 1, scMaxLights);	
	m_pPointLights = new(m_pStackAllocator->Alloc(sizeof(CElementArray<CPointLight>))) CElementArray<CPointLight>(m_pPointLightsElementArrayPool);	
	m_pDirectionalLightsElementArrayPool = new(m_pStackAllocator->Alloc(sizeof(CElementArrayPool<CDirectionalLight>))) CElementArrayPool<CDirectionalLight>(m_pStackAllocator, 1, scMaxLights);	
	m_pDirectionalLights = new(m_pStackAllocator->Alloc(sizeof(CElementArray<CDirectionalLight>))) CElementArray<CDirectionalLight>(m_pDirectionalLightsElementArrayPool);	
	m_NumDrawnPointLights = 0;
	m_TotalLights = 0;
}

//----------------------------------------------------------------------------------
CLightSystem::~CLightSystem()
{
	delete m_pStackAllocator;
}

//----------------------------------------------------------------------------------
CPointLight& CLightSystem::CreateLight(CColor& Color, float Brightness, XMFLOAT3& Position, float Radius)
{
	CPointLight* pLight;	
	pLight = &(m_pPointLights->Add());
	pLight = new(pLight) CPointLight(Color, Brightness, Position, Radius);
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
CDirectionalLight& CLightSystem::CreateLight(CColor& Color, float Brightness, XMFLOAT3& Direction)
{
	CDirectionalLight* pLight;	
	pLight = &(m_pDirectionalLights->Add());
	pLight = new(pLight) CDirectionalLight(Color, Brightness, Direction);
	char LightName[256];
	sprintf_s(LightName, 255, "Graphics/Lights/Light_%d", m_TotalLights);					
	m_TotalLights++;		
	return *pLight;
}

//----------------------------------------------------------------------------------
void CLightSystem::CalculateDrawnLights()	
{
	m_NumDrawnPointLights = 0;
	for( CElementEntry* pElement = m_pPointLights->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
	{
		m_pDrawnPointLights[m_NumDrawnPointLights] = (CPointLight*)pElement->m_pData;
		m_NumDrawnPointLights++;
	}
	Assert(m_NumDrawnPointLights <= scMaxDrawnPointLights);
	for( CElementEntry* pElement = m_pDirectionalLights->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
	{
		m_pDirectionalLight = (CDirectionalLight*)pElement->m_pData;
		break; // only one for now
	}
}

//----------------------------------------------------------------------------------
void CLightSystem::SetDrawnLights(CConstantsSystem* pConstantsSystem)
{
	CConstantsSystem::cbLights* pcbLights;    
	pConstantsSystem->UpdateConstantBufferBegin(CConstantsSystem::eLights, (void**)&pcbLights, sizeof(CConstantsSystem::cbLights));
	Assert(m_NumDrawnPointLights == 1); // only 1 light for now!!!
	for(unsigned int LightIndex = 0; LightIndex < m_NumDrawnPointLights; LightIndex++)
	{
		CPointLight* pLight = m_pDrawnPointLights[LightIndex];
		float RadiusSq = pLight->m_Radius * pLight->m_Radius;														
		pcbLights->m_ColorSqR = XMVectorSet(pLight->m_Brightness * (float)pLight->m_Color.m_Color.m_Channels.m_R / 255.0f,
											pLight->m_Brightness * (float)pLight->m_Color.m_Color.m_Channels.m_G / 255.0f,
											pLight->m_Brightness * (float)pLight->m_Color.m_Color.m_Channels.m_B / 255.0f,												
											RadiusSq);
		pcbLights->m_PosInvSqR = XMVectorSet(pLight->m_Position.x, pLight->m_Position.y, pLight->m_Position.z, 1.0f / RadiusSq);

		//pcbLights++;
	}	
	// directional should really be separate, although wasting a whole constant buffer slot just for 1 light seems a waste
	pcbLights->m_DirectionalDir = XMVectorSet(m_pDirectionalLight->m_Direction.x, m_pDirectionalLight->m_Direction.y, m_pDirectionalLight->m_Direction.z, 0.0f);
	pcbLights->m_DirectionalCol = XMVectorSet(	m_pDirectionalLight->m_Brightness * (float)m_pDirectionalLight->m_Color.m_Color.m_Channels.m_R / 255.0f,
												m_pDirectionalLight->m_Brightness * (float)m_pDirectionalLight->m_Color.m_Color.m_Channels.m_G / 255.0f,
												m_pDirectionalLight->m_Brightness * (float)m_pDirectionalLight->m_Color.m_Color.m_Channels.m_B / 255.0f,												
												0.0f);
	pConstantsSystem->UpdateConstantBufferEnd();
	pConstantsSystem->SetConstantBuffer(CConstantsSystem::eLights);
}

