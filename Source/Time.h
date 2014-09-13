#ifndef _TIME_H_
#define _TIME_H_

#ifdef _WIN64
	unsigned __int64 RDTSC()
	{		
		return __rtdsc();
	}
#else	// _WIN64
	#define rdtsc __asm __emit 0fh __asm __emit 031h
#endif

class CTime
{
	public:

		static void Initialize();
		static void Tick();
		static float GetDeltaTimeSec();
		static unsigned long long GetClockFrequency();
		static inline unsigned __int64 RDTSC()
		{
			LARGE_INTEGER li;
			__asm	push eax
			__asm	push edx
			rdtsc;
			__asm	mov	li.LowPart, eax;
			__asm	mov	li.HighPart, edx;
			__asm	pop edx
			__asm	pop eax
			return li.QuadPart;
		}

	private:	
		CTime() {} // Can only be static
		static long long m_PerformanceFrequency;
		static long long m_CurrentTime;
		static long long m_DeltaTime;
		static unsigned long long m_ClockTicksPerSec;
		static float m_DeltaTimeSec;
};

#endif
