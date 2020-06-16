#ifndef __hd24fs_h__
#define __hd24fs_h__

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <hd24utils.h>
#include "memutils.h"
#include "convertlib.h"
#define CLUSTER_UNDEFINED (0xFFFFFFFF)

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

class hd24fs;
class hd24project;
class hd24raw;
class hd24song;

class AudioStorage
{
public:
	virtual uint32_t samplerate() { return 0; };
	virtual void currentlocation(uint32_t newpos) { return; };
	virtual uint32_t currentlocation() { return 0; };
	virtual uint32_t getlocatepos(int locatepoint) { return 0; };
	virtual uint32_t setlocatepos(int locatepoint,uint32_t newpos) { return 0; };
	virtual bool trackarmed(uint32_t base1tracknum) { return false; }
	virtual void trackarmed(uint32_t base1tracknum,bool arm) { return; }
	virtual ~AudioStorage() { return; } ;
};

class hd24song : public AudioStorage
{
	friend class hd24project;
	friend class hd24fs;
	friend class hd24transferjob;
	friend class hd24transferengine;
	private:
		uint32_t framespersec;
		unsigned char* buffer;		// for songinfo
		unsigned char* audiobuffer;	// for audio data 
		unsigned char* scratchbook;	// for write-back audio data
		uint32_t* blocksector;
		int evenodd; /* specifies if we are dealing with 
				 even or odd samples in high speed mode. */
		bool lengthened; /* Indicate if reallocating song length change occured */
		bool busyrecording;
		int mysongid;
		int polling;
		int currentreadmode;
		int currcachebufnum;
		bool rehearsemode;
		bool lastallocentrynum;
		hd24fs* parentfs;
		unsigned char** cachebuf_ptr;
		uint32_t* cachebuf_blocknum;
		hd24project* parentproject;
		hd24song(hd24project* p_parent,uint32_t p_songid);
		unsigned char* getcachedbuffer(uint32_t);
		void	queuecacheblock(uint32_t blocknum);
		void	loadblockintocache(uint32_t blocknum);
		void	setblockcursor(uint32_t blocknum);
		void 	memoizeblocksectors(uint32_t lastblock);
		uint32_t memblocksector(uint32_t blocknum);
		uint32_t blocktoqueue;		// next block to cache
		uint32_t songcursor;		// current cursor pos within song, in samples
		uint32_t allocentrynum;		// which allocation entry is currently being used
		uint32_t allocstartblock;	// the first audioblock in given entry
		uint32_t allocstartsector;	// the sector pointed to by that entry
		uint32_t allocaudioblocks;	// the number of audioblocks in the block
		uint32_t divider;
		uint32_t lastreadblock;
		uint32_t lastavailablecacheblock;
		uint32_t mustreadblock;
		uint32_t track_armed[24];
		uint32_t track_readenabled[24]; // used to speed up copy mode.
		void unmark_used_clusters(unsigned char* sectors_orphan);
		uint32_t used_alloctable_entries();
		uint32_t audioblocks_in_alloctable();
		void silenceaudioblocks(uint32_t allocsector,uint32_t blocks);
		bool allocatenewblocks(uint32_t, bool, char*, int*, int (*)());

	public:
		~hd24song();
		string* songname();
		bool endofsong();
		uint32_t songid();
		hd24fs* fs();
		static void sectorinit(unsigned char* sectorbuf);
		static void settrackcount(unsigned char* sectorbuf,uint32_t trackcount);
		static void songname(unsigned char* sectorbuf,string newname);
		void songname(string newname);
		static string* songname(hd24fs* parentfs,unsigned char* sectorbuf);
		void bufferpoll();
		
		bool loadlocpoints(string* filename);
		bool savelocpoints(string* filename);
		void readenabletrack(uint32_t tracknum);
		void readenabletrack(uint32_t tracknum,bool enable);

		bool isrehearsemode();
		void setrehearsemode(bool p_rehearsemode);
		bool recording();

		bool trackarmed(uint32_t tracknum);
		void trackarmed(uint32_t tracknum,bool arm);

		void armalltracks();
		void unarmalltracks();

		bool istrackmonitoringinput(uint32_t tracknum);
		void startrecord(int record_mode);
		void stoprecord();
		uint32_t samplerate();
		static void samplerate(unsigned char* sectorbuf,uint32_t samplerate);
		void samplerate(uint32_t newrate);
		static uint32_t samplerate(unsigned char* songbuf);
		uint32_t bitdepth();
		uint32_t physical_channels();
		uint32_t chanmult();
		static uint32_t chanmult(unsigned char* songbuf);
		static uint32_t physical_channels(unsigned char* songbuf);
		void physical_channels(uint32_t newchannelcount);
		static void physical_channels(unsigned char* songbuf,uint32_t newchannelcount);
		uint32_t logical_channels();
		static uint32_t logical_channels(unsigned char* songbuf);
		void logical_channels(uint32_t newchannelcount);
		static void logical_channels(unsigned char* songbuf,uint32_t channelcount);
		uint64_t songsize_in_bytes();
		uint64_t bytes_allocated_on_disk();
		/* Songlength_in_samples isn't suitable for high-samplerate songs
                   as they can be twice the amount of samples long which would require __uint33. 
                   By introducing the concept of sample pair words, or "wamples", a 32 bit number
                   is still enough for double the length in samples at double the sample rate.  */
		uint32_t songlength_in_wamples();
		uint32_t songlength_in_wamples(uint32_t newlen);
		uint32_t songlength_in_wamples(uint32_t newlen,bool silence);
		uint32_t songlength_in_wamples(uint32_t newlen,bool silence,char* savemessage,int* cancel);
		uint32_t songlength_in_wamples(uint32_t newlen,bool silence,char* savemessage,int* cancel,int (*checkfunc)());

		uint32_t display_hours();
		uint32_t display_hours(uint32_t offset);
		static uint32_t display_hours(uint32_t offset,uint32_t samrate);
		
		uint32_t display_minutes();
		uint32_t display_minutes(uint32_t offset);
		static uint32_t display_minutes(uint32_t offset,uint32_t samrate);

		uint32_t display_seconds();
		uint32_t display_seconds(uint32_t offset);
		static uint32_t display_seconds(uint32_t offset,uint32_t samrate);

		uint32_t display_subseconds();
		uint32_t display_subseconds(uint32_t offset);
		static uint32_t display_subseconds(uint32_t offset,uint32_t samrate);

		string*  display_duration();
		string*  display_duration(uint32_t offset);
		string*  display_duration(uint32_t offset,uint32_t samrate);

		string*  display_cursor();
		uint32_t cursorpos();
		uint32_t locatepointcount();
		uint32_t getlocatepos(int locatepoint);

		uint32_t currentlocation();
		void currentlocation(uint32_t offset);
		uint32_t golocatepos(uint32_t offset);

		uint32_t setlocatepos(int locatepoint,uint32_t offset);
		void setlocatename(int locatepoint,string newname);
		string* getlocatename(int locatepoint);
		bool iswriteprotected();
		void setwriteprotected(bool prot);
		void getmultitracksample(long* mtsample,int readmode);
		int getmtrackaudiodata(uint32_t firstsamnum,uint32_t samples,unsigned char* buffer,int readmode);
		int putmtrackaudiodata(uint32_t firstsamnum,uint32_t samples,unsigned char* buffer,int writemode);
		void deinterlaceblock(unsigned char* buffer,unsigned char* targetbuffer);
		void interlaceblock(unsigned char* buffer,unsigned char* targetbuffer);
		static const int  LOCATEPOS_SONGSTART;	
		static const int  LOCATEPOS_LOOPSTART;	
		static const int  LOCATEPOS_LOOPEND;
		static const int  LOCATEPOS_PUNCHIN;
		static const int  LOCATEPOS_PUNCHOUT;
		static const int  LOCATEPOS_EDITIN;	
		static const int  LOCATEPOS_EDITOUT;	
		static const int  LOCATEPOS_LAST;
		static const int  LOCATELIST_BYTELEN;
		static const int  READMODE_COPY;
		static const int  READMODE_REALTIME;
		static const int  WRITEMODE_COPY;
		static const int  WRITEMODE_REALTIME;

		uint32_t getnextfreesector(uint32_t allocsector);
		bool has_unexpected_end();	  // indicates if there is an 'unexpected end of song' error
		bool is_fixable_unexpected_end(); // ...and if so, if we know how to fix it.
		bool setallocinfo(bool silencenew);
		bool setallocinfo(bool silencenew,char* message,int* cancel,int (*checkfunc)());
		uint32_t requiredaudioblocks(uint32_t songlen);
		void appendorphanclusters(unsigned char*,bool allowsongresize);
		void save();
};

class hd24project 
{
	friend class hd24fs;
	friend class hd24song;

	private:
		unsigned char* buffer;
		hd24fs*	parentfs;
		int32_t myprojectid;
		hd24song* songlist;	
		hd24project(hd24fs* p_parent,int32_t projectid);
		hd24project(hd24fs* p_parent,int32_t projectid,uint32_t projsector,const char* projectname,bool isnew);
		uint32_t getsongsectornum(int i);
		void setsongsectornum(int i,uint32_t newsector);
		void save(uint32_t projsector);
		void populatesongusagetable(unsigned char* songused);
		uint32_t getunusedsongslot();
		void initvars(hd24fs* p_parent,int32_t p_projectid);
	public:
		~hd24project();
		string* projectname();
		void projectname(string newname);
		int32_t lastsongid();
		void lastsongid(int32_t songid);
		uint32_t songcount();
		uint32_t maxsongs();
		int32_t projectid();
		hd24song* getsong(uint32_t songid);
		hd24song* createsong(const char* songname,uint32_t trackcount,uint32_t samplerate);
		uint32_t deletesong(uint32_t songid);
		void save();
		void sort();
};

#ifndef hd24driveimage
class hd24driveimage;
#endif

class hd24fs 
{
	friend class hd24project;
	friend class hd24song;
	friend class hd24raw;
	friend class hd24utils;
	friend class hd24test;
	friend class hd24driveimage;

	private:
		const char* imagedir;
		int transportstatus;		
		bool writeprotected;
		bool allinput;
		bool autoinput;
		bool forcemode;
		bool formatting; // true during a format operation
		bool headermode;
		bool maintenancemode;
		bool wavefixmode;
		static uint32_t bytenumtosectornum(uint64_t flen);
		static uint64_t windrivesize(FSHANDLE handle);
		uint32_t highestFSsectorwritten;
		bool needcommit;

		uint32_t nextfreeclusterword;	// memoization cache for write allocation
		
		FSHANDLE foundlastsectorhandle; // last handle for which we found last sectornum
		FSHANDLE devhd24;	// device handle
		FSHANDLE hd24header;	// header device handle
		bool m_isOpen;
		bool gotlastsectornum;
		uint32_t foundlastsectornum;
		int p_mode;
		hd24project* projlist;	
		string* devicename;	
		unsigned char* sector_boot;
		unsigned char* sector_diskinfo;
		unsigned char* sectors_driveusage;	
		unsigned char* sectors_orphan;	
		unsigned char* sectors_songusage;
		long readsectors(FSHANDLE handle, uint32_t secnum, unsigned char* buffer,int sectors);
//		long readsector(FSHANDLE handle, uint32_t secnum, unsigned char* buffer);
//		long readsector_noheader(FSHANDLE handle, uint32_t secnum, unsigned char* buffer);
//		long readsector_noheader(hd24fs* currenthd24, uint32_t secnum, unsigned char* buffer);
		long readsectors_noheader(FSHANDLE handle, uint32_t secnum, unsigned char* buffer,uint32_t count);
		long readsectors_noheader(hd24fs* currenthd24, uint32_t secnum, unsigned char* buffer,uint32_t count);
		long writesectors(FSHANDLE handle, uint32_t secnum, unsigned char* buffer,int sectors);
//		long writesector(FSHANDLE handle, uint32_t secnum, unsigned char* buffer);

		string* gethd24currentdir(int, char**);
		void hd24closedevice(FSHANDLE handle,const char* source);
		void hd24sync();
		static void hd24seek(FSHANDLE handle,uint64_t seekpos);
		void clearbuffer(unsigned char* buffer);
		void clearbuffer(unsigned char* buffer,uint32_t bytes);
		FSHANDLE findhd24device();
		FSHANDLE findhd24device(int mode);
		FSHANDLE findhd24device(int mode, int base0devnum);
		FSHANDLE findhd24device(int mode, string* dev,bool force,bool tryharder);
		static bool isinvalidhandle(FSHANDLE handle);
		unsigned char* readbootinfo();
		unsigned char* readdiskinfo();
		unsigned char* readdriveusageinfo();
		unsigned char* resetsongusage();
		unsigned char* resetdriveusage();
		unsigned char* calcsongusage();
		void rewritesongusage();
		unsigned char* getsector_bootinfo();
		unsigned char* getsector_diskinfo();
		unsigned char* getsectors_driveusage();
		unsigned char* getcopyofusagetable(); // allocates memory and fills it up with a copy of the current usage table
		void initvars();
		uint32_t driveusagesectorcount();
		uint32_t clustercount();
		uint32_t driveusagefirstsector();
		uint32_t getblockspercluster();
		uint32_t getprojectsectornum(uint32_t base1proj);
		bool isfreecluster(uint32_t clusternum);
		bool isfreecluster(uint32_t clusternum,unsigned char* usagebuffer);
		uint32_t freeclustercount();
		uint32_t getlastsectornum(int* lastsecerror);
		uint32_t getlastsectornum(FSHANDLE handle,int* lastsecerror);
		uint32_t headersectors;
		bool comparebackupblock(uint32_t sector,uint32_t blocksize,
				      uint32_t lastsec);
		void writebackupblock(uint32_t sector,uint32_t blocksize,
				      uint32_t lastsec,bool fullcommit);
		uint32_t getnextfreesectorword();
		uint32_t getnextfreeclusterword();
		uint32_t getnextfreesector(uint32_t cluster);
		void enablebit(uint32_t ibit,unsigned char* usagebuffer);
		void disablebit(uint32_t ibit,unsigned char* usagebuffer);
		bool isbitzero(uint32_t ibit,unsigned char* usagebuffer);
		uint32_t songentry2songsector(uint32_t entry);
		uint32_t songsector2songentry(uint32_t entry);
		void allocsongentry(uint32_t entrynum);
		void allocatecluster(uint32_t clusternum);
		void allocatecluster(uint32_t clusternum,unsigned char* usagebuffer);
		void freecluster(uint32_t clusternum);
		void freecluster(uint32_t clusternum,unsigned char* usagebuffer);
		int32_t nextunusedsongentry();
		void useinternalboot(unsigned char*, uint32_t);
		uint32_t getunusedsongsector();
		uint32_t songsondisk();
		void songsondisk(uint32_t songsondisk);
		void setprojectsectornum(int i,uint32_t newsector);
		void setimagedir(const char* newdir);
		int deviceid;
		
		int driveimagetype;
		hd24driveimage* smartimage;
		FSHANDLE smartimagehandle;


	public:
		uint32_t lasterror;
		static void 	dumpsector(const unsigned char* buffer);
		void force_reload();
		bool isdevicefile(); /* indicates whether the current device
				        is a device file or an (deletable)
				        user file */
		static void setname(unsigned char* namebuf,string newname,uint32_t shortnameoff,uint32_t longnameoff);
		string* drivetype(string*);
		void dumpclusterusage();
		void dumpclusterusage(unsigned char* buffer);
		void dumpclusterusage2();
		void dumpclusterusage2(unsigned char* buffer);
		unsigned char* findorphanclusters();
		static const int TRANSPORTSTATUS_PLAY;
		static const int TRANSPORTSTATUS_STOP;
		static const int TRANSPORTSTATUS_REC;
		uint32_t cluster2sector(uint32_t clusternum);
		uint32_t sector2cluster(uint32_t sectornum);
		uint32_t songentry2sector(uint32_t entrynum);
		uint32_t songsector2entry(uint32_t sectornum);
		void settransportstatus(int newstatus);
		int gettransportstatus();
		bool	isallinput();
		void	setallinput(bool p_allinput);
		void	setallinput(void);
		bool	isautoinput();
		void	setautoinput(bool p_autoinput);
		void	setautoinput(void);

		hd24fs(const char* imagedir);
		hd24fs(const char* imagedir,int mode);
		hd24fs(const char* imagedir,int mode,int base0devnum);
		hd24fs(const char* imagedir,int mode,string* dev,bool force);		
		~hd24fs();
		uint32_t getblocksizeinsectors();
		uint32_t getbytesperaudioblock();
		static void fstfix(unsigned char* bootblock,int fixsize);
		bool isOpen();	
		int mode();
		string* volumename();
		string* getdevicename();
		int getdeviceid();
		void setdevicename(const char* orig,string* newname);
		void setvolumename(string newname);
		string* version();
		uint32_t hd24devicecount();
		int32_t lastprojectid();
		void lastprojectid(int32_t projectid);
		uint32_t projectcount();
		uint32_t maxprojects();
		uint32_t maxsongsperproject();	
		hd24project* getproject(int32_t projectid);
		hd24project* createproject(const char* projectname);
		void setmaintenancemode(int mode);
		int getmaintenancemode();
		void setwavefixmode(int mode);
		int getwavefixmode();
		string* gethd24currentdir();
		static const int MODE_RDONLY;
		static const int MODE_RDWR;
		string* freespace(uint32_t rate,uint32_t tracks);
		bool useheaderfile(string headerfilename);
		bool isexistingdevice(string *devname);
		bool commit(bool fullcommit);
		bool commit();		
		bool commit_ok(); /* compares header with its backup */
		uint32_t setsectorchecksum(unsigned char* buffer,uint32_t startoffset,uint32_t startsector,uint32_t sectors);
		void savedriveinfo();
		void savedriveusage();
		uint32_t writesuperblock(uint32_t lastsectornum);
		void cleardriveinfo(unsigned char* buffer);
		uint32_t writedriveinfo();
		uint32_t writeemptysongusagetable();
		uint32_t writedriveusage();
		uint32_t quickformat(char* lasterror);
		uint32_t deleteproject(int32_t projid);
		void write_enable();
		void write_disable();
};

/* Provides sector level access to hd24 disks 
 * (functionality which is private in hd24fs) 
 */
class hd24raw 
{
	friend class hd24fs;

	private:
		hd24fs* fsys;
		
	public:
		hd24raw(hd24fs* fsys);
		hd24raw(hd24fs* fsys,bool notranslate);
		~hd24raw() {};
		long readsectors(uint32_t secnum, unsigned char* buffer,int sectors);
		long writesectors(uint32_t secnum, unsigned char* buffer,int sectors);
		uint32_t getnextfreesector(uint32_t cluster);
		uint32_t getprojectsectornum(uint32_t projectid);
		uint32_t getlastsectornum(int* lastsecerror);
		uint32_t songsondisk();
		uint32_t quickformat(char* lasterror); 
};

#endif
