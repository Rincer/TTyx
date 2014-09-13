#ifndef _JOBCREATESHADER_H_
#define _JOBCREATESHADER_H_

#include "MaterialSystem.h"
#include "JobSystem.h"

class CShaderBase;

class CJobCreateShader : public CJobSystem::CJob
{
	public:
		CJobCreateShader(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		CJobCreateShader(CShaderBase* pShader,
						 const char* pFileName,
						 const char* pMain,
						 const D3D_SHADER_MACRO* pDefines,
						 const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout,
						 unsigned int LayoutSize,
						 CRenderer* pRenderer,
						 CHeapAllocator* pHeapAllocator);
								
		virtual ~CJobCreateShader();
		virtual unsigned int Execute(unsigned int ThreadID); 

	private:
		CShaderBase* m_pShader;
		const char* m_pFileName;
		const char* m_pMain; 
		D3D_SHADER_MACRO m_Defines[CMaterialSystem::scMaxDefines];
		const D3D11_INPUT_ELEMENT_DESC* m_pVertexShaderLayout;
		unsigned int m_LayoutSize;
		CRenderer* m_pRenderer;
		CHeapAllocator* m_pHeapAllocator;
};

#endif