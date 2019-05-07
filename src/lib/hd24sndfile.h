#ifndef __hd24sndfile_h__
#define __hd24sndfile_h__
class SoundFileWrapper;
#include <config.h>
#include <fstream>
using std::ofstream;
#include <sharedlibs.h>
#include <sndfile.h>

class hd24sndfile
{
private:

	SF_INFO* sfinfo;
	int _handletype;
	SoundFileWrapper* soundfile; /* runtime loading wrapper for libsndfile */
	int sf_format;
public:
	SNDFILE* sndfilehandle;
	ofstream *outfilehandle;

	hd24sndfile(int p_format,SoundFileWrapper* p_soundfile);
	~hd24sndfile();
	int handletype();
	void* handle();
	void handle(SNDFILE* newhandle);
	void handle(ofstream* newhandle);
	void handle(void* newhandle,int htype);
	void open(const char* filename,int filemode,SF_INFO* infoblock,
			SoundFileWrapper* _soundfile);
	void open(const char* filename,int filemode,SF_INFO* infoblock);
	void writerawbuf(unsigned char* buf,__uint32 subblockbytes);
	void close();
	void write_float(float* buf,__uint32 frames);
};

#endif
