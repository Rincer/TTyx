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

		void CreatePool(unsigned char* pMemory);

		class CLink
		{
			public:
				CLink* m_pNext;
		};

		class CMemoryBlockLink
		{
			public:
				unsigned char*		m_pMemory;
				CMemoryBlockLink*	m_pNext;
		};

		unsigned int		m_ElementSize;
		unsigned int		m_NumElements;
		unsigned int		m_Alignment;
		CMemoryBlockLink*	m_pMemoryBlock;
		IAllocator*			m_pAllocator;
		CLink*				m_pFreeElement;	 
};

#endif