#ifndef _SEGMENTEDBUFFER_H_
#define _SEGMENTEDBUFFER_H_

#include "ElementList.h"
#include "LWMutex.h"
#include "List.h"
#include "RWBuffer.h"

class IAllocator;

// Memory buffer that handles data allocation in segments of preset size, dataflow is:
// Producer gets buffer from write list, writes to it, moves it to the read list
// Consumer gets buffer from read list processes it, moves it back to write list
// If either list is empty NULL is returned
class CSegmentedBuffer
{
	public:
		CSegmentedBuffer();
		~CSegmentedBuffer();

		void Initialize(unsigned int NumSegments, unsigned int SegmentSize, IAllocator* pAllocator);

		// Gets the next available read buffer
		CRWBuffer* GetRead();

		// Gets the next available write buffer
		CRWBuffer* GetWrite();

		// Moves a buffer to read list
		void PutRead(CRWBuffer* pBuffer);

		// Moves a buffer to write list
		void PutWrite(CRWBuffer* pBuffer);


	private:
		CList<CRWBuffer>	m_WriteList;
		CList<CRWBuffer>	m_ReadList;
		CLWMutex			m_Mutex;
		unsigned char*		m_pBufferMemory;
		IAllocator*			m_pAllocator;
};


#endif