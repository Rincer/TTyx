#include "stdafx.h"

#include "Event.h"

//-----------------------------------------------------------------------------------------------------
CEvent::CEvent(bool Signalled)
{
	m_Handle = CreateEvent( 
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        Signalled,			// initial state 
        NULL
        ); 
	m_State = Signalled ? eSet : eReset;
}

//-----------------------------------------------------------------------------------------------------
void CEvent::Set()
{
	Assert(m_State == eReset);
	m_State = eSet;
	SetEvent(m_Handle);
}

//-----------------------------------------------------------------------------------------------------
void CEvent::Reset()
{
	Assert(m_State == eSet);
	m_State = eReset;
	ResetEvent(m_Handle);

}

//-----------------------------------------------------------------------------------------------------
bool CEvent::Wait(unsigned int TimeMs)
{
	DWORD Res = WaitForSingleObject(m_Handle, TimeMs);
	return (Res == WAIT_OBJECT_0);
}

//-----------------------------------------------------------------------------------------------------
CEvent::~CEvent()
{
	CloseHandle(m_Handle);
}

//-----------------------------------------------------------------------------------------------------
CEventArray::CEventArray(bool Signalled, unsigned int NumEvents, HANDLE* pHandles)
{
	m_NumEvents = NumEvents;
	m_pHandles = pHandles;
	
	for(unsigned int EventIndex = 0; EventIndex < m_NumEvents; EventIndex++)
	{
		m_pHandles[EventIndex] = CreateEvent( 
			NULL,               // default security attributes
			TRUE,               // manual-reset event
			Signalled,			// initial state 
			NULL
			); 
	}
	memset(m_States, 0, sizeof(m_States));
}

//-----------------------------------------------------------------------------------------------------
void CEventArray::Set(unsigned int EventIndex)
{
	m_States[EventIndex] = 1;
	SetEvent(m_pHandles[EventIndex]);
}

//-----------------------------------------------------------------------------------------------------
void CEventArray::Reset(unsigned int EventIndex)
{
	m_States[EventIndex] = 2;
	ResetEvent(m_pHandles[EventIndex]);
}

//-----------------------------------------------------------------------------------------------------
bool CEventArray::WaitAll(unsigned int TimeMs)
{
	DWORD Res = WaitForMultipleObjects(m_NumEvents, m_pHandles, TRUE, TimeMs);
	return (Res == (WAIT_OBJECT_0 + m_NumEvents - 1));
}

//-----------------------------------------------------------------------------------------------------
bool CEventArray::WaitMultiple(unsigned int NumEvents, unsigned int TimeMs)
{
	DWORD Res = WaitForMultipleObjects(NumEvents, m_pHandles, TRUE, TimeMs);
	return (Res == (WAIT_OBJECT_0 + NumEvents - 1));
}

//-----------------------------------------------------------------------------------------------------
bool CEventArray::Wait(unsigned int EventIndex, unsigned int TimeMs)
{
	DWORD Res = WaitForSingleObject(m_pHandles[EventIndex], TimeMs);
	return (Res == WAIT_OBJECT_0);
}

//-----------------------------------------------------------------------------------------------------
CEventArray::~CEventArray()
{
	for(unsigned int EventIndex = 0; EventIndex < m_NumEvents; EventIndex++)
	{
		CloseHandle(m_pHandles[EventIndex]);
	}
}
