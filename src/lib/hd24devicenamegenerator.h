#ifndef __hd24devnamegenerator_h__
#define __hd24devnamegenerator_h__

#include <stdint.h>
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
		uint32_t getnumberoffiles();
		uint32_t filecount; // cache for getnumberoffiles
		uint32_t getnumberofsysdevs();
		uint32_t hd24filecount(const char* imagedir);
		void initvars();
		const char* getfilename(uint32_t filenum);
		void clearfilelist();
	public:
		~hd24devicenamegenerator();
		hd24devicenamegenerator();
		
		uint32_t getnumberofnames();
		string* getdevicename(uint32_t number);
		
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

