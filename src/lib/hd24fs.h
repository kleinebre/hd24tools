#ifndef __hd24fs_h__
#define __hd24fs_h__

#include <config.h>
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
	virtual __uint32 samplerate() { return 0; };
	virtual void currentlocation(__uint32 newpos) { return; };
	virtual __uint32 currentlocation() { return 0; };
	virtual __uint32 getlocatepos(int locatepoint) { return 0; };
	virtual __uint32 setlocatepos(int locatepoint,__uint32 newpos) { return 0; };
	virtual bool trackarmed(__uint32 base1tracknum) { return false; }
	virtual void trackarmed(__uint32 base1tracknum,bool arm) { return; }
	virtual ~AudioStorage() { return; } ;
};

class hd24song : public AudioStorage
{
	friend class hd24project;
	friend class hd24fs;
	friend class hd24transferjob;
	friend class hd24transferengine;
	private:
		__uint32 framespersec;
		unsigned char* buffer;		// for songinfo
		unsigned char* audiobuffer;	// for audio data 
		unsigned char* scratchbook;	// for write-back audio data
		__uint32* blocksector;
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
		__uint32* cachebuf_blocknum;
		hd24project* parentproject;
		hd24song(hd24project* p_parent,__uint32 p_songid);
		unsigned char* getcachedbuffer(long unsigned int);
		void	queuecacheblock(__uint32 blocknum);
		void	loadblockintocache(__uint32 blocknum);
		void	setblockcursor(__uint32 blocknum);
		void 	memoizeblocksectors(__uint32 lastblock);
		__uint32 memblocksector(__uint32 blocknum);
		__uint32 blocktoqueue;		// next block to cache
		__uint32 songcursor;		// current cursor pos within song, in samples
		__uint32 allocentrynum;		// which allocation entry is currently being used
		__uint32 allocstartblock;	// the first audioblock in given entry
		__uint32 allocstartsector;	// the sector pointed to by that entry
		__uint32 allocaudioblocks;	// the number of audioblocks in the block
		__uint32 divider;
		__uint32 lastreadblock;
		__uint32 lastavailablecacheblock;
		__uint32 mustreadblock;
		__uint32 track_armed[24];
		__uint32 track_readenabled[24]; // used to speed up copy mode.
		void unmark_used_clusters(unsigned char* sectors_orphan);
		__uint32 used_alloctable_entries();
		__uint32 audioblocks_in_alloctable();
		void silenceaudioblocks(__uint32 allocsector,__uint32 blocks);
		bool allocatenewblocks(long unsigned int, bool, char*, int*, int (*)());

	public:
		~hd24song();
		string* songname();
		bool endofsong();
		__uint32 songid();
		hd24fs* fs();
		static void sectorinit(unsigned char* sectorbuf);
		static void settrackcount(unsigned char* sectorbuf,__uint32 trackcount);
		static void songname(unsigned char* sectorbuf,string newname);
		void songname(string newname);
		static string* songname(hd24fs* parentfs,unsigned char* sectorbuf);
		void bufferpoll();
		
		bool loadlocpoints(string* filename);
		bool savelocpoints(string* filename);
		void readenabletrack(__uint32 tracknum);
		void readenabletrack(__uint32 tracknum,bool enable);

		bool isrehearsemode();
		void setrehearsemode(bool p_rehearsemode);
		bool recording();

		bool trackarmed(__uint32 tracknum);
		void trackarmed(__uint32 tracknum,bool arm);

		void armalltracks();
		void unarmalltracks();

		bool istrackmonitoringinput(__uint32 tracknum);
		void startrecord(int record_mode);
		void stoprecord();
		__uint32 samplerate();
		static void samplerate(unsigned char* sectorbuf,__uint32 samplerate);
		void samplerate(__uint32 newrate);
		static __uint32 samplerate(unsigned char* songbuf);
		__uint32 bitdepth();
		__uint32 physical_channels();
		__uint32 chanmult();
		static __uint32 chanmult(unsigned char* songbuf);
		static __uint32 physical_channels(unsigned char* songbuf);
		void physical_channels(__uint32 newchannelcount);
		static void physical_channels(unsigned char* songbuf,__uint32 newchannelcount);
		__uint32 logical_channels();
		static __uint32 logical_channels(unsigned char* songbuf);
		void logical_channels(__uint32 newchannelcount);
		static void logical_channels(unsigned char* songbuf,__uint32 channelcount);
		__uint64 songsize_in_bytes();
		__uint64 bytes_allocated_on_disk();
		/* Songlength_in_samples isn't suitable for high-samplerate songs
                   as they can be twice the amount of samples long which would require __uint33. 
                   By introducing the concept of sample pair words, or "wamples", a 32 bit number
                   is still enough for double the length in samples at double the sample rate.  */
		__uint32 songlength_in_wamples();
		__uint32 songlength_in_wamples(__uint32 newlen);
		__uint32 songlength_in_wamples(__uint32 newlen,bool silence);
		__uint32 songlength_in_wamples(__uint32 newlen,bool silence,char* savemessage,int* cancel);
		__uint32 songlength_in_wamples(__uint32 newlen,bool silence,char* savemessage,int* cancel,int (*checkfunc)());

		__uint32 display_hours();
		__uint32 display_hours(__uint32 offset);
		static __uint32 display_hours(__uint32 offset,__uint32 samrate);
		
		__uint32 display_minutes();
		__uint32 display_minutes(__uint32 offset);
		static __uint32 display_minutes(__uint32 offset,__uint32 samrate);

		__uint32 display_seconds();
		__uint32 display_seconds(__uint32 offset);
		static __uint32 display_seconds(__uint32 offset,__uint32 samrate);

		__uint32 display_subseconds();
		__uint32 display_subseconds(__uint32 offset);
		static __uint32 display_subseconds(__uint32 offset,__uint32 samrate);

		string*  display_duration();
		string*  display_duration(__uint32 offset);
		string*  display_duration(__uint32 offset,__uint32 samrate);

		string*  display_cursor();
		__uint32 cursorpos();
		__uint32 locatepointcount();
		__uint32 getlocatepos(int locatepoint);

		__uint32 currentlocation();
		void currentlocation(__uint32 offset);
		__uint32 golocatepos(__uint32 offset);

		__uint32 setlocatepos(int locatepoint,__uint32 offset);
		void setlocatename(int locatepoint,string newname);
		string* getlocatename(int locatepoint);
		bool iswriteprotected();
		void setwriteprotected(bool prot);
		void getmultitracksample(long* mtsample,int readmode);
		int getmtrackaudiodata(__uint32 firstsamnum,__uint32 samples,unsigned char* buffer,int readmode);
		int putmtrackaudiodata(__uint32 firstsamnum,__uint32 samples,unsigned char* buffer,int writemode);
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

		__uint32 getnextfreesector(__uint32 allocsector);
		bool has_unexpected_end();	  // indicates if there is an 'unexpected end of song' error
		bool is_fixable_unexpected_end(); // ...and if so, if we know how to fix it.
		bool setallocinfo(bool silencenew);
		bool setallocinfo(bool silencenew,char* message,int* cancel,int (*checkfunc)());
		__uint32 requiredaudioblocks(__uint32 songlen);
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
		__sint32 myprojectid;
		hd24song* songlist;	
		hd24project(hd24fs* p_parent,__sint32 projectid);
		hd24project(hd24fs* p_parent,__sint32 projectid,__uint32 projsector,const char* projectname,bool isnew);
		__uint32 getsongsectornum(int i);
		void setsongsectornum(int i,__uint32 newsector);
		void save(__uint32 projsector);
		void populatesongusagetable(unsigned char* songused);
		__uint32 getunusedsongslot();
		void initvars(hd24fs* p_parent,__sint32 p_projectid);
	public:
		~hd24project();
		string* projectname();
		void projectname(string newname);
		__sint32 lastsongid();
		void lastsongid(signed long songid);
		__uint32 songcount();
		__uint32 maxsongs();
		__sint32 projectid();
		hd24song* getsong(__uint32 songid);
		hd24song* createsong(const char* songname,__uint32 trackcount,__uint32 samplerate);
		__uint32 deletesong(__uint32 songid);
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
		static __uint32 bytenumtosectornum(__uint64 flen);
		static __uint64 windrivesize(FSHANDLE handle);
		__uint32 highestFSsectorwritten;
		bool needcommit;

		__uint32 nextfreeclusterword;	// memoization cache for write allocation
		
		FSHANDLE foundlastsectorhandle; // last handle for which we found last sectornum
		FSHANDLE devhd24;	// device handle
		FSHANDLE hd24header;	// header device handle
		bool m_isOpen;
		bool gotlastsectornum;
		__uint32 foundlastsectornum;
		int p_mode;
		hd24project* projlist;	
		string* devicename;	
		unsigned char* sector_boot;
		unsigned char* sector_diskinfo;
		unsigned char* sectors_driveusage;	
		unsigned char* sectors_orphan;	
		unsigned char* sectors_songusage;
		long readsectors(FSHANDLE handle, __uint32 secnum, unsigned char* buffer,int sectors);
//		long readsector(FSHANDLE handle, __uint32 secnum, unsigned char* buffer);
//		long readsector_noheader(FSHANDLE handle, __uint32 secnum, unsigned char* buffer);
//		long readsector_noheader(hd24fs* currenthd24, __uint32 secnum, unsigned char* buffer);
		long readsectors_noheader(FSHANDLE handle, __uint32 secnum, unsigned char* buffer,__uint32 count);
		long readsectors_noheader(hd24fs* currenthd24, __uint32 secnum, unsigned char* buffer,__uint32 count);
		long writesectors(FSHANDLE handle, __uint32 secnum, unsigned char* buffer,int sectors);
//		long writesector(FSHANDLE handle, __uint32 secnum, unsigned char* buffer);

		string* gethd24currentdir(int, char**);
		void hd24closedevice(FSHANDLE handle,const char* source);
		void hd24sync();
		static void hd24seek(FSHANDLE handle,__uint64 seekpos);
		void clearbuffer(unsigned char* buffer);
		void clearbuffer(unsigned char* buffer,unsigned int bytes);
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
		__uint32 driveusagesectorcount();
		__uint32 clustercount();
		__uint32 driveusagefirstsector();
		__uint32 getblockspercluster();
		__uint32 getprojectsectornum(__uint32 base1proj);
		bool isfreecluster(__uint32 clusternum);
		bool isfreecluster(__uint32 clusternum,unsigned char* usagebuffer);
		__uint32 freeclustercount();
		__uint32 getlastsectornum(int* lastsecerror);
		__uint32 getlastsectornum(FSHANDLE handle,int* lastsecerror);
		__uint32 headersectors;
		bool comparebackupblock(__uint32 sector,__uint32 blocksize,
				      __uint32 lastsec);
		void writebackupblock(__uint32 sector,__uint32 blocksize,
				      __uint32 lastsec,bool fullcommit);
		__uint32 getnextfreesectorword();
		__uint32 getnextfreeclusterword();
		__uint32 getnextfreesector(__uint32 cluster);
		void enablebit(__uint32 ibit,unsigned char* usagebuffer);
		void disablebit(__uint32 ibit,unsigned char* usagebuffer);
		bool isbitzero(__uint32 ibit,unsigned char* usagebuffer);
		long unsigned int songentry2songsector(long unsigned int entry);
		long unsigned int songsector2songentry(long unsigned int entry);
		void allocsongentry(__uint32 entrynum);
		void allocatecluster(__uint32 clusternum);
		void allocatecluster(__uint32 clusternum,unsigned char* usagebuffer);
		void freecluster(__uint32 clusternum);
		void freecluster(__uint32 clusternum,unsigned char* usagebuffer);
		signed long nextunusedsongentry();
		void useinternalboot(unsigned char*, long unsigned int);
		__uint32 getunusedsongsector();
		__uint32 songsondisk();
		void songsondisk(__uint32 songsondisk);
		void setprojectsectornum(int i,__uint32 newsector);
		void setimagedir(const char* newdir);
		int deviceid;
		
		int driveimagetype;
		hd24driveimage* smartimage;
		FSHANDLE smartimagehandle;


	public:
		__uint32 lasterror;
		static void 	dumpsector(const unsigned char* buffer);
		void force_reload();
		bool isdevicefile(); /* indicates whether the current device
				        is a device file or an (deletable)
				        user file */
		static void setname(unsigned char* namebuf,string newname,__uint32 shortnameoff,__uint32 longnameoff);
		string* drivetype(string*);
		void dumpclusterusage();
		void dumpclusterusage(unsigned char* buffer);
		void dumpclusterusage2();
		void dumpclusterusage2(unsigned char* buffer);
		unsigned char* findorphanclusters();
		static const int TRANSPORTSTATUS_PLAY;
		static const int TRANSPORTSTATUS_STOP;
		static const int TRANSPORTSTATUS_REC;
		__uint32 cluster2sector(__uint32 clusternum);
		__uint32 sector2cluster(__uint32 sectornum);
		__uint32 songentry2sector(__uint32 entrynum);
		__uint32 songsector2entry(__uint32 sectornum);
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
		__uint32 getblocksizeinsectors();
		__uint32 getbytesperaudioblock();
		static void fstfix(unsigned char* bootblock,int fixsize);
		bool isOpen();	
		int mode();
		string* volumename();
		string* getdevicename();
		int getdeviceid();
		void setdevicename(const char* orig,string* newname);
		void setvolumename(string newname);
		string* version();
		__uint32 hd24devicecount();
		__sint32 lastprojectid();
		void lastprojectid(signed long projectid);
		__uint32 projectcount();
		__uint32 maxprojects();
		__uint32 maxsongsperproject();	
		hd24project* getproject(__sint32 projectid);
		hd24project* createproject(const char* projectname);
		void setmaintenancemode(int mode);
		int getmaintenancemode();
		void setwavefixmode(int mode);
		int getwavefixmode();
		string* gethd24currentdir();
		static const int MODE_RDONLY;
		static const int MODE_RDWR;
		string* freespace(__uint32 rate,__uint32 tracks);
		bool useheaderfile(string headerfilename);
		bool isexistingdevice(string *devname);
		bool commit(bool fullcommit);
		bool commit();		
		bool commit_ok(); /* compares header with its backup */
		long unsigned int setsectorchecksum(unsigned char* buffer,unsigned int startoffset,unsigned int startsector,unsigned int sectors);
		void savedriveinfo();
		void savedriveusage();
		__uint32 writesuperblock(__uint32 lastsectornum);
		void cleardriveinfo(unsigned char* buffer);
		__uint32 writedriveinfo();
		__uint32 writeemptysongusagetable();
		__uint32 writedriveusage();
		__uint32 quickformat(char* lasterror);
		__uint32 deleteproject(__sint32 projid);
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
		long readsectors(__uint32 secnum, unsigned char* buffer,int sectors);
		long writesectors(__uint32 secnum, unsigned char* buffer,int sectors);
		__uint32 getnextfreesector(__uint32 cluster);
		__uint32 getprojectsectornum(__uint32 projectid);
		__uint32 getlastsectornum(int* lastsecerror);
		__uint32 songsondisk();
		__uint32 quickformat(char* lasterror); 
};

#endif
