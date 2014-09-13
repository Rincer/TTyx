#define _MATERIALPOSTPROCESS_H_

#include "MaterialSystem.h"
#include "ShaderPipeline.h"

class CDepthStencilState;

// Instance for a mip generator (compute shader) material
class CMaterialPostprocess final : public CMaterialRender
{
	public:

		class CParameters final : public CParametersBase
		{
			public:
				CParameters(const char* pName, 
					CTexture* pHDRTexture, 
					CSampler* pSampler, 
					CDepthStencilState* pSetDepthStencilState,
					CDepthStencilState* pUnsetDepthStencilState);

				CParameters();

				virtual void Initialize(CConstantsSystem* pConstantsSystem);
				virtual bool IsLoaded() const;
				virtual void Update(CConstantsSystem* pConstantsSystem, CParametersBase* pParameters);

				CTexture*			m_pHDRTexture;			
				CSampler*			m_pSampler;
				CDepthStencilState* m_pSetDepthStencilState;
				CDepthStencilState* m_pUnsetDepthStencilState;
		};

		CMaterialPostprocess();
		CMaterialPostprocess(CParameters* pParameters);
		~CMaterialPostprocess();

		virtual bool AllResourcesLoaded() const;
		virtual void InitializeShaders(CShaderPipeline* pShaderPipeline);
		virtual void InitializeBindings(CConstantsSystem* pConstantsSystem);
		virtual void Set(ID3D11DeviceContext* pDeviceContext) const;
		virtual void Unset(ID3D11DeviceContext* pDeviceContext) const;
		virtual void Release();
		virtual void CreateViews();
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


	protected:
		static D3D11_INPUT_ELEMENT_DESC CMaterialPostprocess::m_VertexShaderLayout[];
		CParameters  m_Parameters; 

		ID3D11ShaderResourceView* m_pHDRTextureSRV;
		CShaderResourceInfo	m_HDRSampler;
		CShaderResourceInfo	m_HDRTexture; 
};