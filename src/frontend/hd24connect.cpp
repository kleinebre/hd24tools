#define MAINDEBUG 1
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include "ui_hd24connect.h"
#include <hd24fs.h>
#include <hd24utils.h>
#include <hd24driveimage.h>
#include <memutils.h>
#include <FL/x.H> //dave
#ifdef LINUX
#include <X11/xpm.h> //dave
#endif
#include "images/hd24connect_64x64.xpm" //dave
#include <iostream>
#include <string>
#include <stdlib.h>
#ifndef MAX_PATH
#define MAX_PATH 127
#endif

string device;
string headerfilename;
int force;
int maintmode;
int wavefixmode;
int testmode;
#ifdef LINUX
#define TIMEOUT 0.03
#endif
#define ARGDEV "--dev="
#define ARGHEADER "--header="
#define ARGFORCE "--force"
#define ARGMAINT "--maint"
#define ARGWAVEFIX "--wavefix"
#define ARGTEST "--test"

#include "selftest.cpp"

int parsecommandline(int argc, char ** argv) 
{
	int invalid=0;
	force=0;
	maintmode=0;
	wavefixmode=0;
	testmode=0;
	device="";
	for (int c=1;c<argc;c++) {

		string arg=argv[c];
		if (arg.substr(0,1)=="-") {
			if (arg.substr(1,1)!="-") {
				continue; // skip single-dash arguments
				// for instance to skip -display
				// and to skip macOS process number
			}
		}
		if (arg.substr(0,strlen(ARGTEST))==ARGTEST) {
			testmode=1;
			continue;
		}
		if (arg.substr(0,strlen(ARGDEV))==ARGDEV) {
			device=arg.substr(strlen(ARGDEV));
			continue;
		}
		if (arg.substr(0,strlen(ARGMAINT))==ARGMAINT) {
			maintmode=1;
#if (MAINDEBUG==1)
			cout << "Running in maintenance mode." << endl;
#endif
			continue;
		}
		if (arg.substr(0,strlen(ARGWAVEFIX))==ARGWAVEFIX) {
			wavefixmode=1;
#if (MAINDEBUG==1)
			cout << "Running in wavefix mode." << endl;
#endif
			continue;
		}
		if (arg.substr(0,strlen(ARGHEADER))==ARGHEADER) {
			headerfilename=arg.substr(strlen(ARGHEADER));
			continue;
		}
		if (arg.substr(0,strlen(ARGFORCE))==(ARGFORCE)) {
			force=1;
			continue;
		}
#ifdef DARWIN
		// on MacOS ignore all crap on the command line
		// (system adds process ID info etc)
		continue;
#endif

#if (MAINDEBUG==1)
		cout << "Invalid argument: " << arg << endl;
#endif
		invalid=1;
	}
	return invalid;
}

int main(int argc, char **argv) 
{
#if (MAINDEBUG==1)
	cout << "Welcome to HD24connect" << endl;
#endif

	int result=0;
        char absprogpath[2048];

	hd24utils::getmyabsolutepath((const char**)argv,(char*)&absprogpath);
	if (strlen(absprogpath)==0)
	{
		cout << "Cannot determine absolute program path." << endl;
		return 0;
	}

	int invalid=parsecommandline(argc,argv);
	if (invalid!=0) {
		return invalid;
	}

	hd24fs* sysob=NULL;
	if (testmode==1)
	{
#ifdef LINUX
		fl_open_display();
#endif
		HD24UserInterface ui(argc,argv,absprogpath);
		// testmode active- run self test.
		result=hd24test::runselftest(&ui,&device);
		cout << "Selftest run complete - Thank you for using HD24connect." << endl;
		ui.finish();
		return result;
	}	
	
	string* imagedir=hd24utils::getlastdir("driveimagedir");
#if (MAINDEBUG==1) 
	cout << "imagedir="<<*imagedir<< endl;
#endif	
	int openmode=hd24fs::MODE_RDONLY;
	if (!hd24utils::insafemode())
	{
		openmode=hd24fs::MODE_RDWR;
	}	
	if (device=="") {
		if (sysob!=NULL)
		{
			delete sysob;
		}
		sysob=new hd24fs(imagedir->c_str(),openmode);
	} else {
#if (MAINDEBUG==1)
		cout << "Trying to use " << device << " as hd24 device." << endl;
#endif
		if (sysob!=NULL)
		{
			delete sysob;
		}
		sysob=new hd24fs(imagedir->c_str(),openmode,&device,(force==1));
		if (!(sysob->isOpen())) 
		{
			if (!force) {
				cout << "Cannot open hd24 device" << endl;
				delete sysob;
				return 1;
			}
		}
	}
	delete imagedir;
	if (headerfilename!="") 
	{
		if (!(sysob->useheaderfile(headerfilename))) 
		{
			cout << "Couldn't load header file "<<headerfilename << endl;
		};
	}
#if (MAINDEBUG==1) 
	cout << "mdb(2)" << endl;
#endif	
//	Fl::scheme("gtk+");
#ifdef LINUX
        cout << "opening fl display" << endl;
	fl_open_display();
        cout << "opened fl display" << endl;
        cout << "creating ui, absprogpath = " << absprogpath << endl;
#else
        cout << "not opening fl display" << endl;
#endif
HD24UserInterface ui(argc,argv,absprogpath);

#if (MAINDEBUG==1) 
	cout << "ui createD" << endl;
	cout << "mdb(3b)" << endl;
#endif	
	sysob->setmaintenancemode(maintmode);
	sysob->setwavefixmode(wavefixmode);
#if (MAINDEBUG==1) 
	cout << "mdb(3c)" << endl;
#endif	
	Fl_Window *window = ui.make_window(sysob);
#if (MAINDEBUG==1) 
	cout << "mdb(3d)" << endl;
#endif	
#ifdef WINDOWS

	window->icon((char *)LoadIcon(fl_display,MAKEINTRESOURCE(IDI_ICON1)));
#endif
	window->end();
	window->show(); //argc, argv);
#ifdef WINDOWS
	if (!(hd24utils::isXPorlater()))
	{
		if (sysob!=NULL)
		{
			delete sysob;
			sysob=NULL;
		}
		fl_message("HD24connect for Windows requires Windows XP or later.");
		ui.finish();
		return 1;
	}
	
#endif
/* Note: string below will be replaced during compile time. */
string* hd24ver=new string("HD24VERSION");
bool isreleasecandidate=false;
bool isalpha=false;
bool ispre=false;
if (hd24ver->find("rc")!=string::npos)
{
	isreleasecandidate=true;
}
if (hd24ver->find("RC")!=string::npos)
{
	isreleasecandidate=true;
}
if (hd24ver->find("ALPHA")!=string::npos)
{
	isalpha=true;
}
if (hd24ver->find("Alpha")!=string::npos)
{
	isalpha=true;
}
if (hd24ver->find("alpha")!=string::npos)
{
	isalpha=true;
}
if (hd24ver->find("pre")!=string::npos)
{
	ispre=true;
}

delete hd24ver;

if (isalpha)
{
	fl_message("Welcome to HD24connect HD24VERSION.\n\nWARNING:\n\n"
                   "This is an ALPHA release. This means that it may contain serious bugs, \n"
                   "including ones that may corrupt your drive. Use at your own risk. When \n"
                   "using ALPHA versions, it is recommended never to work with drives that \n"
                   "contain important audio unless you have backups of those drives; Alpha \n"
                   "versions should be considered UNSUITABLE for production use.");
}
if (isreleasecandidate)
{
	fl_message("Welcome to HD24connect HD24VERSION.\n\n"
          "This is a RELEASE CANDIDATE. Core functionality was thoroughly tested,\n"
          "but your assistance for further field-testing is required. If you find\n"
          "any anomalies, please email the author (mrjb AT dnd.utwente.nl) with\n"
          "details about the bug to help make the official release as reliable\n"
          "as possible.");
}
if (ispre)
{
	fl_message("Welcome to HD24connect HD24VERSION.\n\nWARNING:\n\n"
                   "This is PRE-RELEASE. This means that it was released outside the usual \n"
                   "release schedule, typically to a single person, to provide a quick solution\n"
                   "for a problem that was already addressed by the development version of this\n"
                   "program. PRE-RELEASE versions should be considered UNTESTED and may crash or\n"
                   "have other unpredictable side effects. Please use at your own risk, and\n"
                   "upgrade as soon as possible.");
}
#ifdef LINUX //dave
        Pixmap p, mask;
        XpmCreatePixmapFromData(fl_display,DefaultRootWindow(fl_display),(char **)hd24connect_xpm, &p, &mask, NULL);

/*
FLTK support for transparent background is broken. So instead of this:
window->icon((char *)p);
We will write the following: */
XWMHints *hints=NULL;
hints = XGetWMHints(fl_display, fl_xid(window));
hints->icon_pixmap = p;
hints->icon_mask = mask;
hints->flags = IconPixmapHint | IconMaskHint;
XSetWMHints(fl_display, fl_xid(window), hints);

/* -- end of transparency fix */
#endif
#if (MAINDEBUG==1) 
	cout << "mdb(3)" << endl;
#endif	
	if (!(sysob->isOpen())) 
	{
		window->deactivate();
#ifdef WINDOWS
	if (hd24utils::isVistaorlater())
	{
		fl_message("No valid HD24 drive is connected to the system.\n"
			   "Make sure to 'Run as administrator' or use the \n"
                           "File menu to resolve this problem.");
	} else {
		fl_message("No valid HD24 drive is connected to the system.\n"
			   "Use the File menu to resolve this problem.");
	}
#else
	fl_message("No valid HD24 drive is connected to the system.\n"
                   "Use the File menu to resolve this problem.");
#endif
	window->activate();
	}
	// run normally.
	result= Fl::run(); 
#ifdef hints
	if (hints!=NULL)
	{
		delete hints;
	}
#endif
	ui.control->ready(0);
	if (sysob!=NULL)
	{
		delete sysob;
		sysob=NULL;
	}
#ifndef WINDOWS
	cout << "Thank you for using HD24connect." << endl;
#endif
//	ui.finish();
	
	return result;
}
