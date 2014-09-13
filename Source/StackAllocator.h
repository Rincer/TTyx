#ifndef STACKALLOCATOR_H_
#define STACKALLOCATOR_H_

#include "Allocator.h"
//--------------------------------------------------------------------------------
// Stack allocator. Designed to be used by systems where no memory resources are
// freed while the system is running. All memory is given back once the systems 
// classes go out of scope. 
//--------------------------------------------------------------------------------
class CStackAllocator : public IAllocator
{
	public:
		CStackAllocator();
		CStackAllocator(unsigned int Size, unsigned int Alignment = 16);
		CStackAllocator(unsigned char* pMemory, unsigned int Size, unsigned int Alignment = 16);
		~CStackAllocator();
		virtual void* Alloc(unsigned int Size);
		virtual void* AlignedAlloc(unsigned int Size, unsigned int Alignment);
		virtual void Free(void* pMemory);
		virtual size_t Msize(void* pMemory);		
		virtual size_t AlignedMsize(void* pMemory);	
		virtual void Reset(void);
		virtual void VerifyConsistency() {}	
		unsigned int GetOffset();
		void SetOffset(unsigned int Offset);
		
		
	private:
		unsigned char*	m_pMemory;		
		unsigned int	m_Offset;
		unsigned int	m_Size;		
		bool			m_Placement;
};
#endif