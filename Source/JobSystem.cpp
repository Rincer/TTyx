#include "stdafx.h"
#include "HeapAllocator.h"
#include "MemoryManager.h"
#include "ThreadPool.h"
#include "TimeLine.h"

#include "JobSystem.h"

static const unsigned int sc_JobSystemMemory = 128 * 1024; // Memory for creating jobs

//-------------------------------------------------------------------------------------------
CJobSystem::CJob::CJob(unsigned int Id, eJobPriority Priority, CJobSystem*	pJobSystem) :m_Type(eJobInvalid),
																						 m_Id(Id),
																						 m_pJobSystem(pJobSystem),
																						 m_Priority(Priority),
																						 m_Iterator(this)

{ 
}


//-------------------------------------------------------------------------------------------
CJobSystem::CJob::CJob(eJobType Type) : m_Type(Type),
										m_Iterator(this)
{ 
}

//-------------------------------------------------------------------------------------------
CJobSystem::CJob::~CJob()
{ 
}



//-------------------------------------------------------------------------------------------
void CJobSystem::CJob::OnComplete()
{
	m_pJobSystem->ReleaseJob(this);
}

//-------------------------------------------------------------------------------------------
CJobSystem::CJobSystem(CTimeLine** ppTimeLine) : m_TotalJobs(0),
												m_rTimeLine(ppTimeLine)
{
	m_pAllocator = new CHeapAllocator(sc_JobSystemMemory, false);
	Startup();
}

//-------------------------------------------------------------------------------------------
CJobSystem::~CJobSystem()
{
	Shutdown();
	delete m_pAllocator;
}

//-------------------------------------------------------------------------------------------
void CJobSystem::Acquire()
{
	m_Mutex.Acquire();
}

//-------------------------------------------------------------------------------------------
void CJobSystem::Release()
{
	m_Mutex.Release();
}

//-------------------------------------------------------------------------------------------
void CJobSystem::Startup()
{
	for(unsigned int PriorityIndex = 0; PriorityIndex < eMaxPriorities; PriorityIndex++)
	{
		m_pJobQueue[PriorityIndex] = new(m_pAllocator->Alloc(sizeof(CList<CJob>))) CList<CJob>();
	}
	m_pThreadPool = new(m_pAllocator->Alloc(sizeof(CThreadPool))) CThreadPool(this, m_pAllocator, sc_ThreadsPerCore, *m_rTimeLine);
	m_JobId = 0;
}

//-------------------------------------------------------------------------------------------
void CJobSystem::Shutdown()
{	
	m_pThreadPool->~CThreadPool();
	m_pAllocator->Free(m_pThreadPool);
	for(unsigned int PriorityIndex = 0; PriorityIndex < eMaxPriorities; PriorityIndex++)
	{
		m_pJobQueue[PriorityIndex]->~CList<CJob>();
		m_pAllocator->Free(m_pJobQueue[PriorityIndex]);
	}
}

//-------------------------------------------------------------------------------------------
void CJobSystem::AddJob(CJob* pJob)
{
	CList<CJob>* pJobQueue = m_pJobQueue[pJob->GetPriority()];
	Acquire();
	pJobQueue->AddBack(pJob);
	m_TotalJobs++;
	Release();
	m_pThreadPool->StartJobs();
}

//-------------------------------------------------------------------------------------------
CJobSystem::CJob* CJobSystem::GetJob()
{
	CJobSystem::CJob* pJob = NULL;
	Acquire();
	for (int PriorityIndex = eMaxPriorities - 1; PriorityIndex >= ePriority0; PriorityIndex--) // From highest to lowest
	{
		CList<CJobSystem::CJob>::CIterator* pIterator = NULL;
		pIterator = m_pJobQueue[PriorityIndex]->GetFirst();
		if (pIterator)
		{
			pJob = pIterator->GetData();
			m_pJobQueue[PriorityIndex]->Remove(pJob);
			m_TotalJobs--;
			break;
		}
	}
	Release();
	return pJob;
}

//-------------------------------------------------------------------------------------------
void CJobSystem::ReleaseJob(CJob* pJob)
{
	pJob->~CJob();
	m_pAllocator->Free(pJob);
}

//-------------------------------------------------------------------------------------------
void CJobSystem::YieldExecution(unsigned int JobId, eJobType JobType, unsigned int ThreadID, IBlockingCondition* pBlockingCondition)
{
#if THREADPOOL_EVENT_TRACKING		
	m_pThreadPool->AddEvent(CThreadPool::eJobYield, JobId, JobType, ThreadID);
#else
	JobId; // C4100
	JobType;
#endif
	while(pBlockingCondition->IsBlocked())
	{
		bool Ret = m_pThreadPool->ExecuteJob(ThreadID);
		if(Ret == false)
		{
			// There was nothing to execute and we are waiting on something. What do we do?
		}
	}
#if THREADPOOL_EVENT_TRACKING		
	m_pThreadPool->AddEvent(CThreadPool::eJobResume, JobId, JobType, ThreadID);	
#endif
}

//-------------------------------------------------------------------------------------------
void CJobSystem::YieldToHigherPriority(eJobPriority Priority, unsigned int ThreadID)
{
	for (int PriorityIndex = CJobSystem::eMaxPriorities - 1; PriorityIndex > Priority; PriorityIndex--) // From highest to lowest
	{
		if (!m_pJobQueue[PriorityIndex]->IsEmpty()) // Not thread safe, but we are only reading, worst case we will yield unnecesserily
		{
			m_pThreadPool->ExecuteJob(ThreadID);
			return;
		}
	}
}

//-------------------------------------------------------------------------------------------
void CJobSystem::PrintStats()
{
}