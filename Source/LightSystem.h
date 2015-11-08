#ifndef _LIGHTSYSTEM_H_
#define _LIGHTSYSTEM_H_

#include "MaterialSystem.h"
#include "ConstantsSystem.h"
#include "Color.h"

class CConstantsSystem;



class CPointLight : public IAccessible
{
	public:
		CPointLight()
		{
		}
		
		CPointLight(CColor& Color, float Brightness, XMFLOAT3& Position, float Radius) :	m_Color(Color),
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

class CDirectionalLight : public IAccessible
{
	public:
		CDirectionalLight()
		{
		}
		
		CDirectionalLight(CColor& Color, float Brightness, XMFLOAT3& Direction) :	m_Color(Color),
																					m_Brightness(Brightness),
																					m_Direction(Direction)
																					
		{
			XMVECTORF32 normalDirection;
			normalDirection.v = XMVector3Normalize(XMLoadFloat3(&m_Direction));
			m_Direction = XMFLOAT3(normalDirection.f[0], normalDirection.f[1], normalDirection.f[2]);
		}
				
		virtual bool IsLoaded() const
		{
			return true;
		}
		
		CColor					m_Color;
		float					m_Brightness;		
		XMFLOAT3				m_Direction;
};

class CLightSystem
{
	public:
		CLightSystem();
		~CLightSystem();
				
		void CalculateDrawnLights();	
		CPointLight& CreateLight(CColor& Color, float Brightness, XMFLOAT3& Position, float Radius);
		CDirectionalLight& CreateLight(CColor& Color, float Brightness, XMFLOAT3& Direction);
		void SetDrawnLights(CConstantsSystem* pConstantsSystem);
		
	private:
		static const unsigned int scMaxDrawnPointLights = 1; 
		enum eLightType
		{
			ePoint,
			eDirectional,
			eMaxTypes
		};
		
		CElementArrayPool<CPointLight>*			m_pPointLightsElementArrayPool;
		CElementArray<CPointLight>*				m_pPointLights;		
		CElementArrayPool<CDirectionalLight>*	m_pDirectionalLightsElementArrayPool;
		CElementArray<CDirectionalLight>*		m_pDirectionalLights;		
		
		CPointLight*		m_pDrawnPointLights[scMaxDrawnPointLights];
		unsigned int		m_NumDrawnPointLights;
		CDirectionalLight*	m_pDirectionalLight; // should only ever be one

		unsigned int	m_TotalLights;
		
		CStackAllocator* m_pStackAllocator; // Allocator used by the Light System
};

#endif