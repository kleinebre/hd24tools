#ifndef __hd24driveimage_h__
#define __hd24driveimage_h__

#include <stdint.h>
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
	bool isreserved(uint32_t blocknum);
	void reserveblock(uint32_t blocknum);
        static bool isinvalidhandle(FSHANDLE handle);

	uint32_t blocknumtosector(uint32_t blocknum);

	/* Read/write sectors from/to file without translation- 
           meaning it can read outside drive image boundaries and 
           access "smart drive image" info as well. */
        int32_t rawreadsectors(uint32_t sectornum,
                            unsigned char * buffer,int32_t sectors);
        int32_t rawwritesectors(uint32_t sectornum,
                            unsigned char * buffer,int32_t sectors);

	/* Read/write sectors from/to clusters in drive image file. 
	   Will do a sector lookup, but will not cross cluster boundaries.
	   Can only access the wrapped drive image. */
        long readblocksectors(uint32_t blocknum, 
			uint32_t secnum,unsigned char* buffer,int sectors);
        long writeblocksectors(uint32_t blocknum, 
			uint32_t secnum,unsigned char* buffer,int sectors);

	void rawseek(uint64_t seekpos);
	bool suppresslogs;
public:
	hd24driveimage();
	~hd24driveimage();
        void initvars();
	int initimage(string* imagefilename,uint32_t endsector);
	FSHANDLE open(string* imagefilename);
	FSHANDLE handle(FSHANDLE p_handle);
	FSHANDLE handle();
	long content_readsectors(uint32_t secnum, 
                          unsigned char* buffer,int sectors);
	long content_writesectors(uint32_t secnum, 
                          unsigned char* buffer,int sectors);
	void close();
	uint32_t getcontainerlastsectornum();
	uint32_t getcontentlastsectornum();
	int test(string* imagefilename);
};

#endif
