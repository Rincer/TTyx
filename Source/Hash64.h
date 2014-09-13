#ifndef HASH64_H_
#define HASH64_H_

//--------------------------------------------------------------------------------
// Wrapper around string hashing code found on the internet
//--------------------------------------------------------------------------------
class CHash64
{
	public:
		static unsigned long long GetHash(const char* pString);
};

#endif
