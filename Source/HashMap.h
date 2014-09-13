#ifndef HASHMAP_H_
#define HASHMAP_H_

class IAllocator;
//--------------------------------------------------------------------------------
// Implements a sorted hash map. Maximum number of map entries should be kept
// pretty small since even though finding the location to insert uses binary
// search the insert itself is O(n). Retrieveing values uses just the binary search
// thus its fast O(log(n)). 
class CHashMap
{
	public:
		CHashMap(IAllocator* pAllocator, unsigned int NumEntries);
		~CHashMap();
		void* AddEntry(unsigned long long Key, const void* pValue, unsigned int ValueSize);
		void* GetValue(unsigned long long Key) const;
		unsigned int GetNumEntries();
		void GetEntry(unsigned int Index, unsigned long long& Key, void** ppValue);

	private:	
		bool BinarySearch(unsigned long long Key, int& Pos) const;	
		void SortedInsert(unsigned long long Key, void* pValue);
		
		int m_MaxEntries;
		int m_NumEntries;		
		class CEntry	// key value pair
		{
			public:
				CEntry();				
				unsigned long long	m_Key;
				void*				m_pValue;
		};		
		CEntry*		m_pEntries;
		IAllocator*	m_pAllocator;
};

//--------------------------------------------------------------------------------
template<class DataType>
class CStaticHashMap
{
	public:

		//---------------------------------------------------------------------------------------
		CStaticHashMap(IAllocator* pAllocator, unsigned int NumEntries)
		{
			m_TotalEntries = NumEntries;
			m_NumEntries = 0;
			if (pAllocator)
			{
				m_pAllocator = pAllocator;
				m_pEntries = PlacementNew<CEntry>(m_pAllocator->Alloc(sizeof(CEntry)* m_TotalEntries), NumEntries);
			}
			else
			{
				m_pAllocator = NULL;
				m_pEntries = new CEntry[m_TotalEntries];
			}
		}

		//---------------------------------------------------------------------------------------
		~CStaticHashMap()
		{
			if (m_pAllocator)
			{
				m_pAllocator->Free(m_pEntries);
			}
			else
			{
				delete[] m_pEntries;
			}
		}

		//---------------------------------------------------------------------------------------		
		void AddEntry(unsigned long long Key, const DataType& Value)
		{
			const DataType* pLocalValue = GetValue(Key);
			if (!pLocalValue)
			{
				Assert(m_NumEntries < m_TotalEntries); // check if full
				SortedInsert(Key, Value);
				m_NumEntries++;
			}
		}

		//---------------------------------------------------------------------------------------		
		const DataType* GetValue(unsigned long long Key) const
		{
			int Pos;
			bool Found = BinarySearch(Key, Pos);
			if (!Found)
			{
				return NULL;
			}
			return &m_pEntries[Pos].m_Value;
		}

	private:	
		//---------------------------------------------------------------------------------------
		bool BinarySearch(unsigned long long Key, int& Pos) const
		{
			// in place binary search
			int Num = m_NumEntries;
			int Lo = 0;
			int Hi = Num;
			bool KeyGreater = false;
			while (Num > 0)
			{
				Pos = (Num / 2) + Lo;
				if (Key < m_pEntries[Pos].m_Key)		// search the lower half
				{
					Hi = Pos;
					KeyGreater = false;
				}
				else if (Key > m_pEntries[Pos].m_Key)	// search the upper half
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
			if (KeyGreater)
			{
				Pos++;
			}
			return false;
		}

		//---------------------------------------------------------------------------------------
		void SortedInsert(unsigned long long Key, const DataType& Value)
		{
			if (m_NumEntries == 0) // Special case for empty map
			{
				m_pEntries[0].m_Key = Key;
				m_pEntries[0].m_Value = Value;
				return;
			}

			// Search for the entry position
			int Pos = 0;
			bool Found;
			Found = BinarySearch(Key, Pos);

			Assert(!Found); // check that the entry isnt already in the map

			// Shift the existing entries to make space for the new one, this is the expensive O(n) part.
			// For large maps a real binary red and black tree should be used to make insertion O(log(n)) 
			for (int i = m_NumEntries; i > Pos; i--)
			{
				m_pEntries[i] = m_pEntries[i - 1];
			}
			m_pEntries[Pos].m_Key = Key;
			m_pEntries[Pos].m_Value = Value;
		}

		int m_NumEntries;	
		int m_TotalEntries;	
		class CEntry	// key value pair
		{
			public:
				unsigned long long	m_Key;
				DataType			m_Value;
		};		
		CEntry*		m_pEntries;	
		IAllocator*	m_pAllocator;
};

#endif

