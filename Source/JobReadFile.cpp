#include "stdafx.h"
#if 0 // Currently not used, will have to add and input buffer to read the file into
#include "LoadingSystem.h"

#include "JobReadFile.h"

//-------------------------------------------------------------------------------
CJobReadFile::CJobReadFile() : CJob(CJobSystem::eJobReadFile)
{
	m_File = INVALID_HANDLE_VALUE;
}

//-------------------------------------------------------------------------------
CJobReadFile::~CJobReadFile()
{
}

//-------------------------------------------------------------------------------
unsigned int CJobReadFile::Execute(unsigned int ThreadID)
{
	ThreadID; // C4100
	DWORD BytesRead; 			
	BOOL Res = ReadFile(m_File, m_pOutputs[0], m_OutputSizes[0], &BytesRead, NULL);
	LPCSTR lpMsgBuf;
	if(!Res)
	{
		DWORD dw = GetLastError(); 
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
		OutputDebugStringA(lpMsgBuf);
	}			
	Assert(BytesRead == m_OutputSizes[0]);
	return 0;
}

//-------------------------------------------------------------------------------
void CJobReadFile::Initialize(const char* pName)
{
	strcpy_s(m_Name, 255, pName);	
	m_File =   CreateFileA(	pName,				   // file to open
							GENERIC_READ,          // open for reading
							FILE_SHARE_READ,       // share for reading
							NULL,                  // default security
							OPEN_EXISTING,         // existing file only
							FILE_ATTRIBUTE_NORMAL, // normal file
							NULL);                 // no attr. template	
	Assert(m_File != INVALID_HANDLE_VALUE);
#if 0			
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError(); 
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );
	}
#endif
	m_OutputSizes[0] = GetFileSize(m_File, NULL);
}

//-------------------------------------------------------------------------
bool CJobReadFile::Startup()
{
	m_pOutputs[0] = (unsigned char*)gTTyx->GetLoadingSystem().Alloc(m_OutputSizes[0], this);
	if(m_pOutputs[0])
	{
		gTTyx->GetJobSystem().AddJob(this);
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------
void CJobReadFile::Shutdown()
{
	if(m_File != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_File);
	}
	if(m_pOutputs[0])
	{
		gTTyx->GetLoadingSystem().Free(m_pOutputs[0], m_OutputSizes[0], this);
	}
	gTTyx->GetJobSystem().ReleaseJob(this);
}
#endif