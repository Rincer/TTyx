#ifndef _DEBUGGING_H_
#define _DEBUGGING_H_

#define DebugPrintf(...) \
{ \
	char Str[256]; \
	sprintf_s(Str, 255, __VA_ARGS__); \
	OutputDebugStringA(Str); \
}

bool FailAssert(char* Expression);

#define Assert(Expression) (void)( (!!(Expression)) || FailAssert(#Expression) )


#endif _DEBUGGING_H_