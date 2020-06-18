#define PROJINFO_PROJECTNAME_8 0x0
#define PROJINFO_SONGCOUNT 0x0c
#define PROJINFO_LASTSONG 0x10
#define PROJINFO_SONGLIST 0x20
#define PROJINFO_PROJECTNAME 0x1b8
#define INVALID_SONGENTRY 0xFFFFFFFF
#define RESULT_SUCCESS 0
#define RESULT_FAIL 1
#define PROJDEBUG 1
#include <hd24utils.h>
#include "memutils.h"
void hd24project::initvars(hd24fs* p_parent,int32_t p_myprojectid)
{
	this->myprojectid=p_myprojectid;
	this->parentfs = p_parent;
	return;
}

int32_t hd24project::projectid()
{
	return this->myprojectid;
}

hd24project::hd24project(hd24fs* p_parent, int32_t p_myprojectid,
			uint32_t p_projectsector, const char* p_projectname,
			bool isnew) 
{
#if (PROJDEBUG == 1) 
	cout << "CONSTRUCT hd24project " << p_myprojectid << endl;
#endif
	// initialize a new project.
	// Boolean is obligatory to prevent accidents;
	// one can still create a project and not save it immediately.
	this->myprojectid = p_myprojectid;
	buffer = (unsigned char*)memutils::mymalloc("hd24project(1)",1024,1);
	parentfs = p_parent;
	if (!isnew) {
		p_parent->readsectors(p_parent->devhd24,
			p_parent->getprojectsectornum(myprojectid),
			buffer,1); // fstfix follows
			
		p_parent->fstfix(buffer, 512);
		projectname(p_projectname);
	} else {
#if (PROJDEBUG == 1) 
		cout << "Create new project " 
		<< p_projectname 
		<< " with id "
	        << p_myprojectid << " on sec " <<p_projectsector << endl;
#endif

		for (int i=0;i<1024;i++) {
			buffer[i]=0;
		}
		projectname(p_projectname);
		this->save(p_projectsector);		
	}
}

hd24project::hd24project(hd24fs* p_parent, int32_t p_myprojectid) 
{
	// project id is 1-based
#if (PROJDEBUG == 1) 
	cout << "Here we are. time to create the project object for proj. "
	<< p_myprojectid << endl;
#endif
	this->myprojectid = p_myprojectid;
	buffer = (unsigned char*)memutils::mymalloc("hd24project(2)",1024,1);
	if (buffer==NULL) {
		return;
	}
	parentfs = p_parent;
	uint32_t projsecnum=p_parent->getprojectsectornum(myprojectid);
	if (projsecnum==0) {
		for (int i=0;i<512;i++) {
			buffer[i]=0;
		}
	} else {
		p_parent->readsectors(p_parent->devhd24,
			projsecnum,
			buffer,1); // fstfix follows
			
		p_parent->fstfix(buffer, 512);
		//this->sort();
	}

}

hd24project::~hd24project() 
{
#if (PROJDEBUG == 1) 
	cout << "DESTRUCT hd24project " << myprojectid << endl;
#endif
	if (buffer != NULL) memutils::myfree("hd24project::buffer",buffer);
}

string* hd24project::projectname() 
{
	string* ver=parentfs->version();
	if (*ver == "1.00") 
	{
		delete ver;	
		// version 1.0 filesystem.
		string* tmp = new string("");
		string* dummy = Convert::readstring(buffer, PROJINFO_PROJECTNAME_8, 8);
		
		*tmp += *dummy;
		delete dummy;
                
		if (tmp->length() == 8) 
		{
			dummy = Convert::readstring(buffer, PROJINFO_PROJECTNAME_8 + 10, 2);
			*tmp += *dummy;
			delete dummy;
		}
		
		return tmp;
	}
	delete ver;	
	return Convert::readstring(buffer, PROJINFO_PROJECTNAME, 64);
}

void hd24project::projectname(string newname)
{
	hd24fs::setname(this->buffer,newname,PROJINFO_PROJECTNAME_8,PROJINFO_PROJECTNAME);	
	return;
}

uint32_t hd24project::maxsongs() 
{
	return parentfs->maxsongsperproject();
}

uint32_t hd24project::getsongsectornum(int i) 
{
	uint32_t songsec = Convert::getint32(buffer, 
		PROJINFO_SONGLIST + ((i - 1) * 4));
	// FIXME: Check for sensible values.		
	return songsec;
}

void hd24project::setsongsectornum(int i,uint32_t sector) 
{
	Convert::setint32(buffer, 
		PROJINFO_SONGLIST + ((i - 1) * 4),sector);
	// FIXME: Check for sensible values.		
	return;
}

void hd24project::lastsongid(int32_t songid)
{
	if (songid<1)
	{
		return;

	}
	int maxsongsperproj = maxsongs();	
	if (songid>maxsongsperproj)
	{
		return;
	}
	if (songid==lastsongid())
	{
		/* same 'last song id' as before-
		   nothing changes.
		*/
		return;
	}
	Convert::setint32(buffer, PROJINFO_LASTSONG,getsongsectornum(songid));
	this->save();
	return;
}

int32_t hd24project::lastsongid() 
{
	uint32_t lastsongsec = Convert::getint32(buffer, PROJINFO_LASTSONG);
	
	if (lastsongsec == 0)
	{
		return -1;
	}

	int maxsongsperproj = maxsongs();
	for (int i = 1; i <= maxsongsperproj; i++) 
	{
		uint32_t songsec = getsongsectornum(i);

		if (songsec == lastsongsec)
		{
			return i;
		}
	}
	
	if (maxsongsperproj >= 1)
	{
		return 1;
	}
	
	return -1;
}

uint32_t hd24project::songcount() 
{
	if (this==NULL)
	{
		return 0;
	}
	if (myprojectid == UINT32_MAX)
	{
		return 0;
	}
	if (buffer==NULL) {
		return 0;
	}
	uint32_t scount = Convert::getint32(buffer, PROJINFO_SONGCOUNT);
	
	if (scount > 99)
	{
		return 99;
	}
	
	return scount;
}

hd24song* hd24project::getsong(uint32_t songid) 
{
#if (PROJDEBUG == 1) 
	cout << "get song " << songid << endl;
#endif	
	return new hd24song(this,songid);
}

void hd24project::save(uint32_t projsector)
{
#if (PROJDEBUG == 1) 
	cout << "save project info to sector " << projsector << endl;
#endif
	if (buffer==NULL) 
	{
		return;	 // nothing to do!
	}
	parentfs->fstfix(buffer,512); // sector is now in native format again 
	parentfs->setsectorchecksum(buffer,0,projsector,1);     
	parentfs->writesectors(parentfs->devhd24,
			projsector,
			buffer,1);
	
	parentfs->fstfix(buffer,512); // sector is now in 'fixed' format
	parentfs->commit();
}

void hd24project::save()
{
	// This is capable of handling only 1-sector-per-project projects.
	// save(projsector) is only intended for new projects.
	uint32_t projsector=parentfs->getprojectsectornum(this->myprojectid);
#if (PROJDEBUG == 1) 
	cout << "save project info to sector " << projsector << endl;
#endif	
	if (projsector==0) 
	{
		return; // safety measure
	}
	save(projsector);
}

uint32_t hd24project::getunusedsongslot()
{
	// Find the first unused song slot in the current project.
	uint32_t unusedsongslot=INVALID_SONGENTRY;

	int maxsongcount=this->maxsongs();
	for (int j=1; j<=maxsongcount;j++) 
	{
		// get song sector info.
		uint32_t songsector = getsongsectornum(j);
		if (songsector==0) 
		{
			unusedsongslot=(j-1);
			break;
		}
	}
	
	return unusedsongslot;
}

hd24song* hd24project::createsong(const char* songname,uint32_t logicaltrackcount,
				  uint32_t samplerate)
{
	/* This creates a new song (with given songname)
	   on the drive (if possible).
           NULL is returned when unsuccessful, a pointer to the
           new song object otherwise.
        */

	if ((samplerate>=88200) && (logicaltrackcount>12))
	{
		// HD24 doesn't know how to handle over 12 tracks at 88k2+
		return NULL;
	}
	uint32_t songslot=getunusedsongslot();
	if (songslot==INVALID_SONGENTRY) 
	{
		// project is full.
		return NULL;
	}

	// Project is not full so there must be unused song sectors
	// (if not, something seriously fishy is going on).
	uint32_t newsongsector=this->parentfs->getunusedsongsector();
	if (newsongsector==0)
	{
		// no songsector found (0 is not a valid songsector)
		return NULL;
	}

	// So, we have got a free songsector and a free song slot.
	unsigned char songbuf[512*7];
	for (int i=0;i<(7*512);i++) 
	{
		songbuf[i]=(unsigned char)(0);
	}
	hd24song::sectorinit(songbuf);	// put some default info in there

	hd24song::songname((unsigned char*)songbuf,songname);
	string* setsongname=hd24song::songname(parentfs,songbuf);
	delete setsongname;

	hd24song::samplerate((unsigned char*)songbuf,samplerate); 
	hd24song::logical_channels((unsigned char*)songbuf,logicaltrackcount) ;// depends on samplerate
	uint32_t songsector=newsongsector;

	// - Create the empty song at the songsector 
	// (2 sectors in size)
 	// - Create empty allocation info at songsector+2
	// (5 sectors in size)
	// write song info to drive

        parentfs->fstfix(songbuf,TOTAL_SECTORS_PER_SONG*512); // sector is now once again in native format
        parentfs->setsectorchecksum(songbuf,0,songsector,2);
        parentfs->setsectorchecksum(songbuf,2*512,songsector+2,5);
	parentfs->writesectors(parentfs->devhd24,songsector,songbuf,TOTAL_SECTORS_PER_SONG);
        parentfs->fstfix(songbuf,TOTAL_SECTORS_PER_SONG*512); // sector is now once again in fixed format

	/////////////////////////////
	// -Update the project slot info to point at that sector
        Convert::setint32(buffer,PROJINFO_SONGLIST+(songslot*4),songsector);
	this->lastsongid(songslot+1);
        Convert::setint32(buffer,PROJINFO_SONGCOUNT,
                Convert::getint32(buffer,PROJINFO_SONGCOUNT)+1);
	this->save();
	parentfs->rewritesongusage();

	////////////////////////
	// Now commit the FS
	parentfs->commit();
	
	return getsong(songslot+1);
}

uint32_t hd24project::deletesong(uint32_t songid) 
{
	hd24song* songtodel=this->getsong(songid);
#if (PROJDEBUG == 1)
	cout << "del song with id " << songid << endl;
#endif
	uint32_t songsector=getsongsectornum(songid);
#if (PROJDEBUG == 1)
	cout << "sector " << songsector << endl;
#endif
	songsector++; songsector--; // prevent 'variable not used' warning

	if (songtodel==NULL) 
	{
#if (PROJDEBUG == 1)
	cout << "song to del==null " << endl;
#endif
		return RESULT_FAIL;
	}
	songtodel->songlength_in_wamples(0,false); // deallocate allocated clusters
	songtodel->save();

	// delete song from project list by shifting left
	// all other songs...
	for (uint32_t i=songid;i<99;i++) 
	{
#if (PROJDEBUG == 1)
	cout << "set new sector for song id " << i <<" to " << getsongsectornum(i+1) << endl;
#endif
		setsongsectornum(i,getsongsectornum(i+1));
	}
	setsongsectornum(99,0); // ...and clearing the last entry.
	uint32_t scount = Convert::getint32(buffer, PROJINFO_SONGCOUNT);
	Convert::setint32(buffer,PROJINFO_SONGCOUNT,scount-1);
#if (PROJDEBUG == 1)
	cout << "set new song count to " << (scount-1) << endl;
#endif

	// set 'last accessed song' to first song in list
	// (if there are no songs anymore, this automatically
	// results in 'last song' to be on sector zero.
#if (PROJDEBUG ==1)
	cout << "set default project song to song 1 with sectornum " << getsongsectornum(1) << endl;
#endif
	Convert::setint32(buffer, PROJINFO_LASTSONG,getsongsectornum(1));
#if (PROJDEBUG ==1)
	cout << "save project." << endl;
#endif

	this->save();
#if (PROJDEBUG ==1)
	cout << "refresh song usage table (+superblock) " << endl;
#endif
	parentfs->rewritesongusage(); 
	
	unsigned char* sectors_driveusage=parentfs->getsectors_driveusage();

	// now, we still have the song object songtodel,
	// plus its original sector number.
#if (PROJDEBUG ==1)
	cout << "Reset song length" << endl;
#endif
#if (PROJDEBUG ==1)
	cout << "Unmark used clusters " << endl;
#endif
	songtodel->unmark_used_clusters(sectors_driveusage);
#if (PROJDEBUG ==1)
	cout << "Destruct song object." << endl;
#endif
	delete(songtodel);
#if (PROJDEBUG ==1)
	cout << "Save drive usage." << endl;
#endif
	parentfs->savedriveusage();	
#if (PROJDEBUG ==1)
	cout << "Commit FS" << endl;
#endif
	parentfs->commit();
	return RESULT_SUCCESS;
}

void hd24project::sort()
{
	// a max. of 99 elements is still doable by bubble sort.
	int songs=this->songcount();
	if (songs<2) return; // nothing to sort!
	string** songnames=(string**)memutils::mymalloc("hd24project::sort",99,sizeof(string*));
	hd24song* song1=NULL;

      	int i, j;
     	int arrayLength = songs;
	for (i=1;i<=arrayLength;i++)
	{
		song1=getsong(i);
		songnames[i-1]=song1->songname();
		delete song1;
		song1=NULL;
	}
	
      	for(i = 1; i <= arrayLength ; i++)
     	{
          	for (j=0; j < (arrayLength -1); j++)
         	{
			int compare=strncmp(songnames[j]->c_str(),songnames[j+1]->c_str(),64);
			if (compare>0)
              		{
				// swap entries
				uint32_t a=getsongsectornum(j+1);
				uint32_t b=getsongsectornum(j+2);
				setsongsectornum(j+2,a);
				setsongsectornum(j+1,b);
				string* tmp=songnames[j];
				songnames[j]=songnames[j+1];
				songnames[j+1]=tmp;
			}
		}
	}
	
	for (i=1;i<=arrayLength;i++)
	{
		free(songnames[i-1]);
	}
	free(songnames);
	return;
}	
