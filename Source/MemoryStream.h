#ifndef _MEMORYSTREAM_H_
#define _MEMORYSTREAM_H_

#include "Color.h"

// Generic serialization classes
// Not for high performance


//---------------------------------------------------------------------------------------------------
class CMemoryStreamBase
{
	public:
		CMemoryStreamBase(unsigned int Offset, unsigned int Size);
		void Align(unsigned int Alignment);
		unsigned int GetOffset();
	protected:
		unsigned int	m_Offset;			
		unsigned int	m_Size;			
		
};

//---------------------------------------------------------------------------------------------------
class CMemoryStreamReader : public CMemoryStreamBase
{
	public:
		CMemoryStreamReader(const void* pBuffer, unsigned int Offset, unsigned int Size);
		
		template<typename Type>
		Type Read()
		{
			Assert(m_Offset + sizeof(Type) <= m_Size);
			Type Ret;
			unsigned char* pDst = (unsigned char*)&Ret;
			const unsigned char* pSrc = &m_pBuffer[m_Offset];
			for(unsigned int i = 0; i < sizeof(Type); i++)
			{
				*pDst++ = *pSrc++;
			}
			m_Offset += sizeof(Type);
			return Ret;			
		}
		
		//---------------------------------------------------------------------------------------------------
		// advances the offset to next character after 0 terminator and returns ptr of current offset
		char* ReadStringInPlace();
		
	private:
		const unsigned char*	m_pBuffer;
};


//---------------------------------------------------------------------------------------------------
class CMemoryStreamWriter : public CMemoryStreamBase
{
	public:
		CMemoryStreamWriter(void* pBuffer, unsigned int Offset, unsigned int Size);
		
		//---------------------------------------------------------------------------------------------------
		template<typename Type>
		void Write(Type Val)
		{
			Assert(m_Offset + sizeof(Type) <= m_Size);
			unsigned char* pDst = &m_pBuffer[m_Offset];
			unsigned char* pSrc = (unsigned char*)&Val;
			for(unsigned int i = 0; i < sizeof(Type); i++)
			{
				*pDst++ = *pSrc++;
			}
			m_Offset += sizeof(Type);
		}

		//---------------------------------------------------------------------------------------------------
		// ptr version
		template<typename Type>
		void Write(Type* Val)
		{
			Assert(m_Offset + sizeof(Type) <= m_Size);
			unsigned char* pDst = &m_pBuffer[m_Offset];
			unsigned char* pSrc = (unsigned char*)Val;
			for(unsigned int i = 0; i < sizeof(Type); i++)
			{
				*pDst++ = *pSrc++;
			}
			m_Offset += sizeof(Type);
		}

				
		void WriteString(const char* pString);				
				
	private:
		unsigned char*	m_pBuffer;
};

#endif