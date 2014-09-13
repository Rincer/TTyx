#include "stdafx.h"
#include "Timer.h"

//----------------------------------------------------------------------------------------------
CTimer::CTimer()
{
}

//----------------------------------------------------------------------------------------------
void CTimer::SetFrequency(float Frequency)
{
	m_Frequency = Frequency;
}

//----------------------------------------------------------------------------------------------
bool CTimer::Tick(float DeltaSec)
{
	m_Interval -= DeltaSec;
	if(m_Interval < 0)
	{
		m_Interval = m_Frequency;
		return true;
	}
	return false;
}

