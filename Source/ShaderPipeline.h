#ifndef _SHADER_PIPELINE_H_
#define _SHADER_PIPELINE_H_

#include "ElementList.h"
#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "Reference.h"

class CVertexShader;
class CPixelShader;
class CComputeShader;
class CHeapAllocator;
class CHashMap;
class CResourceInfo;
class CStringDictionary;
class CJobSystem;
class CRenderer;

class CShaderState 
{
	public:
		enum eStateType
		{
			eUnloaded,
			eReference,
			eDeviceRelease,
			eLoaded,
			eMaxStates
		};
};

enum eShaderType
{
	eVertexShader = 0,
	ePixelShader,
	eComputeShader,
	eMaxShaderTypes
};


//-----------------------------------------------------------------------------
class CResourceInfo
{

	public:
		CResourceInfo(unsigned int StartSlot, unsigned int NumViews) : m_StartSlot(StartSlot),
			m_NumViews(NumViews)
		{
		}

		CResourceInfo()
		{
			m_StartSlot = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1;
			m_NumViews = 0;
		}
		unsigned int m_StartSlot;
		unsigned int m_NumViews;
};

class CShaderResourceInfo
{
public:
	CResourceInfo m_ResourceInfo[eMaxShaderTypes];
};


class CShaderBase : public CStateAccessor<CShaderState::eStateType>
{
	public:
		
		CShaderBase()
		{
			SetState(CShaderState::eUnloaded);
		}
		
		virtual void Release() = 0;
		virtual void Build(	const char* pFileName,
							const char* pMain,
							const D3D_SHADER_MACRO* pDefines,
							const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout,
							unsigned int LayoutSize,
							CRenderer* pRenderer,
							CHeapAllocator* pHeapAllocator) = 0;

		
	protected:
		friend class CMultiStageAssetContainer<CPixelShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>;
		friend class CMultiStageAssetContainer<CVertexShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>;
		friend class CMultiStageAssetContainer<CComputeShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference>;
};


// Vertex shaders ---------------------------------------------------------------------
// generic base class for vertex shaders
class CVertexShader : public CShaderBase
{
	public:
		CVertexShader();
		virtual ~CVertexShader();
		virtual void Build(	const char* pFileName, 
					const char* pMain, 
					const D3D_SHADER_MACRO* pDefines, 
					const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout, 
					unsigned int LayoutSize, 
					CRenderer* pRenderer, 
					CHeapAllocator* pHeapAllocator);
		const CResourceInfo* GetResourceInfo(const char* pName) const;
		void Set(ID3D11DeviceContext* pDeviceContext);
		void SetClassInstance(unsigned int InterfaceSlot, ID3D11ClassInstance* pClassInstance);
		void GetClassInstance(ID3D11ClassInstance** ppClassInstance, const char* pName);		
		unsigned int GetInterfaceSlot(const char* pName);
		virtual void Release();
		
	protected:
		void BuildInternalStructures(ID3D11Device* pd3dDevice, D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout, unsigned int LayoutSize);
		ID3D11InputLayout*				m_pVertexLayout;
		ID3D11VertexShader*				m_pVertexShader;
		ID3D11ClassLinkage*				m_pVSClassLinkage; 
		ID3D11ClassInstance**			m_ppDynamicLinkageArray;	
		ID3D11ShaderReflection*			m_pReflector; 				
		unsigned int					m_NumVSInterfaces;			 				
		char*							m_pFileName;	
		char*							m_pEntryFunction;	
		CHashMap*						m_pConstantBuffersMap;	
};

// Pixel shaders ---------------------------------------------------------------------
// generic base class for Pixel shaders
class CPixelShader : public CShaderBase
{
	public:
		CPixelShader();
		virtual ~CPixelShader();
		virtual void Build(	const char* pFileName, 
							const char* pMain, 
							const D3D_SHADER_MACRO* pDefines, 
							const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout,
							unsigned int LayoutSize,
							CRenderer* pRenderer, 
							CHeapAllocator* pHeapAllocator);

		const CResourceInfo* GetResourceInfo(const char* pName) const;
		void Set(ID3D11DeviceContext* pDeviceContext);		
		void SetClassInstance(unsigned int InterfaceSlot, ID3D11ClassInstance* pClassInstance);
		void GetClassInstance(ID3D11ClassInstance** ppClassInstance, const char* pName);
		unsigned int GetInterfaceSlot(const char* pName);
		virtual void Release();
		
	protected:
		ID3D11PixelShader*				m_pPixelShader;
		ID3D11ClassLinkage*				m_pPSClassLinkage;  
		ID3D11ClassInstance**			m_ppDynamicLinkageArray;
		ID3D11ShaderReflection*			m_pReflector; 		
		unsigned int					m_NumPSInterfaces;
		char*							m_pFileName;	
		char*							m_pEntryFunction;	
		CHashMap*						m_pResourceMap;	// Maps a texture parameter to a resource slot.			

};

// Compute shaders ---------------------------------------------------------------------
// generic base class for Compute shaders
class CComputeShader : public CShaderBase
{
	public:
		CComputeShader();
		virtual ~CComputeShader();
		virtual void Build(const char* pFileName,
			const char* pMain,
			const D3D_SHADER_MACRO* pDefines,
			const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout,
			unsigned int LayoutSize,
			CRenderer* pRenderer,
			CHeapAllocator* pHeapAllocator);

		const CResourceInfo* GetResourceInfo(const char* pName) const;
		void Set(ID3D11DeviceContext* pDeviceContext);
		virtual void Release();

	protected:
		ID3D11ComputeShader*			m_pShader;
		ID3D11ShaderReflection*			m_pReflector;
		char*							m_pFileName;
		char*							m_pEntryFunction;
		CHashMap*						m_pResourceMap;			
};


// Class that handles all shader operations
class CShaderPipeline
{
	public:					
		CShaderPipeline(CStringDictionary** ppStringDictionary,
						CJobSystem** ppJobSystem,
						CRenderer** ppRenderer);
		~CShaderPipeline();
		void Tick(float DeltaSec);
		void Shutdown();
		bool IsShutdown();
		CVertexShader& GetVertexShader(const char* pFileName, const char* pMain, const D3D_SHADER_MACRO* pDefines, D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout, unsigned int LayoutSize);
		CPixelShader& GetPixelShader(const char* pFileName, const char* pMain, const D3D_SHADER_MACRO* pDefines);						
		CComputeShader& GetComputeShader(const char* pFileName, const char* pMain, const D3D_SHADER_MACRO* pDefines);
		CHeapAllocator* GetHeapAllocator();
				
	private:	
		void GetUniqueName(char* pUniqueName, const char* pFileName, const char* pMainName, const D3D_SHADER_MACRO* pDefines);
		CMultiStageAssetContainer<CVertexShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference> m_VertexShaders;
		CMultiStageAssetContainer<CPixelShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference> m_PixelShaders;
		CMultiStageAssetContainer<CComputeShader, CShaderState::eMaxStates, CShaderState::eStateType, CShaderState::eUnloaded, CShaderState::eReference> m_ComputeShaders;

		CHeapAllocator*	m_pHeapAllocator;	// used for any internal allocations inside CShaderPipeline
		CLWMutex m_AddVShaderMutex;
		CLWMutex m_AddPShaderMutex;
		CLWMutex m_AddCShaderMutex;
		CReference<CStringDictionary*> m_rStringDictionary;
		CReference<CJobSystem*> m_rJobSystem;
		CReference<CRenderer*> m_rRenderer;

};



#endif