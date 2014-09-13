#include "stdafx.h"
#include "Time.h"

long long CTime::m_PerformanceFrequency;
long long CTime::m_CurrentTime;
long long CTime::m_DeltaTime;
unsigned long long CTime::m_ClockTicksPerSec;
float CTime::m_DeltaTimeSec;


//----------------------------------------------------------------------------------------------
void CTime::Initialize()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&m_PerformanceFrequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&m_CurrentTime);
	m_ClockTicksPerSec = RDTSC();
	Sleep(1000);
	m_ClockTicksPerSec = RDTSC() - m_ClockTicksPerSec;
}

//----------------------------------------------------------------------------------------------
void CTime::Tick()
{
	long long NewTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&NewTime);
	m_DeltaTime = NewTime - m_CurrentTime;
	m_CurrentTime = NewTime;
	// handle wrap around
	if(m_DeltaTime < 0)
	{
		m_DeltaTime = m_PerformanceFrequency / 60; // 60 fps default
	}
	m_DeltaTimeSec = (float)((double)m_DeltaTime / (double)m_PerformanceFrequency);
}

//----------------------------------------------------------------------------------------------
float CTime::GetDeltaTimeSec()
{
	return m_DeltaTimeSec;
}


//----------------------------------------------------------------------------------------------
unsigned long long CTime::GetClockFrequency()
{
	return m_ClockTicksPerSec;
}