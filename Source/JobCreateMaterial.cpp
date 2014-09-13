#include "stdafx.h"
#include "JobSystem.h"
#include "MaterialSystem.h"

#include "JobCreateMaterial.h"

//--------------------------------------------------------------------------------------
CJobCreateMaterial::CJobCreateMaterial(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//--------------------------------------------------------------------------------------
CJobCreateMaterial::CJobCreateMaterial(	CMaterial* pMaterial,
										CConstantsSystem* pConstantsSystem,
										CShaderPipeline* pShaderPipeline) : CJobSystem::CJob(CJobSystem::eJobCreateMaterial),
																			m_pMaterial(pMaterial),
																			m_pConstantsSystem(pConstantsSystem),
																			m_pShaderPipeline(pShaderPipeline)

																	
{
}
								
//--------------------------------------------------------------------------------------
CJobCreateMaterial::~CJobCreateMaterial()
{
}

//--------------------------------------------------------------------------------------
unsigned int CJobCreateMaterial::Execute(unsigned int ThreadID)
{
	m_pMaterial->InitializeShaders(m_pShaderPipeline);
	class CBlockingCondition : public CJobSystem::IBlockingCondition
	{
		public:
			CBlockingCondition(CMaterial* pMaterial) : m_pMaterial(pMaterial)
			{
			}
			
			virtual bool IsBlocked()
			{
				return (!m_pMaterial->ShadersLoaded());
			}
			
		private:
			CMaterial* m_pMaterial;		
	};
	
	CBlockingCondition BlockingCondition(m_pMaterial);	
	if(BlockingCondition.IsBlocked())
	{
		m_pJobSystem->YieldExecution(m_Id, m_Type, ThreadID, &BlockingCondition);
	}
	m_pMaterial->InitializeBindings(m_pConstantsSystem);
	return 0;
}
