#ifndef _RENDERTARGETS_H_
#define _RENDERTARGETS_H_

class CTexture;
class CTextureSystem;


class CRenderTargets final
{
	public:

		static const unsigned int sc_MaxMRTs = 4;
		static const unsigned int sc_DynamicEnvironmentMapWidth = 256;
		static const unsigned int sc_DynamicEnvironmentMapHeight = 256;

		enum eRenderTargetTypes
		{
			eNull = 0, // null target
			eBackBuffer,
			eFrameBuffer, 
			eDepthStencil,
			eDynamicEnvironmentMap,
			eDynamicEnvironmentMapFace0,
			eDynamicEnvironmentMapFace1,
			eDynamicEnvironmentMapFace2,
			eDynamicEnvironmentMapFace3,
			eDynamicEnvironmentMapFace4,
			eDynamicEnvironmentMapFace5,
			eDynamicEnvironmentMapDepth,
			eMaxRenderTargets
		};

		CRenderTargets();
		void Startup(unsigned int BackBufferWidth, unsigned int BackBufferHeight, CTextureSystem* pTextureSystem, IDXGISwapChain* pSwapChain);
		void Shutdown();
		ID3D11View* GetRTV(eRenderTargetTypes Type);
		CTexture*	GetTexture(eRenderTargetTypes Type) 
		{
			return m_RenderTargets[Type].m_pTexture;
		}

	private:

		class CRenderTarget
		{
			public:
				CRenderTarget() : m_pTexture(NULL),
					m_pRTV(NULL),
					m_pSRV(NULL),
					m_pUAV(NULL)
				{
				} 

				ID3D11View* GetView() const
				{
					return m_pRTV;
				}

				CTexture*	m_pTexture;
				ID3D11View* m_pRTV;
				ID3D11View* m_pSRV;
				ID3D11View* m_pUAV;
		};

		CRenderTarget	m_RenderTargets[eMaxRenderTargets];
};


#endif