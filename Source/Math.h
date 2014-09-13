#ifndef _MATH_H_
#define _MATH_H_

//-----------------------------------------------------------------------------------------
inline unsigned int Log2(unsigned int Val)
{
	unsigned int Log = 0;
	while(Val - 1)
	{
		Val >>= 1;	
		Log++;
	}		
	return Log;
}

// Based on http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor
static unsigned int s_RandSeed;
inline void IntelSRand( unsigned int Seed)
{
	s_RandSeed = Seed;
}

//-----------------------------------------------------------------------------------------
inline unsigned int IntelRand()
{
	s_RandSeed = (214013 * s_RandSeed + 2531011);
	return (s_RandSeed >> 16) & 0x7FFF;
}

//-----------------------------------------------------------------------------------------
inline unsigned int CountOnes(unsigned int Num)
{
    unsigned int Count = 0;
    while (Num > 0)
	{
        Count++;
        Num = Num & (Num - 1);
	}
    return Count;
}

inline bool IsPowerOf2(unsigned int Num)
{
	return ((Num & (Num - 1)) == 0);
}

#endif
