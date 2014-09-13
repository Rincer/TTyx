#include "stdafx.h"

bool FailAssert(char* Expression)
{
	DebugPrintf("Assertion failed (%s)\n",Expression);
	__debugbreak(); 
	return false;
}
