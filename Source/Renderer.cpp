#include "stdafx.h"
#include "RingBuffer.h"
#include "RenderCommand.h"
#include "StackAllocator.h"
#include "MemoryManager.h"
#include "Macros.h"
#include "MaterialSystem.h"
#include "SegmentedBuffer.h"
#include "GeometrySystem.h"
#include "TextureSystem.h"
#include "Event.h"
#include "JobProcessRenderCommands.h"
#include "JobUtilityDraw.h"
#include "DrawPrimitive.h"
#include "ConstantsSystem.h"
#include "UtilityDraw.h"

#include "Renderer.h"

static const unsigned int scConstantDataSize = 16 * 1024; // 16K
static const unsigned int scCommandBufferSize = 256 * 1024; // 256K command buffer for render thread
static const unsigned int scRendererStackSize = 1 * 1024 * 1024; // 1MB



//--------------------------------------------------------------------------------------
CTimeLine::CEventDesc CRenderer::m_ProfilingEvents[RenderThreadEvents::eTotalEvents + 1] = 
{
	{
		RenderThreadEvents::eThreadLoop,
		CColor(128, 0, 0, 255),
		"ThreadLoop",
	},
	{
		RenderThreadEvents::eExecuteCommand,
		CColor(128, 0, 255, 0),
		"Exec Cmd",
	},
	{
		RenderThreadEvents::eCommandDrawPrim,
		CColor(128, 255, 0, 0),
		"DrawPrim",
	},	
	{
		0,
		CColor(0, 0, 0, 0),
		NULL,
	} // Null terminated	
};


//----------------------------------------------------------------------------------------------		
class CRendererRingBufferCallback : public CRingBuffer::CRingBufferCallback
{
	virtual void Execute(CRingBuffer* pRingBuffer, unsigned int BytesBeforeWrap)
	{
		// reserve all free space to the end of the buffer, when CRenderCommandSkipBytes gets executed
		// it will do nothing and the write ptr will wrap around to the beginning of the buffer.
		unsigned char* pData;
		pRingBuffer->ReserveUnsafe(&pData, BytesBeforeWrap);
		CRenderCommandSkipBytes* pCommand = new(pData) CRenderCommandSkipBytes(BytesBeforeWrap);
		// C4100
		pCommand;
	}
};

//----------------------------------------------------------------------------------------------		
CRenderer::CRenderer(HWND Wnd,	CTimeLine**  ppTimeLine,
								CTextureSystem** ppTextureSystem,
								CJobSystem** ppJobSystem,
								CMaterialSystem** ppMaterialSystem,
								CConstantsSystem** ppConstantsSystem,
								CDrawPrimitiveSystem** ppDrawPrimitiveSystem,
								CGeometrySystem** ppGeometrySystem,
								CPooledAllocator* pPooledAllocator) : m_rTimeLine(ppTimeLine),
																		m_rTextureSystem(ppTextureSystem),
																		m_rJobSystem(ppJobSystem),
																		m_rMaterialSystem(ppMaterialSystem),
																		m_rConstantsSystem(ppConstantsSystem),
																		m_rDrawPrimitiveSystem(ppDrawPrimitiveSystem),
																		m_rGeometrySystem(ppGeometrySystem),
																		m_CommandBuffer(pPooledAllocator)
{
	m_pd3dDevice = NULL;
	m_pImmediateContext = NULL;
	m_pSwapChain = NULL;
	m_pRasterizerState = NULL;
	
	m_pStackAllocator = new CStackAllocator(scRendererStackSize);
// Multithreaded deferred
	m_CommandBuffer.Initialize(sc_CommandBufferSegments, sc_CommandBufferSegmentSize, m_pStackAllocator);
	m_pCurrentRWBuffer = m_CommandBuffer.GetWrite();
	m_NumCommandLists[0] = 0;
	m_NumCommandLists[1] = 0;
	m_NumCommands = 0;
	m_CurrList = 0;
	m_pProcessRenderCommandsSync = new(m_pStackAllocator->Alloc(sizeof(CEventArray))) CEventArray(true, sc_MaxCommandLists, m_EventHandles);

// Multithreaded immediate
	unsigned char* pCommandBufferData = (unsigned char*)m_pStackAllocator->Alloc(scCommandBufferSize + sizeof(CRenderCommandSkipBytes)); // reserve some extra space for when we need a wrap command at the end of the buffer
	m_pRingBufferCallback = new(m_pStackAllocator->Alloc(sizeof(CRendererRingBufferCallback))) CRendererRingBufferCallback;
	m_pCommandBuffer = new(m_pStackAllocator->Alloc(sizeof(CRingBuffer))) CRingBuffer(pCommandBufferData, scCommandBufferSize, m_pRingBufferCallback);

	m_RenderThreadFrameCounter = 0;
	RECT rc;
	GetClientRect(Wnd, &rc);
	m_Width = rc.right - rc.left;
	m_Height = rc.bottom - rc.top;

	unsigned int CreateDeviceFlags = 0;
#ifdef _DEBUG 
	CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		//D3D_DRIVER_TYPE_UNKNOWN
		D3D_DRIVER_TYPE_HARDWARE
	};

	static const unsigned int scNumFeatureLevels = 1;
	D3D_FEATURE_LEVEL FeatureLevels[scNumFeatureLevels] =
	{
		D3D_FEATURE_LEVEL_11_0
	};

	HRESULT hr = S_OK;
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = m_Width;
	sd.BufferDesc.Height = m_Height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = Wnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	hr = D3D11CreateDeviceAndSwapChain(	NULL, DriverTypes[0], NULL, CreateDeviceFlags, FeatureLevels, scNumFeatureLevels, D3D11_SDK_VERSION, 
										&sd, &m_pSwapChain, &m_pd3dDevice, &m_FeatureLevel, &m_pImmediateContext);
	Assert(hr == S_OK);


	for (unsigned int ContextIndex = 0; ContextIndex < sc_MaxProcessAffinity; ContextIndex++)
	{
		hr = m_pd3dDevice->CreateDeferredContext(0, &m_pDeferredContexts[ContextIndex]);
		Assert(hr == S_OK);
	}
	
	D3D11_RASTERIZER_DESC RSDesc;
	RSDesc.AntialiasedLineEnable = FALSE;
	RSDesc.CullMode = D3D11_CULL_BACK;
	RSDesc.DepthBias = 0;
	RSDesc.DepthBiasClamp = 0.0f;
	RSDesc.DepthClipEnable = TRUE;
	RSDesc.FillMode = D3D11_FILL_SOLID;
	RSDesc.FrontCounterClockwise = FALSE;
	RSDesc.MultisampleEnable = TRUE;
	RSDesc.ScissorEnable = FALSE;
	RSDesc.SlopeScaledDepthBias = 0.0f;
	m_pd3dDevice->CreateRasterizerState(&RSDesc, &m_pRasterizerState);

	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
	ID3D11DepthStencilState* pDepthStencilState;
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DepthStencilDesc.StencilEnable = FALSE;
	DepthStencilDesc.StencilReadMask = 0;
	DepthStencilDesc.StencilWriteMask = 0;
	DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	m_pd3dDevice->CreateDepthStencilState(&DepthStencilDesc, &pDepthStencilState);
	m_pDepthStates[eDepthLE].Initialize(pDepthStencilState);

	DepthStencilDesc.DepthEnable = FALSE;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_NEVER;
	DepthStencilDesc.StencilEnable = FALSE;
	DepthStencilDesc.StencilReadMask = 0;
	DepthStencilDesc.StencilWriteMask = 0;
	DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	m_pd3dDevice->CreateDepthStencilState(&DepthStencilDesc, &pDepthStencilState);
	m_pDepthStates[eDepthNone].Initialize(pDepthStencilState);
	

	D3D11_BLEND_DESC BlendDesc;
	BlendDesc.AlphaToCoverageEnable = FALSE;
	BlendDesc.IndependentBlendEnable = FALSE;
	BlendDesc.RenderTarget[0].BlendEnable = FALSE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	m_pd3dDevice->CreateBlendState(&BlendDesc, &m_pBlendStates[eBlendNone]);

	BlendDesc.AlphaToCoverageEnable = FALSE;
	BlendDesc.IndependentBlendEnable = FALSE;
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	m_pd3dDevice->CreateBlendState(&BlendDesc, &m_pBlendStates[eBlendModulate]);
	m_RenderThread.Startup(RenderLoop, this, "Render", m_ProfilingEvents, *m_rTimeLine);

	m_CurrentCommandSize = 0;
	m_ExecutionModel = eMultithreadedImmediate;
	m_pConstantsData = m_pStackAllocator->AlignedAlloc(scConstantDataSize, 16);
}

//----------------------------------------------------------------------------------------------		
CRenderer::~CRenderer()
{
	m_RenderThread.Shutdown();
	delete m_pStackAllocator;
}

//----------------------------------------------------------------------------------------------		
void* CRenderer::Allocate(unsigned int Size)
{
	return m_pStackAllocator->Alloc(Size);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::Startup()
{
//	unsigned int MaxWidth;
//	unsigned int MaxHeight;
//	FindHighestDisplayResolution(MaxWidth, MaxHeight);
	m_RenderTargets.Startup(m_Width, m_Height, *m_rTextureSystem, m_pSwapChain);
	m_Postprocessor.Startup(*m_rTextureSystem, &m_RenderTargets, *m_rDrawPrimitiveSystem, *m_rMaterialSystem, *m_rGeometrySystem, this);
	InitDevice(m_pImmediateContext);
	for (unsigned int ContextIndex = 0; ContextIndex < sc_MaxProcessAffinity; ContextIndex++)
	{
		InitDevice(m_pDeferredContexts[ContextIndex]);
	}
	m_rTextureSystem->CreateSamplers(m_pd3dDevice);
	NextFrame(); // Clears the screen 
}

//----------------------------------------------------------------------------------------------		
void CRenderer::Shutdown()
{
	EndFrame();
	m_RenderThread.WaitForWorkComplete();
	m_rTextureSystem->ReleaseSamplers();
	m_RenderTargets.Shutdown();
	CleanupDevice();
}

//----------------------------------------------------------------------------------------------		
void CRenderer::ExecuteCommand(IRenderCommand* pRenderCommand, bool& EndFrame)
{
	//	CTimeLine::CScopedEvent Event(RenderThreadEvents::eExecuteCommand);
	pRenderCommand->Execute(m_pImmediateContext, EndFrame);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::RenderLoop(void* pContext)
{
	CRenderer* pRenderer = (CRenderer*)pContext;
	CTimeLine::CScopedEvent Event(RenderThreadEvents::eThreadLoop, *pRenderer->m_rTimeLine);
	pRenderer->InitDevice(pRenderer->GetDeviceContext()); // Restore device context at start of render loop
	pRenderer->Present();
	switch(pRenderer->m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			break;
		}
		case eMultithreadedImmediate:
		{
			bool EndFrame = false;
			while(!EndFrame)
			{
				IRenderCommand* pRenderCommand;
				pRenderer->m_pCommandBuffer->BeginRead((unsigned char**)&pRenderCommand, sizeof(IRenderCommand));	// this makes an assumption that the compiler stores the 
																													// virtual function table pointer as the 1st piece of data in a class
				pRenderer->ExecuteCommand(pRenderCommand, EndFrame);
				pRenderer->m_pCommandBuffer->EndRead(pRenderCommand->Size());		
			}	
			break;
		}

		case eMultithreadedDeferred:
		{
			pRenderer->ExecuteCommandLists();
			break;
		}
		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------				
void CRenderer::Tick(unsigned int CurrentFrame, float DeltaTime)
{
	// C4100
	DeltaTime;
	CurrentFrame;
	CTimeLine::CScopedEvent ScopedEvent((unsigned int)MainThreadEvents::eRenderSync, *m_rTimeLine);
	switch(m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			break;
		}
		case eMultithreadedImmediate:
		{
			EndFrame();
			m_RenderThread.WaitForWorkComplete();
			break;
		}

		case eMultithreadedDeferred:
		{
			if (m_NumCommands > 0)
			{
				FlushCommands();
				m_NumCommands = 0;
			}
			m_RenderThread.WaitForWorkComplete();
			if (m_NumCommandLists[m_CurrList] > 0)	// If command list jobs were spawned, wait for them all to finish
			{
				m_pProcessRenderCommandsSync->WaitMultiple(m_NumCommandLists[m_CurrList], INFINITE);
			}
			m_CurrList = 1 - m_CurrList;
			m_NumCommandLists[m_CurrList] = 0;
			break;
		}
		default:
			Assert(0);
			break;
	}
	NextFrame(); // Start building of the next frame
}

//----------------------------------------------------------------------------------------------				
void CRenderer::NextFrame()
{
	// Render thread will go and render the current frame
	if(m_ExecutionModel != eSingleThreaded)
	{
		m_RenderThread.SetWorkReady();
	}
	else
	{
		InitDevice(m_pImmediateContext); // Restore device context at start of render loop
		Present();
	}
	SetRenderTargets(CRenderTargets::eFrameBuffer, CRenderTargets::eDepthStencil);
	Clear(); // Start next frame with a clear
}

//----------------------------------------------------------------------------------------------		
void CRenderer::GenerateDynamicEnvMipMaps()
{
	m_rTextureSystem->GenerateDynEnvMapMipmaps(m_RenderTargets.GetTexture(CRenderTargets::eDynamicEnvironmentMap));
}

//----------------------------------------------------------------------------------------------		
void CRenderer::Postprocess()
{
	CDrawPrimitive* pDrawPrimitive = m_Postprocessor.GetPostprocessPrimitive();
	if(pDrawPrimitive->IsLoaded())
	{
		SetRenderTargets(CRenderTargets::eBackBuffer, CRenderTargets::eNull);
		DrawIndexedPrimitive(pDrawPrimitive);
	}
}

//----------------------------------------------------------------------------------------------		
void CRenderer::FindHighestDisplayResolution(unsigned int& Width, unsigned int& Height)
{
	DEVMODE DisplayMode;
	unsigned int SettingIndex = 0;
	Width = 0;
	Height = 0;
	while (EnumDisplaySettings(NULL, SettingIndex, &DisplayMode))
	{
		if (DisplayMode.dmPelsWidth >= Width && DisplayMode.dmPelsHeight >= Height)
		{
			Width = DisplayMode.dmPelsWidth;
			Height = DisplayMode.dmPelsHeight;
		}
		SettingIndex++;
	}
}

//----------------------------------------------------------------------------------------------		
void CRenderer::InitDevice(ID3D11DeviceContext* pDeviceContext)
{
	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)m_Width;
	vp.Height = (FLOAT)m_Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	
	ID3D11RenderTargetView* pRTs[1];
	pRTs[0] = (ID3D11RenderTargetView*)m_RenderTargets.GetRTV(CRenderTargets::eFrameBuffer);
	pDeviceContext->OMSetRenderTargets(1, pRTs, (ID3D11DepthStencilView*)m_RenderTargets.GetRTV(CRenderTargets::eDepthStencil));
	pDeviceContext->RSSetViewports(1, &vp);
	pDeviceContext->RSSetState(m_pRasterizerState);
	m_pDepthStates[eDepthLE].Set(pDeviceContext);
	pDeviceContext->OMSetBlendState(m_pBlendStates[eBlendNone], 0, 0xFFFFFFFF);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::CleanupDevice()
{
    if(m_pImmediateContext) 
    {
		m_pImmediateContext->ClearState();
	}
        
    if(m_pSwapChain)
    {
        m_pSwapChain->Release();
        m_pSwapChain = NULL;
    }

    if(m_pImmediateContext) 
    {
		m_pImmediateContext->Release();
		m_pImmediateContext = NULL;
	}

	for (unsigned int ContextIndex = 0; ContextIndex < sc_MaxProcessAffinity; ContextIndex++)
	{
		if (m_pDeferredContexts[ContextIndex])
		{
			m_pDeferredContexts[ContextIndex]->Release();
			m_pDeferredContexts[ContextIndex] = NULL;
		}
	}

    if(m_pRasterizerState)
    {
		m_pRasterizerState->Release();
		m_pRasterizerState = NULL;
    }
	
	for(unsigned int i = eDepthNone; i < eMaxDepthStates; i++)
	{
		m_pDepthStates[i].Release();	
	}	

	for(unsigned int i = eBlendNone; i < eMaxBlendStates; i++)
	{
		if(m_pBlendStates[i])
		{
			m_pBlendStates[i]->Release();
			m_pBlendStates[i] = NULL;
		}		
	}	
		
#if 0 
	ID3D11Debug* pDebugDevice;
	HRESULT hr = m_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pDebugDevice));
	pDebugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	pDebugDevice->Release();
	Assert(hr == S_OK);
#endif

    if( m_pd3dDevice )
    {
		m_pd3dDevice->Release();
		m_pd3dDevice = NULL;
    }        
}

//----------------------------------------------------------------------------------------------		
void CRenderer::SetRenderTargets(CRenderTargets::eRenderTargetTypes ColorTarget, CRenderTargets::eRenderTargetTypes DepthTarget)
{
	ID3D11RenderTargetView* pRTs[1];
	ID3D11DepthStencilView* pDepthTarget;
	pRTs[0] = ColorTarget == CRenderTargets::eNull ? NULL : (ID3D11RenderTargetView*)m_RenderTargets.GetRTV(ColorTarget);
	pDepthTarget = DepthTarget == CRenderTargets::eNull ? NULL : (ID3D11DepthStencilView*)m_RenderTargets.GetRTV(DepthTarget);
	SetRenderTargets(pRTs, 1, pDepthTarget);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::ClearColor(CRenderTargets::eRenderTargetTypes ColorTarget, float Color[4])
{
	ID3D11RenderTargetView* pRT = (ID3D11RenderTargetView*)m_RenderTargets.GetRTV(ColorTarget);
	ClearColor(pRT, Color);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::ClearDepthStencil(CRenderTargets::eRenderTargetTypes DepthTarget, unsigned int Flags, float Depth, unsigned char Stencil)
{
	ID3D11DepthStencilView* pDepthTarget = (ID3D11DepthStencilView*)m_RenderTargets.GetRTV(DepthTarget);
	ClearDepthStencil(pDepthTarget, Flags, Depth, Stencil);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::Clear(ID3D11DeviceContext* pDeviceContext)
{
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; //red,green,blue,alpha
	pDeviceContext->ClearRenderTargetView((ID3D11RenderTargetView*)m_RenderTargets.GetRTV(CRenderTargets::eFrameBuffer), ClearColor);
	pDeviceContext->ClearDepthStencilView((ID3D11DepthStencilView*)m_RenderTargets.GetRTV(CRenderTargets::eDepthStencil), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::Present()
{
	ClearStates();
    m_pSwapChain->Present(0, 0);	
}


//----------------------------------------------------------------------------------------------		
// Sets a new depth state 
void CRenderer::SetDepthState(ID3D11DeviceContext* pDeviceContext, eDepthStates DepthState)
{
	m_pDepthStates[DepthState].Set(pDeviceContext);
}
		
//----------------------------------------------------------------------------------------------		
// Sets a new Blend state 
void CRenderer::SetBlendState(ID3D11DeviceContext* pDeviceContext, eBlendStates BlendState)
{
	pDeviceContext->OMSetBlendState(m_pBlendStates[BlendState], 0, 0xFFFFFFFF);
}
		
//----------------------------------------------------------------------------------------------		
void CRenderer::ClearStates()
{
	if(m_pImmediateContext)
	{
		m_pImmediateContext->IASetInputLayout( 0 );
		m_pImmediateContext->PSSetShader(0, 0, 0);
		m_pImmediateContext->VSSetShader(0, 0, 0);		
		for(unsigned int SamplerIndex = 0; SamplerIndex < 16; SamplerIndex++)
		{
			m_pImmediateContext->PSSetSamplers(SamplerIndex, 0, 0);
			m_pImmediateContext->PSSetShaderResources(0, 0, 0);	
		}
		m_pImmediateContext->IASetVertexBuffers( 0, 0, 0, 0, 0);
		m_pImmediateContext->IASetVertexBuffers( 1, 0, 0, 0, 0);
	}
}

//----------------------------------------------------------------------------------------------		
float CRenderer::GetAspectRatio()
{
	return (float)m_Width / m_Height;
}

//----------------------------------------------------------------------------------------------		
void CRenderer::GetViewportSize(unsigned int& Width, unsigned int& Height)
{
	Width = m_Width;
	Height = m_Height;
}

//----------------------------------------------------------------------------------------------		
ID3D11Device* CRenderer::GetDevice()
{
	return m_pd3dDevice;
}

//----------------------------------------------------------------------------------------------		
ID3D11DeviceContext* CRenderer::GetDeviceContext()
{
	return m_pImmediateContext;
}

//----------------------------------------------------------------------------------------------		
ID3D11DeviceContext* CRenderer::GetDeferredContext(unsigned int ContextIndex)
{
	return m_pDeferredContexts[ContextIndex];
}

// Multithreaded deferred
//----------------------------------------------------------------------------------------------		
void CRenderer::ReturnCommandBufferSegment(CRWBuffer* pRWBuffer)
{
	m_CommandBuffer.PutWrite(pRWBuffer);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::ExecuteCommandLists()
{
	CTimeLine::CScopedEvent Event(RenderThreadEvents::eExecuteCommand, *m_rTimeLine);
	// Executes the previous frames' command lists
	for (unsigned int CLIndex = 0; CLIndex < m_NumCommandLists[1 - m_CurrList]; CLIndex++)
	{
		m_pImmediateContext->ExecuteCommandList(m_pCommandLists[1 - m_CurrList][CLIndex], TRUE);
		m_pCommandLists[1 - m_CurrList][CLIndex]->Release();
		m_pCommandLists[1 - m_CurrList][CLIndex] = NULL;
	}
}

//----------------------------------------------------------------------------------------------		
void CRenderer::FlushCommands()
{
	m_pCurrentRWBuffer->SwapModes(); // Swap to read mode

	// spawn the job to process commands into command list
	CJobProcessRenderCommands* pJob = m_rJobSystem->AcquireJob<CJobProcessRenderCommands>(CJobSystem::ePriority1);
	unsigned int NumCommandLists = m_NumCommandLists[m_CurrList];
	m_pProcessRenderCommandsSync->Reset(NumCommandLists); // Reset the event
	new (pJob)CJobProcessRenderCommands(this, m_pCurrentRWBuffer, &m_pCommandLists[m_CurrList][NumCommandLists], m_NumCommands, m_pProcessRenderCommandsSync, NumCommandLists, *m_rConstantsSystem);
	m_rJobSystem->AddJob(pJob);

	// get the next buffer ready, the job will return the previous buffer to the segmented command buffer after completing
	m_NumCommandLists[m_CurrList]++;
	m_pCurrentRWBuffer = m_CommandBuffer.GetWrite();
	Assert(m_NumCommandLists[m_CurrList] < sc_MaxCommandLists);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::IncrementCommands()
{
	m_NumCommands++;
	if (m_NumCommands == sc_CommandsPerSegment)
	{
		FlushCommands();
		m_NumCommands = 0;
	}
}


//----------------------------------------------------------------------------------------------
// All the commands
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------		
void CRenderer::MapBufferImmediate(CBuffer* pBuffer) // don't schedule render command
{
	pBuffer->MapResourceData(m_pImmediateContext);
}

//----------------------------------------------------------------------------------------------		
void CRenderer::SetViewport(float TopLeftX, float TopLeftY, float Width, float Height, float MinDepth, float MaxDepth)
{
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			D3D11_VIEWPORT vp;
			vp.TopLeftX = TopLeftX;
			vp.TopLeftY = TopLeftY;
			vp.Width = Width;
			vp.Height = Height;
			vp.MinDepth = MinDepth;
			vp.MaxDepth = MaxDepth;
			m_pImmediateContext->RSSetViewports(1, &vp);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandSetViewport));
			new(pMem)CRenderCommandSetViewport(TopLeftX, TopLeftY, Width, Height, MinDepth, MaxDepth);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandSetViewport));
			break;
		}
		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandSetViewport));
			new(pMem)CRenderCommandSetViewport(TopLeftX, TopLeftY, Width, Height, MinDepth, MaxDepth);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandSetViewport));
			IncrementCommands();
			break;
		}
		default:
			Assert(0);
			break;
	}
}


//----------------------------------------------------------------------------------------------		
void CRenderer::Clear()
{
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			Clear(m_pImmediateContext);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandClear));
			new(pMem)CRenderCommandClear(this);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandClear));
			break;
		}
		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandClear));
			new(pMem)CRenderCommandClear(this);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandClear));
			IncrementCommands();
			break;
		}
		default:
			Assert(0);
			break;
	}
}
//----------------------------------------------------------------------------------------------		
void CRenderer::MapBuffer(CBuffer* pBuffer)
{
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			pBuffer->MapResourceData(m_pImmediateContext);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandMapBuffer));
			new(pMem)CRenderCommandMapBuffer(pBuffer);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandMapBuffer));
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandMapBuffer));
			new(pMem)CRenderCommandMapBuffer(pBuffer);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandMapBuffer));
			IncrementCommands();
			break;
		}
		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------			
void CRenderer::UnMapBuffer(CBuffer* pBuffer)
{
	IRenderCommand* pCommand;
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			pBuffer->UnMapResourceData(m_pImmediateContext);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandUnMapBuffer));
			pCommand = new(pMem)CRenderCommandUnMapBuffer(pBuffer);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandUnMapBuffer));
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandUnMapBuffer));
			pCommand = new(pMem)CRenderCommandUnMapBuffer(pBuffer);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandUnMapBuffer));
			IncrementCommands();
			break;
		}

		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------		
void CRenderer::DrawIndexedPrimitive(CDrawPrimitive* pDrawPrimitive)
{
	IRenderCommand* pCommand;
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			pDrawPrimitive->Draw(m_pImmediateContext);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandDrawIndexedPrimitive));
			pCommand = new(pMem)CRenderCommandDrawIndexedPrimitive(pDrawPrimitive);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandDrawIndexedPrimitive));
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandDrawIndexedPrimitive));
			pCommand = new(pMem)CRenderCommandDrawIndexedPrimitive(pDrawPrimitive);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandDrawIndexedPrimitive));
			IncrementCommands();
			break;
		}

		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------		
void CRenderer::MakeCopyAndDrawPrimitive(CBasePrimitive* pBasePrimitive)
{
	unsigned char* pMem;
	unsigned int CommandSize = sizeof(CRenderCommandMakeCopyAndDrawPrimitive) + pBasePrimitive->GetSizeWithMaterial();
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			pBasePrimitive->Draw(m_pImmediateContext);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, CommandSize);
			new(pMem)CRenderCommandMakeCopyAndDrawPrimitive(pBasePrimitive, CommandSize);
			m_pCommandBuffer->EndWrite(CommandSize);
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, CommandSize);
			new(pMem)CRenderCommandMakeCopyAndDrawPrimitive(pBasePrimitive, CommandSize);
			m_pCurrentRWBuffer->EndWrite(CommandSize);
			IncrementCommands();
			break;
		}

		default:
			Assert(0);
			break;
	}
}


//----------------------------------------------------------------------------------------------		
void CRenderer::SetConstantBuffer(CConstantBufferBase* pBuffer, CShaderResourceInfo* pResourceInfo)
{
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			pBuffer->Set(m_pImmediateContext, pResourceInfo);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandSetConstantBuffer));
			new(pMem)CRenderCommandSetConstantBuffer(pBuffer, pResourceInfo);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandSetConstantBuffer));
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandSetConstantBuffer));
			new(pMem)CRenderCommandSetConstantBuffer(pBuffer, pResourceInfo);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandSetConstantBuffer));
			IncrementCommands();
			break;
		}
		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------		
void CRenderer::UpdateConstantBufferBegin(CConstantBufferBase* pBuffer, void** ppData, unsigned int Size)
{
	unsigned char* pMem;
	Assert(m_CurrentCommandSize == 0);
	m_CurrentCommandSize = sizeof(CRenderCommandUpdateConstantBuffer) + Size + 16;
	Assert(m_CurrentCommandSize <= scConstantDataSize);
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			pMem = (unsigned char*)m_pConstantsData;
			*ppData = (void*)&pMem[sizeof(CRenderCommandUpdateConstantBuffer)];
			*ppData = (void*)POW2ALIGN((unsigned int)*ppData, 16);
			new(pMem)CRenderCommandUpdateConstantBuffer(pBuffer, *ppData, Size);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, m_CurrentCommandSize); // start of constant buffer data needs to be 16 byte aligned, so reserve 16 extra bytes
			*ppData = (void*)&pMem[sizeof(CRenderCommandUpdateConstantBuffer)];
			*ppData = (void*)POW2ALIGN((unsigned int)*ppData, 16);
			new(pMem)CRenderCommandUpdateConstantBuffer(pBuffer, *ppData, Size);
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, m_CurrentCommandSize); // start of constant buffer data needs to be 16 byte aligned, so reserve 16 extra bytes
			*ppData = (void*)&pMem[sizeof(CRenderCommandUpdateConstantBuffer)];
			*ppData = (void*)POW2ALIGN((unsigned int)*ppData, 16);
			new(pMem)CRenderCommandUpdateConstantBuffer(pBuffer, *ppData, Size);
			break;
		}

		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------
void CRenderer::UpdateConstantBufferEnd()
{
	Assert(m_CurrentCommandSize > 0);
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			CRenderCommandUpdateConstantBuffer* pRenderCommand = (CRenderCommandUpdateConstantBuffer*)m_pConstantsData;
			bool EndFrame = false;
			pRenderCommand->Execute(m_pImmediateContext, EndFrame);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->EndWrite(m_CurrentCommandSize);
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->EndWrite(m_CurrentCommandSize);
			IncrementCommands();
			break;
		}
		default:
			Assert(0);
			break;
	}
	m_CurrentCommandSize = 0;
}


//----------------------------------------------------------------------------------------------	
void CRenderer::SetRenderTargets(ID3D11RenderTargetView** pRTs, unsigned int NumTargets, ID3D11DepthStencilView* pDepthTarget)
{
	IRenderCommand* pCommand;
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			m_pImmediateContext->OMSetRenderTargets(NumTargets, pRTs, pDepthTarget);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandSetRenderTargets));
			pCommand = new(pMem)CRenderCommandSetRenderTargets(pRTs, NumTargets, pDepthTarget);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandSetRenderTargets));
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandSetRenderTargets));
			pCommand = new(pMem)CRenderCommandSetRenderTargets(pRTs, NumTargets, pDepthTarget);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandSetRenderTargets));
			IncrementCommands();
			break;
		}
		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------	
void CRenderer::ClearColor(ID3D11RenderTargetView* pRT, float Color[4])
{
	IRenderCommand* pCommand;
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			m_pImmediateContext->ClearRenderTargetView(pRT, Color);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandClearColor));
			pCommand = new(pMem)CRenderCommandClearColor(pRT, Color);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandClearColor));
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandClearColor));
			pCommand = new(pMem)CRenderCommandClearColor(pRT, Color);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandClearColor));
			IncrementCommands();
			break;
		}

		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------	
void CRenderer::ClearDepthStencil(ID3D11DepthStencilView* pDepthTarget, unsigned int Flags, float Depth, unsigned char Stencil)
{
	unsigned char* pMem;
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
		{
			m_pImmediateContext->ClearDepthStencilView(pDepthTarget, Flags, Depth, Stencil);
			break;
		}

		case eMultithreadedImmediate:
		{
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandClearDepth));
			new(pMem)CRenderCommandClearDepth(pDepthTarget, Flags, Depth, Stencil);
			m_pCommandBuffer->EndWrite(sizeof(CRenderCommandClearDepth));
			break;
		}

		case eMultithreadedDeferred:
		{
			m_pCurrentRWBuffer->BeginWrite((void**)&pMem, sizeof(CRenderCommandClearDepth));
			new(pMem)CRenderCommandClearDepth(pDepthTarget, Flags, Depth, Stencil);
			m_pCurrentRWBuffer->EndWrite(sizeof(CRenderCommandClearDepth));
			IncrementCommands();
			break;
		}

		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------	
void CRenderer::UtilityDrawRenderer(CUtilityDraw* pUtilityDraw, ID3D11DeviceContext* pDeviceContext, unsigned int VertexBuffer)
{
	switch (m_ExecutionModel)
	{
		case eSingleThreaded:
			pUtilityDraw->Render(NULL, NULL, VertexBuffer);
			break;
		case eMultithreadedImmediate:
		{
			unsigned char* pMem;
			m_pCommandBuffer->BeginWrite(&pMem, sizeof(CUtilityDrawRenderCommand));
			new(pMem)CUtilityDrawRenderCommand(pUtilityDraw, VertexBuffer);
			m_pCommandBuffer->EndWrite(sizeof(CUtilityDrawRenderCommand));
			break;
		}
		case eMultithreadedDeferred:
		{
		// need to flush all commands up to this point so that utility draw is executed at the end.
			if (m_NumCommands > 0)
			{
				FlushCommands();
				m_NumCommands = 0;
			}

			CJobUtilityDraw* pJob = m_rJobSystem->AcquireJob<CJobUtilityDraw>(CJobSystem::ePriority1);
			unsigned int NumCommandLists = m_NumCommandLists[m_CurrList];
			m_pProcessRenderCommandsSync->Reset(NumCommandLists); // Reset the event
			new (pJob)CJobUtilityDraw(pUtilityDraw, pDeviceContext, &m_pCommandLists[m_CurrList][NumCommandLists], VertexBuffer, m_pProcessRenderCommandsSync, NumCommandLists);
			m_rJobSystem->AddJob(pJob);
			m_NumCommandLists[m_CurrList]++;
			break;
		}
		default:
			Assert(0);
			break;
	}
}

//----------------------------------------------------------------------------------------------		
void CRenderer::EndFrame()
{
	unsigned char* pMem;
	m_pCommandBuffer->BeginWrite(&pMem, sizeof(CRenderCommandEndFrame));
	new(pMem)CRenderCommandEndFrame(&m_RenderThreadFrameCounter);
	m_pCommandBuffer->EndWrite(sizeof(CRenderCommandEndFrame));
}
