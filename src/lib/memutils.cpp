#include <stdlib.h>
#include <iostream>
#include "memutils.h"
void* memutils::mymalloc(const char* wherefrom,uint32_t elcount,uint32_t elsize)
{
	void* q=calloc(elcount,elsize);
#if (MEMDEBUG==1) 
	cout 	<< "ALLOC: " << wherefrom 
		<<" allocated " 
		<< elcount<<" bytes at " << q << endl;
#else
	wherefrom=NULL;
#endif
	return q;
}

void memutils::myfree(const char* wherefrom,void* freewhat)
{
#if (MEMDEBUG==1) 
	cout << "FREE: "<< wherefrom <<" free bytes at " << freewhat << endl;
#else
	wherefrom=NULL;
#endif
	free(freewhat);
}

