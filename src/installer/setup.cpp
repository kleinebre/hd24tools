// This is a very rudimentary installer framework-
// it won't do much more than copy a few files to the proper location.
// Of course there are a few wizard screens which are made in fluid (FLTK),
// these can be altered as needed.
// A small portion of the actual install stuff is done in the READY screen
// (search for it in the code), this could still be moved out to the
// installer_resources.h file.
// At this moment, desktop shortcuts are not yet created, for more info
// read 
// http://delphi.about.com/od/windowsshellapi/a/create_lnk.htm (shell links)
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include "ui_welcome.h"
#include "ui_license.h"
#include "ui_instdir.h"
#include "ui_ready.h"
#include "setupapi.cpp"
#include "installer_resources.h"
#include <iostream>
#include <fstream>
#include <string>
#ifdef WINDOWS
#define DEFAULT_INSTDIR "C:\\HD24tools\\"
#include <windows.h>
#include <FL/x.H>
#define IDI_ICON1 101
#endif
#ifndef WINDOWS
#define DEFAULT_INSTDIR "/usr/local/bin/HD24tools/"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WINDOWS
#include <unistd.h>
#endif

#define ALLOW_OVERWRITE true
#define NO_OVERWRITE false

#ifndef S_IXGRP
#define S_IXGRP 0
#endif	
#ifndef S_IXOTH
#define S_IXOTH 0
#endif
bool confirm(const char* question)
{
	int result=fl_choice(question,"No","Yes",NULL);
	if (result==1) {
       	 return true;
	}
return false;
}

int lastx=0;
int lasty=0;
string fullbinname="";
int installer_writefile(unsigned char* filedata,long long filesize,const char* instdir,const char* filename,bool allow_overwrite)
{
	string fname=instdir;
	fname+=filename;
	fstream binfile(fname.c_str(),ios::binary|ios::out);
	binfile.write(reinterpret_cast<char *>(filedata),filesize);
	binfile.close();
#ifndef WINDOWS
	chmod(fname.c_str(),S_IRWXU|S_IXGRP|S_IRGRP|S_IXOTH|S_IROTH);
#endif
	return 0;
}
void installer_getsysdir(char* sysdir)
{
	// on Windows: GetSystemDirectory()
}
void installer_getprogdir(char* progdir)
{
	// on Windows: getProgramDirectory()
}
int main(int argc, char **argv,char *envp[]) {
	UI_Welcome ui_welcome;
	UI_License ui_license;
	UI_Instdir ui_instdir;
	char* instdir=new char[4096];
	instdir[0]='\0';
	bool done=false;
	string currscreen;
	currscreen="welcome";
#ifndef WINDOWS
     // on non-windows systems, default to user home dir
     // (on non windows or no home found, 
     // defaults to systemwide dir)
     int count=0;
     char homedir[255];
     homedir[0]='\0';
     while (envp[count] != NULL) 
     {
	if (strncmp(envp[count],"HOME=",5)==0)
        {
		strncpy(&homedir[0],&envp[count][5],250);
	}
	++count;
     }
     
	if (strlen(homedir)>0) 
	{
		strncpy(&instdir[0],&homedir[0],128);
                int dirlen=strlen(instdir)-1;
                if ((instdir[dirlen]!='/')
                     &&(instdir[dirlen]!='\\'))
               {
#ifdef WINDOWS
                     instdir[dirlen+1]='\\';
                     instdir[dirlen+2]='\0';
#else
                     instdir[dirlen+1]='/';
	             instdir[dirlen+2]='\0';
#endif
	       }
               dirlen=strlen(instdir)-1;
	       strcat(instdir,"HD24tools/");	
	}
#endif	
	int insterror=0;
	int result=0;
	UI_Ready ui_ready;
	char* sysdir=new char[4096];
	installer_getsysdir(&sysdir[0]);
	while (done==false) {
		if (currscreen.compare("done")==0)
		{
			done=true;
			continue;
		}
		if (currscreen.compare("welcome")==0)
		{
			Fl_Window *window = ui_welcome.make_window(&currscreen[0]);
#ifdef WINDOWS
			window->icon((char *)LoadIcon(fl_display,MAKEINTRESOURCE(IDI_ICON1)));
#endif
			window->position(lastx,lasty);
			window->end();
			window->show(); //argc, argv);
			Fl::run(); lastx=window->x(); lasty=window->y();
			
		     	currscreen = ui_welcome.currscreen;	
			if (currscreen.compare("next")==0) {
				currscreen="license";
				continue;
			}
			if (currscreen.compare("cancel")==0) {
				currscreen="done";
				continue;
			}
			// screen closed otherwise
			currscreen="done";
			continue;
		}
		if (currscreen.compare("license")==0)
		{
			Fl_Window *window = ui_license.make_window(&currscreen[0]);
#ifdef WINDOWS
			window->icon((char *)LoadIcon(fl_display,MAKEINTRESOURCE(IDI_ICON1)));
#endif
			window->position(lastx,lasty);
			window->end();
			window->show(); //argc, argv);
			Fl::run(); lastx=window->x(); lasty=window->y();
		     	currscreen = ui_license.currscreen;	
			if (currscreen.compare("back")==0) {
				currscreen="welcome";
				continue;
			}
			if (currscreen.compare("next")==0) {
				currscreen="instdir";
				continue;
			}
			if (currscreen.compare("cancel")==0) {
				currscreen="done";
				continue;
			}
			// screen closed otherwise
			currscreen="done";
			continue;
		}
		if (currscreen.compare("instdir")==0)
		{
			if (instdir[0]=='\0')
			{
				strncpy(&instdir[0],DEFAULT_INSTDIR,128);
			}
			Fl_Window *window = ui_instdir.make_window(&currscreen[0],&instdir[0]);
#ifdef WINDOWS
			window->icon((char *)LoadIcon(fl_display,MAKEINTRESOURCE(IDI_ICON1)));
#endif
			window->position(lastx,lasty);
			window->end();
			window->show(); //argc, argv);
			Fl::run(); lastx=window->x(); lasty=window->y();
		     	currscreen = ui_instdir.currscreen;	
			instdir = ui_instdir.instdir;
			int dirlen=strlen(instdir)-1;
			if ((instdir[dirlen]!='/') 
			 &&(instdir[dirlen]!='\\'))
			{
#ifdef WINDOWS
				instdir[dirlen+1]='\\';
				instdir[dirlen+2]='\0';
#else
				instdir[dirlen+1]='/';	
				instdir[dirlen+2]='\0';
#endif
			}
			if (currscreen.compare("cancel")==0) {
				currscreen="done\0";
				continue;
			}
			if (currscreen.compare("back")==0) {
				currscreen="license";
				continue;
			}
			if (!setupDirExists(instdir)) 
			{
				bool mustcreate=confirm("Install directory does not exist. Create it?");
				if (!mustcreate) {
					currscreen="instdir";
					continue;
				}
				setupCreateDir(instdir,0755);
				
				if (!(setupDirExists(instdir))) 
				{
					fl_message("Could not create install directory.\nPlease create it manually, then try again.");
					currscreen="instdir";
					continue;
				}
			}
			if (currscreen.compare("next")==0) {
				currscreen="ready";
				continue;
			}
			// screen closed otherwise
			currscreen="done";
			continue;
		}
		if (currscreen.compare("ready")==0)
		{
			// READY
			cout << "actually doing the install now" << endl;
			insterror=0;
			result=installer_writefile(
				resource_portaudio,sizeof(resource_portaudio),(const char*)instdir,resource_portaudio_filename,ALLOW_OVERWRITE);
			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; } 
/*			result=installer_writefile(
				resource_hd24tools_manual,sizeof(resource_hd24tools_manual),(const char*)instdir,resource_hd24tools_manual_filename,ALLOW_OVERWRITE);
			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; } */
			result=installer_writefile(
				resource_longliverec,sizeof(resource_longliverec),(const char*)instdir,resource_longliverec_filename,ALLOW_OVERWRITE);
			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; }
			result=installer_writefile(
				resource_unquickformat,sizeof(resource_unquickformat),instdir,resource_unquickformat_filename,ALLOW_OVERWRITE);
			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; }
#ifdef LINUX
			/* for linux, wrap the binary into a shell script
			 * that prepends the application dir to the library
			 * search path */
			fullbinname=resource_hd24connect_filename;
			fullbinname+="_bin";
			result=installer_writefile(
				resource_hd24connect,sizeof(resource_hd24connect),(const char*)instdir,fullbinname.c_str(),ALLOW_OVERWRITE);
			fullbinname=instdir;
			fullbinname+=resource_hd24connect_filename;
			fstream wrapper(fullbinname.c_str(),ios::out);
			wrapper << "#!/bin/sh" << endl;
			wrapper << "export LD_LIBRARY_PATH=" << instdir <<":$LD_LIBRARY_PATH" << endl;
			wrapper << "exec " << instdir << "hd24connect_bin $*" << endl;
			wrapper.close();
			chmod(fullbinname.c_str(),S_IRWXU|S_IXGRP|S_IRGRP|S_IXOTH|S_IROTH);
#endif
#ifdef WINDOWS
			result=installer_writefile(
				resource_hd24connect,sizeof(resource_hd24connect),(const char*)instdir,resource_hd24connect_filename,ALLOW_OVERWRITE);
#endif
#ifdef DARWIN
			/* For MacOS, wrap the binary into a .app bundle.
			 * Note: What is called LD_LIBRARY_PATH under Linux
			 * is called DYLD_LIBRARY_PATH on MacOSX
			 */
			string bundledir=instdir;
			bundledir+="hd24connect.app/";
			setupCreateDir(bundledir.c_str(),0755);
			bundledir+="Contents/";	
			setupCreateDir(bundledir.c_str(),0755);
			string macosdir;
			macosdir=bundledir+"MacOS/";
			setupCreateDir(macosdir.c_str(),0755);
			string resourcedir=bundledir+"Resources/";
			setupCreateDir(resourcedir.c_str(),0755);
			string strplist="Info.plist";	
			string strpkginfo="PkgInfo";
				
			result=installer_writefile(resource_plist_connect,sizeof(resource_plist_connect),bundledir.c_str(),"info.plist",ALLOW_OVERWRITE);
			result=installer_writefile(resource_pkginfo_connect,sizeof(resource_pkginfo_connect),bundledir.c_str(),strpkginfo.c_str(),ALLOW_OVERWRITE);
			result=installer_writefile(resource_hd24connect,sizeof(resource_hd24connect),macosdir.c_str(),resource_hd24connect_filename,ALLOW_OVERWRITE);
			
			
#endif
			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; }
			result=installer_writefile(
				resource_hd24hexview,sizeof(resource_hd24hexview),(const char*)instdir,resource_hd24hexview_filename,ALLOW_OVERWRITE);
			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; }
			result=installer_writefile(
				resource_libsndfile_1,sizeof(resource_libsndfile_1),(const char*)instdir,resource_libsndfile_1_filename,ALLOW_OVERWRITE);
			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; }
//			result=installer_writefile(
//				resource_portaudio,sizeof(resource_portaudio),(const char*)instdir,resource_portaudio_filename,NO_OVERWRITE);
//			if (result!=0) 	{ insterror=1; 	currscreen="done\0"; 	continue; }
				
			Fl_Window *window = ui_ready.make_window(&currscreen[0]);
#ifdef WINDOWS
			window->icon((char *)LoadIcon(fl_display,MAKEINTRESOURCE(IDI_ICON1)));
#endif
			window->position(lastx,lasty);
			window->end();
			window->show(); //argc, argv);
			Fl::run(); lastx=window->x(); lasty=window->y();
			
		     	currscreen = ui_ready.currscreen;	
			if (currscreen.compare("back")==0) {
				currscreen="license";
				continue;
			}
			if (currscreen.compare("next")==0) {
				currscreen="doinst";
				continue;
			}
			if (currscreen.compare("cancel")==0) {
				currscreen="done";
				continue;
			}
			// screen closed otherwise
			cout << "???" << currscreen << endl;
			currscreen="done";
			continue;
		}
		if (currscreen.compare("doinst")==0)
		{
			currscreen="done";
			continue;
		}
		cout << "Unknown screen '" << currscreen << "'." << endl;
		done=true;
	}
	return 0;
}
