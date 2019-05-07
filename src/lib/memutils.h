#ifndef __memutils_h__
#define __memutils_h__

using namespace std;
#include "config.h"

class memutils
{
public:
		static void* mymalloc(const char* wherefrom,__uint32 elcount,__uint32 elsize);
		static void myfree(const char* wherefrom,void* freewhat);
};

#endif

