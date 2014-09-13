#ifndef _JOBREADFILE_H_
#define _JOBREADFILE_H_

#if 0 // Currently not used, will have to add and input buffer to read the file into
#include "JobSystem.h"

class CJobReadFile : public CJobSystem::CJob
{
	public:
		CJobReadFile();
		~CJobReadFile();
		virtual unsigned int Execute(unsigned int ThreadID);
		void Initialize(const char* pName);
		virtual bool Startup();
		void Shutdown();

	private:			
		HANDLE			m_File; 	
		char			m_Name[256];
};
#endif
#endif