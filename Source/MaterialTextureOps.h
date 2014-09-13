#ifndef _MATERIALTEXTUREOPS_H_
#define _MATERIALTEXTUREOPS_H_

#include "MaterialSystem.h"
#include "ShaderPipeline.h"

// Instance for a mip generator (compute shader) material
class CMaterialTextureOps final : public CMaterialCompute
{
	public:

		enum eSamplerOp
		{
			eByteAddressBuffer,
			eDownsampleBox2x2,
			eDownsampleBox2x2Cube,
			eMaxSamplerOp
		};

		class CParameters final : public CParametersBase
		{
			public:
				CParameters(const char* pName, CTexture* pTexture, CSampler* pSampler, eSamplerOp SamplerOp, unsigned int Width, unsigned int Height, unsigned int Face, unsigned int DstMipLevel);
				CParameters();

				virtual void Initialize(CConstantsSystem* pConstantsSystem);
				virtual bool IsLoaded() const;
				virtual void Update(CConstantsSystem* pConstantsSystem, CParametersBase* pParameters);

				struct cbTextureParams
				{
					float m_PixelsToUVs[4];
				};
				
				cbTextureParams		m_TextureParams;
				CTexture*			m_pTexture;			// Contains the byte address buffer and the UAVs
				CCBuffer*			m_pCBuffer;			// buffer for texture params
				CSampler*			m_pSampler;
				unsigned int		m_Face;
				unsigned int		m_DstMipLevel;
				eSamplerOp			m_SamplerOp;
		};

		CMaterialTextureOps();
		CMaterialTextureOps(CParameters* pParameters);
		~CMaterialTextureOps();

		virtual bool AllResourcesLoaded() const;
		virtual void InitializeShaders(CShaderPipeline* pShaderPipeline);
		virtual void InitializeBindings(CConstantsSystem* pConstantsSystem);
		virtual void Set(ID3D11DeviceContext* pDeviceContext) const;
		virtual void Unset(ID3D11DeviceContext* pDeviceContext) const;
		virtual void Release();
		virtual void CreateViews() {}
		virtual bool ShadersLoaded() const;
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
		CParameters  m_Parameters; // Will point into ParametersMemory

		CShaderResourceInfo			m_TextureParams;
		CShaderResourceInfo			m_SrcBuffer;
		CShaderResourceInfo			m_SrcSampler;
		CShaderResourceInfo			m_SrcTexture; 
		CShaderResourceInfo			m_DstTexture;
		CShaderResourceInfo			m_SrcTextureCube;
		CShaderResourceInfo			m_DstTextureCube;

};

#endif