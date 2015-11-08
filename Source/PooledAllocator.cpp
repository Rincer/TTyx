#include "stdafx.h"
#include "Macros.h"
#include "Debugging.h"

#include "PooledAllocator.h"

//-----------------------------------------------------------------------------------------------------------------
CPooledAllocator::CPooledAllocator(unsigned int ElementSize, unsigned int NumElements, unsigned int Alignment, IAllocator* pAllocator) : m_NumElements(NumElements),
	m_Alignment(Alignment),
	m_pMemoryBlock(nullptr),
	m_pAllocator(pAllocator)
{
	m_Alignment = MAX(Alignment, 4); // at least 4
	m_ElementSize = MAX(ALIGN(ElementSize, m_Alignment), 8); // at least 8 to be able to store 64 bit ptrs
	unsigned char* pMemory = (unsigned char*)pAllocator->AlignedAlloc(m_ElementSize * m_NumElements + sizeof(CMemoryBlockLink), m_Alignment);		
	CreatePool(pMemory);
}

//-----------------------------------------------------------------------------------------------------------------
CPooledAllocator::~CPooledAllocator()
{
	while(m_pMemoryBlock)
	{
		CMemoryBlockLink MemoryBlockLink = *m_pMemoryBlock;	// create local copy
		m_pAllocator->Free(m_pMemoryBlock->m_pMemory);		// this will free the memory occupied by m_pMemoryBlock too, thats why we need a local copy
		m_pMemoryBlock = MemoryBlockLink.m_pNext;			// move to next allocated block
	}
}

//-----------------------------------------------------------------------------------------------------------------
void* CPooledAllocator::Alloc(unsigned int Size)
{
	Assert(Size <= m_ElementSize);
	if(m_pFreeElement == nullptr)	// grow the pool
	{
		unsigned char* pMemory = (unsigned char*)m_pAllocator->AlignedAlloc(m_ElementSize * m_NumElements + sizeof(CMemoryBlockLink), m_Alignment);		
		CreatePool(pMemory);
	}
	void* Res = m_pFreeElement;
	m_pFreeElement = m_pFreeElement->m_pNext; // advance
	return (void*)Res;
}

//-----------------------------------------------------------------------------------------------------------------
void CPooledAllocator::Free(void* pMemory)
{
	CLink* pFreeBlock = (CLink*)pMemory;
	pFreeBlock->m_pNext = m_pFreeElement; // link to the front of the list
	m_pFreeElement = pFreeBlock; // update the 1st free element offset to the start of the free block
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
void CPooledAllocator::CreatePool(unsigned char* pMemory)
{
	CMemoryBlockLink* pNewMemoryBlock = (CMemoryBlockLink*)&pMemory[m_ElementSize * m_NumElements];	// stored at the end
	pNewMemoryBlock->m_pMemory = pMemory;
	pNewMemoryBlock->m_pNext = m_pMemoryBlock;
	m_pMemoryBlock = pNewMemoryBlock;

	m_pFreeElement = (CLink*)pMemory;
	CLink* pCurrElement = m_pFreeElement;
	for(unsigned int ElementIndex = 1; ElementIndex < m_NumElements; ElementIndex++)
	{
		pCurrElement->m_pNext = (CLink*)&pMemory[ElementIndex * m_ElementSize];
		pCurrElement = pCurrElement->m_pNext;		
	}
	pCurrElement->m_pNext = nullptr;
}

//-----------------------------------------------------------------------------------------------------------------
void CPooledAllocator::VerifyConsistency()
{
	unsigned int TotalFree = 0;
	CLink* pFree = m_pFreeElement;
	while(pFree != nullptr)
	{
		DebugPrintf("0x%x ", pFree);
		TotalFree++;
		pFree = pFree->m_pNext;
	}
	DebugPrintf("\n Total free %d\n", TotalFree);
}