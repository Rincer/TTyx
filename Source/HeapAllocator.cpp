#include "stdafx.h"
#include "Macros.h"
#include "Math.h"
#include "HeapAllocator.h"

#define DEFAULT_ALIGNMENT (4096)


//------------------------------------------------------------------------------------
// CMemoryBlock implementation start 
//------------------------------------------------------------------------------------
class CMemoryBlock
{
	static const unsigned int scAllocateStatusMask	= 0x80000000;
	static const unsigned int scDefaultBlockAlignment = 4;	
	
	public:
		//------------------------------------------------------------------------------------
		CMemoryBlock(unsigned int Size)
		{
			m_Size = Size - sizeof(CMemoryBlock); // reserve space for embedded CMemoryBlock data
			m_PrevInMemory = 0xFFFFFFFF;
			m_NextInBin = 0xFFFFFFFF;
			m_PrevInBin = 0xFFFFFFFF;
		}	
		
		//------------------------------------------------------------------------------------
		CMemoryBlock* GetPrevInMemory(CHeapAllocator* pCtxt)
		{
			if(m_PrevInMemory == 0xFFFFFFFF)
				return NULL;
			CMemoryBlock* pPrevInMemory = (CMemoryBlock*)&(pCtxt->m_pMemoryStart[m_PrevInMemory]);
			return pPrevInMemory;
		}	
		
		//------------------------------------------------------------------------------------
		CMemoryBlock* GetNextInMemory(CHeapAllocator* pCtxt)
		{
			unsigned int ThisOffset = (unsigned int)((unsigned char*)this - pCtxt->m_pMemoryStart);
			CMemoryBlock* pNextInMemory = (CMemoryBlock*)&(pCtxt->m_pMemoryStart[ThisOffset + GetSize() + sizeof(CMemoryBlock)]);
			return (unsigned char*)pNextInMemory == pCtxt->m_pMemoryEnd ? NULL : pNextInMemory; // if we are the last block, return NULL
		}
		
		//------------------------------------------------------------------------------------
		void SetNextInBin(CHeapAllocator* pCtxt, CMemoryBlock* pBlock)
		{
			m_NextInBin = (unsigned int)((unsigned char*)pBlock - pCtxt->m_pMemoryStart);
		}
		
		//------------------------------------------------------------------------------------
		void SetPrevInBin(CHeapAllocator* pCtxt, CMemoryBlock* pBlock)
		{
			m_PrevInBin = (unsigned int)((unsigned char*)pBlock - pCtxt->m_pMemoryStart);
		}
		
		//------------------------------------------------------------------------------------
		CMemoryBlock* GetPrevInBin(CHeapAllocator* pCtxt)							
		{
			if(m_PrevInBin == 0xFFFFFFFF)
				return NULL;
			CMemoryBlock* pPrevInBin = (CMemoryBlock*)&(pCtxt->m_pMemoryStart[m_PrevInBin]);
			return pPrevInBin;
		}
		
		//------------------------------------------------------------------------------------
		CMemoryBlock* GetNextInBin(CHeapAllocator* pCtxt)
		{
			if(m_NextInBin == 0xFFFFFFFF)
				return NULL;
			CMemoryBlock* pNextInBin = (CMemoryBlock*)&(pCtxt->m_pMemoryStart[m_NextInBin]);
			return pNextInBin;
		}
								
		//------------------------------------------------------------------------------------								
		unsigned int GetSize()		// msb is 1 when allocated 0 when free
		{
			return m_Size & (~scAllocateStatusMask);
		}
				
		//------------------------------------------------------------------------------------				
		bool IsFree()
		{
			return m_Size & (scAllocateStatusMask) ? false : true;
		}
		
		//------------------------------------------------------------------------------------
		void Free()
		{
			m_Size &= (~scAllocateStatusMask);
		}
		
		//------------------------------------------------------------------------------------
		void* Allocate(CHeapAllocator* pCtxt, unsigned int Size)
		{		
			// There need to be 16 extra bytes to store CMemoryBlock header data for us to be able to split off the extra
			// memory into its own block
			if((GetSize() > sizeof(CMemoryBlock)) && (Size < (GetSize() - sizeof(CMemoryBlock))))
			{		
				Split(pCtxt, Size);
			}
			m_Size |= scAllocateStatusMask;			
			return (void*)(((unsigned char*)this) + sizeof(CMemoryBlock));
		}
		
		//------------------------------------------------------------------------------------
		void RemoveFromBin(CHeapAllocator* pCtxt, CMemoryBlock** ppFirstBlock)
		{
			CMemoryBlock* pPrevInBin = GetPrevInBin(pCtxt);
			CMemoryBlock* pNextInBin = GetNextInBin(pCtxt);	
			if(pPrevInBin == NULL)	// 1st block in bin
			{
				*ppFirstBlock = pNextInBin;
				if(pNextInBin)
					pNextInBin->m_PrevInBin = 0xFFFFFFFF;
			}
			else
			{
				pPrevInBin->m_NextInBin = m_NextInBin;
				if(pNextInBin)
					pNextInBin->m_PrevInBin = m_PrevInBin;
			}
			m_NextInBin = 0xFFFFFFFF;
			m_PrevInBin = 0xFFFFFFFF;
		}
		
		//------------------------------------------------------------------------------------
		void AddToBin(CHeapAllocator* pCtxt, CMemoryBlock** ppFirstBlock)
		{
			if(*ppFirstBlock != NULL)	// bin is not empty
			{
				SetNextInBin(pCtxt, *ppFirstBlock);
				(*ppFirstBlock)->SetPrevInBin(pCtxt, this);				
			}
			*ppFirstBlock = this;
		}
				
		//------------------------------------------------------------------------------------				
		void Split(CHeapAllocator* pCtxt, unsigned int Size) // split off a memory block from this block so that this block is reduced to the specified size
		{
			unsigned int AllocationSize = POW2ALIGN((Size + sizeof(CMemoryBlock)), scDefaultBlockAlignment);
			unsigned char* pNewBlockLocation = (unsigned char*)this + AllocationSize;
			unsigned char* pEndOfCurrentBlock = ((unsigned char*)this + m_Size + sizeof(CMemoryBlock));	// this block is free so m_Size msb will not be set, and it can be used in calculations
			unsigned int NewBlockSize = (unsigned int)(pEndOfCurrentBlock - pNewBlockLocation);		
			m_Size -= NewBlockSize;		
			CMemoryBlock* pNewBlock = new ((void*)pNewBlockLocation) CMemoryBlock(NewBlockSize);	
			pNewBlock->m_PrevInMemory = (unsigned int)((unsigned char*)this - pCtxt->m_pMemoryStart);	
			CMemoryBlock* pNextInMemory = pNewBlock->GetNextInMemory(pCtxt);
			if(pNextInMemory)
			{
				pNextInMemory->m_PrevInMemory = (unsigned int)((unsigned char*)pNewBlock - pCtxt->m_pMemoryStart);
			}
			pCtxt->AddToBin(pNewBlock);
		}
		
		//------------------------------------------------------------------------------------		
		void Merge(CHeapAllocator* pCtxt, CMemoryBlock* pBlock)
		{
			m_Size += pBlock->GetSize() + sizeof(CMemoryBlock);
			CMemoryBlock* pNextInMemory = pBlock->GetNextInMemory(pCtxt);		
			if(pNextInMemory)
			{
				pNextInMemory->m_PrevInMemory = (unsigned int)((unsigned char*)this - pCtxt->m_pMemoryStart);	
			}
		}				
		unsigned int	m_PrevInMemory;	// store offsets rather than ptrs, for platform compatibility 	
		unsigned int	m_PrevInBin;								
		unsigned int	m_NextInBin;				
		unsigned int	m_Size;				
};
//------------------------------------------------------------------------------------
// CMemoryBlock implementation end
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// CHeapAllocator implementation start
//------------------------------------------------------------------------------------
CHeapAllocator::CHeapAllocator(unsigned int Size, bool OSAlloc) : IAllocator()
{
	if(OSAlloc)
	{	
		m_pMemoryStart = (unsigned char*)VirtualAlloc(NULL, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	}
	else
	{
		m_pMemoryStart = (unsigned char*)_aligned_malloc(Size, DEFAULT_ALIGNMENT);
	}			
				
	m_pMemoryEnd = m_pMemoryStart + Size;
	m_pFreeBlockBins = (CMemoryBlockBins*)m_pMemoryStart; // embedded bin data
	for(int i = 0; i < scNumSmallBins; i++)
	{
		m_pFreeBlockBins->m_pSmallBlocks[i] = NULL;
	}
	for(int i = 0; i < scNumLargeBins; i++)
	{
		m_pFreeBlockBins->m_pLargeBlocks[i] = NULL;
	}
	unsigned int BinDataSize = POW2ALIGN(sizeof(CMemoryBlockBins), DEFAULT_ALIGNMENT);
	m_pMemoryStart += BinDataSize;
	Size -= BinDataSize;	
	m_NumAllocations = 0;

	CMemoryBlock** ppFirstFreeBlock;
	if(Size >= scMaxSmallAllocation)
	{
		unsigned int Index = GetLargeBinIndex(Size);
		ppFirstFreeBlock = &m_pFreeBlockBins->m_pLargeBlocks[Index];	
	}	
	else
	{
		ppFirstFreeBlock = &m_pFreeBlockBins->m_pSmallBlocks[Size / scGranularity];
	}
	(*ppFirstFreeBlock) = new (m_pMemoryStart) CMemoryBlock(Size);
	m_OSAlloc = OSAlloc;
	debugPrint = false;
	m_TotalAllocated = 0;
	m_TotalSize = Size;
}

//------------------------------------------------------------------------------------
void CHeapAllocator::ShutDown()
{
	m_LWMutex.Acquire();
	VirtualFree(m_pMemoryStart, 0,  MEM_RELEASE);
	m_LWMutex.Release();
}

//------------------------------------------------------------------------------------
void CHeapAllocator::FindFreeBlock(unsigned int Size, CGetFreeBlockResult* pGetFreeBlockResult)
{
	if(Size < scMaxSmallAllocation)
	{
		SearchSmallBlocks(Size, pGetFreeBlockResult);		
		if(!pGetFreeBlockResult->m_pFreeBlock)
			SearchLargeBlocks(Size, pGetFreeBlockResult);
		return;			
	}	
	SearchLargeBlocks(Size, pGetFreeBlockResult);	
}

//------------------------------------------------------------------------------------
void CHeapAllocator::SearchSmallBlocks(unsigned int Size, CGetFreeBlockResult* pGetFreeBlockResult)
{
	pGetFreeBlockResult->m_pFreeBlock = NULL;
	CMemoryBlock* pFreeBlock = pGetFreeBlockResult->m_pFreeBlock;	
	int SmallBinIndex = Size / scGranularity;
	for(int i = SmallBinIndex; i < scNumSmallBins; i++)
	{
		for(pFreeBlock = m_pFreeBlockBins->m_pSmallBlocks[i]; pFreeBlock != NULL; pFreeBlock = pFreeBlock->GetNextInBin(this))
		{
			if(pFreeBlock->GetSize() >= Size)
			{
				pGetFreeBlockResult->m_ppFirstInBin = &m_pFreeBlockBins->m_pSmallBlocks[i];			
				break;
			}
		}				
		if(pFreeBlock)
		{
			break;
		}			
	}
}

//------------------------------------------------------------------------------------
void CHeapAllocator::SearchLargeBlocks(unsigned int Size, CGetFreeBlockResult* pGetFreeBlockResult)
{	
	pGetFreeBlockResult->m_pFreeBlock = NULL;
	CMemoryBlock* pFreeBlock = pGetFreeBlockResult->m_pFreeBlock; // for ease of reading
	unsigned int LargeBinIndex = GetLargeBinIndex(Size);;	
	for(int i = LargeBinIndex; i < scNumLargeBins; i++)
	{
		for(pFreeBlock = m_pFreeBlockBins->m_pLargeBlocks[i]; pFreeBlock != NULL; pFreeBlock = pFreeBlock->GetNextInBin(this))
		{
			if(pFreeBlock->GetSize() >= Size)
			{
				pGetFreeBlockResult->m_ppFirstInBin = &m_pFreeBlockBins->m_pLargeBlocks[i];
				break;
			}
		}			
		if(pFreeBlock)
		{
			break;
		}					
	}		
	pGetFreeBlockResult->m_pFreeBlock = pFreeBlock; // store the result
}

//------------------------------------------------------------------------------------
void CHeapAllocator::AddToBin(CMemoryBlock* pBlock)
{
	unsigned int Size = pBlock->GetSize();
	if(Size < scMaxSmallAllocation)	
	{
		unsigned int SmallBinIndex = Size / scGranularity;	
		pBlock->AddToBin(this, &m_pFreeBlockBins->m_pSmallBlocks[SmallBinIndex]);			
	}
	else
	{
		unsigned int LargeBinIndex = GetLargeBinIndex(Size);	
		pBlock->AddToBin(this, &m_pFreeBlockBins->m_pLargeBlocks[LargeBinIndex]);
	}
}

// IAllocator overloads START

//------------------------------------------------------------------------------------
void* CHeapAllocator::Alloc(unsigned int Size)
{	
	if(Size == 0)
		__debugbreak();	
	m_LWMutex.Acquire();// mt safe
	CGetFreeBlockResult GetFreeBlockResult;
	FindFreeBlock(Size, &GetFreeBlockResult);
	void* Res = NULL;	
	if(GetFreeBlockResult.m_pFreeBlock)
	{
		Res = GetFreeBlockResult.m_pFreeBlock->Allocate(this, Size);	
		GetFreeBlockResult.m_pFreeBlock->RemoveFromBin(this, GetFreeBlockResult.m_ppFirstInBin);
	}
	else
	{
		m_LWMutex.Release();
		return NULL; // Out of memory 
	}

	if((unsigned int)Res < (unsigned int)m_pMemoryStart)
	{
		PrintAllAllocations();
		__debugbreak();
	}
	if((unsigned int)Res > (unsigned int)m_pMemoryEnd)
		__debugbreak();		
	m_NumAllocations++;
	VerifyConsistency();
	m_TotalAllocated += Size;
	m_LWMutex.Release();
	return Res;
}

//------------------------------------------------------------------------------------   
void CHeapAllocator::InternalFree(void* pMemory)
{
	if(pMemory == NULL)
		__debugbreak();	
	if((unsigned int)pMemory < (unsigned int)m_pMemoryStart)
		__debugbreak();
	if((unsigned int)pMemory > (unsigned int)m_pMemoryEnd)
		__debugbreak();

	m_LWMutex.Acquire();// mt safe
	CMemoryBlock* pBlock = (CMemoryBlock*)((unsigned char*)pMemory - sizeof(CMemoryBlock));
	unsigned int Size = pBlock->GetSize();
	pBlock->Free();
	CMemoryBlock* pPrevInMem = pBlock->GetPrevInMemory(this);
	if(pPrevInMem)
	{
		if(pPrevInMem->IsFree())
		{
			RemoveFromBin(pPrevInMem);
			pPrevInMem->Merge(this, pBlock);			
			pBlock = pPrevInMem;				
		}
	} 	
	CMemoryBlock* pNextInMem = pBlock->GetNextInMemory(this);	
	if(pNextInMem)
	{
		if(pNextInMem->IsFree())
		{
			RemoveFromBin(pNextInMem);				
			pBlock->Merge(this, pNextInMem);	
		}	
	}		
	AddToBin(pBlock);
	m_NumAllocations--;
	VerifyConsistency();
	m_TotalAllocated -= Size;
	m_LWMutex.Release();	
}

//------------------------------------------------------------------------------------
void* CHeapAllocator::AlignedAlloc(unsigned int Size, unsigned int Alignment)
{
	void* pMem = Alloc(Size + Alignment);
	// check if default allocation fails to satisfy alignment requirements
	if((POW2ALIGN((unsigned long long)pMem, (unsigned long long)Alignment)) != (unsigned long long)pMem)
	{
		unsigned long long AlignedAddress = POW2ALIGN((unsigned long long)pMem, (unsigned long long)Alignment);
		// store the offset to the unaligned address returned from Alloc in the 4 bytes preceeding the new aligned address
		unsigned int* pOffset = (unsigned int*)(AlignedAddress - 4);
		*pOffset = (unsigned int)(AlignedAddress - (unsigned long long)pMem) | scAlignedAllocMask;
		pMem = (void*)AlignedAddress;				
	}	
	VerifyConsistency();	
	return pMem;
}

//------------------------------------------------------------------------------------
void CHeapAllocator::Free(void* pMemory)
{
	unsigned int* pMemoryBlockOffset = (unsigned int*)pMemory;
	pMemoryBlockOffset--;
	if(*pMemoryBlockOffset & scAlignedAllocMask)	// do the 4 byte preceeding this address contain an offset to the actual analigned allocation address returned by Alloc?
	{
		unsigned int UnAlignedOffset = (*pMemoryBlockOffset & 0x3FFFFFFF);
		unsigned long long UnalignedMemory = (unsigned long long)pMemory - UnAlignedOffset;		
		InternalFree((void*)UnalignedMemory);		
	}
	else
	{
		InternalFree(pMemory);
	}
}

//------------------------------------------------------------------------------------
size_t CHeapAllocator::Msize(void* pMemory)
{
	m_LWMutex.Acquire();// mt safe
	CMemoryBlock* pBlock = (CMemoryBlock*)((unsigned char*)pMemory - sizeof(CMemoryBlock));
	size_t Size = pBlock->GetSize();
	m_LWMutex.Release();	
	return Size;
}

//------------------------------------------------------------------------------------
size_t CHeapAllocator::AlignedMsize(void* pMemory)
{
	m_LWMutex.Acquire();// mt safe
	size_t Size;
	unsigned int* pMemoryBlockOffset = (unsigned int*)pMemory;
	pMemoryBlockOffset--;
	if(*pMemoryBlockOffset & scAlignedAllocMask)	// do the 4 byte preceeding this address contain an offset to the actual analigned allocation address returned by Alloc?
	{
		unsigned int UnAlignedOffset = (*pMemoryBlockOffset & 0x3FFFFFFF);
		unsigned long long MemoryBlockAddress = (unsigned long long)pMemory - UnAlignedOffset - sizeof(CMemoryBlock);		
		CMemoryBlock* pBlock = (CMemoryBlock*)MemoryBlockAddress;		
		Size = pBlock->GetSize() - UnAlignedOffset;		
	}
	else
	{	
		CMemoryBlock* pBlock = (CMemoryBlock*)((unsigned char*)pMemory - sizeof(CMemoryBlock));
		Size = pBlock->GetSize();
	}		
	m_LWMutex.Release();	
	return Size;
}

//--------------------------------------------------------------------------------
void CHeapAllocator::Reset(void)
{
	__debugbreak();
}

// IAllocator overloads END


//------------------------------------------------------------------------------------
void CHeapAllocator::RemoveFromBin(CMemoryBlock* pBlock)
{
	unsigned int Size = pBlock->GetSize();
	if(Size < scMaxSmallAllocation)	
	{
		unsigned int SmallBinIndex = Size / scGranularity;	
		pBlock->RemoveFromBin(this, &m_pFreeBlockBins->m_pSmallBlocks[SmallBinIndex]);			
	}
	else
	{
		unsigned int LargeBinIndex = GetLargeBinIndex(Size);	
		pBlock->RemoveFromBin(this, &m_pFreeBlockBins->m_pLargeBlocks[LargeBinIndex]);
	}	
}

//------------------------------------------------------------------------------------
unsigned int CHeapAllocator::GetLargeBinIndex(unsigned int Size)
{
	unsigned int LargeBinIndex = Log2(Size);	
	LargeBinIndex = LargeBinIndex > scLog2MaxSmallAllocation ? LargeBinIndex - scLog2MaxSmallAllocation : 0;
	return LargeBinIndex;
}


//------------------------------------------------------------------------------------
void CHeapAllocator::PrintAllAllocations()
{
	CMemoryBlock* pBlock = (CMemoryBlock*)m_pMemoryStart;
	while(pBlock)
	{
		if(!pBlock->IsFree())
		{
			DebugPrintf("%d\n", pBlock->GetSize());
		}
		CMemoryBlock* pNextBlock = pBlock->GetNextInMemory(this);
		pBlock = pNextBlock;		
	}
}

void CHeapAllocator::VerifyConsistency()
{
	// iterate over all small bins and make sure that all block ptrs are correct
	for(int i = 0; i < scNumSmallBins; i++)
	{
		if(m_pFreeBlockBins->m_pSmallBlocks[i])
		{
			CMemoryBlock* pFreeBlock = m_pFreeBlockBins->m_pSmallBlocks[i];
			Assert(pFreeBlock->m_PrevInBin == 0xFFFFFFFF);
			for(pFreeBlock = m_pFreeBlockBins->m_pSmallBlocks[i]; pFreeBlock != NULL; pFreeBlock = pFreeBlock->GetNextInBin(this))
			{
				CMemoryBlock* pNextBlock = pFreeBlock->GetNextInBin(this);
				if(pNextBlock)
				{
					if(pNextBlock->GetPrevInBin(this) != pFreeBlock)
					{
						__debugbreak();
					}
				}
			}			
		}
	}
	
	// iterate over all large bins and make sure that all block ptrs are correct
	for(int i = 0; i < scNumLargeBins; i++)
	{
		if(m_pFreeBlockBins->m_pLargeBlocks[i])
		{
			CMemoryBlock* pFreeBlock = m_pFreeBlockBins->m_pLargeBlocks[i];
			Assert(pFreeBlock->m_PrevInBin == 0xFFFFFFFF);
			for(pFreeBlock = m_pFreeBlockBins->m_pSmallBlocks[i]; pFreeBlock != NULL; pFreeBlock = pFreeBlock->GetNextInBin(this))
			{
				CMemoryBlock* pNextBlock = pFreeBlock->GetNextInBin(this);
				if(pNextBlock)
				{
					if(pNextBlock->GetPrevInBin(this) != pFreeBlock)
					{
						__debugbreak();
					}
				}
			}			
		}
	}	
	unsigned int TotalAllocations = 0;
	CMemoryBlock* pBlock = (CMemoryBlock*)m_pMemoryStart;
	
	// iterate over the whole heap
	while(pBlock)
	{
		if(!pBlock->IsFree())
		{
			TotalAllocations++;
		}
		else // If block is free make sure it is in one of the free block bins
		{
			CMemoryBlock* pBlockIt;
			if(pBlock->m_Size < scMaxSmallAllocation)	
			{
				unsigned int SmallBinIndex = pBlock->m_Size / scGranularity;					
				for(pBlockIt = m_pFreeBlockBins->m_pSmallBlocks[SmallBinIndex]; pBlockIt != NULL; pBlockIt = pBlockIt->GetNextInBin(this))
				{
					if(pBlockIt == pBlock)
					{
						break;
					}
				}
			}
			else
			{
				unsigned int LargeBinIndex = GetLargeBinIndex(pBlock->m_Size);
				for(pBlockIt = m_pFreeBlockBins->m_pLargeBlocks[LargeBinIndex]; pBlockIt != NULL; pBlockIt = pBlockIt->GetNextInBin(this))
				{
					if(pBlockIt == pBlock)
					{
						break;
					}
				}
			}
			if(pBlockIt == NULL)
				__debugbreak();			
		}
		
		// check for block ptr consistency
		CMemoryBlock* pNextBlock = pBlock->GetNextInMemory(this);
		if(pNextBlock)
		{
			CMemoryBlock* pNextPrev = pNextBlock->GetPrevInMemory(this);
			if(pNextPrev != pBlock)
				__debugbreak();
		}
		pBlock = pNextBlock;
	}	
	if(TotalAllocations != m_NumAllocations)
		__debugbreak();
}

//------------------------------------------------------------------------------------
// CHeapAllocator implementation end
//------------------------------------------------------------------------------------
