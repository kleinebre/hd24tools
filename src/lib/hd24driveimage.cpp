/* 

This class implements "smart" drive images (see doc/ directory).
The class itself however is pretty "dumb" in that it does not
have knowledge about the structure of the data it contains.

*/

#include "hd24driveimage.h"
#include "hd24utils.h"
#include <string.h> /* for memset, memcpy etc */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define SECTORSIZE 512
#define CLUSTERSIZE 4096
#define FS_OFFSET 8193
#define ALLOCSECTORNUM 1 /* TODO: should get from superblock but would be slower */
#define ERROR_CANNOT_READ_ALLOCTABLE 1
#define ERROR_CANNOT_WRITE 2
#define ERROR_FILESIZE_LOOKUP_FAILED 3
#define ERROR_APPEND_FAILED 4
#define ERROR_CANNOT_WRITE_ALLOCTABLE 5
#define ERROR_CANNOT_READ_SUPERBLOCK 6
#define ERROR_INVALID_DRIVEIMAGEHANDLE 7

#       define INT_FSHANDLE_INVALID -1
#if defined(LINUX) || defined(DARWIN)
#       define FSHANDLE int
#       define FSHANDLE_INVALID -1
#endif

#ifdef WINDOWS
#       include <windows.h>
#       include <winioctl.h>
#       define FSHANDLE HANDLE
#       define FSHANDLE_INVALID INVALID_HANDLE_VALUE
#endif
#ifdef DARWIN
#define creat64 creat
#define open64 ::open
#define lseek64 lseek
#define pread64 pread
#define pwrite64 pwrite
#endif
bool hd24driveimage::isinvalidhandle(FSHANDLE handle)
{
#ifdef WINDOWS
        if (handle==FSHANDLE_INVALID) {
                return true;
        }
#endif
#if defined(LINUX) || defined(DARWIN)
        if (handle==0) {
                return true;
        }
        if (handle==FSHANDLE_INVALID) {
                return true;
        }
#endif  
        return false;
}

void hd24driveimage::rawseek(uint64_t seekpos) {
#if defined(LINUX) || defined(DARWIN)
        lseek64(this->m_handle,seekpos,SEEK_SET);
#endif
#ifdef WINDOWS
        LARGE_INTEGER li;
        li.HighPart=seekpos>>32;
        li.LowPart=seekpos%((uint64_t)1<<32);
        SetFilePointerEx(this->m_handle,li,NULL,FILE_BEGIN);
#endif
        return;
}

int32_t hd24driveimage::rawwritesectors(uint32_t sectornum,unsigned char* buffer,int32_t sectors)
{
#if (SMARTIMAGEDEBUG==1)
	if (!this->suppresslogs) 
	{
	cout << "rawwritesectors(handle="<<this->m_handle
	     <<",sector="<<sectornum
             <<",buffer="<<buffer
             <<",sectors="<<sectors
        <<"), first sector being written follows:"<<endl;
//	hd24utils::dumpsector((const char*)buffer);
	}
	
#endif
       hd24driveimage::rawseek((uint64_t)sectornum*SECTORSIZE);
       int WRITESIZE=SECTORSIZE*sectors;
#if defined(LINUX) || defined(DARWIN) || defined(__APPLE__)
       long bytes=pwrite64(this->m_handle,buffer,WRITESIZE,(uint64_t)sectornum*SECTORSIZE); //1,devhd24);
#endif
#ifdef WINDOWS
        DWORD dummy;
        long bytes=0;
        if (WriteFile(this->m_handle,buffer,WRITESIZE,&dummy,NULL)) {
                bytes=WRITESIZE;
        };
#endif  
        return bytes>>9;
}

int32_t hd24driveimage::rawreadsectors(uint32_t sectornum,unsigned char * buffer,int32_t sectors) 
{
#if (SMARTIMAGEDEBUG==1)
	cout << "hd24driveimage::rawreadsectors(devhd24="<<this->m_handle<<",sectornum="<<sectornum<<",buf="<<buffer<<",#="<<sectors<<"),"
	<< " first sector having been read follows:"
	<< endl;
#endif
	//FSHANDLE currdevice=devhd24;
       	//rawseek(currdevice,(uint64_t)sectornum*SECTORSIZE);
       	rawseek((uint64_t)sectornum*SECTORSIZE);
       int READSIZE=SECTORSIZE*(sectors);
#if defined(LINUX) || defined(DARWIN) 
       long bytes_read=pread64(this->m_handle,buffer,READSIZE,(uint64_t)sectornum*512); //1,currdevice);
#endif
#ifdef WINDOWS
	DWORD bytes_read;
	//long bytes=0;
	if( ReadFile(this->m_handle,buffer,READSIZE,&bytes_read,NULL)) {
	} else {
		bytes_read = 0;
	}
#endif
//	hd24utils::dumpsector((const char*)buffer);
        return bytes_read>>9;
}

uint32_t hd24driveimage::blocknumtosector(uint32_t blocknum)
{
	uint32_t sectornum=(blocknum>>7)+ALLOCSECTORNUM;
	uint32_t wordnum=blocknum & 0x7f; /* entry index within sector */
	uint32_t bytenum=wordnum*4;  /* byte offset of entry index */

	unsigned char sectorbuffer[SECTORSIZE];
	memset(&sectorbuffer[0],0,SECTORSIZE); /* Init sector with 0 values only */

	uint32_t secsread=this->rawreadsectors(sectornum,&sectorbuffer[0],1);
	if (secsread==0)
	{
		/* Didn't manage to read the alloc table. 
		   Shouldn't happen, but perhaps we run into a data error
                   or we try to read a sector of an alloc table that wasn't
                   created yet (could be the case when used incorrectly on 
                   an uninitialized image?) 
                */
		throw ERROR_CANNOT_READ_ALLOCTABLE;
	}
	uint32_t sec=0;
	         sec+=sectorbuffer[bytenum+3];
	sec<<=8; sec+=sectorbuffer[bytenum+2];
	sec<<=8; sec+=sectorbuffer[bytenum+1];
	sec<<=8; sec+=sectorbuffer[bytenum];
	if (blocknum!=0) {
		return sec+FS_OFFSET;
	}
	return sec;
}

bool hd24driveimage::isreserved(uint32_t blocknum)
{
	unsigned char sectorbuffer[SECTORSIZE];

	/* Verifies if the given block number is already in use on disk.
	   blocknum*4=bytenum of word to verify in 
           drive image allocation table.
	   If the word contains zero, it's not reserved; otherwise it is.
           We have 512/4=128 entries per sector.
           To determine which sector to verify, we divide blocknum by 128.
           If we do this prior to the *4 multiplication, we also work 
           around surpassing the 32-bit limit.
	   FIXME: Should look at boot sector to find out sectornum of
                  alloc table sector.
	*/

	uint32_t sectornum=(blocknum>>7)+ALLOCSECTORNUM;
	uint32_t wordnum=blocknum & 0x7f; /* entry index within sector */
	uint32_t bytenum=wordnum*4;  /* byte offset of entry index */

	memset(&sectorbuffer[0],0,SECTORSIZE); /* Init sector with 0 values only */
	
	uint32_t secsread=hd24driveimage::rawreadsectors(sectornum,&sectorbuffer[0],1);
	if (secsread==0)
	{
		/* Didn't manage to read the alloc table. 
		   Shouldn't happen, but perhaps we run into a data error
                   or we try to read a sector of an alloc table that wasn't
                   created yet (could be the case when used incorrectly on 
                   an uninitialized image?) 
                */
		throw ERROR_CANNOT_READ_ALLOCTABLE;
	}
	if (
		(sectorbuffer[bytenum]==((unsigned char)0))
		&&(sectorbuffer[bytenum+1]==((unsigned char)0))
		&&(sectorbuffer[bytenum+2]==((unsigned char)0))
		&&(sectorbuffer[bytenum+3]==((unsigned char)0))
	)
	{
		/* Word is zero, in other words: it's not reserved. */
#if (SMARTIMAGEDEBUG==1)
		cout << "Blocknum "<<blocknum<<" is not reserved."<<endl;
#endif
		return false;	
	}
#if (SMARTIMAGEDEBUG==1)
		cout << "Blocknum "<<blocknum<<" was already reserved."<<endl;
#endif
	return true;
}

uint32_t hd24driveimage::getcontentlastsectornum()
{
	// returns the last addressable sector number of the image content
	// may be larger than the actual image file size.
	// sectors on drive-1
	unsigned char* sectorbuffer=(unsigned char*)malloc(512);
	if (sectorbuffer==NULL)
	{
		throw ERROR_CANNOT_READ_SUPERBLOCK;
	}
	// last sectornum is specified in smart image superblock (sector 0)
	memset(sectorbuffer,0,SECTORSIZE);
#if (SMARTIMAGEDEBUG==1)
	cout << "Read image superblock to get last sectornum of contained drive." << endl;
#endif
	uint32_t secsread=hd24driveimage::rawreadsectors(0,sectorbuffer,1);
	if (secsread==0)
	{
		/* Didn't manage to read the superblock.
		   Shouldn't happen, but perhaps we run into a data error
                   or we try to read a sector of an alloc table that wasn't
                   created yet (could be the case when used incorrectly on 
                   an uninitialized image?) 
                */
		throw ERROR_CANNOT_READ_SUPERBLOCK;
	};
	uint32_t lastsector=0;
//	hd24utils::dumpsector((const char*)sectorbuffer);

	int b1=(int)((unsigned char)sectorbuffer[28]);
	int b2=(int)((unsigned char)sectorbuffer[29]);
	int b3=(int)((unsigned char)sectorbuffer[30]);
	int b4=(int)((unsigned char)sectorbuffer[31]);
#if (SMARTIMAGEDEBUG==1)
	cout << "b1,b2,b3,b4="<<b1<<","<<b2<<","<<b3<<","<<b4<<endl;
#endif
	lastsector=(b4<<24)+(b3<<16)+(b2<<8)+b1;
	free(sectorbuffer);
	return lastsector;
}

uint32_t hd24driveimage::getcontainerlastsectornum()
{

	if (isinvalidhandle(this->m_handle)) 
	{
		return 0;
	}

#if defined(LINUX) || defined(DARWIN)
	uint64_t curroff=lseek64(this->m_handle,0,SEEK_CUR);
	// >>9 equals /512
	uint64_t sects=(lseek64(this->m_handle,0,SEEK_END));
	uint32_t remainder=sects%SECTORSIZE;
	sects-=remainder;
	if (remainder!=0) remainder=1;
	sects=(sects>>9)+remainder;
	lseek64(this->m_handle,curroff,SEEK_SET);
#if (SMARTIMAGEDEBUG==1)
		cout << "Image container file size in sectors=" << sects-1 << endl;
#endif
	return sects-1; // sector num is zero based and thus last sector num=sector count-1.
#endif
	
#ifdef WINDOWS
	//unsigned char buffer[2048];
	LARGE_INTEGER lizero;
	lizero.QuadPart=0;
	LARGE_INTEGER saveoff;
	LARGE_INTEGER filelen;
	filelen.QuadPart=0;
	SetFilePointerEx(this->m_handle,lizero,&saveoff,FILE_CURRENT); // save offset

	/* If things would work properly, the following would
	 * not only work for regular files, but also for devices.
	 * Perhaps the next version of Windows has proper hardware
	 * abstraction and this will magically start working.
	 */
	if (0!=SetFilePointerEx(this->m_handle,lizero,&filelen,FILE_END)) {
		long long flen=filelen.QuadPart;
		SetFilePointerEx(this->m_handle,saveoff,NULL,FILE_BEGIN); // restore offset
#if (SMARTIMAGEDEBUG==1)
		cout << "Image container file size in sectors=" << hd24fs::bytenumtosectornum(flen) << endl;
#endif
		return hd24fs::bytenumtosectornum(flen);
	}

	/* file size lookup failed. Throw. */
	throw ERROR_FILESIZE_LOOKUP_FAILED;
#endif
}

void hd24driveimage::reserveblock(uint32_t blocknum)
{

	/* Given a drive image and a block number, verifies if the block is
           already reserved or not.
           If not, a block is appended to the file and the allocation info
           is set to point to it (subtracting the FS overhead to permit
           using the full 2TB range).
        */
#if (SMARTIMAGEDEBUG==1)
	cout << "hd24driveimage::reserveblock " << endl;
        cout << "for handle " << this->m_handle  << endl;
#endif
#if (SMARTIMAGEDEBUG==1)
        cout << ", blocknum to reserve=" << blocknum 
             << endl;
	cout << "Check if block "<<blocknum<<" is reserved." << endl;
#endif
	
	if (isreserved(blocknum))
	{
#if (SMARTIMAGEDEBUG==1)
		cout << "Block "<<blocknum<<"is already reserved, no need to re-reserve." << endl;
#endif
		return;
	}
#if (SMARTIMAGEDEBUG==1)
	cout << "Block "<<blocknum<<" still needs to be reserved." << endl;
#endif
	unsigned char buffer[CLUSTERSIZE*SECTORSIZE];
	unsigned char sectorbuffer[SECTORSIZE];
        memset(buffer,0,CLUSTERSIZE*SECTORSIZE);
	uint32_t lastsec=hd24driveimage::getcontainerlastsectornum();
#if (SMARTIMAGEDEBUG==1)
	cout << "Container's last sectornum="<<lastsec << endl;
#endif
        long sectors_written=rawwritesectors(lastsec+1,&buffer[0],CLUSTERSIZE);
#if (SMARTIMAGEDEBUG==1)
	cout << "Sectors written, clustersize=" << sectors_written <<","<<CLUSTERSIZE << endl;
#endif
	if (sectors_written!=CLUSTERSIZE)
	{
		throw ERROR_APPEND_FAILED;
	}

	/* Now update entry for blocknum to point to newly appended block */
	uint32_t sectornum=(blocknum>>7)+ALLOCSECTORNUM;
	uint32_t wordnum=blocknum & 0x7f; /* entry index within sector */
	uint32_t bytenum=wordnum*4;  /* byte offset of entry index */
#if (SMARTIMAGEDEBUG==1)
	cout << "bytenum for block "<<blocknum<<"=" << bytenum << endl;
#endif
	memset(&sectorbuffer[0],0,SECTORSIZE); /* Init sector with 0 values only */
	
	uint32_t secsread=hd24driveimage::rawreadsectors(sectornum,&sectorbuffer[0],1);
	if (secsread==0)
	{
		/* Didn't manage to read the alloc table. 
		   Shouldn't happen, but perhaps we run into a data error
                   or we try to read a sector of an alloc table that wasn't
                   created yet (could be the case when used incorrectly on 
                   an uninitialized image?) 
                */
		throw ERROR_CANNOT_READ_ALLOCTABLE;
	}

	if (blocknum!=0) { 
		lastsec-=FS_OFFSET;
	} else { 
		//newly added sector for blocknum is on lastsec+1;
		// (which incidentally equals FS_OFFSET for blocknum 0)
		lastsec++; 
	}
#if (SMARTIMAGEDEBUG==1)
	cout << "Setting entry for blocknum " <<blocknum<<"to point to sector "<<lastsec;
#endif
	if (blocknum!=0) {
#if (SMARTIMAGEDEBUG==1)
		cout <<" of content"<<endl;
#endif
	} else {
#if (SMARTIMAGEDEBUG==1)
		cout <<" of container" << endl;
#endif
	}
	sectorbuffer[bytenum]=(unsigned char)(lastsec&0xff); lastsec>>=8;
	sectorbuffer[bytenum+1]=(unsigned char)(lastsec&0xff); lastsec>>=8;
	sectorbuffer[bytenum+2]=(unsigned char)(lastsec&0xff); lastsec>>=8;
	sectorbuffer[bytenum+3]=(unsigned char)(lastsec&0xff); lastsec>>=8;

	sectors_written=hd24driveimage::rawwritesectors(sectornum,&sectorbuffer[0],1);
#if (SMARTIMAGEDEBUG==1)
	cout << "Sectors written to sector "<< sectornum<<"=" << sectors_written << ", handle " << this->m_handle<<endl;
#endif
	if (sectors_written!=1)
	{
		throw ERROR_CANNOT_WRITE_ALLOCTABLE;
	}
	return;
}

int hd24driveimage::initimage(string* imagefilename,uint32_t endsector)
{
	/* This creates and "formats" a new image, setting up a basic structure 
           permitting a certain number of sectors. */
	/* Returns 0 on success, error code otherwise. */

#define MULTISECTOR 100
	/* This is for quickly clearing large blocks of sectors... 
           may not be needed for smart drive images. */

        unsigned char bootblock[(MULTISECTOR+1)*SECTORSIZE]; // +1 sector spare

	/* Clear "boot" block buffer */
        memset(bootblock,0,MULTISECTOR*SECTORSIZE);

	/* Create the actual drive image. */
#if defined(LINUX) || defined(DARWIN)
        this->m_handle=open64(imagefilename->c_str(),O_CREAT|O_RDWR|O_TRUNC,0664);
#endif

#ifdef WINDOWS
        this->m_handle=
              CreateFile(
		imagefilename->c_str(),
		GENERIC_WRITE|GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS|FILE_FLAG_WRITE_THROUGH,
		NULL);
#endif

        if (hd24driveimage::isinvalidhandle(this->m_handle)) {
		throw ERROR_INVALID_DRIVEIMAGEHANDLE;
	}
	hd24driveimage::rawseek(0);

	/*
	Right. The file exists, has been opened, and
	we've performed the seek to place the write cursor
	at position 0 in the file. Let's create the drive
	image structure.

	8 byte virt drive signature SMARTIMG
	16 byte (128 bit) drive id (UUID) (optional; 0 if not used)
	32 bit cluster size in 512-byte sectors, e.g. 4096 (2^12) gives 
	   clusters with a size of 2MB each for a max. of 2^20 entries 
           for 2 TB drives;
	   with each entry being 4 bytes, this means we'll need ((2^20)*4)/512)
	   =8192 sectors (4 megabytes) for the drive block usage table.
	32 bit number of last allowed sector (=tot drive size in sectors-1).
	32 bit start sector of virtual drive inside image

	*/

	// drive signature
	memcpy(bootblock,(const void*)("SMARTIMG"),8); 

	// cluster size
	uint32_t clustersize=CLUSTERSIZE;
	bootblock[24]=(unsigned char)(clustersize&0xff); clustersize >>=8;
	bootblock[25]=(unsigned char)(clustersize&0xff); clustersize >>=8;
	bootblock[26]=(unsigned char)(clustersize&0xff); clustersize >>=8;
	bootblock[27]=(unsigned char)(clustersize&0xff); clustersize >>=8;

	// sectors on drive-1
	uint32_t lastsector=endsector;
	bootblock[28]=(unsigned char)(lastsector&0xff); lastsector>>=8;
	bootblock[29]=(unsigned char)(lastsector&0xff); lastsector>>=8;
	bootblock[30]=(unsigned char)(lastsector&0xff); lastsector>>=8;
	bootblock[31]=(unsigned char)(lastsector&0xff); lastsector>>=8;

	// 8192 sectors worth of allocation table+1 superblock
	// =8193 sectors so sector number where actual data starts=8193
	// (0-based counting).
	uint32_t startsector=FS_OFFSET; //8193;
	bootblock[32]=(unsigned char)(startsector&0xff); startsector>>=8;
	bootblock[33]=(unsigned char)(startsector&0xff); startsector>>=8;
	bootblock[34]=(unsigned char)(startsector&0xff); startsector>>=8;
	bootblock[35]=(unsigned char)(startsector&0xff); startsector>>=8;

	int canwrite=hd24driveimage::rawwritesectors(
					  (unsigned long)0 /*sectornum*/,
					  bootblock,
					  (int)1/*sectors*/);
	if (canwrite==0)
	{
		// write failed
		throw ERROR_CANNOT_WRITE;
	}

        memset(bootblock,0,SECTORSIZE);
	this->suppresslogs=true;
	for (int i=0;i<8192;i++)
	{
		int canwrite=hd24driveimage::rawwritesectors(
					  (unsigned long)(1+i) /*sectornum*/,
					  bootblock,
					  (int)1/*sectors*/);
		if (canwrite==0)
		{
			// write failed
			throw ERROR_CANNOT_WRITE;
		}
		
	}
	this->suppresslogs=false;
	try 
	{
#if (SMARTIMAGEDEBUG==1)
	cout << "reserving block 0."<<endl;
	cout << "this="<<this << endl;
#endif
		hd24driveimage::reserveblock(0);
#if (SMARTIMAGEDEBUG==1)
	cout << "reserved block 0."<<endl;
#endif
	}
	catch (int e)
	{
		/* could not reserve block. Reason: e.
		   We probably should retry initializing the image. */ 
		cout << "Cannot reserve - error " << e << endl;
		return INT_FSHANDLE_INVALID;
	}
	return 0;
}

FSHANDLE hd24driveimage::handle(FSHANDLE p_handle)
{
	// TODO: format image as needed.
	this->m_handle=p_handle;
	return m_handle;
}

FSHANDLE hd24driveimage::handle()
{
	return this->m_handle;
}

FSHANDLE hd24driveimage::open(string* imagefilename)
{
#if (SMARTIMAGEDEBUG==1)
	cout << "Open existing smartimage " << *imagefilename << endl;
#endif
	/* This opens an existing (smart) image and will format it as needed when it's a new one. */
#if defined(LINUX) || defined(DARWIN)
        this->m_handle=open64(imagefilename->c_str(),O_RDWR|O_CREAT,0660); 
#endif
#ifdef WINDOWS
        _unlink(imagefilename->c_str());
        this->m_handle=CreateFile(imagefilename->c_str(),GENERIC_WRITE|GENERIC_READ,
              FILE_SHARE_READ|FILE_SHARE_WRITE,
              NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
#endif
	return this->m_handle;
}

long hd24driveimage::readblocksectors(uint32_t blocknum, uint32_t secnum,unsigned char* buffer,int sectors)
{
	/* Given a custernum, startsector within that cluster and sector count,
	   will read the requested sectors from that cluster (where sector 0 is
           considered the first sector of a cluster).
	*/
#if (SMARTIMAGEDEBUG==1)
	cout <<"hd24driveimage::readblocksectors("
		<<"hnd="<<this->m_handle
		<<",blocknum="<<blocknum
		<<",sec="<<secnum
		<<",buf="<<&buffer[0]
		<<",sectors="<<sectors
	<<endl;
#endif
        if (!isreserved(blocknum))
	{
           /* Cluster doesn't exist, all-zero sectors will be returned. */
	 	memset(&buffer[0],0,sectors*SECTORSIZE);
		return sectors; // FIXME: return 0 when surpassing drive size
	}
	uint32_t blockstartsector=blocknumtosector(blocknum);
	uint32_t readcount=rawreadsectors(blockstartsector+secnum,&buffer[0],sectors);
	return readcount; // TODO: Return sector count actually read
}

long hd24driveimage::writeblocksectors(uint32_t blocknum, uint32_t secnum,unsigned char* buffer,int sectors)
{
	/* Given a custernum, startsector within that cluster and sector count,
	   will write the requested sectors to that cluster (where sector 0 is
           considered the first sector of a cluster).
	*/
#if (SMARTIMAGEDEBUG==1)
	cout <<"hd24driveimage::writeblocksectors("
		<<"hnd="<<this->m_handle
		<<",blocknum="<<blocknum
		<<",sec="<<secnum
		<<",buf="<<&buffer[0]
		<<",sectors="<<sectors
	<<endl;
#endif
        reserveblock(blocknum); // create block if it doesn't exist yet
	// Now find out on which sector the block starts
	uint32_t blockstartsector=blocknumtosector(blocknum);
	uint32_t writecount=rawwritesectors(blockstartsector+secnum,buffer,sectors);
	return writecount; // TODO: Return sector count actually written
}


long hd24driveimage::content_readsectors(uint32_t secnum, unsigned char* buffer,int sectors)
{
	/* Read a number of sectors from the image. 
           The call is split up so it never crosses block boundaries. */
	if (sectors==0)
	{
		// cannot read 0 sectors. Error condition.
		return 0;
	}

#if (SMARTIMAGEDEBUG==1)
	cout <<"hd24driveimage::content_readsectors("
		<<"hnd="<<this->m_handle
		<<",sec="<<secnum
		<<",buf="<<&buffer[0]
		<<",sectors="<<sectors
	<<endl;
#endif
	uint32_t clustersize=CLUSTERSIZE; // TODO: get from superblock
	uint32_t startcluster=int(secnum/clustersize);
	uint32_t endcluster=int((secnum-1+sectors)/clustersize);

	uint32_t sectors_in_startblock=clustersize - (secnum % clustersize);
	uint32_t startsector=(secnum%CLUSTERSIZE);

	if ((uint32_t)(sectors)<sectors_in_startblock) { sectors_in_startblock=sectors; }

#if (SMARTIMAGEDEBUG==1)
	cout 
		<< "startcluster="<<startcluster
		<< "endcluster="<<endcluster
		<< "sectors to read from startblock="<<sectors_in_startblock
	<<endl;
#endif
	uint32_t sectors_in_rest=sectors-sectors_in_startblock;
	uint32_t sectors_in_endblock=sectors_in_rest % clustersize;
	uint32_t sectors_in_middleblocks=sectors-(sectors_in_startblock+sectors_in_endblock);

	uint32_t totsectors=0;
	uint32_t clusteroffset=0;

	if (sectors_in_startblock>0)
	{
		totsectors+=readblocksectors(startcluster,startsector,&(buffer[0]),sectors_in_startblock);
	}

	while (sectors_in_middleblocks>0)
	{
		clusteroffset++;	
		sectors_in_middleblocks-=clustersize;	/* always a multiple of cluster size */
		uint32_t currcluster=startcluster+clusteroffset;
		totsectors+=readblocksectors(currcluster,0,&(buffer[clusteroffset*clustersize]),clustersize);
	}

	if (sectors_in_endblock>0)
	{
		clusteroffset++;
		totsectors+=readblocksectors(endcluster,0,&(buffer[clusteroffset*clustersize]),sectors_in_endblock);
	}
	return totsectors;	
}

long hd24driveimage::content_writesectors(uint32_t secnum, unsigned char* buffer,int sectors)
{
#if (SMARTIMAGEDEBUG==1)
	cout <<"hd24driveimage::content_writesectors("
		<<"hnd="<<this->m_handle
		<<",sec="<<secnum
		<<",buf="<<&buffer[0]
		<<",sectors="<<sectors
	<<endl;
#endif
	/* Write a number of sectors to the image. 
           The call is split up so it never crosses block boundaries. */
	uint32_t clustersize=CLUSTERSIZE;  // TODO: get from superblock
	uint32_t startcluster=int(secnum/clustersize);
	uint32_t endcluster=int((secnum-1+sectors)/clustersize);

	uint32_t startsector=(secnum%CLUSTERSIZE);

	uint32_t sectors_in_startblock=clustersize - (secnum % clustersize);
	if ((uint32_t)sectors<sectors_in_startblock) { sectors_in_startblock=sectors; }
	uint32_t sectors_in_rest=sectors-sectors_in_startblock;
	uint32_t sectors_in_endblock=sectors_in_rest % clustersize;
	uint32_t sectors_in_middleblocks=sectors-(sectors_in_startblock+sectors_in_endblock);

	uint32_t totsectors=0;
	uint32_t clusteroffset=0;

	if (sectors_in_startblock>0)
	{
		totsectors+=writeblocksectors(startcluster,startsector,&(buffer[0]),sectors_in_startblock);
	}

	while (sectors_in_middleblocks>0)
	{
		clusteroffset++;	
		sectors_in_middleblocks-=clustersize;	/* always a multiple of cluster size */
		uint32_t currcluster=startcluster+clusteroffset;
		totsectors+=writeblocksectors(currcluster,0,&(buffer[clusteroffset*clustersize]),clustersize);
	}

	if (sectors_in_endblock>0)
	{
		clusteroffset++;
		totsectors+=writeblocksectors(endcluster,0,&(buffer[clusteroffset*clustersize]),sectors_in_endblock);
	}
	return totsectors;
}

void hd24driveimage::initvars()
{
	this->suppresslogs=false;
	this->m_handle=FSHANDLE_INVALID;
}

hd24driveimage::hd24driveimage()
{
	initvars();
}

hd24driveimage::~hd24driveimage()
{

}

/*======================================
   Unit tests
 */

int hd24driveimage::test(string* imagefilename)
{
	cout << "Running internal tests for hd24driveimage for file " << *imagefilename << endl;	
        int initresult=0;
	try {
		initresult=initimage(imagefilename,312581808); // 160GB worth of sectors (mimic ST3160215A)
		if (initresult!=0) return initresult;
	} catch (int e) {
		cout << "Exception " << e << endl;
		return 1;
	}
	cout << "Handle= " << this->m_handle << endl;
	uint32_t endsector=this->getcontentlastsectornum();
	cout << "Last sectornum of contained image is " << endsector << endl;
	uint32_t endsector2=this->getcontainerlastsectornum();
	cout << "Container's size in sectors is " << endsector2 << endl;

	return 0; // assume errors to terminate the process.
}

