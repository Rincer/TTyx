#ifndef _THREAD_H_
#define _THREAD_H_

#include "TimeLine.h"
#include "Event.h"

class CThread
{
	public:
		typedef void(*ThreadFunc)(void* pUserArgs);	
		CThread();
		~CThread();				
		void Startup(ThreadFunc pUserFunc, void* pUserArgs, const char* pName, CTimeLine::CEventDesc* pEventDesc, CTimeLine* pTimeLine);
		void SetAffinity(unsigned int AffinityMask);
		void Shutdown();		
		void SetWorkReady();
		bool WaitForWorkComplete();
		bool IsWorkReady();
		bool IsWorkComplete();
		unsigned int GetID();	
		static void UpdateThreadID();
		static long GetThreadID();
		
	private:				
		class SThreadArgs
		{	
			public:
				SThreadArgs()
				{
					m_pUserArgs = NULL;
					m_pUserFunc = NULL;
					m_pThread = NULL;	
					m_Name[0] = '\0';
					m_pEventDesc = NULL;					
					m_pTimeLine = NULL;
				}
				void*		m_pUserArgs;
				ThreadFunc	m_pUserFunc;
				CThread*	m_pThread;	
				char		m_Name[256];
				const CTimeLine::CEventDesc* m_pEventDesc;	
				CTimeLine*  m_pTimeLine;
		};
		bool WaitForWorkReady();
		void ResetWorkReady();		
		void SetWorkComplete();
		void SetThreadExit();		

		static DWORD WINAPI DispatchThreadFunction(LPVOID lpParam);	
		SThreadArgs     m_ThreadArgs;				
		CEvent			m_WorkReady;
		CEvent			m_WorkComplete;	
		CEvent			m_ThreadExit;			
		HANDLE			m_ThreadHandle;
		unsigned long	m_ThreadId;
		volatile bool	m_Terminate;		
};

#endif
