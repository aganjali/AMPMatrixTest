#pragma once 

#include <iostream>
#include "HiPerfTimer.h"

#define SAFE_RELEASE(p)     do { if ((p) != 0) { (p)->Release(); (p) = NULL; } } while(0)
#define SAFE_DELETE(p)     do { if ((p) != 0) { delete p; (p) = NULL; } } while(0)

#ifndef Debug
	#define Debug
#endif

#define PRINT_ERROR(_msg)  std::cout << _msg << "\n";	

#if defined(Debug)
#define ASSERT(_failed, _msg)                     \
if (_failed)                                    \
{                                                   \
	PRINT_ERROR(_msg);		\
	__debugbreak();									\
}
#endif