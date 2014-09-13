#ifndef _JOBLOADMATERIALLIBRARY_H_
#define _JOBLOADMATERIALLIBRARY_H_

#include "JobSystem.h"
#include "MaterialPhong.h"

class CLoadingSystem;
class CTextureSystem;
class CMaterialSystem;

class CJobLoadMaterialLibrary : public CJobSystem::CJob
{
	public:
		CJobLoadMaterialLibrary(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		CJobLoadMaterialLibrary(const char* pName,	CLoadingSystem * pLoadingSystem,
													CTextureSystem* pTextureSystem,
													CMaterialSystem* pMaterialSystem,
													CMaterialPhong::eLocalToWorldTransform LocalToWorldTransform);
		virtual ~CJobLoadMaterialLibrary();
		virtual unsigned int Execute(unsigned int ThreadID); 
		
	private:
		void AddMaterial(const char* pName, const char* pDiff, const char* pSpec, const char* pAlpha, const char* pNorm, XMVECTORF32& DiffuseParameters, XMVECTORF32& SpecularParameters);
		char m_Name[256];
		CLoadingSystem * m_pLoadingSystem;
		CTextureSystem* m_pTextureSystem;
		CMaterialSystem* m_pMaterialSystem;
		CMaterialPhong::eLocalToWorldTransform m_LocalToWorldTransform;

};


#endif