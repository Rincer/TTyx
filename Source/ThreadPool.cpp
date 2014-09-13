#include "stdafx.h"
#include "Thread.h"
#include "Math.h"
#include "MemoryManager.h"
#include "ElementList.h"
#include "Time.h"
#include "Event.h"
#include "TimeLine.h"

#include "ThreadPool.h"

class CJobEvent
{
	public:
		CThreadPool::eEventType	m_Event;
		unsigned int m_JobId;
		CJobSystem::eJobType m_JobType;
		unsigned long long m_TimeStamp;
};

#if THREADPOOL_EVENT_TRACKING		
static const unsigned int scMaxEventsPerFrame = 1024;
static CJobEvent sJobEvents[sc_MaxProcessAffinity][scMaxEventsPerFrame];
static unsigned int sJobEventCounters[sc_MaxProcessAffinity];
#endif


CTimeLine::CEventDesc CThreadPool::m_ProfilingEvents[CJobSystem::eMaxJobType + 1] =
{
	{
		CJobSystem::eJobLoadTexture,
		CColor(128, 0, 0, 255),
		"JobLoadTexture",
	},
	{		
		CJobSystem::eJobLoadXFile,
		CColor(128, 0, 255, 0),
		"JobLoadXFile",
	},
	{		
		CJobSystem::eJobCreateDrawPrimitive,
		CColor(128, 0, 255, 255),
		"JobCreateDrawPrimitive",
	},		
	{
		CJobSystem::eJobLoadMaterialLibrary,
		CColor(128, 255, 0, 0),
		"JobLoadMaterialLibrary",
	},		
	{
		CJobSystem::eJobLoadObjFile,
		CColor(128, 255, 0, 255),
		"JobLoadObjFile",
	},		
	{
		CJobSystem::eJobCreateShader,
		CColor(128, 255, 255, 0),
		"JobCreateShader",
	},		
	{
		CJobSystem::eJobCreateMaterial,			
		CColor(192, 0, 0, 128),
		"JobCreateMaterial",
	},		
	{
		CJobSystem::eJobProcessRenderCommands,
		CColor(192, 255, 128, 0),
		"JobProcessRenderCommands",
	},
	{
		CJobSystem::eJobUtilityDraw,
		CColor(192, 128, 0, 0),
		"JobUtilityDraw",
	},
	{
		0,
		CColor(0, 0, 0, 0),
		NULL,
	} // Null terminated	
};


//----------------------------------------------------------------------------
CThreadPool::CThreadPool(CJobSystem* pJobSystem, CHeapAllocator* pAllocator, unsigned int ThreadsPerCore, CTimeLine* pTimeLine) : m_pJobSystem(pJobSystem),
																																  m_pTimeLine(pTimeLine)
{
#if THREADPOOL_EVENT_TRACKING		
	memset(sJobEventCounters, 0, sizeof(sJobEventCounters));
#endif
	m_Terminate = false;
	unsigned long ProcessAffinity = 0, SystemAffinity = 0;
	unsigned int Res = GetProcessAffinityMask(GetCurrentProcess(), &ProcessAffinity, &SystemAffinity);
	Assert(Res != 0);
	m_NumThreads = CountOnes(ProcessAffinity) * ThreadsPerCore;
		
	m_pWorkerThreads = PlacementNew<CThread>(pAllocator->Alloc(m_NumThreads * sizeof(CThread)), m_NumThreads);
	m_pThreadPoolContexts = (CThreadPoolContext*)pAllocator->Alloc(m_NumThreads * sizeof(CThreadPoolContext));
	unsigned int HWThreadAffinity = 1;
	for(unsigned int HWThreadIndex = 0; HWThreadIndex < sc_MaxProcessAffinity; HWThreadIndex++)
	{
		if(ProcessAffinity & HWThreadAffinity)
		{
			for(unsigned int ThreadIndex = 0; ThreadIndex < ThreadsPerCore; ThreadIndex++)
			{
				char Name[256];
				sprintf_s(Name, 255, "Pool_%d_%d", HWThreadIndex, ThreadIndex);
				m_pThreadPoolContexts[HWThreadIndex * ThreadsPerCore + ThreadIndex].m_pThreadPool = this;
				m_pThreadPoolContexts[HWThreadIndex * ThreadsPerCore + ThreadIndex].m_ThreadID = HWThreadIndex * ThreadsPerCore + ThreadIndex;
				m_pWorkerThreads[HWThreadIndex * ThreadsPerCore + ThreadIndex].Startup(JobProcessLoop, &m_pThreadPoolContexts[HWThreadIndex * ThreadsPerCore + ThreadIndex], Name, m_ProfilingEvents, m_pTimeLine);
				m_pWorkerThreads[HWThreadIndex * ThreadsPerCore + ThreadIndex].SetAffinity(HWThreadAffinity);
			}
		}		
		HWThreadAffinity <<= 1;
	}	
}

//----------------------------------------------------------------------------
CThreadPool::~CThreadPool()
{
	for(unsigned int ThreadIndex = 0; ThreadIndex < m_NumThreads; ThreadIndex++)
	{
		m_pWorkerThreads[ThreadIndex].Shutdown();
		m_pWorkerThreads[ThreadIndex].~CThread();
	}	
}


//-------------------------------------------------------------------------------------------
void CThreadPool::StartJobs()
{
	m_StartJobMutex.Acquire(); // Can only start from one thread
	unsigned int ThreadIndex;
	for(ThreadIndex = 0; ThreadIndex < m_NumThreads; ThreadIndex++)
	{
		if(m_pWorkerThreads[ThreadIndex].IsWorkComplete()) // Only kick it off if its not active already
		{
			m_pWorkerThreads[ThreadIndex].SetWorkReady(); // Can this still fail?
			break;
		}
	}
	m_StartJobMutex.Release();
}

//-------------------------------------------------------------------------------------------
bool CThreadPool::ExecuteJob(unsigned int ThreadID)
{
	CJobSystem::CJob* pJob = m_pJobSystem->GetJob();
	if (pJob)
	{					
		CTimeLine::CScopedEvent Event(pJob->GetType(), m_pTimeLine);
#if THREADPOOL_EVENT_TRACKING		
		AddEvent(eJobStart, pJob->GetId(), pJob->GetType(), ThreadID);
#endif
		pJob->Execute(ThreadID);
		pJob->OnComplete();
#if THREADPOOL_EVENT_TRACKING		
		AddEvent(eJobEnd, pJob->GetId(), pJob->GetType(), ThreadID);				
#endif
		return false; // There were jobs to execute, queue wasnt empty
	}
	return true; // All queues were empty
}

//-------------------------------------------------------------------------------------------
void CThreadPool::JobProcessLoop(void* pContext)
{
	CThreadPoolContext* pThreadPoolContext = (CThreadPoolContext*)pContext;
	bool IsQueueEmpty;
	// Try to do as many jobs as we can
	do
	{
		IsQueueEmpty = pThreadPoolContext->m_pThreadPool->ExecuteJob(pThreadPoolContext->m_ThreadID);
	} while(!IsQueueEmpty); // While there is work to do
}

#if THREADPOOL_EVENT_TRACKING		
//-------------------------------------------------------------------------------------------
void CThreadPool::AddEvent(eEventType Type, unsigned int JobId, CJobSystem::eJobType JobType, unsigned int ThreadID)
{
	sJobEvents[ThreadID][sJobEventCounters[ThreadID]].m_Event = Type;
	sJobEvents[ThreadID][sJobEventCounters[ThreadID]].m_JobId = JobId;
	sJobEvents[ThreadID][sJobEventCounters[ThreadID]].m_JobType = JobType;
	sJobEvents[ThreadID][sJobEventCounters[ThreadID]].m_TimeStamp = CTime::RDTSC();	
	sJobEventCounters[ThreadID] = (sJobEventCounters[ThreadID] + 1) % scMaxEventsPerFrame;
}
#endif
