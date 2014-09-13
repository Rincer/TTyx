#include "stdafx.h"
#include "LWMutex.h"

//----------------------------------------------------------------------------------------------------
CLWMutex::CLWMutex()
{
	InitializeSRWLock(&m_Lock);
	m_State = eReleased;
}

//----------------------------------------------------------------------------------------------------
void CLWMutex::Acquire()
{
	AcquireSRWLockExclusive(&m_Lock);
	m_State = eAcquired;
}

//----------------------------------------------------------------------------------------------------
void CLWMutex::Release()
{
	Assert(m_State == eAcquired); // Make sure the state is acquired since releasing an unacquired SRWLOCK will corrupt its internal state
	m_State = eReleased;
	ReleaseSRWLockExclusive(&m_Lock);
}
		
