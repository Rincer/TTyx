#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

class CFileSystem
{
	public:
		static HANDLE Open(const char* pName);
		static void Read(unsigned char* pData, unsigned int Size, HANDLE File);
		static void Close(HANDLE File);
		static void SetRootFolder(const char* pName);
	private:
		static const char* m_pRootFolder;
};

#endif