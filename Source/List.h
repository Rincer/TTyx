#ifndef _LIST_H_
#define _LIST_H_

#include "PooledAllocator.h"

//---------------------------------------------------------------------
// Double linked list of pointers to ValueType
template<class ValueType>
class CMultiList
{
	public:
		static const unsigned int sc_MaxMultiListIterators = 16 * 1024;
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
		CMultiList(unsigned int NumLists, CPooledAllocator* pAllocator)
		{	
			Initialize(NumLists, pAllocator);
		}

		//-------------------------------------------------------------------------------------------------------
		CMultiList() : m_NumLists(0),
			m_pAllocator(nullptr)
		{	
		}

		//-------------------------------------------------------------------------------------------------------
		~CMultiList()
		{
			for(unsigned int ListIndex = 0; ListIndex < m_NumLists; ListIndex++)
			{					  
				FreeList(ListIndex);		
			}
			delete[] m_pFirst;
			delete[] m_pLast;
		}

		//-------------------------------------------------------------------------------------------------------
		void Initialize(unsigned int NumLists, CPooledAllocator* pAllocator)
		{
			m_pAllocator = pAllocator;
			m_NumLists = NumLists;
			m_pFirst = new CIterator*[NumLists];
			m_pLast = new CIterator*[NumLists];
			for(unsigned int ListIndex = 0; ListIndex < NumLists; ListIndex++)
			{
				m_pFirst[ListIndex] = nullptr;
				m_pLast[ListIndex] = nullptr;
			}
		}

		//-------------------------------------------------------------------------------------------------------
		CIterator* AddFront(unsigned int ListIndex, ValueType* pValue)
		{	
			Assert(ListIndex < m_NumLists);
			CIterator* pIterator = GetIterator(pValue);	
			LinkFront(ListIndex, pIterator);
			return pIterator;
		}

		//-------------------------------------------------------------------------------------------------------
		CIterator* AddBack(unsigned int ListIndex, ValueType* pValue)
		{		
			Assert(ListIndex < m_NumLists);
			CIterator* pIterator = GetIterator(pValue);	
			LinkBack(ListIndex, pIterator);
			return pIterator;
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

		void FreeList(unsigned int ListIndex)
		{
			CIterator* pCurr = m_pFirst[ListIndex];
			while(pCurr)
			{
				CIterator* pNext = pCurr->Next();
				UnLink(ListIndex, pCurr);
				pCurr = pNext;
			}
		}

		// get from a list of discarded iterators
		CIterator* GetIterator(ValueType* pValue)
		{
			CIterator* pIterator;			
			pIterator = (CIterator*)m_pAllocator->Alloc(sizeof(CIterator));
			Assert(pIterator);
			pIterator->m_pValue = pValue;
			return pIterator;
		}

		// put in a list of discarded iterators
		void PutIterator(CIterator* pIterator)
		{
			m_pAllocator->Free(pIterator);
		}

		CIterator**			m_pFirst;
		CIterator**			m_pLast;
		unsigned int		m_NumLists;
		CPooledAllocator*	m_pAllocator;
};

#endif