#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "JobSystem.h"
#include "LWMutex.h"
#include "TimeLine.h"

class CHeapAllocator;
class CThread;
class CEventArray;

static const unsigned int sc_MaxProcessAffinity = 32; // Hw threads	

// Enable to track all events on a thread (debugging)
//#define THREADPOOL_EVENT_TRACKING 1 

class CThreadPool
{
	public:
		enum eEventType
		{
			eJobStart,
			eJobEnd,
			eJobYield,
			eJobResume
		};
	
	
		CThreadPool(CJobSystem*	pJobSystem, CHeapAllocator* pAllocator, unsigned int ThreadsPerCore, CTimeLine*	pTimeLine);
		~CThreadPool();	
		void StartJobs();
		bool ExecuteJob(unsigned int ThreadID); // Tries to get and execute the next job from the work queue, returns false if queue is empty

#if THREADPOOL_EVENT_TRACKING		
		void AddEvent(eEventType Type, unsigned int JobId, CJobSystem::eJobType JobType, unsigned int ThreadID);
#endif

	private:
		class CThreadPoolContext
		{
			public:
				CThreadPool* m_pThreadPool;
				unsigned int m_ThreadID;
				
		};


		static void JobProcessLoop(void* pContext);

		unsigned int			m_NumThreads;
		CThread*				m_pWorkerThreads;
		CThreadPoolContext*		m_pThreadPoolContexts;
		CJobSystem*				m_pJobSystem;
		CTimeLine*				m_pTimeLine;
		CLWMutex				m_StartJobMutex;
		volatile bool			m_Terminate;
				
// Profiling
		static CTimeLine::CEventDesc m_ProfilingEvents[CJobSystem::eMaxJobType + 1];
		

};
#endif