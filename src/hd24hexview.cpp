#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hd24fs.h"
#include "convertlib.h"

#define VERSION "1.3 beta"
#define _LARGE_FILES
#define _FILE_OFFSET_BITS 64
#define FILE_OFFSET_BITS 64
#define LARGE_FILES
#define LARGEFILE64_SOURCE
#define SECTORSIZE 512
#define ARGHEADER "--header="

#ifdef DARWIN
#	define open64 open
#	define lseek64 lseek
#	define pread64 pread
#	define creat64 creat
#	define pwrite64 pwrite
#endif

#ifndef WINDOWS
#	include <unistd.h>
#endif

string device;
string headerfilename;
uint64_t writeoffset;
int force;
int expertmode;
int graphmode;
int enable_graphmode;
int disable_graphmode;

// Check if a file exists
bool fileexists (string* fname) 
{
	struct stat fi;
	
	if ((stat(fname->c_str(), &fi) != -1) && ((fi.st_mode & S_IFDIR) == 0)) {
		return true;
	}
	
	return false;
}

// Check if file handle is invalid
bool isinvalidhandle(FSHANDLE handle)
{
#ifdef WINDOWS
	if (handle == FSHANDLE_INVALID) {
		return true;
	}
#else
	if (handle == 0 || handle == FSHANDLE_INVALID) {
		return true;
	}
#endif
	return false;
}

// Output a message for expert only features
void expertmodemessage(string feature) 
{
	cout << feature <<" is only allowed in expert mode." << endl;
}

// Seek to a position in the drive
void hd24seek(FSHANDLE devhd24, uint64_t seekpos) 
{
#ifdef WINDOWS
	LARGE_INTEGER li;
	li.HighPart = seekpos >> 32;
	li.LowPart = seekpos % ((uint64_t) 1 << 32);
	SetFilePointerEx(devhd24, li, NULL, FILE_BEGIN);
#else
	lseek64(devhd24, seekpos, 0);
#endif
	return;
}

// Calculate a 32-bit checksum for a block
long unsigned int calcblockchecksum(hd24raw* rawdevice, unsigned long firstsector, unsigned long endsector)
{
	long unsigned int checksum32 = 0;
	unsigned char origblock[5120];

	for (unsigned long k = firstsector; k < endsector; k++)
	{
		rawdevice->readsectors(k, origblock, 1);
		
		for (unsigned long i = 0; i < SECTORSIZE; i += 4) 
		{
			unsigned long num = Convert::getint32(origblock, i);
			int byte1 = num % 256;
			int byte2 = (num >> 8) % 256;
			int byte3 = (num >> 16) % 256;
			int byte4 = (num >> 24) % 256;
			num = byte4 + (byte3 << 8) + (byte2 << 16) + (byte1 << 24);
			checksum32 += num;
		}
	}
	
	return checksum32;
}

// Write to a sector
long writesectors(FSHANDLE devhd24, unsigned long sectornum, unsigned char * buffer, int sectors)
{
	int WRITESIZE = SECTORSIZE * sectors; // allows searching across sector boundaries
	hd24seek(devhd24, (uint64_t) sectornum * 512);

#ifdef WINDOWS
	DWORD dummy;
	long bytes = 0;
	
	if (WriteFile(devhd24, buffer, WRITESIZE, &dummy, NULL)) {
		bytes = WRITESIZE;
	};
#else
	long bytes = pwrite64(devhd24, buffer, WRITESIZE, (uint64_t) sectornum * 512);
#endif

	return bytes;
}

void writetofile(hd24raw* rawdevice,string filename, long firstsector,long endsector) 
{
	int i;
	unsigned char bootblock[5120];
#if defined(LINUX) || defined(DARWIN)
	FSHANDLE handle=creat64(filename.c_str(),O_WRONLY);
#endif
#ifdef WINDOWS
	FSHANDLE handle=CreateFile(filename.c_str(),GENERIC_WRITE|GENERIC_READ,
              FILE_SHARE_READ|FILE_SHARE_WRITE,
              NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (isinvalidhandle(handle)) {
		handle=CreateFile(filename.c_str(),GENERIC_WRITE|GENERIC_READ,
              FILE_SHARE_READ|FILE_SHARE_WRITE,
              NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	}
		
#endif
	if (isinvalidhandle(handle)) {
		cout << "Cannot open file "<<filename <<" for writing. Access denied?" << endl;
		return;
	}
	cout << "Write range from offset "<< *Convert::int64tohex((uint64_t)firstsector*512)<< " to offset " << *Convert::int64tohex((uint64_t)endsector*512-1) << endl;
	for (i=firstsector;i<endsector;i++) {
		rawdevice->readsectors(i,bootblock,1);
#if defined(LINUX) || defined(DARWIN)
		uint64_t targetoff=i;
		targetoff-=firstsector;
		targetoff+=writeoffset;
		targetoff*=512;
		ssize_t byteswritten=pwrite64(handle,bootblock,512,targetoff);
#endif
#ifdef WINDOWS
		//DWORD dummy;
		//long bytes=0;
		uint64_t targetoff=i;
		targetoff-=firstsector;
		targetoff+=writeoffset;
		uint64_t byteswritten=writesectors(handle,targetoff,bootblock,1);
#endif
		if (byteswritten==0) {
			cout << "Wrote 0 bytes to file. Access denied?" << endl;
#if defined(LINUX) || defined(DARWIN)
			close (handle);
#endif
#ifdef WNDOWS
			CloseHandle(handle);
#endif
			return;
		}
		if ((i%1000)==0) {
			string* x=Convert::int64tohex((uint64_t)i*0x200);
			cout << "Write offset " << *x << "\r";	
			if (x!=NULL) { delete x; 	x=NULL; }
		}
	}
#if defined(LINUX) || defined(DARWIN)
	close (handle);
#endif
#ifdef WINDOWS
	CloseHandle(handle);
#endif
	return;
}

string getbinstr(string tofind) 
{
	if (tofind.substr(0,1)=="'") {
		// find literal string
		tofind=tofind.substr(1,tofind.length()-2);
		cout << "Find string " << tofind << endl;
	} else {
		string binstr="";
		string tmp="";
		tofind+=" ";
		unsigned int i;
		for (i=0;i<tofind.length();i++) {
			string onechar=tofind.substr(i,1);
			if (onechar!=" ") {
				if (!Convert::isnibble(onechar)) {
					cout << "Error: not a valid hex string" << endl;
					return "";	
				}
				tmp+=onechar;
			} else {
				if (tmp!="") {
					binstr+=Convert::hex2byte(tmp);
				}
				tmp="";
			}
		}
		tofind=binstr;
		cout << "looking for " << binstr << endl;
	}
	return *(new string(tofind));
}

long movebytes(unsigned char* bootblock,string editstr) 
{
	string strfrompos="";
	string strmovelen="";
	string strtopos="";
	while (
		  (editstr.substr(0,1)!=" ") 
		&&(editstr.substr(0,1)!="l") 
		&&(editstr.substr(0,1)!="L") 
		&&(editstr!="")) 
	{
		strfrompos+=editstr.substr(0,1);
		editstr=editstr.substr(1,editstr.length()-1);
	}
	if (
		(editstr.substr(0,1)=="l")	
		||(editstr.substr(0,1)=="L"))
	{
		editstr=editstr.substr(1,editstr.length()-1);

		while (
			  (editstr.substr(0,1)!=" ") 
			&&(editstr!="")) 
		{
			strmovelen+=editstr.substr(0,1);
			editstr=editstr.substr(1,editstr.length()-1);
		}
	} else {
		cout << "Syntax: m<from>L<len> <target>" << endl;
		return 0;
	}

	if (
		(editstr.substr(0,1)==" ")	
	)
	{
		editstr=editstr.substr(1,editstr.length()-1);

		while (
			  (editstr.substr(0,1)!=" ") 
			&&(editstr!="")) 
		{
			strtopos+=editstr.substr(0,1);
			editstr=editstr.substr(1,editstr.length()-1);
		}
	} else {
		cout << "Syntax: m<from>L<len> <target>" << endl;
		return 0;
	}
	cout << " strfrompos=" << strfrompos << endl;	
	cout << " strmovelen=" << strmovelen << endl;
	cout << " strtopos=" << strtopos << endl;

	long frompos=(Convert::hex2long(strfrompos)%SECTORSIZE);
	long movelen=(Convert::hex2long(strmovelen)%SECTORSIZE);
	long topos=(Convert::hex2long(strtopos)%SECTORSIZE);
	cout << " pos=" << frompos << endl;	
	cout << " movelen=" << movelen << endl;
	cout << " movepos=" << topos << endl;
	void* x=malloc(movelen);
	memcpy(x,bootblock+frompos,movelen);
	memcpy(bootblock+topos,x,movelen);
	free (x);
	return 0;
}

long editbytes(unsigned char* bootblock,string editstr) 
{
	string strpos="";
	while ((editstr.substr(0,1)!=" ") &&(editstr!="")) {
		strpos+=editstr.substr(0,1);
		editstr=editstr.substr(1,editstr.length()-1);
	}
	if (editstr!="") {
		editstr=editstr.substr(1,editstr.length()-1);
	}
	string binstr=getbinstr(editstr);
	long pos=(Convert::hex2long(strpos)%SECTORSIZE);
	cout << "binstr=" << binstr << " " << pos << endl;
	memcpy(bootblock+pos,binstr.c_str(),binstr.length());
	return 0;
}

void compareblock(hd24raw* rawdevice,unsigned long firstsector,unsigned long endsector,long current)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned char origblock[5120];
	unsigned char destblock[5120];
	string* startoff=Convert::int64tohex((uint64_t)firstsector*512);
	string* endoff=Convert::int64tohex((uint64_t)endsector*512-1);
	cout << "Compare range from offset "<< *startoff << " to offset " << *endoff <<endl;
	delete startoff; 	startoff=NULL;
	delete endoff;		endoff=NULL;
	int anydifs=0;
	for (k=firstsector; k<endsector; k++) 
	{
		rawdevice->readsectors(k,origblock,1);
		rawdevice->readsectors(k-firstsector+current,destblock,1);
		for (i=0;i<SECTORSIZE;i+=16) {
			string strline1="";
			string strline2="";
			int havedifs=0;
			long offset=k*512;
			string* result1=Convert::int32tohex(offset+i);
			strline1+= *result1+" ";
			offset=current*512;
			delete result1;	result1=NULL;
			string* result2=Convert::int32tohex(offset+i);
			strline2+= *result2+" ";
			delete result2;	result2=NULL;
			for (j=0;j<16;j++) 
			{
				if (origblock[i+j]!=destblock[i+j]) 
				{
					string* result=Convert::byte2hex(origblock[i+j]);
					strline1+= *result;
					delete result; result=NULL;
					havedifs=1;
				} else {
					strline1+="  ";
				}
				string *result= Convert::byte2hex(destblock[i+j]) ;
				strline2+= *result;
				delete result; result=NULL;
				if (j==7) {
					strline1+= "-" ;
					strline2+= "-" ;
				} else {
					strline1+= " " ;
					strline2+= " " ;
				}
			}
			strline1+= " ";
			strline2+= " ";
			for (j=0;j<16;j++) {
				strline1+= Convert::safebyte(origblock[i+j]);
				if  (origblock[i+j]!=destblock[i+j]) {
					strline2+= Convert::safebyte(destblock[i+j]);
				} else {
					strline2+=" ";
				}
			}
			if (havedifs==1) {
				anydifs=1;
				cout << strline1 << endl;
				cout << strline2 << endl;
			}
		}
	}
	if (anydifs==0) {
		cout << "Blocks are equal." << endl;
	}
}

long scanforblock(hd24raw* rawdevice,string tofind,unsigned long firstsector,unsigned long endsector,long current) 
{
	unsigned int i;
	unsigned char bootblock[5120];
	tofind=getbinstr(tofind);
	if (tofind=="") return current;

	/* Now that we know what to look for, let us try to find the string.
	 * Note that we can search across sector boundaries because we read
	 * 2 sectors at a time.
	 */
	string* startoff= Convert::int64tohex((uint64_t)firstsector*512);
	string* endoff=Convert::int64tohex((uint64_t)endsector*512-1);
	cout << "Scan range from offset "<< *startoff << " to offset " << *endoff << endl;
	delete startoff; startoff=NULL;
	delete endoff;	endoff=NULL;
	for (i=firstsector; i<endsector; i++) {
		rawdevice->readsectors(i,bootblock,2);
		if ((i%0x1000)==0) {
			string* curroff=Convert::int64tohex((uint64_t)i*0x200);
			cout << "Scan offset " << *curroff << "\r";
			delete curroff;	curroff=NULL;
		}	
		/* check if string found. */
		int j=0;
		for (j=0;j<512;j++) {
			
			if (memcmp((const void *)&bootblock[j],(const void *)tofind.c_str(),(size_t)tofind.length())==0) {
				string* foundoff=Convert::int64tohex((uint64_t)i*512+j);
				cout << endl << "Found on offset " << *foundoff << endl;
				delete foundoff; foundoff=NULL;
				return i;
			}
		}
	}	
	cout << endl << "Not found." << endl;
	return current;
}

void fstfix(unsigned char * bootblock,int fixsize) 
{
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

int parsecommandline(int argc, char ** argv) 
{
	int invalid=0;
	force=0;
	expertmode=0;
	device="";
	headerfilename="";
	for (int c=1;c<argc;c++) {
		string arg=argv[c];
		if (arg.substr(0,2)!="--") {
			if (fileexists(&arg)) {
				device=arg;
				force=1;
				continue;
			}
		}
		if (arg.substr(0,strlen("--dev="))=="--dev=") {
			device=arg.substr(strlen("--dev="));
			continue;
		}
		if (arg.substr(0,strlen("--expert"))=="--expert") {
			expertmode=1;
			continue;
		}
                if (arg.substr(0,strlen(ARGHEADER))==ARGHEADER) {
                        headerfilename=arg.substr(strlen(ARGHEADER));
                        continue;
                }

		if (arg.substr(0,strlen("--force"))=="--force") {
			force=1;
			continue;
		}
		cout << "Invalid argument: " << arg << endl;
		invalid=1;
	}
	return invalid;
}

int main (int argc,char ** argv) 
{
	int invalid=parsecommandline(argc,argv);
	if (invalid!=0) {
		return invalid;
	}

	hd24fs* fsys = NULL;
	if (device=="") {
		fsys=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR);
	} else {
		cout << "Trying to use " << device << " as hd24 device." << endl;
		fsys=new hd24fs((const char*)NULL,hd24fs::MODE_RDWR,&device,(force==1));
	}

	hd24raw* rawdevice = NULL;

	rawdevice=new hd24raw(fsys); //1==1 is true -> no translate
	if (!fsys->isOpen()) {
		cout << "Cannot open hd24 device." << endl;
		delete fsys; fsys=NULL;
		if (rawdevice!=NULL) 
		{
			delete rawdevice;
			rawdevice=NULL;
		}
		return 1;
	}
        if (headerfilename!="")
        {
		if (!(fsys->useheaderfile(headerfilename)))
		{
                	cout << "Couldn't load header file "<<headerfilename << endl;
			delete fsys; fsys=NULL;
			return 1;
		};
        }
	//delete fsys;	//not yet, rawdevice is using this
	cout << "Using device " << *fsys->getdevicename() << endl;
	/* Initialization */
	unsigned char bootblock[5120];
	unsigned long sectornum=0;
	int nodump=0;
	int noread=0;
	int writesec=0;
	int lastsecerror=0;
	unsigned long blockstart=0;
	unsigned long blockend=0;
	writeoffset=0;
	unsigned long checksum=0;
	string userinput;
	string lastsearch="";
	if (expertmode==1) {
		cout << "Expert mode enabled. " << endl;
		cout << "Warning! Disk writes are enabled." << endl;
		cout << "You now have the capability to destroy." << endl;
		cout << "Proceed with extreme caution!" << endl;
	}

try{
	/* try block is intended to make sure hd24 device is closed on exit*/
	string filename="outfile.dmp";

	do {	
		int i;
		int j;
		if (enable_graphmode==1) 
		{
			cout << "Enabling graph mode." << endl;
			graphmode=1;
			enable_graphmode=0;
		}
		if (disable_graphmode==1) {
			cout << "Disabling graph mode." << endl;
			graphmode=0;
			disable_graphmode=0;
		}
		if (writesec==1) {
			cout << "Writing sector " << sectornum << " to raw device." << endl;
			if (expertmode==1) fsys->write_enable();
			rawdevice->writesectors(sectornum,bootblock,1);
			writesec=0;
		}
		if (noread==0) {
			rawdevice->readsectors(sectornum,bootblock,1);
		}
		noread=0;
		long offset=sectornum*512;
		if (nodump==0) {
			cout << "Sector " << *Convert::int32tohex((long)sectornum) << endl;
			
			for (i=0;i<SECTORSIZE;i+=16) {
				string* dummy=Convert::int32tohex(offset+i);
			       	cout << *dummy	<< "  ";
				delete dummy; dummy=NULL;
				for (j=0;j<16;j++) {
					string* dummy= Convert::byte2hex(bootblock[i+j]);
					cout << *dummy;
					if (j==7) {
						cout << "-" ;
					} else {
						cout << " " ;
					}
					delete dummy; dummy=NULL;
				}
				cout << " ";
				for (j=0;j<16;j++) {
					cout << Convert::safebyte(bootblock[i+j]);
				}
				cout << "" << endl;
			}
		}
		nodump=0;
		cout << "-";
		char inputbuf[1025];
		cin.getline(inputbuf,1024);
		userinput="";
		userinput+=inputbuf;
		if (userinput=="") {
			continue;
		}
		while (
		       	(userinput.substr(userinput.length()-1,1)==" ") 
			||(userinput.substr(userinput.length()-1,1)=="\n") 
			||(userinput.substr(userinput.length()-1,1)=="\r") 
		)	
		{
			userinput=userinput.substr(0,userinput.length()-1);
		}
		if (userinput=="+") {
			sectornum++;
			lastsecerror=0;
			if (sectornum>(rawdevice->getlastsectornum(&lastsecerror)))
			{
				sectornum=0;
			}
			continue;
		}
		if (userinput=="gon") {
			if (graphmode==0) {
				enable_graphmode=1;
			}
			continue;
		}
		if ((userinput=="goff")||(userinput=="gof")) {
			if (graphmode==1) {
				disable_graphmode=1;
			}
			continue;
		}
		if (userinput=="wo") {
			writeoffset=0;
			cout << "Writeoffset cleared." << endl;
			nodump=1; // inhibit viewing the sector after this command.
			continue;
		}
		if (userinput.substr(0,2)=="wo") {
			writeoffset=Convert::hex2long(userinput.substr(2,userinput.length()-2));
			string* convwrite=Convert::int32tohex(writeoffset);
			cout << "Writeoffset set to " << *convwrite << " sectors." << endl;
			delete convwrite; convwrite=NULL;
			nodump=1; // inhibit viewing the sector after this command.
			continue;
		}
		if (userinput.substr(0,1)=="+") {
			long sectoadd=Convert::hex2long(userinput.substr(1,userinput.length()-1));
			sectornum+=sectoadd;
			continue;
		}
		if (userinput=="-") {
			if (sectornum>0)
			{
				sectornum--;
			} else {
				sectornum=0;
			}
			continue;
		}
		if (userinput.substr(0,1)=="-") {
			long sectoadd=Convert::hex2long(userinput.substr(1,userinput.length()-1));
			if ((uint32_t)sectoadd>(uint32_t)sectornum)
			{
				sectornum=0;
			} else {
				sectornum-=sectoadd;
			}
			continue;
		}
		if (userinput==".") {
			noread=1;
			continue;
		}
		if ((userinput.substr(0,1)=="?")||(userinput=="help")) {
			cout << "Help for hd24hexview " << VERSION << endl;
			cout << "==============================" << endl;
			cout << "General:" <<endl;
			cout << "   q       quit" << endl;
			cout << "   ?       shows this help" << endl;
			cout << endl;
			cout << "Navigation:" << endl;
			cout << "   +        dump next sector" << endl;
			cout << "   +<n>     increment n sectors, then dump sector"<<endl;
			cout << "   -        dump prev sector" << endl;
			cout << "   -<n>     decrement n sectors, then dump sector"<<endl;
			cout << "   .        re-display current sector" << endl;
			cout << "   d<hex>   dump sector <hex>" << endl;
			cout << "   d-<hex>  dump sector totalnumberofsectors-<hex>" << endl;
			cout << "   d        re-read current sector from disk" << endl;
			cout << "   m<from>l<hexlen> <to>" << endl;
			cout << "            move <hexlen> bytes from offset <from> to <to>" << endl;
			cout << "   o<hex>   dump sector that contains offset <hex>" << endl;
			cout << "   s x y..  scan marked block for byte sequence x y ..." << endl;
			cout << "   s'...'   scan marked block for exact string" << endl;
			cout << "   e<p> xx  edit byte sequence at pos p to xxxx " << endl;
			cout << "   e<p> 'x' edit pos p to string 'x'" << endl;
			cout << endl;
			cout << "Block commands:" << endl;
			cout << "   bb       mark start of sector as block beginning" << endl;
			cout << "   be       mark end of sector as block end" << endl;
			cout << "   bc       block clear" << endl;
			cout << "   diff     compare marked block with current" << endl;
			cout << "   p        paste first sector of marked block to current sector" << endl;
			cout << endl;
			cout << "File commands:" << endl;
			cout << "   n<xx>    set filename to xx" << endl;
			cout << "   n        clear filename" << endl;
			cout << "   w        write marked block to named file/device" << endl;
			cout << "   wo       clear sector write offset for file/device write" << endl;
			cout << "   wo<xx>   set write offset to xx sectors" << endl;
			cout << "   ws       write back current edited sector to disk" << endl;
			cout << "HD24 specific features:" << endl;
			cout << "   fix      reverse byte ordering of 32 bit words" << endl;
			cout << "            for better human readability" << endl;
			cout << "   cs       Calc checksum of selected block" << endl;
			cout << "   scs      Set zero checksum of selected block" << endl;

			nodump=1; // inhibit viewing the sector after this command.
			continue;
		}
		if (userinput.substr(0,2)=="bc") {
			blockstart=0;
			blockend=0;
			cout << "Block selection cleared." << endl;
			nodump=1; // inhibit viewing the sector after this command.
			continue;
		}
		if (userinput.substr(0,2)=="bb") {
			cout << "Block start set." << endl;
			nodump=1; // inhibit viewing the sector after this command.
			blockstart=sectornum;
			continue;
		}
		if (userinput.substr(0,2)=="be") {
			cout << "Block end set." << endl;
			/* dump is startsector..endsector, 
			 * excl. end sector. number of bytes to dump=
			 * (blockend-blockstart)*512
			 */
			nodump=1; // inhibit viewing the sector after this command.
			blockend=sectornum+1; 
			continue;
		}
		if (userinput.substr(0,2)=="cs") {
  			checksum=calcblockchecksum(rawdevice,blockstart,blockend);
			cout << "Block checksum is " << checksum << endl;
			nodump=1;
			noread=1;
			continue;
		}
		if (userinput.substr(0,3)=="scs") {
			if (blockend-1!=sectornum) {
				cout << "Current sector must be block end." << endl;
				nodump=1;
				noread=1;
				continue;
			}
  			unsigned long checksum=calcblockchecksum(rawdevice,blockstart,blockend);
			unsigned long oldchecksum=0;
			oldchecksum+=((unsigned char)(bootblock[511])); oldchecksum=oldchecksum <<8;
			oldchecksum+=((unsigned char)(bootblock[510])); oldchecksum=oldchecksum <<8;
			oldchecksum+=((unsigned char)(bootblock[509])); oldchecksum=oldchecksum <<8;
			oldchecksum+=((unsigned char)(bootblock[508])); 
			oldchecksum-=checksum;
			bootblock[508]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
			bootblock[509]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
			bootblock[510]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
			bootblock[511]=oldchecksum%256; oldchecksum=oldchecksum >> 8;
			
			noread=1;
			continue;
		}
		if (userinput.substr(0,4)=="diff") {
			nodump=1; // inhibit viewing the sector after this command.
			if (blockend==blockstart) {
				cout << "Please set block start/end first." << endl;
				continue;
			}
			compareblock(rawdevice,blockstart,blockend,sectornum);
			continue;	
		}
		if (userinput.substr(0,1)=="s") {
			if (userinput=="s") {
				userinput+=lastsearch;
			} else {
				lastsearch=userinput.substr(1,userinput.length()-1);
			}
			if (blockend==blockstart) {
  			   sectornum=scanforblock(rawdevice,lastsearch,0,0xffffffff,sectornum);
			} else {
  			   sectornum=scanforblock(rawdevice,lastsearch,blockstart,blockend,sectornum);
			}
			continue;
		}
		if (userinput=="version") {
			cout << VERSION << endl;
			nodump=1; // inhibit viewing the sector after this command.
			noread=1;
			continue;
		}
		if (userinput.substr(0,1)=="p") {
			//paste
			if (blockend==blockstart) {
				nodump=1;
				cout << "Mark a block first." << endl;
				continue;
			}
			rawdevice->readsectors(blockstart,bootblock,1);
			noread=1;
			continue;
		}
		if (userinput.substr(0,1)=="e") {
			string editstr=userinput.substr(1,userinput.length()-1);
			editbytes(bootblock,editstr);
			noread=1;
			continue;
		}
		if (userinput.substr(0,1)=="m") {
			string editstr=userinput.substr(1,userinput.length()-1);
			movebytes(bootblock,editstr);
			noread=1;
			continue;
		}
		if (userinput=="d") {
			continue;
		}

		if (userinput.substr(0,2)=="d-") {
			// if we have 10 sectors (0..9), last one is d-1=sector 9
			// so sector num is total number of secs - negnum
			sectornum=rawdevice->getlastsectornum(&lastsecerror)-Convert::hex2long(userinput.substr(2,userinput.length()-2))+1;
			continue;
		}

		if (userinput.substr(0,1)=="d") {
			sectornum=Convert::hex2long(userinput.substr(1,userinput.length()-1));
			continue;
		}
		if (userinput.substr(0,1)=="o") {
			long offset=Convert::hex2long(userinput.substr(1,userinput.length()-1));
			offset-=(offset%512);
			sectornum=offset/512;
		}
		if (userinput=="fix") {
			fstfix(bootblock,512);
			noread=1;
			continue;
		}
		if (userinput.substr(0,1)=="n") {
			int dangerousname=0;
			if (userinput.substr(1,4)=="\\\\.\\") {
				dangerousname=1;
			}
			if (userinput.substr(1,5)=="/dev/") {
				dangerousname=1;
			}
			if (userinput.substr(1,4)=="//./") {
				dangerousname=1;
			}
			if (dangerousname==1) {
				if (expertmode==0) {
					expertmodemessage("Writing to devices");
				} else {
					filename=userinput.substr(1,userinput.length()-1);
					cout << "OK, setting output file to device " << filename << endl;
					cout << "Please take your time to make sure this is really the correct drive."<<endl;
					string strlower=filename;
					int (*pf)(int)=tolower;
					std::transform (strlower.begin(),strlower.end(),strlower.begin(),pf);
					if (
						(strlower=="\\\\.\\physicaldrive0") 
					||	(strlower=="/dev/hda"))
					{
						cout << "CAREFUL!!! " << strlower << " is probably your main system disk!" << endl;
					       	cout << "If you continue you might mess up your computer!!!" << endl;
						cout << "I will not stop you from doing this, so " << endl;
						cout << "MAKE SURE THIS IS REALLY THE DRIVE YOU WANT TO WRITE TO!" << endl;
						cout << "UNLESS ABSOLUTELY SURE, STOP WHAT YOU ARE DOING RIGHT NOW!" << endl;
					}
				}
			} else {
				filename=userinput.substr(1,userinput.length()-1);
			}
			nodump=1; // inhibit viewing the sector after this command.
			if (filename=="") {
				string filename="outfile.dmp";
			}
			cout << "Current output filename is " << filename << endl;

			continue;
		}
		if (userinput=="w") {
			cout << "Writing " << blockend-blockstart << " sectors to " << filename << endl;
			writetofile(rawdevice,filename,blockstart,blockend);
			nodump=1; // inhibit viewing the sector after this command.
			cout << "Done." << endl;
			continue;
		}
		if (userinput=="ws") {
			cout << "user request to write sectors" << endl;
			if (expertmode==1) {
				cout << "Passing on user request to writer" << endl;
				writesec=1;
			} else {
				noread=1;
				expertmodemessage("Writing sectors");
				nodump=1;
			}
		}
		if (userinput=="freesec") {
			nodump=1;
			cout << "Sector " << *Convert::int32tohex((long)rawdevice->getnextfreesector(0xFFFFFFFF)) << endl;
			cout << rawdevice->getnextfreesector(0xFFFFFFFF) << endl;
		}
		if (userinput=="commit") {
			if (expertmode==1) {
				cout << "Commit FS to disk." << endl;
				fsys->commit();
			}
		}
	} while (userinput!="q");

} catch (string e) {
	cout << "Exiting because of an unexpected error." << endl;
	delete rawdevice; rawdevice=NULL;
	delete fsys;	  fsys=NULL;
	return 1;
}
    delete rawdevice; rawdevice=NULL;
    delete fsys;      fsys=NULL;
    return 0;	
}

