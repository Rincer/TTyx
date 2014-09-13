#ifndef HEAPALLOCATOR_H_
#define HEAPALLOCATOR_H_

#include "LWMutex.h"
#include "Allocator.h"
//--------------------------------------------------------------------------------
// Heap allocator. 
// Allocates a large block of memory from either the OS or main heap. The 1st 4k of that memory are used for internal structures.
// The implementation is based on Doug Lea memory manager. Free block bins are grouped by size. Small block bins go from 0 bytes per block to 508 bytes per block in 4 byte increments.
// Large block bins start from 512 bytes per block and go up to 2^31 bytes per block where each successive block is twice the size of its predecessor. Each block contains header data, 
// which is immediately followed by user data. Header data contains user data size, and 32 bit offsets from the beginning of allocated memory block. m_PrevInMemory is the offset of the 
// block that precedes the current block in memory. m_PrevInBin and m_NextInBin are offsets of previous and next block in the free block bins. An offset of 0xFFFFFFFF indicates that there 
// is no previous or next block.
// When Alloc is called a search is performed to find the smallest size bin that has free blocks bigger than the required allocation size. The first block in the bin is used to do the allocation.
// If the block size if bigger than the required allocation size, it is split into 2 blocks. The 1st one matches the size exactly and is used for the allcation while the remainder is
// added to the free block pool.
// When Free is called a check is performed if the previous in memory or the next in memory blocks are free then they are merged into the block being freed and added back to the
// free block pool.
//--------------------------------------------------------------------------------
class CMemoryBlock;

class CHeapAllocator : public IAllocator
{
		static const unsigned int scAlignedAllocMask	= 0x40000000;		
		static const unsigned int scMaxSmallAllocation	= 512;
		static const unsigned int scLog2MaxSmallAllocation = 9;		
		static const unsigned int scGranularity = 4;
		static const unsigned int scNumSmallBins = scMaxSmallAllocation / scGranularity;	// max allocation of 512 bytes, smallest alloction size 4 bytes
		static const unsigned int scNumLargeBins = 22;										// (31 - 9) 512 = 2^9, max allocation size 2^31 

	public:
		
		CHeapAllocator(unsigned int Size, bool OSAlloc);

		virtual void* Alloc(unsigned int Size);
		virtual void* AlignedAlloc(unsigned int Size, unsigned int Alignment);
		virtual void Free(void* pMemory);
		virtual size_t Msize(void* pMemory);		
		virtual size_t AlignedMsize(void* pMemory);		
		virtual void Reset(void);
		
		bool debugPrint;											
		// this class can not have a destructor defined, because it is used in a singleton pattern to override default crt memory functions. 
		// when crt tries to allocate some memory for the 1st time during initialization it will call atexit() to add it to the list 
		// of dtors called on program exit however that list hasnt been allocated yet and the application will crash, for this reason it needs to be shut down manually.
		void ShutDown();
	
	private:
					
		class CGetFreeBlockResult
		{
			public:
				CMemoryBlock*	m_pFreeBlock;
				CMemoryBlock**	m_ppFirstInBin;
		};

		class CMemoryBlockBins
		{
			public:
				CMemoryBlock* m_pSmallBlocks[scNumSmallBins];
				CMemoryBlock* m_pLargeBlocks[scNumLargeBins];
		};
		
		void InternalFree(void* pMemory);
		void SearchSmallBlocks(unsigned int Size, CGetFreeBlockResult* pGetFreeBlockResult);
		void SearchLargeBlocks(unsigned int Size, CGetFreeBlockResult* pGetFreeBlockResult);			
		void FindFreeBlock(unsigned int Size, CGetFreeBlockResult* pGetFreeBlockResult);		
		void RemoveFromBin(CMemoryBlock* pBlock);
		void AddToBin(CMemoryBlock* pBlock);		
		unsigned int GetLargeBinIndex(unsigned int Size);
		
		virtual void VerifyConsistency();
		void PrintAllAllocations();
		
// None of the member variables here can have d'tors defined or c runtime will try to add them to the d'tor list called on program shut down when that list doesnt exist yet.		
// see note above
		CMemoryBlockBins*	m_pFreeBlockBins;
		unsigned char*		m_pMemoryStart;
		unsigned char*		m_pMemoryEnd;
		unsigned int		m_NumAllocations;	
		unsigned int		m_TotalAllocated;
		unsigned int		m_TotalSize;
		CLWMutex			m_LWMutex;				
		bool				m_OSAlloc;		
		friend class		CMemoryBlock;
};
#endif