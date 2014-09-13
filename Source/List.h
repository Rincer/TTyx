#ifndef _LIST_H_
#define _LIST_H_

//---------------------------------------------------------------------
// Double linked list
template<class DataType>
class CList
{
	public:
		class CIterator
		{
			public:
				//-------------------------------------------------------------------------------------------------------
				CIterator(DataType* pData)
				{
					m_pPrev = NULL;
					m_pNext = NULL;
					m_pData = pData;
				}

				//-------------------------------------------------------------------------------------------------------
				DataType* GetData()
				{
					return m_pData;
				}

				//-------------------------------------------------------------------------------------------------------
				CIterator* Next()
				{
					return m_pNext;
				}

			private:
				CIterator*	m_pPrev;
				CIterator*	m_pNext;
				DataType*	m_pData;

			friend class CList<DataType>;
		};

		//-------------------------------------------------------------------------------------------------------
		CList()
		{
			m_pFirst = NULL;
			m_pLast = NULL;
		}

		//-------------------------------------------------------------------------------------------------------
		~CList()
		{
		}

		//-------------------------------------------------------------------------------------------------------
		void AddFront(DataType* pData)
		{
			CIterator* pIterator = pData->GetIterator();
			if (m_pFirst == NULL)
			{
				pIterator->m_pNext = NULL;
				pIterator->m_pPrev = NULL;
				m_pFirst = pIterator;
				m_pLast = m_pFirst;
			}
			else					// add at the start
			{
				pIterator->m_pNext = m_pFirst;
				pIterator->m_pPrev = NULL;
				m_pFirst->m_pPrev = pIterator;
				m_pFirst = pIterator;
			}
		}

		//-------------------------------------------------------------------------------------------------------
		void AddBack(DataType* pData)
		{
			CIterator* pIterator = pData->GetIterator();
			if (m_pFirst == NULL)
			{
				Assert(m_pLast == NULL);
				pIterator->m_pNext = NULL;
				pIterator->m_pPrev = NULL;
				m_pFirst = pIterator;
				m_pLast = m_pFirst;
			}
			else
			{
				m_pLast->m_pNext = pIterator;
				pIterator->m_pPrev = m_pLast;
				m_pLast = pIterator;
				m_pLast->m_pNext = NULL;
			}
		}

		//-------------------------------------------------------------------------------------------------------
		void Remove(DataType* pData)
		{
			CIterator* pIterator = pData->GetIterator();
			if (pIterator == m_pFirst)
			{
				m_pFirst = m_pFirst->m_pNext;
				if (!m_pFirst) // Was 1st and last element in the list
				{
					m_pLast = NULL;
				}
			}
			else if (pIterator == m_pLast)
			{
				m_pLast = pIterator->m_pPrev;
				m_pLast->m_pNext = NULL;
			}
			else
			{
				pIterator->m_pPrev->m_pNext = pIterator->m_pNext;
				if (pIterator->m_pNext)
				{
					pIterator->m_pNext->m_pPrev = pIterator->m_pPrev;
				}
			}
		}

		bool IsEmpty()
		{
			return m_pFirst == NULL;
		}

		CIterator* GetFirst()
		{
			return m_pFirst;
		}

	private:
		CIterator* m_pFirst;
		CIterator* m_pLast;
};

//--------------------------------------------------------------------------------------------
// Interface for any class that wants to be inserted into a List
template<class DataType>
class IIterant
{
	public:
		virtual typename DataType::CIterator* GetIterator() = 0;
};

#endif