#include "stdafx.h"
#include "Allocator.h"
#include "Skeleton.h"
#include "Animation.h"
#include "MemoryManager.h"

#include "HashMap.h"

//--------------------------------------------------------------------------------
CHashMap::CEntry::CEntry()
{
	m_Key = 0;
	m_pValue = NULL;
}				

//--------------------------------------------------------------------------------
CHashMap::CHashMap(IAllocator* pAllocator, unsigned int NumEntries)
{
	m_MaxEntries = NumEntries;
	m_NumEntries = 0;
	m_pEntries = PlacementNew<CEntry>(pAllocator->Alloc(NumEntries * sizeof(CEntry)), NumEntries); // allocate in place
	m_pAllocator = pAllocator;
}

//--------------------------------------------------------------------------------
CHashMap::~CHashMap()
{
	// nothing to free since memory will be given back once the  allocator is deleted
}

//--------------------------------------------------------------------------------
void* CHashMap::AddEntry(unsigned long long Key, const void* pValue, unsigned int ValueSize)
{
	void* pLocalValue = GetValue(Key);
	if(!pLocalValue)
	{
		Assert(m_NumEntries < m_MaxEntries); // check if full
		// create a copy of the value and add to the map
		pLocalValue = m_pAllocator->Alloc(ValueSize);
		memcpy(pLocalValue, pValue, ValueSize);	
		SortedInsert(Key, pLocalValue);
		m_NumEntries++;
	}	
	return pLocalValue;	
}

//--------------------------------------------------------------------------------
void* CHashMap::GetValue(unsigned long long Key) const
{
	int Pos;
	bool Found = BinarySearch(Key, Pos);
	if(!Found)
	{
		return NULL;
	}
	return m_pEntries[Pos].m_pValue;
}

//--------------------------------------------------------------------------------
// If map contains 'Key' 'Pos' is set to the map entry and 'true' is returned.
// Otherwise. 'Pos' is set to the 1st key value in the map that is greater than 'Key',
// or the end of the map if no values are greater and 'false' is returned
bool CHashMap::BinarySearch(unsigned long long Key, int& Pos) const
{
	// in place binary search
	int Num = m_NumEntries;
	int Lo = 0;
	int Hi = Num;
	bool KeyGreater = false;
	while( Num > 0 ) 
	{
		Pos = (Num / 2) + Lo;
		if( Key < m_pEntries[Pos].m_Key )		// search the lower half
		{
			Hi = Pos;
			KeyGreater = false;
		} 
		else if( Key > m_pEntries[Pos].m_Key )	// search the upper half
		{
			Lo = Pos + 1;
			KeyGreater = true;			
		} 
		else									// found
		{
			return true;
		}
		Num = Hi - Lo;
	}
	// adjust by 1 if 'Key' is greater than the last tested entry, so that 'Pos' points
	// to the 1st entry greater than 'Key'
	if(KeyGreater)
	{
		Pos++;
	}
	return false;
}

//--------------------------------------------------------------------------------
void CHashMap::SortedInsert(unsigned long long Key, void* pValue)
{
	if( m_NumEntries == 0 ) // Special case for empty map
	{
		m_pEntries[0].m_Key = Key;
		m_pEntries[0].m_pValue = pValue;
		return;
	}

	// Search for the entry position
	int Pos = 0;
	bool Found;
	Found = BinarySearch(Key, Pos);

	Assert(!Found); // check that the entry isnt already in the map

	// Shift the existing entries to make space for the new one, this is the expensive O(n) part.
	// For large maps a real binary red and black tree should be used to make insertion O(log(n)) 
	for(int i = m_NumEntries; i > Pos; i--) 
	{
		m_pEntries[i] = m_pEntries[i - 1];
	}
	m_pEntries[Pos].m_Key = Key;
	m_pEntries[Pos].m_pValue = pValue;	
}

//--------------------------------------------------------------------------------
unsigned int CHashMap::GetNumEntries()
{
	return m_NumEntries;
}

//--------------------------------------------------------------------------------
void CHashMap::GetEntry(unsigned int Index, unsigned long long& Key, void** ppValue)
{
	Key = m_pEntries[Index].m_Key;
	*ppValue = m_pEntries[Index].m_pValue;
}


