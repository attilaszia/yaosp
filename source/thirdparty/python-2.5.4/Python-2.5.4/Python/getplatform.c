
#include "Python.h"

#ifndef PLATFORM
#define PLATFORM "yaosp"
#endif

const char *
Py_GetPlatform(void)
{
	return PLATFORM;
}
