#ifndef __UTIL_TYPE_H__
#define __UTIL_TYPE_H__

#include "src/util/namespace-start.h"

#define CONST_GLOBAL_SIZE (2<<10)

typedef float BaseFloat;
typedef float float32;
typedef double double64;
typedef short int16;
typedef int int32;
typedef long long int int64;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long int uint64;
typedef float BaseFloat;
enum RetVal
{
	ERROR = -1,
	OK = 0
};

#include "src/util/namespace-end.h"
#endif
