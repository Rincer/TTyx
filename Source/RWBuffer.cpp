#include "stdafx.h"

#include "RWBuffer.h"

//---------------------------------------------------------------------------------------------------
CRWBuffer::CRWBuffer(unsigned char* pMemory, unsigned int Size) :	m_State(eWriting),
																	m_pMemory(pMemory),
																	m_Offset(0),
																	m_Capacity(0),
																	m_Size(Size),
																	m_Iterator(this),
																	m_IsLocked(false)
{	
}

//---------------------------------------------------------------------------------------------------
void CRWBuffer::SwapModes()
{
	Assert(m_IsLocked == false);
	if (m_State == eWriting) // going from writing to reading
	{
		m_Capacity = m_Offset;
		m_Offset = 0;
		m_State = eReading;
	}
	else // Reading to writing
	{
		m_Capacity = 0;
		m_Offset = 0;
		m_State = eWriting;
	}
}


//---------------------------------------------------------------------------------------------------
void CRWBuffer::BeginWrite(void** ppData, unsigned int Size)
{
	Assert(!m_IsLocked); 
	m_IsLocked = true;
	Assert(m_State == eWriting);
	Assert(m_Offset + Size <= m_Size);
	*ppData = &m_pMemory[m_Offset];
}

//---------------------------------------------------------------------------------------------------
void CRWBuffer::EndWrite(unsigned int Size)
{
	Assert(m_IsLocked);
	Assert(m_State == eWriting);
	Assert(m_Offset + Size <= m_Size);
	m_Offset += Size;
	m_IsLocked = false;
}

//---------------------------------------------------------------------------------------------------
void CRWBuffer::BeginRead(void** ppData, unsigned int Size)
{
	Assert(!m_IsLocked);
	m_IsLocked = true;
	Assert(m_State == eReading);
	Assert(m_Offset + Size <= m_Capacity);
	*ppData = &m_pMemory[m_Offset];
}

//---------------------------------------------------------------------------------------------------
void CRWBuffer::EndRead(unsigned int Size)
{
	Assert(m_IsLocked);
	Assert(m_State == eReading);
	Assert(m_Offset + Size <= m_Capacity);
	m_Offset += Size;
	m_IsLocked = false;
}

//---------------------------------------------------------------------------------------------------
bool CRWBuffer::IsEmpty()
{
	Assert(m_State == eWriting); // If nothing was written there is no need to pass it to the consumer, so only test in write mode
	return (m_Offset == 0);
}
