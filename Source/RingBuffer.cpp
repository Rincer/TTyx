#include "stdafx.h"

#include "RingBuffer.h"

//----------------------------------------------------------------------------------------------
CRingBuffer::CRingBuffer()
{
	m_Size = 0;
	m_pBuffer = NULL;
	m_ReadOffset = 0;
	m_WriteOffset = 0;		
	m_pRingBufferCallback = 0;	
}

//----------------------------------------------------------------------------------------------
CRingBuffer::CRingBuffer(unsigned int Size, CRingBufferCallback* pRingBufferCallback)
{
	m_Size = Size;
	m_pBuffer = new unsigned char[Size];
	m_ReadOffset = 0;
	m_WriteOffset = 0;	
	m_pRingBufferCallback = pRingBufferCallback;
}

//----------------------------------------------------------------------------------------------
CRingBuffer::CRingBuffer(unsigned char* pBuffer, unsigned int Size, CRingBufferCallback* pRingBufferCallback)
{
	Initialize(pBuffer, Size, pRingBufferCallback);
}

//----------------------------------------------------------------------------------------------
void CRingBuffer::Initialize(unsigned char* pBuffer, unsigned int Size, CRingBufferCallback* pRingBufferCallback)
{
	m_Size = Size;
	m_pBuffer = pBuffer;
	m_ReadOffset = 0;
	m_WriteOffset = 0;		
	m_pRingBufferCallback = pRingBufferCallback;	
}

//----------------------------------------------------------------------------------------------
CRingBuffer::~CRingBuffer()
{
	delete[] m_pBuffer;
}

//----------------------------------------------------------------------------------------------		
void CRingBuffer::TryReserve(unsigned char** ppData, unsigned int Size)
{
	*ppData = NULL;
	unsigned int ReadOffset = m_ReadOffset;	// get a copy of m_ReadOffset and use it internally, do not use m_ReadOffset more than one, since Read can change it
	if(m_Size - (m_WriteOffset - ReadOffset) >= Size)
	{
		unsigned int SpaceBeforeWrap = m_Size - (m_WriteOffset % m_Size);
		if(Size > SpaceBeforeWrap) // not enough space till the end of the buffer, need to wrap 
		{
			m_pRingBufferCallback->Execute(this, SpaceBeforeWrap);
		}
		else // success
		{
			*ppData = &m_pBuffer[m_WriteOffset % m_Size];
		}
	}
}


//----------------------------------------------------------------------------------------------		
void CRingBuffer::BeginWrite(unsigned char** ppData, unsigned int Size)
{
	Assert(Size < m_Size / 2); // writes greater than half buffer size can cause a lock condition
	m_WriteMutex.Acquire();
	m_ReservedSize = Size;
	do
	{
		TryReserve(ppData, Size);
		if(!*ppData)
		{
			Sleep(0);
		}
	}while (*ppData == NULL);	
}

//----------------------------------------------------------------------------------------------		
void CRingBuffer::EndWrite(unsigned int Size)
{
	Assert(m_ReservedSize == Size);
	m_WriteOffset += Size;
	m_WriteMutex.Release();
}

//----------------------------------------------------------------------------------------------
void CRingBuffer::ReserveUnsafe(unsigned char** ppData, unsigned int Size)
{		
	*ppData = &m_pBuffer[m_WriteOffset % m_Size];
	m_WriteOffset += Size;			
}
		
//----------------------------------------------------------------------------------------------				
void CRingBuffer::BeginRead(unsigned char** ppData, unsigned int BytesToRead)
{
	m_ReadMutex.Acquire();
	while(m_WriteOffset - m_ReadOffset < BytesToRead) // check if there is enough data
	{
		Sleep(0);
	}
	*ppData = &m_pBuffer[m_ReadOffset % m_Size];
}

//----------------------------------------------------------------------------------------------						
void CRingBuffer::EndRead(unsigned int BytesRead)
{
	m_ReadOffset += BytesRead;
	m_ReadMutex.Release();
}
						
//----------------------------------------------------------------------------------------------
bool CRingBuffer::TryWrite(unsigned char* pData, unsigned int Size)
{
	m_WriteMutex.Acquire();
	unsigned int ReadOffset = m_ReadOffset;	// get a copy of m_ReadOffset and use it internally, do not use m_ReadOffset more than one, since Read can change it
	bool Res = false;
	if(m_Size - (m_WriteOffset - ReadOffset) >= Size)
	{
		unsigned int SpaceBeforeWrap = m_Size - (m_WriteOffset % m_Size);
		if(Size > SpaceBeforeWrap) // not enough space till the end of the buffer, need to wrap 
		{
			memcpy(&m_pBuffer[m_WriteOffset % m_Size], pData, SpaceBeforeWrap);
			m_WriteOffset += SpaceBeforeWrap;
			memcpy(&m_pBuffer[m_WriteOffset % m_Size], &pData[SpaceBeforeWrap], Size - SpaceBeforeWrap);
			m_WriteOffset += (Size - SpaceBeforeWrap);
		}
		else // no wrap is required, just a single copy
		{
			memcpy(&m_pBuffer[m_WriteOffset % m_Size], pData, Size);
			m_WriteOffset += Size;
		}
		Res = true;
	}
	m_WriteMutex.Release();
	return Res;
}

//----------------------------------------------------------------------------------------------
void CRingBuffer::Write(unsigned char* pData, unsigned int Size)
{
	Assert(Size < m_Size / 2); // writes greater than half buffer size can cause a lock condition
	bool Res = false;
	while(!Res)
	{
		Res = TryWrite(pData, Size);
		if(!Res)
		{
			Sleep(0);
		}
	}
}

//----------------------------------------------------------------------------------------------
bool CRingBuffer::TryRead(unsigned char* pData, unsigned int Size)
{
	m_ReadMutex.Acquire();
	unsigned int WriteOffset = m_WriteOffset; // get a copy of m_WriteOffset and use it internally, do not use m_WriteOffset more than one, since Write can change it
	bool Res = false;
	if(WriteOffset - m_ReadOffset >= Size)
	{
		unsigned int SpaceBeforeWrap = m_Size - (m_ReadOffset % m_Size);
		if(Size > SpaceBeforeWrap) // not enough space till the end of the buffer, need to wrap 
		{
			memcpy(pData, &m_pBuffer[m_ReadOffset % m_Size], SpaceBeforeWrap);
			m_ReadOffset += SpaceBeforeWrap;
			memcpy(&pData[SpaceBeforeWrap], &m_pBuffer[m_ReadOffset % m_Size], Size - SpaceBeforeWrap);
			m_ReadOffset += (Size - SpaceBeforeWrap);
		}
		else // no wrap is required, just a single copy
		{
			memcpy(pData, &m_pBuffer[m_ReadOffset % m_Size], Size);
			m_ReadOffset += Size;
		}
		Res = true;
	}
	m_ReadMutex.Release();	
	return Res;
}

//----------------------------------------------------------------------------------------------
void CRingBuffer::Read(unsigned char* pData, unsigned int Size)
{
	bool Res = false;
	while(!Res)
	{
		Res = TryRead(pData, Size);
		if(!Res)
		{
			Sleep(0);
		}
	}
}

//----------------------------------------------------------------------------------------------
unsigned int CRingBuffer::UnreadBytes()
{
	return m_WriteOffset - m_ReadOffset;
}
