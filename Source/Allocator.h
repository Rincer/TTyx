#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

class IAllocator
{
	public:
		IAllocator()
		{
			m_CurrentThreadId = GetCurrentThreadId();
		}
		virtual void* Alloc(unsigned int Size) = 0;
		virtual void* AlignedAlloc(unsigned int Size, unsigned int Alignment) = 0;
		virtual void Free(void* pMemory) = 0;
		virtual size_t Msize(void* pMemory) = 0;		
		virtual size_t AlignedMsize(void* pMemory) = 0;	
		virtual void Reset(void) = 0;
		virtual void VerifyConsistency() = 0;
	protected:
		DWORD	m_CurrentThreadId;
		
};


#endif
