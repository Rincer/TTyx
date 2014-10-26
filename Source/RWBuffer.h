#ifndef _RWBUFFER_H_
#define _RWBUFFER_H_

#include "List.h"

// Read write buffer
class CRWBuffer
{
	public:

		enum eState
		{
			eReading,
			eWriting
		};

		CRWBuffer(unsigned char* pMemory, unsigned int Size);
		void SwapModes();
		void BeginWrite(void** ppData, unsigned int Size);	// Get memory for writing checking if enough space is available
		void EndWrite(unsigned int Size);					// Adjust the offset with the actual amount of data written
		void BeginRead(void** ppData, unsigned int Size);	// Get memory for reading checking if enough data is available
		void EndRead(unsigned int Size);					// Adjust the offset with the actual amount of data read
		bool IsEmpty();
		eState GetState() { return m_State; }	

	private:

		eState			m_State;
		unsigned char*	m_pMemory;
		unsigned int	m_Offset;
		unsigned int	m_Capacity;
		unsigned int	m_Size;
		bool			m_IsLocked;
};


#endif
