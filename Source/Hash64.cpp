#include "stdafx.h"
#include "Hash64.h"
// from http://burtleburtle.net/bob/c/lookup8.c

/*
--------------------------------------------------------------------
This works on all machines, is identical to hash() on little-endian 
machines, and it is much faster than hash(), but it requires
-- that the key be an array of unsigned long long's, and
-- that all your machines have the same endianness, and
-- that the length be the number of unsigned long long's in the key
--------------------------------------------------------------------
*/
#define hashsize(n) ((unsigned long long)1<<(n))
#define hashmask(n) (hashsize(n)-1)

#define mix64(a,b,c) \
{ \
	a -= b; a -= c; a ^= (c>>43); \
	b -= c; b -= a; b ^= (a<<9); \
	c -= a; c -= b; c ^= (b>>8); \
	a -= b; a -= c; a ^= (c>>38); \
	b -= c; b -= a; b ^= (a<<23); \
	c -= a; c -= b; c ^= (b>>5); \
	a -= b; a -= c; a ^= (c>>35); \
	b -= c; b -= a; b ^= (a<<49); \
	c -= a; c -= b; c ^= (b>>11); \
	a -= b; a -= c; a ^= (c>>12); \
	b -= c; b -= a; b ^= (a<<18); \
	c -= a; c -= b; c ^= (b>>22); \
}

unsigned long long hash(const char* k, unsigned long long length, unsigned long long level)
{
	unsigned long long a,b,c,len;

	/* Set up the internal state */
	len = length;
	a = b = level;                         /* the previous hash value */
	c = 0x9e3779b97f4a7c13ULL; /* the golden ratio; an arbitrary value */

	/*---------------------------------------- handle most of the key */
	while (len >= 3)
	{
		a += (unsigned long long)k[0] << 8;
		b += (unsigned long long)k[1] << 16;
		c += (unsigned long long)k[2] << 32;
		mix64(a,b,c);
		k += 3; len -= 3;
	}

	/*-------------------------------------- handle the last 2 unsigned long long's */
	c += (length<<3);
	switch(len)              /* all the case statements fall through */
	{
		/* c is reserved for the length */
	case  2: b+=(unsigned long long)k[1];
	case  1: a+=(unsigned long long)k[0];
		/* case 0: nothing left to add */
	}
	mix64(a,b,c);
	/*-------------------------------------------- report the result */
	return c;
}

unsigned long long CHash64::GetHash(const char* pString)
{
	unsigned int Len =  (unsigned int)strlen(pString);
	return hash(pString, (unsigned long long)Len, 0ULL);
}



