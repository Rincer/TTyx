#include "stdafx.h"
#include "Macros.h"
#include "ShaderPipeline.h"
#include "Renderer.h"
#include "Input.h"
#include "ConstantsSystem.h"
#include "Window.h"

#include "View.h"

//----------------------------------------------------------------------------------------------
CView::CView(CConstantsSystem* pConstantsSystem) : m_pConstantsSystem(pConstantsSystem)
{
	m_View = XMMatrixIdentity(); 
	m_InverseView = XMMatrixIdentity();
	m_Projection = XMMatrixIdentity();
	m_InvViewProjection = XMMatrixIdentity();
	m_Pos.v = XMVectorSet(0, 0, 0, 0);
}

//----------------------------------------------------------------------------------------------
void CView::Initialize(float FOV, float AspectRatio, float NearPlane, float FarPlane)
{
	m_View = XMMatrixIdentity();
	m_Pos.v = XMVectorSet(0, 0, 0, 0);
	m_InverseView = XMMatrixIdentity();
	m_Projection = XMMatrixPerspectiveFovLH(FOV, AspectRatio, NearPlane, FarPlane);	
}

//----------------------------------------------------------------------------------------------
void CView::Set()
{
	CConstantsSystem::cbView* pcbView;
	m_pConstantsSystem->UpdateConstantBufferBegin(CConstantsSystem::eView, (void**)&pcbView, sizeof(CConstantsSystem::cbView));
	pcbView->m_InvViewProjection = m_InvViewProjection;
	pcbView->m_CameraPos = m_Pos;
	m_pConstantsSystem->UpdateConstantBufferEnd();
	m_pConstantsSystem->SetConstantBuffer(CConstantsSystem::eView);
}

//----------------------------------------------------------------------------------------------
void CView::SetPosition(float x, float y, float z)
{
	m_Pos.v = XMVectorSet(x, y, z, 1.0f);
}

//----------------------------------------------------------------------------------------------
XMFLOAT3 CView::GetPosition()
{
	XMFLOAT3 Res;
	Res.x = m_Pos.f[0];
	Res.y = m_Pos.f[1];
	Res.z = m_Pos.f[2];		
	return Res;
}

//----------------------------------------------------------------------------------------------
void CView::Update(float Pitch, float Yaw, float Roll, float Forward, float Lateral)
{
	XMMATRIX RotPitch = XMMatrixRotationAxis(m_View.r[0], Pitch);
	XMMATRIX RotYaw = XMMatrixRotationAxis(m_View.r[1], Yaw);		
	XMMATRIX RotRoll = XMMatrixRotationAxis(m_View.r[2], Roll);			

	// add rotation and orthonormalize
	m_View.r[3] = XMVectorZero();
	m_View = m_View * RotPitch * RotYaw * RotRoll;
	m_View.r[0] = XMVector3Normalize(m_View.r[0]);
	m_View.r[1] = XMVector3Normalize(m_View.r[1]);	
	m_View.r[2] = XMVector3Cross(m_View.r[0], m_View.r[1]);
	m_View.r[2] = XMVector3Normalize(m_View.r[2]);		
		
	// add translation
	m_Pos.v += m_View.r[2] * Forward;
	m_Pos.v += m_View.r[0] * Lateral;
	m_View.r[3] = m_Pos;
	
	// calculate the inverse
	XMVECTORF32 Det;
	Det.v = XMMatrixDeterminant(m_View);
	m_InverseView = XMMatrixInverse(&Det.v, m_View);
	m_InvViewProjection = m_InverseView * m_Projection;
}

	
	
