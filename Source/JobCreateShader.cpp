#include "stdafx.h"
#include "JobSystem.h"
#include "ShaderPipeline.h"

#include "JobCreateShader.h"

//--------------------------------------------------------------------------------------
CJobCreateShader::CJobCreateShader(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//--------------------------------------------------------------------------------------
CJobCreateShader::CJobCreateShader(CShaderBase* pShader,
									const char* pFileName,
									const char* pMain,
									const D3D_SHADER_MACRO* pDefines,
									const D3D11_INPUT_ELEMENT_DESC* pVertexShaderLayout,
									unsigned int LayoutSize,
									CRenderer* pRenderer,
									CHeapAllocator* pHeapAllocator) :
																	CJobSystem::CJob(CJobSystem::eJobCreateShader),
																	m_pShader(pShader),
																	m_pFileName(pFileName),
																	m_pMain(pMain),
																	m_pVertexShaderLayout(pVertexShaderLayout),
																	m_LayoutSize(LayoutSize),
																	m_pRenderer(pRenderer),
																	m_pHeapAllocator(pHeapAllocator)
{
	unsigned int DefineIndex = 0;
	m_Defines[DefineIndex].Name = NULL;	
	m_Defines[DefineIndex].Definition = NULL;
	if(pDefines)
	{
		while(pDefines[DefineIndex].Name)
		{
			m_Defines[DefineIndex] = pDefines[DefineIndex];
			DefineIndex++;
		}
		m_Defines[DefineIndex] = pDefines[DefineIndex]; //NULL terminate
	}
}
								
//--------------------------------------------------------------------------------------
CJobCreateShader::~CJobCreateShader()
{
}

//--------------------------------------------------------------------------------------
unsigned int CJobCreateShader::Execute(unsigned int ThreadID)
{
	ThreadID;
	m_pShader->Build(m_pFileName, m_pMain, m_Defines, m_pVertexShaderLayout, m_LayoutSize, m_pRenderer, m_pHeapAllocator);
	return 0;
}