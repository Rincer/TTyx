#ifndef _RENDERCOMMAND_H_
#define _RENDERCOMMAND_H_

#include "JobSystem.h"
#include "RenderTargets.h"

class CRenderer;
class CTexture;
class CShaderBase;
class CMaterial;
class CBuffer;
class CDrawPrimitive;
class CBasePrimitive;
class CConstantBufferBase;
class CShaderResourceInfo;
class CUtilityDraw;
class CRWBuffer;

class IRenderCommand
{
	public: 
		virtual void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame) = 0;
		virtual unsigned int Size() = 0;
};

//----------------------------------------------------------------------------------------------
class CRenderCommandClear : public IRenderCommand
{
	public:
		CRenderCommandClear(CRenderer* pRenderer) : m_pRenderer(pRenderer)
		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();				
	private:
		CRenderer* m_pRenderer;
};

//----------------------------------------------------------------------------------------------
class CRenderCommandSkipBytes : public IRenderCommand
{
	public:
		CRenderCommandSkipBytes(unsigned int BytesToSkip) : m_BytesToSkip(BytesToSkip)
		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();
				
	private:
		unsigned int m_BytesToSkip;
};

//----------------------------------------------------------------------------------------------
class CRenderCommandEndFrame : public IRenderCommand
{
	public:
		CRenderCommandEndFrame(volatile unsigned int* pFrameCount) : m_pFrameCount(pFrameCount)
		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();		

	private:
		volatile unsigned int* m_pFrameCount;
};


//----------------------------------------------------------------------------------------------
class CRenderCommandMapBuffer : public IRenderCommand
{
	public:
		CRenderCommandMapBuffer(CBuffer* pBuffer) : m_pBuffer(pBuffer)

		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();				
	private:
		CBuffer*		m_pBuffer;	
};

//----------------------------------------------------------------------------------------------
class CRenderCommandUnMapBuffer : public IRenderCommand
{
	public:
		CRenderCommandUnMapBuffer(CBuffer* pBuffer) : m_pBuffer(pBuffer)

		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();				
	private:
		CBuffer*		m_pBuffer;	
};


//----------------------------------------------------------------------------------------------
class CRenderCommandDrawIndexedPrimitive : public IRenderCommand
{
	public:
		CRenderCommandDrawIndexedPrimitive(CDrawPrimitive*	pDrawPrimitive) : m_pDrawPrimitive(pDrawPrimitive)

		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();				
	private:
		CDrawPrimitive*		m_pDrawPrimitive;	
};

//----------------------------------------------------------------------------------------------
class CRenderCommandMakeCopyAndDrawPrimitive : public IRenderCommand
{
public:
	CRenderCommandMakeCopyAndDrawPrimitive(CBasePrimitive*	pPrimitive, unsigned int Size);

	void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
	unsigned int Size();
private:
	CBasePrimitive*		m_pPrimitive;
	unsigned int		m_Size;
};


//----------------------------------------------------------------------------------------------
class CRenderCommandSetConstantBuffer : public IRenderCommand
{
	public:
		CRenderCommandSetConstantBuffer(CConstantBufferBase* pBuffer, CShaderResourceInfo* pResourceInfo) : m_pBuffer(pBuffer),
																									  m_pResourceInfo(pResourceInfo)

		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();	
	private:
		CConstantBufferBase* m_pBuffer;
		CShaderResourceInfo* m_pResourceInfo;
};

//----------------------------------------------------------------------------------------------
class CRenderCommandUpdateConstantBuffer : public IRenderCommand
{
	public:
		CRenderCommandUpdateConstantBuffer(CConstantBufferBase* pBuffer, const void* pData, unsigned int Size) : m_pBuffer(pBuffer),
																												 m_pData(pData),
																												 m_Size(Size)
																											

		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();	
	private:
		CConstantBufferBase* m_pBuffer;
		const void* m_pData;
		unsigned int m_Size; 
};

//----------------------------------------------------------------------------------------------
class CRenderCommandSetRenderTargets : public IRenderCommand
{
	public:
		CRenderCommandSetRenderTargets(ID3D11RenderTargetView** pRTs, unsigned int NumTargets, ID3D11DepthStencilView* pDepthTarget);
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();
	private:
		ID3D11RenderTargetView* m_pRTs[CRenderTargets::sc_MaxMRTs];
		ID3D11DepthStencilView* m_pDepth;
		unsigned int m_NumTargets;
};

//----------------------------------------------------------------------------------------------
class CRenderCommandClearColor : public IRenderCommand
{
	public:
		CRenderCommandClearColor(ID3D11RenderTargetView* pRT, float Color[4]) : m_pRT(pRT)
		{
			m_Color[0] = Color[0];
			m_Color[1] = Color[1];
			m_Color[2] = Color[2];
			m_Color[3] = Color[3];
		}

		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();
	private:
		ID3D11RenderTargetView* m_pRT;
		float					m_Color[4];		
};

//----------------------------------------------------------------------------------------------
class CRenderCommandClearDepth : public IRenderCommand
{
	public:
		CRenderCommandClearDepth(ID3D11DepthStencilView* pDepthTarget, unsigned int Flags, float Depth, unsigned char Stencil) : m_pDepthTarget(pDepthTarget),
			m_Flags(Flags),
			m_Depth(Depth),
			m_Stencil(Stencil)
		{}

		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();

	private:
		ID3D11DepthStencilView* m_pDepthTarget;
		unsigned int			m_Flags;
		float					m_Depth;
		unsigned char			m_Stencil;
};


//----------------------------------------------------------------------------------------------
class CRenderCommandSetViewport : public IRenderCommand
{
	public:
		CRenderCommandSetViewport(float TopLeftX, float TopLeftY, float Width, float Height, float MinDepth, float MaxDepth) : m_TopLeftX(TopLeftX),
			m_TopLeftY(TopLeftY),
			m_Width(Width),
			m_Height(Height),
			m_MinDepth(MinDepth),
			m_MaxDepth(MaxDepth)
		{}

		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();

	private:
		float m_TopLeftX;
		float m_TopLeftY;
		float m_Width; 
		float m_Height; 
		float m_MinDepth; 
		float m_MaxDepth;
};



//----------------------------------------------------------------------------------------------
class CUtilityDrawRenderCommand : public IRenderCommand
{
	public:
		CUtilityDrawRenderCommand(CUtilityDraw* pUtilityDraw, unsigned int VertexBuffer) :	m_pUtilityDraw(pUtilityDraw),
																							m_VertexBuffer(VertexBuffer)																														
																											

		{
		}
		
		void Execute(ID3D11DeviceContext* pDeviceContext, bool& EndFrame);
		unsigned int Size();	
	private:
		CUtilityDraw* m_pUtilityDraw;
		unsigned int m_VertexBuffer; 		
};

#endif _RENDERCOMMAND_H_