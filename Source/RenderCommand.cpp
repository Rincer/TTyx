#include "stdafx.h"
#include "Renderer.h"
#include "ShaderPipeline.h"
#include "TextureSystem.h"
#include "GeometrySystem.h"
#include "DrawPrimitive.h"
#include "ConstantsSystem.h"
#include "View.h"
#include "LightSystem.h"
#include "UtilityDraw.h"
#include "TimeLine.h"

#include "RenderCommand.h"

//--------------------------------------------------------------------------------
void CRenderCommandClear::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	pDeviceContext;
	m_pRenderer->Clear(pDeviceContext);
}

//--------------------------------------------------------------------------------
unsigned int CRenderCommandClear::Size()
{
	return sizeof(*this);
}

//--------------------------------------------------------------------------------
void CRenderCommandSkipBytes::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	pDeviceContext;
}

//--------------------------------------------------------------------------------
unsigned int CRenderCommandSkipBytes::Size()
{
	// this will tell the renderer to advance the command buffer by this number of bytes
	return m_BytesToSkip;
}

//--------------------------------------------------------------------------------
void CRenderCommandEndFrame::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	pDeviceContext; // C4100
	(*m_pFrameCount)++;
	EndFrame = true;
}

//--------------------------------------------------------------------------------
unsigned int CRenderCommandEndFrame::Size()
{
	return sizeof(*this);
}

//----------------------------------------------------------------------------------------------
void CRenderCommandMapBuffer::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	m_pBuffer->MapResourceData(pDeviceContext);
}

//--------------------------------------------------------------------------------
unsigned int CRenderCommandMapBuffer::Size()
{
	return sizeof(*this);
}

//----------------------------------------------------------------------------------------------
void CRenderCommandUnMapBuffer::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	m_pBuffer->UnMapResourceData(pDeviceContext);
}

//--------------------------------------------------------------------------------
unsigned int CRenderCommandUnMapBuffer::Size()
{
	return sizeof(*this);
}

//----------------------------------------------------------------------------------------------
void CRenderCommandDrawIndexedPrimitive::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	m_pDrawPrimitive->Draw(pDeviceContext);
}

//--------------------------------------------------------------------------------
unsigned int CRenderCommandDrawIndexedPrimitive::Size()
{
	return sizeof(*this);
}


CRenderCommandMakeCopyAndDrawPrimitive::CRenderCommandMakeCopyAndDrawPrimitive(CBasePrimitive*	pPrimitive, unsigned int Size) : m_Size(Size)

{
	unsigned char *pMem = (unsigned char*)this;
	pMem += sizeof(*this);
	pPrimitive->Clone(pMem);
	m_pPrimitive = (CBasePrimitive*)pMem;
}

//----------------------------------------------------------------------------------------------
void CRenderCommandMakeCopyAndDrawPrimitive::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	m_pPrimitive->Draw(pDeviceContext);
}

//--------------------------------------------------------------------------------
unsigned int CRenderCommandMakeCopyAndDrawPrimitive::Size()
{
	return m_Size;
}

//----------------------------------------------------------------------------------------------
void CRenderCommandSetConstantBuffer::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	m_pBuffer->Set(pDeviceContext, m_pResourceInfo);
}

//----------------------------------------------------------------------------------------------
unsigned int CRenderCommandSetConstantBuffer::Size()
{
	return sizeof(*this); 
}

//----------------------------------------------------------------------------------------------
void CRenderCommandUpdateConstantBuffer::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	m_pBuffer->Update(pDeviceContext, m_pData, m_Size);
}

//----------------------------------------------------------------------------------------------
unsigned int CRenderCommandUpdateConstantBuffer::Size()
{
	return sizeof(*this) + m_Size + 16; // This command reserves extra 16 bytes to align the start of the constant buffer data
}


//----------------------------------------------------------------------------------------------
CRenderCommandSetRenderTargets::CRenderCommandSetRenderTargets(ID3D11RenderTargetView** pRTs, unsigned int NumTargets, ID3D11DepthStencilView* pDepthTarget) : m_NumTargets(NumTargets)
{
	Assert(NumTargets <= CRenderTargets::sc_MaxMRTs);
	for (unsigned int RTIndex = 0; RTIndex < NumTargets; RTIndex++)
	{
		m_pRTs[RTIndex] = pRTs[RTIndex];
	}
	m_pDepth = pDepthTarget;
}

//----------------------------------------------------------------------------------------------	
void CRenderCommandSetRenderTargets::Execute(ID3D11DeviceContext* pDeviceContext, bool& /*EndFrame*/)
{
	pDeviceContext->OMSetRenderTargets(m_NumTargets, m_pRTs, m_pDepth);
}

//----------------------------------------------------------------------------------------------	
unsigned int CRenderCommandSetRenderTargets::Size()
{
	return sizeof(*this);
}

//----------------------------------------------------------------------------------------------
void CRenderCommandClearColor::Execute(ID3D11DeviceContext* pDeviceContext, bool& /*EndFrame*/)
{
	pDeviceContext->ClearRenderTargetView(m_pRT, m_Color);
}

//----------------------------------------------------------------------------------------------
unsigned int CRenderCommandClearColor::Size()
{
	return sizeof(*this);
}

//----------------------------------------------------------------------------------------------
void CRenderCommandClearDepth::Execute(ID3D11DeviceContext* pDeviceContext, bool& /*EndFrame*/)
{
	pDeviceContext->ClearDepthStencilView(m_pDepthTarget, m_Flags, m_Depth, m_Stencil);
}

//----------------------------------------------------------------------------------------------
unsigned int CRenderCommandClearDepth::Size()
{
	return sizeof(*this);
}

//----------------------------------------------------------------------------------------------
void CRenderCommandSetViewport::Execute(ID3D11DeviceContext* pDeviceContext, bool& /*EndFrame*/)
{
	D3D11_VIEWPORT vp;
	vp.TopLeftX = m_TopLeftX;
	vp.TopLeftY = m_TopLeftY;
	vp.Width = m_Width;
	vp.Height = m_Height;
	vp.MinDepth = m_MinDepth;
	vp.MaxDepth = m_MaxDepth;
	pDeviceContext->RSSetViewports(1, &vp);
}
	
//----------------------------------------------------------------------------------------------
unsigned int CRenderCommandSetViewport::Size()
{
	return sizeof(*this);
}

//----------------------------------------------------------------------------------------------		
void CUtilityDrawRenderCommand::Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame)
{
	EndFrame; // C4100
	pDeviceContext; // C4100
	m_pUtilityDraw->Render(NULL, NULL, m_VertexBuffer);
}

//----------------------------------------------------------------------------------------------
unsigned int CUtilityDrawRenderCommand::Size()
{
	return sizeof(*this);
}
