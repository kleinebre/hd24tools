#ifndef __hd24driveimage_h__
#define __hd24driveimage_h__

#include <config.h>
#include <stdio.h>
#include <string>
#include "convertlib.h"
#define IMGCLUSTER_FREE (0x00000000)

#if defined(LINUX) || defined(DARWIN)
#	define FSHANDLE int
#	define FSHANDLE_INVALID -1
#endif

#ifdef WINDOWS
#	include <windows.h>
#	include <winioctl.h>
#	define FSHANDLE HANDLE
#	define FSHANDLE_INVALID INVALID_HANDLE_VALUE
#endif

using namespace std;

class hd24driveimage 
{
private:
	FSHANDLE m_handle;
	bool isreserved(__uint32 blocknum);
	void reserveblock(__uint32 blocknum);
        static bool isinvalidhandle(FSHANDLE handle);

	__uint32 blocknumtosector(__uint32 blocknum);

	/* Read/write sectors from/to file without translation- 
           meaning it can read outside drive image boundaries and 
           access "smart drive image" info as well. */
        long rawreadsectors(unsigned long sectornum,
                            unsigned char * buffer,int sectors);
        long rawwritesectors(unsigned long sectornum,
                            unsigned char * buffer,int sectors);

	/* Read/write sectors from/to clusters in drive image file. 
	   Will do a sector lookup, but will not cross cluster boundaries.
	   Can only access the wrapped drive image. */
        long readblocksectors(__uint32 blocknum, 
			__uint32 secnum,unsigned char* buffer,int sectors);
        long writeblocksectors(__uint32 blocknum, 
			__uint32 secnum,unsigned char* buffer,int sectors);

	void rawseek(__uint64 seekpos);
	bool suppresslogs;
public:
	hd24driveimage();
	~hd24driveimage();
        void initvars();
	int initimage(string* imagefilename,__uint32 endsector);
	FSHANDLE open(string* imagefilename);
	FSHANDLE handle(FSHANDLE p_handle);
	FSHANDLE handle();
	long content_readsectors(__uint32 secnum, 
                          unsigned char* buffer,int sectors);
	long content_writesectors(__uint32 secnum, 
                          unsigned char* buffer,int sectors);
	void close();
	__uint32 getcontainerlastsectornum();
	__uint32 getcontentlastsectornum();
	int test(string* imagefilename);
};

#endif
