#include "hd24transferengine.h"
#include <stdint.h>
#include <hd24fs.h>
#include <hd24sndfile.h>

#define SHOWAUDIODATA 0
#define MAXPHYSICALCHANNELS 24
#define MAXLOGICALCHANNELS 12
#define TRACKACTION_UNDEF -1
#define TRACKACTION_ERASE 0
#define TRACKACTION_SMPTE 1
#define TRACKACTION_MONO 2
/* 
	Note: prior to working on this code, it's recommended to read
	document samplerates.txt in the doc directory as some of the
	high samplerate issues are counter-intuitive.
	The document will help you straighten out your mind so 
	things are simpler to understand.
*/
hd24transferjob::hd24transferjob()
{
#if (HD24TRANSFERDEBUG==1)
	cout << "hd24transferjob::hd24transferjob();" << endl;
#endif
	init_vars();
}

hd24transferjob::~hd24transferjob()
{
#if (HD24TRANSFERDEBUG==1)
	cout << "hd24transferjob::~hd24transferjob();" << endl;
#endif
	if (trackselected!=NULL) 
	{
		memutils::myfree("trackselected",trackselected);
		trackselected=NULL;
	}
	usecustomrate=0; // by default, match export samrate with song rate
	if (m_projectdir!=NULL)
	{
		delete m_projectdir;
		m_projectdir=NULL;
	}
	for (int i=0;i<24;i++)
	{
		if (this->filepath[i]!=NULL)
		{
			memutils::myfree("this->filepath[i]",this->filepath[i]);
			this->filepath[i]=NULL;
		}
		if (this->filehandle[i]!=NULL)
		{
			memutils::myfree("this->filehandle[i]",this->filehandle[i]);
			this->filehandle[i]=NULL;
		}
	}
	if (m_filenameformat!=NULL)
	{
		delete m_filenameformat;
		m_filenameformat=NULL;		
	}
	
	if (filehandle!=NULL)
	{
		memutils::myfree("filehandle",filehandle);
		filehandle=NULL;
	}
	if (filepath!=NULL)
	{
		memutils::myfree("filepath",filepath);
		filepath=NULL;
	}
}

void hd24transferjob::init_vars()
{
#if (HD24TRANSFERDEBUG==1)
	cout << "hd24transferjob::init_vars()" << endl;
#endif
	
	trackselected=(int*)memutils::mymalloc("trackselected",MAXPHYSICALCHANNELS,sizeof(int));
	for (int i=0;i<MAXPHYSICALCHANNELS;i++) 
	{
		trackselected[i]=0;
	}
	filepath=NULL;
	filehandle=NULL;
	m_projectdir=NULL;	
	llsizelimit=1024*1024*1024;
	mixleft=0;
	mixright=0;
	m_selectedformat=0;
	usecustomrate=0;
	stamprate=48000;
	wantsplit=1;

	m_reeloffset=0;
	
	jobsourcesong=NULL;
	jobtargetsong=NULL;
	jobsourcefs=NULL;	
	jobtargetfs=NULL;
	have_smpte=false;
	m_filenameformat=NULL;
	
	m_startoffset=0;
	m_endoffset=0;
	filepath=(char**)memutils::mymalloc("filepath",MAXPHYSICALCHANNELS,sizeof(char*));
	filehandle=(SNDFILE**)memutils::mymalloc("filehandle",MAXPHYSICALCHANNELS,sizeof(SNDFILE*));
	for (int i=0;i<MAXPHYSICALCHANNELS;i++)
	{
		this->filepath[i]=NULL;
		this->filehandle[i]=NULL;
		this->m_trackaction[i]=TRACKACTION_UNDEF; // undefined trackaction
	}
	return;
}

void hd24transferjob::trackaction(int base1tracknum,int action)
{
#if (HD24TRACACTDEBUG==1)
	cout << "hd24transferjob::trackaction(base1track="
		<< base1tracknum << ", action=" << action << ")" << endl;
#endif

	m_trackaction[base1tracknum-1]=action;
}

int hd24transferjob::trackaction(int base1tracknum)
{
#if (HD24TRANSFERDEBUG==1)
	cout << "returning hd24transferjob::trackaction(base1track="
	     << base1tracknum << ")=" << m_trackaction[base1tracknum-1]
	     << endl;
#endif	
	return m_trackaction[base1tracknum-1];
}

void hd24transferengine::trackaction(int base1tracknum,int action)
{
	job->trackaction(base1tracknum,action);
}

int hd24transferengine::trackaction(int base1tracknum)
{
	return job->trackaction(base1tracknum);
}

char* hd24transferjob::sourcefilename(int base1tracknum)
{
	if (base1tracknum<1) return NULL;
	if (base1tracknum>24) return NULL;
	return filepath[base1tracknum-1];
}

void hd24transferjob::sourcefilename(int base1tracknum,const char* filename)
{
#if (HD24TRANSFERDEBUG==1)
	cout << "hd24transferjob::sourcefilename(base1track="
		<< base1tracknum << ", name=" << filename << ")" << endl;
#endif

	/* set nth filename to the given char string */
	if (base1tracknum<1) return;
	if (base1tracknum>MAXPHYSICALCHANNELS) return;

	if (filepath[base1tracknum-1]!=NULL)
	{
		memutils::myfree("filepath[base1track-1]",filepath[base1tracknum-1]);
		filepath[base1tracknum-1]=NULL;
	}
	if (filename==NULL)
	{
		return;
	}
	uint32_t n=strlen(filename);
	if (n==0)
	{
		return;
	}
	filepath[base1tracknum-1]=(char*)memutils::mymalloc("filepath",n+1,1);	
	strncpy(filepath[base1tracknum-1],filename,n+1);
        
	return;
}

void hd24transferengine::sourcefilename(int base1tracknum,const char*  filename)
{
	job->sourcefilename(base1tracknum,filename);
}

char* hd24transferengine::sourcefilename(int base1tracknum)
{
	return job->sourcefilename(base1tracknum);
}

void hd24transferjob::startoffset(uint32_t newoff)
{
	m_startoffset=newoff;
}

void hd24transferjob::endoffset(uint32_t newoff)
{
	m_endoffset=newoff;
}

uint32_t hd24transferjob::startoffset()
{
	return m_startoffset;
}

uint32_t hd24transferjob::endoffset()
{
	return m_endoffset;
}

void hd24transferjob::reeloffset(uint32_t newoff)
{
	m_reeloffset=newoff;
}

uint32_t hd24transferjob::reeloffset()
{
	return m_reeloffset;
}

uint32_t hd24transferengine::reeloffset()
{
	if (job==NULL) return 0;
	return job->reeloffset();
}

void hd24transferengine::reeloffset(uint32_t newoff)
{
	if (job==NULL) return;
	job->reeloffset(newoff);
}

hd24fs* hd24transferjob::sourcefs()
{
	return this->jobsourcefs;
}

hd24song* hd24transferjob::sourcesong()
{
	return this->jobsourcesong;
}

void hd24transferjob::sourcefs(hd24fs* fs)
{
	this->jobsourcefs=fs;
}

void hd24transferjob::sourcesong(hd24song* song)
{
	this->jobsourcesong=song;
	this->jobsourcefs=jobsourcesong->parentfs;
}

hd24fs* hd24transferjob::targetfs()
{
	return this->jobtargetfs;
}

hd24song* hd24transferjob::targetsong()
{
	return this->jobtargetsong;
}

void hd24transferjob::targetfs(hd24fs* fs)
{
	this->jobtargetfs=fs;
}

void hd24transferjob::targetsong(hd24song* song)
{
	this->jobtargetsong=song;
	this->jobtargetfs=jobtargetsong->parentfs;
}

void hd24transferjob::filenameformat(string* filenameformat)
{
	if (m_filenameformat!=NULL)
	{
		delete m_filenameformat;
		m_filenameformat=NULL;
	}
	m_filenameformat=new string(filenameformat->c_str());
	return;
}

string* hd24transferjob::filenameformat()
{
	return m_filenameformat;
}

void hd24transferengine::filenameformat(string* newformat)
{
	if (job==NULL)
	{
		return;
	}
	job->filenameformat(newformat);
}

string* hd24transferengine::filenameformat()
{
	if (job==NULL)
	{
		return NULL;
	}
	return job->filenameformat();
}

void hd24transferjob::projectdir(const char* projdir)
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferjob::projectdir("<<projdir<< ")"<<endl;
#endif
	if (m_projectdir!=NULL)
	{
		delete m_projectdir;
		m_projectdir=NULL;
	}
	m_projectdir=new string(projdir);
	return;
}

const char* hd24transferjob::projectdir()
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "returning hd24transferjob::projectdir()=";
	if (m_projectdir==NULL)
	{
		cout <<" NULL " << endl; // PRAGMA allowed
	} else {
		cout <<*m_projectdir<<endl; // PRAGMA allowed
	}
#endif
	return m_projectdir->c_str();
}

void hd24transferengine::lasterror(const char* errormessage)
{
	if (m_lasterror!=NULL)
	{
		delete m_lasterror;
	}
	m_lasterror=new string(errormessage);
	return;
}

string* hd24transferengine::lasterror()
{
	return m_lasterror;
}


int hd24transferengine::format_outputchannels(int format)
{
	return m_format_outputchannels[format];
}

const char* hd24transferengine::projectdir()
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "returning hd24transferengine::projectdir()"<<endl;
#endif
	return job->projectdir();
}

void hd24transferengine::projectdir(const char* projdir)
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferengine::projectdir("<<projdir<< ")"<<endl;
#endif
	job->projectdir(projdir);
	return;
}

hd24transferengine::hd24transferengine()
{
#if (HD24TRANSFERDEBUG==1) 
	cout <<"hd24transferengine::hd24transferengine()" << endl;
#endif
	init_vars();
}

hd24transferengine::~hd24transferengine()
{
#if (HD24TRANSFERDEBUG==1) 
	cout <<"hd24transferengine::~hd24transferengine()" << endl;
#endif
	if (m_lasterror!=NULL)
	{
		delete m_lasterror;
		m_lasterror=NULL;
	}
	if (job!=NULL)
	{
		delete job;
		job=NULL;
	}
	if (m_format_outputextension!=NULL)
	{
		delete m_format_outputextension;
		m_format_outputextension=NULL;
	}
	if (m_format_shortdesc!=NULL)
	{
		delete m_format_shortdesc;
		m_format_shortdesc=NULL;
	}
	if (strdatetime!=NULL)
	{
		delete strdatetime;
		strdatetime=NULL;
	}
}

void hd24transferengine::init_vars()
{
	songnum=0;
	totsongs=0;
	ui=NULL;
	totbytestotransfer=0;
	totbytestransferred=0;
	prefix=0;
	
	trackspergroup=0;
	transfermixer=NULL;
	job=NULL;
	m_lasterror=NULL;
	job=new hd24transferjob();
	uiconfirmfunction=NULL;
	setstatusfunction=NULL;
	soundfile=NULL;
	strdatetime=NULL;
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferengine::init_vars() - job="<<job<<endl;
#endif
	m_format_outputextension=new vector<string>;
	m_format_shortdesc=new vector<string>;
	populate_formatlist();
}

void hd24transferengine::populate_formatlist()
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferengine::populate_formatlist()" << endl;
#endif
/* Set up transfer format list */
	formatcount=0;
	m_format_shortdesc->push_back("WAV (24 bit), mono");
	m_format_outputformat[formatcount]=SF_FORMAT_WAV|SF_FORMAT_PCM_24|SF_ENDIAN_LITTLE; 
	m_format_outputchannels[formatcount]=1; // mono
	m_format_bitdepth[formatcount]=24;
	m_format_sndfile[formatcount]=false; // false=do not use libsndfile for this file format.
						// no longer set to true, see  #1665
	m_format_outputextension->push_back(".wav");
	formatcount++;

	m_format_shortdesc->push_back("WAV (24 bit), stereo");
	m_format_outputformat[formatcount]=SF_FORMAT_WAV|SF_FORMAT_PCM_24|SF_ENDIAN_LITTLE; 
	m_format_outputchannels[formatcount]=2; // stereo
	m_format_bitdepth[formatcount]=24;
	m_format_sndfile[formatcount]=true;
	m_format_outputextension->push_back(".wav");
	formatcount++;

	m_format_shortdesc->push_back("WAV (24 bit), multi");
	m_format_outputformat[formatcount]=SF_FORMAT_WAV|SF_FORMAT_PCM_24|SF_ENDIAN_LITTLE; 
	m_format_outputchannels[formatcount]=0; // multi
	m_format_bitdepth[formatcount]=24;
	m_format_sndfile[formatcount]=true;
	m_format_outputextension->push_back(".wav");
	formatcount++;

	m_format_shortdesc->push_back("AIF (24 bit), mono");
	m_format_outputformat[formatcount]=SF_FORMAT_AIFF|SF_FORMAT_PCM_24; 
	m_format_outputchannels[formatcount]=1; // mono
	m_format_bitdepth[formatcount]=24;
	m_format_sndfile[formatcount]=false;
	m_format_outputextension->push_back(".aif");
	formatcount++;

	m_format_shortdesc->push_back("AIF (24 bit), stereo");
	m_format_outputformat[formatcount]=SF_FORMAT_AIFF|SF_FORMAT_PCM_24; 
	m_format_outputchannels[formatcount]=2; // stereo
	m_format_bitdepth[formatcount]=24;
	m_format_sndfile[formatcount]=true;
	m_format_outputextension->push_back(".aif");
	formatcount++;

	m_format_shortdesc->push_back("AIF (24 bit), multi");
	m_format_outputformat[formatcount]=SF_FORMAT_AIFF|SF_FORMAT_PCM_24; 
	m_format_outputchannels[formatcount]=0; // stereo
	m_format_bitdepth[formatcount]=24;
	m_format_sndfile[formatcount]=true;
	m_format_outputextension->push_back(".aif");
	formatcount++;
	selectedformat(0);
}

const char* hd24transferengine::getformatdesc(int formatnum)
{
	return ((*m_format_shortdesc)[formatnum]).c_str();
}

int hd24transferengine::supportedformatcount()
{
	return formatcount;
}

void hd24transferengine::trackselected(uint32_t base0tracknum,bool selected)
{
	if (base0tracknum<0) return;
	if (base0tracknum>=MAXPHYSICALCHANNELS) return;

	if (selected)
	{
		job->trackselected[base0tracknum]=1;
	} else {
		job->trackselected[base0tracknum]=0;
	}
	return;
}

bool hd24transferengine::trackselected(uint32_t base0tracknum)
{
	if (base0tracknum<0) return false;
	if (base0tracknum>=MAXPHYSICALCHANNELS) return false;

	return ((job->trackselected[base0tracknum])==1);
}


void hd24transferengine::openbuffers(unsigned char** audiobuf,
                                     unsigned int channels,
				     unsigned int bufsize)
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferengine::openbuffers" << endl; 
#endif

	for (unsigned int handle=0;handle<channels;handle++) 
	{
		audiobuf[handle]=NULL;
		if (!trackselected(handle)) {
			// channel not selected for export
			continue;
		}
		audiobuf[handle]=
			(unsigned char *)memutils::mymalloc("openbuffers",
							     bufsize,1);
	}
	#if (HD24TRANSFERDEBUG==1) 
	cout << "opened buffers" << endl; 
	#endif
}

void hd24transferengine::closebuffers(unsigned char** audiobuf,unsigned int channels)
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferengine::closebuffers" << endl; 
#endif
	for (uint32_t handle=0;handle<channels;handle++) 
	{
		if (!trackselected(handle)) {
			// channel not selected for export
			continue;
		}
		if (audiobuf[handle]!=NULL)
		{
			memutils::myfree("closebuffers",audiobuf[handle]);
		}
	}
}

void hd24transferengine::writerawbuf(hd24sndfile* filehandle,unsigned char* buf,long subblockbytes)
{
	filehandle->writerawbuf(buf,subblockbytes);
}

bool hd24transferengine::openinputfiles(SNDFILE** filehandle,SF_INFO* sfinfoin,unsigned int channels)
{
	int cannotopen=0;
	if (job==NULL)
	{
		lasterror("No job found, transfer aborted.");
		return false;
	}
	
	int trackcount=0;
	for (unsigned int handle=0;handle<channels;handle++) 
	{
#if (HD24TRANSFERDEBUG!=0)
		cout << "Openinputfiles - handle=" << handle << endl;
#endif
		char* currsourcefilename=job->sourcefilename(handle+1);
		if (currsourcefilename==NULL) {
#if (HD24TRANSFERDEBUG!=0)
		cout << "Channel "
		     << (handle+1)
		     << " not selected for transfer, skipping" << endl;
#endif				
			continue;
		}
		uint32_t filelen=strlen(currsourcefilename);
		if (filelen==0) {
#if (HD24TRANSFERDEBUG!=0)
		cout << "Channel "
		     << (handle+1)
		     << "not selected for transfer, skipping" << endl;
#endif				
			continue;
		}		
		trackcount++;
		
		
#if (HD24TRANSFERDEBUG!=0)
		cout << "Channel "
		     << (handle+1)
		     << "selected for transfer" << endl
		     << "That brings the total track count to "
		     << trackcount << endl
		     << "job=" << job << endl
		     << "soundfile=" << soundfile << endl
		     << "sourcefilename=" << currsourcefilename
		     << endl;
#endif						
		
		job->filehandle[handle]=soundfile->sf_open(currsourcefilename,SFM_READ,&sfinfoin[handle]);
		if (!(job->filehandle[handle])) 
		{
			cannotopen=1;
			continue;
		}
	}

	return (cannotopen==0); // return true if successful, flase otherwise
}

bool hd24transferengine::openoutputfiles(hd24sndfile** filehandle,unsigned int channels,unsigned int partnum,int prefix)
{
	trackspergroup=0;

	SF_INFO sfinfoout;
	bool uselibsndfile=m_format_sndfile[selectedformat()];
	sfinfoout.format=m_format_outputformat[selectedformat()];
	int chcount=0;
	if (m_format_outputchannels[selectedformat()]==0) {
		// count channels
		for (unsigned int handle=0;handle<channels;handle++) 
		{
			if (job->trackselected[handle]!=0) {
				chcount++;
			}
		}
		sfinfoout.channels=chcount;
	} else {
		sfinfoout.channels=m_format_outputchannels[selectedformat()];
	}
	if (job->usecustomrate==1) {
		long samrate=job->stamprate;
		sfinfoout.samplerate=samrate;
	} else {
		sfinfoout.samplerate=job->sourcesong()->samplerate();
	}
	// chcount and lasthandle are intended to allow
	// support for writing multichannel files.
	int lasthandle=0;
	int lasttrack=-1;
	int trackcount=0;
	int cannotopen=0;
	chcount=0;
		
	for (unsigned int handle=0;handle<channels;handle++) 
	{

		islastchanofgroup[handle]=false;
		isfirstchanofgroup[handle]=false;
	
		if (job->trackselected[handle]==0) {
			// channel not selected for export
			continue;
		}
		trackcount++;
		lasttrack=handle;

		string* fname=generate_filename(handle,partnum,prefix);
		if (fname==NULL)
		{
			/* cannot generate filename. Most likely cause is
			   no song set */
			lasterror("Cannot generate filename. No song set?");
			return false; // problem opening files.
		}
		bool direxists=hd24utils::guaranteefiledirexists(fname);
		if (!direxists)
		{
			/* Dir mentioned in filename did not exist
			   and could not be created. */
			lasterror("Could not create directory for files.");
			return false;
		}
		
		if (chcount==0) {
			isfirstchanofgroup[handle]=true;
			if (uselibsndfile)
			{
				filehandle[handle]->sndfilehandle=NULL;
				if (soundfile->sf_open) {
					filehandle[handle]->open(fname->c_str(),(int)SFM_WRITE,&sfinfoout,soundfile);
				}
				if (!(filehandle[handle]->handle()))
				{
					cannotopen=1;
				}
			} 
			else 
			{
				filehandle[handle]->open(fname->c_str(),(int)SFM_WRITE,&sfinfoout);		
			}
			lasthandle=handle;
		} else {		
			// copy handle of previously opened file to current track
			filehandle[handle]->handle(filehandle[lasthandle]->handle(),filehandle[lasthandle]->handletype());
			if (!(filehandle[handle]->handle()))
			{
				cannotopen=1;
			}
		}
		chcount++;
		if ((sfinfoout.channels)!=0) 
		{
			// sfinfoout.channels=0 means full multitrack file
			// so only a single file will be opened.
			chcount=chcount%(sfinfoout.channels);
			if (chcount==0) {
				islastchanofgroup[handle]=true;
				if (trackspergroup==0) {
					trackspergroup=trackcount;
				}
			}
		}

		delete fname;
	}
	if (lasttrack>=0) 
	{
		if (trackspergroup==0) {
			trackspergroup=trackcount;
		}

		islastchanofgroup[lasttrack]=true;
	}
	return (cannotopen==0); // return true if successful, false otherwise
}

void hd24transferengine::closeinputfiles(SNDFILE** filehandle,unsigned int channels)
{
	SNDFILE* lasthandle=NULL;
	for (unsigned int handle=0;handle<channels;handle++) 
	{
		if (job->trackselected[handle]==0) continue;
		if (filehandle[handle]!=lasthandle) {
			soundfile->sf_close(filehandle[handle]);
			lasthandle=filehandle[handle];
			filehandle[handle]=NULL;
		}
	}
}

void hd24transferengine::closeoutputfiles(hd24sndfile** filehandle,unsigned int channels)
{
	void* lasthandle=NULL;

	for (unsigned int handle=0;handle<channels;handle++) 
	{
		if (job->trackselected[handle]==0) continue;
		if (filehandle[handle]->handle()!=lasthandle) {
			lasthandle=(void*)filehandle[handle]->handle();
			if (lasthandle!=NULL) {
				filehandle[handle]->close();
			}
		}
	}
}

bool hd24transferengine::confirmfileoverwrite()
{
	if (this->uiconfirmfunction==NULL)
	{
		lasterror("Output files already exist, transfer aborted.");
		return false;
	}
	
	return uiconfirmfunction(ui,"One or more output files already exist. "
				 "Do you want to overwrite them?");	
}	

bool hd24transferengine::overwritegivesproblems(hd24song* thesong,int partnum)
{
	/* this function will return TRUE if overwriting 
	  existing files causes a problem, FALSE otherwise. 
	  
	  Overwriting files does NOT cause problems if
	  - no files are going to be overwritten
	  or
	  - Files are going to be overwritten but the user
	    permits us to do so.
	    
	  */

	if (anyfilesexist(thesong)) 
	{
		return (!(confirmfileoverwrite()));
	}
	return false;
}

bool hd24transferengine::dontopenoutputfiles(hd24sndfile** filehandle,unsigned int channels,unsigned int partnum,int prefix)
{
	// HACK 
	trackspergroup=0;

	SF_INFO sfinfoout;
	//bool uselibsndfile=m_format_sndfile[selectedformat()];
	sfinfoout.format=m_format_outputformat[selectedformat()];
	int chcount=0;
	if (m_format_outputchannels[selectedformat()]==0) {
		// count channels
		for (unsigned int handle=0;handle<channels;handle++) 
		{
			if (job->trackselected[handle]!=0) {
				chcount++;
			}
		}
		sfinfoout.channels=chcount;
	} else {
		sfinfoout.channels=m_format_outputchannels[selectedformat()];
	}
	if (job->usecustomrate==1) {
		long samrate=job->stamprate;
		sfinfoout.samplerate=samrate;
	} else {
		sfinfoout.samplerate=job->sourcesong()->samplerate();
	}
	// chcount and lasthandle are intended to allow
	// support for writing multichannel files.
	int lasthandle=0;
	int lasttrack=-1;
	int trackcount=0;
	int cannotopen=0;
	chcount=0;
	for (unsigned int handle=0;handle<channels;handle++) 
	{

		islastchanofgroup[handle]=false;
		isfirstchanofgroup[handle]=false;
	
		if (job->trackselected[handle]==0) {
			// channel not selected for export
			continue;
		}
		trackcount++;
		lasttrack=handle;

		string* fname=generate_filename(handle,partnum,prefix);
		if (fname==NULL)
		{
			// Cannot generate filename. No song set?			
			return false; // problem (not) opening files.
		}
		if (chcount==0) {
			isfirstchanofgroup[handle]=true;
	//		if (uselibsndfile)
	//		{
	//			filehandle[handle]->sndfilehandle=NULL;
	//			if (soundfile->sf_open) {
	//				filehandle[handle]->open(fname->c_str(),(int)SFM_WRITE,&sfinfoout,soundfile);
	//			}
	//			if (!(filehandle[handle]->handle()))
	//			{
	//				cannotopen=1;
	//			}
	//		} 
	//		else 
	//		{
	//			filehandle[handle]->open(fname->c_str(),(int)SFM_WRITE,&sfinfoout);		
	//		}
			lasthandle=handle;
		} else {		
			// copy handle of previously opened file to current track
	//		filehandle[handle]->handle(filehandle[lasthandle]->handle(),filehandle[lasthandle]->handletype());
	//		if (!(filehandle[handle]->handle()))
	//		{
	//			cannotopen=1;
	//		}
		}
		chcount++;
	//	if ((sfinfoout.channels)!=0) 
	//	{
	//		// sfinfoout.channels=0 means full multitrack file
	//		// so only a single file will be opened.
	//		chcount=chcount%(sfinfoout.channels);
	//		if (chcount==0) {
	//			islastchanofgroup[handle]=true;
	//			if (trackspergroup==0) {
	//				trackspergroup=trackcount;
	//			}
	//		}
	//	}

		delete fname;
	}

	if (lasttrack>=0) 
	{
		if (trackspergroup==0) {
			trackspergroup=trackcount;
		}

		islastchanofgroup[lasttrack]=true;
	}
	return (cannotopen==0); // return true if successful, false otherwise
}
void hd24transferengine::flushbuffer(hd24sndfile** filehandle,unsigned char** buffer,uint32_t flushbytes,unsigned int channels)
{
	#if (HD24TRANSFERDEBUG==1) 
	cout << "Flushbuffer " << flushbytes << " bytes" <<endl; 
	#endif
	for (unsigned int handle=0;handle<channels;handle++) 
	{
		if (job->trackselected[handle]==0) continue;

		writerawbuf(&(*filehandle)[handle],buffer[handle],flushbytes);
	}
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "Flushed" << endl; 
	#endif
}

void hd24transferengine::prepare_transfer_to_pc(uint32_t songnum,
			uint32_t totsongs,int64_t totbytestotransfer,
			int64_t totbytestransferred,
			int p_wantsplit,uint32_t prefix)
{
	// set properties as needed
#if (HD24TRANSFERDEBUG==1) 
	cout << "prepare transfer to pc:" << endl
	 << "song "<< songnum<< "/" << totsongs << ", "
	 << totbytestotransfer << " total bytes to transfer" << endl
	 << totbytestransferred << " transferred so far" << endl
	 << "wantsplit=" << p_wantsplit 
	 << ", prefix=" << prefix << endl;
#endif
	this->totbytestotransfer=totbytestotransfer;
	job->wantsplit=p_wantsplit;
	//	this->transfer_to_pc();
}

void hd24transferengine::transfer_in_progress(bool active)
{
	// TODO: callback function
	//	stop_transfer->show();
}

void hd24transferengine::mixleft(bool select)
{
	if (select) { job->mixleft=1; }
}

bool hd24transferengine::mixleft()
{
	if (job->mixleft==1)
	{
		return true;
	}
	return false;
}

void hd24transferengine::mixright(bool select)
{
	if (select) { job->mixright=1; }
}

bool hd24transferengine::mixright()
{
	if (job->mixright==1)
	{
		return true;
	}
	return false;
}

void hd24transferengine::generatetimestamp()
{
	//////////// Generate time stamp
	time (&jobtimestamp);
	struct tm timestamp;
	char timebuf[80];
			
	timestamp = *localtime(&jobtimestamp);
	
	strftime(timebuf,sizeof(timebuf),"%Y-%m-%d_%H:%M:%S", &timestamp);
	if (strdatetime!=NULL)
	{
		delete strdatetime;
		strdatetime=NULL;
	}
	strdatetime=new string(timebuf);
	//////////////
}

int64_t hd24transferengine::transfer_to_pc()
{
	// function returns bytes transferred for the transfer of only the current file.

	int64_t totsamcount;  /* Total number of samples exported */
	int64_t partsamcount; /* When splitting up the song into multiple parts, 
		                   number of samples exported to current part */
	int64_t blockcount;	/* Allows us to keep a write buffer as cache for
		                   better export performance */
	if (job->sourcesong()==NULL) 
	{
		lasterror("No source song defined, transfer aborted.");
		return 0;
	}
	if (ui!=NULL)
	{
		if (((HD24UserInterface*)ui)->transfer_cancel==1) 
		{
			lasterror("Cancel hit, transfer aborted.");
			return 0;
		}
	}

	// limit which tracks need to be actually read from HD24 disk
	for (uint32_t i=1;i<=MAXPHYSICALCHANNELS;i++)
	{
		if (job->trackselected[i-1]==1) 
		{
			job->sourcesong()->readenabletrack(i,true);
		} else {
			job->sourcesong()->readenabletrack(i,false);
		}
	}
	bool mustmixdown=false;
	if (job->mixleft==1)
	{
		mustmixdown=true;
	}
	if (job->mixright==1)
	{
		mustmixdown=true;
	}
	if (transfermixer==NULL)
	{
		/* as much as we'd like, there is no mixer so we can't mix. */
		mustmixdown=false;
	}
	generatetimestamp();
	#if (HD24TRANSFERDEBUG==1) 
	    cout << "start export 1" << endl; 
	#endif
	uint32_t lastremain=0xffffffff; //a
	uint32_t songlen_wam=job->sourcesong()->songlength_in_wamples();
	uint32_t logical_channels=job->sourcesong()->logical_channels();
	uint32_t bytespersam=(job->sourcesong()->bitdepth()/8);
	uint32_t chanmult=job->sourcesong()->chanmult();
	int mustdeinterlace=chanmult-1;
	uint32_t oldmixersamplerate=0;
	if (mustmixdown)
	{
		if (transfermixer!=NULL) {
			oldmixersamplerate=transfermixer->samplerate();
			transfermixer->samplerate(job->sourcesong()->samplerate());
		}
	}

	#if (HD24TRANSFERDEBUG==1)    
	 cout << "Log.Channels="<<logical_channels << endl
	  << "Songlen="<<songlen_wam << endl; 
	#endif
	double oldpct=0;
	#if (HD24TRANSFERDEBUG==1) 
	  cout << "start export 2" << endl; 
	#endif

	//SNDFILE* filehandle[channels];
	hd24sndfile* filehandle[MAXPHYSICALCHANNELS];

	for (int i=0;i<MAXPHYSICALCHANNELS;i++) 
	{
		filehandle[i]=new hd24sndfile(m_format_outputformat[selectedformat()],soundfile);
	}

	uint32_t partnum=0;
	uint64_t MAXBYTES=job->sizelimit();

	/*
		if total number of samples is likely to overflow 
		max allowable length, ask if user wants to split up
		the files.
	*/
	if (job->wantsplit==1) {
		if ((songlen_wam*bytespersam*chanmult)>MAXBYTES)
		{
			// splitting is needed
			partnum=1; // count part numbers in filename
		} else {
			// no splitting needed
			partnum=0;
		}		
	} else {
		partnum=0; // do not count part numbers in filename
		MAXBYTES=songlen_wam*bytespersam*chanmult; // allow the full song length in a single file
	}

	/* Start location 0 is absolute, all other offsets 
	   are relative to the start offset. The only difference 
	   is in the timecode.
	   Because of this, if startoffset>0 is given, we
	   can ignore this. */
	uint64_t translen;
	uint64_t tottranslen=0; // for percentage calc in multi-song exports
	signed int stepsize;
	if (job->startoffset()>job->endoffset()) 
	{
		uint32_t tempoffset=job->startoffset();
		job->startoffset(job->endoffset());
		job->endoffset(tempoffset);
	}
	
	translen=(job->endoffset()-job->startoffset())*(job->sourcesong()->physical_channels()/job->sourcesong()->logical_channels());
#if (HD24TRANSFERDEBUG==1)
	cout << "startoffset=" << job->startoffset() << endl
	 << "endoffset=" << job->endoffset() << endl;
#endif
	
	if (translen==0)
	{
		lasterror("Start offset equals end offset- nothing to do!");
	}
	stepsize=1;

	bool canopen=true;
	MixerChannelControl* mixerchannel[24];
	for (unsigned int tracknum=0;tracknum<MAXPHYSICALCHANNELS;tracknum++)
	{
		mixerchannel[tracknum]=NULL;
	}
	
	if (mustmixdown)
	{	
		for (unsigned int tracknum=0;tracknum<MAXPHYSICALCHANNELS;tracknum++) 
		{
			mixerchannel[tracknum]=transfermixer->parentui()->mixerchannel[tracknum]->control;	
		}
		// HACK
		canopen=dontopenoutputfiles(
			(hd24sndfile**)&filehandle[0],
			logical_channels,
			partnum,
			prefix); // partnum is for splitting large files
	} else {
		canopen=openoutputfiles(
			(hd24sndfile**)&filehandle[0],
			logical_channels,
			partnum,
			prefix); // partnum is for splitting large files
	}
	
	if (!canopen) 
	{
	#if (HD24TRANSFERDEBUG==1) 
	  cout << "error opening output files" << endl; 
	#endif
		lasterror("Error opening output files, transfer aborted.");
		if (ui!=NULL)
		{
			((HD24UserInterface*)ui)->transfer_cancel=1;
		}
		return 0;
	} else {
		if (ui!=NULL)
		{
			((HD24UserInterface*)ui)->transfer_cancel=0;
		}
	}
	
	this->transfer_in_progress(true);
	time (&transferstarttime);
		
	partsamcount=0; totsamcount=0;
	int trackwithingroup=0;
	#if (HD24TRANSFERDEBUG==1) 
	cout << "songlen in samples=" << songlen_wam << endl; 
	#endif
	blockcount=0; // for buffered writing.
	hd24fs* currenthd24=job->sourcefs();
	if (currenthd24==NULL)
	{
#if (HD24TRANSFERDEBUG==1)
		cout << "Critical error- current FS not defined." << endl; 
#endif
	}
	int blocksize=currenthd24->getbytesperaudioblock();
	// to hold normally read audio data:
	unsigned char* audiodata=(unsigned char*)memutils::mymalloc("ftransfer_to_pc",blocksize,1);

	// for high-samplerate block deinterlacing:
	unsigned char* deinterlacedata=(unsigned char*)memutils::mymalloc("ftransfer_to_pc",blocksize,1); 

	// for interlacing multi-track data:
	unsigned char* interlacedata=(unsigned char*)memutils::mymalloc("ftransfer_to_pc",blocksize,1); 


	uint32_t samplesperlogicalchannel=(blocksize/logical_channels)/bytespersam;
	float* outputBuffer=NULL;
	SF_INFO infoblock;
	hd24sndfile* mixdownfile=NULL;
	if (mustmixdown)
	{
		outputBuffer=(float*)memutils::mymalloc("outputBuffer",2*samplesperlogicalchannel,sizeof(float)); // 2 because it is stereo
		mixdownfile=new hd24sndfile(SF_FORMAT_WAV|SF_FORMAT_PCM_32
		                            |SF_FORMAT_FLOAT,soundfile);
		infoblock.channels=2;
		infoblock.samplerate=job->sourcesong()->samplerate();
		infoblock.format=SF_FORMAT_WAV|SF_FORMAT_PCM_32|SF_FORMAT_FLOAT;
	
		int mixdownfilenum=0;
		int fileopensuccess=0;
		do {
			string* mixdownfilename=new string("");
			*mixdownfilename+=*job->m_projectdir;
			string* currsongname=job->sourcesong()->songname();
			*mixdownfilename+=*currsongname;
			delete currsongname;
			*mixdownfilename+="_mixdown";		
			if (mixdownfilenum>0)
			{
				*mixdownfilename+="_";	
				string* mixnumstr=Convert::int2str(mixdownfilenum);
				*mixdownfilename+=*mixnumstr;
				delete mixnumstr;
			}
			*mixdownfilename+=".wav";
			if (!hd24utils::fileExists(mixdownfilename->c_str()))
			{
				mixdownfile->open(mixdownfilename->c_str(),SFM_WRITE,&infoblock,soundfile);
				fileopensuccess=1;
			}
			mixdownfilenum++;
			delete mixdownfilename;
		} while (fileopensuccess==0);
	}


	uint32_t samplesinfirstblock=samplesperlogicalchannel;
	if (job->startoffset()!=0) {
		if (job->startoffset()>=samplesperlogicalchannel) {
			samplesinfirstblock=samplesperlogicalchannel-(job->startoffset()%samplesperlogicalchannel);
		} else {
			samplesinfirstblock=samplesperlogicalchannel-(job->startoffset());
		}
	}
	#if (HD24TRANSFERDEBUG==1) 
	cout << "go to startoffset " << job->startoffset() << endl;
	#endif
	job->sourcesong()->golocatepos(job->startoffset());
#if (HD24TRANSFERDEBUG==1) 
	cout << "got to startoffset " << job->startoffset() << endl
	 << "translen= " << translen << endl;
#endif
	uint32_t bytesperlogicalchannel=samplesperlogicalchannel*bytespersam;
	uint32_t samsincurrblock=samplesinfirstblock;
	if (translen<samplesperlogicalchannel) 
	{
		samplesinfirstblock=translen;
	}

	int64_t difseconds=0;
	int64_t olddifseconds=0;
	uint64_t currbytestransferred=0;

	for (uint32_t samplenum=0;
		samplenum<translen;
		samplenum+=samsincurrblock)
	{
		if (ui!=NULL)
		{
			if (((HD24UserInterface*)ui)->transfer_cancel==1) 
			{
				break;
			}
		}
		uint32_t subblockbytes=bytesperlogicalchannel;	
		if (translen==samplesinfirstblock) 
		{
	#if (HD24TRANSFERDEBUG==1) 
	cout << "translen==samplesinfirstblock" << endl;
			subblockbytes=translen*bytespersam;
			samsincurrblock=samplesinfirstblock;		
	#endif	
		} else {
			if (samplenum==0) {
				samsincurrblock=samplesinfirstblock;
			} 
			else 
			{
				samsincurrblock=samplesperlogicalchannel;
			}
		
			if (samplenum+samsincurrblock>=translen) 
			{
				subblockbytes=((translen-samplesinfirstblock) % samplesperlogicalchannel ) *bytespersam;
			} else {	
				if (samsincurrblock!=samplesperlogicalchannel) 
				{
					subblockbytes=samsincurrblock*bytespersam;
				}
			}		
		}

	#if (HD24TRANSFERDEBUG==1) 
	  cout << "samplenum=" << samplenum << ", sams in block=" << samsincurrblock << endl; 
	#endif	
		
		int skipsams=job->sourcesong()->getmtrackaudiodata(samplenum+job->startoffset(),samsincurrblock,&audiodata[0],hd24song::READMODE_COPY);	
		unsigned char* whattowrite=&audiodata[0];
		if (mustdeinterlace==1) 
		{
			job->sourcesong()->deinterlaceblock(&audiodata[0],&deinterlacedata[0]);
			whattowrite=&deinterlacedata[0];
		}
	
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "check for split" << endl;
	#endif	
		if (job->wantsplit==1) 
		{	
			uint64_t filesize=partsamcount;
			filesize+=samsincurrblock;
			filesize*=bytespersam;
			if (filesize>MAXBYTES) /// total filesize reached for current part
			{ 
				closeoutputfiles((hd24sndfile**)&filehandle[0],logical_channels);
				partnum++;
				openoutputfiles((hd24sndfile**)&filehandle[0],logical_channels,partnum,prefix); // partnum is for splitting large files
				partsamcount=0;
			}
		}

		// check if we need to save a mixdown
		if (mustmixdown)
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "Mixing " << endl;
#endif									
			
			for (uint32_t tracknum=0;tracknum<logical_channels;tracknum++) 
			{
				unsigned char* currsam=&whattowrite[tracknum*bytesperlogicalchannel
						 +((mustdeinterlace+1)*skipsams*bytespersam)];
				for (uint32_t samnum=0;samnum<subblockbytes;samnum+=3)
				{
					float subsamval=currsam[samnum+0]
						  +(currsam[samnum+1]<<8)
						  +(currsam[samnum+2]<<16);
					
					if (subsamval>=(1<<23)) {
						subsamval-=(1<<24);
					}
					subsamval=(subsamval/(double)0x800000);				
					mixerchannel[tracknum]->sample(samnum/3,subsamval);
				}
			}
			transfermixer->mix(subblockbytes/3);
	
			if (outputBuffer!=NULL) 
			{		
				// if stereo output, interlace (TODO: check if this is what we want)
				for (uint32_t i=0;i<(subblockbytes/3);i++) 
				{
					((float*)outputBuffer)[i*2] = transfermixer->masterout(0,i); // left 
					((float*)outputBuffer)[i*2+1] = transfermixer->masterout(1,i); // right	
				}
				mixdownfile->write_float(outputBuffer,(subblockbytes/3)*infoblock.channels);
			}		
		}
		// when mixing down, let's not export raw files as well.
					
		for (uint32_t tracknum=0;tracknum<logical_channels;tracknum++) 
		{	
			if (job->trackselected[tracknum]==0) 
			{
				// track not selected for export
#if (HD24TRANSFERDEBUG==1) 
				cout << "Track " << tracknum+1 << " not selected for export " << endl;
#endif					
				continue; 
			}

			if (ui!=NULL)
			{
				if (((HD24UserInterface*)ui)->transfer_cancel==1)
				{
#if (HD24TRANSFERDEBUG==1) 
					cout << "Transfer cancelled by user. " << endl;
#endif									
					break;
				}
			}
		
			// Mono files or multiple tracks?
			if (
				(isfirstchanofgroup[tracknum]) 
				&&(islastchanofgroup[tracknum]) 
			) {
				// file is mono
				if (!mustmixdown) {
	   			   writerawbuf(filehandle[tracknum],&whattowrite[tracknum*bytesperlogicalchannel+((mustdeinterlace+1)*skipsams*bytespersam)],subblockbytes);
				}
				currbytestransferred+=subblockbytes;
			} else {
				if (isfirstchanofgroup[tracknum]) {
				// first channel of group, clear interlace buffer
					trackwithingroup=0;
#if (HD24TRANSFERDEBUG==1) 
	 cout << "first track " << samplenum << endl;
#endif				
				} else {
					trackwithingroup++;
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "nonfirst track " << samplenum << endl;
	#endif							
				}
				// interlace channel onto multi channel file buffer
				if (!mustmixdown) {
					hd24utils::interlacetobuffer(&whattowrite[tracknum*bytesperlogicalchannel+((mustdeinterlace+1)*skipsams*bytespersam)],&interlacedata[0],subblockbytes,bytespersam,trackwithingroup,trackspergroup);
				}
		
				if (islastchanofgroup[tracknum]) {
					// last channel of group, write interlace buffer to file
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "write track " << samplenum << endl;
	#endif							
					//soundfile->sf_write_raw(filehandle[tracknum],&interlacedata[0],subblockbytes*trackspergroup); 
					if (!mustmixdown) {
						writerawbuf(filehandle[tracknum],&interlacedata[0],subblockbytes*trackspergroup); 
					}
					currbytestransferred+=subblockbytes*trackspergroup;
				}
			}
		}
		partsamcount+=(subblockbytes/bytespersam);
	//	totsamcount+=subblockbytes;
	
		uint32_t pct; // to use for display percentage
		double dblpct; // to use for ETA calculation
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "about to calc pct" << endl
	  << "totbytestransferred=" << totbytestransferred
	  << ", currbytestransferred=" << currbytestransferred
	  << ", totbytestotransfer=" << totbytestotransfer
	  << endl;
	#endif	
	
	/* following two calculations give the same mathematical result
	   but help keep the required word length narrow for large numbers
	   and reduce floating point rounding errors for small numbers.
	*/
	if (tottranslen<100000)
	{
		dblpct=(
			    (
			        (
				    (double)totbytestransferred
   				    +(double)currbytestransferred
			        )
			        *(double)100
			    )
			/
		            (double)totbytestotransfer
			);	
	}
	else
	{
		dblpct=(
			    (
				(double)totbytestransferred
				+(double)currbytestransferred
			    )
			/
			    (double)
			    (
				(double)totbytestotransfer
				/
				(double)100
			    )			    
			);		
	}
	pct=(uint32_t)(dblpct);
	
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "about to calc pct2" << endl;
	#endif	
	
	
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "computed pct " << samplenum << endl;
	#endif
	
	
	
	
		time (&endtime);
		olddifseconds=difseconds;
		difseconds=(long long int)difftime(endtime,transferstarttime);
	#if (HD24TRANSFERDEBUG==1) 
		cout << "difseconds=" << difseconds << endl
		 << "olddifseconds=" << olddifseconds << endl;
	#endif	
		if ( 
	   	    ((dblpct-oldpct)>=1)  ||
		    ((difseconds-olddifseconds)>=1)
		)
		{			
			/* update on percentage change, and also every
			   so many samples (to allow frequent enough 
		           screen updates with large audio files) */
			oldpct=dblpct;

			string* pctmsg=new string("Transferring audio to PC... ");
			if (totsongs>1)
			{
				*pctmsg+="Song ";
				string* cursongnum=Convert::int2str(songnum);
				*pctmsg+=*cursongnum+"/";			
				delete cursongnum;
				string* totsongnum=Convert::int2str(totsongs);
				*pctmsg+=*totsongnum+", ";			
				delete totsongnum;
			}
			string* strpct=Convert::int2str(pct);
			*pctmsg+=*strpct+"%";
			
			if (pct>0)
			{
				long long int tdifseconds=(long long int)difftime(endtime,transferstarttime);		
				uint32_t estimated_seconds=(uint32_t)((tdifseconds*100)/dblpct);
				uint32_t remaining_seconds=estimated_seconds-tdifseconds;

				lastremain=remaining_seconds;
				
				uint32_t seconds=(lastremain%60);
				uint32_t minutes=(lastremain-seconds)/60;
				uint32_t hours=0;
				
				*pctmsg+=" Time remaining: ";

				if (minutes>59) {
					hours=(minutes/60);
					minutes=(minutes%60);
					string* strhours=Convert::int2str(hours,2,"0");
					*pctmsg+=*strhours;
					*pctmsg+=":";
					delete(strhours);				
				}
	
				string* minsec=Convert::int2str(minutes,2,"0");
		
				*minsec+=":";
			
				string* strsecs=Convert::int2str(seconds,2,"0");
				*minsec+=*strsecs;
				delete(strsecs);
					
				*pctmsg+=*minsec;
				delete(minsec);				
			}
			
			delete (strpct);
			setstatus(ui,pctmsg,dblpct);
			delete pctmsg;
		}			
	}
	
	closeoutputfiles((hd24sndfile**)&filehandle[0],logical_channels);
	for (int i=0;i<MAXPHYSICALCHANNELS;i++) {
		if (filehandle[i]!=NULL)
		{
			delete filehandle[i];
		}
	}
	// from now on we'll read all tracks again
	for (uint32_t i=1;i<=MAXPHYSICALCHANNELS;i++)
	{
		if (job->trackselected[i-1]==1) 
		{
			job->sourcesong()->readenabletrack(i,true);
		}
	}
	if (audiodata!=NULL)
	{
		memutils::myfree("audiodata",audiodata);
	}
	if (deinterlacedata!=NULL)
	{
		memutils::myfree("deinterlacedata",deinterlacedata);
	}
	if (interlacedata!=NULL)
	{
		memutils::myfree("interlacedata",interlacedata);
	}
	if (outputBuffer!=NULL)
	{
		memutils::myfree("outputBuffer",outputBuffer);

	}
	if (mixdownfile!=NULL)
	{
		mixdownfile->close();
	}
	if (transfermixer!=NULL) {
		transfermixer->samplerate(oldmixersamplerate);
	}

	return currbytestransferred;
}

void hd24transferengine::selectedformat(int i)
{
	if (i>=formatcount) return;
	job->selectedformat(i);
}
void hd24transferjob::selectedformat(int i)
{
	m_selectedformat=i;
}

int hd24transferengine::selectedformat()
{
	return job->selectedformat();
}
int hd24transferjob::selectedformat()
{
	return m_selectedformat;
}

void hd24transferengine::mixer(MixerControl* m_mixer)
{
	this->transfermixer=m_mixer;
}

MixerControl* hd24transferengine::mixer()
{
	return this->transfermixer;	
}

string* hd24transferengine::generate_filename(int tracknum,int partnum,int prefix)
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferengine::generate_filename(track=" << tracknum << ",part="<<partnum<<",prefix="<<prefix<<endl
	 << "Selected format="<<selectedformat() << endl
	 << "Job="<<job<<endl
	 << "Projdir="<<job->m_projectdir<<endl;
#endif
	if (job->m_projectdir==NULL)
	{
#if (HD24TRANSFERDEBUG==1) 
		cout << "No project dir set!" << endl;
#endif
		return NULL;
	}
	const char* pdir=job->m_projectdir->c_str(); //projectdir();
#if (HD24TRANSFERDEBUG==1) 
	cout << "Projdir (const char*)="<<pdir << endl;
#endif

	if (pdir==NULL)
	{
#if (HD24TRANSFERDEBUG==1) 
		cout << "No project dir set!" << endl;
#endif
		return NULL;
	}
	string* dirname=new string(pdir);
	if (dirname->substr(dirname->length()-1,1)!="/") {
		*dirname+="/";
	}
#if (HD24TRANSFERDEBUG==1) 
	cout << "filename so far is just dir with slash=" << *dirname << endl;
#endif
	/* Now add the rest of the filename according to output file format */
	string* fnameformat=filenameformat();
#if (HD24TRANSFERDEBUG==1) 
	cout << "filenameformat is: " << *fnameformat << endl;
	if (strdatetime==NULL)
	{
		generatetimestamp();
	}
	cout << "timestamp is:" << *strdatetime << endl; // PRAGMA allowed
#endif
	if (fnameformat==NULL)
	{
		fnameformat=new string("");
	}
	if (strcmp(fnameformat->c_str(),"")==0)
	{
		/* specified filename format is empty, use program defaults */
		/* Proj name-Song Name-Track01 */
		*fnameformat+="<pn>-<sn>-Track<t#>";
	}
	hd24song* genfilesong=job->sourcesong();
///////////////////////////////////////////////////////////////

	string* fname=new string(dirname->c_str());
	delete dirname;
	
	int formatptr=0;
	int formatlen=strlen(fnameformat->c_str());
	int needprefix=1;
	
	while (formatptr<formatlen)
	{
		if (fnameformat->substr(formatptr,1)!="<")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add 1 char" << endl;
#endif
			/* No special format codes found at this pointer
			   position
			*/
			*fname+=fnameformat->substr(formatptr,1);
			formatptr++;
			continue;
		}
		
		if (fnameformat->substr(formatptr,6)=="<time>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add time" << endl;
#endif						
			if (strdatetime!=NULL)
			{
				*fname+=*strdatetime;
			}
			formatptr+=6;
			continue;
		}
		if (fnameformat->substr(formatptr,5)=="<vol>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add vol" << endl;
#endif			
			string* volname=genfilesong->parentfs->volumename();
			*fname+=*volname;
			delete volname;
			
			formatptr+=5;
			continue;			
		}
		
		if (fnameformat->substr(formatptr,4)=="<pn>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add projname" << endl;
#endif			
			string* projname=genfilesong->parentproject->projectname();
			*fname+=*projname;
			delete projname;
			
			formatptr+=4;
			continue;
		}
		if (fnameformat->substr(formatptr,4)=="<sn>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add songname" << endl;
#endif						
			string* songname=genfilesong->songname();
			*fname+=*songname;
			delete songname;
			formatptr+=4;
			continue;
		}
		if (fnameformat->substr(formatptr,4)=="<p#>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add projid" << endl;
#endif						
			int genprojid=genfilesong->parentproject->projectid();
			string* strprojnum=Convert::int2str(genprojid,2,"0");
			*fname+=*strprojnum;
			delete strprojnum;
			
			formatptr+=4;
			continue;
		}
		if (fnameformat->substr(formatptr,4)=="<s#>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add songid" << endl;
#endif						
			int gensongid=genfilesong->songid();
			string* strsongnum=Convert::int2str(gensongid,2,"0");
			*fname+=*strsongnum;
			delete strsongnum;
			
			needprefix=0;
			formatptr+=4;
			continue;
		}
		if (fnameformat->substr(formatptr,4)=="<t#>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add tracknum" << endl;
#endif						
			string* strtracknum=Convert::int2str(tracknum+1,2,"0");
			*fname+=*strtracknum;
			delete strtracknum;
			
			formatptr+=4;
			continue;
		}
		if (fnameformat->substr(formatptr,8)=="<t#+off>")
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "add tracknum+off" << endl;
#endif						
			string* strtracknum=Convert::int2str(
				tracknum+1+reeloffset(),2,"0"
			);
			*fname+=*strtracknum;
			delete strtracknum;
			
			formatptr+=8;
			continue;
		}
		
		/* Unknown format code. Skip it. */
#if (HD24TRANSFERDEBUG==1) 
		cout << "unknown format code"
		     << fnameformat->substr(formatptr,3)
		     << endl;
#endif					
		while (fnameformat->substr(formatptr,1)!=">")
		{
			formatptr++;
			if (formatptr==formatlen)
			{
				break;
			}
		}
		formatptr++;
	}
	
	/* only add prefix if song num is not included in format */
        if ((prefix!=0) && (needprefix==1))
        {
		string* strsongnum=Convert::int2str(prefix,2,"0");
		string* newfname=new string(fname->c_str());
		delete fname;
		fname=new string("Song");
		*fname+=*strsongnum;        
		*fname+="-";
		delete (strsongnum);
		*fname+=*newfname;
		delete newfname;
        }
#if (HD24TRANSFERDEBUG==1) 
	cout << "filename so far is " << *fname << endl
	 << "About to add songname" << endl;
#endif
	if (job->sourcesong()==NULL)
	{
#if (HD24TRANSFERDEBUG==1) 
		cout << "No song set!" << endl;
#endif
		return NULL;
	}
	
	
	/* file name was generated. Do not clean up filename format string
	   because it will automatically be cleaned on object destruction */

	
////////////////////////////////////////////////////////////////////////	
	if (partnum>0) {
		/* partnum 0 implies no splitting and thus no partnum 
		   added to the filename. In other words, we either use
		   partnum 0 (invisible in filename), or partnum 1..n. */
		*fname+="_part";
		*fname+=*Convert::int2str(partnum);
	}
	string* extension=new string((*m_format_outputextension)[selectedformat()]);
	*fname+=*extension;
	delete extension;
#if (HD24TRANSFERDEBUG==1) 
	cout << "Generated filename="<<fname<<endl;
#endif
	return fname;
}

void hd24transferjob::sizelimit(int64_t p_sizelimit)
{
	this->llsizelimit=p_sizelimit;
}

int64_t hd24transferjob::sizelimit()
{
	return this->llsizelimit;
}

void hd24transferengine::sizelimit(int64_t p_sizelimit)
{
	job->sizelimit(p_sizelimit);
}

int64_t hd24transferengine::sizelimit()
{
	return job->sizelimit();
}

bool hd24transferengine::anyfilesexist(hd24song* thesong) 
{
	bool anyexist=false;
	struct stat fi;
	uint32_t channels=thesong->logical_channels();
	for (unsigned int q=0; q<100; q++)
	{
		for (unsigned int handle=0;handle<channels;handle++) 
		{
			if (job->trackselected[handle]==0) 
			{
				// channel not selected for export
				continue;
			}
			string* fname=this->generate_filename(handle,0,q);
			if (fname==NULL)
			{
				// cannot generate filename (no song set?)
				break;
			}
#if (HD24TRANSFERDEBUG==1) 
			cout << "check if file exists:" << *fname << endl; 
#endif    
        		if ((stat (fname->c_str(), &fi) != -1) && ((fi.st_mode & S_IFDIR) == 0)) 
			{
				anyexist=true;
#if (HD24TRANSFERDEBUG==1) 
  cout << "yes, exists" << *fname << endl; 
#endif            
				delete fname;
            			break;
			}
    
			fname=this->generate_filename(handle,1,q);
			if (fname==NULL)
			{
				// cannot generate filename (no song set?)
				break;
			}
#if (HD24TRANSFERDEBUG==1) 
			cout << "check if file exists:" << *fname << endl; 
#endif    
			if ((stat (fname->c_str(), &fi) != -1) && ((fi.st_mode & S_IFDIR) == 0)) 
			{
            			anyexist=true;
#if (HD24TRANSFERDEBUG==1) 
  cout << "yes, exists" << *fname << endl; 
#endif            
				delete fname;
				break;
        		}    
#if (HD24TRANSFERDEBUG==1) 
			cout << "no, doesnt exist" << endl;
#endif            

			delete fname;
		}  
		if (anyexist==true) break;
	}
	return anyexist;
}

void hd24transferengine::setstatus(void* ui,string* message,double percentage)
{	
#if (HD24TRANSFERDEBUG==1)
	cout << message->c_str() << endl;
	if (ui==NULL)
	{
		cout << "WARNING-- no ui defined, transfer status not updating in GUI" << endl;// PRAGMA allowed
	}	
#endif
	if (setstatusfunction!=NULL)
	{
		setstatusfunction(ui,message->c_str(),percentage);
	}
	return;
}

void hd24transferengine::set_ui(void* p_ui)
{
	this->ui=p_ui;
}

void hd24transferengine::sourcesong(hd24song* song)
{
#if (HD24TRANSFERDEBUG==1) 
	cout << "hd24transferengine::sourcesong(" << song << ")" << endl;
#endif
	if (this->job==NULL)
	{
		return;
	}
	this->job->sourcesong(song);
}

hd24song* hd24transferengine::sourcesong()
{
#if (HD24TRANSFERDEBUG==1) 
	cout 	<< "hd24transferengine::sourcesong()="
		<< this->job->sourcesong()<< endl;
#endif
	if (this->job==NULL)
	{
		return NULL;
	}
	return this->job->sourcesong();
}

void hd24transferengine::startoffset(uint32_t newoff)
{
	job->startoffset(newoff);	
}

void hd24transferengine::endoffset(uint32_t newoff)
{
	job->endoffset(newoff);
}

uint32_t hd24transferengine::startoffset()
{
	return job->startoffset();
}

uint32_t hd24transferengine::endoffset()
{
	return job->endoffset();
}

void hd24transferengine::targetsong(hd24song* song)
{
	if (job==NULL)
	{
		return;
	}
	job->targetsong(song);
}

hd24song* hd24transferengine::targetsong()
{
	if (job==NULL)
	{
		return NULL;
	}
	return job->targetsong();
}

uint32_t hd24transferengine::requiredsonglength_in_wamples(hd24song* tsong,SF_INFO* sfinfoin)
{
	/* a 10 ksample 96k song requires only 5k wamples (word-samples)
	   because the each wample is 2 samples interleaved across 2 tracks. */
	
	uint32_t maxlen_in_wamples=tsong->songlength_in_wamples();
	
	for (int i=0;i<MAXPHYSICALCHANNELS;i++) {
		if (job->filehandle[i]==NULL) continue;
		int64_t framecount=(int64_t)sfinfoin[i].frames;
		if (tsong->chanmult()==2)
		{
			if ((framecount%2)==1)
			{
				framecount++;
			}
			framecount/=2;
		}

		if (((int64_t)framecount) > (int64_t)maxlen_in_wamples) 
		{			
			maxlen_in_wamples=(uint32_t)framecount;
		}
	}
	
	return maxlen_in_wamples;
}
/*
void hd24transferengine::_generate_smpte(uint32_t wamplesperlogicalchannel,uint32_t wamsincurrblock,uint32_t samplenum,unsigned char* audiodata)
{
	hd24song* tsong=job->targetsong(); // should exist as was verified by transfer_to_hd24()
	if (tsong==NULL) return; // just in case it's destructed+cleared.

	uint32_t bytespersam=(tsong->bitdepth()/8);
	uint32_t logical_channels=tsong->logical_channels();
	//  Fill audio buffer for tracks that need SMPTE striping 
	for (uint32_t tracknum=0;tracknum<logical_channels;tracknum++) 
	{
		if (!(tsong->trackarmed(tracknum+1)))
		{
			// track not selected for export
			continue; 
		}
		int action=job->trackaction(tracknum+1);
		if (action!=1)
		{
			// track not marked for smpte striping
			continue;
		}
		// TODO: samsincurrblock instead of samplesperlogicalchannel?
		uint32_t firstbyte=tracknum*samplesperlogicalchannel*bytespersam;
		uint32_t bytenum=firstbyte;

		for (uint32_t samnum=0;samnum<samsincurrblock;samnum++) {
			// smpte stripe:
			uint32_t samval=((job->smptegen->getbit(samplenum+samnum)*2)-1)*2000000;
			
			audiodata[bytenum]=(unsigned char)samval & 0xff;
			audiodata[bytenum+1]=(unsigned char)(samval>>8) & 0xff;
			audiodata[bytenum+2]=(unsigned char)(samval>>16) & 0xff;
			bytenum+=3;
		}
	}
}
*/

void hd24transferengine::_generate_silence(uint32_t wamplesperlogicalchannel,uint32_t wamsincurrblock,uint32_t wamplenum,unsigned char* audiodata)
{
	// TODO: join this with _generate_smpte?
	hd24song* tsong=job->targetsong(); // should exist as was verified by transfer_to_hd24()
	if (tsong==NULL) return; // just in case it's destructed+cleared.

	uint32_t bytesperwam=(tsong->bitdepth()/8)*tsong->chanmult();
	uint32_t logical_channels=tsong->logical_channels();
	/* Fill audio buffer for tracks that need SMPTE striping */
	for (uint32_t tracknum=0;tracknum<logical_channels;tracknum++) 
	{
		if (!(tsong->trackarmed(tracknum+1)))
		{
			// track not selected for export
			continue; 
		}
		int action=job->trackaction(tracknum+1);
		if (action!=0)
		{
			// track not marked for silencing
			continue;
		}
		// TODO: samsincurrblock instead of samplesperlogicalchannel?
		// No clever (de)interlacing here, as we're doing silence only.
		uint32_t firstbyte=tracknum*wamplesperlogicalchannel*bytesperwam;
		uint32_t bytenum=firstbyte;
		for (uint32_t wamnum=0;wamnum<wamsincurrblock;wamnum++) {
			audiodata[bytenum]=0;
			audiodata[bytenum+1]=0;
			audiodata[bytenum+2]=0;
			if (bytesperwam>3)
			{
			audiodata[bytenum+3]=0;
			audiodata[bytenum+4]=0;
			audiodata[bytenum+5]=0;
			}
			bytenum+=bytesperwam;
		}
	}
}
double hd24transferengine::update_eta(const char* etamessage,uint64_t translen,
				uint64_t currbytestransferred,
				uint64_t totbytestransferred,
				uint64_t totbytestotransfer,double oldpct)
{
	/* TODO: We could probably store the numeric info in the job object
	   and use this method for transfers in both directions. */	
	double dblpct; // to use for ETA calculation
	uint32_t lastremain=0xffffffff; //a		
	int64_t difseconds=0;
	int64_t olddifseconds=0;

#if (HD24TRANSFERDEBUG==1) 
 cout << "about to calc pct" << endl
  << "totbytestransferred=" << totbytestransferred
  << ", currbytestransferred=" << currbytestransferred
  << ", totbytestotransfer=" << totbytestotransfer
  << endl;
#endif	

#if (HD24TRANSFERDEBUG==1) 
 cout << "about to calc pct2" << endl;
#endif	
	if (translen<100000) {
		dblpct=(((totbytestransferred+currbytestransferred)*100)/(totbytestotransfer));
	} else {
		dblpct=((totbytestransferred+currbytestransferred)/(totbytestotransfer/100));
	}
	oldpct=dblpct;
	
	uint32_t pct=(uint32_t)dblpct;

#if (HD24TRANSFERDEBUG==1) 
	cout << "pct=" << pct << " oldpct="<<oldpct<<endl
	 << "dblpct=" << dblpct <<endl;
#endif
	time (&endtime);
	olddifseconds=difseconds;
	difseconds=(long long int)difftime(endtime,transferstarttime);
#if (HD24TRANSFERDEBUG==1) 
	cout << "difseconds=" << difseconds << endl
	<< "olddifseconds=" << olddifseconds << endl;
#endif	
	if ( 
   	    ((dblpct-oldpct)>=1)  ||
	    ((difseconds-olddifseconds)>=1)
	)
	{
		/* update on percentage change, and also every
		   so many samples (to allow frequent enough 
	           screen updates with large audio files) */
		oldpct=pct;

		string* pctmsg=new string(etamessage);
		// TODO: Estimated time remaining goes here			
		string* strpct=Convert::int2str(pct);
		*pctmsg+=*strpct+"%";
                delete strpct;
		
		
		if (pct>0)
		{
			long long int tdifseconds=(long long int)difftime(endtime,transferstarttime);		
			uint32_t estimated_seconds=(uint32_t)((tdifseconds*100)/dblpct);
			uint32_t remaining_seconds=estimated_seconds-tdifseconds;

			lastremain=remaining_seconds;
			
			uint32_t seconds=(lastremain%60);
			uint32_t minutes=(lastremain-seconds)/60;
			uint32_t hours=0;
			
			*pctmsg+=" Time remaining: ";

			if (minutes>59) {
				hours=(minutes/60);
				minutes=(minutes%60);
				string* strhours=Convert::int2str(hours,2,"0");
				*pctmsg+=*strhours;
				*pctmsg+=":";
				delete(strhours);				
			}

			string* minsec=Convert::int2str(minutes,2,"0");
	
			*minsec+=":";
		
			string* strsecs=Convert::int2str(seconds,2,"0");
			*minsec+=*strsecs;
			delete(strsecs);
				
			*pctmsg+=*minsec;
			delete(minsec);				
		}	

		setstatus(ui,pctmsg,dblpct);
		delete pctmsg;
	}
	return dblpct;
}

void hd24transferengine::_prepare_audio(uint32_t wamplesperlogicalchannel,
					uint32_t wamsincurrblock,
					uint32_t wamplenum,
					unsigned char* audiodata,
					SF_INFO* sfinfoin, int* sfeof
)
{
	hd24song* tsong=job->targetsong(); // should exist as was verified by transfer_to_hd24()
	if (tsong==NULL) return; // just in case it's destructed+cleared.
	uint32_t bytespersam=(tsong->bitdepth()/8);

	uint32_t samsread=0; // these come from file so they're actually samples rather than wamples.
	uint32_t logchans=tsong->logical_channels();
	uint32_t chanmult=tsong->chanmult();
	uint32_t halfchansize=0;
#if (HD24TRANSFERDEBUG==1)
	uint32_t physchans=tsong->physical_channels();
	cout << "logchans="<<logchans <<", physchans="<<physchans << endl;  // PRAGMA allowed
#endif
	if (chanmult==2)
	{
		halfchansize=wamplesperlogicalchannel*bytespersam;
#if (HD24TRANSFERDEBUG==1)
		cout << "HALFCHANSIZE=" << halfchansize << endl;
#endif
	}
	for (uint32_t logtracknum=0;logtracknum<logchans;logtracknum++) 
	{
		if (!(tsong->trackarmed(logtracknum+1)))
		{
			// track not selected for export
			continue; 
		}
		
		int action=job->trackaction(logtracknum+1);
		if (action<2)
		{
			// track not marked as read-from-file audio
			continue;
		}
		
		if (job->filehandle[logtracknum] == NULL) {
			// no filehandle for current logical channel
			continue;
		}
		
		/* trackchans is number of channels in file.
		   if our block is 10 samples long and we read from a stereo file,
		   even if we only need the left channel, we'll need to read 10 samples
		   *2 channels because audio files will be interlaced.
		   For high samplerate audio, we use twice the HD24 channels and thus
		   need to read twice as many samples from audio file. */

		uint32_t trackchans=sfinfoin[logtracknum].channels;
		
		if (sfeof[logtracknum]==1)
		{
			samsread=0;
/*
			uint32_t blen=wamsincurrblock*trackchans*chanmult*sizeof(int);
			for (uint32_t silentsam=0;silentsam<blen;silentsam++)
			{
				audiobuf[logtracknum][silentsam]=0;
			}
*/
		}
		else
		{
			samsread=soundfile->sf_read_int(
				job->filehandle[logtracknum],
				audiobuf[logtracknum],
				wamsincurrblock*trackchans*chanmult);
		}
		#if (HD24TRANSFERDEBUG==1)
			cout << "sams read="
			<< samsread
			<< endl;
		#endif
		
		/* Clear nonempty bytes not read from file */
		if (samsread<(wamsincurrblock*trackchans*chanmult))
		{
			sfeof[logtracknum]=1;				
			// TODO: clear rest of block (or should we?)
/*
			uint32_t bstart=samsread*trackchans*chanmult*sizeof(int);
			uint32_t blen=wamsincurrblock*trackchans*chanmult*sizeof(int);
			for (uint32_t silentsam=bstart;silentsam<blen;silentsam++)
			{
				audiobuf[logtracknum][silentsam]=0;
			}
*/
		}

		/*
			Now either select a track or mixdown to mono
		*/
		uint32_t firstbyte=logtracknum*wamsincurrblock*bytespersam*chanmult;
		uint32_t whichbyte=firstbyte;
	
		int samval;
		uint32_t maxcount=wamsincurrblock*trackchans*chanmult;
		int evenodd=0;	
#if (HD24TRANSFERDEBUG==1)
		cout << "Writing file audio to HD24 buffer, "
			<<"sam count=" <<maxcount 
			<<"trackchans (file)=" <<trackchans
			<< endl;
#endif
		for (unsigned int sam=0;sam<maxcount;sam+=trackchans)
		{
			int whichsam=sam;
			int* buf=audiobuf[logtracknum];
			if (buf==NULL) break;
			samval=0;
			if (action==2)
			{
				/* (mixdown to) mono */
				if (sam<samsread)
				{
					/* we still have more samples*/
					if (trackchans==1)
					{
						// use the only track
						samval=(buf[whichsam])/256;
					}
					else
					{
						// mixdown multi to mono
						for (unsigned int whichchan=0;whichchan<trackchans;whichchan++) {
							samval+=(buf[whichsam+whichchan])/256;
						}
						// TODO: clip handling
						samval/=trackchans;
					}
				} 				
			}
			else
			{
				/* use only the selected track */
				if (sam<samsread)
				{
					samval = (buf[whichsam+(action-3)])/256;
				}
			}
#if (HD24TRANSFERDEBUG==1)
#if (SHOWAUDIODATA==1)
			cout << "whichsam=" << whichsam << " " 
			<< "buf[" << logtracknum*samsincurrblock+(whichsam*bytespersam) << "]=";
			<< (samval & 0xff) << " " << ((samval>>8) & 0xff) << " " << ((samval>>16) & 0xff) <<"//";
#endif				
#endif	

			int offs=(evenodd*halfchansize);
#if (HD24TRANSFERDEBUG==1)
			   cout << "put audio data "<< samval <<" at " << whichbyte+offs+0 << endl;
#endif
			audiodata[whichbyte+offs+0]=(unsigned char)samval & 0xff;
			audiodata[whichbyte+offs+1]=(unsigned char)(samval>>8) & 0xff;
			audiodata[whichbyte+offs+2]=(unsigned char)(samval>>16) & 0xff;
			if (chanmult==2)
			{
				whichbyte+=(evenodd*bytespersam);
				evenodd=1-evenodd;
			} else {
				whichbyte+=bytespersam;
			}
		} // end for (unsigned int sam=0;sam<maxcount;sam+=trackchans)
	} // end for (uint32_t logtracknum=0;logtracknum<logical_channels;logtracknum++)
}

uint32_t hd24transferengine::_lengthen_song_as_needed(hd24song* tsong,SF_INFO* sfinfoin)
{
/////////////// START: ALLOCATE AUDIO SPACE ON DRIVE ///////////////
	// Find file with largest number of samples;
	// if song is shorter than that, lengthen song to fit it.
	// Note: if nonzero start offset is specified, add this offset 
	// to each file length.

	uint32_t maxlen_wamples=this->requiredsonglength_in_wamples(tsong,sfinfoin) ;
#if (HD24TRANSFERDEBUG==1) 
	cout << "required songlength in wamples is " << maxlen_wamples
		<< "=" << (maxlen_wamples*tsong->chanmult()) 
		<<" samples." << endl
	 << "(already compensated for high samplerate mode as appropriate)" << endl;
#endif
	uint32_t translen_wamples=0; 
	if (maxlen_wamples > (tsong->songlength_in_wamples())) 
	{
		string* lengthening=new string("Lengthening song... ");
		setstatus(ui,lengthening,0);
		delete lengthening;
#if (HD24TRANSFERDEBUG==1) 
		cout << "about to start lengthening song to maxlen=" << maxlen_wamples << endl;
#endif
		bool clearnew=true; /* silence newly allocated part as not 
                                       all tracks may be set to erase/arm/overwrite */
		translen_wamples=tsong->songlength_in_wamples(maxlen_wamples,clearnew);
#if (HD24TRANSFERDEBUG==1) 
		cout << "verifying actual song length " << endl
		 << "new len=" << translen_wamples << " wamples." << endl;
#endif
		if (translen_wamples!=maxlen_wamples) 
		{
#if (HD24TRANSFERDEBUG==1) 
			cout << "new len<> maxlen so not enough space." << endl;
#endif
			this->lasterror("Not enough space on HD24 drive.");
			return 0;
		}
		tsong->save();	
		// ui_refresh("tohd24");
	} else {
		translen_wamples=tsong->songlength_in_wamples();
	}
#if (HD24TRANSFERDEBUG==1) 
	cout << "translen is now " << translen_wamples << "wamples." << endl;
#endif

	return translen_wamples;
}

int64_t hd24transferengine::transfer_to_hd24()
{
	if (soundfile==NULL)
	{
		lasterror("Soundfile library not loaded, transfer aborted");
		return 0;
	}
	hd24song* tsong=job->targetsong();
	if (tsong==NULL)
	{
		lasterror("No target song selected, transfer aborted.");
		return 0;
	}
///////// START: VARIOUS INIT /////////////////
	time(&transferstarttime); // for transfer speed statistics, ETA


	double oldpct=0; // to use for display percentage
	
	int64_t currbytestransferred=0;

	/* Even at high sample rates we null all 24 audio buffer pointers.
	   Later on we might only use 12 if we work with higher sample rates,
           but at this point we're not making that distinction yet.
           Strictly speaking the below should say 'MAXLOGICALCHANNELS' but
           that's not possible to define as a constant when sample rates 
           dictate what the actual number of logical channels is.
        */
 
	for (unsigned int currchan=0;currchan<MAXPHYSICALCHANNELS;currchan++) 
	{
		job->filehandle[currchan]=NULL;
	   	audiobuf[currchan]=NULL;
	}
////////// END: VARIOUS INIT //////////////

/////// START: OPEN INPUT FILES ///////////////
	SF_INFO sfinfoin[MAXPHYSICALCHANNELS];
	for (int currchan=0;currchan<MAXPHYSICALCHANNELS;currchan++)
	{
		sfinfoin[currchan].frames=0;
		sfinfoin[currchan].samplerate=0;
		sfinfoin[currchan].channels=0;
		sfinfoin[currchan].format=0;
		sfinfoin[currchan].sections=0;
		sfinfoin[currchan].seekable=0;
	}
	int logchans=tsong->logical_channels();

	SNDFILE** handlearray=(SNDFILE**)(&(job->filehandle[0]));
	bool canopen=this->openinputfiles(
			handlearray, (SF_INFO*)&sfinfoin[0],	logchans); 
	if (!canopen) 
	{
		lasterror("Problem opening input files, transfer aborted");
		return 0;
	}
/////// END: OPEN INPUT FILES ///////////////

	uint32_t newsonglen=this->_lengthen_song_as_needed(tsong,(SF_INFO*)&sfinfoin[0]);
	if (newsonglen==0)
	{
		/* either lenghtening failed or total transfer size equals zero.
		   cancel transfer.
		*/
		lasterror("Lengthening failed or zero transfer size- aborting transfer.");
		this->closeinputfiles((SNDFILE**)&(job->filehandle[0]),tsong->logical_channels());
		return 0;		
	}

//////// START: GET SONG/FS METRICS //////////////
	hd24fs* currenthd24=job->targetfs();
	int audioblocksizebytes=currenthd24->getbytesperaudioblock(); // typically 1152*512.

	// to hold normally read audio data:
	unsigned char* audiodata=NULL;
//	ui->stop_transfer->show();

	audiodata=(unsigned char*)memutils::mymalloc("button_transfertohd24",audioblocksizebytes,1);
	uint32_t logical_channels=tsong->logical_channels();
	uint32_t bytespersam=(tsong->bitdepth()/8);
	uint32_t samplesperlogicalchannel=(audioblocksizebytes/logical_channels)/bytespersam;
	uint32_t wamplesperlogicalchannel=(samplesperlogicalchannel/tsong->chanmult());

//////// END: GET SONG/FS METRICS //////////////

////// ENABLE RECORD MODE ////////////
	if (ui!=NULL)
	{
		// Init cancel button status to "not clicked"
		// this will allow us to detect cancel clicks
		((HD24UserInterface*)ui)->transfer_cancel=0;
	}

	tsong->golocatepos(0); // TODO: startoffset
	tsong->startrecord(hd24song::WRITEMODE_COPY);
/////// RECORD MODE ENABLED //////////

	// Create an smpte generator object. 
	// It won't slow things down if it turns out we don't use it.
	job->smptegen=new SMPTEgenerator(tsong->samplerate());

	///////// START: clear audio buffers, keep EOF status of all input files //////////
	int sfeof[MAXPHYSICALCHANNELS];	
	for (unsigned int fh=0;fh<logical_channels;fh++) {
		int action=job->trackaction(fh+1);
		sfeof[fh]=0;
		if ((job->filehandle[fh]!=NULL)||(action<=1 /* erase, SMPTE stripe */)) {
			uint32_t chans=((action==0)||(action==1))?(1):(sfinfoin[fh].channels);
			uint32_t bytestoalloc=chans*samplesperlogicalchannel*sizeof(int);
			audiobuf[fh]=(int*)memutils::mymalloc(
			"button_transfertohd24 audiobuf",bytestoalloc,1
			);

			if (action == 0) {
				// clear erase buffer once
				int* buf=audiobuf[fh];
				for (uint32_t samnum=0;samnum<samplesperlogicalchannel;samnum++) {
					buf[samnum]=0;
				}
			}
			if (action == 1) {
				// clear SMPTE buffer once (mostly for debugging purposes)
				int* buf=audiobuf[fh];
				for (uint32_t samnum=0;samnum<samplesperlogicalchannel;samnum++) {
					buf[samnum]=1;
				}
			}
			// if action==1, we need to repopulate the buffer while striping
		} else {
			audiobuf[fh]=NULL;
		}
	}
	///////// END: clear audio buffers, keep EOF status of all input files //////////

	// number of samples in current block- equal to block samples for full song
	// transfer
	uint32_t bytesperlogicalchannel=samplesperlogicalchannel*bytespersam;

	#if (HD24TRANSFERDEBUG==1)    
	 cout << "Log.Channels="<<logical_channels << endl; 
	#endif
	uint32_t startoffset=0;
	//uint32_t endoffset=currsong->songlength_in_samples();
	uint32_t wamplesinfirstblock=wamplesperlogicalchannel;

	tsong->getlocatepos(25); // 25= virtual endpoint of song
	// calc number of samples to transfer.
	// (for partial transfers, which are not supported at this time, the 
	// number of samples is based on (endoffset-startoffset) instead of songlength
	uint32_t translen_wamples=tsong->songlength_in_wamples();
	uint64_t totbytestotransfer=translen_wamples*tsong->chanmult()*bytespersam;
	//transfer_to_hd24();

	if (startoffset!=0) {
		if (startoffset>=wamplesperlogicalchannel) {
			wamplesinfirstblock=wamplesperlogicalchannel-(startoffset%wamplesperlogicalchannel);
		} else {
			wamplesinfirstblock=wamplesperlogicalchannel-startoffset;
		}
	}
	#if (HD24TRANSFERDEBUG==1) 
	cout << "go to startoffset " << startoffset << endl;
	#endif
	tsong->golocatepos(startoffset);
	#if (HD24TRANSFERDEBUG==1) 
	cout << "got to startoffset " << startoffset << "(wamples)" << endl
	 << "translen= " << translen_wamples << " wamples " << endl;
	#endif

	uint32_t wamsincurrblock=wamplesinfirstblock;
	if (translen_wamples<wamplesperlogicalchannel)
	{
		wamplesinfirstblock=translen_wamples;
	}

	uint64_t totbytestransferred=0; // only for multi song transfer- does not apply.
	//deactivate_ui();	
	for (uint32_t wamplenum=0;
		wamplenum<translen_wamples;
		wamplenum+=wamsincurrblock)
	{
		if (ui!=NULL)
		{
			if (((HD24UserInterface*)ui)->transfer_cancel==1) 
			{
				//ui_refresh("tohd24_cancel");
				break;
			}
		}
	#if (HD24TRANSFERDEBUG==1) 
	 cout << "wamnum= " << wamplenum << endl;
	#endif	
		uint32_t subblockbytes=bytesperlogicalchannel;
		if (translen_wamples==wamplesinfirstblock) 
		{
	#if (HD24TRANSFERDEBUG==1) 
	cout << "translen==samplesinfirstblock" << endl;
			subblockbytes=translen_wamples*bytespersam*tsong->chanmult();
			wamsincurrblock=wamplesinfirstblock;		
	#endif	
		} else {
			if (wamplenum==0) {
				wamsincurrblock=wamplesinfirstblock;
			} 
			else 
			{
				wamsincurrblock=wamplesperlogicalchannel;
			}
		
			if (wamplenum+wamsincurrblock>=translen_wamples) 
			{
				subblockbytes=
				(
					(translen_wamples-wamplesinfirstblock) 
					% wamplesperlogicalchannel 
				) * bytespersam
				  * tsong->chanmult();
			} else {	
				if (wamsincurrblock!=wamplesperlogicalchannel) 
				{
					subblockbytes=wamsincurrblock*bytespersam*tsong->chanmult();
				}
			}		
		}

	#if (HD24TRANSFERDEBUG==1) 
	  cout << "wamplenum=" << wamplenum << ", wams in block=" << wamsincurrblock << endl; 
	#endif	
	
		/* Fill audio buffer (for tracks that need silencing) */
		_generate_silence(wamplesperlogicalchannel,wamsincurrblock,wamplenum,&audiodata[0]);
		
		/* Fill audio buffer (for tracks that need SMPTE striping) */
//		_generate_smpte(wamplesperlogicalchannel,wamsincurrblock,wamplenum,&audiodata[0]);
		
		/* Process (mix-to-)mono audio tracks - Read audio */
		_prepare_audio(wamplesperlogicalchannel,wamsincurrblock,wamplenum,&audiodata[0],&sfinfoin[0],&sfeof[0]);
	
		/*
		 
		We have collected data to write for all tracks.
		Finally we write the audio. the hd24 library will make
		sure that only the relevant tracks will be overwritten
		and the rest are protected.
		
		*/

		#if ((HD24TRANSFERDEBUG==1) && (SHOWAUDIODATA==1))		
			cout << "1st 30 bytes of track: " << endl;
			for (unsigned int fbyte=0; fbyte<30; fbyte++)
			{
				cout << audiodata[(wamsincurrblock*tsong->chanmult())+fbyte]	<< " "; // PRAGMA allowed
			}
			cout << endl; // PRAGMA allowed
		#endif
		//int writesams=  (result was not used)
		// FIXME: putmtrackaudiodata still thinks it's working with samples instead of wamples.
		tsong->putmtrackaudiodata(
			(wamplenum+startoffset),
			wamsincurrblock,
			&audiodata[0],
			hd24song::WRITEMODE_COPY
		);	
	
		totbytestransferred+=(wamsincurrblock*bytespersam*tsong->chanmult());

		oldpct=update_eta("Transferring audio to HD24...",
			translen_wamples,currbytestransferred,totbytestransferred,
			totbytestotransfer,oldpct);  // recalculate/show progress
	} // end for (uint32_t samplenum=0; 	((uint64_t)samplenum)<translen_wamples;		samplenum+=samsincurrblock)

	if (job->smptegen!=NULL)
	{
		// TODO: move to job destructor?
		delete job->smptegen;
		job->smptegen=NULL;
	}

	if (audiodata!=NULL)
	{
	    memutils::myfree("audiodata",audiodata);
	    audiodata=NULL;
	}

	for (unsigned int bufnum=0;bufnum<MAXPHYSICALCHANNELS;bufnum++)
	{
		if (audiobuf[bufnum]!=NULL)
		{
			memutils::myfree("audiobuf transfer_to_hd24",audiobuf[bufnum]);
			audiobuf[bufnum]=NULL;
		}
	}

	// --------------End transfer_to_hd24----------------
	string* pctmsg=new string("Transferring audio to HD24... %d%%");
	// TODO: Estimated time remaining goes here
	setstatus(ui,pctmsg,100);
	delete pctmsg;

	tsong->stoprecord();
	tsong->unarmalltracks();

	this->closeinputfiles((SNDFILE**)&(job->filehandle[0]),tsong->logical_channels());
	#if (HD24TRANSFERDEBUG==1) 
		cout << "transfer complete" << endl;
	#endif	
	return totbytestransferred;
}
