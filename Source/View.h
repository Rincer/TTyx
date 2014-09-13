#ifndef _VIEW_H_
#define _VIEW_H_

#include "MaterialSystem.h"
#include "ConstantsSystem.h"
#include "HIDInputInterface.h"

class CInput;
class CConstantsSystem;

class CView
{
	public:
		CView(CConstantsSystem* pConstantsSystem);
		void Initialize(float FOV, float AspectRatio, float NearPlane, float FarPlane);
		void Set();
		void SetPosition(float x, float y, float z);
		void Update(float Pitch, float Yaw, float Roll, float Forward, float Lateral);
		XMFLOAT3 GetPosition();
		
	private:
		
		XMMATRIX m_View;
		XMMATRIX m_InverseView;
		XMMATRIX m_Projection;		
		XMMATRIX m_InvViewProjection;	
				
		XMVECTORF32 m_Pos;
		CConstantsSystem* m_pConstantsSystem;

	
		friend class CViewController;
};


#endif