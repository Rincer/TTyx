#ifndef _JOBSYSTEM_H_
#define _JOBSYSTEM_H_

#include "Thread.h"
#include "LWMutex.h"
#include "List.h"
#include "HeapAllocator.h"
#include "Reference.h"

class CThreadPool;
class CTimeLine;

//-------------------------------------------------------------------------------------------
class CJobSystem
{
	public:
		enum eJobType
		{			
			eJobInvalid = -1,
			eJobLoadTexture,
			eJobLoadXFile,
			eJobCreateDrawPrimitive,
			eJobLoadMaterialLibrary,
			eJobLoadObjFile,
			eJobCreateShader,
			eJobCreateMaterial,
			eJobProcessRenderCommands,
			eJobUtilityDraw,
			eMaxJobType
		};
		
		enum eJobPriority
		{
			ePriority0, // lowest
			ePriority1,
			eMaxPriorities
		};

		//-------------------------------------------------------------------------------------------
		class CJob : public IIterant<CList<CJob>>
		{
			public:

				CJob(unsigned int Id, eJobPriority Priority, CJobSystem* pJobSystem);
				CJob(eJobType Type);				
				virtual ~CJob();
				virtual unsigned int Execute(unsigned int ThreadID) = 0;
				virtual void OnComplete(); // default behavior is that the job releases itself, this is overriden when job needs custom termination (such as releasing memory being used on a different thread)
				virtual CList<CJob>::CIterator* GetIterator() { return &m_Iterator; }
				eJobType GetType() { return m_Type;}
				eJobPriority GetPriority() { return m_Priority; }
				unsigned int GetId() { return m_Id; }

			protected:
		
				eJobType		m_Type;
				unsigned int	m_Id;
				eJobPriority	m_Priority;		
				CList<CJob>::CIterator m_Iterator;
				CJobSystem*	m_pJobSystem; // Parent system
		};				

		class IBlockingCondition
		{
			public:
				virtual bool IsBlocked() = 0;
		};
		
		CJobSystem(CTimeLine** ppTimeLine);
		~CJobSystem();

		//-------------------------------------------------------------------------------------------
		template<class JobType>
		JobType* AcquireJob(eJobPriority Priority)
		{
			JobType* pJob = (JobType*)m_pAllocator->Alloc(sizeof(JobType));
			if(pJob)
			{
				new(pJob)JobType(m_JobId++, Priority, this);
				return pJob;
			}
			return NULL;
		}

		void AddJob(CJob* pJob);	
		CJob* GetJob();

		void ReleaseJob(CJob* pJob);
		void YieldExecution(unsigned int JobId, eJobType JobType, unsigned int ThreadID, IBlockingCondition* pBlockingCondition); // Use when a job is blocked to try and execute any other jobs that are queued up		
		void YieldToHigherPriority(eJobPriority Priority, unsigned int ThreadID);
		void PrintStats();
		
	private:			
		void Startup();
		void Shutdown();
		void Acquire();
		void Release();
		static const unsigned int sc_ThreadsPerCore = 2;
		CList<CJob>*	m_pJobQueue[eMaxPriorities];
		CThreadPool*	m_pThreadPool;
		CHeapAllocator* m_pAllocator;
		CLWMutex		m_Mutex;
		volatile unsigned int m_TotalJobs;
		unsigned int m_JobId;		
		CReference<CTimeLine*> m_rTimeLine;
};



#endif