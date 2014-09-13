#ifndef _TIMER_H_
#define _TIMER_H_

class CTimer
{
	public:
		CTimer();
		void SetFrequency(float Frequency);
		bool Tick(float DeltaSec);

	private:	
		float m_Frequency;
		float m_Interval;
};

#endif
