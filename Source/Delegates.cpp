#include "stdafx.h"
#include "HeapAllocator.h"

#include "Delegates.h"

//----------------------------------------------------------------------------------
bool IDelegate::Execute()
{
	bool Res = ExecuteDerived();
	if(Res)	// Derived delegate class will do something meaningful here, if succeeded delete ourselves
	{
		CDelegates::Instance().ReleaseDelegate(this);	// Delete the data that we executed on
	}
	return Res;
}

//----------------------------------------------------------------------------------
CDelegates::CDelegates(unsigned int DelegatesMemory)
{
	m_pAllocator = new CHeapAllocator(DelegatesMemory, false);
}

//----------------------------------------------------------------------------------
CDelegates::~CDelegates()
{
	delete m_pAllocator;
}
		
//----------------------------------------------------------------------------------		
IDelegate* CDelegates::CreateDelegate(unsigned int Size)
{
	return (IDelegate*)m_pAllocator->Alloc(Size);
}

//----------------------------------------------------------------------------------
void CDelegates::ReleaseDelegate(IDelegate* pDelegate)
{
	m_pAllocator->Free(pDelegate);
}
