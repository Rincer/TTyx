#ifndef _EVENT_H_
#define _EVENT_H_

// Single synchronization event
class CEvent
{
	public:
		CEvent(bool Signalled);
		~CEvent();
		void Set();
		void Reset();
		bool Wait(unsigned int TimeMs);
		
	private:
		enum eState
		{
			eSet,
			eReset
		};

		HANDLE m_Handle;	
		eState m_State;

};

// Array of synchronization events
class CEventArray
{
	public:
		CEventArray(bool Signalled, unsigned int NumEvents, HANDLE* pHandles);
		~CEventArray();
		
		// Sets event specified by index
		void Set(unsigned int EventIndex);
		
		// Resets event specified by index
		void Reset(unsigned int EventIndex);
		
		// Waits for all events to be signalled
		bool WaitAll(unsigned int TimeMs);

		// Waits for a specifed number events to be signalled
		bool WaitMultiple(unsigned int NumEvents, unsigned int TimeMs);

		// Waits for event specified by index
		bool Wait(unsigned int EventIndex, unsigned int TimeMs);

		
	private:
		unsigned int m_NumEvents;
		HANDLE*		 m_pHandles;	
		unsigned int m_States[64];
};


#endif