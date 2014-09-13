#ifndef _MATERIALSYSTEM_H_
#define _MATERIALSYSTEM_H_

#include "MultiStageAssetContainer.h"
#include "LWMutex.h"
#include "Reference.h"
#include "ShaderPipeline.h"
#include "StringDictionary.h"
#include "JobCreateMaterial.h"

class CMaterialPlainColor;
class CMaterialTextured2D;
class CMaterialPhong;
class CMaterialMipGenerator;
class CMaterialTextureOps;
class CMaterialExperimental;
class CMaterialPostprocess;
class CVertexShader;
class CPixelShader;
class CComputeShader;
class CHashMap;
class CHeapAllocator;
class CTexture;
class CCBuffer;
class CStringDictionary;
class CJobSystem;
class CShaderPipeline;
class CConstantsSystem;
class CSampler;

//-----------------------------------------------------------------------------
class CClassInstanceInfo
{
	public:
		CClassInstanceInfo()
		{
			m_InterfaceSlot = 0;
			m_pClassInstance = NULL;
		}

		CClassInstanceInfo(unsigned int	InterfaceSlot, ID3D11ClassInstance*	pClassInstance) : m_InterfaceSlot(InterfaceSlot),
																							  m_pClassInstance(pClassInstance)
		{
		}
		
		unsigned int			m_InterfaceSlot;
		ID3D11ClassInstance*	m_pClassInstance;
};

//-----------------------------------------------------------------------------
class CParametersBase
{
	public:
		CParametersBase(const char* pName)
		{
			strcpy_s(m_Name, 255, pName);
		}
		
		char m_Name[256];		
		virtual void Initialize(CConstantsSystem* pConstantsSystem) = 0;
		virtual bool IsLoaded() const = 0;
		virtual void Update(CConstantsSystem* pConstantsSystem, CParametersBase* pParameters) = 0;
};

//---------------------------------------------------------------------------------------------------------------------------------
class CMaterialState
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



//---------------------------------------------------------------------------------------------------------------------------------
class CMaterial : public CStateAccessor<CMaterialState::eStateType>
{
	public:		

		typedef CMultiStageAssetContainer<CMaterialPhong, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference> T_PhongMaterials;
		typedef CMultiStageAssetContainer<CMaterialPlainColor, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference> T_PlainColorMaterials;
		typedef CMultiStageAssetContainer<CMaterialTextured2D, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference> T_Textured2DMaterials;
		typedef CMultiStageAssetContainer<CMaterialTextureOps, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference> T_TextureOpsMaterials;
		typedef CMultiStageAssetContainer<CMaterialExperimental, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference> T_ExperimentalMaterials;
		typedef CMultiStageAssetContainer<CMaterialPostprocess, CMaterialState::eMaxStates, CMaterialState::eStateType, CMaterialState::eUnloaded, CMaterialState::eReference> T_PostprocessMaterials;


		CMaterial() 					 
		{			
			SetState(CMaterialState::eUnloaded);
		}
		virtual void InitializeShaders(CShaderPipeline* pShaderPipeline) = 0;
		virtual void InitializeBindings(CConstantsSystem* pConstantsSystem) = 0;
		virtual void Set(ID3D11DeviceContext* pDeviceContext) const = 0;
		virtual void Unset(ID3D11DeviceContext* pDeviceContext) const = 0;
		virtual void Release() = 0;
		virtual bool AllResourcesLoaded() const = 0;
		virtual bool ShadersLoaded() const = 0;
		virtual unsigned int GetSize() const = 0;
		virtual void Update(CConstantsSystem* pConstantsSystem, CParametersBase* pParameters) = 0;
		virtual void Clone(void* pMem) const = 0; 
		virtual void CreateViews() = 0;

	protected:			
		virtual void BindResource(CShaderResourceInfo& ResourceInfo, const char* pName) = 0;
		void SetSRV(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo& ResourceInfo, ID3D11ShaderResourceView* pSRV) const;
		void SetUAV(ID3D11DeviceContext* pDeviceContext, const CShaderResourceInfo& ResourceInfo, ID3D11UnorderedAccessView* pUAV) const;

		friend class T_PhongMaterials;
		friend class T_PlainColorMaterials; 
		friend class T_Textured2DMaterials; 
		friend class T_TextureOpsMaterials;
		friend class T_ExperimentalMaterials;
		friend class T_PostprocessMaterials;
};

//---------------------------------------------------------------------------------------------------------------------------------
class CMaterialRender : public CMaterial
{
	public:
		CMaterialRender() : m_pVertexShader(NULL),
							m_pPixelShader(NULL)
		{
		}

		virtual bool ShadersLoaded() const
		{
			return	m_pVertexShader && m_pPixelShader &&
					m_pVertexShader->IsLoaded() && m_pPixelShader->IsLoaded();
		}

	protected:
		virtual void BindResource(CShaderResourceInfo& ResourceInfo, const char* pName)
		{
			const CResourceInfo* pResourceInfo;
			if (m_pVertexShader)
			{
				pResourceInfo = m_pVertexShader->GetResourceInfo(pName);
				if (pResourceInfo)
				{
					ResourceInfo.m_ResourceInfo[eVertexShader] = *pResourceInfo;
				}
			}

			if (m_pPixelShader)
			{
				pResourceInfo = m_pPixelShader->GetResourceInfo(pName);
				if (pResourceInfo)
				{
					ResourceInfo.m_ResourceInfo[ePixelShader] = *pResourceInfo;
				}
			}
		}
		CVertexShader*	m_pVertexShader;
		CPixelShader*	m_pPixelShader;
};

//---------------------------------------------------------------------------------------------------------------------------------
class CMaterialCompute : public CMaterial
{
	public:
		CMaterialCompute() : m_pComputeShader(NULL)
		{
		}

		virtual bool ShadersLoaded() const
		{
			return m_pComputeShader->IsLoaded() && m_pComputeShader->IsLoaded();
		}

	protected:
		virtual void BindResource(CShaderResourceInfo& ResourceInfo, const char* pName)
		{
			const CResourceInfo* pResourceInfo;
			if (m_pComputeShader)
			{
				pResourceInfo = m_pComputeShader->GetResourceInfo(pName);
				if (pResourceInfo)
				{
					ResourceInfo.m_ResourceInfo[eComputeShader] = *pResourceInfo;
				}
			}
		}
		CComputeShader* m_pComputeShader;
};

// Class that handles all material operations
class CMaterialSystem
{
	public:
		static const unsigned int scMaxDefines = 10;
		enum eMaterials
		{
			eMaterialPhong = 0,
			eMaterialPlainColor,
			eMaterialTextured2D,
			eMaterialMipGenerator,
			eMaterialLinearTo2D,
			eMaterialExperimental,
			eMaxMaterials
		};
				
		CMaterialSystem(CStringDictionary**	ppStringDictionary,
						CJobSystem**		ppJobSystem,
						CShaderPipeline**	ppShaderPipeline,
						CConstantsSystem**  ppConstantsSystem);
		~CMaterialSystem();
		void Tick(float DeltaSec);
		void Shutdown();
		bool IsShutdown();
		
		CMaterial& GetMaterialReference(eMaterials Type, const char* pName);

		template<class Material, class MaterialParameters> Material& GetMaterial(MaterialParameters* pParameters)
		{
			m_AddMaterialMutex.Acquire();
			bool Exists = false;
			unsigned long long Key = m_rStringDictionary->AddString(pParameters->m_Name);
			Material* pMaterial = NULL;
			unsigned int ForwardReference = 0;
			pMaterial = &(MaterialContainerGetter(pMaterial).AddAsset(Key, Exists));
			ForwardReference = pMaterial->IsReference();
			if ((!Exists) || ForwardReference)
			{
				new (pMaterial)Material(pParameters);
				CJobCreateMaterial* pJobCreateMaterial = m_rJobSystem->AcquireJob<CJobCreateMaterial>(CJobSystem::ePriority0);
				new (pJobCreateMaterial)CJobCreateMaterial(pMaterial, *m_rConstantsSystem, *m_rShaderPipeline);
				m_rJobSystem->AddJob(pJobCreateMaterial);
			}
			m_AddMaterialMutex.Release();
			return *pMaterial;
		}

		CHeapAllocator* GetHeapAllocator();
		
	private:

		CMaterial::T_PhongMaterials			m_PhongMaterials;
		CMaterial::T_PlainColorMaterials	m_PlainColorMaterials;
		CMaterial::T_Textured2DMaterials	m_Textured2DMaterials;
		CMaterial::T_TextureOpsMaterials	m_TextureOpsMaterials;
		CMaterial::T_ExperimentalMaterials	m_ExperimentalMaterials;
		CMaterial::T_PostprocessMaterials	m_PostprocessMaterials;

		CMaterial::T_PhongMaterials&		MaterialContainerGetter(CMaterialPhong*) { return m_PhongMaterials; }
		CMaterial::T_PlainColorMaterials&	MaterialContainerGetter(CMaterialPlainColor*) { return m_PlainColorMaterials; }
		CMaterial::T_Textured2DMaterials&	MaterialContainerGetter(CMaterialTextured2D*) { return m_Textured2DMaterials; }
		CMaterial::T_TextureOpsMaterials&	MaterialContainerGetter(CMaterialTextureOps*) { return m_TextureOpsMaterials; }
		CMaterial::T_ExperimentalMaterials&	MaterialContainerGetter(CMaterialExperimental*) { return m_ExperimentalMaterials; }
		CMaterial::T_PostprocessMaterials&	MaterialContainerGetter(CMaterialPostprocess*) { return m_PostprocessMaterials; }

		CHeapAllocator*	m_pHeapAllocator;		// used for any internal allocations inside CMaterialSystem
		CLWMutex m_AddMaterialMutex;
		CReference<CStringDictionary*>	m_rStringDictionary;
		CReference<CJobSystem*>			m_rJobSystem;
		CReference<CShaderPipeline*>	m_rShaderPipeline;
		CReference<CConstantsSystem*>   m_rConstantsSystem;

};

#endif