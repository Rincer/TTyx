#ifndef _MATERIALTEXTURED2D_H_
#define _MATERIALTEXTURED2D_H_


#include "MaterialSystem.h"
#include "ShaderPipeline.h"

// Instance for a 2D textured material
class CMaterialTextured2D final : public CMaterialRender
{
public:

	enum eTextureChannels
	{
		eRGBA,
		eA,
		eMaxTextureChannelInterfaces
	};

	class CParameters : public CParametersBase
	{
	public:
		CParameters(const char* pName,
			CTexture* pTexture,
			CSampler* pSampler,
			eTextureChannels TextureChannelsInterface);
		CParameters();
		virtual void Initialize(CConstantsSystem* pConstantsSystem);
		virtual bool IsLoaded() const;
		virtual void Update(CConstantsSystem* /*pConstantsSystem*/, CParametersBase* /*pParameters*/) {}
		CTexture* m_pTexture;
		CSampler* m_pSampler;

		eTextureChannels m_TextureChannelsInterface;
	};

	CMaterialTextured2D();
	CMaterialTextured2D(CParameters* pParameters);
	~CMaterialTextured2D();

	virtual bool AllResourcesLoaded() const;
	virtual void InitializeShaders(CShaderPipeline* pShaderPipeline);
	virtual void InitializeBindings(CConstantsSystem* pConstantsSystem);
	virtual void Set(ID3D11DeviceContext* pDeviceContext) const;
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

	virtual void Clone(void* /*pMem*/) const { }
	virtual void Unset(ID3D11DeviceContext* /*pDeviceContext*/) const {}

protected:
	static D3D11_INPUT_ELEMENT_DESC CMaterialTextured2D::m_VertexShaderLayout[];

	CParameters m_Parameters;
	// Per draw primitive parameters
	CShaderResourceInfo m_Texture;	// Texture      			
	CShaderResourceInfo m_Sampler;	// Sampler      
	ID3D11ShaderResourceView* m_pTextureView;
};

#endif