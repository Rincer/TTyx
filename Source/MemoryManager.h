#ifndef _MEMORYMANAGER_H_
#define _MEMORYMANAGER_H_

#include "HeapAllocator.h"
#include "LWMutex.h"

// Use this instead of the C++ array version of placement new since the C++ version can add an unspecified overhead
template <class DataType>
DataType *PlacementNew(void* pMem, unsigned int NumElements)
{
	unsigned char* pElement = (unsigned char*)pMem;
	for(unsigned int ElementIndex = 0; ElementIndex < NumElements; ElementIndex++)
	{
		new(pElement) DataType;
		pElement += sizeof(DataType);
	}
	return (DataType*)pMem;
}

// Overrides all the C/C++ memory management functions and creates a main heap to manage memory while the application is running
class CMemoryManager
{
	static const unsigned int scDefaultSize = (200 * 1024 * 1024);	
	
	public:	
		inline static CHeapAllocator& GetAllocator()
		{
			static CHeapAllocator s_MainHeap(scDefaultSize, true); // allocate from the OS
			// OverrideMSVCRT(); // not doing this until can figure out a way to reliable override all memory allocations
			return s_MainHeap;
		}	
		
		static void ShutDown();		
	private:
		static void OverrideMSVCRT();
};

// Utility class used to track and cap memory allocation for a system
class CMemoryCapper
{
	public:
		CMemoryCapper(const unsigned int Capacity);
		void* Alloc(unsigned int Size);
		void Free(const void* pMemory, unsigned int Size);

	private:
		CLWMutex m_Mutex;
		unsigned int m_Capacity;
		unsigned int m_Size;
};

#endif
