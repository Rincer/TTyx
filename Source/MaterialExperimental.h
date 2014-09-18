#ifndef _MATERIALEXPERIMENTAL_H_
#define _MATERIALEXPERIMENTAL_H_

#include "MaterialSystem.h"


// Instance for a plain color material
class CMaterialExperimental final : public CMaterialRender
{
	public:
		class CParameters final : public CParametersBase
		{
			public:
				CParameters(const char* pName,
							CTexture* DynEnvMap,
							CSampler* DynEnvMapSampler,
							XMFLOAT4 BaseColor,
							XMFLOAT4 RSM);
				CParameters();
				virtual void Initialize(CConstantsSystem* pConstantsSystem);
				virtual bool IsLoaded() const;
				virtual void Update(CConstantsSystem* /*pConstantsSystem*/, CParametersBase* /*pParameters*/) {}

				struct cbBrdfParams
				{
					XMFLOAT4 BaseColor;
					XMFLOAT4 RSM; // Roughness, Specular, Metalness
				};

				cbBrdfParams m_BrdfParams;
				CTexture*	m_pDynEnvMap;
				CSampler*	m_pDynEnvMapSampler;
				CCBuffer*	m_pCBuffer;			// buffer for the shader constants	
		};

		CMaterialExperimental();
		CMaterialExperimental(CParameters* pParameters);
		~CMaterialExperimental();

		virtual bool AllResourcesLoaded() const;
		virtual void InitializeShaders(CShaderPipeline* pShaderPipeline);
		virtual void InitializeBindings(CConstantsSystem* pConstantsSystem);
		virtual void Set(ID3D11DeviceContext* pDeviceContext) const;
		virtual void CreateViews();
		virtual void Release();

		virtual unsigned int GetSize() const
		{
			return sizeof(*this);
		}

		virtual void Update(CConstantsSystem* pConstantsSystem, CParametersBase* pParameters)
		{
			m_Parameters.Update(pConstantsSystem, pParameters);
		}

		virtual void Clone(void* pMem) const
		{
			memcpy(pMem, this, GetSize());
		}

		virtual void Unset(ID3D11DeviceContext* pDeviceContext) const; 


	protected:
		static D3D11_INPUT_ELEMENT_DESC CMaterialExperimental::m_VertexShaderLayout[];

		CParameters  m_Parameters; 
		CShaderResourceInfo m_ShaderConstants;	// All shader constants for brdf parameters	
		CShaderResourceInfo m_DynEnvMap;
		ID3D11ShaderResourceView* m_pDynEnvMapSRV;
		CShaderResourceInfo m_DynEnvMapSampler;
};

#endif