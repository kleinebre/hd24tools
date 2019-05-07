#ifndef __hd24utils_h__
#define __hd24utils_h__
#ifndef WINDOWS
#define PATHSLASH "/"
#	include <sys/dir.h>
#	include <dirent.h>
#else
#define PATHSLASH "\\"
#	include <dir.h>
#	include <windows.h>
#	include <winioctl.h>
#	define FSHANDLE HANDLE
#	define FSHANDLE_INVALID INVALID_HANDLE_VALUE
#endif

#if defined(LINUX) || defined(DARWIN)
#	define FSHANDLE int
#	define FSHANDLE_INVALID -1
#endif

#include <config.h>
#include <stdio.h>
#include <string>
#include <FL/FLTKstuff.H>
#include "convertlib.h"
#include "memutils.h"
#include <hd24fs.h>
#include <hd24sndfile.h>

#ifdef WINDOWS
#include <windows.h>

#define IDI_ICON1 101

using namespace std;

#endif

class hd24fs;
class hd24project;
class hd24song;

class hd24utils 
{	
	friend class hd24fs;
	friend class hd24project;
	friend class hd24song;

	private:
		// static void savecatalog_showprojects(hd24fs* currenthd24,fstream & to_out);
		// static void savecatalog_showsongs(hd24project* currentproj,fstream & to_out);
		// locmode decides when locate points are displayed in the
		// catalog
		static bool gencatalog_showlocs(hd24song* currentsong,
						string* strcatalog,
						int locmode);
		static bool gencatalog_showsongsize(hd24song* currentsong,
						string* strcatalog,
						int catalogoptions);
		static void gencatalog_showsongs(hd24project* currentproj,
						string* strcatalog,
						int locmode);		
		static void gencatalog_showprojects(hd24fs* currenthd24,
						string* strcatalog,
						int locmode);
		static void getprogdir(const char* currpath,const char* callpath,char* result);
		static int isabsolutepath(const char* pathname);

	public:
		static const int LOCMODE_NONE;
		static const int LOCMODE_ALL;
		static const int LOCMODE_NONZERO;
		static const int LOCMODE_MASK;
		static const int SIZEMODE_NONE;
		static const int SIZEMODE_ACTUAL;
		static const int SIZEMODE_ALLOCATED;
		static const int SIZEMODE_BOTH;
		static const int SIZEMODE_MASK;
		// static int savecatalog(hd24fs* currenthd24,string* headerfilename,bool toprint);
		// static int gencatalog(hd24fs* currenthd24,string* headerfilename,bool toprint);
		static string* savecatalog(	hd24fs* currenthd24,
						string* filename,
						int locmode);
		static string* savecatalog(	hd24fs* currenthd24,
						string* filename);
		static void findfile(const char* rawfilename, const char* searchpath, char* result);
		static int gencatalog(hd24fs* currenthd24,string* strcatalog);
		static int gencatalog(hd24fs* currenthd24,string* strcatalog,
					int locmode);
		static string* printcatalog(hd24fs* currenthd24);
		static string* printcatalog(hd24fs* currenthd24,int locmode);
		static int saveheader(hd24fs* currenthd24,string* headerfilename);
		static int savedriveimage(hd24fs* currenthd24,string* imagefilename, char* message,int* cancel);
		static int newdriveimage(string* imagefilename,__uint32 endsector,char* message,int* cancel);
		static int savedrivesectors(hd24fs* currenthd24,string* imagefilename,unsigned long startsector,unsigned long endsector,char* message,int* cancel);
		static void interlacetobuffer(unsigned char* sourcebuf,unsigned char* targetbuf, __uint32 totbytes,__uint32 bytespersam,__uint32 trackwithingroup,__uint32 trackspergroup);
		static bool isdir(const char * name);
		static bool isfile(const char * name);
#ifdef WINDOWS
		static bool isXPorlater();
		static bool isVistaorlater();
#endif
		static bool dirExists(const char * pszDirName);
		static bool fileExists(const char * filename);
		static string* getlastdir(string which);
		static void dumpsector (const char* buffer);
		static void setlastdir(string which,const char* newdir);
		static string* getconfigvalue(string which,string strdefault);	
		static void setconfigvalue(string which,const char* newval);
		static void recursivemkdir(string* dirname);
		static bool guaranteefiledirexists(string* fname);
		static bool wavefix(SoundFileWrapper* sndfile,const char* filetofix);
		static void getmyabsolutepath(const char** argv,char* absprogpath);
		static bool insafemode();
};

#endif
