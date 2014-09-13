#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include "LWMutex.h"

//----------------------------------------------------------------------------------------------
// Ring buffer class. 
class CRingBuffer
{
	public:
		// User callback function to handle wrap
		class CRingBufferCallback
		{
			public:
				virtual void Execute(CRingBuffer* pRingBuffer, unsigned int BytesBeforeWrap) = 0;
		};
	
		
		CRingBuffer();
		CRingBuffer(unsigned int Size, CRingBufferCallback* pRingBufferCallback);
		CRingBuffer(unsigned char* pBuffer, unsigned int Size, CRingBufferCallback* pRingBufferCallback);
		~CRingBuffer();
		
		void Initialize(unsigned char* pBuffer, unsigned int Size, CRingBufferCallback* pRingBufferCallback);
		//----------------------------------------------------------------------------------------------------
		// This group of member functions is used when writing and reading data from the ring buffer memory in place. Processing of the data
		// should be fast so that other potential users of the ring buffer dont get blocked on Read or Write mutexes. Every single piece of data
		// needs to be contiguous and m_pHandleWrapCB should handle the wrapping during writes.
		
		// reserves Size bytes in the buffer, returns the ptr to current write offset and advances the write offset, blocking call
		void BeginWrite(unsigned char** ppData, unsigned int Size);

		// Releases the Write mutex
		void EndWrite(unsigned int Size);
		
		// gets a lock on the read mutex and returns the ptr to current read offset in the buffer, blocking call
		void BeginRead(unsigned char** ppData, unsigned int BytesToRead);

		// advances the read offset and releases the read mutex
		void EndRead(unsigned int BytesRead);


		//----------------------------------------------------------------------------------------------------
		// This group of member functions copies data to/from an external buffer from/to the ring buffer memory, in contrast to the
		// member functions above this is useful when processing of the data takes a long time so in order not to block
		// other users of the ringbuffer it is better to work on a local copy of the data and release the Read/Write mutexes
		// tries to write Size bytes to the buffer and returns true or false depending on success. Buffer wrap around is handled internally
		// for both read and write, since it gets copied to/from external buffer the data doesn't have to be contiguous in ring buffer memory.
		bool TryWrite(unsigned char* pData, unsigned int Size);

		// blocking write
		void Write(unsigned char* pData, unsigned int Size);

		// tries to read Size bytes from the buffer and returns true or false depending on success
		bool TryRead(unsigned char* pData, unsigned int Size);

		// blocking read
		void Read(unsigned char* pData, unsigned int Size);

		// Returns the amount of unread bytes in the buffer
		unsigned int UnreadBytes();

	
	private:		
		// returns the current write address and advances the write offset by Size, should only be called from m_pHandleWrapCB
		void ReserveUnsafe(unsigned char** ppData, unsigned int Size);
	
		// tries to reserve Size bytes in the buffer and sets ppData to NULL or a valid address depending on success
		void TryReserve(unsigned char** ppData, unsigned int Size);

		CLWMutex				m_ReadMutex;
		CLWMutex				m_WriteMutex;
		volatile unsigned int	m_ReadOffset;
		volatile unsigned int	m_WriteOffset;				
		unsigned int			m_Size;	
		unsigned int			m_ReservedSize;
		unsigned char*			m_pBuffer;
		CRingBufferCallback*	m_pRingBufferCallback;
		
		friend class CRendererRingBufferCallback;
};
#endif