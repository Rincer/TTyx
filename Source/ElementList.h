#ifndef ELEMENTLIST_H_
#define ELEMENTLIST_H_

#include "MemoryManager.h"

// Some forward declarations
class IAllocator;
class CElementEntry;


//--------------------------------------------------------------------------------
// Standard doubly linked list data container class
class CElement
{
	public:
		CElement()
		{
			m_pNext = NULL;
			m_pPrev = NULL;
			m_pData = NULL;		
		}		
		CElement*	  m_pNext;
		CElement*	  m_pPrev;
		const void*	  m_pData;
};


// Interface to access the CElement through the stored data
class ILinkable
{
	public:
		ILinkable(CElement* pElement);
		CElement* GetElement();	
	private:
		CElement* m_pElement;
};


//--------------------------------------------------------------------------------
// Maintains a singly linked list of element that is shared between multiple
// CElementLists.  
class CElementPool
{
	public:
		CElementPool(unsigned int NumNodes, IAllocator* pAllocator);
		~CElementPool();
		CElement* AllocElement();
		void FreeElement(CElement* pElement);
		
	private:
		CElement* m_pFree;		
};

//--------------------------------------------------------------------------------
// Maintains a doubly linked list of data ptrs. When new data is added to the list
// gets an available data container (CElement) from a common pool shared by
// other CElementLists. When data is removed, returns the data container back to
// the pool. Data is allocated and maintained exterally to the list and only pointers to data are stored
class CElementList
{
	public:			
		CElementList(CElementPool* pPool);
		~CElementList();

		void Add(const void* pData);
		void Remove(CElement& Element);
		CElement* GetFirst() const;

	private:
		CElement* m_pFirst;
		CElementPool* m_pElementPool;	
};


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------------
// Interface to access the CElement through the stored data
class IAccessible
{
	public:
		IAccessible();
		void RegisterElement(const CElementEntry* pElement);
		const CElementEntry* GetElement();	
	private:
		const CElementEntry* m_pElementEntry; // used to store reference to this object in the asset storage	

};

//--------------------------------------------------------------------------------
template<class StateType>
class CStateAccessor : public IAccessible
{
	public:
		void SetState(StateType State) { m_State = State; }
		StateType GetState() const { return m_State; }
		bool IsLoaded() const { return m_State == StateType::eLoaded; }
		bool IsReference() const { return m_State == StateType::eReference; }

	private:
		volatile StateType m_State;
};

//--------------------------------------------------------------------------------
// Standard doubly linked list data container class
class CElementEntry
{
	public:
		CElementEntry()
		{
			m_pNext = NULL;
			m_pPrev = NULL;
			m_pData = NULL;		
			m_SegmentIndex = 0xFFFFFFFF;
		}		
		CElementEntry*	m_pNext;
		CElementEntry*	m_pPrev;
		IAccessible*	m_pData;
		unsigned int	m_SegmentIndex;
};

//--------------------------------------------------------------------------------
// Maintains a double linked list of element that is shared between multiple
// CElementArray<DataType>s  
template<class DataType>
class CElementArrayPool
{
	public:

		//--------------------------------------------------------------------------------
		CElementArrayPool(IAllocator* pAllocator, unsigned int NumSegments, unsigned int SegmentSize)
		{
			m_pAllocator = pAllocator;
			m_pFree = NULL;
			m_NumSegments = NumSegments;
			m_SegmentSize = SegmentSize;

			if (m_pAllocator == NULL)
			{
				m_ppElements = new CElementEntry*[NumSegments];
				m_ppData = new DataType*[NumSegments];
				m_pFreeElementCounts = new unsigned int[NumSegments];
			}
			else
			{
				m_ppElements = new (m_pAllocator->Alloc(sizeof(CElementEntry*)* NumSegments)) CElementEntry*[NumSegments];
				m_ppData = new (m_pAllocator->Alloc(sizeof(DataType*)* NumSegments)) DataType*[NumSegments];
				m_pFreeElementCounts = new (m_pAllocator->Alloc(sizeof(unsigned int)* NumSegments)) unsigned int[NumSegments];
			}

			for (unsigned int SegmentIndex = 0; SegmentIndex < NumSegments; SegmentIndex++)
			{
				m_ppElements[SegmentIndex] = NULL;
				m_ppData[SegmentIndex] = NULL;
				m_pFreeElementCounts[SegmentIndex] = SegmentSize;	// All elements are free, although not allocated yet
			}
		}


		//--------------------------------------------------------------------------------
		~CElementArrayPool()
		{
			if (!m_pAllocator)
			{
				for (unsigned int SegmentIndex = 0; SegmentIndex < m_NumSegments; SegmentIndex++)
				{
					if (m_ppElements[SegmentIndex] != NULL)
					{
						delete[] m_ppElements[SegmentIndex];
					}

					if (m_ppData[SegmentIndex] != NULL)
					{
						delete[] m_ppData[SegmentIndex];
					}
				}
				delete[] m_pFreeElementCounts;
				delete[] m_ppData;
				delete[] m_ppElements;
			}
		}

		//--------------------------------------------------------------------------------

		void AllocSegment()
		{
			unsigned int FreeSegment;
			for (FreeSegment = 0; FreeSegment < m_NumSegments; FreeSegment++)
			{
				if (m_pFreeElementCounts[FreeSegment] == m_SegmentSize)
					break;
			}
			Assert(FreeSegment < m_NumSegments);

			if (m_pAllocator == NULL) // Use global memory allocator
			{
				m_ppElements[FreeSegment] = new CElementEntry[m_SegmentSize];
				m_ppData[FreeSegment] = new DataType[m_SegmentSize];
			}
			else
			{
				// allocate in place
				m_ppElements[FreeSegment] = PlacementNew<CElementEntry>(m_pAllocator->Alloc(sizeof(CElementEntry)* m_SegmentSize), m_SegmentSize);
				m_ppData[FreeSegment] = PlacementNew<DataType>(m_pAllocator->Alloc(sizeof(DataType)* m_SegmentSize), m_SegmentSize);
			}
			m_pFreeElementCounts[FreeSegment] = m_SegmentSize;

			// create a doubly linked list of free elements
			m_ppElements[FreeSegment][0].m_pPrev = NULL;
			for (unsigned int i = 1; i < m_SegmentSize; i++)
			{
				m_ppElements[FreeSegment][i - 1].m_pNext = &m_ppElements[FreeSegment][i];
				m_ppElements[FreeSegment][i].m_pPrev = &m_ppElements[FreeSegment][i - 1];
			}
			m_ppElements[FreeSegment][m_SegmentSize - 1].m_pNext = NULL;
			// setup data pointers
			for (unsigned int i = 0; i < m_SegmentSize; i++)
			{
				m_ppElements[FreeSegment][i].m_pData = &m_ppData[FreeSegment][i];
				m_ppElements[FreeSegment][i].m_pData->RegisterElement(&m_ppElements[FreeSegment][i]);
				m_ppElements[FreeSegment][i].m_SegmentIndex = FreeSegment;
			}
			m_pFree = &m_ppElements[FreeSegment][0];
		}


		//--------------------------------------------------------------------------------
		void FreeSegment(unsigned int SegmentIndex)
		{
			CElementEntry* pElement = m_pFree;
			unsigned int ElementCount = 0;
			while (ElementCount < m_SegmentSize)
			{
				CElementEntry* pNextElement = pElement->m_pNext;
				if (pElement->m_SegmentIndex == SegmentIndex)
				{
					if (pElement == m_pFree) // Unlink at head
					{
						m_pFree = pElement->m_pNext;
						if (m_pFree)
						{
							m_pFree->m_pPrev = NULL;
						}
					}
					else
					{
						pElement->m_pPrev->m_pNext = pElement->m_pNext;
						if (pElement->m_pNext)
						{
							pElement->m_pNext->m_pPrev = pElement->m_pPrev;
						}
					}
					pElement->m_pNext = NULL;
					pElement->m_pPrev = NULL;
					ElementCount++;
				}
				pElement = pNextElement;
			}
			if (!m_pAllocator)
			{
				delete[] m_ppElements[SegmentIndex];
				delete[] m_ppData[SegmentIndex];
			}
			m_ppElements[SegmentIndex] = NULL;
			m_ppData[SegmentIndex] = NULL;
		}
		
		//--------------------------------------------------------------------------------
		// Standard singly linked list insertion		
		CElementEntry& AllocElement()
		{
			if (m_pFree == NULL) // check if out of free elements
			{
				AllocSegment();
			}
			CElementEntry& Element = *m_pFree;
			m_pFreeElementCounts[Element.m_SegmentIndex]--; // Update the element free count
			m_pFree = m_pFree->m_pNext;
			if (m_pFree)
			{
				m_pFree->m_pPrev->m_pNext = NULL;
				m_pFree->m_pPrev = NULL; // Unlink from the free list
			}
			return Element;
		}

		//--------------------------------------------------------------------------------
		// Standard singly linked list removal
		void FreeElement(CElementEntry* pElement)
		{
			if (m_pFree == NULL)	// if empty, add as the 1st element and null terminate
			{
				m_pFree = pElement;
				pElement->m_pNext = NULL;
				pElement->m_pPrev = NULL;
			}
			else				// add at the head of the list	
			{
				pElement->m_pPrev = NULL;
				pElement->m_pNext = m_pFree;
				m_pFree->m_pPrev = pElement;
				m_pFree = pElement;
			}
			m_pFreeElementCounts[pElement->m_SegmentIndex]++; // Update the element free count
			if (m_pFreeElementCounts[pElement->m_SegmentIndex] == m_SegmentSize) // All the elements in this segment have been freed.
			{
				FreeSegment(pElement->m_SegmentIndex);
			}
		}

		private:
			CElementEntry*	m_pFree;		
			CElementEntry**	m_ppElements; // Ptrs to the allocated memory for the elements	
			DataType**		m_ppData;	
			unsigned int*	m_pFreeElementCounts;
			unsigned int	m_NumSegments;
			unsigned int	m_SegmentSize;		
			IAllocator*		m_pAllocator;			
};



//--------------------------------------------------------------------------------
// Maintains a doubly linked list of data ptrs. When new data is added to the list
// gets an available data container (CElement) from a common pool shared by
// other CElementArray<DataType>s. When data is removed, returns the data container back to
// the pool, data is allocated and maintained internally to the array
template<class DataType>
class CElementArray
{
	public:	
		
		//--------------------------------------------------------------------------------
		CElementArray(CElementArrayPool<DataType>* pPool)
		{
			m_pFirst = NULL;
			m_pElementPool = pPool;
		}

		//--------------------------------------------------------------------------------
		~CElementArray()
		{
			// nothing to free since memory will be given back once the allocator is deleted
		}

		//--------------------------------------------------------------------------------
		// Standard doubly linked list insertion	
		DataType& Add()
		{
			CElementEntry& Element = m_pElementPool->AllocElement();
			Link(Element);
			return *((DataType*)Element.m_pData);
		}

		//--------------------------------------------------------------------------------
		// Standard doubly linked list removal
		void Remove(CElementEntry& Element)
		{
			Unlink(Element);
			m_pElementPool->FreeElement(&Element);
		}

		//--------------------------------------------------------------------------------	
		CElementEntry* GetFirst() const
		{
			return m_pFirst;
		}

		//--------------------------------------------------------------------------------
		bool IsUsingPool(CElementArrayPool<DataType>& Pool) const
		{
			return (&Pool == m_pElementPool);
		}

		//--------------------------------------------------------------------------------
		// Moves an element to another ElementArray that shares the same ArrayPool directly
		void Move(CElementArray<DataType>* pDstElementArray, CElementEntry& Element)
		{
			Assert(pDstElementArray->IsUsingPool(*m_pElementPool));
			Unlink(Element);
			pDstElementArray->Link(Element);
		}

		//--------------------------------------------------------------------------------
		bool IsEmpty() const
		{
			return m_pFirst == NULL;
		}

	private:	

		//--------------------------------------------------------------------------------		
		// Links the element to the used list without removing it from the pool
		void Link(CElementEntry& Element)
		{
			// No assumptions about what Next and Prev are
			if (m_pFirst == NULL)	// empty list
			{
				Element.m_pNext = NULL;
				Element.m_pPrev = NULL;
				m_pFirst = &Element;
			}
			else					// add at the start
			{
				Element.m_pNext = m_pFirst;
				Element.m_pPrev = NULL;
				m_pFirst->m_pPrev = &Element;
				m_pFirst = &Element;
			}
		}

		//--------------------------------------------------------------------------------
		// Unlinks the element from the used list without re-adding it back to the pool
		void Unlink(CElementEntry& Element)
		{
			if (&Element == m_pFirst)	// remove at start
			{
				m_pFirst = Element.m_pNext;
				if (m_pFirst)
				{
					m_pFirst->m_pPrev = NULL;
				}
			}
			else						// remove at middle or end
			{
				if (Element.m_pNext)
				{
					Element.m_pNext->m_pPrev = Element.m_pPrev;
				}
				Element.m_pPrev->m_pNext = Element.m_pNext; // dont need to check if (Element.m_pPrev == NULL), because we are not at start
			}
		}

		CElementEntry*					m_pFirst;
		CElementArrayPool<DataType>*	m_pElementPool;
};
#endif

