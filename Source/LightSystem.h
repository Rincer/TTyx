#ifndef _LIGHTSYSTEM_H_
#define _LIGHTSYSTEM_H_

#include "MaterialSystem.h"
#include "ConstantsSystem.h"
#include "Color.h"

class CConstantsSystem;

class CLight : public IAccessible
{
	public:
		CLight()
		{
		}
		
		CLight(CColor& Color, float Brightness, XMFLOAT3& Position, float Radius) :	m_Color(Color),
																					m_Brightness(Brightness),
																					m_Position(Position),
																					m_Radius(Radius)
		{
		}
				
		virtual bool IsLoaded() const
		{
			return true;
		}
		
		CColor					m_Color;
		float					m_Brightness;		
		XMFLOAT3				m_Position;
		float					m_Radius;	
};


class CLightSystem
{
	public:
		CLightSystem();
		~CLightSystem();
				
		void CalculateDrawnLights();	
		CLight& CreateLight(CColor& Color, float Brightness, XMFLOAT3& Position, float Radius);
		void SetDrawnLights(CConstantsSystem* pConstantsSystem);
		
	private:
		static const unsigned int scMaxDrawnLights = 1; 
		
		CElementArrayPool<CLight>*	m_pLightsElementArrayPool;
		CElementArray<CLight>*		m_pLights;		
		
		CLight*			m_pDrawnLights[scMaxDrawnLights];
		unsigned int	m_NumDrawnLights;
		unsigned int	m_TotalLights;
		
		CStackAllocator* m_pStackAllocator; // Allocator used by the Light System
};

#endif