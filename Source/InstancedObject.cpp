#include "stdafx.h"
#include "RenderObject.h"
#include "Renderer.h"
#include "ConstantsSystem.h"

#include "InstancedObject.h"

//------------------------------------------------------------------------------------------------
CInstancedObject::CInstancedObject() : m_pRenderObject(NULL)
{
}

//------------------------------------------------------------------------------------------------
void CInstancedObject::Create(const char* pName, CRenderObjectSystem* pRenderObjectSystem)
{
	char Name[256];
	sprintf_s(Name, "%s_0.%s", pName, CInstancedRenderObject::GetTypeName());
	m_pRenderObject = &pRenderObjectSystem->GetInstancedRenderObjectRef(Name);
}

//------------------------------------------------------------------------------------------------
void CInstancedObject::Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{
	if (IsLoaded())
	{
		m_pRenderObject->Draw(pRenderer, pConstantsSystem);
	}
}

//------------------------------------------------------------------------------------------------
void CInstancedObject::SetScaleRotationPosition(float ScaleX, float ScaleY, float ScaleZ, float Roll, float Pitch, float Yaw, float OffsetX, float OffsetY, float OffsetZ)
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

//------------------------------------------------------------------------------------------------
bool CInstancedObject::IsLoaded()
{
	return m_pRenderObject ? m_pRenderObject->IsLoaded() : false;
}
