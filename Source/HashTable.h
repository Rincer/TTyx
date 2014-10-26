#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include "Allocator.h"
#include "List.h"

template<class KeyType, class ValueType>
class CHashTable
{
	public:
		class CKeyValuePair
		{
			public:
				CKeyValuePair(KeyType Key, ValueType Value) :
					m_Key(Key),
					m_Value(Value)
				{}

				KeyType		m_Key;
				ValueType	m_Value;
		};


		CHashTable(unsigned int InitialSize, IAllocator* pAllocator)
		{
			m_HashSize = InitialSize;
		}

		bool Find(KeyType Key)
		{
			unsigned int ListIndex = Key % HashSize;
			return FindInList(ListIndex, Key, pValue);
		}

		typename CList<CKeyValuePair*>::CIterator* Alloc(unsigned long long Key)
		{
			CKeyValuePair* KeyValuePair = new CKeyValuePair(Key, Value);
			CDoubleLinkedList<CKeyValuePair*>::CIterator* pIterator = new CDoubleLinkedList<CKeyValuePair*>::CIterator(KeyValuePair);
			unsigned int ListIndex = Key % HashSize;
			m_UsedLists[ListIndex].AddBack(pIterator);
			return pIterator;
		}

		void Free(typename CDoubleLinkedList<CKeyValuePair*>::CIterator* pIterator)
		{
			unsigned int ListIndex = pIterator->GetData()->m_Key % HashSize;
			m_UsedLists[ListIndex].Remove(pIterator);
			m_FreeList.AddBack(pIterator);
		}

	private:
		bool FindInList(unsigned int ListIndex, unsigned long long Key, ValueType& pValue)
		{
			CList<CKeyValuePair*>::CIterator* pIterator = m_UsedLists[ListIndex].GetFirst();
			while (pIterator)
			{
				CKeyValuePair* pData = pIterator->GetData();
				if (pData->m_Key == Key)
				{
					pValue = pData->m_Value;
					return true;
				}
				pIterator = pIterator->Next();
			}
			return false;
		}
		unsigned int						m_HashSize;
		IAllocator*							m_pAllocator;

		CDoubleLinkedList<CKeyValuePair*>	m_UsedLists[HashSize];
		CDoubleLinkedList<CKeyValuePair*>	m_FreeList;				// store freed key/value pairs here to be reused.
};

#endif