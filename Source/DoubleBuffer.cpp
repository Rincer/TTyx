#include "stdafx.h"

#include "DoubleBuffer.h"

//--------------------------------------------------------------------
void CDoubleBuffer::Initialize(void* pMemory, unsigned int Size)
{
	m_Size = Size / 2;
	m_State[0] = eNone;
	m_State[1] = eNone;	
	m_ReadOffset = 0;
	m_WriteOffset = 0;			
	m_WriteIndex = 0;
	m_EndData = 0;
	m_ReadIndex = 1;
	m_pBuffers[0] = (unsigned char*)pMemory;
	m_pBuffers[1] = m_pBuffers[0] + m_Size;	
}
		
//--------------------------------------------------------------------
void CDoubleBuffer::BeginRead(unsigned int BufferIndex, unsigned char** ppData, unsigned int BytesToRead)
{
	Assert(m_State[m_ReadIndex] == eNone);
	Assert(m_EndData - m_ReadOffset >= BytesToRead); // check for enough data
	*ppData = &m_pBuffers[m_ReadIndex][m_ReadOffset];
	m_State[BufferIndex] = eReading;		
}
				
//--------------------------------------------------------------------
void CDoubleBuffer::EndRead(unsigned int BufferIndex, unsigned int BytesRead)
{
	Assert(m_State[m_ReadIndex] == eReading);
	m_ReadOffset += BytesRead;
	m_State[BufferIndex] = eNone;
}
				
//--------------------------------------------------------------------		
void CDoubleBuffer::BeginWrite(unsigned int BufferIndex, unsigned char** ppData, unsigned int Size)
{
	Assert(m_State[m_WriteIndex] == eNone);
	Assert(m_WriteOffset + Size < m_Size); // Out of space
	*ppData = &m_pBuffers[m_WriteIndex][m_WriteOffset];
	m_WriteOffset += Size;
	m_State[m_WriteIndex] = eWriting;
}
				
//--------------------------------------------------------------------				
void CDoubleBuffer::EndWrite(unsigned int BufferIndex)
{
	Assert(m_State[m_WriteIndex] == eWriting);
	m_State[m_WriteIndex] = eNone;
}

//--------------------------------------------------------------------				
unsigned int CDoubleBuffer::ReadBufferSize()
{
	return m_EndData;
}

//--------------------------------------------------------------------				
unsigned int CDoubleBuffer::WriteBufferSize()
{
	return m_WriteOffset;
}

//--------------------------------------------------------------------						
void CDoubleBuffer::SwapBuffers()
{
	m_EndData = m_WriteOffset;
	m_WriteOffset = 0;
	m_ReadOffset = 0;
	m_ReadIndex = m_WriteIndex;
	m_WriteIndex = 1 - m_WriteIndex;
}