#ifndef _TEXTURESYSTEM_H_
#define _TEXTURESYSTEM_H_

#include "ElementList.h"
#include "ImageProcessing.h"
#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "JobSystem.h"
#include "Reference.h"

class CElementList;		
class CElementPool;
class CStackAllocator;
class CHashMap;
class CShaderResourceInfo;
class CStringDictionary;
class CJobSystem;
class CRenderer;
class CLoadingSystem;
class CConstantsSystem;
class CDrawPrimitiveSystem;
class CMaterialSystem;
class CComputePrimitive;
class CTextureSystem;

class CTextureState 
{
	public:
		enum eStateType
		{
			eUnloaded = 0,		// not loaded
			eReference,
			eRawDataLoaded,		// 
			eCreateMips,		// 
			eLoaded,			// texture is ready
			eDeviceRelease,		// graphics device is releasing the texture
			eMaxTextureLoadStates
		};
};



class CTexture : public CStateAccessor<CTextureState::eStateType>
{	
	public:

		enum eFormatModifiers
		{
			eNONE,
			eUNORM,
			eUNORM_SRGB,
			eFLOAT,
			eD24_UNORM_S8_UINT, // For depth target
			eMaxModifiers
		};

		static const unsigned int sc_MaxMipLevel = 12;
		CTexture();
		~CTexture();


		// Uses the d3d device to create the texture directly
		void CreateTexture2D(ID3D11Device* pd3dDevice, const void* pData, const char* pName);

		// Uses the d3d device to create the texture directly
		void CreateTexture2DCube(ID3D11Device* pd3dDevice, const char* pName);

		// uses compute shaders to create the texture from raw data
		void CreateTexture2D_Compute(ID3D11Device* pd3dDevice, const void* pData, const char* pName);

		//
		void Release();

		// gets a SRV of specifed mip level
		void GetSRV(ID3D11ShaderResourceView** ppSRV, eFormatModifiers FormatModifier, int MipLevel) const;

		// gets a render target view of specified mip level
		void GetRTV(ID3D11RenderTargetView** ppRTV, eFormatModifiers FormatModifier, int MipLevel ) const;

		// gets a UAV of specifed mip level
		void GetUAV(ID3D11UnorderedAccessView** ppUAV, eFormatModifiers FormatModifier, int MipLevel) const;

		// gets a depth stencil view of specified mip level
		void GetDSV(ID3D11DepthStencilView** ppRTV, eFormatModifiers FormatModifier, int MipLevel) const;

		// gets a render target view of specified mip level of a cube map face
		void GetCubeRTV(ID3D11RenderTargetView** ppRTV, eFormatModifiers FormatModifier, int MipLevel, unsigned int Face) const;

		// gets a shader resource view of specified mip level of a cube map face
		void GetCubeSRV(ID3D11ShaderResourceView** ppSRV, eFormatModifiers FormatModifier, int MipLevel, unsigned int Face) const;

		// gets an unordered access view of specified mip level of a cube map face
		void GetCubeUAV(ID3D11UnorderedAccessView** ppUAV, eFormatModifiers FormatModifier, int MipLevel,  unsigned int Face) const;

		// gets an SRV into buffer holding the raw texture data
		void GetRawDataSRV(ID3D11ShaderResourceView** ppSRV) const;

		// 
		unsigned int GetSubresourceDataSize(unsigned int StartMip, unsigned int EndMip) const;
		D3D11_TEXTURE2D_DESC		m_Desc;				// Texture is created using this

	private:								
		bool GenerateMipmaps();
		void CreateRawDataResource(ID3D11Device* pd3dDevice, const void* pRawData, unsigned int Size);
		void Create2D(ID3D11Device* pd3dDevice, const void* pData, const char* pName);
		void Create2DCube(ID3D11Device* pd3dDevice, const char* pName);
		static DXGI_FORMAT ApplyModifier(DXGI_FORMAT SrcFormat, eFormatModifiers FormatModifier);

		ID3D11Texture2D*			m_pTexture;

		ID3D11Buffer*				m_pRawTextureData;					// Buffer containing raw data for the highest mip level
		CTextureSystem*				m_pOwnerSystem;
		
	friend class CTextureSystem;
	friend class CMultiStageAssetContainer<CTexture, CTextureState::eMaxTextureLoadStates, CTextureState::eStateType, CTextureState::eUnloaded, CTextureState::eReference>;
};

class CSampler
{
	public:
		CSampler() : m_pSampler(NULL)
		{
		}
		void Initialize(ID3D11SamplerState*	pSampler);
		void Set(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo* pResourceInfo);
		void Release();
	private:
		ID3D11SamplerState*	m_pSampler;
};

class CTextureSystem
{
	public:
		enum eSamplerTypes
		{
			eSamplerLinear = 0,
			eSamplerPoint,
			eMaxSamplers
		};

		CTextureSystem(	CStringDictionary**  ppDictionary,
						CJobSystem** ppJobSystem,
						CRenderer**	ppRenderer,
						CLoadingSystem** ppLoadingSystem,
						CDrawPrimitiveSystem** ppDrawPrimitiveSystem,
						CMaterialSystem** ppMaterialSystem,
						CConstantsSystem** ppConstantsSystem);
		~CTextureSystem();
						

		CTexture& GetTextureRef(const char* pName);

		CTexture& CreateTextureFromFile(DXGI_FORMAT Format, unsigned int MipLevels, const char* pName); // 0 MipLevels = entire chain
		CTexture& CreateTexture(DXGI_FORMAT Format, unsigned int Width, unsigned int Height, unsigned int MipLevels, unsigned int ArraySize, D3D11_BIND_FLAG BindFlag, const char* pName);
		CTexture& CreateCubeTexture(DXGI_FORMAT Format, unsigned int Width, unsigned int Height, unsigned int MipLevels, unsigned int ArraySize, D3D11_BIND_FLAG BindFlags, const char* pName);

		CTexture& CreateBackBuffer(IDXGISwapChain* pSwapChain, unsigned int BackBufferIndex);

		void Tick(float DeltaSec);
		void Startup();
		void Shutdown();
		bool IsShutdown();
		void CreateSamplers(ID3D11Device* pd3dDevice);
		void ReleaseSamplers();
		CSampler& GetSampler(eSamplerTypes Type);
		void AcquireAddMutex();
		void ReleaseAddMutex();		
		static unsigned int GetFormatBPP(DXGI_FORMAT Format);
		bool GenerateMipmaps(CTexture* pTexture);
		bool GenerateDynEnvMapMipmaps(CTexture* pTexture);
		ID3D11Device* GetDevice();


	private:		

		CMultiStageAssetContainer<CTexture, CTextureState::eMaxTextureLoadStates, CTextureState::eStateType, CTextureState::eUnloaded, CTextureState::eReference> m_TextureAssets;
		bool									m_SamplersCreated;
		CSampler								m_Samplers[eMaxSamplers];

		CTexture*								m_pBlackTexture;
		unsigned int							m_BlackTextureData;

		CStackAllocator*						m_pStackAllocator; // Allocator used by the Texture System
		CLWMutex								m_AddTextureMutex;
		unsigned int							m_TotalTextures;
		CReference<CStringDictionary*>			m_rStringDictionary;
		CReference<CJobSystem*>					m_rJobSystem;
		CReference<CRenderer*>					m_rRenderer;
		CReference<CLoadingSystem*>				m_rLoadingSystem;
		CReference<CDrawPrimitiveSystem*>		m_rDrawPrimitiveSystem;
		CReference<CMaterialSystem*>			m_rMaterialSystem;
		CReference<CConstantsSystem*>			m_rConstantsSystem;

		CComputePrimitive*						m_pLinearTo2DPrimitive;
		CMaterial*								m_pLinearTo2DMaterial;
		CComputePrimitive*						m_pDownsampleBox2x2Primitive;
		CMaterial*								m_pDownsampleBox2x2Material;
		CComputePrimitive*						m_pDownsampleCubeBox2x2Primitive;
		CMaterial*								m_pDownsampleCubeBox2x2Material;
};

#endif