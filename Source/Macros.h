#ifndef _MACROS_H_
#define _MACROS_H_

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define POW2ALIGN(value, alignment) ((value + (alignment - 1)) & (~(alignment - 1)))
#define ALIGN(value, alignment) (((value + (alignment - 1)) / alignment) * alignment)
#define Clamp(min, max, val) ((val) = (val) < (min) ? (min) : (val)); ((val) = (val) > (max) ? (max) : (val));

#endif