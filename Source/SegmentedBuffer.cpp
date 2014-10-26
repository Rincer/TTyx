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
		m_WriteList.AddFront(0, pRWBuffer);
		pDataMemory += SegmentSize;
		pRWBufferMemory += sizeof(CRWBuffer);
	}
}

//------------------------------------------------------------------------------------------------------------------
CSegmentedBuffer::CSegmentedBuffer() :	m_pAllocator(NULL),
										m_pBufferMemory(NULL),
										m_WriteList(&CMemoryManager::GetAllocator()),
										m_ReadList(&CMemoryManager::GetAllocator())	// use global allocator for lists internal structs

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
	auto pIterator = m_ReadList.GetFirst(0);
	CRWBuffer* pRWBuffer = NULL;
	if (pIterator)
	{
		pRWBuffer = pIterator->GetValue();
		m_ReadList.Remove(0, pIterator);
	}
	m_Mutex.Release();
	return pRWBuffer;
}

//------------------------------------------------------------------------------------------------------------------
CRWBuffer* CSegmentedBuffer::GetWrite()
{
	m_Mutex.Acquire(); // Thread safe
	auto pIterator = m_WriteList.GetFirst(0);
	CRWBuffer* pRWBuffer = NULL;
	if (pIterator)
	{
		pRWBuffer = pIterator->GetValue();
		m_WriteList.Remove(0, pIterator);
	}
	m_Mutex.Release();
	return pRWBuffer;
}

//------------------------------------------------------------------------------------------------------------------
void CSegmentedBuffer::PutRead(CRWBuffer* pBuffer)
{
	Assert(pBuffer->GetState() == CRWBuffer::eReading);
	m_Mutex.Acquire(); // Thread safe
	m_ReadList.AddBack(0, pBuffer);
	m_Mutex.Release();
}

//------------------------------------------------------------------------------------------------------------------
void CSegmentedBuffer::PutWrite(CRWBuffer* pBuffer)
{
	Assert(pBuffer->GetState() == CRWBuffer::eWriting);
	m_Mutex.Acquire(); // Thread safe
	m_WriteList.AddFront(0, pBuffer);
	m_Mutex.Release();
}


