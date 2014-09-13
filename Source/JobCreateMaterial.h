#ifndef _JOBCREATEMATERIAL_H_
#define _JOBCREATEMATERIAL_H_

#include "JobSystem.h"

class CMaterial;
class CConstantsSystem;
class CShaderPipeline;

class CJobCreateMaterial : public CJobSystem::CJob
{
	public:
		CJobCreateMaterial(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		CJobCreateMaterial(	CMaterial* pMaterial,
							CConstantsSystem* pConstantsSystem,
							CShaderPipeline* pShaderPipeline);
								
		virtual ~CJobCreateMaterial();
		virtual unsigned int Execute(unsigned int ThreadID); 

	private:
		CMaterial* m_pMaterial;	
		CConstantsSystem* m_pConstantsSystem;
		CShaderPipeline* m_pShaderPipeline;

};

#endif