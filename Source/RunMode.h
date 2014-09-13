#ifndef _RUNMODE_H_
#define _RUNMODE_H_

// Base class for different runmodes that the main thread can be in
class CRunMode
{
	public:
		CRunMode()
		{
		}
		virtual ~CRunMode()
		{
		}
		virtual void Tick(float DeltaSec) = 0;
};

#endif