#include "stdafx.h"
#include "Macros.h"
#include "Debugging.h"

#include "PooledAllocator.h"

//-----------------------------------------------------------------------------------------------------------------
CPooledAllocator::CPooledAllocator(unsigned int ElementSize, unsigned int NumElements, unsigned int Alignment, IAllocator* pAllocator) : m_NumElements(NumElements),
m_pAllocator(pAllocator)
{
	Alignment = MAX(Alignment, 4); // at least 4 
	m_ElementSize = ALIGN(ElementSize, Alignment);	
	m_pMemory = (unsigned char*)pAllocator->AlignedAlloc(m_ElementSize * NumElements, Alignment);	
	for(unsigned int ElementIndex = 0; ElementIndex < m_NumElements - 1; ElementIndex++)
	{
		unsigned int* NextElementOffset = (unsigned int*)&m_pMemory[ElementIndex * m_ElementSize];
		*NextElementOffset = (ElementIndex + 1) * m_ElementSize; // point to the next element
	}
	*(unsigned int*)&m_pMemory[(m_NumElements - 1) * m_ElementSize]	= scEndOfListOffset; //	list terminator
	m_FreeElementOffset = 0;
}

//-----------------------------------------------------------------------------------------------------------------
CPooledAllocator::~CPooledAllocator()
{
	m_pAllocator->Free(m_pMemory);
}

//-----------------------------------------------------------------------------------------------------------------
void* CPooledAllocator::Alloc(unsigned int Size)
{
	Assert(Size <= m_ElementSize);
	if(m_FreeElementOffset == scEndOfListOffset)
	{
		return nullptr;
	}
	unsigned int* Res = (unsigned int*)&m_pMemory[m_FreeElementOffset];
	m_FreeElementOffset	= *Res; // Advance to next free element
	return (void*)Res;
}

//-----------------------------------------------------------------------------------------------------------------
void CPooledAllocator::Free(void* pMemory)
{
	unsigned int* FreeBlock = (unsigned int*)pMemory;
	*FreeBlock = m_FreeElementOffset; // link to the front of the list
	m_FreeElementOffset = (unsigned char*)pMemory - m_pMemory; // update the 1st free element offset to the start of the free block
}

//-----------------------------------------------------------------------------------------------------------------
void* CPooledAllocator::AlignedAlloc(unsigned int /*Size*/, unsigned int /*Alignment*/)
{
	Assert(0);
	return nullptr;
}

//-----------------------------------------------------------------------------------------------------------------
size_t CPooledAllocator::Msize(void* /*pMemory*/)	
{
	Assert(0);
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------
size_t CPooledAllocator::AlignedMsize(void* /*pMemory*/)	
{
	Assert(0);
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------
void CPooledAllocator::Reset(void)
{
	Assert(0);
}

//-----------------------------------------------------------------------------------------------------------------
void CPooledAllocator::VerifyConsistency()
{
	unsigned int TotalFree = 0;
	unsigned int FreeOffset = m_FreeElementOffset;
	while(FreeOffset != scEndOfListOffset)
	{
		DebugPrintf("%d ", FreeOffset);
		TotalFree++;
		FreeOffset = *(unsigned int*)&m_pMemory[FreeOffset];
	}
	DebugPrintf("\n Total free %d\n", TotalFree);
}