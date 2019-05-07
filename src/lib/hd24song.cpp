#include <config.h>
#include <fstream>
#include "hd24fs.h"
#include "convertlib.h"
#include "memutils.h"
#define FRAMESPERSEC 30 /* 30 or 100; 30=default for HD24 (=SMPTE rate) */
#if (SONGDEBUG==1)
#define MEMLEAKMULT 10000
#else
#define MEMLEAKMULT 1
#endif
#define CACHEBUFFERS	30		/* enough for 25 locate points+some lookahead */
#define NOTHINGTOQUEUE			0xFFFFFFFF 
#define CACHEBLOCK_UNUSED		0xFFFFFFFF /* a song can never have this number of blocks
						      because this is the max no. of samples in a song
						      and a block consists of multiple samples */
#define SONGINFO_AUDIOBLOCKS		0x00
#define SONGINFO_SECSAMPLESWIDTH	0x8 /* *512=samples before switch to next track */
#define SONGINFO_SECBYTESWIDTH		0xC /* *512=bytes before switch to next track */
#define SONGINFO_SECSECWIDTH		0x10 /* SECSAMPLESWIDTH*512 */
#define SONGINFO_SONGNAME_8		0x28
#define SONGINFO_CHANNELS		0x31
#define SONGINFO_SAMPLERATE		0x34
#define SONGINFO_BITDEPTH		0x37
#define SONGINFO_SONGLENGTH_IN_WAMPLES	0x38
#define SONGINFO_WRITEPROTECTED		0x3c
#define	SONGINFO_LOCATEPOINTLIST	0xb8
#define LOCATEENTRY_LENGTH		12
#define SONGINFO_SONGNAME 		0x3b8
#define SONGINFO_ALLOCATIONLIST		0x400
#define ALLOCINFO_ENTRYLEN	8
#define ALLOCINFO_SECTORNUM		0x00
#define ALLOCINFO_AUDIOBLOCKSINBLOCK	0x04
#define ALLOC_ENTRIES_PER_SONG	(ALLOC_SECTORS_PER_SONG*(512/ALLOCINFO_ENTRYLEN))
#define LOCATE_TIMECODE		0
#define LOCATE_NAME		4
/*	   Quick calculation: A song is max 2^32 samples * 3 bytes *24 tracks
	   = 309 237 645 312 bytes
	   1 block=0x480h sectors = 589 824 bytes
	   So the max number of blocks in a song = (309237645312 / 589824) blocks = 524288 blocks
*/
#define MAX_BLOCKS_IN_SONG 524288
const int hd24song::LOCATEPOS_SONGSTART	=0;
const int hd24song::LOCATEPOS_LOOPSTART	=1;
const int hd24song::LOCATEPOS_LOOPEND	=2;
const int hd24song::LOCATEPOS_PUNCHIN	=21;
const int hd24song::LOCATEPOS_PUNCHOUT	=22;
const int hd24song::LOCATEPOS_EDITIN	=23;
const int hd24song::LOCATEPOS_EDITOUT	=24;
const int hd24song::LOCATEPOS_LAST	=24;
const int hd24song::LOCATELIST_BYTELEN  =(hd24song::LOCATEPOS_LAST+1)*LOCATEENTRY_LENGTH;
const int hd24song::READMODE_COPY       =0;
const int hd24song::READMODE_REALTIME   =1;
const int hd24song::WRITEMODE_COPY      =2;
const int hd24song::WRITEMODE_REALTIME  =3;

bool hd24song::loadlocpoints(string* locpointfilename)
{	
//SONGINFO_LOCATEPOINTLIST	0xb8
// LOCATEENTRY_LENGTH		12
//	LOCATEPOS_LAST	=24;
        char locheader[5]="\0loc";
	ifstream loadFile(locpointfilename->c_str(),ios::in|ios::binary);
	if (!loadFile)
	{
		return false;
	}
	char loadbuffer[LOCATELIST_BYTELEN+4];
	loadFile.read(loadbuffer,LOCATELIST_BYTELEN+4);

	if (memcmp(loadbuffer,locheader,4)!=0)
	{
		return false;
	}
	memcpy((&(this->buffer[0])+SONGINFO_LOCATEPOINTLIST),&loadbuffer[4],LOCATELIST_BYTELEN);
	return true;
}

bool hd24song::savelocpoints(string* locpointfilename)
{
	ofstream saveFile(locpointfilename->c_str(),ios::out|ios::binary);


        char locheader[5]="\0loc";
	saveFile.write(&(locheader[0]),4);


	saveFile.write((const char*)(&(this->buffer[0])+SONGINFO_LOCATEPOINTLIST),LOCATELIST_BYTELEN);
	
	return true;
}

void hd24song::loadblockintocache(__uint32 blocktoqueue) 
{
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	
	for (int i=LOCATEPOS_LAST;i<CACHEBUFFERS;i++) {
		if (cachebuf_blocknum[i]==blocktoqueue) {
			return; // already in cache.
		}
	}
//	Cache block blocktoqueue
	cachebuf_blocknum[currcachebufnum]=blocktoqueue;

////////////////////// TODO: THIS BLOCK OF CODE CAN BE REPLACED BY GETFIRSTBLOCKSECTOR
	__uint32 rtallocentrynum=0;		// reset cursor to start of song
	__uint32 rtallocstartblock=0; 	// blocknum of first block in current allocation entry
	__uint32 rtallocstartsector=Convert::getint32(buffer,SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*rtallocentrynum)+ALLOCINFO_SECTORNUM);
	__uint32 rtallocaudioblocks=Convert::getint32(buffer,SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*rtallocentrynum)+ALLOCINFO_AUDIOBLOCKSINBLOCK);
	__uint32 blocknum=blocktoqueue;
	
	while ((blocknum-rtallocstartblock) >= rtallocaudioblocks) {
		rtallocentrynum++;			// reset cursor to start of song
		if (rtallocentrynum>=ALLOC_ENTRIES_PER_SONG ) break;
		rtallocstartblock+=rtallocaudioblocks; 	// blocknum of first block in current allocation entry
		rtallocstartsector=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*rtallocentrynum)+ALLOCINFO_SECTORNUM);
		rtallocaudioblocks=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*rtallocentrynum)+ALLOCINFO_AUDIOBLOCKSINBLOCK);

	}
///////////////////////////////////////////////////
	parentfs->readsectors(parentfs->devhd24,
		rtallocstartsector+((blocknum-rtallocstartblock)*blocksize_in_sectors),
		cachebuf_ptr[currcachebufnum],blocksize_in_sectors); // raw read

//	cachebuf_ptr[currcachebufnum]=NULL; // TODO: READ SECTORS!!!
	
	currcachebufnum++;
	if (currcachebufnum>=CACHEBUFFERS)
	{
		currcachebufnum=LOCATEPOS_LAST+1;
	}
	return;
}


void hd24song::bufferpoll() {
	// A process can call this procedure to tell the song
	// object to check the cache request queue for 	blocks
	// to cache.
	if (currentreadmode==READMODE_COPY) return;
	if (polling==1) 
	{ 
		// Previous poll is still in progress.
		// This is not a proper semaphore system but will
		// help relief processing weight should the system
		// get overloaded. Normally bufferpoll shouldn't be 
		// called much more than around 20 times per second,
		// so the chance two polls interfere with one another
                // is minimal.
		return; 
	}
	polling=1;	// semaphore
	if (blocktoqueue!=NOTHINGTOQUEUE) 
	{
		loadblockintocache(blocktoqueue);
		blocktoqueue=NOTHINGTOQUEUE;
	}
	polling=0;	// poll done
}

__uint32 hd24song::locatepointcount() {
	return LOCATEPOS_LAST+1;
}

__uint32 hd24song::getlocatepos(int locatepoint)
{
	if (locatepoint<0) locatepoint=0;
	if (locatepoint>LOCATEPOS_LAST) return songlength_in_wamples();
	long entryoffset=SONGINFO_LOCATEPOINTLIST+(locatepoint*LOCATEENTRY_LENGTH);
	return Convert::getint32(buffer,entryoffset+LOCATE_TIMECODE);
}

string* hd24song::getlocatename(int locatepoint)
{
	if (locatepoint<0) locatepoint=0;
	if (locatepoint>LOCATEPOS_LAST) {
		string* newstr=new string("END");
		return newstr;
	}
	long entryoffset=SONGINFO_LOCATEPOINTLIST+(locatepoint*LOCATEENTRY_LENGTH);
	return Convert::readstring(buffer,entryoffset+LOCATE_NAME,8);
}


void hd24song::setlocatename(int locatepoint,string newname)
{
	
	if (locatepoint<0) locatepoint=0;
	if (locatepoint>LOCATEPOS_LAST) return;
	while (newname.length()<8) {
		newname+=" ";
	}
	long entryoffset=SONGINFO_LOCATEPOINTLIST+(locatepoint*LOCATEENTRY_LENGTH);
	for (__uint32 i=0;i<8;i++) {
		buffer[entryoffset+LOCATE_NAME+i]=newname.c_str()[i];
	}
	return;
}
void hd24song::silenceaudioblocks(__uint32 allocsector,__uint32 numblocks)
{
	/* Given a sector number and a block count, silence the 
           given number of audio blocks on the drive starting
           from the given sector number.
           This function has 2 modes- one working with a 1-sector
	   stack-allocated block (slow), the other working with
           a heap-allocated cluster (fast but memory intensive).
           The heap method may need up to a few (2.3 or so) megabytes of RAM.
	   If heap allocation fails, the slow method is used.
	*/
	unsigned char onesector[512];
	memset(onesector,0,512);

	__uint32 sectorstoclear=parentfs->getblocksizeinsectors();
	sectorstoclear*=numblocks;
	unsigned char* clearblock=(unsigned char*)memutils::mymalloc("silenceaudioblocks",sectorstoclear*512,1);
	if (clearblock==NULL) 
	{
		// Alloc failed, use low-memory use version
		for (__uint32 i=0;i<sectorstoclear;i++) 
		{
			parentfs->writesectors(parentfs->devhd24,
			allocsector+i,
			onesector,
			1);
		}
	}
	else 
	{
		memset(clearblock,0,512*sectorstoclear);
		parentfs->writesectors(parentfs->devhd24,
		allocsector,
		clearblock,
		sectorstoclear);
		memutils::myfree("silenceaudioblocks",clearblock);
	}
	return;
}

bool hd24song::setallocinfo(bool silencenew)
{
	return setallocinfo(silencenew,NULL,NULL,NULL);
}

__uint32 hd24song::requiredaudioblocks(__uint32 songlen_in_wamps)
{
	/* Figure out how many audio blocks we would expect 
	   the song to have based on the songlength in wamples. 
           Blocks will be used twice as fast for high samplerate songs
           as a "wample" equals 2 samples. */
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;
	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 tracks_per_song=physical_channels();
	__uint32 tracksamples_per_block=0;
	if (tracks_per_song>0) {
		tracksamples_per_block=(blocksize_in_bytes / bytes_per_sample) / tracks_per_song;
	}
	__uint32 wamples_per_block=tracksamples_per_block/this->chanmult();	
	__uint32 remainder=songlen_in_wamps%wamples_per_block;
	__uint32 blocks_expected=(songlen_in_wamps-remainder)/wamples_per_block;
	if (remainder!=0) {
		blocks_expected++;
	}
        return blocks_expected;
}

bool hd24song::allocatenewblocks(__uint32 blockstoalloc,bool silencenew,char* message,int* cancel,int (*checkfunc)())
{
	/* Allocate space in the real drive usage table.
           Mind that we've been asked to allocate a certain
           number of blocks, although in reality we are not
           allocating blocks but clusters of blocks. */
#if (SONGDEBUG == 1)
	cout << "Total blocks to alloc=" << blockstoalloc << endl;
#endif
	__uint32 totblockstoalloc=blockstoalloc;
	__uint32 allocsector=0;
	__uint32 blockspercluster=parentfs->getblockspercluster();
	cancel=cancel;
	while (blockstoalloc>0)
	{
		__uint32 pct=(__uint32)((100*(totblockstoalloc-blockstoalloc))/totblockstoalloc);
		if (message!=NULL) {
			sprintf(message,
				"Lengthening song... allocating block %ld of %ld, %ld%% done",
				(long)(totblockstoalloc-blockstoalloc),
				(long)totblockstoalloc,
				(long)pct
			);
		}
		if (checkfunc!=NULL)
		{
			checkfunc();
		}
		allocsector=getnextfreesector(allocsector);

#if (SONGDEBUG == 1)
		cout << "Allocsector=" << allocsector << endl
		<< "Blockstoalloc=" << blockstoalloc << endl;
#endif
		if (allocsector==0) {
#if (SONGDEBUG == 1)
	cout << "Ran out of space with " << blockstoalloc 
             <<" left to alloc " << endl;
#endif
			
			return false;
		}
		if (silencenew)
		{
			// overwrite cluster with silence.
#if (SONGDEBUG == 1)
			cout << "Overwriting cluster with silence." << endl;
#endif
			this->silenceaudioblocks(allocsector,blockspercluster);
		}
		__uint32 alloccluster=parentfs->sector2cluster(allocsector);
#if (SONGDEBUG == 1)
		cout << "Alloccluster=" << alloccluster << endl;
#endif
		parentfs->enablebit(alloccluster,parentfs->sectors_driveusage);
                if (blockstoalloc>=blockspercluster)
                {
			blockstoalloc-=blockspercluster;	
                } 
		else 
		{
			blockstoalloc=0;
		}
	}
	return true;
}

bool hd24song::setallocinfo(bool silencenew,char* message,int* cancel,int (*checkfunc)())
{
	/* This function is intended for recovering live recordings
	   and lengthening songs. To use it, set a new song length
           first, then call this function- it will add the required
           number of blocks to the song.

	   Boolean 'silencenew' indicates if newly allocated space
           should be overwritten with silence.

           For initializing songs to nonzero length, you will want
           to set this to TRUE.

           For recovering live recordings you will want to set it
           to FALSE.

           For realtime recording, it is set to FALSE for efficiency
           reasons, as the recording algorithm itself will overwrite
           newly allocated space with audio (and silence as needed).
 
	   Savemessage allows giving textual feedback to the user 
           and the int pointed to by cancel will be set to 1 by the 
           GUI if the user interrupts the process. 
           In case of recovering a song, after setting the length of
           a crashed song to the estimated duration, we want to try 
           to find back the audio.

           It is reasonable that this previously recorded audio can be
           found by simply allocating as many unused clusters to the
           song as needed to reach the desired length; because those
           same clusters would have been allocated to the song after
           pressing 'stop'.

           This function attempts to perform this allocation (which
           should also allow people to perform headerless 
           live recoveries).

	   The way this will work is:
 	   -- find out how many audio blocks are allocated to the song;
           -- find out how many we think there *should* be allocated;
           -- then, allocate those blocks (in a copy of the usage table);
           -- finally, append the newly allocated blocks to the song.
           -- the last can be done by subtracting the old usage table
              from the new one and calling appendorphanclusters.

	   FIXME: operation is not correct when running out of drive space.
	   TODO: we count blocks to allocate but in reality clusters are
                 being reserved. Shouldn't we work cluster based all the
                 way?
        */

	__uint32 blocksinalloctable=audioblocks_in_alloctable();
	__uint32 blocks_expected=requiredaudioblocks(songlength_in_wamples());


#if (SONGDEBUG == 1)
	cout << "Actual   blocks allocated for song:" << blocksinalloctable << endl
	<< "Expected blocks allocated for song:" << blocks_expected << endl;
#endif
	if (blocksinalloctable==blocks_expected) 
	{
		// right amount of space is already allocated.
		return true;
	}

	if (blocksinalloctable>blocks_expected) 
	{
		// looks like too much space is allocated,
		// but setallocinfo() won't support song shrinking
		// for now.
		return false;
	}

	/* Not enough space is allocated-- allocate as much extra as needed. */
#if (SONGDEBUG == 1)
	cout << "Allocating space for song. " <<endl;
#endif
	__uint32 blockstoalloc=blocks_expected-blocksinalloctable;

	unsigned char* copyusagetable=parentfs->getcopyofusagetable();
	if (copyusagetable==NULL) 
	{
		/* Cannot get usage table (out of memory?) */
		return false;
	}

	bool tryallocnew=this->allocatenewblocks(blockstoalloc,silencenew,message,cancel,checkfunc);
	if (tryallocnew==false)
	{
		/* Cannot allocate new blocks (out of drive space?) */
		memutils::myfree("copyusagetable",copyusagetable);
		return false;
	}

	/* Cluster allocation succeeded. 
           To find out which clusters have been allocated,
           XOR the previous copy of the usage table over it.
           This will result in a list of newly allocated 
           (orphan) clusters still to be appended to the song.
        */
	for (__uint32 i=0;i<(512*15);i++) 
	{
		copyusagetable[i]=(copyusagetable[i])
                                 ^(parentfs->sectors_driveusage[i]);
	}
	
#if (SONGDEBUG == 1)
	cout << "Alloc action successful- append orphan clusters now." << endl;
#endif
	// call appendorphanclusters
	if (message!=NULL) 
	{
		sprintf(message,"Adding allocated space to song...");
		if (checkfunc!=NULL)
		{
			checkfunc();
		}
	}
	bool songresize;
	if (silencenew) {
		songresize=false;
	}
	else 
	{
		songresize=true;
	}
	appendorphanclusters(copyusagetable,songresize);
	memutils::myfree("copyusagetable",copyusagetable);
	// save for either song or drive usage table is not
	// to be called here- it would violate the concept
	// of safe, read-only recovery.	
	if (message!=NULL) 
	{
		sprintf(message,"Added allocated space to song.");
		if (checkfunc!=NULL)
		{
			checkfunc();
		}
	}
	return true;
}

void hd24song::appendorphanclusters(unsigned char* usagebuffer,bool allowsongresize)
{
	__uint32 clusters=parentfs->clustercount();
	__uint32 currpos=0;
	__uint32 curralloctableentry=used_alloctable_entries();
#if (SONGDEBUG == 1)
	cout << "Appending orphan clusters to song. Used alloctable entries=" << curralloctableentry 
	<< "clusters=" << clusters
	<< endl;
#endif

	while (currpos<clusters) {
		__uint32 blockstart=currpos;
		while (parentfs->isfreecluster(blockstart,usagebuffer) && (blockstart<clusters)) {
			blockstart++;
		}
#if (SONGDEBUG == 1)
		cout << "Block starts at cluster " <<blockstart << endl;
#endif
		if (blockstart==clusters) {
			break;
		}

		// blockstart now points to a nonfree cluster
		__uint32 blockend=blockstart;
		while (!parentfs->isfreecluster(blockend,usagebuffer) && (blockend<clusters)) {
			blockend++;
		}
		// blockend now points to a free cluster
		currpos=blockend;
		__uint32 blocklen=blockend-blockstart;

		__uint32 entrystartsector=(unsigned int) (parentfs->cluster2sector(blockstart));
		__uint32 entrynumblocks=(unsigned int)( parentfs->getblockspercluster()*blocklen );
		Convert::setint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_SECTORNUM
			+(ALLOCINFO_ENTRYLEN*curralloctableentry),entrystartsector);

		Convert::setint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_AUDIOBLOCKSINBLOCK
			+(ALLOCINFO_ENTRYLEN*curralloctableentry),entrynumblocks);
		curralloctableentry++;
#if (SONGDEBUG == 1)
		printf("%x %x\n",(unsigned int)parentfs->cluster2sector(blockstart),(unsigned int)( parentfs->getblockspercluster()*blocklen ));
#endif
	}
	/* the operation may have resulted in the song getting	
	   longer. This is due to the fact that while recording,
           'stop' may be pressed before all audio blocks of the
           cluster have been used.
           Also, of course, there may be more orphaned clusters
           around than belong to the song- for whatever reason.
        */

	__uint32 blocksinalloctable=audioblocks_in_alloctable();
	
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;
	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 bytes_per_wample=bytes_per_sample*chanmult();
	__uint32 tracks_per_song=physical_channels();
	__uint32 trackwamples_per_block=0;
	if (tracks_per_song>0) {
		trackwamples_per_block=(blocksize_in_bytes / bytes_per_wample) / tracks_per_song;
	}
	__uint32 newsonglen=trackwamples_per_block*blocksinalloctable;
	Convert::setint32(buffer,SONGINFO_AUDIOBLOCKS,blocksinalloctable);

	/* The following directly sets the songlength in the song buffer
	   rather than via songlength_in_wamples(val) to prevent
	   testing whether more space needs to be allocated-
           which is not needed as space has just been allocated. 
           (Also, the below number may be less accurate than the
           number some user might specify via songlength_in_samples(val)).
        */
	if (allowsongresize)
	{
        	Convert::setint32(buffer,SONGINFO_SONGLENGTH_IN_WAMPLES,newsonglen);
	}
       	return; 
}

void hd24song::setblockcursor(__uint32 blocknum) 
{
	allocentrynum=0;	// reset cursor to start of song
	allocstartblock=0; 	// blocknum of first block in current allocation entry
	allocstartsector=Convert::getint32(buffer,SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*allocentrynum)+ALLOCINFO_SECTORNUM);
	allocaudioblocks=Convert::getint32(buffer,SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*allocentrynum)+ALLOCINFO_AUDIOBLOCKSINBLOCK);
	while ((blocknum-allocstartblock) >= allocaudioblocks) {
		allocentrynum++;			// reset cursor to start of song
		allocstartblock+=allocaudioblocks; 	// blocknum of first block in current allocation entry
		allocstartsector=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*allocentrynum)+ALLOCINFO_SECTORNUM);
		allocaudioblocks=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*allocentrynum)+ALLOCINFO_AUDIOBLOCKSINBLOCK);
	}
	return;
}

void hd24song::unmark_used_clusters(unsigned char* sectors_inuse)
{
	/*
 	 * Given an image of used clusters, this function
 	 * will alter that image to unmark the clusters
 	 * in use by this song.
 	 * Under normal circumstances, this is used to
 	 * delete songs. 
 	 * However, it is also useful to search for orphan 
 	 * clusters (by unmarking all clusters in use by 
 	 * all songs- the remaining clusters then must be 
 	 * orphan clusters)
 	 */
#if (SONGDEBUG == 1)
	cout << "unmark used clusters." << endl;
#endif
	__uint32 allocentries=used_alloctable_entries();

	if (allocentries==0) {
#if (SONGDEBUG == 1)
		cout << "Song claims no used allocation entries." << endl;
#endif
		return;
	}

	__uint32 blockspercluster=parentfs->getblockspercluster();

	for (__uint32 i=0; i<allocentries; i++) 
	{
		__uint32 entrystartsector=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_SECTORNUM
			+(ALLOCINFO_ENTRYLEN*i));

		__uint32 entrynumblocks=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_AUDIOBLOCKSINBLOCK
			+(ALLOCINFO_ENTRYLEN*i));


		__uint32 entrystartcluster=parentfs->sector2cluster(entrystartsector);
#if (SONGDEBUG == 1)
		cout << "startsector=" <<entrystartsector << " blocks=" << entrynumblocks <<" clust=" << entrystartcluster << endl;
#endif

		__uint32 entrynumclusters=(entrynumblocks-(entrynumblocks%blockspercluster))/blockspercluster;
		if ((entrynumblocks%blockspercluster)!=0) 
		{
			entrynumclusters++;
		}
#if (SONGDEBUG == 1)
			cout << "buffer=" << buffer << endl;
#endif
		if (entrynumclusters==0) 
		{
#if (SONGDEBUG == 1)
			cout << "nothing to free here." << endl;
#endif

		}
		for (__uint32 j=0;j<entrynumclusters;j++) {
			__uint32 clust2free=j+entrystartcluster;
			parentfs->freecluster(clust2free,sectors_inuse);
#if (SONGDEBUG == 1)
			cout << clust2free << " ";
#endif
		}
#if (SONGDEBUG == 1)
		cout << endl;
#endif
	}
}

__uint32 hd24song::currentlocation()
{
	return songcursor;
}
void hd24song::currentlocation(__uint32 offset)
{
	golocatepos(offset);
}

__uint32 hd24song::golocatepos(__uint32 offset)
{
	/* Offset indicates next sample that will be 
	   played back (or recorded). A song of 1 sample long
	   can have the cursor set at offset 0 or offset 1;
           offset 1 is then beyond the end of the song, which is
	   meaningful for recording but not for playback. */

	__uint32 songlen=songlength_in_wamples();

	if (offset>songlen) {
		offset=songlen;
		//return offset;
	}

	songcursor=offset;
	evenodd=0;

	__uint32 samplenumber=songcursor;	
#if (SONGDEBUG == 1)
//	cout << "songcursor=" << songcursor << endl; 
#endif
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;
	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 tracks_per_song=physical_channels();
	__uint32 tracksamples_per_block=0;
	if (tracks_per_song>0) {
		tracksamples_per_block=(blocksize_in_bytes / bytes_per_sample) / tracks_per_song;
	}
	__uint32 blocknum=0;
	if (tracksamples_per_block>0) {
		blocknum=(samplenumber/(tracksamples_per_block));
	}

#if (SONGDEBUG == 1)
//	cout << "still going strong" << endl; 
#endif

	setblockcursor(blocknum);	

	return songcursor;
}

__uint32 hd24song::setlocatepos(int locatepoint,__uint32 offset)
{
	/** Sets the value of a locate point to the given offset.
            Parameters: 
            locatepoint
		The 0-based locate point identifier
            offset
		The new offset (in samples*) for the locate point.
 		* In high samplerate songs (88k2, 96k), the offset is given as
                number of sample pairs, because audio data is interlaced 
            	across 2 physical tracks.
        */

	if (locatepoint<0) 
        {
		locatepoint=0;
	}

	if (locatepoint>LOCATEPOS_LAST) 
	{
		return 0;
	}

	long entryoffset=SONGINFO_LOCATEPOINTLIST
                        +(locatepoint*LOCATEENTRY_LENGTH);

	buffer[entryoffset+LOCATE_TIMECODE+3]=offset%256;
	offset=offset>>8;
	buffer[entryoffset+LOCATE_TIMECODE+2]=offset%256;
	offset=offset>>8;
	buffer[entryoffset+LOCATE_TIMECODE+1]=offset%256;
	offset=offset>>8;
	buffer[entryoffset+LOCATE_TIMECODE+0]=offset%256;
	return getlocatepos(locatepoint);
}

hd24song::hd24song(hd24project* p_parent,__uint32 p_songid) 
{
#if (SONGDEBUG == 1)
	cout << "CONSTRUCT hd24song " << p_songid << endl;
#endif
	currentreadmode=READMODE_COPY;
	blocktoqueue=NOTHINGTOQUEUE;
	polling=0;
	evenodd=0;
	audiobuffer=NULL;
	scratchbook=NULL;
	buffer=NULL;
	framespersec=FRAMESPERSEC;
	lastallocentrynum=0; 	
	busyrecording=false;
	mysongid=p_songid;
	rehearsemode=false;
	lengthened=false;
	lastavailablecacheblock=0xFFFFFFFF;
	currcachebufnum=LOCATEPOS_LAST+1;
	buffer=(unsigned char*)memutils::mymalloc("hd24song-buffer",16384,1);
	parentfs=p_parent->parentfs;
	parentproject=p_parent;
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;

	for (__uint32 tracknum=1;tracknum<=24;tracknum++) 
	{
		track_armed[tracknum-1]=false;
	}

	// 'read enabled' is used in copy mode to reduce the amount of
	// secors that need to be read from disk.	
	for (__uint32 tracknum=1;tracknum<=24;tracknum++) 
	{
		track_readenabled[tracknum-1]=true; // by default all are read enabled.
	}

#if (SONGDEBUG == 1)
	cout << "2" << endl;
#endif
	audiobuffer=(unsigned char *)memutils::mymalloc("hd24song-audiobuffer",blocksize_in_bytes+SECTORSIZE,1);
	scratchbook=(unsigned char *)memutils::mymalloc("hd24song-scratchbook",blocksize_in_bytes+SECTORSIZE,1);

	if (audiobuffer==NULL) {
#if (SONGDEBUG ==1)
		cout << "could not allocate audio buffer" << endl;
#endif
	}
	
	// Set up cache buffers for realtime access
	// first, dynamically create pointer array
	cachebuf_blocknum=(__uint32*)memutils::mymalloc("hd24song-cachebuf",sizeof(__uint32)*CACHEBUFFERS,1);
	cachebuf_ptr=(unsigned char**)memutils::mymalloc("hd24song-cachebufptr",sizeof (unsigned char *)*CACHEBUFFERS,1);
	// then, allocate blocks and point array to it.
	int i;
	
	for (i=0;i<CACHEBUFFERS;i++)
	{
		cachebuf_ptr[i]=NULL;
	}

	for (i=0;i<CACHEBUFFERS;i++)
	{
		cachebuf_blocknum[i]=CACHEBLOCK_UNUSED;
		cachebuf_ptr[i]=(unsigned char*)memutils::mymalloc("hd24song-cachebufptr[i]",blocksize_in_bytes,1);
	}

	__uint32 songsector=parentproject->getsongsectornum(mysongid);
#if (SONGDEBUG ==1) 
	cout << "Reading # song sectors= " << TOTAL_SECTORS_PER_SONG 
	<< "from sec " << songsector << endl;
#endif	
	parentfs->readsectors(parentfs->devhd24,
			songsector,
			buffer,TOTAL_SECTORS_PER_SONG);
	parentfs->fstfix(buffer,TOTAL_SECTORS_PER_SONG*512);
	
#if (SONGDEBUG ==1) 
	cout << "alloc mem for blocksectors" << endl;	
#endif

	blocksector=(__uint32*)memutils::mymalloc("blocksector",600000,sizeof(__uint32));
#if (SONGDEBUG ==1) 
	cout << "Blocksector=" <<blocksector << endl
	<< "clear blocksectors" << endl;	
#endif
	for (int i=0; i<600000;i++) { 
		blocksector[i]=0; 
	}
	// how many blocks in this song?
#if (SONGDEBUG == 1)
	cout << "cleared blocksectors" << endl
	 << "create song" << mysongid << endl
	 << "blocksize in bytes=" << blocksize_in_bytes << endl
	 << "bitdepth in bytes=" << bitdepth()/8 << endl
	 << "phys_channels=" << physical_channels() << endl;
#endif
	if (physical_channels() >0) 
	{
		__uint32 blocksize_in_wamples=blocksize_in_bytes / (chanmult()*physical_channels()* (bitdepth()/8));
		__uint32 number_of_blocks=(__uint32) floor ( songlength_in_wamples() / blocksize_in_wamples  );
#if (SONGDEBUG == 1)
		cout << "songlen in wam=" <<  songlength_in_wamples() << endl;
#endif
		if (	( songlength_in_wamples() % blocksize_in_wamples ) !=0 ) 
		{
			number_of_blocks++;
		}

	
#if (SONGDEBUG == 1)
		cout << " blocksize in wams = " << blocksize_in_wamples
	       	 << "=" << number_of_blocks << "blocks " << endl

		 << "memoize alloc info for " << number_of_blocks << "blocks." << endl;
#endif
		memoizeblocksectors(number_of_blocks);
	}
	
	divider=0;
	lastreadblock=0; 
	mustreadblock=1; // next time a sample is requested, we must read from disk
	golocatepos(0);
}

__uint32 hd24song::songid()
{
	return this->mysongid;
}

bool hd24song::has_unexpected_end()
{
	// Check if this song has an 'unexpected end of song' error
	// (in header mode, this always returns false)
	if (this->parentfs->headersectors!=0) 
	{
		return false;
	}
	// find out how many audioblocks are claimed to be allocated in the
	// song allocation info table

	__uint32 blocksinalloctable=audioblocks_in_alloctable();

	if ( blocksinalloctable < Convert::getint32(buffer,SONGINFO_AUDIOBLOCKS) ) 
	{
		// the song itself claims it should have more audioblocks
		return true;
	}
	// Over here we could also verify the expected number of blocks
	// against the given song length in samples.
	return false;
}

bool hd24song::is_fixable_unexpected_end()
{
	/** Checks if this song has a FIXABLE 'unexpected end of song' error */
	__uint32 blocksinalloctable=audioblocks_in_alloctable();
#if (SONGDEBUG == 1)
		cout << "Blocks in alloctable=" << blocksinalloctable << endl;
#endif
        __uint32 songblockcount=Convert::getint32(buffer,SONGINFO_AUDIOBLOCKS);
	if (songblockcount>MAX_BLOCKS_IN_SONG)
        {
		/* Safety feature: corruption detected, 
                   block count of song is greater than theoretical maximum. */
                songblockcount=MAX_BLOCKS_IN_SONG;
        }

        /* Values in songblockcount and blocksinalloctable should be equal 
           unless the song is corrupt. If the latter value lower,
           there is an 'unexpected end of song' error. */

	if (!( blocksinalloctable < songblockcount ))
	{
		// No unexpected end of song error, nothing to fix
		return false;
	}

        /* There is an unexpected end of song error. But is it one of
           the type we know how to automatically fix? */

	if (used_alloctable_entries() == (512/ALLOCINFO_ENTRYLEN) ) {
		/* Yes, it is. We have exactly 1 sector of allocated data and
		   the rest is zero data, due to a known (presumed) bug in 
                   the HD24 recorder. */
		return true;
	}
	
	/* No, it isn't. Then assume we cannot fix it. */
	return false;
}

__uint32 hd24song::used_alloctable_entries()
{
	/** Counts how many entries in the song allocation table
            are in use. */
	__uint32 MAXALLOCENTRIES=((512/ALLOCINFO_ENTRYLEN)*5)-1;

	for (__uint32 i=0;i<MAXALLOCENTRIES;i++)
	{
		__uint32 entrystartsector=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_SECTORNUM
			+(ALLOCINFO_ENTRYLEN*i));
		if (entrystartsector==0) {
			return i;
		}
	}
	return MAXALLOCENTRIES;
}

__uint32 hd24song::audioblocks_in_alloctable()
{	
	/** Finds out how many audio blocks are claimed in
	    the allocation table of the song. */
	__uint32 checkentries=used_alloctable_entries();
	if (checkentries==0) {
		return 0;
	}
	__uint32 totblocks=0;

	for (__uint32 i=0; i<checkentries; i++) 
	{
		__uint32 entrynumblocks=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_AUDIOBLOCKSINBLOCK
			+(ALLOCINFO_ENTRYLEN*i));
		totblocks+=entrynumblocks;

	        if (totblocks>MAX_BLOCKS_IN_SONG) 
        	{
	            /* Safety net: Corruption detected, song claims to use 
                       more blocks than the theoretical possible maximum. */
                    return MAX_BLOCKS_IN_SONG;
                }
	}

	return totblocks;
}

hd24song::~hd24song() 
{
#if (SONGDEBUG == 1)
	cout << "DESTRUCT hd24song " << mysongid << endl;
#endif
	if (buffer!=NULL) 
	{
		memutils::myfree("~hd24song-buffer",buffer);
		buffer=NULL;
	}
	if (scratchbook != NULL)
	{
		memutils::myfree("~hd24song-scratchbook",scratchbook);
		scratchbook=NULL;
	}
	if (audiobuffer != NULL)
	{
		memutils::myfree("~hd24song-audiobuffer",audiobuffer);
		audiobuffer=NULL;
	}
	if (blocksector != NULL) 
	{
		memutils::myfree("~hd24song-blocksector",blocksector);
		blocksector=NULL;
	}
	int i;
	
	// clear cache
	for (i=0;i<CACHEBUFFERS;i++) 
	{
		if (cachebuf_ptr[i]!=NULL) {
			memutils::myfree("cachebuf_ptr[i]",cachebuf_ptr[i] );	
		}
	}
	if (cachebuf_ptr!=NULL)
	{
		memutils::myfree("cachebuf_ptr",cachebuf_ptr);
	}
	if (cachebuf_blocknum!=NULL) 
	{
		memutils::myfree("cachebuf_blocknum",cachebuf_blocknum);
	}
}

void hd24song::queuecacheblock(__uint32 blocknum) 
{
	// Only process request if the block is neither cached nor queued yeta
	// This function is only called if a block is not cached.
	// In addition, as playback progresses, the more blocks are queued,
	// the less importance the oldest blocks have.
	// For this reason, a circular queue would make sense-- if too many blocks
	// get queued, the last one requested can be ignored.
	// The queue needn't be very big; a shortcut is to use a queue of
	// just 1 element long. This should still work OK because a block
	// queue request may be issued over and over again until it is cached.
	if (blocknum!=blocktoqueue) 
	{
		// block not yet queued
	}	
	blocktoqueue=blocknum;
	return;
}

string* hd24song::songname(hd24fs* parentfs, unsigned char* songbuf)
{
	string* ver=parentfs->version();
	if (*ver == "1.00") {
		// version 1.0 filesystem.
		delete ver;
		string* tmp=new string("");
		string* dummy=Convert::readstring(songbuf,SONGINFO_SONGNAME_8,8);
	
		*tmp+=*dummy;
		delete dummy;
		if (tmp->length()==8) {
			dummy=Convert::readstring(songbuf,SONGINFO_SONGNAME_8+10,2);
		        *tmp+=*dummy;
			delete dummy;
		}
		return tmp;
	}
	delete ver;
	string* tmp=Convert::readstring(songbuf,SONGINFO_SONGNAME,64);
	return tmp;
}

string* hd24song::songname() 
{
	return songname(this->parentfs,buffer);
}

void hd24song::songname(string newname)
{
	songname(buffer,newname);
}

void hd24song::songname(unsigned char* songbuf,string newname)
{
	hd24fs::setname(songbuf,newname,SONGINFO_SONGNAME_8,SONGINFO_SONGNAME);
	return;
}

bool hd24song::iswriteprotected() 
{
	__uint32 writeprot=(Convert::getint32(buffer,SONGINFO_WRITEPROTECTED));
	writeprot&=0x04000000;
	if (writeprot==0) return false;
	return true;
}

void hd24song::setwriteprotected(bool prot)
{
	__uint32 writeprot=(Convert::getint32(buffer,SONGINFO_WRITEPROTECTED));
	writeprot&=0xFBFFFFFF;
	
	if (prot) {
		writeprot|=0x04000000;
	}
	Convert::setint32(buffer,SONGINFO_WRITEPROTECTED,writeprot);
	return;
}

void hd24song::physical_channels(unsigned char* songbuf,__uint32 newchannelcount)
{
	if (newchannelcount>24) newchannelcount=24;
	songbuf[SONGINFO_CHANNELS]=(unsigned char)(newchannelcount&0xFF);
	Convert::setint32(songbuf,SONGINFO_SECSAMPLESWIDTH,(0x480/(newchannelcount*3)));
	Convert::setint32(songbuf,SONGINFO_SECBYTESWIDTH,(0x480/(newchannelcount)));
	Convert::setint32(songbuf,SONGINFO_SECSECWIDTH,0x200*(0x480/(newchannelcount*3)));
}

void hd24song::physical_channels(__uint32 newchannelcount)
{
	physical_channels(buffer,newchannelcount);
}

__uint32 hd24song::physical_channels(unsigned char* songbuf)
{
	int channels=Convert::getint32(songbuf,SONGINFO_CHANNELS)>>24;
	channels=(channels & 0x1f);
	if (channels>24) channels=24;
	return channels;
}

__uint32 hd24song::physical_channels() 
{
	return physical_channels(buffer);
}
__uint32 hd24song::chanmult(unsigned char* songbuf)
{
	return physical_channels(songbuf)/logical_channels(songbuf);
}
__uint32 hd24song::chanmult()
{
	return physical_channels(buffer)/logical_channels(buffer);
}
__uint32 hd24song::logical_channels() 
{
	if (this->samplerate()>=88200) 
	{
		return (physical_channels()>>1);
	} else {
		return (physical_channels());
	}
}

__uint32 hd24song::logical_channels(unsigned char* songbuf) 
{
	if (samplerate(songbuf)>=88200) 
	{
		return (physical_channels(songbuf)>>1);
	} else {
		return (physical_channels(songbuf));
	}
}

void hd24song::logical_channels(unsigned char* songbuf,__uint32 channelcount)
{
	if (samplerate(songbuf)>=88200) {
		physical_channels(songbuf,channelcount*2);
	} else {
		physical_channels(songbuf,channelcount);
        }
}

__uint32 hd24song::samplerate(unsigned char* songbuf) 
{
	__uint32 samrate=Convert::getint32(songbuf,SONGINFO_SAMPLERATE)>>8;
	return samrate;
}

__uint32 hd24song::samplerate() 
{
	return samplerate(buffer);
}

void hd24song::samplerate(unsigned char* songbuf,__uint32 newrate) 
{
	__uint32 samrate=(newrate<<8);
	__uint32 bd=((unsigned char)songbuf[SONGINFO_BITDEPTH]);
	samrate|=bd;
	Convert::setint32(songbuf,SONGINFO_SAMPLERATE,samrate);        
}

void hd24song::samplerate(__uint32 newrate) 
{
	samplerate(buffer,newrate);
}


__uint32 hd24song::bitdepth() 
{
	__uint32 depth=(__uint32)((unsigned char)buffer[SONGINFO_BITDEPTH]);
	if ((depth!=24) && (depth !=16) && (depth!=32)) return 24;
	return depth;
}
__uint32 hd24song::songlength_in_wamples()
{
	return Convert::getint32(buffer,SONGINFO_SONGLENGTH_IN_WAMPLES);
}
__uint64 hd24song::songsize_in_bytes() 
{
	// actual bytes in recording
	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	return 	bytes_per_sample
		*physical_channels()
		*(Convert::getint32(buffer,SONGINFO_SONGLENGTH_IN_WAMPLES));
}

__uint64 hd24song::bytes_allocated_on_disk() 
{
	// bytes allocated on HD24 drive
	__uint64 songlen=this->songsize_in_bytes();
	
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blockspercluster=parentfs->getblockspercluster();
	__uint32 blocksize_in_bytes=blockspercluster*blocksize_in_sectors*SECTORSIZE;
	__uint32 s=songlen % blocksize_in_bytes;	
	if (s!=0) {
		songlen-=s;
		songlen+=blocksize_in_bytes;
	}
	return songlen;
}

__uint32 hd24song::songlength_in_wamples(__uint32 newlen,bool silencenew)
{
	return  hd24song::songlength_in_wamples(newlen,silencenew,NULL,NULL);
}
__uint32 hd24song::songlength_in_wamples(__uint32 newlen,bool silencenew,char* savemessage,int* cancel)
{
	return hd24song::songlength_in_wamples(newlen,silencenew,savemessage,cancel,NULL);
}

hd24fs* hd24song::fs()
{
	return this->parentfs;
}

__uint32 hd24song::songlength_in_wamples(__uint32 newlen,bool silencenew,char* savemessage,int* cancel,int (*checkfunc)())
{
	// Sets the length of a song and updates any allocation
	//   info as needed.
        //   The return value of the function is the actual song length
	//   set. Return value may differ from newlen if not enough drive
	//   space was available or if allocating ran into problems
	//   otherwise. 
	if (this==NULL)
	{
#if (SONGDEBUG==1)
	cout << "Song object is NULL! Cannot lengthen song." << endl;
#endif
		return 0;
	}
	__uint32 oldlen=songlength_in_wamples();
	if (savemessage!=NULL)
	{
		// clear default save message
		savemessage[0]='\0';
	}
	if (cancel!=NULL)
	{
		*cancel=0;
	}
#if (SONGDEBUG==1)
	cout << "Lengthening song to " << newlen << " wamples" << endl;
	if (silencenew) {
#if (SONGDEBUG==1)
		cout << "(And silencing new blocks)" << endl;
#endif
	}
#endif
        Convert::setint32(buffer,SONGINFO_SONGLENGTH_IN_WAMPLES,newlen);
	if (newlen==0) {
		Convert::setint32(buffer,SONGINFO_AUDIOBLOCKS,0);
		return 0;
	}

	// the above is required by setallocinfo
	if (setallocinfo(silencenew,savemessage,cancel,checkfunc)) {
		// setting alloc info succeeded
#if (SONGDEBUG==1)
	cout << "Success lengthening song to " << newlen << " samples" << endl;
#endif
		this->lengthened=true;
                memoizeblocksectors(Convert::getint32(buffer,SONGINFO_AUDIOBLOCKS));
		return newlen;
	}
	// setting new length failed- reset song to old length.
#if (SONGDEBUG==1)
	cout << "Failed. Keep at old length of " <<oldlen << endl;
#endif
        Convert::setint32(buffer,SONGINFO_SONGLENGTH_IN_WAMPLES,oldlen);
	return oldlen;
}
__uint32 hd24song::songlength_in_wamples(__uint32 newlen)
{
	return songlength_in_wamples(newlen,true);
}

string* hd24song::display_cursor() 
{
	return (display_duration(songcursor));	
}
__uint32 hd24song::cursorpos() 
{
	return songcursor;
}
bool hd24song::endofsong()
{
	if (songcursor>=songlength_in_wamples()) return true;
	return false;
}
string* hd24song::display_duration(__uint32 offset,__uint32 samrate) 
{
	if (samrate==0) 
	{
		string* nulldur=Convert::int2str(0,2,"0");
		*nulldur+=":00:00.00";
		return nulldur;	
	}
	samrate/=chanmult();
	__uint32 subsec=display_subseconds(offset,samrate);
	if (samrate>0) {
		subsec=this->framespersec*subsec/samrate;
	}
	string* newstr=	Convert::int2str(display_hours(offset),2,"0");
	*newstr+=":";
	string* mins=Convert::int2str(display_minutes(offset),2,"0");
	*newstr+=*mins;
	delete mins;
        *newstr+=":";
	string* secs=Convert::int2str(display_seconds(offset),2,"0");
	*newstr+=*secs;
	delete secs;
        *newstr+=".";
	string* subsecs=Convert::int2str(subsec,2,"0");
	*newstr+=*subsecs;
	delete subsecs;
	return newstr;
}

string* hd24song::display_duration(__uint32 offset) 
{
	return display_duration(offset,samplerate());
}

string* hd24song::display_duration()
{
	return display_duration(songlength_in_wamples());
}

__uint32 hd24song::display_hours() 
{
	return display_hours(songlength_in_wamples());
}

__uint32 hd24song::display_hours(__uint32 offset,__uint32 samrate) 
{
	if (samrate==0) 
	{	
		return 0;	
	}
	if (samrate>=88200) { samrate=samrate>>1; }
	
        __uint32 totsonglen=offset;
	__uint32 songsubsecs=totsonglen%samrate;
	__uint32 cutsonglen=(totsonglen-songsubsecs);
	__uint32 totsongsecs=(cutsonglen/samrate);
	__uint32 viewsongsecs=totsongsecs%60;
	__uint32 totsongmins=(totsongsecs-viewsongsecs)/60;
	__uint32 viewsongmins=(totsongmins%60);
	__uint32 totsonghours=(totsongmins-viewsongmins)/60;
	return totsonghours;
}

__uint32 hd24song::display_minutes() 
{
	return display_minutes(songlength_in_wamples());
}

__uint32 hd24song::display_minutes(__uint32 offset,__uint32 samrate) 
{
	if (samrate==0) 
	{	
		return 0;	
	}
	if (samrate>=88200) { samrate=samrate>>1; }
        __uint32 totsonglen=offset;
	__uint32 songsubsecs=totsonglen%samrate;
	__uint32 cutsonglen=(totsonglen-songsubsecs);
	__uint32 totsongsecs=(cutsonglen/samrate);
	__uint32 viewsongsecs=totsongsecs%60;
	__uint32 totsongmins=(totsongsecs-viewsongsecs)/60;
	__uint32 viewsongmins=(totsongmins%60);
	return viewsongmins;
}

__uint32 hd24song::display_seconds() 
{
	return display_seconds(songlength_in_wamples());
}

void hd24song::sectorinit(unsigned char* songsector)
{	
	unsigned char emptysong[512] = {
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X10,0X00,0X00,0X00,0X30,0X00,0X00,0X00,
		0X00,0X20,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X67,0X6E,0X6F,0X53,0X6D,0X61,0X4E,0X20,
		0X20,0X65,0X18,0X00,0X18,0X44,0XAC,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X01,0X02,0X00,0X00,0XE4,0X12,0X04,0X30,0XA8,0X10,0X00,0X20,
		0X00,0X00,0X00,0X20,0X01,0X00,0X13,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X41,0X54,0X53,0X20,
		0X20,0X20,0X54,0X52,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X31,0X30,0X6D,0X61,
		0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X32,0X30,0X6D,0X61,0X00,0X00,0X00,0X00,
		0X4E,0X63,0X6F,0X4C,0X33,0X30,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,
		0X34,0X30,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X35,0X30,0X6D,0X61,
		0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X36,0X30,0X6D,0X61,0X00,0X00,0X00,0X00,
		0X4E,0X63,0X6F,0X4C,0X37,0X30,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,
		0X38,0X30,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X39,0X30,0X6D,0X61,
		0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X30,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,
		0X4E,0X63,0X6F,0X4C,0X31,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,
		0X32,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X33,0X31,0X6D,0X61,
		0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X34,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,
		0X4E,0X63,0X6F,0X4C,0X35,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,
		0X36,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X37,0X31,0X6D,0X61,
		0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,0X38,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,
		0X4E,0X63,0X6F,0X4C,0X39,0X31,0X6D,0X61,0X00,0X00,0X00,0X00,0X4E,0X63,0X6F,0X4C,
		0X30,0X32,0X6D,0X61,0X00,0X00,0X00,0X00,0X63,0X6E,0X75,0X50,0X20,0X6E,0X49,0X68,
		0X00,0X00,0X00,0X00,0X63,0X6E,0X75,0X50,0X74,0X75,0X4F,0X68,0X00,0X00,0X00,0X00,
		0X74,0X69,0X64,0X45,0X20,0X6E,0X49,0X20,0X00,0X00,0X00,0X00,0X74,0X69,0X64,0X45,
		0X74,0X75,0X4F,0X20,0X04,0X04,0X00,0X00,0X78,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
		0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00
	};
	for (int i=0;i<1024;i++) {
		songsector[i]=0; // wipe clean entire buffer
	}
	for (int i=0;i<512;i++) {
		songsector[i]=emptysong[i]; // init with empty song info
	}
	hd24fs::fstfix(songsector,1024); // from native drive format to normal byte ordering
	songname(songsector,"Song Name");
}

__uint32 hd24song::display_seconds(__uint32 offset,__uint32 samrate)
{
	if (samrate==0) 
	{	
		return 0;	
	}
	if (samrate>=88200) { samrate=samrate>>1; }
	__uint32 cutsonglen=offset-display_subseconds(offset,samrate);
	__uint32 totsongsecs=(cutsonglen/samrate);
	__uint32 viewsongsecs=totsongsecs%60;
	return viewsongsecs;
}

__uint32 hd24song::display_subseconds() {
	return display_subseconds(songlength_in_wamples());
}

__uint32 hd24song::display_subseconds(__uint32 offset,__uint32 samrate) 
{
	if (samrate==0) 
	{	
		return 0;	
	}
	if (samrate>=88200) { samrate=samrate>>1; }
        __uint32 totsonglen=offset;
	__uint32 songsubsecs=totsonglen%samrate;
	return songsubsecs;
}

__uint32 hd24song::display_hours(__uint32 offset) 
{
	return display_hours(offset,samplerate());
}

__uint32 hd24song::display_minutes(__uint32 offset) 
{
	return display_minutes(offset,samplerate());
}

__uint32 hd24song::display_seconds(__uint32 offset) 
{
	return display_seconds(offset,samplerate());
}

__uint32 hd24song::display_subseconds(__uint32 offset) 
{
	return display_subseconds(offset,samplerate());
}

unsigned char* hd24song::getcachedbuffer(__uint32 blocknum) 
{
	// This will return a pointer to an audio buffer containing
	// the audio of the given blocknum, if available.
	// If not available, it will return a pointer to a silent
	// block and queue the blocknum for caching
	int i;
	bool foundbuf=false;
	unsigned char* bufptr=NULL;

	/* A straight loop isn't the fastest way to find the
	 * correct buffer (a binary tree or hash would perform
	 * better). However the advantage for a total of around
	 * 40 blocks (25 locate points and some lookahead) 
	 * would be rather marginal. */

	bool havenext=false;
	bool haveprev=false;

	for (i=LOCATEPOS_LAST;i<CACHEBUFFERS;i++) 
	{
		if (blocknum>0) {
			if (cachebuf_blocknum[i]==(blocknum-1)) {
				haveprev=true;
				if (havenext && foundbuf) break;
			}
		}
		if (cachebuf_blocknum[i]==(blocknum+1)) {
			havenext=true;
			if (haveprev && foundbuf) break;
		}
		if (cachebuf_blocknum[i]==blocknum) 
		{
			bufptr=cachebuf_ptr[i];
			foundbuf=true;
			if (havenext && haveprev) break;
		}
	}
	if (!(foundbuf)) 
	{
		if (!haveprev) 
		{
			queuecacheblock(blocknum-1);
		}
		if (!havenext) 
		{
			queuecacheblock(blocknum+1);
		}
		queuecacheblock(blocknum);
		return NULL;
	}
	if (!haveprev) 
	{
		queuecacheblock(blocknum-1);
	}
	if (!havenext) 
	{
		queuecacheblock(blocknum+1);
	}
	// Cache buffer was found.
	// If we want we can optimize the cache here.
	// Otherwise, just return the buffer pointer.
	lastavailablecacheblock=blocknum;
	return bufptr;
}

void hd24song::memoizeblocksectors(__uint32 number_of_blocks) 
{
	__uint32 totblocksfound=0;
	__uint32 myallocentrynum=0;

	__uint32 entrystartsector=0;
	__uint32 entrynumblocks=0;

	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();

	while (
		(totblocksfound < number_of_blocks)
		&& (totblocksfound <= MAX_BLOCKS_IN_SONG)
		&& (myallocentrynum<ALLOC_ENTRIES_PER_SONG)
	)
	{

		entrystartsector=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_SECTORNUM
			+(ALLOCINFO_ENTRYLEN*myallocentrynum));
		entrynumblocks=Convert::getint32(buffer,
			SONGINFO_ALLOCATIONLIST+ALLOCINFO_AUDIOBLOCKSINBLOCK
			+(ALLOCINFO_ENTRYLEN*myallocentrynum));
#if (SONGDEBUG == 1)
		cout << "Entry " << myallocentrynum << " start sector=" << entrystartsector 
		<< "# blocks in entry=" << entrynumblocks << endl;
#endif

		for (__uint32 filler=0;filler<entrynumblocks;filler++) {
			if (totblocksfound+filler > MAX_BLOCKS_IN_SONG) break;
			blocksector[totblocksfound+filler]=entrystartsector+(blocksize_in_sectors*filler);
		}
		totblocksfound+=entrynumblocks;
		myallocentrynum++;
	}
        lastallocentrynum=myallocentrynum;
#if (SONGDEBUG == 1)
	cout << "Tot blocks found = " << totblocksfound << "/" << number_of_blocks << endl;
#endif
	return	;
}
	
/*	   Quick calculation: 
	   Saving 1 block sectornum=32 bit (4 bytes).
           MAX_BLOCKS_IN_SONG=524288, so the maximum number of bytes needed to 
           memoize all song allocation info=524288*4=2097152 bytes (~2 megabyte) 
           for the worst case song, which is certainly doable.

	   As memoization can be done efficiently when carried out sequentially, it can be done in O(n)

	   When this function is called once with last blocknum, all blocks can be memoized during a
	   single pass of the WHILE loop
           Lookup will be O(1). 
	   A typical song transfer will take X tracks 
           (each track requires a sector calc for all blocks).
*/

void hd24song::getmultitracksample(long* mtsample,int readmode)
{
	/* This procedure is intended for copying audio from disk (and for realtime
           playback). This procedure assumes sequential reading.

	   If reverse playback is desired, golocatepos() must be called for 
           every sample. This is a bit more expensive in resources.
	   However, as golocatepos() 
	   doesn't cause any I/O, it should still be light enough for regular use.

	   As such, allocation info for every sample will only be recalculated 
	   when needed. This results in the best possible performance.

	   There are two playback modes: copy and realtime. Copy mode guarantees
           that a bit-accurate copy of the disk contents is returned, but may
           require (slow) disk reads in the process, which makes it unsuitable
           for anything requiring realtime response.

           Realtime mode guarantees to return a result in a short amount of time, 
           by using a cache. This makes it suitable for realtime playback.
           When a block is not available in cache, silence is returned. This makes
           realtime mode unsuitable for accurate transfers, but suitable for direct
           from-disk mixing. Blocks that are not available in cache are queued for
           caching. Periodic background checks should be performed on this queue to
           help guarantee availability of the blocks to cache. 

	   In high samplerate mode, samples are interlaced between odd tracks and
	   even tracks. This allows the song cursor to keep running at normal speed-
	   the difference is that at double the speed the songcursor is only
 	   updated every other multitrack sample request. This means that at the
	   cost of only being able to do locate operations to even samples,
	   we can maintain use the same code for block calculation.
	*/
#if (SONGDEBUG==1)
	cout << "Getmultitracksample mtsample=" << mtsample <<" mode=" <<readmode<< "this=" <<this << endl
	<< "parentfs=" << parentfs << endl;
#endif
	unsigned char* buffertouse=NULL;
	currentreadmode=readmode;	
	__uint32 samrate=samplerate();
	__uint32 samplenumber=songcursor;	
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;
	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 tracks_per_song=physical_channels();
	__uint32 tracksamples_per_block=(blocksize_in_bytes / bytes_per_sample) / tracks_per_song;
	__uint32 blocknum=(samplenumber/(tracksamples_per_block));
#if (SONGDEBUG==1)
	cout << "tracksamples per block="<<tracksamples_per_block << endl
	<< "readmtsample MARK" << endl;
#endif
	bool mustgetaudiodata=false;
	if (parentfs->maintenancemode==1) {
		readmode=hd24song::READMODE_COPY;
	}
	switch (readmode) 
	{
		case hd24song::READMODE_COPY:
			if ((lastreadblock!=blocknum)||(mustreadblock==1)) 
			{
				mustgetaudiodata=true;
			}
			break;
		case hd24song::READMODE_REALTIME:
			mustgetaudiodata=false;
			if ((lastavailablecacheblock!=blocknum)||(mustreadblock==1)) {
				mustgetaudiodata=true;
			}
			break;
		default: 
			mustgetaudiodata=false;
			break;
	}
	
#if (SONGDEBUG==1)
	cout << "readmtsample MARK2" << endl;
#endif
	if (mustgetaudiodata)
	{
		// We advanced a block. This means we need to read more audio data.
		// (or in case of realtime reading, at least find out what next block to get)
		if (blocknum==(allocstartblock+allocaudioblocks)) 
		{
			// In fact, we've read all data in the current allocation entry.
			allocentrynum++;			// reset cursor to start of song
			allocstartblock=blocknum; 	// blocknum of first block in current allocation entry
			allocstartsector=Convert::getint32(buffer,
				SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*allocentrynum)+ALLOCINFO_SECTORNUM);
			allocaudioblocks=Convert::getint32(buffer,
				SONGINFO_ALLOCATIONLIST+(ALLOCINFO_ENTRYLEN*allocentrynum)+ALLOCINFO_AUDIOBLOCKSINBLOCK);
		}
		
		switch (readmode) 
		{
			case (hd24song::READMODE_COPY):
				if (parentfs->maintenancemode==1) 
				{
                                        // in maintenance mode, we will display the sector currently
                                        // being played back (that is what maintenance mode is all
                                        // about
					__uint32 currsector=allocstartsector+((blocknum-allocstartblock)*blocksize_in_sectors);
					string* hexsector=Convert::int32tohex(currsector);
					string* cluster=Convert::int32tostr(parentfs->sector2cluster(currsector));

					cout 	<< "sector "  // maintenance mode, PRAGMA allowed
					     	<< *hexsector 
						<< " (cluster " 
						<< *cluster 
						<< ")-1" << endl; // maintenance mode, PRAGMA allowed
					delete hexsector;
				}
				
				parentfs->readsectors(parentfs->devhd24,
				allocstartsector+((blocknum-allocstartblock)*blocksize_in_sectors),
				audiobuffer,blocksize_in_sectors); // raw audio read, no fstfix needed
				mustreadblock=0;
				break;
			case (hd24song::READMODE_REALTIME):
				buffertouse=getcachedbuffer(blocknum);
					
			default: break;
		}
	}
#if (SONGDEBUG==1)
	cout << "readmtsample MARK 3" << endl
	 << "audiobuffer=" << audiobuffer << endl
	 << "readmtsample MARK 3b" << endl;
#endif

	int sample_within_block=samplenumber%(tracksamples_per_block);
	if (readmode==hd24song::READMODE_COPY) 
	{
		buffertouse=audiobuffer;
	}
	__uint32 trackspersam;
	if (samrate>=88200) {
		trackspersam=2; 
	} else {
		trackspersam=1;
	}
#if (SONGDEBUG==1)
	cout << "readmtsample MARK 3c" << endl;
#endif
	/* Audio buffer has been read, now copy multi track sample
           to multi track sample buffer. In high sample rate mode,
           either even or odd samples is returned (alternating
	   each call) 
        */
	__uint32 tottracks=logical_channels();
	for (__uint32 tracknum=0;tracknum<tottracks;tracknum++) 
	{
		__uint32 samval;
		if (buffertouse==NULL) 
		{
			samval=0;
		} 
		else 
		{
			int offset_first_blocksample=(((tracknum*trackspersam)+evenodd)*tracksamples_per_block*bytes_per_sample);
			int sample_offset=offset_first_blocksample+(sample_within_block*bytes_per_sample);
			samval=Convert::getint24(buffertouse,sample_offset);
			// TODO: Handle word lengths other than 24 bits
		}
#if (SONGDEBUG==1)
	cout << "readmtsample MARK 3e" << endl
	 << "tracknum=" << tracknum << endl;
#endif
		mtsample[tracknum]=samval;
#if ( SONGDEBUG == 1 )
	cout << "posttracknum=" << tracknum << endl;
		if ((tracknum==0) && (songcursor<20)) {
			string* bla=Convert::int32tohex(samval);
#if ( SONGDEBUG == 1 )
			cout << *bla << "-2"<< endl; 
#endif
			delete bla;
		}
#if ( SONGDEBUG == 1 )
	cout << "readmtsample MARK 3f" << endl;
#endif
#endif
	}
#if (SONGDEBUG==1)
	cout << "readmtsample MARK 4" << endl;
#endif
	lastreadblock=blocknum;
	if (samrate>=88200) 
	{
		// For high sample rate mode the song cursor advances 
		// only every other sample.
		// Variable evenodd keeps track of what to return.
		evenodd=1-evenodd;
		if (evenodd==0) {
			songcursor++;	
		}
	} else {
		songcursor++;
	}

	return;
}

int hd24song::getmtrackaudiodata(__uint32 firstsamnum,__uint32 samples,unsigned char* buffer,int readmode)
{
	/* WARNING: For best performance the number of samples must not cross
           audio block boundaries. This function has been tested in such a fashion only.

           This procedure is intended for reading audio data from disk,
	   in a completely random access fashion.
	   It assumes single track audio and will always read only whole blocks,
           directly to the given buffer. Return value is a pointer to the
           first sample that was supposed to be read.
 
	   The buffer should be sufficiently large to hold the total audio size.
  	   (number of samples*3 bytes for normal sample rates or
            number of samples*3*2 bytes for high sample rates (88k2, 96k).

	   In copy mode, only required blocks will be read from disk
           (no caching will take place- we'll leave this to the OS)

           Realtime mode guarantees to return a result in a short amount of time, 
           by using a cache. This makes it suitable for realtime playback.
           REALTIME MODE IS NOT IMPLEMENTED YET.
           When a block is not available in cache, silence is returned. This makes
           realtime mode unsuitable for accurate transfers, but suitable for direct
           from-disk mixing. Blocks that are not available in cache are queued for
           caching. Periodic background checks should be performed on this queue to
           help guarantee availability of the blocks to cache. 
        */

	currentreadmode=readmode;	
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;

	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 tracks_per_song=logical_channels();
	__uint32 tracksamples_per_block=(blocksize_in_bytes / bytes_per_sample) / tracks_per_song;

	__uint32 startblocknum=((firstsamnum-(firstsamnum%tracksamples_per_block))/(tracksamples_per_block));
	__uint32 lastsamnum=firstsamnum+samples-1;
	__uint32 endblocknum=((lastsamnum-(lastsamnum%tracksamples_per_block))/(tracksamples_per_block));

	// check read enable flags to allow reducing number of sectors to be transferred.
	__uint32 first_readenabled=0;
	__uint32 last_readenabled=23;
	for (__uint32 i=0;i<logical_channels();i++)
	{
		if (track_readenabled[i])
		{
			first_readenabled=i;
			break;
		}
	}
	for (__uint32 i=logical_channels();i>0;i--)
	{
		if (track_readenabled[i-1])
		{
			last_readenabled=i-1;
			break;
		}
	}
#if (SONGDEBUG==1)
	cout << "first,last track="<<first_readenabled<<","<<last_readenabled<<endl;
#endif
	__uint32 chanmult=physical_channels()/logical_channels();

	__uint32 physicaltracksreadenabled=chanmult*(last_readenabled-first_readenabled)+1;
	__uint32 firsttrackoffset=chanmult*first_readenabled*tracksamples_per_block*bytes_per_sample;
	__uint32 sectoroffset=firsttrackoffset/SECTORSIZE;
	__uint32 readlength=(physicaltracksreadenabled*bytes_per_sample*tracksamples_per_block)/SECTORSIZE;
#if (SONGDEBUG==1)
	cout << "sectoroffset,readlength="<<sectoroffset<<","<<readlength<< endl;
#endif
	for (__uint32 blocknum=startblocknum;blocknum<=endblocknum;blocknum++) 
	{
#if (SONGDEBUG == 1)
			string* bla=Convert::int32tohex(blocksector[blocknum]);
#if (SONGDEBUG == 1)
			cout << *bla << "-3" << endl; // maintenance mode
#endif
			delete bla;
#endif
		// now read trackblocksize_in_sectors sectors from sector blocksec into buffer
		parentfs->readsectors(parentfs->devhd24,
			blocksector[blocknum]+sectoroffset,
			&buffer[firsttrackoffset],
			readlength); // raw audio read, no fstfix needed
	}
	return firstsamnum%tracksamples_per_block;
}

void hd24song::interlaceblock(unsigned char* sourcebuffer,unsigned char* targetbuffer) 
{
	/* This is needed for high sample rates as high sample rate recordings
           take up two physical channels for each logical audio channel */
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;
	__uint32 blocksize_doubleblock=blocksize_in_bytes/logical_channels();
	__uint32 blocksize_halfblock=blocksize_in_bytes/physical_channels();

	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 tracksamples_per_halfblock=(blocksize_halfblock/bytes_per_sample);
	__uint32 choffset=0;
	for (__uint32 ch=0;ch<logical_channels();ch++) 
	{	
		for (__uint32 i=0;i<tracksamples_per_halfblock;i++) 
		{
			__uint32 samoff_target=i*bytes_per_sample+choffset;
			__uint32 samoff_source=2*i*bytes_per_sample+choffset;
			for (__uint32 j=0;j<bytes_per_sample;j++) {	
				targetbuffer[samoff_target+j]
					=sourcebuffer[samoff_source+j];
				targetbuffer[samoff_target+j+bytes_per_sample]
					=sourcebuffer[samoff_source+j+blocksize_halfblock];
			}
		}
		choffset+=blocksize_doubleblock;
	}
}

void hd24song::deinterlaceblock(unsigned char* sourcebuffer,unsigned char* targetbuffer) 
{
	/* This is needed for high sample rates as high sample rate recordings
           take up two physical channels for each logical audio channel */	
	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;
	__uint32 blocksize_doubleblock=blocksize_in_bytes/logical_channels();
	__uint32 blocksize_halfblock=blocksize_in_bytes/physical_channels();

	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 tracksamples_per_halfblock=(blocksize_halfblock/bytes_per_sample);
	__uint32 choffset=0;
	for (__uint32 ch=0;ch<logical_channels();ch++) 
	{	
		for (__uint32 i=0;i<tracksamples_per_halfblock;i++) 
		{
			__uint32 samoff_source=i*bytes_per_sample+choffset;
			__uint32 samoff_target=2*i*bytes_per_sample+choffset;
			for (__uint32 j=0;j<bytes_per_sample;j++) {	
				targetbuffer[samoff_target+j]
					=sourcebuffer[samoff_source+j];
				targetbuffer[samoff_target+j+bytes_per_sample]
					=sourcebuffer[samoff_source+j+blocksize_halfblock];
			}
		}
		choffset+=blocksize_doubleblock;
	}
}

int hd24song::putmtrackaudiodata(__uint32 firstwamnum,__uint32 wamples,unsigned char* writebuffer,int writemode)
{
#if (HD24TRANSFERDEBUG==1)
	cout << "putmtrackaudiodata("
	<<firstwamnum
	<<","<<wamples<< ", writebuffer,writemode)" << endl
	<< " first 30 bytes of write buffer: " ;
	for (int i=0;i<30;i++) { cout << " " << (short)((unsigned char)writebuffer[i]); } // PRAGMA allowed
	cout << endl; // PRAGMA allowed
#endif
	/* 
           This procedure is intended for writing audio data to disk.
           Contrary to reading audio (where realtime mode is OK to drop
           some audio during heavy seeking), write mode should always
           write reliably- caching is not allowed.
           As such only sequential operation is allowed.

           NOTE: THIS FUNCTION WAS NOT TESTED FOR REALTIME OPERATION.
           
           Before writing, you need to arm the tracks that you want to
           write to (using the unarmtrack and armtrack functions),
           then enable record mode (function startrecord). 

           Startrecord will disable seeking while recording and perform
           any tasks needed to initialize drive usage administration.

           When no tracks are armed or record mode is not enabled, 
           nothing will be written to disk. (A rehearse mode may be
           added at some point to prevent writing to disk even in
 	   record mode).

           After writing, you need to call stoprecord. This will
           re-enable seek operations and write out any drive usage
           information, increase file length etc, should any space have 
    	   been allocated during the write operation.

           Before calling this function, the write buffer needs to contain
           the audio to record in the tracks that are armed.

           After calling this function, the write buffer contents will be
           altered: the non-armed tracks will contain the audio that
           was already on disk.

           The write buffer should be sufficiently large to hold the total
           audio size for all tracks.

	   With regards to interlacing: This should already be done by the
           caller. Reason is that filling the buffer is an already heavy
           copy operation. Already interlacing during the buffer fill is
           much lighter than doing another copy operation just for the
           interlacing.
        */

	currentreadmode=writemode;

	__uint32 blocksize_in_sectors=parentfs->getblocksizeinsectors();
	__uint32 blocksize_in_bytes=blocksize_in_sectors*SECTORSIZE;

	__uint32 bits=(this->bitdepth());
	__uint32 bytes_per_sample=bits/8;
	__uint32 tracks_per_song=logical_channels();
	__uint32 trackbytes_per_block=(blocksize_in_bytes / logical_channels());
	__uint32 trackwamples_per_block=trackbytes_per_block / (bytes_per_sample*chanmult());
	
	// track bytes is correct as it deals with logical tracks.
	// tracksamples per block ought to be duplicated for high samplerates though.

	__uint32 startblocknum=((firstwamnum-(firstwamnum%trackwamples_per_block))/(trackwamples_per_block));
	__uint32 lastwamnum=firstwamnum+wamples-1;
	__uint32 endblocknum=((lastwamnum-(lastwamnum%trackwamples_per_block))/(trackwamples_per_block));
#if (HD24TRANSFERDEBUG==1)
          cout << "tracksams_per_block=" << trackwamples_per_block
	<<"startblocknum="<<startblocknum
		<<", endblocknum="<<endblocknum
		<<endl; 
#endif
	for (__uint32 blocknum=startblocknum;blocknum<=endblocknum;blocknum++) 
	{
#if (HD24TRANSFERDEBUG==1)
		cout << "blocknum="<<blocknum<<endl;
#endif
		// now read trackblocksize_in_sectors sectors from sector blocksec into buffer

		if (blocksector[blocknum]<0x1397f6) {
			// safety feature- drop out of write mode when superblock is targeted.
#if (HD24TRANSFERDEBUG==1)
			cout << "Detected audio write request to administration area. " << endl << "Possible bug, dropping out of write mode. " << endl;
#endif
			setrehearsemode(true);
		}
		
		parentfs->readsectors(parentfs->devhd24,
			blocksector[blocknum],
			scratchbook,
			blocksize_in_sectors); // raw audio read, no fstfix needed

		// now overwrite only the armed tracks with contents of buffer
		__uint32 armedtrackcount=0;
		if (!(this->isrehearsemode())) {

			for (__uint32 tracknum=1; tracknum<=tracks_per_song; tracknum++) {
				if (!(this->trackarmed(tracknum))) {
					continue;
				}
				armedtrackcount++;
				__uint32 firsttrackbyte=(tracknum-1)*trackbytes_per_block;
				for (__uint32 q=0; q<trackbytes_per_block;q++) {
					if (q<10) {
#if (SONGDEBUG==1)
//nn					cout << "scratchbook[" << firsttrackbyte+q <<"]=writebuffer[dito]=" << (int)((unsigned char)writebuffer[firsttrackbyte+q]) << endl;
#endif
					}
					scratchbook[firsttrackbyte+q]=(unsigned char)writebuffer[firsttrackbyte+q];
				}
			}
			if (armedtrackcount>0) {		
#if (HD24TRANSFERDEBUG==1)
				cout << "writing back " <<  armedtrackcount
				 << " armed tracks to sector "<< blocksector[blocknum] << endl;
#endif
				parentfs->writesectors(parentfs->devhd24,
					blocksector[blocknum],
					scratchbook,
					blocksize_in_sectors);
			}
			else 
			{
#if (HD24TRANSFERDEBUG==1)
				cout << "no armed tracks, not writing." << endl;
#endif
			}
		}
	}
	return firstwamnum%trackwamples_per_block;
}

void hd24song::unarmalltracks()
{
	for (unsigned int chnum=1;chnum<=this->logical_channels();chnum++)
	{
		this->trackarmed(chnum,false);
	}
}

void hd24song::armalltracks()
{
	for (unsigned int chnum=1;chnum<=this->logical_channels();chnum++)
	{
		this->trackarmed(chnum,true);
	}
}

void hd24song::startrecord(int recordmode)
{
	// TODO: recordmode to distinguish between realtime and copy mode
	recordmode=recordmode;
	this->busyrecording=true;
}

void hd24song::stoprecord()
{
	this->busyrecording=false;
}

bool hd24song::recording()
{
	return (this->busyrecording);
}


void hd24song::readenabletrack(__uint32 tracknum,bool enable) 
{
	if (tracknum<1) return;
	if (tracknum>24) return;
	if (tracknum>logical_channels()) return;
	track_readenabled[tracknum-1]=enable;
}

void hd24song::readenabletrack(__uint32 tracknum)
{
	readenabletrack(tracknum,true);
}

bool hd24song::isrehearsemode()
{
	return this->rehearsemode;
}

void hd24song::setrehearsemode(bool p_rehearsemode)
{
	this->rehearsemode=p_rehearsemode;
	return;
}

void hd24song::trackarmed(__uint32 tracknum,bool arm) 
{
	if (tracknum<1) return;
	if (tracknum>24) return;
	if (tracknum>logical_channels()) return;
	track_armed[tracknum-1]=arm;
	return;
}

bool hd24song::trackarmed(__uint32 tracknum) 
{
	if (tracknum<1) return false;
	if (tracknum>24) return false;
	return track_armed[tracknum-1];
}

bool hd24song::istrackmonitoringinput(__uint32 tracknum)
{
	// TODO: PROPERLY SET TRANSPORT STATUS! (for now done by GUI)

	if (tracknum<1) return false;
	if (tracknum>(this->logical_channels())) return false;

	// indicates if a given track is (supposed to be)
	// monitoring input (if false, playback is being monitored). 
	// This is based on the following decision matrix:
        //
	// All input | auto input | Track rec-enabled | Transport status | result
        // ----------+------------+-------------------+------------------+--------
        //  on       |            | don't care        | don't care       | true
        //  off      | off        | false             | stop             | false 
        //  off      | off        | false             | play             | false
        //  off      | off        | false             | rec              | false
        //  off      | off        | true              | stop             | true
        //  off      | off        | true              | play             | true
        //  off      | off        | true              | rec              | true
        //  off      | on         | false             | stop             | false
        //  off      | on         | false             | play             | false
        //  off      | on         | false             | rec              | false
        //  off      | on         | true              | stop             | true
        //  off      | on         | true              | play             | false
        //  off      | on         | true              | rec              | true
        // ----------+------------+-------------------+------------------+--------
	if (parentfs->isallinput()) {
		return true;
	}
	if (!(this->trackarmed(tracknum))) return false;

	if (this->parentfs->transportstatus==hd24fs::TRANSPORTSTATUS_PLAY) {
		if (this->parentfs->isautoinput()) {	
			return false;
		}
	}
	return true;
}

__uint32 hd24song::getnextfreesector(__uint32 lastallocsector)
{
	/* Based on the alloc info of the current song, this function
	   will return the sector number of the next unallocated cluster.
	   When no unallocated sectors are found, the function will return 0.

           Sector 0 is never in the data area, so this will allow us to 
	   distinguish between this situation and real cluster numbers.
	   Sector 0 is the superblock- as allocation implies writing to the
           drive, the code calling this function MUST verify the result. 
	  
           When the allocation info cannot be decided upon based on just
           the unallocated song sectors within the last allocated cluster
           for the song, this function will ask the file system for the
           sector number of the next unused cluster.
           
	*/
#if (SONGDEBUG==1)
	cout << "Song::getnextfreesec(" << lastallocsector << ")" << endl;
#endif
	// lastallocentrynum=last used allocation entry
        __uint32 allocsector=Convert::getint32(buffer,SONGINFO_ALLOCATIONLIST
		+(ALLOCINFO_ENTRYLEN*lastallocentrynum)+ALLOCINFO_SECTORNUM);
        __uint32 allocblocks=Convert::getint32(buffer,SONGINFO_ALLOCATIONLIST
		+(ALLOCINFO_ENTRYLEN*lastallocentrynum)+ALLOCINFO_AUDIOBLOCKSINBLOCK);
	__uint32 nextsec=0;
	
	if ((allocsector==0) && (lastallocsector==0))
	{
		// no sectors allocated yet within song.
		nextsec=this->parentfs->getnextfreesector(CLUSTER_UNDEFINED);
	} else {	
		// find out first cluster used by allocation unit
		__uint32 alloccluster;
		__uint32 blockspercluster;
		__uint32 clustersused;
		__uint32 lastalloccluster;
		if (allocsector==0) {
			lastalloccluster=parentfs->sector2cluster(lastallocsector);
		} else {
			alloccluster=parentfs->sector2cluster(allocsector);
			blockspercluster=parentfs->getblockspercluster();
			clustersused=allocblocks/blockspercluster;
			lastalloccluster=alloccluster+(clustersused-1);
		}

		// check if allocation entry fills up the current cluster word
		// if not, allocate another cluster within current alloc entry
		// otherwise, ask the FS for drive space
		// (update song alloc info)

		nextsec=this->parentfs->getnextfreesector(lastalloccluster);
	}

	if (nextsec==0) 
	{
	   /* 
		TODO: safety feature: If getnextfreesector returns 0, record
          	mode will be disabled to prevent accidentally overwriting the 
           	superblock. (Alternatively transport may be stopped but 
           	auto-stop hasn't been fully designed yet). */
		// write protect of some sort
		setrehearsemode(true);
	} 
	return nextsec;
}

void hd24song::save()
{
	__uint32 songsector=parentproject->getsongsectornum(this->mysongid);
#if (SONGDEBUG == 1)
	cout << "writing buffer to sector " << songsector << ", " <<TOTAL_SECTORS_PER_SONG<<" sectors" << endl;
#endif

	parentfs->fstfix(buffer,TOTAL_SECTORS_PER_SONG*512); // sector is now once again in native format

	parentfs->setsectorchecksum(buffer,0,songsector,2);     // checksum for 2 sectors of song data
	parentfs->setsectorchecksum(buffer,2*512,songsector+2,5); // checksum for 5 sectors of allocation data

	parentfs->writesectors(parentfs->devhd24,
			songsector,
			buffer,TOTAL_SECTORS_PER_SONG);
	
	parentfs->fstfix(buffer,TOTAL_SECTORS_PER_SONG*512); // sector is now in 'fixed' format again
	if (this->lengthened) 
	{
#if (SONGDEBUG == 1)
		cout << "song was lengthened, update of drive usage needed." << endl;
#endif
		parentfs->savedriveusage();
		this->lengthened=false;
	} else 
	{
#if (SONGDEBUG == 1)
		cout << "song was not lengthened, no update of drive usage needed." << endl;
#endif
	}
	parentfs->commit();
}
