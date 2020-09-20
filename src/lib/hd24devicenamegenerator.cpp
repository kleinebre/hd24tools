#include "hd24devicenamegenerator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef DARWIN
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <sys/types.h>
#include <dirent.h>
#include <hd24utils.h>
#include <memutils.h>
int endswith(const char* str, const char* end)
{
#if (DEVGENDEBUG==1)
cout << "endswith("<<str<<","<<end<<")"<<endl;
#endif
	if (str==NULL) return false;
	if (end==NULL) return false;
	size_t str_len = strlen(str);
	size_t end_len = strlen(end);
	if (str_len<end_len) return false;
	
	const char *str_end = str + str_len - end_len;
	return strcmp(str_end, end) == 0;
}

uint32_t hd24devicenamegenerator::hd24filecount(const char* dirname)
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::hd24filecount("<<dirname<<")"<< endl;
#endif
	DIR* Dir;
	struct dirent *DirEntry;
	Dir = opendir(dirname);
	
	int foundcount=0;
	do
	{
		DirEntry=readdir(Dir);
		if (!DirEntry) break;
		if ( hd24utils::isdir(DirEntry->d_name)) continue;
	
		if (!(  endswith(DirEntry->d_name,".h24")
		      ||endswith(DirEntry->d_name,".H24"))) continue;
			 
		foundcount++;
		
		if (filelist==NULL)
		{
			filelist=new vector<string>();
		}
		string* fname=new string(dirname);
		*fname+=DirEntry->d_name;
		filelist->push_back(*fname);
#if (DEVGENDEBUG==1)		
		cout <<"Found a File : " << *fname << endl;
#endif		
		delete fname;
		
		// validity of file image will be extablished by hd24fs device scan
	} while(1);
        closedir(Dir);
	return foundcount;
}

void hd24devicenamegenerator::initvars()
{
	imagespath=NULL;
	filelist=NULL;
	filecount=0; // cache for getnumberoffiles
}

void hd24devicenamegenerator::clearfilelist()
{
	if (filelist==NULL)
	{
		return;
	}
	while (filelist->size()!=0)
	{
		filelist->pop_back();
	}
	delete filelist;
	filelist=NULL;
}

hd24devicenamegenerator::~hd24devicenamegenerator()
{
	if (imagespath!=NULL)
	{
		memutils::myfree("~hd24devnamgen-imagespath",imagespath);
		imagespath=NULL;
	}
	
	clearfilelist();
}
hd24devicenamegenerator::hd24devicenamegenerator()
{
	initvars();
}

const char* hd24devicenamegenerator::imagedir()
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::imagedir()"<<endl;
#endif
	return imagespath;	
}

const char* hd24devicenamegenerator::imagedir(const char* newdir)
{
if (newdir==NULL)
{
#if (DEVGENDEBUG==1)
	cout << "hd24devicenamegenerator::imagedir(NULL)"<< endl;
#endif
}
else
{
#if (DEVGENDEBUG==1)
	cout << "hd24devicenamegenerator::imagedir("<<newdir<<")"<< endl;
#endif
}
	
	filecount=0;
	clearfilelist();
	
	if (imagespath!=NULL)
	{
		memutils::myfree("free old imagespath",imagespath);
		imagespath=NULL;
	}

	if (newdir!=NULL)
	{
		imagespath=(char*)memutils::mymalloc("devnamgen::imagespath",strlen(newdir)+1,1);
		if (imagespath!=NULL)
		{
			strncpy(imagespath,newdir,strlen(newdir)+1);		
		}
#if (DEVGENDEBUG==1)		
		cout << "new images path is " ;
#endif
		if (imagespath==NULL)
		{
#if (DEVGENDEBUG==1)		
			cout << "NULL" << endl;
#endif		
		} else {
#if (DEVGENDEBUG==1)		
			cout << imagespath << endl;
#endif		
		}		
	}
	
	return (const char*)imagespath;
}

uint32_t hd24devicenamegenerator::getnumberoffiles()
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::getnumberoffiles()"<< endl;
#endif
	// Files in the image directory will also be considered devices.
	if (filecount==0)
	{
		if (imagedir()==NULL)
		{
			return 0;
		}
		// retrieve a list of .h24 image files.
		filecount=hd24filecount(imagedir());
	}
	return filecount;
}

uint32_t hd24devicenamegenerator::getnumberofsysdevs()
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::getnumberofsysdevs()"<< endl;
#endif
#ifdef LINUX
	// For linux the total number of names is 26x /dev/hd* + 26x /dev/sd*
	return 52;
#endif
#ifdef WINDOWS
	return 128;
#endif
#ifdef DARWIN
	return 100;
#endif
}

uint32_t hd24devicenamegenerator::getnumberofnames()
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::getnumberofnames()"<< endl;
#endif
	uint32_t filecount=this->getnumberoffiles();
	uint32_t devcount=this->getnumberofsysdevs();
#if (DEVGENDEBUG==1)	
	cout << "return: filecount="<<filecount<<", devcount="<<devcount<<endl;
#endif
	return filecount+devcount;
}

const char* hd24devicenamegenerator::getfilename(uint32_t filenum)
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::getfilename("<<filenum<<")"<< endl;
#endif
	// Given the list of files, this will return the Nth name (base 0).
	if (filelist==NULL)
	{
		return "";
	}
	if (filelist->size()==0)
	{
		return "";
	}
	return filelist->at(filenum).c_str();
}

#ifdef LINUX
string* hd24devicenamegenerator::getdevicename(uint32_t devicenumber)
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::getdevicename("<<devicenumber<<")"<< endl;
#endif
	if (devicenumber>=this->getnumberofsysdevs())
	{
		return new string(getfilename(devicenumber-this->getnumberofsysdevs()));
	}
	string* devname;
	uint32_t devgroup = devicenumber;
	devicenumber = (devicenumber%26);
	devgroup -= devicenumber;
	devgroup /= 26;
	
	switch (devgroup) 
	{
		case 0: 
			devname = new string("/dev/hd"); 
			break;
		case 1: 
			devname = new string("/dev/sd"); 
			break;
		default: 
			return new string("");
	}
	
	char devletter = (char) ('a' + devicenumber);
	*devname += devletter;
	
	return devname;
}
#endif

#ifdef WINDOWS

string* hd24devicenamegenerator::getdevicename(uint32_t devicenumber)
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::getdevicename("<<devicenumber<<")"<< endl;
#endif	
	if (devicenumber>=this->getnumberofsysdevs())
	{
		return new string(getfilename(devicenumber-this->getnumberofsysdevs()));
	}

	string* devname = new string("//./PHYSICALDRIVE");
	string* devnum = Convert::int2str(devicenumber);
	*devname += *devnum;
	delete devnum;
	return devname;
}
#endif

#ifdef DARWIN
string* hd24devicenamegenerator::getdevicename(uint32_t devicenumber)
{
#if (DEVGENDEBUG==1)
cout << "hd24devicenamegenerator::getdevicename("<<devicenumber<<")"<< endl;
#endif	
	if (devicenumber>=this->getnumberofsysdevs())
	{
		return new string(getfilename(devicenumber-this->getnumberofsysdevs()));
	}
	
	string* devname = new string("/dev/disk");
	string* devnum = Convert::int2str(devicenumber);
	*devname += *devnum;
	delete devnum;
	return devname;
}
#endif

#ifdef LINUX
const char* hd24devicenamegenerator::DEVICEPREFIX="/dev/";
#endif
#ifdef DARWIN
const char* hd24devicenamegenerator::DEVICEPREFIX="/dev/";
#endif
#ifdef WINDOWS
const char* hd24devicenamegenerator::DEVICEPREFIX="//./PHYSICALDRIVE";
#endif

