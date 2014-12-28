#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Thread.h"
#include "JobSystem.h"
#include "TimeLine.h"
#include "SegmentedBuffer.h"
#include "ThreadPool.h"
#include "Reference.h"
#include "RenderTargets.h"
#include "Postprocessor.h"

class CRingBuffer;
class CRendererRingBufferCallback;
class CStackAllocator;
class CTexture;
class CBuffer;
class CConstantBufferBase;
class CShaderResourceInfo;
class CUtilityDraw;
class CDrawPrimitive;
class CBasePrimitive;
class CShaderBase;
class CMaterial;
class IRenderCommand;
class CRWBuffer;
class CTextureSystem;
class CConstantsSystem;
class CDrawPrimitiveSystem;
class CGeometrySystem;
class CPooledAllocator;

class CDepthStencilState
{
	public:
		CDepthStencilState() : m_pDepthStencilState(NULL)
		{
		}

		void Initialize(ID3D11DepthStencilState* pDepthStencilState)
		{
			Assert(m_pDepthStencilState == NULL);
			m_pDepthStencilState = pDepthStencilState;
		}

		void Set(ID3D11DeviceContext* pDeviceContext)
		{
			pDeviceContext->OMSetDepthStencilState(m_pDepthStencilState, 0);
		}

		void Release()
		{
			if(m_pDepthStencilState)
			{
				m_pDepthStencilState->Release();
				m_pDepthStencilState = NULL;
			}
		}
	private:
		ID3D11DepthStencilState* m_pDepthStencilState;
};


class CRenderer
{
	public:
		CRenderer(HWND Wnd, CTimeLine**  ppTimeLine,
							CTextureSystem** ppTextureSystem,
							CJobSystem** ppJobSystem,
							CMaterialSystem** ppMaterialSystem,
							CConstantsSystem** ppConstantsSystem,
							CDrawPrimitiveSystem** ppDrawPrimitiveSystem,
							CGeometrySystem** ppGeometrySystem,
							CPooledAllocator* pPooledAllocator);
		~CRenderer();
		
		enum eDepthStates
		{
			eDepthNone,		
			eDepthLE,
			eMaxDepthStates
		};

		enum eBlendStates
		{
			eBlendNone,		
			eBlendModulate,
			eMaxBlendStates
		};

		enum eExecutionModel
		{
			eSingleThreaded,
			eMultithreadedImmediate,
			eMultithreadedDeferred
		};
		
		void Startup();
		void Shutdown();
		void InitDevice(ID3D11DeviceContext* pDeviceContext); // Sets some default values for the device context
		void Tick(unsigned int CurrentFrame, float DeltaTime);

		void Clear(); // Clear 'the command'
		void MapBuffer(CBuffer* pBuffer);	
		void MapBufferImmediate(CBuffer* pBuffer); // don't schedule render command
		void UnMapBuffer(CBuffer* pBuffer);		
		void DrawIndexedPrimitive(CDrawPrimitive* pDrawPrimitive);
		// Makes a copy of the primitive and material in the command buffer
		void MakeCopyAndDrawPrimitive(CBasePrimitive* pBasePrimitive);	
		
		void SetRenderTargets(CRenderTargets::eRenderTargetTypes ColorTarget, CRenderTargets::eRenderTargetTypes DepthTarget);
		void ClearColor(CRenderTargets::eRenderTargetTypes ColorTarget, float Color[4]);
		void ClearDepthStencil(CRenderTargets::eRenderTargetTypes DepthTarget, unsigned int Flags, float Depth, unsigned char Stencil);

		void SetConstantBuffer(CConstantBufferBase* pBuffer, CShaderResourceInfo* pResourceInfo);
		void UpdateConstantBufferBegin(CConstantBufferBase* pBuffer, void** ppData, unsigned int Size);
		void UpdateConstantBufferEnd();

		void GenerateDynamicEnvMipMaps();
		void Postprocess();

		void UtilityDrawRenderer(CUtilityDraw* pUtilityDraw, ID3D11DeviceContext* pDeviceContext, unsigned int VertexBuffer);

		void SetViewport(float TopLeftX, float TopLeftY, float Width, float Height, float MinDepth, float MaxDepth);
		void SetDepthState(ID3D11DeviceContext* pDeviceContext, eDepthStates DepthState);
		void SetBlendState(ID3D11DeviceContext* pDeviceContext, eBlendStates BlendState);
		ID3D11Device* GetDevice();
		ID3D11DeviceContext* GetDeviceContext(); // returns immediate context
		ID3D11DeviceContext* GetDeferredContext(unsigned int ContextIndex);	// Returns deferred context indexed by the processing thread
// Multithreaded deferred
		void ReturnCommandBufferSegment(CRWBuffer* pRWBuffer); // Job returns a command buffer segment back to the renderer after procesing it into a command list

		// use the renderer stack allocator
		void* Allocate(unsigned int Size);			
		float GetAspectRatio();
		void GetViewportSize(unsigned int& Width, unsigned int& Height);
		eExecutionModel GetExecutionModel() const
		{
			return m_ExecutionModel;
		}

		CDepthStencilState* GetDepthStencilState(eDepthStates DepthState)
		{
			return &m_pDepthStates[DepthState];
		}

	private:
		static void RenderLoop(void* pContext);
		void ExecuteCommand(IRenderCommand* pRenderCommand, bool& EndFrame);
		void Clear(ID3D11DeviceContext* pDeviceContext);
		void ClearStates();
		void Present();		
		void CleanupDevice();
		void NextFrame();
		void FindHighestDisplayResolution(unsigned int& Width, unsigned int& Height);
		void SetRenderTargets(ID3D11RenderTargetView** pRTs, unsigned int NumTargets, ID3D11DepthStencilView* pDepthTarget);
		void ClearColor(ID3D11RenderTargetView* pRT, float Color[4]);
		void ClearDepthStencil(ID3D11DepthStencilView* pDepthTarget, unsigned int Flags, float Depth, unsigned char Stencil);

// Multithreaded deferred		
		static const unsigned int		sc_CommandBufferSegments = 16;
		static const unsigned int		sc_CommandBufferSegmentSize = 4096;
		CSegmentedBuffer				m_CommandBuffer;
		static const unsigned int		sc_MaxCommandLists = 64;
		static const unsigned int		sc_CommandsPerSegment = 100;
		ID3D11CommandList*				m_pCommandLists[2][sc_MaxCommandLists];
		unsigned int					m_NumCommandLists[2];
		unsigned int					m_NumCommands;
		unsigned int					m_CurrList;
		CRWBuffer*						m_pCurrentRWBuffer;
		HANDLE							m_EventHandles[sc_MaxCommandLists];
		CEventArray*					m_pProcessRenderCommandsSync;
		void ExecuteCommandLists();
		void FlushCommands();			// Flush all commands to the thread pool for processing
		void IncrementCommands();		// Increments the number of commands and flushes if max per segment is exceeded

// Multithreaded immediate
		void EndFrame();
		CRendererRingBufferCallback*	m_pRingBufferCallback;
		CRingBuffer*					m_pCommandBuffer;

// Single threaded						
		void*							m_pConstantsData;
		eExecutionModel					m_ExecutionModel;
		CThread							m_RenderThread;
		volatile unsigned int			m_RenderThreadFrameCounter;
		CStackAllocator*				m_pStackAllocator;

		unsigned int					m_Width;
		unsigned int					m_Height;				
		D3D_FEATURE_LEVEL				m_FeatureLevel;
		ID3D11Device*					m_pd3dDevice;
		CRenderTargets					m_RenderTargets;
		CPostprocessor					m_Postprocessor;
		unsigned int					m_CurrentCommandSize;
#if _DEBUG
//		ID3D11Debug*					m_pD3DDebug;
#endif
		ID3D11DeviceContext*			m_pImmediateContext;
		ID3D11DeviceContext*			m_pDeferredContexts[sc_MaxProcessAffinity];
		IDXGISwapChain*					m_pSwapChain;
		ID3D11RasterizerState*			m_pRasterizerState;
		CDepthStencilState				m_pDepthStates[eMaxDepthStates];
		ID3D11BlendState*				m_pBlendStates[eMaxBlendStates];
	
// Profiling
		static CTimeLine::CEventDesc m_ProfilingEvents[RenderThreadEvents::eTotalEvents + 1];

// All the render commands		
		friend class CRenderCommandClear;
		friend class CRenderCommandMapBuffer;
		friend class CRenderCommandUnMapBuffer;		
		friend class CRenderCommandDrawIndexedPrimitive;
		friend class CRenderCommandMakeCopyAndDrawPrimitive;
		friend class CRenderCommandSetConstantBuffer;
		friend class CRenderCommandUpdateConstantBuffer;
		friend class CUtilityDrawRenderCommand;

// Components
		CReference<CTimeLine*> m_rTimeLine;
		CReference<CTextureSystem*> m_rTextureSystem;
		CReference<CJobSystem*> m_rJobSystem;
		CReference<CMaterialSystem*> m_rMaterialSystem;
		CReference<CConstantsSystem*> m_rConstantsSystem;
		CReference<CDrawPrimitiveSystem*> m_rDrawPrimitiveSystem;
		CReference<CGeometrySystem*> m_rGeometrySystem;

};


#endif