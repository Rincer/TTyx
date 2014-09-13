#ifndef _DOUBLEBUFFER_H_
#define _DOUBLEBUFFER_H_

class CDoubleBuffer
{
	public:
		
		void Initialize(void* pMemory, unsigned int Size);
		
		// Returns the pointer to start of read offset, passing in the minimum number of bytes we expect to read to check if there is enough data
		// If there isnt enough data, returns NULL
		void BeginRead(unsigned int BufferIndex, unsigned char** ppData, unsigned int BytesToRead);
				
		// Advances the read offset
		void EndRead(unsigned int BufferIndex, unsigned int BytesRead);
		
		// Reserves Size bytes in the buffer, returns the ptr to current write offset and advances the write offset
		void BeginWrite(unsigned int BufferIndex, unsigned char** ppData, unsigned int Size);
		
		// Ends the write
		void EndWrite(unsigned int BufferIndex);
		
		//
		unsigned int ReadBufferSize();

		unsigned int WriteBufferSize();
				
		void SwapBuffers();
		
		
	private:
		enum eState
		{
			eNone,
			eReading,
			eWriting
		};
		
		eState			m_State[2];
		unsigned char*	m_pBuffers[2];

		unsigned int	m_Size;
		unsigned int	m_ReadOffset;
		unsigned int	m_EndData;
		unsigned int	m_WriteOffset;							

		unsigned int	m_WriteIndex;
		unsigned int	m_ReadIndex;
		
};

#endif