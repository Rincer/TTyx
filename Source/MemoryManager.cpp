#include "stdafx.h"
#include "Macros.h"
#include "Math.h"
#include "MemoryManager.h"
#include "JobSystem.h"

#define DEFAULT_ALIGNMENT (4096)

// C++ new and delete variants
//----------------------------------------------------------------------------------------------
void* operator new (unsigned int Size)
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc(Size);
	return pMemory;
}

//----------------------------------------------------------------------------------------------
void operator delete (void *pMemory)
{
	CMemoryManager::GetAllocator().Free(pMemory);
}

//----------------------------------------------------------------------------------------------
void* operator new[] (unsigned int Size)
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc(Size);
	return pMemory;
}

//----------------------------------------------------------------------------------------------
void operator delete[] (void *pMemory)
{
	CMemoryManager::GetAllocator().Free(pMemory);
}

//----------------------------------------------------------------------------------------------
void CMemoryManager::ShutDown()
{
	GetAllocator().ShutDown();
}


//----------------------------------------------------------------------------------------------
CMemoryCapper::CMemoryCapper(const unsigned int Capacity) : m_Capacity(Capacity),
															m_Size(0)
{
}

//--------------------------------------------------------------------------------
void* CMemoryCapper::Alloc(unsigned int Size)
{
	m_Mutex.Acquire();
	unsigned char* pMemory = NULL;
	if(m_Size + Size <= m_Capacity)
	{
		pMemory = new unsigned char[Size];
		Assert(pMemory);
		m_Size += Size;
	}
	m_Mutex.Release();
	return pMemory;
}

//--------------------------------------------------------------------------------
void CMemoryCapper::Free(const void* pMemory, unsigned int Size)
{
	m_Mutex.Acquire();
	delete[] pMemory;
	m_Size -= Size;
	m_Mutex.Release();	
}

#if 0	// attempt to override all memory allocation calls has failed due to the fact that sometimes we get calls to free for memory locations outside our range
		// perhaps try again sometimes in the future.

// all the C-style memory management functions
// all of the 'malloc' and 'realloc' variants
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl malloc (
		size_t nSize
		)
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _malloc_dbg (
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" void * __cdecl _nh_malloc (
        size_t nSize,
        int nhFlag
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" static void * __cdecl _nh_malloc_dbg_impl(
        size_t nSize,
        int nhFlag,
        int nBlockUse,
        const char * szFileName,
        int nLine,
        int * errno_tmp
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" void * __cdecl _nh_malloc_dbg(
        size_t nSize,
        int nhFlag,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" void * __cdecl _heap_alloc(
        size_t nSize
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" static void * __cdecl _heap_alloc_dbg_impl(
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine,
        int * errno_tmp
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" void * __cdecl _heap_alloc_dbg(
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)nSize);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _realloc_dbg(
        void * pUserData,
        size_t nNewSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(nNewSize));
	CMemoryManager::GetAllocator().Free(pUserData);
	return pMemory;
}

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl realloc(
        void * pUserData,
        size_t nNewSize
        )
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(nNewSize));
	CMemoryManager::GetAllocator().Free(pUserData);
	return pMemory;
}

//----------------------------------------------------------------------------------------------
extern "C" static void * __cdecl realloc_help(
        void * pUserData,
        size_t * pnNewSize,
        int nBlockUse,
        const char * szFileName,
        int nLine,
        int fRealloc
        )
{
	__debugbreak(); // who is calling this? Implement when needed     
	return NULL;  
}        

// all 'calloc' and 'recalloc' variants
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl calloc(
		size_t nNum, size_t nSize
		)
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(nSize * nNum));
	memset(pMemory, 0, (unsigned int)(nSize * nNum));	
	return pMemory;
}

//----------------------------------------------------------------------------------------------
extern "C" void * __cdecl _calloc_impl(
        size_t nNum,
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine,
        int * errno_tmp
        )
{        
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(nSize * nNum));
	memset(pMemory, 0, (unsigned int)(nSize * nNum));	
	return pMemory;
}

//----------------------------------------------------------------------------------------------
extern "C" void * __cdecl _calloc_dbg_impl(
        size_t nNum,
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine,
        int * errno_tmp
        )
{        
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(nSize * nNum));
	memset(pMemory, 0, (unsigned int)(nSize * nNum));	
	return pMemory;
}

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _calloc_dbg(
        size_t nNum,
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{        
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(nSize * nNum));
	memset(pMemory, 0, (unsigned int)(nSize * nNum));	
	return pMemory;
}

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _recalloc(
		void *memblock,
		size_t count,
		size_t size
		)
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(size * count));
	memset(pMemory, 0, (unsigned int)(size * count));	
	size_t OldSize = _msize(memblock);
	memcpy(pMemory, memblock, MIN(OldSize, size * count));
	CMemoryManager::GetAllocator().Free(memblock);
	return pMemory;
}	

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _recalloc_dbg(
		void * memblock,
		size_t count,
		size_t size,
		int nBlockUse,
		const char * szFileName,
		int nLine
		)
{
	void *pMemory = CMemoryManager::GetAllocator().Alloc((unsigned int)(size * count));
	memset(pMemory, 0, (unsigned int)(size * count));	
	size_t OldSize = _msize(memblock);
	memcpy(pMemory, memblock, MIN(OldSize, count * size));
	CMemoryManager::GetAllocator().Free(memblock);
	return pMemory;
}

// all the 'free' variants
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void __cdecl free(
		void* pMemory
		)
{
	CMemoryManager::GetAllocator().Free(pMemory);
}

//----------------------------------------------------------------------------------------------
extern "C" void __cdecl _free_nolock(
        void * pUserData
        )
{
	CMemoryManager::GetAllocator().Free(pUserData);
}        

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void __cdecl _free_dbg(
        void * pUserData,
        int nBlockUse
        )
{        
	CMemoryManager::GetAllocator().Free(pUserData);
}

//----------------------------------------------------------------------------------------------
extern "C" void __cdecl _free_dbg_nolock(
        void * pUserData,
        int nBlockUse
        )
{
	CMemoryManager::GetAllocator().Free(pUserData);
}        

// all the 'msize' variants
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP size_t __cdecl _msize (
        void * pUserData
        )
{
	return CMemoryManager::GetAllocator().Msize(pUserData);
}        

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP size_t __cdecl _msize_dbg (
        void * pUserData,
        int nBlockUse
        )
{
	return CMemoryManager::GetAllocator().Msize(pUserData);
}        

// all the 'aligned malloc' variants
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_malloc(
        size_t size,
        size_t align
        )
{
	void* pMemory = CMemoryManager::GetAllocator().AlignedAlloc((unsigned int)size, (unsigned int)align);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_malloc_dbg(
        size_t size,
        size_t align,
        const char * f_name,
        int line_n
        )
{
	void* pMemory = CMemoryManager::GetAllocator().AlignedAlloc((unsigned int)size, (unsigned int)align);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_realloc(
        void * memblock,
        size_t size,
        size_t align
        )
{
	void* pMemory = CMemoryManager::GetAllocator().AlignedAlloc((unsigned int)size, (unsigned int)align);
	CMemoryManager::GetAllocator().AlignedFree(memblock);
	return pMemory;
}        

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_realloc_dbg(
        void *memblock,
        size_t size,
        size_t align,
        const char * f_name,
        int line_n
        )
{
	void* pMemory = CMemoryManager::GetAllocator().AlignedAlloc((unsigned int)size, (unsigned int)align);
	CMemoryManager::GetAllocator().AlignedFree(memblock);
	return pMemory;
}

// all the 'aligned recalloc' variants, weird that they exist since there is no '_aligned_calloc'
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_recalloc(
        void * memblock,
        size_t count,
        size_t size,
        size_t align
        )
{        
	__debugbreak(); // who is calling this? Implement when needed    
	return NULL;   
}	

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_recalloc_dbg(
        void *memblock,
        size_t count,
        size_t size,
        size_t align,
        const char * f_name,
        int line_n
        )
{        
	__debugbreak(); // who is calling this? Implement when needed   
	return NULL;   	    
}	

// all the 'aligned free' variants
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void __cdecl _aligned_free(
        void *memblock
        )
{
    CMemoryManager::GetAllocator().AlignedFree(memblock);
}

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void __cdecl _aligned_free_dbg(
        void * memblock
        )
{
    CMemoryManager::GetAllocator().AlignedFree(memblock);
}

// all the 'aligned msize' variants
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP size_t __cdecl _aligned_msize(
        void *memblock,
        size_t align,
        size_t offset
        )
{
	return CMemoryManager::GetAllocator().AlignedMsize(memblock);
}        

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP size_t __cdecl _aligned_msize_dbg(
        void * memblock,
        size_t align,
        size_t offset
        )
{
	return CMemoryManager::GetAllocator().AlignedMsize(memblock);
}        
        
// all the 'aligned offset' variants, implement if someone actually uses them
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_offset_malloc(
        size_t size,
        size_t align,
        size_t offset
        )
{        
	__debugbreak(); // who is calling this? Implement when needed   
	return NULL;   	    
}	
        
//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_offset_malloc_dbg(
        size_t size,
        size_t align,
        size_t offset,
        const char * f_name,
        int line_n
        )
{        
	__debugbreak(); // who is calling this? Implement when needed   
	return NULL;   	    
}	

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_offset_realloc(
        void * memblock,
        size_t size,
        size_t align,
        size_t offset
        )
{        
	__debugbreak(); // who is calling this? Implement when needed   
	return NULL;   	    
}	

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_offset_recalloc(
        void * memblock,
        size_t count,
        size_t size,
        size_t align,
        size_t offset
        )
{        
	__debugbreak(); // who is calling this? Implement when needed   
	return NULL;   	    
}	

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_offset_realloc_dbg(
        void * memblock,
        size_t size,
        size_t align,
        size_t offset,
        const char * f_name,
        int line_n
        )
{        
	__debugbreak(); // who is calling this? Implement when needed   
	return NULL;   	    
}	

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _aligned_offset_recalloc_dbg
(
    void * memblock,
    size_t count,
    size_t size,
    size_t align,
    size_t offset,
    const char * f_name,
    int line_n
)
{        
	__debugbreak(); // who is calling this? Implement when needed   
	return NULL;   	    
}	

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _expand(
        void * pUserData,
        size_t nNewSize
        )
{
	__debugbreak(); // who is calling this? Implement when needed       
	return NULL;   	
}

//----------------------------------------------------------------------------------------------
extern "C" _CRTIMP void * __cdecl _expand_dbg(
        void * pUserData,
        size_t nNewSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
	__debugbreak(); // who is calling this? Implement when needed    
	return NULL;   	   
}


void ReplaceFunction(HMODULE H, const char* pName, unsigned char* pAddress)
{
	DWORD dwOldProtect;
	DWORD dwAddressToIntercept = (DWORD)GetProcAddress( H, pName); 
	if(dwAddressToIntercept)
	{
		unsigned char *pbTargetCode = (unsigned char *) dwAddressToIntercept;
		VirtualProtect((void *) dwAddressToIntercept, 5, PAGE_WRITECOPY, &dwOldProtect);
		*pbTargetCode++ = 0xE9;       
		*((signed int *)(pbTargetCode)) = pAddress - (pbTargetCode + 4);  
		VirtualProtect((void *) dwAddressToIntercept, 5, PAGE_EXECUTE, &dwOldProtect); 
		FlushInstructionCache(GetCurrentProcess(), NULL, NULL); 		
	}
}

static bool MSVCRTOverriden = false;
//----------------------------------------------------------------------------------------------
void CMemoryManager::OverrideMSVCRT()
{
	if(MSVCRTOverriden)
		return;
	MSVCRTOverriden = true;
	HMODULE H = GetModuleHandleA("msvcrt.dll");
	if(H)
	{
		ReplaceFunction(H, "malloc", (unsigned char*)malloc);
		ReplaceFunction(H, "_malloc_dbg",(unsigned char*)_malloc_dbg);
		ReplaceFunction(H, "_nh_malloc",(unsigned char*)_nh_malloc);
		ReplaceFunction(H, "_nh_malloc_dbg_impl",(unsigned char*)_nh_malloc_dbg_impl);
		ReplaceFunction(H, "_nh_malloc_dbg",(unsigned char*)_nh_malloc_dbg);
		ReplaceFunction(H, "_heap_alloc",(unsigned char*)_heap_alloc);
		ReplaceFunction(H, "_heap_alloc_dbg_impl",(unsigned char*)_heap_alloc_dbg_impl);
		ReplaceFunction(H, "_heap_alloc_dbg",(unsigned char*)_heap_alloc_dbg);
		ReplaceFunction(H, "_realloc_dbg",(unsigned char*)_realloc_dbg);
		ReplaceFunction(H, "realloc",(unsigned char*)realloc);
		ReplaceFunction(H, "realloc_help",(unsigned char*)realloc_help);
		ReplaceFunction(H, "calloc",(unsigned char*)calloc);
		ReplaceFunction(H, "_calloc_impl",(unsigned char*)_calloc_impl);
		ReplaceFunction(H, "_calloc_dbg_impl",(unsigned char*)_calloc_dbg_impl);
		ReplaceFunction(H, "_calloc_dbg",(unsigned char*)_calloc_dbg);
		ReplaceFunction(H, "_recalloc",(unsigned char*)_recalloc);
		ReplaceFunction(H, "_recalloc_dbg",(unsigned char*)_recalloc_dbg);
		ReplaceFunction(H, "free",(unsigned char*)free);
		ReplaceFunction(H, "_free_nolock",(unsigned char*)_free_nolock);
		ReplaceFunction(H, "_free_dbg",(unsigned char*)_free_dbg);
		ReplaceFunction(H, "_free_dbg_nolock",(unsigned char*)_free_dbg_nolock);
		ReplaceFunction(H, "_msize",(unsigned char*)_msize);
		ReplaceFunction(H, "_msize_dbg",(unsigned char*)_msize_dbg);
		ReplaceFunction(H, "_aligned_malloc",(unsigned char*)_aligned_malloc);
		ReplaceFunction(H, "_aligned_malloc_dbg",(unsigned char*)_aligned_malloc_dbg);
		ReplaceFunction(H, "_aligned_realloc",(unsigned char*)_aligned_realloc);
		ReplaceFunction(H, "_aligned_realloc_dbg",(unsigned char*)_aligned_realloc_dbg);
		ReplaceFunction(H, "_aligned_recalloc",(unsigned char*)_aligned_recalloc);
		ReplaceFunction(H, "_aligned_recalloc_dbg",(unsigned char*)_aligned_recalloc_dbg);
		ReplaceFunction(H, "_aligned_free",(unsigned char*)_aligned_free);
		ReplaceFunction(H, "_aligned_free_dbg",(unsigned char*)_aligned_free_dbg);
		ReplaceFunction(H, "_aligned_msize",(unsigned char*)_aligned_msize);
		ReplaceFunction(H, "_aligned_msize_dbg",(unsigned char*)_aligned_msize_dbg);
		ReplaceFunction(H, "_aligned_offset_malloc",(unsigned char*)_aligned_offset_malloc);
		ReplaceFunction(H, "_aligned_offset_malloc_dbg",(unsigned char*)_aligned_offset_malloc_dbg);
		ReplaceFunction(H, "_aligned_offset_realloc",(unsigned char*)_aligned_offset_realloc);
		ReplaceFunction(H, "_aligned_offset_recalloc",(unsigned char*)_aligned_offset_recalloc);
		ReplaceFunction(H, "_aligned_offset_realloc_dbg",(unsigned char*)_aligned_offset_realloc_dbg);
		ReplaceFunction(H, "_aligned_offset_recalloc_dbg",(unsigned char*)_aligned_offset_recalloc_dbg);
		ReplaceFunction(H, "_expand",(unsigned char*)_expand);
		ReplaceFunction(H, "_expand_dbg",(unsigned char*)_expand_dbg);
	}
}
#endif


