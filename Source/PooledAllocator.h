#ifndef _POOLEDALLOCATOR_H_
#define _POOLEDALLOCATOR_H_

#include "Allocator.h"

class CPooledAllocator final : public IAllocator
{
	public:
		CPooledAllocator(unsigned int ElementSize, unsigned int NumElements, unsigned int Alignment, IAllocator* pAllocator);
		~CPooledAllocator();
   		virtual void* Alloc(unsigned int Size);
		virtual void* AlignedAlloc(unsigned int Size, unsigned int Alignment);
		virtual void Free(void* pMemory);
		virtual size_t Msize(void* pMemory);		
		virtual size_t AlignedMsize(void* pMemory);	
		virtual void Reset(void);
		virtual void VerifyConsistency();

	private:
		static const unsigned int scEndOfListOffset = 0x1; // valid offsets cant be odd, because at least 4 byte alignment is imposed
		unsigned int	m_ElementSize;
		unsigned int	m_NumElements;
		unsigned char*	m_pMemory;
		IAllocator*		m_pAllocator;
		unsigned int	m_FreeElementOffset;
};

#endif