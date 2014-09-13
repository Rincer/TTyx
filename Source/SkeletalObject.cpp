#include "stdafx.h"
#include "RenderObject.h"
#include "Renderer.h"
#include "ConstantsSystem.h"

#include "SkeletalObject.h"

//----------------------------------------------------------------------------------------
CSkeletalObject::CSkeletalObject() : m_pRenderObject(NULL)
{
}

//----------------------------------------------------------------------------------------
void CSkeletalObject::Create(const char* pName, CRenderObjectSystem* pRenderObjectSystem, CAnimationSystem* pAnimationSystem)
{
	char Name[256];
	sprintf_s(Name, "%s.%s.MatParams_1", pName, CSkeletalRenderObject::GetTypeName());
	m_pRenderObject = &pRenderObjectSystem->GetSkeletalRenderObjectRef(Name);
	sprintf_s(Name, "%s.%s", pName, CAnimationSetTable::GetTypeName());
	m_AnimationController.Create(Name, pAnimationSystem);
}

//----------------------------------------------------------------------------------------
void CSkeletalObject::TickAnimation(float DeltaSec)
{
	if (IsLoaded())
	{
		m_AnimationController.m_CurrentSet = 2;
		m_AnimationController.Tick(DeltaSec);
		m_AnimationController.Apply(m_pRenderObject->GetMatrixPtr());
	}
}

//----------------------------------------------------------------------------------------
void CSkeletalObject::Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{
	if (IsLoaded())
	{
		m_pRenderObject->Draw(pRenderer, pConstantsSystem);
	}
}

//----------------------------------------------------------------------------------------
void CSkeletalObject::SetScaleRotationPosition(float ScaleX, float ScaleY, float ScaleZ, float Roll, float Pitch, float Yaw, float OffsetX, float OffsetY, float OffsetZ)
{
	if (IsLoaded())
	{
		XMMATRIX LocalToWorldNormals = XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);
		XMMATRIX LocalToWorld = XMMatrixScaling(ScaleX, ScaleY, ScaleZ) *
			LocalToWorldNormals *
			XMMatrixTranslation(OffsetX, OffsetY, OffsetZ);
		m_pRenderObject->SetLocalToWorld(LocalToWorld, LocalToWorldNormals);
	}
}

//----------------------------------------------------------------------------------------
bool CSkeletalObject::IsLoaded()
{
	return (m_pRenderObject != NULL) && m_pRenderObject->IsLoaded() && m_AnimationController.IsLoaded();
}

