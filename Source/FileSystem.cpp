#include "stdafx.h"

#include "FileSystem.h"

const char* CFileSystem::m_pRootFolder;

//-----------------------------------------------------------------------------
HANDLE CFileSystem::Open(const char* pName)
{
//	char FileName[256];
//	sprintf_s(FileName,"%s%s", m_pRootFolder, pName);
	HANDLE File =   CreateFileA(pName,				   // file to open
								GENERIC_READ,          // open for reading
								FILE_SHARE_READ,       // share for reading
								NULL,                  // default security
								OPEN_EXISTING,         // existing file only
								FILE_ATTRIBUTE_NORMAL, // normal file
								NULL);                 // no attr. template	
	Assert(File != INVALID_HANDLE_VALUE);
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
	return File;
}

//-----------------------------------------------------------------------------
void CFileSystem::Read(unsigned char* pData, unsigned int Size, HANDLE File)
{
	DWORD BytesRead; 			
	BOOL Res = ReadFile(File, pData, Size, &BytesRead, NULL);
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
	Assert(BytesRead == Size);
}

//-----------------------------------------------------------------------------
void CFileSystem::Close(HANDLE File)
{
	CloseHandle(File);
}

//-----------------------------------------------------------------------------
void CFileSystem::SetRootFolder(const char* pName)
{
	m_pRootFolder = pName;
}
	 