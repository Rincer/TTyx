#ifndef _MATERIALPHONG_H_
#define _MATERIALPHONG_H_

#include "MaterialSystem.h"
#include "ShaderPipeline.h"

// Instance for an Phong lighting model material
class CMaterialPhong final : public CMaterialRender
{
	public:

		// Vertex shader interfaces
		enum eTransform
		{
			eUnskinned,
			eSkinned,
			eMaxTransforms
		};

		enum eLocalToWorldTransform
		{
			eWorldPos,
			eLocalPos,
			eMaxLocalToWorldTransforms
		};

		// Pixel shader interfaces
		enum eDiffuse
		{
			eTexturedDiffuse,
			eUnTexturedDiffuse,
			eMaxDiffuseInstances
		};

		enum eOpacity
		{
			eTexturedOpacity,
			eUnTexturedOpacity,
			eMaxOpacityInstances
		};

		enum eNormal
		{
			eNormalMapped,
			eVertexNormal,
			eMaxNormalInstances
		};

		enum eSpecular
		{
			eZeroSpecular,
			eTexturedSpecular,
			eUnTexturedSpecular,
			eMaxSpecularInstances
		};

		class CParameters final : public CParametersBase
		{
			public:
				struct cbObjParams
				{
					XMFLOAT4 m_DiffuseParams;
					XMFLOAT4 m_SpecularParams;
				};

				CParameters(const char* pName,
					CTexture* pmap_Kd,
					CTexture* pmap_Ks,
					CTexture* pmap_d,
					CTexture* pmap_bump,
					CSampler*  m_pKdSampler,
					CSampler*  m_pKsSampler,
					CSampler*  m_pdSampler,
					CSampler*  m_pBumpSampler,
					eTransform TransformInterface,
					eLocalToWorldTransform LocalToWorldInterface,
					XMVECTOR DiffuseParams,
					XMVECTOR SpecularParams);

				CParameters();

				cbObjParams	m_PhongParams;

				CTexture*  m_pmap_Kd;
				CTexture*  m_pmap_Ks;
				CTexture*  m_pmap_d;
				CTexture*  m_pmap_bump;

				CSampler*  m_pKdSampler;
				CSampler*  m_pKsSampler;
				CSampler*  m_pdSampler;
				CSampler*  m_pBumpSampler;

				CCBuffer*	m_pCBuffer;			// buffer for the shader constants	

				eTransform m_TransformInterface;
				eLocalToWorldTransform m_LocalToWorldInterface;
				eDiffuse   m_DiffuseInterface;
				eOpacity   m_OpacityInterface;
				eNormal	   m_NormalInterface;
				eSpecular  m_SpecularInterface;
				virtual void Initialize(CConstantsSystem* pConstantsSystem);
				virtual bool IsLoaded() const;
				virtual void Update(CConstantsSystem* /*pConstantsSystem*/, CParametersBase* /*pParameters*/) {}
		};

		CMaterialPhong(); // Default constructor
		CMaterialPhong(CParameters* pParameters);
		~CMaterialPhong();

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


	private:

		// Per draw primitive parameters
		CShaderResourceInfo m_map_Kd;		// Diffuse map                           
		CShaderResourceInfo m_map_Ks;		// Specular map
		CShaderResourceInfo m_map_d;		// Alpha map
		CShaderResourceInfo m_map_bump;		// Bump map

		ID3D11ShaderResourceView* m_pDiffuse;
		ID3D11ShaderResourceView* m_pSpecular;
		ID3D11ShaderResourceView* m_pAlpha;
		ID3D11ShaderResourceView* m_pNormal;

		CShaderResourceInfo m_sam_Kd;		// Diffuse sampler
		CShaderResourceInfo m_sam_Ks;		// Specular sampler
		CShaderResourceInfo m_sam_d;		// Alpha sampler
		CShaderResourceInfo m_sam_bump;		// Bump sampler		
		CShaderResourceInfo m_ShaderConstants;	// All shader constants for phong	

		CParameters  m_Parameters; 

		// Vertex shader interfaces
		CClassInstanceInfo m_TransformInstances[eMaxTransforms];
		CClassInstanceInfo m_LocalToWorldInstances[eMaxLocalToWorldTransforms];
		// Pixel shader interfaces
		CClassInstanceInfo m_DiffuseInstances[eMaxDiffuseInstances];
		CClassInstanceInfo m_OpacityInstances[eMaxOpacityInstances];
		CClassInstanceInfo m_NormalInstances[eMaxNormalInstances];
		CClassInstanceInfo m_SpecularInstances[eMaxSpecularInstances];
		static D3D11_INPUT_ELEMENT_DESC m_VertexShaderObjLayout[];
		bool m_UseDynamicLinkage;
};

#endif