#ifndef __memutils_h__
#define __memutils_h__

using namespace std;

class memutils
{
public:
		static void* mymalloc(const char* wherefrom,uint32_t elcount,uint32_t elsize);
		static void myfree(const char* wherefrom,void* freewhat);
};

#endif

