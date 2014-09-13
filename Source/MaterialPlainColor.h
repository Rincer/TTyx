#ifndef _MATERIALPLANECOLOR_H_
#define _MATERIALPLANECOLOR_H_

#include "MaterialSystem.h"


// Instance for a plain color material
class CMaterialPlainColor final : public CMaterialRender
{
public:
	enum eTransformSpace
	{
		eWorldSpace,
		eViewSpace,
		eMaxTransformSpaces
	};
	class CParameters : public CParametersBase
	{
		public:
			CParameters(const char* pName, eTransformSpace TransformSpace);
			CParameters();
			virtual void Initialize(CConstantsSystem* pConstantsSystem);
			virtual bool IsLoaded() const;
			virtual void Update(CConstantsSystem* /*pConstantsSystem*/, CParametersBase* /*pParameters*/) {}
			eTransformSpace m_TransformSpace;
	};

	CMaterialPlainColor();
	CMaterialPlainColor(CParameters* pParameters);
	~CMaterialPlainColor();

	virtual bool AllResourcesLoaded() const;
	virtual void InitializeShaders(CShaderPipeline* pShaderPipeline);
	virtual void InitializeBindings(CConstantsSystem* pConstantsSystem);
	virtual void Set(ID3D11DeviceContext* pDeviceContext) const;
	virtual void Release();
	virtual void CreateViews() {}
	virtual unsigned int GetSize() const
	{
		return sizeof(*this);
	}

	virtual void Update(CConstantsSystem* pConstantsSystem, CParametersBase* pParameters)
	{
		m_Parameters.Update(pConstantsSystem, pParameters);
	}

	virtual void Clone(void* /*pMem*/) const {}
	virtual void Unset(ID3D11DeviceContext* /*pDeviceContext*/) const {}


protected:
	static D3D11_INPUT_ELEMENT_DESC CMaterialPlainColor::m_VertexShaderLayout[];
	CParameters m_Parameters;

};

#endif