#include "stdafx.h"
#include "Macros.h"

#include "StackAllocator.h"

//--------------------------------------------------------------------------------
CStackAllocator::CStackAllocator() :	m_Size(0),
										m_pMemory(NULL),
										m_Offset(0),
										m_Placement(false)
{
}

//--------------------------------------------------------------------------------
CStackAllocator::CStackAllocator(unsigned int Size, unsigned int Alignment) : IAllocator()
{
	m_Size = Size;
	m_pMemory = (unsigned char*)_aligned_malloc(Size, Alignment);
	m_Offset = 0;
	m_Placement = false;
}

//--------------------------------------------------------------------------------
// Same as above but with memory passed in
CStackAllocator::CStackAllocator(unsigned char* pMemory, unsigned int Size, unsigned int Alignment)
{
	m_pMemory = (unsigned char*)(POW2ALIGN((unsigned long long)pMemory, Alignment));
	m_Size = Size - (m_pMemory - pMemory);
	m_Offset = 0;
	m_Placement = true;	
}

//--------------------------------------------------------------------------------
CStackAllocator::~CStackAllocator()
{
	if(!m_Placement)
	{
		_aligned_free(m_pMemory);
	}
}

//--------------------------------------------------------------------------------
void* CStackAllocator::Alloc(unsigned int Size)
{
	//Assert(m_CurrentThreadId == GetCurrentThreadId());		// no cross thread memory allocations, comment out for now
	unsigned int UnalignedOffset = m_Offset;
	unsigned int AlignedOffset = POW2ALIGN(UnalignedOffset, 4);	// at least 4 byte alignment
	Assert((AlignedOffset + Size) < m_Size);					// check if out of memory
	m_Offset = AlignedOffset + Size;
	return (void*)&m_pMemory[AlignedOffset];			
}

//--------------------------------------------------------------------------------
void* CStackAllocator::AlignedAlloc(unsigned int Size, unsigned int Alignment)
{
	//Assert(m_CurrentThreadId == GetCurrentThreadId());			// no cross thread memory allocations, comment out for now
	unsigned int UnalignedOffset = m_Offset;
	unsigned int AlignedOffset = ALIGN(UnalignedOffset, Alignment);	
	Assert((AlignedOffset + Size) <= m_Size);						// check if out of memory
	m_Offset = AlignedOffset + Size;
	return (void*)&m_pMemory[AlignedOffset];				
}

//--------------------------------------------------------------------------------
void CStackAllocator::Free(void* pMemory)
{
	// C4100
	pMemory;
	// Do nothing
}

//--------------------------------------------------------------------------------
size_t CStackAllocator::Msize(void* pMemory)
{
	// C4100
	pMemory;
	__debugbreak();
	return 0;
}

//--------------------------------------------------------------------------------
size_t CStackAllocator::AlignedMsize(void* pMemory)
{
	// C4100
	pMemory;
	__debugbreak();
	return 0;	
}

//--------------------------------------------------------------------------------
void CStackAllocator::Reset(void)
{
	m_Offset = 0;
}

//--------------------------------------------------------------------------------
unsigned int CStackAllocator::GetOffset()
{
	return m_Offset;
}

//--------------------------------------------------------------------------------
void CStackAllocator::SetOffset(unsigned int Offset)
{
	m_Offset = Offset;
}