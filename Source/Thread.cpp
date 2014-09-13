#include "stdafx.h"
#include "TimeLine.h"

#include "Thread.h"

static const unsigned int scMaxStackSize = (128 * 1024);

__declspec( thread ) long g_ThreadID;
volatile long g_NumThreads = 0;

//------------------------------------------------------------------------
DWORD WINAPI CThread::DispatchThreadFunction( LPVOID lpParam )
{
	CThread::SThreadArgs* pArgs = (CThread::SThreadArgs*)lpParam;
	CThread* pThread = pArgs->m_pThread;	
	UpdateThreadID();
	pArgs->m_pTimeLine->RegisterThread(pArgs->m_Name, pArgs->m_pEventDesc);
#pragma pack(push,8)            
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = pArgs->m_Name;
	info.dwThreadID = (DWORD)-1;
	info.dwFlags = 0;
#pragma pack(pop)

	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
	}
	_except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}

	while(!pThread->m_Terminate)
	{			
		if(pThread->WaitForWorkReady())	// if work ready was signalled
		{
			pThread->ResetWorkReady();
			if(!pThread->m_Terminate)	// if thread is not terminating then run the specified user function passing it user args (do the work)
			{		
				pArgs->m_pUserFunc(pArgs->m_pUserArgs);
				pThread->SetWorkComplete(); // signal completion of all work			
			}			
		}			
	}
	pThread->SetThreadExit();
	return 0;
}

//------------------------------------------------------------------------
void CThread::UpdateThreadID()
{
	g_ThreadID = InterlockedIncrement(&g_NumThreads) - 1; // store thread ID in TLS
}

//------------------------------------------------------------------------
long CThread::GetThreadID()
{
	return g_ThreadID;
}

//------------------------------------------------------------------------
CThread::CThread() :	m_WorkReady(false),		// non signalled
						m_WorkComplete(true),	// signalled
						m_ThreadExit(false)		// non signalled

{
    // we start with work complete as signalled since there hasnt been any work scheduled yet.
	m_Terminate = false;
}

//------------------------------------------------------------------------
CThread::~CThread()
{
	CloseHandle(m_ThreadHandle);
}

//------------------------------------------------------------------------
void CThread::Startup(ThreadFunc pUserFunc, void* pUserArgs, const char* pName, CTimeLine::CEventDesc* pEventDesc, CTimeLine* pTimeLine)
{	
	// Reset work ready and thread exit
	m_ThreadArgs.m_pUserArgs = pUserArgs;
	m_ThreadArgs.m_pUserFunc = pUserFunc;
	m_ThreadArgs.m_pThread = this;
	m_ThreadArgs.m_pEventDesc = pEventDesc;
	strcpy_s(m_ThreadArgs.m_Name, 255, pName);
	m_ThreadArgs.m_pTimeLine = pTimeLine;
	m_Terminate = false;		            
	m_ThreadHandle = CreateThread(
            NULL,                   // default security attributes
            scMaxStackSize,                 
            DispatchThreadFunction, // thread function name
            &m_ThreadArgs,			// argument to thread function 
            STACK_SIZE_PARAM_IS_A_RESERVATION,
            &m_ThreadId);			// returns the thread identifier 
}

//------------------------------------------------------------------------
void CThread::SetAffinity(unsigned int AffinityMask)
{
	DWORD_PTR NewAffinityMask = SetThreadAffinityMask(m_ThreadHandle, (DWORD_PTR)AffinityMask);
	Assert(NewAffinityMask);
}

//------------------------------------------------------------------------
void CThread::Shutdown()
{
	m_Terminate = true;
	SetWorkReady();
	m_ThreadExit.Wait(INFINITE);
}

//------------------------------------------------------------------------
void CThread::SetWorkReady()
{
	m_WorkComplete.Reset();  // work complete needs to be reset so that we can wait on it 
	m_WorkReady.Set();
}

//------------------------------------------------------------------------
bool CThread::WaitForWorkComplete()
{
	bool Res = m_WorkComplete.Wait(INFINITE);
	return Res;
}

//------------------------------------------------------------------------
bool CThread::WaitForWorkReady()
{
	bool Res = m_WorkReady.Wait(INFINITE);	// wait until work ready is signalled
	return Res;
}

//------------------------------------------------------------------------
bool CThread::IsWorkReady()
{
	bool Res = m_WorkReady.Wait(0);	// wait until work ready is signalled
	return Res;
}

//------------------------------------------------------------------------
bool CThread::IsWorkComplete()
{
	bool Res = m_WorkComplete.Wait(0);	// wait until work complete is signalled
	return Res;
}

		
//------------------------------------------------------------------------		
void CThread::ResetWorkReady()
{
	m_WorkReady.Reset();
}

//------------------------------------------------------------------------
void CThread::SetWorkComplete()
{
	m_WorkComplete.Set();
}

//------------------------------------------------------------------------
void CThread::SetThreadExit()
{
	m_ThreadExit.Set();
}

//------------------------------------------------------------------------
unsigned int CThread::GetID()
{
	return m_ThreadId;
}

