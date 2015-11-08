#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include "Allocator.h"
#include "PooledAllocator.h"
#include "HeapAllocator.h"
#include "List.h"
#include "Pair.h"
/*
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
*/

template<class ValueType, unsigned int HashSize>
class CHashTable
{
	typedef CPair<unsigned long long, ValueType> PairType; 
	public:
		typedef typename CMultiList<PairType>::CIterator IteratorType;
		CHashTable(unsigned int NumElements, CHeapAllocator* pAllocator) : 
			m_PairAllocator(sizeof(PairType), NumElements, 1, pAllocator),
			m_ListAllocator(sizeof(IteratorType), NumElements, 1, pAllocator),
			m_Buckets(HashSize, &m_ListAllocator)
		{
		}

		IteratorType* Add(ValueType Value, unsigned long long Key)
		{
			unsigned int HashIndex = Key % HashSize;
			PairType* pPair = new (m_PairAllocator.Alloc(sizeof(PairType)))PairType(Key, Value);
			return m_Buckets.AddBack(HashIndex, pPair);
		}

		void Remove(IteratorType* pIterator)
		{
			PairType* pPair = pIterator->GetValue();
			unsigned int HashIndex = pPair->m_Value0 % HashSize;
			m_PairAllocator.Free(pPair);
			m_Buckets.Remove(HashIndex, pIterator);
		}

		IteratorType* Find(unsigned long long Key)
		{
			unsigned int HashIndex = Key % HashSize;
			for(IteratorType* pIterator = m_Buckets.GetFirst(HashIndex); pIterator != nullptr; pIterator = pIterator->Next())
			{
				PairType* pPair = pIterator->GetValue();
				if(pPair->m_Value0 == Key)
				{
					return pIterator;
				}
			}
			return nullptr;
		}

	private:
		CMultiList<PairType> m_Buckets;
		CPooledAllocator m_PairAllocator;
		CPooledAllocator m_ListAllocator;
};
#endif