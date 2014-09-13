#include "stdafx.h"
#include "Color.h"
#include "Macros.h"

#include "MemoryStream.h"

//---------------------------------------------------------------------------------------------------
CMemoryStreamBase::CMemoryStreamBase(unsigned int Offset, unsigned int Size):	m_Offset(Offset),			
																				m_Size(Size)

{
}

//---------------------------------------------------------------------------------------------------
void CMemoryStreamBase::Align(unsigned int Alignment)
{
	m_Offset = ALIGN(m_Offset, Alignment);
}

//---------------------------------------------------------------------------------------------------
unsigned int CMemoryStreamBase::GetOffset()
{
	return m_Offset;
}

//---------------------------------------------------------------------------------------------------
CMemoryStreamReader::CMemoryStreamReader(const void* pBuffer, unsigned int Offset, unsigned int Size):	CMemoryStreamBase(Offset, Size),
																										m_pBuffer((const unsigned char*)pBuffer)
{
}

//---------------------------------------------------------------------------------------------------
char* CMemoryStreamReader::ReadStringInPlace()
{
	char* pRet = (char*)&m_pBuffer[m_Offset];
	unsigned int Offset = m_Offset;
	while(m_pBuffer[Offset] != 0)
	{
		Offset++;
	}
	Offset++;
	
	Assert(Offset < m_Size);
	m_Offset = Offset;
	return pRet;
}

//---------------------------------------------------------------------------------------------------
CMemoryStreamWriter::CMemoryStreamWriter(void* pBuffer, unsigned int Offset, unsigned int Size):CMemoryStreamBase(Offset, Size),
																								m_pBuffer((unsigned char*)pBuffer)
{
}

//---------------------------------------------------------------------------------------------------
void CMemoryStreamWriter::WriteString(const char* pString)
{
	Assert(m_Offset + strlen(pString) + 1 <= m_Size); // +1 for null terminated string
	strcpy_s((char*)&m_pBuffer[m_Offset], strlen(pString) + 1, pString);
	m_Offset += (strlen(pString) + 1);
}

