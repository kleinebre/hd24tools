#define UTILDEBUG 1
#ifdef DARWIN
#	define creat64 creat
#	define open64 open
#	define lseek64 lseek
#	define pread64 pread
#	define pwrite64 pwrite
#endif

#if defined(LINUX) || defined(DARWIN)
#	define PRINTAPP "lp"
#endif

#ifdef WINDOWS
#	include <windows.h>
#	include <shellapi.h>
#	define PRINTAPP "print"
#	define popen _popen
#	define pclose _pclose
#define DIRSLASH "\\"
#else
#	include <unistd.h>

#define DIRSLASH "/"
#endif

#include <string>
#include <sys/types.h>
/* sys/type.h should include time.h, but it doesn't seem to on MinGW */
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>

#include <hd24devicenamegenerator.h>
#include <FL/FLTKstuff.H>
#include <FL/Fl_Preferences.H>
#include <FL/filename.H>
#include "convertlib.h"
#include "hd24utils.h"
#include "memutils.h"
#include <hd24sndfile.h>

#define _LARGE_FILES
#define LARGE_FILES
#define LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS		64
#define FILE_OFFSET_BITS		64
#define SECTORSIZE			512
#define FSINFO_VERSION_MAJOR 		0x8
#define FSINFO_VERSION_MINOR 		0x9
#define FSINFO_BLOCKSIZE_IN_SECTORS	0x10
#define FSINFO_AUDIOBLOCKS_PER_CLUSTER	0x14
#define FSINFO_STARTSECTOR_FAT		0x38
#define FSINFO_NUMSECTORS_FAT		0x3c
#define FSINFO_CLUSTERS_ON_DISK		0x44
#define FSINFO_MAXPROJECTS		0x50
#define FSINFO_MAXSONGSPERPROJECT	0x54
#define FSINFO_DATAAREA 		0x7c
#define DRIVEINFO_VOLUME 		0x1b8
#define DRIVEINFO_PROJECTCOUNT		0x0c
#define DRIVEINFO_LASTPROJ		0x10
#define DRIVEINFO_PROJECTLIST		0x20
const int hd24utils::LOCMODE_NONE=0;
const int hd24utils::LOCMODE_NONZERO=1;
const int hd24utils::LOCMODE_ALL=2;
const int hd24utils::LOCMODE_MASK=0x3;

const int hd24utils::SIZEMODE_NONE=0;
const int hd24utils::SIZEMODE_ACTUAL=4;
const int hd24utils::SIZEMODE_ALLOCATED=8;

const int hd24utils::SIZEMODE_MASK=0x0c;
#ifdef WINDOWS
bool hd24utils::isXPorlater()
{
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    // Vista returns 6.0, 2000/xp/2003 are 5.0, 5.1 and 5.2 respectively. 
    return  
       ( (osvi.dwMajorVersion > 5) ||
       ( (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >= 1) ));

}

bool hd24utils::isVistaorlater()
{
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    // Vista returns 6.0, 2000/xp/2003 are 5.0, 5.1 and 5.2 respectively. 
    return  (osvi.dwMajorVersion >= 6) ;
}
#endif
bool hd24utils::gencatalog_showlocs(hd24song* currentsong,string* strcatalog,
				int catalogoptions)
{
	int locmode=catalogoptions & hd24utils::LOCMODE_MASK;
	
	if (locmode==hd24utils::LOCMODE_NONE) return false;

	if (
		(locmode!=hd24utils::LOCMODE_ALL)
		&& (locmode!=hd24utils::LOCMODE_NONZERO)
	) return false;
	bool havelocpoint=false;
	for (uint32_t i=0;i<currentsong->locatepointcount();i++)
	{
		uint32_t locpos=currentsong->getlocatepos(i);
		if (locpos==0) {
			if (locmode==hd24utils::LOCMODE_NONZERO)
			{
				continue;
			}
		}
		havelocpoint=true;
		*strcatalog+="                ";		
		string* locnum=Convert::int2str(i);
		string* loc2=Convert::padleft(*locnum,2," ");
		*strcatalog+=*loc2;
		delete loc2;
		*strcatalog+=": ";
		delete locnum;
		string* locname=currentsong->getlocatename(i);
		*strcatalog+=*locname;
		delete locname;
		*strcatalog+="    ";
		string* timecode=currentsong->display_duration(locpos);
		*strcatalog+=*timecode;
		delete timecode;		
		*strcatalog+="\n";
	}
	
	return havelocpoint;
}

bool hd24utils::gencatalog_showsongsize(hd24song* currentsong,string* strcatalog,
				int catalogoptions)
{
	int sizemode=catalogoptions & SIZEMODE_MASK;
	if (sizemode==SIZEMODE_NONE) return false;
	// max songsize is 2^32 samples*3 bytes*24 channels
	// =309 237 645 312
	switch (sizemode)
	{
		case SIZEMODE_ACTUAL:
                *strcatalog+="                              Recorded size: ";						
			break;
		case SIZEMODE_ALLOCATED:
		*strcatalog+="                             Allocated size: ";			
			break;
		case (SIZEMODE_ACTUAL+SIZEMODE_ALLOCATED):
	        *strcatalog+="                  Recorded / Allocated size: ";			
			break;
		default:
		*strcatalog+="                                             ";
			break;
	}
	
	if ((sizemode & SIZEMODE_ACTUAL) == SIZEMODE_ACTUAL )
	{
            uint64_t i=currentsong->songsize_in_bytes();
	    i>>=20;
            string* songsize=Convert::int2str(i);
	    string* loc2=Convert::padleft(*songsize,12," ");		
	    *strcatalog+=*loc2;
	    *strcatalog+="M";
	    delete loc2;
	    delete songsize;
	} else {
	    *strcatalog+="            ";
	}
	
	*strcatalog+="  ";
	if ((sizemode & SIZEMODE_ALLOCATED) == SIZEMODE_ALLOCATED )
	{
	    uint64_t i=currentsong->bytes_allocated_on_disk();
	    i>>=20;
            string* songsize=Convert::int2str(i);
	    string* loc2=Convert::padleft(*songsize,12," ");
	    *strcatalog+=*loc2;
	    *strcatalog+="M";
	    delete loc2;
	    delete songsize;
	    
	} else {
	    *strcatalog+=" ";
	}
	
	*strcatalog+="\n";
	return true;
}

void hd24utils::gencatalog_showsongs(hd24project* currentproj,
				string* strcatalog,
				int catalogoptions)
{
	if (!currentproj) {
		return;
	}
	if (!strcatalog) {
		return;
	}
	int numsongs=currentproj->songcount();
	*strcatalog+="     ======================================================================\n";
	if (numsongs==0) {
		*strcatalog+="      There are no songs in this project.\n";
		return;
	}
        hd24song* currsong=NULL;
	for (int i=1; i<=numsongs; i++) {
		currsong=currentproj->getsong(i);
		if (!currsong) continue;

		*strcatalog+="     ";
		*strcatalog+="  ";
		if (i<10) {
			*strcatalog+=" ";
		}
		string* songnum=Convert::int2str(i);
		*strcatalog+=*songnum;
		*strcatalog+=": ";
		delete songnum;
	
	        string* currsname=currsong->songname();
		string* pad=Convert::padright(*currsname,35," ");
	        delete(currsname);
		*strcatalog+=*pad;
		delete pad;
		string* dur=currsong->display_duration();
		*strcatalog+=*dur;
		delete dur;
		*strcatalog+= ", " ;

		string* chans=Convert::int2str(currsong->logical_channels());
		string* chans2=Convert::padleft(*chans,2," ");
		*strcatalog+=*chans;
		delete chans;
		delete chans2;
		*strcatalog+="ch. ";

		string* samrate=Convert::int2str(currsong->samplerate());
		*strcatalog+=*samrate;
		delete samrate;
		*strcatalog+=" Hz";


		if (currsong->iswriteprotected()) 
		{
			*strcatalog+="*";
		}

		*strcatalog+="\n";
		bool addnewline=
			gencatalog_showlocs(currsong,strcatalog,
					catalogoptions);
		bool dummy=
			gencatalog_showsongsize(currsong,strcatalog,
					catalogoptions);
		dummy|=dummy; /* suppress compiler warning */
		if (addnewline) {
			*strcatalog+="\n";	
		}
                delete currsong;
                currsong=NULL;
	}
	return;
}

void hd24utils::gencatalog_showprojects(hd24fs* currenthd24,string* strcatalog,
					int catalogoptions)
{
	int numprojs=currenthd24->projectcount();
	hd24project* currproj=NULL;
	for (int i=1; i<=numprojs; i++) 
	{
		currproj=currenthd24->getproject(i);

		*strcatalog+="     ======================================================================\n";
		*strcatalog+="     Project ";
		string* projnum=Convert::int2str(i);
		*strcatalog+=*projnum;
		delete projnum;
		*strcatalog+=": ";

	        string* currpname=currproj->projectname();	        
		*strcatalog+= *currpname;
	        delete(currpname);

		*strcatalog+="\n"; // << endl;
		gencatalog_showsongs (currproj,strcatalog,catalogoptions);
		delete(currproj);
	}
	
}

int hd24utils::gencatalog(hd24fs* currenthd24,string* strcatalog) 
{
	return gencatalog(currenthd24,strcatalog,hd24utils::LOCMODE_NONE);
}

int hd24utils::gencatalog(hd24fs* currenthd24,string* strcatalog,
			  int catalogoptions) 
{
	time_t currenttime;
	struct tm timestamp;
	char timebuf[80];
	time(&currenttime);
	timestamp = *localtime(&currenttime);
	strftime(timebuf,sizeof(timebuf),"%a %Y-%m-%d %H:%M:%S %Z", &timestamp);
	*strcatalog+= "     Catalog timestamp      : ";
	*strcatalog+= timebuf ;
	*strcatalog+="\n";

	*strcatalog+="     Volume name            : ";
	string* volname=currenthd24->volumename();
	*strcatalog+=*volname;
	delete volname;
	*strcatalog+="\n";

	*strcatalog+="     Number of projects     : ";
	string* pcount=Convert::int2str(currenthd24->projectcount());
	*strcatalog+=*pcount;
	delete pcount;

	*strcatalog+="\n";
	gencatalog_showprojects(currenthd24,strcatalog,catalogoptions);
	return 0;
}

string* hd24utils::savecatalog(hd24fs* currenthd24,string* filename)
{
	return savecatalog(currenthd24,filename,hd24utils::LOCMODE_NONE);
}

string* hd24utils::savecatalog(hd24fs* currenthd24,string* filename,int catalogoptions)
{
	string* error=new string("");

	// gencatalog
	string* catalog=new string("");

	if (hd24utils::gencatalog(currenthd24,catalog)!=0) 
	{
		*error+="Error generating catalog.";
		return error;
	}
	fstream to_out(filename->c_str(),ios::out);
	if (!to_out) 
	{
		*error+="Cannot write catalog.";
		return error;
	}	
	to_out << *catalog ;

	to_out.flush();
	to_out.close();
	return NULL;
}

string* hd24utils::printcatalog(hd24fs* currenthd24)
{
	return hd24utils::printcatalog(currenthd24,hd24utils::LOCMODE_NONE);
}

string* hd24utils::printcatalog(hd24fs* currenthd24,int catalogoptions)
{

	string* error=new string("");
	string catname="_hd24cat.txt";	


	// gencatalog
	string* catalog=new string("");

	if (hd24utils::gencatalog(currenthd24,catalog)!=0) 
	{
		*error+="Error generating catalog.";
		return error;
	}
	fstream to_out(catname.c_str(),ios::out);
	if (!to_out) 
	{
		*error+="Cannot write catalog.";
		return error;
	}	
	*catalog += "\f\n"; // form feed
	to_out << *catalog ;

	to_out.flush();
	to_out.close();

#ifdef WINDOWS
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof (SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS; //SEE_MASK_INVOKEIDLIST;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb="print";
	ShExecInfo.lpFile=catname.c_str();
	ShExecInfo.lpParameters = "";
	ShExecInfo.lpDirectory=NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
	unlink (catname.c_str());
#else	
	char s[1024];
	string* catcmd = new string (PRINTAPP);
	*catcmd += " ";
       	*catcmd += catname;
	*catcmd += " 2>&1";
	FILE *fp = popen(catcmd->c_str() , "r");
	while (fgets(s, sizeof(s)-1, fp)) 
	{
		*error += s; 
	}
	pclose(fp);
	unlink (catname.c_str());
#endif
	return error;
}


void hd24utils::interlacetobuffer(unsigned char* sourcebuf,unsigned char* targetbuf,
		uint32_t totbytes,uint32_t bytespersam,uint32_t trackwithingroup,uint32_t trackspergroup)
{
	uint32_t samplenum;
	uint32_t totsams=totbytes/bytespersam;
	uint32_t trackoff=(trackwithingroup*bytespersam);
	uint32_t q=0;
	// unroll loop for bytespersam=1,2,3
	switch (bytespersam) 
	{
		case 3:
		for (samplenum=0;samplenum<totsams;samplenum++)
		{
			uint32_t samoff=(samplenum*bytespersam);
			q=trackspergroup*samoff+trackoff;
			targetbuf[q++]=sourcebuf[samoff];
	                targetbuf[q++]=sourcebuf[samoff+1];
	                targetbuf[q++]=sourcebuf[samoff+2];
		}		
		break;
		case 1:
			for (samplenum=0;samplenum<totsams;samplenum++)
			{
				uint32_t samoff=(samplenum*bytespersam);
				q=trackspergroup*samoff+trackoff;
				targetbuf[q]=sourcebuf[samoff];
			}	
			break;
		case 2:
			for (samplenum=0;samplenum<totsams;samplenum++)
			{
				uint32_t samoff=(samplenum*bytespersam);
				q=trackspergroup*samoff+trackoff;
				targetbuf[q++]=sourcebuf[samoff];
	                        targetbuf[q++]=sourcebuf[samoff+1];
			}		
			break;
		default:
			for (samplenum=0;samplenum<totsams;samplenum++)
			{
				uint32_t samoff=(samplenum*bytespersam);
				q=trackspergroup*samoff;
				for (uint32_t j=0; j<bytespersam; j++) {
				   targetbuf[q+j+trackoff]=sourcebuf[samoff+j];
				}
			}
			break;
	}
}

bool hd24utils::dirExists(const char * pszDirName)
{
#ifdef WIN32
    uint32_t dwAttrs;
    if (!(pszDirName))
    {
      //setupLog(NULL, "DirExists failed: passed in NULL parameter for directory");
      return (1==0);
    }
    dwAttrs = GetFileAttributes(pszDirName);
    if ((dwAttrs != 0xFFFFFFFF) && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
    {
      return (1==1);
    }
    return (1==0);
#else
    struct stat fi;

    if (stat (pszDirName, &fi) == -1 || !S_ISDIR(fi.st_mode))
    {
      return (1==0);
    }
    else
    {
      return (1==1);
    }
#endif
}

bool hd24utils::isdir(const char* name)
{
	struct stat fi;
	if (stat(name, &fi))
	{
		if (S_ISDIR(fi.st_mode))
		{
			// name points to a dir
			return true;
		}
		return false;
	}
	return false;
}

bool hd24utils::isfile(const char* name)
{
	struct stat fi;
	if (stat(name, &fi))
	{
		if (S_ISDIR(fi.st_mode))
		{
			// name points to a dir
			return false;	
		}
		return true;
	}
	return false;
}

int hd24utils::savedrivesectors(hd24fs* currenthd24,string* outputfilename,unsigned long firstsector,unsigned long endsector,char* message,int* cancel) {
	unsigned long i;
#define MULTISECTOR 100	
	unsigned char bootblock[(MULTISECTOR+1)*512]; // 1 sector spare
	memset(bootblock,0,MULTISECTOR*512);
#if defined(LINUX) || defined(DARWIN)
#if (UTILDEBUG==1)
	cout << "creat64" << endl;
#endif
	FSHANDLE handle=creat64(outputfilename->c_str(),O_WRONLY);
#endif
#ifdef WINDOWS
	FSHANDLE handle;
	if (!currenthd24)
	{
              _unlink(outputfilename->c_str());
		
	handle=CreateFile(outputfilename->c_str(),GENERIC_WRITE|GENERIC_READ,
	      FILE_SHARE_READ|FILE_SHARE_WRITE,
	      NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	} else {	
		handle=CreateFile(outputfilename->c_str(),GENERIC_WRITE|GENERIC_READ,
              FILE_SHARE_READ|FILE_SHARE_WRITE,
              NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	}

	if (hd24fs::isinvalidhandle(handle)) {
		handle=CreateFile(outputfilename->c_str(),GENERIC_WRITE|GENERIC_READ,
	      FILE_SHARE_READ|FILE_SHARE_WRITE,
	      NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	}
		
	#endif
	if (hd24fs::isinvalidhandle(handle)) {
#if (UTILDEBUG==1)
		cout << "Cannot open file "<<outputfilename <<" for writing. Access denied?" << endl;
#endif
		return 1;		
	}
#ifdef WINDOWS
	LARGE_INTEGER lizero;
	lizero.QuadPart=0;
	LARGE_INTEGER filelen;
	filelen.QuadPart=0;
	SetFilePointerEx(handle,lizero,&filelen,FILE_BEGIN);
#endif
	int q=0;
	i=firstsector;
	while (i<=endsector)
	{
		int newi=i;
		q++;
		int numsectors=1;
		if ((endsector-i) >=MULTISECTOR) {
			numsectors=MULTISECTOR;
		}
		// if currenthd24 is NULL, we're not doing a copy
		// but creating an empty drive image.
		if (currenthd24!=NULL) {
			currenthd24->readsectors_noheader(currenthd24,i,bootblock,1); // raw reading 
		}
#if defined(LINUX) || defined(DARWIN)
		uint64_t targetoff=i;
		targetoff-=firstsector;
		targetoff*=512;
		ssize_t byteswritten=0;
		if (currenthd24!=NULL) {
			byteswritten=pwrite64(handle,bootblock,512,targetoff);
			newi++;
		} else {
			byteswritten=pwrite64(handle,bootblock,512*numsectors,targetoff);
			newi+=numsectors;
		}
#endif
#ifdef WINDOWS
		//DWORD dummy;
		//long bytes=0;
		uint64_t targetoff=i;
		targetoff-=firstsector;
		uint64_t byteswritten=0;
		if (currenthd24!=NULL) {
			byteswritten=currenthd24->writesectors(handle,targetoff,bootblock,1);
			newi++;
		} else {
#if (HD24DEBUG==1)
//	cout << "write "<<numsectors<<" sectors to output sector "<<i-firstsector<<endl;
#endif
			byteswritten=currenthd24->writesectors(handle,targetoff,bootblock,numsectors);
			newi+=numsectors;
		}
#endif
		if (byteswritten==0) {
#if defined(LINUX) || defined(DARWIN)
			close (handle);
#endif
#ifdef WINDOWS
			CloseHandle(handle);
#endif
			return 1;
		}
		if (message!=NULL) {
			if (q%1000==0) {
				sprintf(message,"Saving sector %ld of %ld",i,(endsector+1));
			
				Fl::wait(0);
			}
		}
		i=newi;
	}
#if defined(LINUX) || defined(DARWIN)
	close (handle);
	chmod(outputfilename->c_str(),0664);
#endif
#ifdef WINDOWS
#if (HD24DEBUG==1)
	// in debugging mode let's report on current file
	// pointer cause something is not right.
	SetFilePointerEx(handle,lizero,&filelen,FILE_CURRENT); // save offset
#if (HD24DEBUG==1)
	cout << "filelen=" << filelen.QuadPart << endl
	 << "filelen/512="<<filelen.QuadPart/512 << endl;
#endif
#endif
	CloseHandle(handle);
#endif

	return 0;
}

int hd24utils::savedriveimage(hd24fs* currenthd24,string* imagefilename,char* message,int* cancel) {
	unsigned long firstsector=0;
	int lastsecerror=0;
	unsigned long endsector=currenthd24->getlastsectornum(&lastsecerror);
	return savedrivesectors(currenthd24,imagefilename,firstsector,endsector,message,cancel);
}

int hd24utils::newdriveimage(string* imagefilename,uint32_t endsector,char* message,int* cancel) {
	if (endsector<1353963)
	{
		// min. number of sectors is 1353964
		// this makes the min. end sector at least 1353963
		// requested drive size is less than possible minimum size
		if (message!=NULL)
		{
			strcpy(message,"Requested drive image is smaller than minimum of 1353964 sectors");
		}
		return -1;
	}
	
	unsigned long firstsector=0;
#if (UTILDEBUG==1)
cout << "about to save drive sectors" <<endl;
#endif
	int result=savedrivesectors(NULL,imagefilename,firstsector,endsector,message,cancel);
	if (result!=0)
	{
		return result;
	}
#if (UTILDEBUG==1)
cout << "saved drive sectors, creating new fs object to format" <<endl;
#endif
	hd24fs* newfs=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,imagefilename,true);
#if (UTILDEBUG==1)
	// cout << "Last sectornum of newly created fs=" << newfs->getlastsectornum() << endl << "write enabling fs" <<endl;
#endif
	newfs->write_enable();
#if (UTILDEBUG==1)
cout << "quickformatting fs" <<endl;
#endif
	newfs->quickformat(message);
#if (UTILDEBUG==1)
	cout << "format result=" << *message << endl
 << "deleting format fs object" <<endl;
#endif
	delete newfs;
#if (UTILDEBUG==1)
cout << "destructed fs object" <<endl;
#endif	
	return 0;
}

void hd24utils::dumpsector(const char* buffer)
{
	for (int i=0;i<512;i+=16) {
		string* dummy=Convert::int32tohex(i);
	       	cout << *dummy	<< "  "; // PRAGMA allowed
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
int hd24utils::saveheader(hd24fs* currenthd24,string* headerfilename) {
	return savedrivesectors(currenthd24,headerfilename,0,0x700,NULL,NULL);
	// TODO: 0x700 sectors is arbitrary. First song entry is at 0x77,
	// total of 99*99 songs of 7 sectors each puts actual required length 
	// at 0x10c76. However, this has served fine so far.

}

void hd24utils::findfile(const char* rawname,const char* path,char* result) 
{
	string* strpath=new string(path);
#ifdef WINDOWS
	string* pathsep=new string(";");
#else
	string* pathsep=new string(":");
#endif
	int last=0;	
	string* exename;
	string* exepath;	
#if (UTILDEBUG==1)
	cout << "search path for exe " << endl;
#endif
	while (last==0)
	{
                cout << "." << endl;
		uint32_t idx=strpath->find(pathsep->c_str());
		if (idx==string::npos) {
			last=1;
			exepath=new string(strpath->c_str());
			exename=new string(strpath->c_str());
			*strpath="";
		} else {
			exepath=new string(strpath->substr(0,idx));
			exename=new string(strpath->substr(0,idx));
			*strpath=strpath->substr(idx+1);
		}
#if (UTILDEBUG==1)
		cout << "exepath= " << *exepath << endl;
#endif
		if (exepath->substr(exepath->length()-1,1)!=PATHSLASH) {
		     *exepath+=PATHSLASH;
		}	
		if (exename->substr(exename->length()-1,1)!=PATHSLASH) {
		     *exename+=PATHSLASH;
		}	

		*exename+=rawname;
#if (UTILDEBUG==1)
		cout << "test if file " << exename->c_str() << " exists" << endl;
#endif
		if (hd24utils::fileExists(exename->c_str())) {
			strncpy(result,exepath->c_str(),2048);
			if (exename!=NULL) {
				delete exename;
				exename=NULL;
			}
			if (exepath!=NULL) {
				delete exepath;
				exepath=NULL;
			}
			if (strpath!=NULL) {		
				delete strpath;
				strpath=NULL;
			}
			if (pathsep!=NULL) {
				delete pathsep;
				pathsep=NULL;
			}
			return ;
		}
		if (exename!=NULL) {
			delete exename;
			exename=NULL;
		}
		if (exepath!=NULL) {
			delete exepath;
			exepath=NULL;
		}		
	}	

	if (strpath!=NULL) {		
		delete strpath;
		strpath=NULL;
	}
	if (pathsep!=NULL) {
		delete pathsep;
		pathsep=NULL;
	}

	result[0]=(char)0;
	return;
}

bool hd24utils::fileExists(const char* strFilename) {
	struct stat stFileInfo;
 	int intStat;

	// Attempt to get the file attributes
	intStat = stat(strFilename,&stFileInfo);
	if(intStat == 0) 
	{
		// We were able to get the file attributes
		// so the file obviously exists.
		return true;
	}
	// We were not able to get the file attributes.
	// This may mean that we don't have permission to
	// access the folder which contains this file. If you
	// need to do that level of checking, lookup the
	// return values of stat which will give you
	// more details on why stat failed.
	return false;
}

string* hd24utils::getconfigvalue(string which,string strdefault)
{
	string* configval;
	char buffer[255];
	Fl_Preferences* userprefs=new Fl_Preferences(Fl_Preferences::USER, "HD24","HD24connect" );
	/* Attempt to find last used project dir */
	if (userprefs->entryExists(which.c_str())) {
		userprefs->get(which.c_str(),buffer,strdefault.c_str(),255);
		configval=new string(buffer);
	} else {
	        configval=new string(strdefault.c_str());
	}
	delete userprefs;
	return configval;

}

void hd24utils::setconfigvalue(string which,const char* newval)
{
	Fl_Preferences* userprefs=new Fl_Preferences(Fl_Preferences::USER, "HD24","HD24connect" );
	userprefs->set(which.c_str(),newval);
	delete userprefs;
}

string* hd24utils::getlastdir(string which)
{
	string* whichdir;
	char buffer[FL_PATH_MAX];

	Fl_Preferences* userprefs=new Fl_Preferences(Fl_Preferences::USER, "HD24","HD24connect" );
	/* Attempt to find last used project dir */
	if (userprefs->entryExists(which.c_str())) {
		userprefs->get(which.c_str(),buffer,".",FL_PATH_MAX);
		whichdir=new string(buffer);
	} else {
	        whichdir=new string(".");
	}
	delete userprefs;
	return whichdir;
}

void hd24utils::setlastdir(string which,const char* newdir)
{
	Fl_Preferences* userprefs=new Fl_Preferences(Fl_Preferences::USER, "HD24","HD24connect" );
	userprefs->set(which.c_str(),newdir);
	delete userprefs;
}

void hd24utils::recursivemkdir(string* dirname)
{
	/* Given a full path name, this function
	   will create a directory to match that path name.
	   If parent directories need to be created first, they will be.
	*/
#if (UTILDEBUG==1)
	cout << "recursivemkdir " << *dirname << endl;
#endif
	string* dirslash=new string(DIRSLASH);
	string* mydirname=new string(dirname->c_str());
	string* mynewdirname=NULL;
	int mylen=mydirname->size();
	
	if (mydirname->substr(mylen-1,1)==(*dirslash))
	{
		mynewdirname=new string(mydirname->substr(0,mylen-1));
		delete mydirname;
		mydirname=mynewdirname;
	}
#if (UTILDEBUG==1)
	cout << "dirname to create without slash=" << *mydirname << endl;
#endif
	
	if ((*mydirname)=="")
	{
		delete dirslash;
		delete mydirname;
		return;
	}
	
	if (hd24utils::dirExists(mydirname->c_str()))
	{
		// dir with current name already exists.
		delete dirslash;
		delete mydirname;
		return;
	}
	
	/* Strip out last dirname from dir string to get parent dir;
	   Create parent dir first, then create full dir. */
	/* Let's start by stripping out the filename and only keeping the
	   directory. */
	string* parentdir=new string(mydirname->c_str());
	int parentlen=parentdir->size();
	string* newparentdir=NULL;
	while (parentdir->substr(parentlen-1,1)!=*dirslash)
	{
		parentlen--;
		if (parentlen==0)
		{
			/* No dir specified in file string, so set to current
			   dir, which should always exist.
			*/
			delete parentdir;
			delete dirslash;
			delete mydirname;
			return;
		}
		newparentdir=new string(parentdir->substr(0,parentlen));
		delete parentdir;
		parentdir=newparentdir;
	}
#if (UTILDEBUG==1)
	cout << "making parent dir "<<parentdir<<" first." << endl;
#endif
	hd24utils::recursivemkdir(parentdir);
#ifdef WINDOWS
	mkdir(mydirname->c_str());
#else
	mkdir(mydirname->c_str(),0777);
#endif
	
	delete parentdir;
	delete dirslash;
	delete mydirname;
	return;
}

bool hd24utils::guaranteefiledirexists(string* fname)
{
	/* Given a full file path+filename, this function verifies whether
	   the directory specified by the file path exists. If it does not,
	   this function will attempt to create it.
	   
	   Returns true if the directory originally existed;
	   Returns true if the directory could be created;
	   Returns false if the directory did not exist AND could not be
	   created.
	  
	*/
	string* myfname=new string(fname->c_str());
	int l=myfname->size();
	string* dirslash=new string(DIRSLASH);
	
	/* Let's start by stripping out the filename and only keeping the
	   directory. */
	while (myfname->substr(l-1,1)!=*dirslash)
	{
		l--;
		if (l==0)
		{
			/* No dir specified in file string, so set to current
			   dir, which should always exist.
			*/
			delete dirslash;
			delete myfname;
			return true;
		}
		string* newmyfname=new string(myfname->substr(0,l));
		delete myfname;
		myfname=newmyfname;
	}
	
	/* myfname now contains the directory which we want to guarantee
	   exists. */
	if (hd24utils::dirExists(myfname->c_str()))
	{
		/* Directory already exists. No need to create it. */
		delete dirslash;
		delete myfname;
		return true;
	}
	hd24utils::recursivemkdir(myfname);
	if (hd24utils::dirExists(myfname->c_str()))
	{
		/* Creating the dir succeeded. */
		delete dirslash;
		delete myfname;
		return true;
	}
	
	// Creating the dir failed.
	delete dirslash;
	delete myfname;
	return false;
}

int hd24utils::isabsolutepath(const char* pathname) {
  if (pathname[0]=='/') return (1==1);
  if (pathname[0]=='\\') return (1==1);
  if (pathname[1]==':') return (1==1);
  return (1==0);
}

void hd24utils::getprogdir(const char* currpath,const char* callpath,char* result) {
        if (isabsolutepath(callpath)) {
                strncpy(result,callpath,strlen(callpath)+1);
                return;
        }
        strcat(result,currpath);
#ifdef WINDOWS
        strcat(result,"\\\0");
#else
        strcat(result,"/\0");
#endif

        if (strcmp(callpath,"./")!=0) {
                strcat(result,callpath);
        }
        return;
}


void hd24utils::getmyabsolutepath(const char** argv,char* absprogpath)
{
	/* First, the program will figure out its own
	   absolute program path. This will be used
           for loading libraries after program startup,
           rather than having hard dependencies *during*. */
	char* currentworkingdir=(char*)memutils::mymalloc("main()",2048,1);

        string currname=argv[0];
        string rawname="";
#if (MAINDEBUG==1)
	cout << "Currname=" << currname << endl;
#endif
        do {
                string lastchar="";
                lastchar+=currname.substr(currname.length()-1,1);
                if ((lastchar=="/") || (lastchar=="\\")) {
                        break;
                }
                rawname=currname.substr(currname.length()-1,1)+rawname;
                currname=currname.substr(0,currname.length()-1);
           
                if (currname=="") break;
        } while (1==1);
        // currname=path given on commandline
	
        char cwd[2048];
        char* mycwd=getcwd(&cwd[0],2048);
	mycwd=&cwd[0];
#if (MAINDEBUG==1)
	cout << "cwd=" << cwd << endl;
#endif

        char fullname[2048];
	absprogpath[0]='\0';
        getprogdir(mycwd,currname.c_str(),&absprogpath[0]);

#if (MAINDEBUG==1)
	cout << "absprogpath=" << absprogpath << endl;
#endif
        memutils::myfree("main()",currentworkingdir);

	/*
         * Either absprogpath now contains absolute program path
         * (if called from current path or by specifying path)
         * or we need to traverse the OS PATH.
         */
	strncpy(fullname,absprogpath,2048);
        strcat(fullname,rawname.c_str());
#if (MAINDEBUG==1)
	cout << "fullname=" << fullname << endl;
#endif
        if (!(hd24utils::fileExists(fullname))) {
		/* File according to full name does not exist. */
#if (MAINDEBUG==1)
		cout << "rawname=" << rawname << ", path=" << getenv("PATH") << endl;
#endif
        	hd24utils::findfile(&rawname[0],getenv("PATH"),&absprogpath[0]);
		if (strlen(absprogpath)==0) {
			cout << "Cannot find executable. "<< endl;
			return;
		}
	}
	// The final word on which path contains the exe is absprogpath.
	return;
}

bool hd24utils::insafemode()
{
    string* y=hd24utils::getconfigvalue("drive_writeenabled","0");
    bool hd24safemode=true;
    if (strcmp(y->c_str(),"1")==0) { hd24safemode=false; }
    delete y; y=NULL;
    return hd24safemode;
}

bool hd24utils::wavefix(SoundFileWrapper* sndfile, const char* filetofix)
{
#define HEADERSIZE 44
#define SAMPLES_IN_HALF_BUF 1024*1024
#define SAMPLES_IN_FULL_BUF 2*SAMPLES_IN_HALF_BUF
#define BYTES_PER_SAM 3
#define BYTES_IN_HALF_BUF BYTES_PER_SAM*SAMPLES_IN_HALF_BUF
#define BYTES_IN_FULL_BUF BYTES_PER_SAM*SAMPLES_IN_FULL_BUF
	char audiobuf[BYTES_IN_FULL_BUF];
//	char fixbuf[BYTES_IN_HALF_BUF];
	FILE* infile;
	FILE* outfile;

	string* outputfilename=new string(filetofix);
	string* ext=new string("");
	while (1==1)
	{
		*ext=outputfilename->substr(outputfilename->length()-1,1)+*ext; // last char
		*outputfilename=outputfilename->substr(0,outputfilename->length()-1);
		if (ext->substr(0,1)==".")
		{
			break;
		}
		if (*outputfilename=="")
		{
			break;
		}
	}
	*outputfilename=*outputfilename+"_fixed";
	*outputfilename=*outputfilename+*ext;

	infile=fopen(filetofix,"rb");
	outfile=fopen(outputfilename->c_str(),"wb");

	/* Copy header */
	char buffer[HEADERSIZE];
	fseek(infile,0,SEEK_SET);
	int readcount;
	readcount=fread((void*)&buffer[0],1,HEADERSIZE,infile);
	if (readcount>0) {
		int writecount=fwrite((const void*)&buffer[0],1,HEADERSIZE,outfile);
		if (writecount==0) {
#if (UTILDEBUG==1)
			cout << "Cannot write output file" << endl;
#endif
		}
	}
	/* end copy header */

//	findmask(infile);
	fseek(infile,HEADERSIZE,SEEK_SET);

	do {
		readcount=fread((void*)&audiobuf[BYTES_IN_HALF_BUF],1,BYTES_IN_HALF_BUF,infile);
		for (int i=0; i<SAMPLES_IN_HALF_BUF-1;i+=2) {
			int q0=3;
			int q1=0;
			audiobuf[BYTES_IN_HALF_BUF+i*3+q0+0]=audiobuf[BYTES_IN_HALF_BUF+i*3+q1+0];
			audiobuf[BYTES_IN_HALF_BUF+i*3+q0+1]=audiobuf[BYTES_IN_HALF_BUF+i*3+q1+1];
			audiobuf[BYTES_IN_HALF_BUF+i*3+q0+2]=audiobuf[BYTES_IN_HALF_BUF+i*3+q1+2];
		}	
		if (readcount>0) {
			int writecount=fwrite(
					(void*)&audiobuf[BYTES_IN_HALF_BUF],1,readcount,outfile);
			if (writecount==0) {
#if (UTILDEBUG==1)
			cout << "Cannot write output file" << endl;
#endif
			}
		}

	}
	while (readcount>0);


	fclose(outfile);
	fclose(infile);

	delete outputfilename;
	delete ext;	
	return true;
}

