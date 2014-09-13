#include "stdafx.h"
#include "Allocator.h"
#include "MemoryManager.h"
#include "Macros.h"

#include "SegmentedBuffer.h"

//------------------------------------------------------------------------------------------------------------------
void CSegmentedBuffer::Initialize(unsigned int NumSegments, unsigned int SegmentSize, IAllocator* pAllocator)
{
	Assert((SegmentSize & 0xF) == 0); // Need to be 16 byte aligned for any XMVECTOR data
	m_pAllocator = pAllocator;
	m_pBufferMemory = (unsigned char*)m_pAllocator->AlignedAlloc(POW2ALIGN(NumSegments * SegmentSize, 16) + NumSegments * sizeof(CRWBuffer), 16);
	unsigned char* pDataMemory = m_pBufferMemory;
	unsigned char* pRWBufferMemory = &m_pBufferMemory[NumSegments * SegmentSize];
	for (unsigned int SegmentIndex = 0; SegmentIndex < NumSegments; SegmentIndex++)
	{
		CRWBuffer* pRWBuffer = new(pRWBufferMemory)CRWBuffer(pDataMemory, SegmentSize);
		m_WriteList.AddFront(pRWBuffer);
		pDataMemory += SegmentSize;
		pRWBufferMemory += sizeof(CRWBuffer);
	}
}

//------------------------------------------------------------------------------------------------------------------
CSegmentedBuffer::CSegmentedBuffer() :	m_pAllocator(NULL),
										m_pBufferMemory(NULL)
{
}


//------------------------------------------------------------------------------------------------------------------
CSegmentedBuffer::~CSegmentedBuffer()
{
	m_pAllocator->Free(m_pBufferMemory);
}

//------------------------------------------------------------------------------------------------------------------
CRWBuffer* CSegmentedBuffer::GetRead()
{
	m_Mutex.Acquire(); // Thread safe
	CList<CRWBuffer>::CIterator* pIterator = m_ReadList.GetFirst();
	CRWBuffer* pRWBuffer = NULL;
	if (pIterator)
	{
		pRWBuffer = pIterator->GetData();
		m_ReadList.Remove(pRWBuffer);
	}
	m_Mutex.Release();
	return pRWBuffer;
}

//------------------------------------------------------------------------------------------------------------------
CRWBuffer* CSegmentedBuffer::GetWrite()
{
	m_Mutex.Acquire(); // Thread safe
	CList<CRWBuffer>::CIterator* pIterator = m_WriteList.GetFirst();
	CRWBuffer* pRWBuffer = NULL;
	if (pIterator)
	{
		pRWBuffer = pIterator->GetData();
		m_WriteList.Remove(pRWBuffer);
	}
	m_Mutex.Release();
	return pRWBuffer;
}

//------------------------------------------------------------------------------------------------------------------
void CSegmentedBuffer::PutRead(CRWBuffer* pBuffer)
{
	Assert(pBuffer->GetState() == CRWBuffer::eReading);
	m_Mutex.Acquire(); // Thread safe
	m_ReadList.AddBack(pBuffer);
	m_Mutex.Release();
}

//------------------------------------------------------------------------------------------------------------------
void CSegmentedBuffer::PutWrite(CRWBuffer* pBuffer)
{
	Assert(pBuffer->GetState() == CRWBuffer::eWriting);
	m_Mutex.Acquire(); // Thread safe
	m_WriteList.AddFront(pBuffer);
	m_Mutex.Release();
}


