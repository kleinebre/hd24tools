#define HD24FSDEBUG 1
#define ALLOC_SECTORS_PER_SONG	5
#define SONG_SECTORS_PER_SONG	2
#define TOTAL_SECTORS_PER_SONG	(ALLOC_SECTORS_PER_SONG+SONG_SECTORS_PER_SONG)
#ifdef DARWIN
#define creat64 creat
#endif
#include "hd24fs.h"
#include "hd24driveimage.h"
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WINDOWS
#include <unistd.h>
#endif
#define RESULT_SUCCESS 0
#define RESULT_FAIL 1
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include "convertlib.h"
#include "memutils.h"
#define ERROR_NOT_OPEN 101
#define ERROR_DRIVEUSAGE_ZERO 102

#include <hd24devicenamegenerator.h>
#define _LARGE_FILES
#define _FILE_OFFSET_BITS 64
#define FILE_OFFSET_BITS 64
#define LARGE_FILES
#define LARGEFILE64_SOURCE
#define SECTORSIZE 512
#ifdef DARWIN
#define open64 open
#define lseek64 lseek
#define pread64 pread
#define pwrite64 pwrite
#endif
#define FSINFO_VERSION_MAJOR 		0x8
#define FSINFO_VERSION_MINOR 		0x9
#define FSINFO_BLOCKSIZE_IN_SECTORS	0x10
#define FSINFO_AUDIOBLOCKS_PER_CLUSTER	0x14
#define FSINFO_STARTSECTOR_DRIVEUSAGE	0x38
#define FSINFO_NUMSECTORS_DRIVEUSAGE	0x3c
#define DEFAULT_SECTORS_DRIVEUSAGE 15
#define FSINFO_CLUSTERWORDS_IN_TABLE	0x40 /* Not sure what this is used for. */
#define FSINFO_FREE_CLUSTERS_ON_DISK	0x44
#define FSINFO_FIRST_PROJECT_SECTOR	0x48
#define FSINFO_SECTORS_PER_PROJECT	0x4C
#define FSINFO_MAXPROJECTS		0x50
#define FSINFO_MAXSONGSPERPROJECT	0x54
#define FSINFO_FIRST_SONG_SECTOR	0x58
#define FSINFO_SECTORS_IN_SONGENTRY	0x5C
#define FSINFO_SECTORS_IN_SONGALLOC	0x60
#define FSINFO_SECTORS_PER_SONG		0x64
#define FSINFO_CURRENT_SONGS_ON_DISK	0x68
#define FSINFO_ALLOCATABLE_SECTORCOUNT  0x80
#define FSINFO_LAST_SECTOR              0x84
#define FSINFO_DATAAREA 		0x7c

#define DRIVEINFO_VOLUME 	0x1b8
#define DRIVEINFO_VOLUME_8 	0x00
#define DRIVEINFO_PROJECTCOUNT	0x0c
#define DRIVEINFO_LASTPROJ	0x10
#define DRIVEINFO_LASTPROJECT	0x10
#define DRIVEINFO_PROJECTLIST	0x20
#define ERROR_INVALID 0xFFFFFFFF
#include "hd24project.cpp"
#include "hd24song.cpp"
#if defined(LINUX) || defined(DARWIN)
const int hd24fs::MODE_RDONLY=O_RDONLY;
const int hd24fs::MODE_RDWR=O_RDWR;
#endif
#ifdef WINDOWS
#include <shellapi.h>
const int hd24fs::MODE_RDONLY=GENERIC_READ;
const int hd24fs::MODE_RDWR=GENERIC_READ|GENERIC_WRITE;
#define popen _popen
#define pclose _pclose
uint32_t hd24fs::bytenumtosectornum(uint64_t flen)
{
	/* This was unit tested to behave identically to
           the previous version.  (note: Windows only!)
           But is (flen-1)/512 what we really mean?
           flen would basically be "file size" so 1..512
           should return 0, 513..1024 should return 1, etc.
	*/
        if (flen==0) return ERROR_INVALID;
        return (uint32_t)((flen-1)/512);
}

uint64_t hd24fs::windrivesize(FSHANDLE hPhysicalDrive)
{
	uint64_t result;
	result=0;
	//LPOVERLAPPED olap=new OVERLAPPED;

	if(hPhysicalDrive==INVALID_HANDLE_VALUE)
	{
		return result;
	}

	DWORD bytesRet;
	DISK_GEOMETRY_EX* outbuffer = (DISK_GEOMETRY_EX*)malloc(5120);//Just to ensure enough space

	if(outbuffer==NULL)
	{
#if (HD24FSDEBUG==1)
		cout<<"Memory Allocation failed"<<endl;
#endif
		return result;
	}

	BOOL ioctlSucceed = DeviceIoControl(
		hPhysicalDrive,
		IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
		NULL,
		0,
		(LPVOID)outbuffer,
		sizeof(*outbuffer),
		(LPDWORD)&bytesRet,
		NULL);

	if(ioctlSucceed)
	{
		LARGE_INTEGER disksize=outbuffer->DiskSize;
		result=disksize.QuadPart;
	}
	memutils::myfree("outbuffer",outbuffer);
	return result;
}
#endif

const int hd24fs::TRANSPORTSTATUS_STOP  =0;
const int hd24fs::TRANSPORTSTATUS_REC   =1;
const int hd24fs::TRANSPORTSTATUS_PLAY  =2;

void hd24fs::dumpsector(const unsigned char* buffer)
{
#if (HD24FSDEBUG==1)
	cout << "dumpsector" << endl;
#endif
	for (int i=0;i<512;i+=16) {
		string* dummy=Convert::int32tohex(i);
	       	cout << *dummy	<< "  "; // PRAGMA allowed, dumpsector
		delete dummy; dummy=NULL;
		for (int j=0;j<16;j++) {
			string* dummy= Convert::byte2hex(buffer[i+j]);
			cout << *dummy; // PRAGMA allowed, dumpsector
			if (j==7) {
				cout << "-" ; // PRAGMA allowed, dumpsector
			} else {
				cout << " " ; // PRAGMA allowed, dumpsector
			}
			delete dummy; dummy=NULL;
		}
		cout << " "; // PRAGMA allowed, dumpsector
		for (int j=0;j<16;j++) {
			cout << Convert::safebyte(buffer[i+j]); // PRAGMA allowed, dumpsector
		}
		cout << "" << endl; // PRAGMA allowed, dumpsector
	}
}

uint32_t hd24fs::songentry2sector(uint32_t songentry)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::songentry2sector(" <<songentry<<")"<< endl;
#endif
	// Given the possibility to store 99*99 songs,
	// converts the song number (0..99*99-1) to
	// the sector where the info of that song starts.
	// TODO: answer is FSINFO_FIRST_SONG_SECTOR+(FSINFO_SECTORS_PER_SONG*songentry)
	// returns 0 when entry number is not legal.
	getsector_bootinfo();
	if (sector_boot==NULL) {
		/*
		   Info needed for the calculation is missing.
		   As boot info should just have been read in the line before
		   this should normally never happen unless we're running out of
                   memory. As such coverage tests will normally miss it.
		*/
		return 0;
	}
	uint32_t firstsongsec=Convert::getint32(sector_boot,FSINFO_FIRST_SONG_SECTOR);
	uint32_t secspersong=Convert::getint32(sector_boot,FSINFO_SECTORS_PER_SONG);
	if (songentry<(99*99)) {
		uint32_t secnum=(firstsongsec+(songentry*secspersong));
		return secnum;
	}
	return 0; // entry number is not legal.
}

bool hd24fs::isdevicefile()
{
	int x=strlen(this->devicename->c_str());
	if (x==0)
	{
		return false;
	}
	if (this->devicename==NULL)
	{
		return false;
	}
	const char* strb=hd24devicenamegenerator::DEVICEPREFIX;
	int y=strlen(strb);

	if (strncmp(this->devicename->c_str(),strb,y)==0)
	{
		return true;
	}
	return false;
}

uint32_t hd24fs::songsondisk()
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::songsondisk()" << endl;
#endif
	getsector_bootinfo();
	if (sector_boot==NULL) {
		// info needed for the calculation is missing.
		return 0;
	}
	return Convert::getint32(sector_boot,FSINFO_CURRENT_SONGS_ON_DISK);
}

void hd24fs::songsondisk(uint32_t songcount)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::songsondisk("<<songcount<<")" << endl;
#endif
	getsector_bootinfo();
	if (sector_boot==NULL) {
		// info needed for the calculation is missing.
		return; //cannot update.
	}
	Convert::setint32(sector_boot,FSINFO_CURRENT_SONGS_ON_DISK,songcount);
}

uint32_t hd24fs::songsector2entry(uint32_t songsector)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::songsector2entry("<<songsector<<")" << endl;
#endif
	// given the sector where a song entry starts,
	// returns the entry number of that song.
	// Entry number typically ranges from 0
	// to (99*99)-1.
	getsector_bootinfo();
	if (sector_boot==NULL) {
		// info needed for the calculation is missing.
		return ERROR_INVALID;
	}
	uint32_t firstsongsec=Convert::getint32(sector_boot,FSINFO_FIRST_SONG_SECTOR);
	uint32_t secspersong=Convert::getint32(sector_boot,FSINFO_SECTORS_PER_SONG);
	uint32_t offset=songsector-firstsongsec;
	if ((offset%secspersong) !=0) {
		return ERROR_INVALID;
	}
	uint32_t resultentry=(offset/secspersong);
	if (resultentry>=(99*99)) return ERROR_INVALID;
	return resultentry;
}

uint32_t hd24fs::cluster2sector(uint32_t clusternum)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::cluster2sector("<<clusternum<<")" << endl;
#endif
	getsector_bootinfo();
	if (sector_boot==NULL) {
		// unknown cluster size.
		return 0;
	}
	return Convert::getint32(sector_boot,FSINFO_DATAAREA) /* first audio sector */
	+(
		Convert::getint32(sector_boot,FSINFO_BLOCKSIZE_IN_SECTORS)
		* Convert::getint32(sector_boot,FSINFO_AUDIOBLOCKS_PER_CLUSTER)
		* clusternum
	);
}

uint32_t hd24fs::sector2cluster(uint32_t sectornum)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::sector2cluster("<<sectornum<<")" << endl;
#endif
	getsector_bootinfo();
	if (sector_boot==NULL) {
		// unknown cluster size. Return 'undefined' cluster number.
		return CLUSTER_UNDEFINED;
	}
	uint32_t dataarea=Convert::getint32(sector_boot,FSINFO_DATAAREA);

	if (sectornum<dataarea) {
		// A cluster number of a sector outside the data area was requested.
		return CLUSTER_UNDEFINED;
	}

	uint32_t unoffsetsector=sectornum-dataarea;
	uint32_t sectorspercluster=
		Convert::getint32(sector_boot,FSINFO_BLOCKSIZE_IN_SECTORS)
		* Convert::getint32(sector_boot,FSINFO_AUDIOBLOCKS_PER_CLUSTER);
	uint32_t firstsectorofcluster=unoffsetsector-(unoffsetsector%sectorspercluster);
	uint32_t cluster=firstsectorofcluster/sectorspercluster;
	return cluster;
}

uint32_t hd24fs::getnextfreesector(uint32_t cluster)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::getnextfreesector("<<cluster<<")" << endl;
#endif
	if (cluster==CLUSTER_UNDEFINED) {
		uint32_t result=getnextfreesectorword();
#if (HD24FSDEBUG==1)
		cout << result << endl;
#endif
		return result;
	}
	if (isfreecluster(cluster+1,&sectors_driveusage[0]))
	{
		return cluster2sector(cluster+1);
	}
	return getnextfreesectorword();
}

uint32_t hd24fs::getnextfreesectorword()
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::getnextfreesectorword()" << endl;
#endif
	uint32_t cluster=getnextfreeclusterword();
	if (cluster==CLUSTER_UNDEFINED) {
		return 0;
	}
	return cluster2sector(cluster);
}

uint32_t hd24fs::getnextfreeclusterword()
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::getnextfreeclusterword()" << endl;
#endif
	getsectors_driveusage();
	uint32_t driveusagecount=driveusagesectorcount();
	uint32_t driveusagewords=driveusagecount*(512/4);

	// For performance reasons, we start searching
	// at last result+1 (this will typically result
	// in an immediate hit).
	// Wrap around to cluster 0 if nothing found.

#if (HD24FSDEBUG==1)
	cout << " start search at " << nextfreeclusterword << endl;
#endif
	uint32_t i=nextfreeclusterword;
	uint32_t initsec=i;

	bool foundfree=false;
	while (i<driveusagewords) {
		if (Convert::getint32(&sectors_driveusage[0],i*4)==0) {
			foundfree=true;
			break;
		}
		i++;
	}
	if (!foundfree) {
		if (initsec==0) {
			// we didnt find anything although
			// we started searching at cluster 0.
			return CLUSTER_UNDEFINED;
		} else {
			// we didnt find anything but started
			// searching at a cluster>0. Wrap around.
			nextfreeclusterword=0;
			return getnextfreeclusterword();
		}
	}
	// we found a (set of 32) free cluster(s).
	nextfreeclusterword=i;
	return (i*32);
}

uint32_t hd24fs::getlastsectornum(int* lastsecerror)
{
	// Will return the last sector num for drives up to 2 TB.
	// Note: lastsecerror will be nonzero if drive size cannot
	// be determined.
	*lastsecerror=0;
#if (HD24FSDEBUG==1)
	cout <<"hd24fs::getlastsectornum(int* lastsecerror)" << endl;
#endif
	// cache result.

	if ((gotlastsectornum==false)||(devhd24!=foundlastsectorhandle)) {
		foundlastsectorhandle=devhd24;
		foundlastsectornum=getlastsectornum(devhd24,lastsecerror);
		if (*lastsecerror==0)
		{
			gotlastsectornum=true;
		}
	}
	return foundlastsectornum;
}

void hd24fs::setwavefixmode(int mode)
{
	wavefixmode=mode;
}

void hd24fs::setmaintenancemode(int mode)
{
	maintenancemode=mode;
}

int hd24fs::getmaintenancemode()
{
	return maintenancemode;
}

string* hd24fs::getdevicename() {

	return this->devicename;
}

void hd24fs::setdevicename(const char* orig,string* devname)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::setdevicename(orig="<<*orig<<", devname="<<*devname<<")" << endl;
#endif
	if (this->devicename!=NULL) {
		delete this->devicename;
		this->devicename=NULL;
	}
	this->devicename=new string(devname->c_str());
}

uint32_t hd24fs::getlastsectornum(FSHANDLE handle,int* lastsecerror)
{
#if (HD24FSDEBUG==1)
	cout <<"hd24fs::getlastsectornum(FSHANDLE handle)" << endl
	 << "handle="<<handle<<endl;
#endif
if (isinvalidhandle(handle))
{
#if (HD24FSDEBUG==1)
	cout << "Handle is not valid." << endl;
#endif
	return 0;
} else {
#if (HD24FSDEBUG==1)
	cout << "Handle is OK" << endl;
#endif
}
if (this!=NULL)
{
	if (handle==this->smartimagehandle)
	{
		if (this->smartimage!=NULL)
		{
			this->smartimage->handle(this->smartimagehandle);
			return this->smartimage->getcontentlastsectornum();
		}
	}
}
#if defined(LINUX) || defined(DARWIN)
	uint64_t curroff=lseek64(handle,0,SEEK_CUR);
	// >>9 equals /512
	uint64_t sects=(lseek64(handle,0,SEEK_END));
#if (HD24FSDEBUG==1)
	cout << "size=" << sects << " bytes " << endl;
#endif
	uint32_t remainder=sects%512;
	sects-=remainder;
	if (remainder!=0) remainder=1;
	sects=(sects>>9)+remainder;
#if (HD24FSDEBUG==1)
	cout << "size=" << sects << " sectors " << endl
	 << "returning lastsec#=" << sects-1  << endl;
#endif
	lseek64(handle,curroff,SEEK_SET);
	return sects-1; // sector num is zero based and thus last sector num=sector count-1.
#endif
	/* unfortunately, apart from other functions being needed
	 * to read 64 bit files, the elegant solution above won't
	 * work on Windows, because it is not possible to
	 * request the file size of a device.
	 */

#ifdef WINDOWS
	unsigned char buffer[2048];
	LARGE_INTEGER lizero;
	lizero.QuadPart=0;
	LARGE_INTEGER saveoff;
	LARGE_INTEGER filelen;
	filelen.QuadPart=0;
	SetFilePointerEx(handle,lizero,&saveoff,FILE_CURRENT); // save offset
#if (HD24FSDEBUG==1)
	cout << "saved fileptr=" << saveoff.QuadPart << endl;
#endif
	/* If things would work properly, the following would
	 * not only work for regular files, but also for devices.
	 * Perhaps the next version of Windows has proper hardware
	 * abstraction and this will magically start working.
	 */
	if (0!=SetFilePointerEx(handle,lizero,&filelen,FILE_END)) {
		long long flen=filelen.QuadPart;
#if (HD24FSDEBUG==1)
	cout << "file end is at " << filelen.QuadPart<<endl;
#endif
		SetFilePointerEx(handle,saveoff,NULL,FILE_BEGIN); // restore offset
		return hd24fs::bytenumtosectornum(flen);
	}
	SetFilePointerEx(handle,saveoff,NULL,FILE_BEGIN); // restore offset
#if (HD24FSDEBUG==1)
	cout << "fileptr method failed" << endl;
#endif
	/* The proper way to do things has failed (probably because
	 * we are dealing with a device file instead of with a
	 * regular one.
	 */
	long long flen=windrivesize(handle);
	if ((flen>0) && ((flen%512)==0))
	{
		return hd24fs::bytenumtosectornum(flen);
	}

	/* TODO: Before trying a raw sectornum scan,
           we can still check if we're dealing with a valid
           superblock, not using a headerfile and checking if
           the last sector as indicated by the superblock points
           to a copy of that superblock. This won't work on a raw
           drive, of course, but will provide a reasonably safe
           workaround for valid HD24 drives.
	*/

	/* .......hmm, do we really want to do the following???? */
#if (HD24FSDEBUG==1)
	cout << "performing raw drive size detection." << endl;
#endif
	/* The workaround has also failed, so we need to perform
           raw drive size detection. Windows does not have a reliable
           way to get the real LBA sector count, so we need to perform
           the following semi-smart trick (if you have a better idea that
	   works on all windows versions and all drive types,
	   I'd be thrilled to know!)

	   - First, iterate reading sector numbers until it fails.
	     To keep speed acceptable, multiply sector number by 2
	     each time.
	   - After failed read, do a binary search between the
	     failed read sector number and the sector number of
	     the last successful read.
	   - As the drive MUST be aware of its own size, this will
             give the real sector count of the drive.
	 */
	lizero.QuadPart=0;
	saveoff.QuadPart=0;

	LARGE_INTEGER maxbytes;
	maxbytes.QuadPart=0;
	SetFilePointerEx(handle,lizero,&saveoff,FILE_CURRENT); // save offset

	long bytesread=1;
	uint32_t trysector=1;
	uint32_t oldtrysector=1;
	int drivetoosmall=1;
	bytesread=readsectors(handle,trysector,buffer,1); // raw/audio read (no fstfix needed)
#if (HD24FSDEBUG==1)
		cout << "bytes read (sector no. 1)=" << bytesread << endl;
#endif
	uint32_t lastok=0;
	uint32_t firstfail=0;
	//int dummy;
	/* This loop doubles the sector number. Actually stays at 2^n-1,
	 * this will likely perform better than 2^n because chances are
	 * greater that it stays below disk boundaries, preventing slow
	 * timeouts.
	 */
	while (bytesread>0)
	{
#if (HD24FSDEBUG==1)
		cout << "trysector=" << trysector << endl;
#endif
//		cin >> dummy;
		drivetoosmall=0;
		oldtrysector=trysector;
		trysector=trysector*2+1; // count will be 1,3,7,15,31,... =(2^n)-1
		if (oldtrysector==trysector)
		{
			// x*2+1 yields x - this means all
			// bits are turned on and we overflow.
			// So we're at 4 tera limit.
			// 4 terabyte and still nothing found? hmmmm
			SetFilePointerEx(handle,saveoff,NULL,FILE_BEGIN);
			return 0;
		}
		bytesread=0;
		bytesread=readsectors(handle,trysector,buffer,1); // raw/audio read (no fstfix needed)
#if (HD24FSDEBUG==1)
		cout << "   bytesread=" <<bytesread << endl;
#endif
		if (bytesread>0) {
			lastok=trysector;
		} else {
			firstfail=trysector;
		}
	}
	/* We have the sector number of the last successful read and
	 * of the failed read. Time to do a binary search. */
	uint32_t lowerbound=lastok;
#if (HD24FSDEBUG==1)
	cout << "lastok=" << lastok << endl
	 << "firstfaiL=" << firstfail << endl;

#endif
	if (lastok==0)
	{
		if (firstfail==0)
		{
			return 0;
		}
	}
	uint32_t upperbound=lastok*2;
	uint32_t midpos=0;
	while (lowerbound<=upperbound)
	{
		//midpos=lowerbound+floor((upperbound-lowerbound)/2);
		midpos=lowerbound+(uint32_t)floor((upperbound-lowerbound)/8);
		// prefer asymmetrical search due to time out when
		// searching past upperbound
		bytesread=readsectors(handle,midpos,buffer,1); // raw/audio read (no fstfix needed)
		if (bytesread>0) {
			lowerbound=midpos+1;
		} else {
			// could not read midpos,
			// so upperbound is before that.
			upperbound=midpos-1;
		}
	}
	if (midpos==lowerbound)
	{
		midpos--;
	}
	/* midpos is now last sector number. counting starts at sector 0
	 * so total number of sectors is one more. We're returning the
         * number of the last sector, however. */
	// midpos++; // NO!
	SetFilePointerEx(handle,saveoff,&saveoff,FILE_BEGIN); //restore

	uint64_t sectors=midpos;
	return sectors;
#endif
}

/* The hd24raw class provides sector level access to hd24 disks
 * which is functionality that is shielded off in hd24fs.
 */

uint32_t hd24raw::songsondisk()
{
	return fsys->songsondisk();
}

uint32_t hd24raw::getlastsectornum(int* lastsecerror)
{
#if (HD24FSDEBUG==1)
	cout << "hd24raw::getlastsectornum()" << endl;
#endif
	return fsys->getlastsectornum(lastsecerror);
}

uint32_t hd24raw::getprojectsectornum(uint32_t x)
{
	return fsys->getprojectsectornum(x);
}

uint32_t hd24raw::quickformat(char* message)
{
#if (HD24FSDEBUG==1)
	cout << "hd24raw::quickformat()" << endl;
#endif
	return fsys->quickformat(message);
}

uint32_t hd24raw::getnextfreesector(uint32_t cluster)
{
	return fsys->getnextfreesector(cluster);
}

hd24raw::hd24raw(hd24fs* p_fsys)
{
	fsys=p_fsys;
}

long hd24raw::readsectors(uint32_t secnum, unsigned char* buffer,int sectors)
{
	return fsys->readsectors(fsys->devhd24, secnum, buffer,sectors);
}

long hd24raw::writesectors(uint32_t secnum, unsigned char* buffer,int sectors)
{
#if (HD24FSDEBUG==1)
	cout << "hd24raw::writesectors()" << endl;
#endif
	return fsys->writesectors(fsys->devhd24, secnum, buffer,sectors);
};

bool hd24fs::isinvalidhandle(FSHANDLE handle)
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

bool hd24fs::isexistingdevice(string* devname)
{
#if defined(LINUX) || defined(DARWIN)
#if (HD24FSDEBUG==1)
cout << "try open device " << devname->c_str() << endl;
#endif
	FSHANDLE handle=open64(devname->c_str(),MODE_RDONLY); //read binary
#endif
#ifdef WINDOWS
	FSHANDLE handle=CreateFile(devname->c_str(),MODE_RDONLY,
	             FILE_SHARE_READ|FILE_SHARE_WRITE,
	             NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#endif
	if (!isinvalidhandle(handle)) return true;
	return false;
}

string* hd24fs::drivetype(string* devname)
{
	/* Does a quickscan reading a minimum of sectors to determine
           whether a drive is most likely to be a PC drive, Mac drive, HD24 drive,
           or something else. This functionality is not to give a "hard"
           judgement about the kind of drive but more to help protect the user
           from formatting the wrong drive by giving a bit of extra info.
        */
        unsigned char findhdbuf[1024];
 	if (devname==NULL)
	{
		devname=getdevicename();
	}
//#if (HD24FSDEBUG_DEVSCAN==1)

//#endif
#if defined(LINUX)
	FSHANDLE handle=open64(devname->c_str(),MODE_RDONLY,0); //read binary
#endif
#if defined(DARWIN)
	FSHANDLE handle=open64(devname->c_str(),MODE_RDONLY); //read binary
#endif
#ifdef WINDOWS

	FSHANDLE handle=CreateFile(devname->c_str(),MODE_RDONLY,
		FILE_SHARE_READ,
		NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#endif

	if (isinvalidhandle(handle)) {
		hd24closedevice(handle,(const char*)"Unknown format");
		return new string("Unknown format");
	}
	this->readsectors(handle, 0, findhdbuf,1);

	if (
		(findhdbuf[510]==0x55)
		&&(findhdbuf[511]==0xaa)
	) {
		hd24closedevice(handle,"PC drive");
		return new string("PC drive");
	}
	if (
  		  (findhdbuf[0]=='T')
		&&(findhdbuf[1]=='A')
		&&(findhdbuf[2]=='D')
		&&(findhdbuf[3]=='A')
		&&(findhdbuf[4]=='T')
		&&(findhdbuf[5]=='S')
		&&(findhdbuf[6]=='F')
	) {
		hd24closedevice(handle,"HD24 drive");
		return new string("HD24 drive");
	}
        if (
                  (findhdbuf[0]=='T')
                &&(findhdbuf[2]=='D')
                &&(findhdbuf[4]=='T')
                &&(findhdbuf[6]=='F')
                &&(findhdbuf[8]==' ')
        ) {
                hd24closedevice(handle,"Corrupt HD24 drive");
                return new string("Corrupt HD24 drive");
        }

	this->readsectors(handle, 1, findhdbuf,1);

	if (
  		  (findhdbuf[16]=='A')
		&&(findhdbuf[17]=='p')
		&&(findhdbuf[18]=='p')
		&&(findhdbuf[19]=='l')
		&&(findhdbuf[20]=='e')

	) {
		hd24closedevice(handle,"Macintosh HD");
		return new string("Macintosh HD");
	}

	hd24closedevice(handle,"Unrecognized format");
	return new string("Unrecognized format");
}

FSHANDLE hd24fs::findhd24device(int mode,string* devname,bool force,bool tryharder)
{
#if (HD24FSDEBUG_DEVSCAN==1)
	cout << "FSHANDLE hd24fs::findhd24device(" << mode << ", " << *devname << ", force=" << force << ",tryharder=" << tryharder <<")" << endl;
#endif
        unsigned char findhdbuf[1024];
        unsigned char compare1buf[1024];
        unsigned char compare2buf[1024];
#if defined(LINUX)
	FSHANDLE handle=open64(devname->c_str(),mode,0); //read binary
#endif
#if defined(DARWIN)
	FSHANDLE handle=open64(devname->c_str(),mode); //read binary
#endif
#ifdef WINDOWS
#if (HD24FSDEBUG_DEVSCAN==1)
	cout << "Mode=" << mode << endl;
#endif
	FSHANDLE handle=CreateFile(devname->c_str(),mode,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#endif

	if (isinvalidhandle(handle)) {
		if (mode==MODE_RDWR)
		{
			// attempt fallback to read-only mode
			// (for CDROM/DVD devices etc)
			mode=MODE_RDONLY;
#if defined(LINUX) || defined(DARWIN)
			handle=open64(devname->c_str(),mode); //read binary
#endif
#ifdef WINDOWS
#if (HD24FSDEBUG==1)
#endif
			handle=CreateFile(devname->c_str(),mode,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#endif

		}


	}

	if (isinvalidhandle(handle)) {
		return handle;
	}

	// can open device.
	uint32_t sectornum=0;
	int lastsecerror=0;
	if (tryharder) {
		sectornum=getlastsectornum(handle,&lastsecerror);
	}

        readsectors_noheader(handle,sectornum,findhdbuf,1); // fstfix follows
	string* rawfstype=Convert::readstring(findhdbuf,0,8);
	fstfix(findhdbuf,512);
        string* fstype=Convert::readstring(findhdbuf,0,8);
#if (HD24FSDEBUG==1)
	cout << "FStype=" <<*fstype<<endl;
#endif
	bool isadat=false;
        if (*rawfstype=="SMARTIMG")
        {
                this->smartimage=new hd24driveimage();
#if (HD24FSDEBUG==1)
		cout << "Mounting FS inside smartimage." << endl;
#endif
		this->smartimagehandle=smartimage->handle(handle);
		delete rawfstype;
		return this->smartimagehandle;
        } else {
		if (this->smartimage!=NULL)
		{
			delete this->smartimage;
		}
		this->smartimage=NULL;
	}
	delete rawfstype;
        if (*fstype=="ADAT FST") {
		/* Okay, but if we are 'trying harder' we can run into
 		   a false positive for old HD24 drives that are now in
		   use as OS drive. So we'll demand that the second and
                   secondlast sector are equal.
		*/
		if (tryharder)
		{
	                readsectors_noheader(handle,sectornum-1,compare1buf,1);
	                readsectors_noheader(handle,1,compare2buf,1);
			if (memcmp(compare1buf,compare2buf,512)==0)
			{
				isadat=true;
			}
			else
			{
				/* Drive may have been an ADAT drive,
				   but no longer is. If it is in fact an
                                   ADAT drive but corrupted, it is still
                                   possible to force detection using force
                                   mode. Give it another chance; do not
                                   enable write prevention for now.
                                */
				isadat=false;
			}
		}
		else
		{
			isadat=true;
		}
	}
	delete fstype;

	if (isadat) return handle;

	if (force)
	{
		forcemode=true;
		m_isOpen=true;
		this->writeprotected=true; // TODO: unless expert mode is enabled too. May also be overridden to allow formatting.
		return handle;
	}
        hd24closedevice(handle,"Invalid handle");
	return FSHANDLE_INVALID;
}

uint32_t hd24fs::hd24devicecount()
{
	/* Attempt to auto-detect a hd24 disk on all IDE and SCSI devices.
           (this should include USB and firewire) */
        FSHANDLE handle;
	int devcount=0;
#if (HD24FSDEBUG==1)
	cout << "====PERFORMING DEVICE COUNT====" << endl;
#endif
	hd24devicenamegenerator* dng=new hd24devicenamegenerator();
	dng->imagedir(this->imagedir);

	uint32_t totnames=dng->getnumberofnames();
        for (uint32_t j=0;j<2;j++)
	{
		// 2 loops: one to try, one to try harder
		// first loop searches strictly valid devices
		// second loop searches for possibly corrupted devices
		bool tryharder=false;
		if (j==1) {
			tryharder=true;
		}
	        for (uint32_t i=0;i<totnames;i++)
		{
			string* devname=dng->getdevicename(i);
			handle=findhd24device(MODE_RDONLY,devname,false,tryharder);
#if (HD24FSDEBUG_DEVSCAN==1)
			cout << "try device no " <<i << "with name" << *devname << "...";
#endif
			delete (devname);
			if (!(isinvalidhandle(handle)))
			{
				devcount++;
				hd24closedevice(handle,"Devscan invalid handle");
			}
	        }

		if (devcount>0) {
			break;
		}
	}
#if (HD24FSDEBUG==1)
	cout << "====END OF DEVICE COUNT, " << devcount << " DEVICES FOUND ====" << endl;
#endif
	delete (dng);
        return devcount;
}

void hd24fs::setimagedir(const char* newdir)
{
if (newdir==NULL)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::setimagedir(NULL)"<< endl;
#endif
}
else
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::setimagedir("<<newdir<<")"<< endl;
#endif
}

	if (this->imagedir!=NULL)
	{
		memutils::myfree("hd24fs::setimagedir()-imagedir",(void*)imagedir);
		this->imagedir=NULL;
	}

	if (newdir!=NULL)
	{
		this->imagedir=(char*)memutils::mymalloc("hd24fs::imagedir",strlen(newdir)+1,1);
		if (this->imagedir!=NULL)
		{
			strncpy((char*)imagedir,newdir,strlen(newdir)+1);
		}
	}

	return; //return (const char*)imagedir;
}

FSHANDLE hd24fs::findhd24device(int mode,int base0devnum)
{
#if (HD24FSDEBUG_DEVSCAN==1)
	cout << "hd24fs::findhd24device(" << mode << "," << base0devnum << ")" << endl;
#endif
	// TODO: Reduce code duplication in this subroutine
	// and hd24devicecount
	/* Attempt to auto-detect a hd24 disk on all known
	   IDE and SCSI devices. (this should include USB
	   and firewire) */
	int currdev=0;
        FSHANDLE handle;
	hd24devicenamegenerator* dng=new hd24devicenamegenerator();
	dng->imagedir(this->imagedir);

	uint32_t totnames=dng->getnumberofnames();
#if (HD24FSDEBUG==1)
	cout << totnames << " devices" << endl;
#endif
        for (uint32_t j=0;j<2;j++)
	{
		// 2 loops: one to try, one to try harder
		// first loop searches strictly valid devices
		// second loop searches for possibly corrupted devices

		bool tryharder=false;
		if (j==1) {
			tryharder=true;
		}
		int devorder=0;
	        for (uint32_t i=0;i<totnames;i++)
		{
			string* devname=dng->getdevicename(i);
			handle=findhd24device(mode,devname,false,tryharder);

			if (!(isinvalidhandle(handle))) {
				if (currdev==base0devnum) {
					// String "3" indicates origin, i.e.
					// who is setting the device name.
					// Useful for debugging purposes.
	                                setdevicename("3",devname);
					deviceid=devorder;
					p_mode=mode;
					delete (devname);
					delete (dng);
					return handle;
				}
				devorder++;
				hd24closedevice(handle,"Invalid handle(3)");
				currdev++;
			}
			delete (devname);
		}
	}
	delete (dng);
        return FSHANDLE_INVALID;
}

void hd24fs::hd24closedevice(FSHANDLE handle,const char* source)
{
	/*
           This function can be called with this=NULL
	   as it can work on non-HD24 filesystems
        */
#if (HD24FSDEBUG==1)
	cout << "hd24fs::closedevice(" << handle <<","<<source<<")" << endl;
#endif
	if (this!=NULL)
	{
		foundlastsectornum=0;
		gotlastsectornum=false;
	}
#if defined(LINUX) || defined(DARWIN)
	close(handle);
#endif
#ifdef WINDOWS
	CloseHandle(handle);
#endif
}

FSHANDLE hd24fs::findhd24device(int mode)
{
	/* Attempt to auto-detect a hd24 disk on all IDE and SCSI devices.
           (this should include USB and firewire) */
	return findhd24device(mode,0);
}

FSHANDLE hd24fs::findhd24device()
{
	return findhd24device(MODE_RDONLY);
}

int hd24fs::gettransportstatus()
{
	return this->transportstatus;
}

void hd24fs::settransportstatus(int newstatus)
{
	this->transportstatus=newstatus;
}

int hd24fs::getdeviceid()
{
	return deviceid;
}

void hd24fs::initvars()
{
	this->foundlastsectorhandle=FSHANDLE_INVALID;
	this->deviceid=-1;

	/* methods readsectors and writesectors will verify these
           and transparently unwrap any "smart" drive images */
	//this->smartimagetype=0;
	this->smartimage=NULL;
	this->smartimagehandle=(FSHANDLE)0;
	///


	this->writeprotected=false; // by default allow writes.
				    // can be disabled if corrupt
				    // state is detected.
	this->transportstatus=TRANSPORTSTATUS_STOP;
	this->imagedir=NULL;
	this->nextfreeclusterword=0;
	this->gotlastsectornum=false;
	this->foundlastsectornum=0;
	this->forcemode=false;
	this->headersectors=0;
	sector_boot=NULL;
	sector_diskinfo=NULL;
	sectors_driveusage=NULL;
	sectors_orphan=NULL;
	sectors_songusage=NULL;
	projlist=NULL;
	this->m_isOpen=false;
	this->allinput=false;
	this->autoinput=false;
	this->wavefixmode=false;
	this->formatting=false;
	this->maintenancemode=false;
	this->headermode=false;
	this->devicename=NULL;
	this->highestFSsectorwritten=0;
	this->needcommit=false;

	// 0x10c76 is last sector of song/project area (without undo buffer)
	return;
}

hd24fs::hd24fs(const char* p_imagedir,int mode)
{
	initvars();
	setimagedir(p_imagedir);
	devicename=new string("");
	devhd24=findhd24device(mode);
	if (!(isinvalidhandle(devhd24))) {
		m_isOpen=true;
		p_mode=mode;
	}
	return;
}

hd24fs::hd24fs(const char* p_imagedir,int mode,int base0devnum)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::hd24fs(" << mode << "," << base0devnum << ")" << endl;
#endif
	initvars();
	setimagedir(p_imagedir);
	devicename=new string("");
	devhd24=findhd24device(mode,base0devnum);
	if (!(isinvalidhandle(devhd24))) {
		m_isOpen=true;
		p_mode=mode;
	}
	return;
}

hd24fs::hd24fs(const char* p_imagedir,int mode,string* devname,bool force)
{
	initvars();
	setimagedir(p_imagedir);
	devicename=new string(devname->c_str());
	bool tryharder=false;
	devhd24=findhd24device(mode,devname,force,tryharder);
	if (!(isinvalidhandle(devhd24))) {
		m_isOpen=true;
		p_mode=mode;
	}
	return;
}

hd24fs::hd24fs(const char* p_imagedir)
{
	initvars();
	setimagedir(p_imagedir);
	devicename=new string("");
	devhd24=findhd24device(MODE_RDONLY);
	if (!(isinvalidhandle(devhd24))) {
		m_isOpen=true;
		p_mode=MODE_RDONLY;
	}
	return;
}

hd24fs::~hd24fs()
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::~hd24fs();" << endl;
#endif
	if (this->devicename!=NULL) {
		delete this->devicename;
		this->devicename=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Commit and close FS handle (if isopen)" << endl;
#endif
	if (isOpen())
	{
		//if (!this->isdevicefile())
		commit();
		hd24closedevice(this->devhd24,"hd24close (after doing a commit)");
	}
#if (HD24FSDEBUG==1)
	cout << "Free superblock mem" << endl;
#endif
	if (sector_boot!=NULL)
	{
		memutils::myfree("sectors_boot",sector_boot);
		sector_boot=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free diskinfo mem" << endl;
#endif
	if (sector_diskinfo!=NULL)
	{
		memutils::myfree("sectors_diskinfo",sector_diskinfo);
		sector_diskinfo=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free drive usage mem" << endl;
#endif
	if (sectors_driveusage!=NULL)
	{
		memutils::myfree("sectors_driveusage",sectors_driveusage);
		sectors_driveusage=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free orphan sectors mem" << endl;
#endif
	if (sectors_orphan!=NULL)
	{
		memutils::myfree("sectors_orphan",sectors_orphan);
		sectors_orphan=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free song usage sectors mem" << endl;
#endif
	if (sectors_songusage!=NULL)
	{
		memutils::myfree("sectors_songusage",sectors_songusage);
		sectors_songusage=NULL;
	};
	if (this->imagedir!=NULL)
	{
		memutils::myfree("~hd24fs::imagedir",(void*)this->imagedir);
		this->imagedir=NULL;
	}
	this->hd24sync();
}

bool hd24fs::isOpen()
{
	if (this->m_isOpen)
	{
		return true;
	}
	return false;
}

void hd24fs::fstfix(unsigned char * bootblock,int fixsize)
{
	if (bootblock==NULL) return;
	if (fixsize<=0) return;
#if (HD24FSDEBUG==1)
	cout << "fstfix("<<bootblock<<","<<fixsize/512 <<"*512)"<< endl;
#endif
        for (int i=0;i<fixsize;i+=4)
	{
                unsigned char a=bootblock[i];
                unsigned char b=bootblock[i+1];
                unsigned char c=bootblock[i+2];
                unsigned char d=bootblock[i+3];
                bootblock[i]=d;
                bootblock[i+1]=c;
               	bootblock[i+2]=b;
        	bootblock[i+3]=a;
	}
}

void hd24fs::hd24seek(FSHANDLE devhd24,uint64_t seekpos) {
#if defined(LINUX) || defined(DARWIN)
	lseek64(devhd24,seekpos,SEEK_SET);
#endif
#ifdef WINDOWS
	LARGE_INTEGER li;
	li.HighPart=seekpos>>32;
	li.LowPart=seekpos%((uint64_t)1<<32);
//LowPart=seekpos%
//	SetFilePointer(devhd24,seekpos,NULL,FILE_BEGIN);
	SetFilePointerEx(devhd24,li,NULL,FILE_BEGIN);
	// TODO: SetFilePointer vs SetFilePointerEx
	// DWORD SetFilePointer(
	//   HANDLE hFile,
	//     LONG lDistanceToMove,
	//       PLONG lpDistanceToMoveHigh,
	//         DWORD dwMoveMethod
	//         );
	//
	// vs.
	// BOOL SetFilePointerEx(
	//   HANDLE hFile,
	//     LARGE_INTEGER liDistanceToMove,
	//       PLARGE_INTEGER lpNewFilePointer,
	//         DWORD dwMoveMethod
	//         );
	//
#endif
	return;
}

long hd24fs::writesectors(FSHANDLE devhd24,uint32_t sectornum,unsigned char * buffer,int sectors)
{
	//////
	// this bit keeps track of the highest FS sector written
	// to keep commit times acceptable: commit function will then
	// only backup up to the highest sector used.
	// (this is still suboptimal performance-wise but requires
	// a minimum of memory and administration during writes).

        // During a quickformat, we don't have a filesystem object
        // and thus no 'this->highestFSsectorwritten'
        // but the whole lot needs to be committed then anyway
	// so no point in keeping track of it.


#if (HD24FSDEBUG==1)
		cout << "   writesectors sectornum " << sectornum << ", sectorcount=" << sectors << endl;
#endif
	if (this!=NULL)
	{
		if (this->writeprotected)
		{
#if (HD24FSDEBUG==1)
			cout << "Write protected- not writing. " << endl;
#endif
			return 0;
		}
	}
	FSHANDLE currdevice=devhd24;
	FSHANDLE mysmartimagehandle=devhd24;
	if (this!=NULL)
	{
		uint32_t lastsec=sectornum+(sectors-1);
		if (lastsec<=0x10c76)
		{
			// 0x10c76 is normally last sector of song/project
			// are (not counting the undo buffer).
			// TODO: calculate based on superblock info.
			// if current sector is inside song/project area,
			// remember it to speed up commits.
			if (lastsec>highestFSsectorwritten)
			{
				highestFSsectorwritten=lastsec;
			}
		}
	}
	//////

#if (HD24FSDEBUG==1)
	cout << "WRITESECTORS sec=" << sectornum << " buf=" << buffer << "#=" << sectors << endl;
#endif
	int WRITESIZE=SECTORSIZE*sectors; // allows searching across sector boundaries
	int setheader=0;
	if (this!=NULL)
	{
		mysmartimagehandle=this->smartimagehandle;
		if ((this->headersectors)!=0)
		{
			// Header mode is active. Only allow writing over header area.
			if (sectornum<this->headersectors)
			{
				currdevice=hd24header;
				setheader=1;
			} else {
				// headermode is active yet caller is trying to write over drive
				// data area. We cannot allow this (for safety reasons).
				return 0;

			}
		} else {
		}
	}
	if (setheader==0)
	{
		if (smartimage!=NULL)
		{
//			cout << "Writing sectors to smartimage content." << endl;
			FSHANDLE oldhandle=smartimage->handle();
			smartimage->handle(mysmartimagehandle);
			uint32_t wresult=512*(smartimage->content_writesectors(sectornum,buffer,sectors));
			smartimage->handle(oldhandle);
			return wresult;
		}
	}

	hd24seek(currdevice,(uint64_t)sectornum*512);
	if (this!=NULL)
	{
		this->needcommit=true;
	}
#if defined(LINUX) || defined(DARWIN) || defined(__APPLE__)
       long bytes=pwrite64(currdevice,buffer,WRITESIZE,(uint64_t)sectornum*512); //1,devhd24);
#endif
#ifdef WINDOWS
	DWORD dummy;
	long bytes=0;
	if (WriteFile(currdevice,buffer,WRITESIZE,&dummy,NULL)) {
		bytes=WRITESIZE;
	};
#endif
       	return bytes;
}

long hd24fs::readsectors(FSHANDLE devhd24,uint32_t sectornum,unsigned char * buffer,int sectors)
{
	int sectorcount=sectors;
#if (HD24FSDEBUG==1)
	cout << "hd24fs::readsectors(devhd24="<<devhd24<<",sectornum="<<sectornum<<",buf="<<buffer<<",sc#="<<sectorcount<<");"<< endl;
#endif

	FSHANDLE currdevice=devhd24;
	FSHANDLE mysmartimagehandle=devhd24;
	int setheader=0;
	if (this!=NULL) {
		if ((this->headersectors)!=0)
		{
			if (sectornum<this->headersectors)
			{
				currdevice=hd24header;
				setheader=1;
			}
		}
		mysmartimagehandle=this->smartimagehandle;
  	}
	if (setheader==0)
	{
		if (this!=NULL) {
			if (smartimage!=NULL)
			{
	//			cout << "Reading "<<sectorcount<<" sectors from smartimage content." << endl;
				FSHANDLE oldhandle=smartimage->handle();
				smartimage->handle(mysmartimagehandle);
				uint32_t intresult=512*(
				smartimage->content_readsectors(sectornum,buffer,sectorcount));

				smartimage->handle(oldhandle);
				return intresult;
			}
		}
	}
	// without "this" defined, we can only read from the actual handle given.
       	hd24seek(currdevice,(uint64_t)sectornum*SECTORSIZE);
       int READSIZE=SECTORSIZE*(sectorcount);
#if defined(LINUX) || defined(DARWIN)
       long bytes_read=pread64(currdevice,buffer,READSIZE,(uint64_t)sectornum*512); //1,currdevice);
#endif
#ifdef WINDOWS
	DWORD bytes_read;
	//long bytes=0;
	if( ReadFile(currdevice,buffer,READSIZE,&bytes_read,NULL)) {
	} else {
		bytes_read = 0;
	}
#endif
        return bytes_read;
}

long hd24fs::readsectors_noheader(hd24fs* currhd24,uint32_t sectornum,unsigned char * bootblock,uint32_t count)
{
	return readsectors_noheader(currhd24->devhd24,sectornum,bootblock,count);
}

/*long hd24fs::readsector_noheader(FSHANDLE devhd24,uint32_t sectornum,unsigned char * bootblock)
{
	if (this==NULL) {
		readsectors(devhd24,sectornum,bootblock,1);
	}
	uint32_t headersecs=this->headersectors;

	this->headersectors=0; // disable header processing, if applies
	long number_read=readsectors(devhd24,sectornum,bootblock,1);
	this->headersectors=headersecs; // re-enable header processing
	return number_read;
}
*/
long hd24fs::readsectors_noheader(FSHANDLE devhd24,uint32_t sectornum,unsigned char * bootblock,uint32_t count)
{
	uint32_t headersecs=this->headersectors;
	this->headersectors=0; // disable header processing, if applies
	long number_read=readsectors(devhd24,sectornum,bootblock,count);
	this->headersectors=headersecs; // re-enable header processing
	return number_read;
}
/*
long hd24fs::writesector(FSHANDLE devhd24,uint32_t sectornum,unsigned char * bootblock)
{
	return writesectors(devhd24,sectornum,bootblock,1);
}
*/
string* hd24fs::gethd24currentdir(int argc,char* argv[])
{
	/* For future use. We may save a file in the
  	   homedir of the user containing info about which
 	   "path" (project/songname/file format) was last
	   selected by the user.
	*/

       return new string("/");
}

string* hd24fs::gethd24currentdir()
{
       return new string("/");
}

unsigned char* hd24fs::readdiskinfo()
{
#if (HD24FSDEBUG==1)
	// cout << "Driveusage before readdiskinfo=" << (int)(this->sectors_driveusage) << endl;
#endif
	// read disk info
	if (/*formatting||*/(sector_boot==NULL))
	{
		this->readbootinfo();
	}
	if (sector_diskinfo!=NULL)
	{
		memutils::myfree("readdiskinfo",sector_diskinfo);
		sector_diskinfo=NULL;
	}
	sector_diskinfo=(unsigned char *)memutils::mymalloc("readdiskinfo",1024,1);
	if (sector_diskinfo!=NULL)
	{
		readsectors(devhd24,1,sector_diskinfo,1); // fstfix follows
		fstfix (sector_diskinfo,512);
	}
#if (HD24FSDEBUG==1)
	// cout << "Driveusage after readdiskinfo=" << (int)(this->sectors_driveusage) << endl;
#endif
	return sector_diskinfo;
}

bool hd24fs::useheaderfile(string headerfilename)
{
	getsector_bootinfo();
	if (sector_boot==NULL)
	{
		/* we haven't got a main device yet
		   so we cannot apply a header to it */
		return false;
	}
	// allow writing to header.
	// TODO: Explain Why? is this safe? What about r/o file systems?
#if defined(LINUX) || defined(DARWIN)
	FSHANDLE handle=open64(headerfilename.c_str(),MODE_RDWR); //read binary
#endif
#ifdef WINDOWS
	FSHANDLE handle=CreateFile(headerfilename.c_str(),MODE_RDWR,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#endif

	if (isinvalidhandle(handle)) return false;
	hd24header=handle;
	this->headersectors=0;
	int lastsecerror=0;
	this->headersectors=getlastsectornum(hd24header,&lastsecerror)+1;
	// re-read disk info as number of projects etc can differ with header
	readsectors(devhd24,1,sector_diskinfo,1); // fstfix follows
	fstfix (sector_diskinfo,512);

	return true;
}

void hd24fs::clearbuffer(unsigned char* buffer,uint32_t bytes)
{
	/* clear buffer */
	for (uint32_t i=0;i<bytes;i++) {
		buffer[i]=0;
	}
	return;
}

void hd24fs::clearbuffer(unsigned char* buffer)
{
	clearbuffer(buffer,512);
}


void hd24fs::cleardriveinfo(unsigned char* buffer)
{
	clearbuffer(buffer);
	string drivename="Drive Name";
	this->setname(buffer,drivename,DRIVEINFO_VOLUME_8,DRIVEINFO_VOLUME);
	return;
}

void hd24fs::useinternalboot(unsigned char* buffer,uint32_t lastsector)
{
	unsigned char internal_boot[136]=
	{
	0x54,0x41,0x44,0x41,0x54,0x53,0x46,0x20, 0x20,0x30,0x31,0x31,0x33,0xcc,0xaa,0x55,
	0x80,0x04,0x00,0x00,0x02,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
	0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00, 0x05,0x00,0x00,0x00,0x0f,0x00,0x00,0x00,
	0x2f,0x04,0x00,0x00,0x00,0x00,0x00,0x00, 0x14,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
	0x63,0x00,0x00,0x00,0x63,0x00,0x00,0x00, 0x77,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
	0x05,0x00,0x00,0x00,0x07,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x49,0x26,0x00,0x00,
	0x76,0x0c,0x01,0x00,0x80,0x8b,0x12,0x00, 0x1f,0x04,0x00,0x00,0xf6,0x97,0x13,0x00,
	0x14,0x89,0xb4,0x04,0x7f,0x2d,0xc9,0x04
	};

	clearbuffer(buffer);

	/* fill buffer with default boot info */
	for (uint32_t i=0;i<sizeof(internal_boot);i++) {
		buffer[i]=internal_boot[i];
	}

	/* If a specific FS size (in sectors) was given, update boot info
	   to match that size */
	if (lastsector!=0)
	{
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Calculating number of clusters on the drive" << endl;
#endif
          	fstfix(buffer,512); // convert into editable format

		Convert::setint32(buffer,FSINFO_LAST_SECTOR,lastsector);
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Last sector=" << lastsector << endl;
#endif
		// allocatable sectors=total sectors-(2*fs sectors+undo area)
		// tot secs-0x14a46b

		uint32_t allocatablesectors=lastsector-0x14a46b;
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "# allocatable sectors=" << allocatablesectors << endl;
#endif
		Convert::setint32(buffer,FSINFO_ALLOCATABLE_SECTORCOUNT,allocatablesectors);


		// (15 sectors of drive usage info=((15*512)-8)*8 bits
		//
		uint32_t maxclusters=((15 /*sectors of alloc info*/
				       *512 /*bytes*/)
				       -8 /* checksum bytes */)
					*8 /* bits per byte */;
		// represents ~ 61376 allocatable clusters
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "max # of clusters=" << maxclusters << endl;
#endif
		uint32_t allocatableaudioblocks=(allocatablesectors - (allocatablesectors%0x480))/0x480;
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "allocatable audio blocks=" << allocatableaudioblocks << endl;
#endif
		// given the maximum number of clusters
		// and the total number of allocatable audio blocks,
		// we must decide on the number of audio blocks per cluster.
		uint32_t rest=0;
		if ((allocatableaudioblocks%maxclusters)>0) rest++;
		uint32_t blockspercluster=rest+((allocatableaudioblocks-(allocatableaudioblocks%maxclusters))/maxclusters);
		Convert::setint32(buffer,FSINFO_AUDIOBLOCKS_PER_CLUSTER,blockspercluster);

		uint32_t divider=(1152*32*blockspercluster);
		uint32_t chunksizemod=allocatableaudioblocks % divider;
		uint32_t chunksize=((allocatablesectors-chunksizemod)/divider);
		if (chunksizemod>0) {chunksize++; }
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "blocks per cluster="<<blockspercluster
		     << "divider=1152*32*blockspercluster=" <<divider
		     << "'chunksize'=" << chunksize
		    << endl;
#endif
		Convert::setint32(buffer,FSINFO_CLUSTERWORDS_IN_TABLE,chunksize);
		// Not sure what this is used for.
		// However presumably space is allocated in chunks of 32 clusters,

		// offset 0x14h: number of audio blocks per cluster
		uint32_t allocatableclusters=(allocatableaudioblocks-(allocatableaudioblocks%blockspercluster))/blockspercluster;
		Convert::setint32(buffer,FSINFO_FREE_CLUSTERS_ON_DISK,allocatableclusters);
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "allocatableclusters="<<allocatableclusters<< endl;
#endif
          	fstfix(buffer,512); // convert back into native format
	}

	/* Calculate the proper checksum for the bootinfo */
	setsectorchecksum(buffer,
		0 /* startoffset */,
		0 /* sector */,
		1 /*sectorcount */
	);
	return;
}

unsigned char* hd24fs::readbootinfo()
{
	// read boot info

	if (sector_boot==NULL)
	{
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Malloc space for bootsector" << endl;
#endif
		sector_boot=(unsigned char *)memutils::mymalloc("readbootinfo",1024,1);
	}

	if (sector_boot!=NULL)
	{
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Malloc space for bootsector succeeded" << endl
		<< "forcemode=" << forcemode << endl;
#endif
		if (formatting||(!forcemode)) {
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Reading sector (From disk)" << endl;
#endif
			readsectors(devhd24,0,sector_boot,1);
		} else {
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Using internal boot." << endl;
#endif
			useinternalboot(sector_boot,0);
		}
          	fstfix(sector_boot,512);
	} else {
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Malloc space for bootsector failed" << endl;
#endif
	}
#if (HD24FSDEBUG_QUICKFORMAT==1)
	hd24utils::dumpsector((const char*)sector_boot);
#endif
	return sector_boot;
}

unsigned char* hd24fs::readdriveusageinfo()
{
#if (HD24FSDEBUG==1)|| (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "hd24fs::readdriveusageinfo()" << endl;
#endif

	// read file allocation table/disk usage table
	uint32_t driveusagecount=driveusagesectorcount();
	if (sectors_driveusage==NULL)
	{

		sectors_driveusage=(unsigned char *)memutils::mymalloc("readdriveusageinfo/sectors_driveusage",512*(driveusagecount+1),1);
	}
	if (sectors_driveusage!=NULL)
	{
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout 	<< "hd24fs::readdriveusageinfo() reading "
		<< driveusagecount << "sectors into buffer at "
		<< &sectors_driveusage << endl;
#endif
		if (driveusagecount==0)
		{

		}
		readsectors(devhd24,driveusagefirstsector(),sectors_driveusage,driveusagecount);
		fstfix(sectors_driveusage,512*driveusagecount);

	}
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "Dumping newly read sector to screen:" << endl ;
	hd24utils::dumpsector((const char*)sectors_driveusage);
#endif
	return sectors_driveusage;
}

unsigned char* hd24fs::resetsongusage()
{
	/* Reset song usage table. First we occupy all of it;
	   then we make only accessible entries available.
	   Called by calcsongusage and writeemptysongusagetable.
	*/
	if (sectors_songusage==NULL)
	{
		sectors_songusage=(unsigned char *)memutils::mymalloc("resetsongusage",512*3,1);
		if (sectors_songusage==NULL)
		{
			return NULL;
		}
	}

	if (sectors_songusage!=NULL)
	{
		for (int i=0;i<512*3;i++)
		{
			sectors_songusage[i]=0xff;
		}
	}

	uint32_t i;
	// table is initialized, now populate it.
	uint32_t maxprojs=this->maxprojects();
	uint32_t maxsongcount=this->maxsongsperproject();
	uint32_t totentries=maxprojs*maxsongcount; // 99 songs, 99 projects
#if (HD24FSDEBUG==1)
	cout << "Clear song usage table..."<< endl
	<< "this=" << this << endl;
#endif
	for (i=0;i<totentries;i++) {
		disablebit(i,sectors_songusage);
		// this is based on song sectors.
		// That is, entry 0=sec 0x77,
		// entry 1=sec 0x77+7, etc.
	}
#if (HD24FSDEBUG==1)
	cout << "Usage table cleared." << endl;
#endif
	return sectors_songusage;
}

unsigned char* hd24fs::resetdriveusage()
{
#if (HD24FSDEBUG==1)||(HD24FSDEBUG_QUICKFORMAT==1)
	cout << "hd24fs::resetdriveusage()" << endl;
#endif
	// Reset drive usage table. First we occupy all of it;
	// then we make only accessible entries available.

	if (this->sectors_driveusage==NULL)
	{
		unsigned char* du=(unsigned char *)memutils::mymalloc("resetdriveusage/sectors_driveusage",512*15,1);
		sectors_driveusage=du;
		if (this->sectors_driveusage==NULL)
		{
			return NULL;
		}
	}

	for (int i=0;i<512*15;i++)
	{
		sectors_driveusage[i]=0xff;
	}

	if (formatting||(sector_boot==NULL) )
	{
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "hd24fs::resetdriveusage() - reloading boot info " << endl;
#endif

		this->readbootinfo();
	} else {
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "hd24fs::resetdriveusage() - using current (not reloading) boot info " << endl;
#endif

	}

	uint32_t i;
	// table is initialized, now populate it.
	uint32_t totentries=Convert::getint32(sector_boot,FSINFO_FREE_CLUSTERS_ON_DISK);
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "According to superblock, free clusters on disk=" << totentries << endl;
	hd24utils::dumpsector((const char*)sector_boot);
#endif
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "before reset, drive usage looks as follows: "<< endl;
	hd24utils::dumpsector((const char*)sectors_driveusage);
#endif
	for (i=0;i<totentries;i++) {
		disablebit(i,sectors_driveusage);
	}
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "after reset, drive usage looks as follows: "<< endl;
	hd24utils::dumpsector((const char*)sectors_driveusage);
#endif

#if (HD24FSDEBUG==1)
	cout << "Drive usage table cleared." << endl;
#endif
	return sectors_driveusage;
}

unsigned char* hd24fs::calcsongusage()
{
	/*
		Recalculate song usage table based on actual usage by song entries of all projects.
		TODO: clean up.
	*/
	resetsongusage();

	hd24project* currproj=NULL;
	uint32_t projcount=projectcount();
	uint32_t i;
	uint32_t j;
	for (i=1; i<=projcount;i++)
	{
		if (currproj!=NULL)
		{
			delete(currproj);
			currproj=NULL;
		}
		currproj=getproject(i);
		if (currproj==NULL)
		{
			continue;
		}

		// currproj!=NULL.
		uint32_t currsongcount=currproj->songcount();
		for (j=1; j<=currsongcount;j++)
		{
			// get song sector info.
			uint32_t songsector = currproj->getsongsectornum(j);
			if (songsector==0)
			{
				// song at given entry is not in use
				// in this project, no need to mark it as used.
				continue;
			}

			// mark the song used based on its entry number
			// (calculated from the sector where it lives)
			uint32_t songentry=songsector2entry(songsector);
			if (songentry!=INVALID_SONGENTRY)
			{
				// given song entry is used.
				enablebit(songentry,sectors_songusage);
			}
		}

		if (currproj!=NULL)
		{
			delete(currproj);
			currproj=NULL;
		}
	}
	if (currproj!=NULL)
	{
		delete(currproj);
		currproj=NULL;
	}

	return sectors_songusage;
}

void hd24fs::rewritesongusage()
{
	unsigned char* songusage=calcsongusage();
	uint32_t sectornum=2; /* TODO: get from fs */
	uint32_t sectorcount=3; /* TODO: get from fs */
	fstfix(songusage,sectorcount*512);
	setsectorchecksum(songusage,
		0 /* startoffset */,
		sectornum /* sector */,
		sectorcount /*sectorcount */
	);
	this->writesectors(this->devhd24,
			sectornum,
			songusage,
			sectorcount);
	fstfix(songusage,sectorcount*512);

	/* this also implies we need to update the superblock
	   with the current song count */
	uint32_t songcount=0;
	for (int i=0;i<99*99;i++) {
		if (!(isbitzero(i,songusage)))
		{
			songcount++;
		}
	};
	songsondisk(songcount);
	fstfix(sector_boot,512);
	setsectorchecksum(sector_boot,
		0 /* startoffset */,
		0 /* sector */,
		1 /*sectorcount */);
	this->writesectors(this->devhd24,
			0,
			sector_boot,
			1);
	fstfix(sector_boot,512);
	return;
}

unsigned char* hd24fs::getsector_diskinfo()
{
	getsector_bootinfo();
	unsigned char* targetbuf=sector_diskinfo;
	if (/*formatting||*/(sector_diskinfo==NULL) )
	{
		targetbuf=readdiskinfo();
	}
	return targetbuf;
}

unsigned char* hd24fs::getsector_bootinfo()
{
	unsigned char* targetbuf=sector_boot;
	if (/*formatting||*/(sector_boot==NULL) )
	{
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Re-reading bootinfo now." << endl;
#endif
		targetbuf=readbootinfo();
	}
	return targetbuf;
}

unsigned char* hd24fs::getsectors_driveusage()
{
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "hd24fs::getsectors_driveusage()" << endl;
#endif

	readbootinfo();
	if (sectors_driveusage==NULL)
	{
		sectors_driveusage=readdriveusageinfo();
	}
	return sectors_driveusage;
}

unsigned char* hd24fs::getcopyofusagetable()
{
	unsigned char* copyusagetable=(unsigned char*)memutils::mymalloc("copyusagetable",15*512,1);
	if (copyusagetable==NULL)
	{
		/* Out of memory */
		return NULL;
	}

	// copy current drive usage table to a copy;
	readdriveusageinfo();
	int i;
	for (i=0;i<(512*15);i++) {
		copyusagetable[i]=sectors_driveusage[i];
	}
	return copyusagetable;
}

string* hd24fs::volumename()
{
	if (!(isOpen()))
	{
	      	return new string("");
	}
	getsector_diskinfo();
	return Convert::readstring(sector_diskinfo,DRIVEINFO_VOLUME,64);
}

void hd24fs::setvolumename(string newname)
{
	if (sector_diskinfo==NULL)
	{
		readdiskinfo();
	}
	this->setname(sector_diskinfo,newname,DRIVEINFO_VOLUME_8,DRIVEINFO_VOLUME);
	return;
}

uint32_t hd24fs::driveusagesectorcount()
{
	if (!(isOpen()))
	{
		throw ERROR_NOT_OPEN;
	}
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Figuring out number of sectors used for drive usage"
		<< endl;
#endif

	getsector_bootinfo();
	int sectors_driveusage=Convert::getint32(sector_boot,FSINFO_NUMSECTORS_DRIVEUSAGE);
	if (sectors_driveusage==0)
	{
		return DEFAULT_SECTORS_DRIVEUSAGE;
	}
	return sectors_driveusage;

}

uint32_t hd24fs::clustercount()
{
	/*
           Clustercount=
		(number of allocatable sectors on disk)/
                ((sectors per audioblock)*(audioblocks per cluster))
        */
	if (!(isOpen()))
	{
		return 0;
	}
	getsector_bootinfo();
	uint32_t allocatablesectorcount=
	    Convert::getint32(sector_boot,FSINFO_ALLOCATABLE_SECTORCOUNT);
	uint32_t audioblocksize=
	    Convert::getint32(sector_boot,FSINFO_BLOCKSIZE_IN_SECTORS);
        uint32_t clustersize=
 	    Convert::getint32(sector_boot,FSINFO_AUDIOBLOCKS_PER_CLUSTER)
            *audioblocksize;

        allocatablesectorcount-=(allocatablesectorcount%clustersize);
	return allocatablesectorcount/clustersize;
}

void hd24fs::dumpclusterusage(unsigned char* usagebuffer)
{
	if (!(isOpen()))
	{
		return;
	}
	uint32_t clusters=clustercount();
#if (HD24FSDEBUG==1)
	cout << "Dumping cluster usage for "<<clusters<<" clusters. "<<endl;
#endif
	for (uint32_t i=0;i<clusters;i++) {
		if (isfreecluster(i,usagebuffer)) {
			cout << "0"; // PRAGMA allowed
		} else {
			cout << "1"; // PRAGMA allowed
		}
	}
	cout <<endl; // PRAGMA allowed
	return;
}

void hd24fs::dumpclusterusage2(unsigned char* usagebuffer)
{
	uint32_t clusters=clustercount();
	uint32_t currpos=0;
	cout << "DumpClusterUsage2" << endl; // PRAGMA allowed
	while (currpos<clusters) {
		uint32_t blockstart=currpos;
		while (isfreecluster(blockstart,usagebuffer) && (blockstart<clusters)) {
			blockstart++;
		}
		cout << "Block starts at cluster " <<blockstart << endl; // PRAGMA allowed
		if (blockstart==clusters) {
			break;
		}

		// blockstart now points to a nonfree cluster
		uint32_t blockend=blockstart;
		while (!isfreecluster(blockend,usagebuffer) && (blockend<clusters)) {
			blockend++;
		}
		// blockend now points to a free cluster
		currpos=blockend;
		uint32_t blocklen=blockend-blockstart;
		printf("%x %x\n",(uint32_t) cluster2sector(blockstart),(uint32_t)( getblockspercluster()*blocklen ));
	}
}

uint32_t hd24fs::driveusagefirstsector()
{
	if (!(isOpen()))
	{
		return 0;
	}
	getsector_bootinfo();

	return Convert::getint32(sector_boot,FSINFO_STARTSECTOR_DRIVEUSAGE);
}

unsigned char* hd24fs::findorphanclusters()
{
	if (!(isOpen()))
	{
		return NULL;
	}
	uint32_t driveusagecount=driveusagesectorcount();
	getsectors_driveusage();
	int numprojs=projectcount();

	if (sectors_orphan==NULL) {
		// only allocate once (free on object destruct)
		sectors_orphan=(unsigned char *)memutils::mymalloc("findorphanclusters",512*(driveusagecount+1),1);
	}
	readsectors(devhd24,driveusagefirstsector(),sectors_orphan,driveusagecount);
	fstfix(sectors_orphan,512*driveusagecount);

	for (int proj=1; proj<=numprojs; proj++) {
		hd24project* currproj=this->getproject(proj);
                if (currproj == NULL) continue;
		int numsongs=currproj->songcount();
		for (int song=1; song<=numsongs; song++) {
			hd24song* currsong=currproj->getsong(song);
			if (currsong==NULL) continue;
			currsong->unmark_used_clusters(sectors_orphan);
			delete currsong;
		}
		if (currproj!=NULL) {
			delete currproj;
			currproj=NULL;
		}
	}
	return sectors_orphan;
}

bool hd24fs::isbitzero(uint32_t i,unsigned char* usagebuffer)
{
	int bitnum=i%32;
	i-=bitnum;
	i/=32;	// i now is word num
	i*=4;	// i now is offset
	uint32_t getword=Convert::getint32(usagebuffer,i);
	uint32_t mask=1;
#if (HD24FSDEBUG_BITSET==1)
//	cout << "bitnum=" << bitnum << " ";
#endif
	mask=mask<<bitnum;
	uint32_t bitval=(getword & mask);
#if (HD24FSDEBUG_BITSET==1)
//	cout << "bitval=" << bitval << endl;
#endif
	if (bitval == 0) return true;
	return false;
}

void hd24fs::enablebit(uint32_t ibitnum,unsigned char* usagebuffer)
{
#if (HD24FSDEBUG_BITSET==1)
	cout << "enable bit " << ibitnum << endl;
#endif
	int bitnum=ibitnum%32;
	ibitnum-=bitnum;
	ibitnum/=32;	// i now is word num
	ibitnum*=4;	// i now is byte offset of word
	uint32_t getword=Convert::getint32(usagebuffer,ibitnum);
	uint32_t mask=1;
	mask=mask<<bitnum;
#if (HD24FSDEBUG_BITSET==1)
	cout << "getword=" << getword << "mask=" << mask << endl;
#endif
	getword=getword|mask;
	Convert::setint32(usagebuffer,ibitnum,getword);
}

void hd24fs::disablebit(uint32_t ibitnum,unsigned char* usagebuffer)
{
#if (HD24FSDEBUG_BITSET==1)
	cout << "disable bit " << ibitnum << endl;
#endif
	int bitnum=ibitnum%32;
	ibitnum-=bitnum;
	ibitnum/=32;	// i now is word num
	ibitnum*=4;	// i now is offset
	uint32_t getword=Convert::getint32(usagebuffer,ibitnum);
	uint32_t mask=1;
	mask=mask<<bitnum;
#if (HD24FSDEBUG_BITSET==1)
	cout << "getword=" << getword << "mask=" << mask << endl;
#endif
	getword=getword& (0xFFFFFFFF ^ mask);
#if (HD24FSDEBUG_BITSET==1)
	cout << "new getword=" << getword << endl;
#endif
	Convert::setint32(usagebuffer,ibitnum,getword);
}

bool hd24fs::isfreecluster(uint32_t i,unsigned char* usagebuffer)
{
	return isbitzero(i,usagebuffer);
}

void hd24fs::allocatecluster(uint32_t clusternum,unsigned char* usagebuffer)
{
	enablebit(clusternum,usagebuffer);
}

void hd24fs::freecluster(uint32_t clusternum,unsigned char* usagebuffer)
{
	disablebit(clusternum,usagebuffer);
}
void hd24fs::allocatecluster(uint32_t clusternum)
{
	allocatecluster(clusternum,sectors_driveusage);
}

void hd24fs::freecluster(uint32_t clusternum)
{
	freecluster(clusternum,sectors_driveusage);
}

uint32_t hd24fs::freeclustercount()
{
	if (!(isOpen()))
	{
		return 0;
	}
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "hd24fs::freeclustercount()" << endl;
#endif

	sectors_driveusage=getsectors_driveusage();

	if (sectors_driveusage==NULL)
	{
		return 0; // cannot get driveusage sectors, 0 clusters free.
	}
	uint32_t i=0;
	uint32_t fsc=driveusagesectorcount();

	if (fsc>0xFF) return 0;
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Sectors used for drive usage=" << fsc << endl;
#endif
	uint32_t clusters=((fsc /*sectors of alloc info*/
				       *512 /*bytes*/)
				       -8 /* checksum bytes */)
					*8 /* bits per byte */;
	//uint32_t clusters=((512*fsc-1)+504)*8;
#if (HD24FSDEBUG_QUICKFORMAT==1)
		cout << "Total clusters=" << clusters << endl;
#endif

	uint32_t freeclusters=0;
	for (i=0;i<clusters;i++) {
		if (isfreecluster(i,sectors_driveusage))
		{
#if (HD24FSDEBUG_QUICKFORMAT==1)
		//cout << "Cluster " << i << " is free" << endl;
#endif
			freeclusters++;
		}
	}
	return freeclusters;
}

string* hd24fs::freespace(uint32_t rate,uint32_t tracks)
{
	if (!(isOpen()))
	{
		return new string("");
	}

	uint32_t freeclusters=freeclustercount();
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "free clusters=" << freeclusters << endl;
#endif
	uint64_t freesectors=freeclusters*getblocksizeinsectors()*getblockspercluster();
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "free sectors=" << freesectors << endl;
#endif
	uint64_t freebytes=freesectors*SECTORSIZE;
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "free bytes=" << freesectors << endl;
#endif
	uint64_t freesamples=(uint64_t)(freebytes/3);
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "free samples=" << freesamples << endl;
#endif
	uint64_t freeseconds=(uint64_t)(freesamples/rate/tracks);
	uint32_t freehours=(freeseconds-(freeseconds%3600))/3600;
	freeseconds-=(freehours*3600);
	uint32_t freeminutes=(freeseconds-(freeseconds%60))/60;
	freeseconds-=(freeminutes*60);
       	string* newst=Convert::int2str(freehours);
	*newst+=" hr ";
	string* freemins=Convert::int2str(freeminutes,2,"0");
	*newst+=*freemins;
	delete freemins;
	*newst+=" min ";
	string* freesecs=Convert::int2str(freeseconds,2,"0");
	*newst+=*freesecs;
	delete freesecs;
	*newst+=" sec ";
      	return newst; // throw exception?
}

string* hd24fs::version()
{
	if (!(isOpen()))
	{
		return new string("");
	}
	getsector_bootinfo();
	string* newst=Convert::readstring(sector_boot,FSINFO_VERSION_MAJOR,1);
	*newst+=".";
	string* dummy=Convert::readstring(sector_boot,FSINFO_VERSION_MINOR,2);
	*newst+=*dummy;
	delete dummy;
	return newst;
}

uint32_t hd24fs::maxprojects()
{
	if (!(isOpen()))
	{
	      	return 0;
	}
	getsector_bootinfo();
	uint32_t maxprojs=Convert::getint32(sector_boot,FSINFO_MAXPROJECTS);
	if (maxprojs>99) {
		maxprojs=99;
		/* safety feature while no larger project
                   counts are known to be valid; gives
		   more stability when working with corrupt drives */
		this->writeprotected=true;
	}
	return maxprojs;
}

uint32_t hd24fs::getblocksizeinsectors()
{
	if (!(isOpen()))
	{
	      	return 0;
	}
	getsector_bootinfo();
	if (forcemode) {
		return 0x480;
	}
	uint32_t blocksize=Convert::getint32(sector_boot,FSINFO_BLOCKSIZE_IN_SECTORS);
	if (blocksize!=0x480)
	{
		this->writeprotected=true;
	}
	return blocksize;
}

uint32_t hd24fs::getbytesperaudioblock()
{
	return getblocksizeinsectors()*512;
}

uint32_t hd24fs::getblockspercluster()
{
	if (!(isOpen()))
	{
	      	return 0;
	}
	getsector_bootinfo();
	uint32_t maxprojs=Convert::getint32(sector_boot,FSINFO_AUDIOBLOCKS_PER_CLUSTER);
	return maxprojs;
}

uint32_t hd24fs::maxsongsperproject()
{
	if (!(isOpen()))
	{
	      	return 0;
	}
	getsector_bootinfo();
	uint32_t maxsongs=Convert::getint32(sector_boot,FSINFO_MAXSONGSPERPROJECT);
	return maxsongs;
}

uint32_t hd24fs::getprojectsectornum(uint32_t i)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::getprojectsectornum("<<i<<")"<< endl;
#endif
	// 1-based project sectornum
	if (!(isOpen()))
	{
	      	return 0;
	}
	if (i<1)
	{
		return 0;
	}
        if (i == UINT32_MAX)
        {
                return UINT32_MAX;
        }
	if (i>maxprojects())
	{
		return 0;
	}
	getsector_diskinfo();
	uint32_t projsec=Convert::getint32(sector_diskinfo,
			DRIVEINFO_PROJECTLIST+((i-1)*4));

#if (HD24FSDEBUG==1)
	cout << "projsec = " << projsec << endl;
#endif
	return projsec;
}

void hd24fs::lastprojectid(int32_t projectid)
{
	if (!(isOpen()))
	{
	      	return;
	}
	getsector_diskinfo();
	uint32_t lastprojsec=getprojectsectornum(projectid);
	if (lastprojsec==0) {
		return;
	}
	if (projectid==lastprojectid())
	{
		// nothing changed- nothing to save
		return;
	}

	Convert::setint32(sector_diskinfo,DRIVEINFO_LASTPROJ,lastprojsec);
	savedriveinfo();
	return;
}

int32_t hd24fs::lastprojectid()
{
	if (!(isOpen()))
	{
	      	return -1;
	}
	getsector_diskinfo();
	uint32_t lastprojsec=Convert::getint32(sector_diskinfo,DRIVEINFO_LASTPROJ);
	if (lastprojsec==0) {
		// TODO: This differs from the real HD24 where even
		// on a freshly formatted drive there always is at least
		// one project.
		return -1;
	}
	int i;
	int maxprojs=maxprojects();
	for (i=1;i<=maxprojs;i++)
	{
		uint32_t projsec=getprojectsectornum(i);

		if (projsec==lastprojsec)
		{
			return i;
		}
	}
	// no default project. hm......
	if (maxprojs>=1) {
		return 1;
	}
	return -1;
}

uint32_t hd24fs::getunusedsongsector()
{
	if (!(isOpen()))
	{
	      	return 0; // return 0- this is an invalid songsector
			  // so error is detectable
	}

	// generate an up-to-date song usage table.
	unsigned char* songusage=calcsongusage();

	int currsongentry=0;
	int32_t foundentry=-1;
	int maxprojs=this->maxprojects();
	int maxsongcount=this->maxsongsperproject();
	int totentries=maxprojs*maxsongcount; // 99 songs, 99 projects
	while (currsongentry<totentries) // 99 sec, 99 proj
	{
		if (isbitzero(currsongentry,songusage)) {
			foundentry=currsongentry;
			break;
		}
		currsongentry++;
	}
	if (foundentry==-1)
	{
		return 0;
	}
	return songentry2sector(foundentry);
}

void hd24fs::allocsongentry(uint32_t songentry)
{
	enablebit(songentry,sectors_songusage);
}

uint32_t hd24fs::projectcount()
{
	if (!(isOpen()))
	{
	      	return 0;
	}
	getsector_diskinfo();
	uint32_t lastprojsec=Convert::getint32(sector_diskinfo,DRIVEINFO_LASTPROJ);
	if (lastprojsec==0)
	{
		return 0;
	}
	uint32_t projcount=Convert::getint32(sector_diskinfo,DRIVEINFO_PROJECTCOUNT);
	if (projcount>maxprojects()) return maxprojects();

	return projcount;
}

int hd24fs::mode() {
	return p_mode;
}

hd24project* hd24fs::getproject(int32_t projectid)
{
	uint32_t projsec=getprojectsectornum(projectid); // 1-based
	if (projsec==0) {
		return NULL;
	}
	return new hd24project(this,projectid);
}

hd24project* hd24fs::createproject(const char* projectname)
{
#if (HD24FSDEBUG==1)
	cout << "hd24fs::createproject(" << projectname << ")" << endl;
#endif
	/* This creates a new project (with given project name)
	   on the drive (if possible).
           NULL is returned when unsuccessful, a pointer to the
           project otherwise.
        */
#if (HD24FSDEBUG==1)
	cout << "hd24fs asked to create project " << projectname << endl;
#endif
	int i;
	// find first project with project sector num 0!
	int maxprojs=maxprojects();

	getsector_bootinfo();
	if (sector_boot==NULL) {
		// unknown cluster size.
		return 0;
	}
	uint32_t firstprojsec=Convert::getint32(sector_boot,FSINFO_FIRST_PROJECT_SECTOR);
#if (HD24FSDEBUG==1)
	cout << "Firstprojsec="<< firstprojsec << endl;
#endif
	uint32_t secsperproj=Convert::getint32(sector_boot,FSINFO_SECTORS_PER_PROJECT);
#if (HD24FSDEBUG==1)
	cout << "Sectors per project="<< secsperproj << endl;
#endif

	// Let's calculate a list of unused project sectors
	char* projused=(char*)memutils::mymalloc("createproject",maxprojs,1);
	if (projused==NULL) {
		return 0; // out of memory
	}

	for (i=0;i<maxprojs;i++) {
		projused[i]=0;
	}

	for (i=1;i<=maxprojs;i++) {
		uint32_t projsec=getprojectsectornum(i); // 1-based
		if (projsec!=0)
		{
			/* projects do not necessarily have to
                           be stored on disk in the same order
                           as their project numbers-
			   so project 1 can be at sector 0x15
                           while project 2 is at sector 0x14.
	 		   This is why we have to convert project
                           sector to project slot. The cast to int
                           makes sure we don't accidentally end up
                           with a float index that can be misinterpreted. */
			projused[(int)((projsec-firstprojsec)/secsperproj)]=1;
		}
	}

	int foundslotnum=0;
	for (i=1;i<=maxprojs;i++)
	{
		uint32_t projsec=getprojectsectornum(i); // 1-based

		if (projsec==0)
		{
			foundslotnum=i;
			break;
		}
	}

	// Now find an unused project sector
	uint32_t foundsecnum=0;
	for (i=0;i<maxprojs;i++) {
		if (projused[i]==0) {
			foundsecnum=(i*secsperproj)+firstprojsec;
			break;
		}
	}
	memutils::myfree("projused",projused);

	if (foundslotnum==0)
	{
		// no unused slots.
		return NULL;
	}
	uint32_t projectid=foundslotnum;
	foundslotnum--;	// use 0-based slot num

	if (foundsecnum==0) {
		// no unsused project sectors found.
		// This is seriously fishy as there *are*
		// unused slots- so that should never happen.
		// Looks like we're dealing with a corrupt FS!
		this->writeprotected=true;
		return NULL;
	}
	// Now to assign the first unused project sector
	// to the first unused project slot.

	// First, update the drive info.
	getsector_diskinfo();

	// Add the new project pointer to the disk info:
	Convert::setint32(sector_diskinfo,DRIVEINFO_PROJECTLIST+(foundslotnum*4),foundsecnum);
	Convert::setint32(sector_diskinfo,DRIVEINFO_LASTPROJ,foundsecnum);

	Convert::setint32(sector_diskinfo,DRIVEINFO_PROJECTCOUNT,
	Convert::getint32(sector_diskinfo,DRIVEINFO_PROJECTCOUNT)+1);

	bool isnew=true;
	hd24project* newproject=new hd24project(this,projectid,foundsecnum,projectname,isnew);
	savedriveinfo();

	return newproject;
}

bool hd24fs::isallinput()
{
	return this->allinput;
}

void hd24fs::setallinput(bool p_allinput)
{
	this->allinput=p_allinput;
}

void hd24fs::setallinput(void)
{
	this->setallinput(true);
}

/* These three functions are for the 'auto input' button
   (having to do with automatic toggling of monitoring
    between 'tape' and inputs during a punch in */
bool hd24fs::isautoinput()
{
	return this->autoinput;
}

void hd24fs::setautoinput(bool p_autoinput)
{
	this->autoinput=p_autoinput;
}

void hd24fs::setautoinput(void)
{
	this->setautoinput(true);
}

bool hd24fs::comparebackupblock(uint32_t p_sector,uint32_t p_blocksize,
			      uint32_t lastsec)
{
	/** Used by commit. This compares a logical block of file system with its backup
	    at the end of the drive. */
        unsigned char headerbuf[10000];
        unsigned char footerbuf[10000];

	uint32_t i;
	uint32_t blocksize=p_blocksize;

#if (HD24FSDEBUG_COMMIT==1)
	cout << "Read " << blocksize << "sectors starting at " << p_sector << endl;
#endif
	readsectors_noheader(this, p_sector, headerbuf,blocksize);
#if (HD24FSDEBUG_COMMIT==1)
	cout << "Read " << blocksize << "sectors starting at " << ((lastsec-(p_sector+blocksize))+1) << endl;
#endif
	readsectors_noheader(this, (lastsec-(p_sector+blocksize))+1, footerbuf,blocksize);

//	uint32_t targetsector=(lastsec-(p_sector+blocksize))+1;
	//uint32_t targetsector=(lastsec-(p_sector));

	for (i=1;i<=blocksize;i++) {

		// read sector $sector+$i-1
		// write to sector -($sector+1+$blocksize-$i)
		// 	(where -1= last sector)
		uint32_t headeroff=(512* (i-1));
		//uint32_t footeroff= (512*(blocksize-i));
		uint32_t footeroff=(512* (i-1));
#if (HD24FSDEBUG_COMMIT==1)
		cout << "compare 512 bytes starting at headerbuf+" << headeroff<< "and footerbuf+" <<footeroff << endl;
#endif
		int x=memcmp ( headerbuf+headeroff,footerbuf+footeroff,512);
		if (x!=0)
		{
			for (int j=0;j<512;j++)
			{
#if (HD24FSDEBUG_COMMIT==1)
				cout << (int)(headerbuf[headeroff+j]) << "/" << (int)(footerbuf[footeroff+j]) << "     ";
#endif
			}
#if (HD24FSDEBUG_COMMIT==1)
			cout << endl;
#endif
			return false;

		}
	}
	return true;
}


void hd24fs::writebackupblock(uint32_t p_sector,uint32_t p_blocksize,
			      uint32_t lastsec,bool fullcommit)
{
	/** Used by commit. This writes a logical block
            of file system data to the end of the drive. */
        unsigned char backbuf[1024];

	uint32_t i;
	uint32_t blocksize=p_blocksize;

	if (fullcommit==false)
	{
		// we're doing a quick commit, so only backup
		// changed blocks.
		if (p_sector>highestFSsectorwritten) return;
	}

	for (i=1;i<=blocksize;i++) {

		// read sector $sector+$i-1
		// write to sector -($sector+1+$blocksize-$i)
		// 	(where -1= last sector)
		uint32_t currentsourcesector=p_sector+(i-1);
#if (HD24FSDEBUG_COMMIT==1)
		cout << "Reading 1 source sector, " << currentsourcesector << "; ";
#endif
		uint32_t secsread=readsectors_noheader(this, currentsourcesector, backbuf,1);
		secsread=secsread;
#if (HD24FSDEBUG_COMMIT==1)
		cout << "secsread=" << secsread << endl;
#endif
		uint32_t targetsector=(lastsec-(p_sector+1+blocksize-i))+1;
		if (targetsector<0x1397F6)
		{
			/* Skip backup block writing if target sector would be placed before
			   audio data area (since we seem to be writing a header file)
			*/
			break;
		}
#if (HD24FSDEBUG_COMMIT==1)
		cout << "TARGET sector=" << targetsector << endl;
		cout << "lastsec=" << lastsec << ", p_sector=" << p_sector<<", blocksize=" << blocksize << ", i=" <<i << endl;
		cout << "Targetsector=(lastsec-(p_sector+1+blocksize-i))+1;" << endl;
#endif
		writesectors(this->devhd24,targetsector,backbuf,1);
	}
}

bool hd24fs::commit()
{
	// default commit is a quick commit rather than full commit.
	// A quick commit only commits sectors up to the last project/song
	// sector changed.
	return this->commit(false);
}

bool hd24fs::commit(bool fullcommit)
{
/*
	// don't try to be smart here. We say commit so commit.
	if (!this->needcommit)
	{

		return true;
	}*/
	/** This creates a backup of the file system to the end of the drive. */


#if (HD24FSDEBUG==1)
	cout << "hd24fs::commit()" << endl;
#endif
	if (this->headersectors!=0)
	{
		// ehm. Obviously we're not going to overwrite the
		// end of the drive with header file information,
		// as that would defeat the purpose of header files
		// (which is to allow safe read-only operation).
		return true;
	};
	uint32_t sector=0;
	uint32_t blocksize=1;
	uint32_t count=1;
	int lastsecerror=0;
	uint32_t lastsec=getlastsectornum(&lastsecerror);
#if (HD24FSDEBUG==1)
	cout << "lastsec before writing backup blocks=" << lastsec << endl;
#endif
	writebackupblock(sector,blocksize,lastsec,fullcommit); // backup superblock

	sector+=(blocksize*count);
	blocksize=1;
	count=1;

	writebackupblock(sector,blocksize,lastsec,fullcommit); // backup drive info

	sector+=(blocksize*count);
	blocksize=3;
	count=1;
	writebackupblock(sector,blocksize,lastsec,fullcommit); // Backup undo (?) usage

	sector+=(blocksize*count);
	blocksize=15;
	count=1;
	writebackupblock(sector,blocksize,lastsec,fullcommit); // Backup drive usage table

	sector+=(blocksize*count);
	blocksize=1;
	count=99;

	uint32_t i;
	for (i=1;i<=count;i++)
	{
#if (HD24FSDEBUG_COMMIT==1)
	cout <<"Going to write proj backup block no. " << i << endl;
#endif
		writebackupblock(sector+(i-1),blocksize,lastsec,fullcommit); // Backup project
	}

	sector+=(blocksize*count);
	count=99*99;
	for (i=1;i<=count;i++)
	{
#if (HD24FSDEBUG_COMMIT==1)
	cout <<"Going to write song backup block no. " << i << endl;
#endif
		blocksize=2;
		writebackupblock(sector+(7*(i-1)),blocksize,lastsec,fullcommit); // Backup song

		blocksize=5;
		writebackupblock(sector+(7*(i-1))+2,blocksize,lastsec,fullcommit); // Backup song alloc info
	}
#if (HD24FSDEBUG_COMMIT==1)
	cout <<"Wrote backup blocks" << endl
	<< "Driveusage pointer after commit=" << this->sectors_driveusage << endl
	<< "." << endl;
#endif
	highestFSsectorwritten=0; // reset
	this->hd24sync();
	this->needcommit=false;
	return true;
}
void hd24fs::hd24sync()
{
#ifdef WINDOWS
	FlushFileBuffers(this->devhd24);
#else
	sync();
#endif
}
bool hd24fs::commit_ok()
{
	/** This compares the header file with its backup to see if the drive is consistent. */

#if (HD24FSDEBUG==1)
	cout << "bool hd24fs::commit_ok()" << endl
	<< "Driveusage pointer before commit=" << this->sectors_driveusage << endl;
#endif
	uint32_t sector=0;
	uint32_t blocksize=1;
	uint32_t count=1;
	int lastsecerror=0;
	uint32_t lastsec=getlastsectornum(&lastsecerror);
#if (HD24FSDEBUG==1)
	cout << "lastsec before writing backup blocks=" << lastsec << endl;
#endif
	if (!comparebackupblock(sector,blocksize,lastsec))
	{
		// superblock mismatch
		return false;
	}

	sector+=(blocksize*count);
	blocksize=1;
	count=1;

	if (!comparebackupblock(sector,blocksize,lastsec))
	{
		// drive info mismatch
		return false;
	}

	sector+=(blocksize*count);
	blocksize=3;
	count=1;
	if (!comparebackupblock(sector,blocksize,lastsec))
	{
		// song usage mismatch
		return false;
	} ;

	sector+=(blocksize*count);
	blocksize=15;
	count=1;
	if (!comparebackupblock(sector,blocksize,lastsec))
	{
		// Drive usage table mismatch
		return false;
	}

	sector+=(blocksize*count);
	blocksize=1;
	count=99;

	uint32_t i;
	for (i=1;i<=count;i++)
	{
		if (!comparebackupblock(sector+(i-1),blocksize,lastsec))
		{
			// project entry mismatch
			return false;
		}
	}

	sector+=(blocksize*count);
	count=99*99;
	for (i=1;i<=count;i++)
	{
		blocksize=2;
		if (!comparebackupblock(sector+(7*(i-1)),blocksize,lastsec))
		{
			// song entry mismatch
			return false;
		}

		blocksize=5;
		if (!comparebackupblock(sector+(7*(i-1))+2,blocksize,lastsec))
		{
			// song alloc info mismatch
			return false;
		}
	}
	return true;
}

uint32_t hd24fs::setsectorchecksum(unsigned char* buffer,uint32_t startoffset,uint32_t startsector,uint32_t sectors)
{
	// Calculates and sets the checksum for a block of data.
	// Data must be in drive-native format.
	uint32_t checksum32 = 0;
	uint32_t totbytes=(SECTORSIZE*sectors);

	buffer[startoffset+totbytes-8]=startsector%256;
	buffer[startoffset+totbytes-7]=(startsector>>8)%256;
	buffer[startoffset+totbytes-6]=255-	buffer[startoffset+totbytes-8];
	buffer[startoffset+totbytes-5]=255-	buffer[startoffset+totbytes-7];

	for (uint32_t i = 0; i < totbytes; i += 4)
	{
		uint32_t num = Convert::getint32(buffer, i+startoffset);
		int byte1 = num % 256;
		int byte2 = (num >> 8) % 256;
		int byte3 = (num >> 16) % 256;
		int byte4 = (num >> 24) % 256;
		num = byte4 + (byte3 << 8) + (byte2 << 16) + (byte1 << 24);
		checksum32 += num;
	}
	uint32_t oldchecksum=0;
	oldchecksum+=((unsigned char)(buffer[startoffset+totbytes-1])); oldchecksum=oldchecksum <<8;
	oldchecksum+=((unsigned char)(buffer[startoffset+totbytes-2])); oldchecksum=oldchecksum <<8;
	oldchecksum+=((unsigned char)(buffer[startoffset+totbytes-3])); oldchecksum=oldchecksum <<8;
	oldchecksum+=((unsigned char)(buffer[startoffset+totbytes-4]));
	oldchecksum-=checksum32;
	buffer[startoffset+totbytes-4]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
	buffer[startoffset+totbytes-3]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
	buffer[startoffset+totbytes-2]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
	buffer[startoffset+totbytes-1]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
	return checksum32;
}

void hd24fs::savedriveinfo()
{
	// This is capable of handling only 1-sector-per-project projects
	uint32_t driveinfosector=1;
#if (HD24FSDEBUG==1)
	gotlastsectornum=false;
	//int lastsecerror=0;
	//uint32_t lastsec=getlastsectornum(&lastsecerror);
#endif
	if (sector_diskinfo==NULL)
	{
		// diskinfo is not available, nothing to do.
		return;
	}
#if (HD24FSDEBUG==1)
	cout << "FSTFIX" << endl;
#endif
	this->fstfix(sector_diskinfo,512); // sector is now once again in native format

#if (HD24FSDEBUG==1)
	cout << "set checksum" << endl;
#endif
	this->setsectorchecksum(sector_diskinfo,0,driveinfosector,1);
#if (HD24FSDEBUG==1)
	cout << "write sectors" << endl;
#endif
	this->writesectors(this->devhd24,
			driveinfosector,
			sector_diskinfo,1);

#if (HD24FSDEBUG==1)
	cout << "unfix" << endl;
#endif
	this->fstfix(sector_diskinfo,512); // sector is now in 'fixed' format
#if (HD24FSDEBUG==1)
	cout << "commit" << endl;
#endif
	this->commit();
}

void hd24fs::setname(unsigned char* namebuf,string newname,uint32_t shortnameoff,uint32_t longnameoff)
{
	/** Used for setting song/project/drive names
            Long name is up to 64 characters; short name
            is up to 10 chars.
        */
#if (HD24FSDEBUG==1)
	cout	<< "hd24fs::setname("
		<< "*namebuf=" << *namebuf << ","
		<< "newname=" << newname << ","
		<< "shortnameoff="<<shortnameoff << ","
		<< "longnameoff=" << longnameoff <<");" << endl;
#endif
	bool foundzero=false;
	for (uint32_t i=0;i<64;i++)
	{
		if (!foundzero)
		{
			namebuf[longnameoff+i]=newname.c_str()[i];
			if (namebuf[longnameoff+i]==0) {
				foundzero=true;
#if (HD24FSDEBUG==1)
				cout << "Found zero at " << i << endl;
#endif
			}
		}
		else
		{
			namebuf[longnameoff+i]=0;
		}
	}
	// Now set FST 1.0 short name
	unsigned char* target=namebuf+shortnameoff;
	foundzero=false;
	uint32_t count=0;
	for (uint32_t i=0;i<10;i++)
	{
		if (!foundzero)
		{
			target[count]=newname.c_str()[i];
			if (target[count]==0)
			{
				foundzero=true;
			}
		}
		else
		{
			target[count]=0x20; /* short name is filled out
					       with spaces */

		}
		count++;
		if (count==8)
		{
			count=0;
			target+=10;
		}
	}
	return;
}

void hd24fs::savedriveusage()
{
	uint32_t driveusagesector=5;
	uint32_t totsectors=15;
	this->fstfix(sectors_driveusage,totsectors*512); // sector is now once again in native format

	this->setsectorchecksum(sectors_driveusage,0,driveusagesector,totsectors);
	this->writesectors(this->devhd24,
			driveusagesector,
			sectors_driveusage,totsectors);

	this->fstfix(sectors_driveusage,totsectors*512); // sector is now in 'fixed' format
#if (HD24FSDEBUG==1)
	cout << "free cluster count=" << freeclustercount() << endl;
#endif
	unsigned char* bootrec=readbootinfo();
	// update FSINFO_FREE_CLUSTERS_ON_DISK

	Convert::setint32(bootrec,FSINFO_FREE_CLUSTERS_ON_DISK,freeclustercount());
	fstfix(bootrec,512); // convert back into native format

	/* Calculate the proper checksum for the bootinfo */
	setsectorchecksum(bootrec,
		0 /* startoffset */,
		0 /* sector */,
		1 /*sectorcount */
	);
	this->writesectors(this->devhd24, 0,bootrec,1);
	fstfix(bootrec,512); // convert back into fixed format
	this->commit();
}


uint32_t hd24fs::writesuperblock(uint32_t lastsec)
{
	// writes a new superblock to an unformatted drive.
	//
	// normal start of data area=1397f6
	// size of 1 audio block=0x480 sectors
	// fs size=0x77+7*99
	// so minimum usable drive is 0x1397f6+0x480+(0x77+(7*99*99))
	// =0x14a8ec sectors.
	// Sectors are counted from base 0
	// so last sector num must be at least 0x14a8ec-1
	if (lastsec<0x14a8eb)
	{
		// drive is smaller than the minimum needed for
		// storing at least 1 audio block.
		return RESULT_FAIL;
	}
	unsigned char superblock[512];
	useinternalboot(superblock,lastsec); // this automatically sets sector checksum
	writesectors(this->devhd24,0,&superblock[0],1);
#if (HD24FSDEBUG_QUICKFORMAT==1)
	cout << "Writing superblock." << endl;
	hd24utils::dumpsector((const char*)superblock);

#endif

	force_reload();

	readbootinfo();

#if (HD24FSDEBUG_CLUSTERddCALC==1)
	cout << "Displaying sector after reload." << endl;
	hd24utils::dumpsector((const char*)sector_boot);
#endif

	return RESULT_SUCCESS;
}

void hd24fs::force_reload()
{
#if (HD24FSDEBUG==1)
	cout << "Free superblock mem" << endl;
#endif
	if (sector_boot!=NULL)
	{
		memutils::myfree("sectors_boot",sector_boot);
		sector_boot=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free diskinfo mem" << endl;
#endif
	if (sector_diskinfo!=NULL)
	{
		memutils::myfree("sectors_diskinfo",sector_diskinfo);
		sector_diskinfo=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free drive usage mem" << endl;
#endif
	if (sectors_driveusage!=NULL)
	{
		memutils::myfree("sectors_driveusage",sectors_driveusage);
		sectors_driveusage=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free orphan sectors mem" << endl;
#endif
	if (sectors_orphan!=NULL)
	{
		memutils::myfree("sectors_orphan",sectors_orphan);
		sectors_orphan=NULL;
	}
#if (HD24FSDEBUG==1)
	cout << "Free song usage sectors mem" << endl;
#endif
	if (sectors_songusage!=NULL)
	{
		memutils::myfree("sectors_songusage",sectors_songusage);
		sectors_songusage=NULL;
	};

}
uint32_t hd24fs::writedriveinfo()
{
#if (HD24FSDEBUG==1)
	gotlastsectornum=false;
	//int lastsecerror=0;
	//uint32_t lastsec=getlastsectornum(&lastsecerror);
#endif

	readdiskinfo();
	cleardriveinfo(sector_diskinfo);
	fstfix(sector_diskinfo,512); // back into native format
	writesectors(this->devhd24,1,sector_diskinfo,1);
	if (sector_diskinfo!=NULL) {
		memutils::myfree("sector_diskinfo",sector_diskinfo);
		sector_diskinfo=NULL; // force re-read
	}
	hd24project* firstproject=createproject("Proj Name");
	memutils::myfree("firstproject",firstproject);

	if (sector_diskinfo!=NULL) {
		memutils::myfree("sector_diskinfo",sector_diskinfo);
		sector_diskinfo=NULL; // force re-read
	}
	uint32_t psec=getprojectsectornum(1);
	if (sector_diskinfo!=NULL) {
		memutils::myfree("sector_diskinfo",sector_diskinfo);
		sector_diskinfo=NULL; // force re-read
	}
	readdiskinfo();
	Convert::setint32(sector_diskinfo,DRIVEINFO_LASTPROJ,psec);
	savedriveinfo();

	return RESULT_SUCCESS;
}

uint32_t hd24fs::writeemptysongusagetable()
{
	// Writes an empty song usage table to the drive.
	unsigned char* buffer=resetsongusage();
	if (buffer==NULL)
	{
		return RESULT_FAIL;
	}
	uint32_t sectornum=2;
	uint32_t sectorcount=3;
	fstfix(buffer,sectorcount*512);
	setsectorchecksum(buffer,
		0 /* startoffset */,
		sectornum /* sector */,
		sectorcount /*sectorcount */
	);
	this->writesectors(this->devhd24,
			sectornum,
			buffer,
			sectorcount);
	fstfix(buffer,sectorcount*512);
	return RESULT_SUCCESS;
}

uint32_t hd24fs::writedriveusage()
{
	if (sectors_driveusage!=NULL)
	{
		memutils::myfree("sectors_driveusage",sectors_driveusage);
		sectors_driveusage=NULL;
	}

	unsigned char* buffer=resetdriveusage();
	if (buffer==NULL)
	{
		return RESULT_FAIL;
	}
	uint32_t sectornum=5;
	uint32_t sectorcount=15;
	fstfix(buffer,sectorcount*512);
	setsectorchecksum(buffer,
		0 /* startoffset */,
		sectornum /* sector */,
		sectorcount /*sectorcount */
	);

	this->writesectors(this->devhd24,
			sectornum,
			buffer,
			sectorcount);
	fstfix(buffer,sectorcount*512);

	return RESULT_SUCCESS;
}

uint32_t hd24fs::quickformat(char* message)
{
	// This procedure performs a quickformat on the current drive.
	// There is no need for the drive to be a valid HD24 drive
	// for this to work.
	// Safety confirmations etc. are considered to be the
	// responsibility of the caller.
	highestFSsectorwritten=0; // reset
#if (HD24FSDEBUG==1)
	cout << "hd24fs::quickformat()" << endl;
#endif

	gotlastsectornum=false; // force re-finding last sector num
	int lastsecerror=0;
	uint32_t lastsec=getlastsectornum(&lastsecerror);

	formatting=true;

	if (lastsec==0)
	{
		if (message!=NULL) strcpy(message,(const char*)&"Lastsec=0");
		return 0;
	}
	uint32_t result=writesuperblock(lastsec);

	if (result==RESULT_FAIL)
	{
		if (message!=NULL) strcpy(message,(const char*)&"Write superblock failed");
		return 0; // lastsec 0 means failed format
	}

	result=writedriveinfo();

	if (result==RESULT_FAIL)
	{
		if (message!=NULL) strcpy(message,(const char*)&"Write driveinfo failed");
		return 0; // lastsec 0 means failed format
	}

	result=writeemptysongusagetable();	// write empty song usage table to the drive.

	if (result==RESULT_FAIL)
	{
		if (message!=NULL) strcpy(message,(const char*)&"Write song usage failed");
		return 0; // lastsec 0 means failed format
	}

	result=writedriveusage();
	if (result==RESULT_FAIL)
	{
		if (message!=NULL) strcpy(message,(const char*)&"Write drive usage failed");
		return 0; // lastsec 0 means failed format
	}

	// write empty song-entry usage table
	// write empty drive usage table
	// write 99 empty projects
	// create default project
	// (write 99*99 empty songs)
	// (empty audio space)
	// commit fs
	this->needcommit=true;
	commit(); // full commit
	formatting=false;
	force_reload();
	return lastsec;
}

void hd24fs::setprojectsectornum(int i,uint32_t sector)
{
	getsector_diskinfo();
	Convert::setint32(sector_diskinfo,
		DRIVEINFO_PROJECTLIST + ((i - 1) * 4),sector);

	return;
}

uint32_t hd24fs::deleteproject(int32_t projid)
{
	if (projid<1)
	{
		/* Illegal project id;
                   project IDs are set in base 1. */
		return RESULT_FAIL;
	}

	getsector_diskinfo(); // has list of pointers to project sectors
	uint32_t pcount = Convert::getint32(sector_diskinfo, DRIVEINFO_PROJECTCOUNT);

	if (pcount<=1)
	{
		/* Attempt to delete last project on drive-
		   not allowed, a drive must always contain
		   at least 1 project */
		return RESULT_FAIL;
	}

	hd24project* projtodel=this->getproject(projid);
	if (projtodel==NULL) {
		return RESULT_FAIL;
	}

	/* If there still are songs in the project, delete them */
	uint32_t currsongcount=projtodel->songcount();
	if (currsongcount>0)
	{
		for (uint32_t j=1; j<=currsongcount;j++)
		{
			projtodel->deletesong(1); // songs will shift
			projtodel->save();
		}
	}

	/* delete project from project list by shifting left
	   all other projects... */
	if (projid<99)
	{
	        /* When project 99 is deleted no shifting is needed */
		for (uint32_t i=projid;i<99;i++)
		{
			setprojectsectornum(i,getprojectsectornum(i+1));
		}
	}

	setprojectsectornum(99,0); // ...and clearing the last entry.

	Convert::setint32(sector_diskinfo,DRIVEINFO_PROJECTCOUNT,pcount-1);

	/* Set 'last accessed project' to first in list
       	   (the project being deleted needs to be accessed
            prior to deletion so this must always be updated)
        */
	Convert::setint32(sector_diskinfo, DRIVEINFO_LASTPROJECT,getprojectsectornum(1));
#if (HD24FSDEBUG==1)
	cout << "save project." << endl;
#endif

	savedriveinfo();
	delete projtodel;
	return RESULT_SUCCESS;
}

void hd24fs::write_enable()
{
	this->writeprotected=false;
}

void hd24fs::write_disable()
{
	this->writeprotected=true;
}
