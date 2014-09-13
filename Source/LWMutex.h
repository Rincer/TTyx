#ifndef _LWMUTEX_H_
#define _LWMUTEX_H_

// Light weight mutex class using SRWLOCK
class CLWMutex
{
	public:
		CLWMutex();
		
		void Acquire();
		void Release();
				
	private:
		enum eState
		{
			eAcquired,
			eReleased
		};

		eState		m_State;
		RTL_SRWLOCK m_Lock;
};

#endif