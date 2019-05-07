#ifndef __hd24devnamegenerator_h__
#define __hd24devnamegenerator_h__

#include <config.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include "convertlib.h"

using namespace std;

class hd24devicenamegenerator 
{
	private:
		char* imagespath; // path of device images
		vector<string>* filelist;
		__uint32 getnumberoffiles();
		__uint32 filecount; // cache for getnumberoffiles
		__uint32 getnumberofsysdevs();
		__uint32 hd24filecount(const char* imagedir);
		void initvars();
		const char* getfilename(__uint32 filenum);
		void clearfilelist();
	public:
		~hd24devicenamegenerator();
		hd24devicenamegenerator();
		
		__uint32 getnumberofnames();
		string* getdevicename(__uint32 number);
		
		const char* imagedir();
		
		/*
		   Setting the image dir resets the filecount to 0
		   which means the next call will actually count the image
		   files in the image dir (a rather heavy operation), and
		   caches the result in filecount.
		   imagedir(imagedir()) will force a filecount reset.
		   
		   If the image dir is NULL, only system devices are returned.
		*/
		
		const char* imagedir(const char* newdir);
		/*
		  returns NULL on alloc error, const char*=dirname otherwise
		*/
		static const char* DEVICEPREFIX;
		  
};

#endif

