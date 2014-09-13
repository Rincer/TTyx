#include "stdafx.h"
#include "Allocator.h"

#include "ElementList.h"

//----------------------------------------------------------
ILinkable::ILinkable(CElement* pElement) : m_pElement(pElement)
{
}

//----------------------------------------------------------
CElement* ILinkable::GetElement()
{
	return m_pElement;
}


//--------------------------------------------------------------------------------
CElementPool::CElementPool(unsigned int NumNodes, IAllocator* pAllocator )
{
	// allocate in place
	CElement* pElement = new (pAllocator->Alloc(NumNodes * sizeof(CElement))) CElement[NumNodes]; 
	
	// create a singly linked list of free elements. dont care about m_pPrev since we add and delete from the head of the free list.
	for(unsigned int i = 1; i < NumNodes; i++)
	{
		pElement[i - 1].m_pNext = &pElement[i];
	}		
	m_pFree = pElement;	
}

//--------------------------------------------------------------------------------
CElementPool::~CElementPool()
{
	// nothing to free since memory will be given back once the stack allocater is deleted
}

//--------------------------------------------------------------------------------
// Standard singly linked list removal
CElement* CElementPool::AllocElement()
{
	Assert(m_pFree != NULL); // check if out of free elements
	CElement* pElement = m_pFree;
	m_pFree = m_pFree->m_pNext;
	return pElement;
}

//--------------------------------------------------------------------------------
// Standard singly linked list insertion
void CElementPool::FreeElement(CElement* pElement)
{
	
	if(m_pFree == NULL)	// if empty, add as the 1st element and null terminate
	{
		m_pFree = pElement;
		pElement->m_pNext = NULL;
	}
	else				// add at the head of the list	
	{
		pElement->m_pNext = m_pFree;
		m_pFree = pElement;
	}
}

//--------------------------------------------------------------------------------
CElementList::CElementList(CElementPool* pPool)
{
	m_pFirst = NULL;
	m_pElementPool = pPool;
}

//--------------------------------------------------------------------------------
CElementList::~CElementList()
{
	// nothing to free since memory will be given back once the stack allocator is deleted
}

//--------------------------------------------------------------------------------
// Standard doubly linked list insertion
void CElementList::Add(const void* pData)
{
	CElement* pNewElement = m_pElementPool->AllocElement();
	pNewElement->m_pData = pData;
	if(m_pFirst == NULL)	// empty list
	{
		pNewElement->m_pNext = NULL;
		m_pFirst = pNewElement;
	}
	else					// add at the start
	{
		pNewElement->m_pNext = m_pFirst;
		m_pFirst->m_pPrev = pNewElement;
	}
	pNewElement->m_pPrev = NULL;						
	m_pFirst = pNewElement;	
}

//--------------------------------------------------------------------------------
// Standard doubly linked list removal
void CElementList::Remove(CElement& Element)
{
	if(&Element == m_pFirst)	// remove at start
	{
		m_pFirst = Element.m_pNext;
		if(m_pFirst)
		{
			m_pFirst->m_pPrev = NULL;
		}
	}
	else						// remove at middle or end
	{
		if(Element.m_pNext)
		{
			Element.m_pNext->m_pPrev = Element.m_pPrev;
		}
		Element.m_pPrev->m_pNext = Element.m_pNext; // dont need to check if (pElement->m_pPrev == NULL), because we are not at start
	}
	m_pElementPool->FreeElement(&Element);
}

//--------------------------------------------------------------------------------
CElement* CElementList::GetFirst() const
{
	return m_pFirst;
}

//--------------------------------------------------------------------------------
IAccessible::IAccessible()
{
}

//--------------------------------------------------------------------------------
void IAccessible::RegisterElement(const CElementEntry* pElement)
{
	m_pElementEntry = pElement;
}

//--------------------------------------------------------------------------------
const CElementEntry* IAccessible::GetElement()
{
	return m_pElementEntry; 
}




