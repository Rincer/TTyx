#ifndef _LIST_H_
#define _LIST_H_

#include "HeapAllocator.h"
//---------------------------------------------------------------------
// Double linked list of pointers to ValueType
template<unsigned int NumLists, class ValueType>
class CMultiList
{
	public:
		class CIterator
		{
			public:
				//-------------------------------------------------------------------------------------------------------
				CIterator(ValueType* pValue)
				{
					m_pPrev = nullptr;
					m_pNext = nullptr;
					m_pValue = pValue;
				}

				//-------------------------------------------------------------------------------------------------------
				ValueType* GetValue()
				{
					return m_pValue;
				}

				//-------------------------------------------------------------------------------------------------------
				CIterator* Next()
				{
					return m_pNext;
				}

			private:
				CIterator*	m_pPrev;
				CIterator*	m_pNext;
				ValueType*	m_pValue;

			friend class CMultiList;
		};

		//-------------------------------------------------------------------------------------------------------
		CMultiList(CHeapAllocator* pAllocator) : m_pFree(nullptr),
			m_pAllocator(pAllocator)
		{	
			for(unsigned int ListIndex = 0; ListIndex < NumLists; ListIndex++)
			{
				m_pFirst[ListIndex] = nullptr;
				m_pLast[ListIndex] = nullptr;
			}
		}

		//-------------------------------------------------------------------------------------------------------
		~CMultiList()
		{
		}

		//-------------------------------------------------------------------------------------------------------
		void AddFront(unsigned int ListIndex, ValueType* pValue)
		{	
			Assert(ListIndex < NumLists);
			CIterator* pIterator = GetIterator(pValue);	
			LinkFront(ListIndex, pIterator);
		}

		//-------------------------------------------------------------------------------------------------------
		void AddBack(unsigned int ListIndex, ValueType* pValue)
		{		
			Assert(ListIndex < NumLists);
			CIterator* pIterator = GetIterator(pValue);	
			LinkBack(ListIndex, pIterator);
		}

		//-------------------------------------------------------------------------------------------------------
		void Remove(unsigned int ListIndex, CIterator* pIterator)
		{
			UnLink(ListIndex, pIterator);		
		}

		bool IsEmpty(unsigned int ListIndex)
		{
			return m_pFirst[ListIndex] == nullptr;
		}

		CIterator* GetFirst(unsigned int ListIndex)
		{
			return m_pFirst[ListIndex];
		}

	private:

		void LinkFront(unsigned int ListIndex, CIterator* pIterator)
		{
			CIterator*& pFirst = m_pFirst[ListIndex];
			CIterator*& pLast = m_pLast[ListIndex];
			if (pFirst == nullptr)
			{
				pIterator->m_pNext = nullptr;
				pIterator->m_pPrev = nullptr;
				pFirst = pIterator;
				pLast = pFirst;
			}
			else					// add at the start
			{
				pIterator->m_pNext = pFirst;
				pIterator->m_pPrev = nullptr;
				pFirst->m_pPrev = pIterator;
				pFirst = pIterator;
			}
		}

		void LinkBack(unsigned int ListIndex, CIterator* pIterator)
		{
			CIterator*& pFirst = m_pFirst[ListIndex];
			CIterator*& pLast = m_pLast[ListIndex];
			if (pFirst == nullptr)
			{
				Assert(pLast == nullptr);
				pIterator->m_pNext = nullptr;
				pIterator->m_pPrev = nullptr;
				pFirst = pIterator;
				pLast = pFirst;
			}
			else
			{
				pLast->m_pNext = pIterator;
				pIterator->m_pPrev = pLast;
				pLast = pIterator;
				pLast->m_pNext = nullptr;
			}
		}

		void UnLink(unsigned int ListIndex, CIterator* pIterator)
		{
			CIterator*& pFirst = m_pFirst[ListIndex];
			CIterator*& pLast = m_pLast[ListIndex];
			if (pIterator == pFirst)
			{
				pFirst = pFirst->m_pNext;
				if (!pFirst) // Was 1st and last element in the list
				{
					pLast = nullptr;
				}
			}
			else if (pIterator == pLast)
			{
				pLast = pIterator->m_pPrev;
				pLast->m_pNext = nullptr;
			}
			else
			{
				pIterator->m_pPrev->m_pNext = pIterator->m_pNext;
				if (pIterator->m_pNext)
				{
					pIterator->m_pNext->m_pPrev = pIterator->m_pPrev;
				}
			}
			// recycle this iterator
			PutIterator(pIterator);
		}

		// get from a list of discarded iterators
		CIterator* GetIterator(ValueType* pValue)
		{
			CIterator* pIterator;
			if(m_pFree == nullptr)
			{
				pIterator = (CIterator*)m_pAllocator->Alloc(sizeof(CIterator));
				Assert(pIterator);
			}
			else
			{
				pIterator = m_pFree;
				m_pFree = m_pFree->m_pNext;
			}
			pIterator->m_pValue = pValue;
			return pIterator;
		}

		// put in a list of discarded iterators
		void PutIterator(CIterator* pIterator)
		{
			if(m_pFree)
			{
				pIterator->m_pNext = m_pFree;
			}
			else
			{
				pIterator->m_pNext = nullptr;
			}
			m_pFree = pIterator;
		}

		CIterator* m_pFirst[NumLists];
		CIterator* m_pLast[NumLists];
		CIterator* m_pFree;
		CHeapAllocator* m_pAllocator;
};

#endif