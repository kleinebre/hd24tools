#include "hd24sndfile.h"
#include <sndfile.h>
#include <fstream>
#include <iostream>
#include <convertlib.h>
using std::ofstream;
using std::cout; // PRAGMA allowed
using std::endl;
hd24sndfile::hd24sndfile(int p_format,SoundFileWrapper* p_soundfile)
{
	sndfilehandle=(SNDFILE*)NULL;
	outfilehandle=(ofstream*)NULL;
	_handletype=0; // default is 0=libsndfile, 1=native file
	sf_format=p_format;
	soundfile=p_soundfile;
}

hd24sndfile::~hd24sndfile()
{
}

int hd24sndfile::handletype()
{
	return _handletype;
}

void* hd24sndfile::handle()
{
	if (_handletype==0)
	{
		return (void*)sndfilehandle;
	}
	return (void*)outfilehandle;
}

void hd24sndfile::handle(SNDFILE* newhandle)
{
	sndfilehandle=newhandle;
	_handletype=0;
}

void hd24sndfile::handle(ofstream* newhandle)
{
	outfilehandle=newhandle;

	_handletype=1;
	// TODO: Write empty header (for filling in later)
}

void hd24sndfile::handle(void* newhandle,int htype)
{
	_handletype=htype;
	if (htype==1) {
		outfilehandle=(ofstream*)newhandle;
		return;
	}

	sndfilehandle=(SNDFILE*)newhandle;
}

void hd24sndfile::open(const char* filename,int filemode,SF_INFO* infoblock,SoundFileWrapper* _soundfile)
{
	_handletype=0; // soundfile
	soundfile=_soundfile;
	sfinfo=infoblock;
	sndfilehandle=soundfile->sf_open(filename,filemode,infoblock);
	outfilehandle=NULL;

	if (!sndfilehandle) 
	{
#if (HD24SNDFILEDEBUG==1)
		cout << "Problem opening file with the following specs: " << endl
		<< "rate=" << infoblock->samplerate << endl
		<< "frames=" << infoblock->frames << endl
		<< "channels=" << infoblock->channels << endl
		<< "sections=" << infoblock->sections << endl
		<< "seekable=" << infoblock->seekable << endl
		<< "format=" << infoblock->format << endl;
#endif
	}
}

void hd24sndfile::open(const char* filename,int filemode,SF_INFO* infoblock)
{
	_handletype=1; // iostream
	sfinfo=infoblock;
	sndfilehandle=NULL;
	outfilehandle=new ofstream(filename,ios_base::out|ios_base::trunc|ios_base::binary);

	if ((sf_format & 0xFF0000) ==SF_FORMAT_WAV)
	{
		char buf[44]={
		'R','I','F','F',	0,0,0,0,	'W','A','V','E', 'f','m','t',' ',

                //                      FMT (was: 1,0 - WAVE_FORMAT_PCM); 0xFFFE=WAVE_FORMAT_EXTENSIBLE
		16,0,0,0,        	1,0,   1,0,  	0,0,0,0,      	 0,0,0,0,

		3,0,24,0,      		'd','a','t','a',    0,0,0,0
		};

		// samplerate follows
		long rate=sfinfo->samplerate; // FIXME: Get proper rate from sfinfo
		buf[24]=(char)((rate)%256);
		buf[25]=(char)(((rate)>>8)%256);	
		buf[26]=(char)(((rate)>>16)%256);	
		buf[27]=(char)(((rate)>>24)%256);	

		rate*=3;
		buf[28]=(char)((rate)%256);
		buf[29]=(char)(((rate)>>8)%256);	
		buf[30]=(char)(((rate)>>16)%256);	
		buf[31]=(char)(((rate)>>24)%256);	
	
		outfilehandle->write(buf,44);
		return;
	
	}

	if ((sf_format & 0xFF0000) ==SF_FORMAT_AIFF)
	{
		char buf[54]={
		'F','O','R','M',	
		0,0,0,0, /* Bytes that follow in this file */
		'A','I','F','F', 'C','O','M','M',
		0,0,0,18, /* chunksize */
		0,1, /*numchannels*/ 
		0,0,0,0, /*sampleframes */ 
		0,24, /* Samplesize in bits */
		0,0,0,0,0,0,0,0,0,0, /* sample rate as 80 bit IEEE float */
		'S','S','N','D',
		0,0,0,0, /* sound block datalen, to be filled afterwards */
		0,0,0,0, /* offset */
		0,0,0,0 /* blocksize */ /* -> data follows */
		};

		// samplerate follows
		long rate=sfinfo->samplerate; // FIXME: Get proper rate from sfinfo
		Convert::setfloat80((unsigned char*)buf,28,rate);
	
		outfilehandle->write(buf,54);
		return;
	
	}
}
void hd24sndfile::writerawbuf(unsigned char* buf,uint32_t subblockbytes)
{
#if (HD24SNDFILEDEBUG==1)
	cout << "writerawbuf(buf,subblockbytes="<<subblockbytes<<")"<<endl;
#endif
	// Byte order swapping for AIFF files
	if ((sf_format & 0xFF0000) ==SF_FORMAT_AIFF)
	{
		// and we're doing 24 bits.
		for (uint32_t i=0;i<subblockbytes;i+=3)
		{
			char a=buf[i];
			buf[i]=buf[i+2];
			buf[i+2]=a;
		}
	}

	// maybe we want to do 24->16 bit conversion here

	if (sndfilehandle != NULL)
	{
	#if (HD24SNDFILEDEBUG==1)
	cout << "soundfile=" << soundfile <<endl;
	#endif
	#if (HD24SNDFILEDEBUG==1)
	cout << "soundfile->sf_write_raw(sndfilehandle,buf,subblockbytes="<<subblockbytes<<");" << endl;
	#endif
		soundfile->sf_write_raw(sndfilehandle,buf,subblockbytes);
		return;
	}
	if (outfilehandle!=NULL)
	{
		outfilehandle->write((const char*)buf,subblockbytes);
	}
}

void hd24sndfile::close()
{

	if (sndfilehandle != NULL)
	{
		soundfile->sf_close(sndfilehandle);
		return;
	}
	if (outfilehandle == NULL) return;


	switch (sf_format & 0xFF0000)
	{
	    case SF_FORMAT_WAV:
	    {
		// set sample rate, len, etc. in header
		long pos=outfilehandle->tellp();

		// Optional zero padding for word alignment
		// (#1666)
		
		if ((pos%2)==1)
		{
			unsigned char zerobuf[3]={0,0,0};
			outfilehandle->write((const char*)zerobuf,1);
		} 
		char bufword[4];
		bufword[0]=(char)((pos-8)%256);
		bufword[1]=(char)(((pos-8)>>8)%256);	
		bufword[2]=(char)(((pos-8)>>16)%256);	
		bufword[3]=(char)(((pos-8)>>24)%256);	
		outfilehandle->seekp(4);	
		outfilehandle->write(bufword,4);

		bufword[0]=(char)((pos-44)%256);
		bufword[1]=(char)(((pos-44)>>8)%256);	
		bufword[2]=(char)(((pos-44)>>16)%256);	
		bufword[3]=(char)(((pos-44)>>24)%256);	
		outfilehandle->seekp(40);	
		outfilehandle->write(bufword,4);
	
		outfilehandle->close();
		break;
	    }
	    case SF_FORMAT_AIFF:
	    {
		long pos=outfilehandle->tellp();
		char bufword[4];
		bufword[3]=(char)((pos-8)%256);
		bufword[2]=(char)(((pos-8)>>8)%256);	
		bufword[1]=(char)(((pos-8)>>16)%256);	
		bufword[0]=(char)(((pos-8)>>24)%256);	
	
		outfilehandle->seekp(4);	
		outfilehandle->write(bufword,4);
	
		long chunksize=(pos-46);	
		long sambytes=chunksize-8;

	
		long samframes=sambytes/3; // Bytes per sample. TODO: Not hardcode this 
		bufword[3]=(char)(samframes%256);
		bufword[2]=(char)((samframes>>8)%256);	
		bufword[1]=(char)((samframes>>16)%256);	
		bufword[0]=(char)((samframes>>24)%256);	
		outfilehandle->seekp(22);	
		outfilehandle->write(bufword,4);

	
		bufword[3]=(char)(chunksize%256);
		bufword[2]=(char)((chunksize>>8)%256);	
		bufword[1]=(char)((chunksize>>16)%256);	
		bufword[0]=(char)((chunksize>>24)%256);	
		outfilehandle->seekp(42);	
		outfilehandle->write(bufword,4);


		    
		outfilehandle->close();
		break;
	    
	    }
	    default:
	    {
		outfilehandle->close();
		break;
	    
	    }    
	}
}

void hd24sndfile::write_float(float* buf,uint32_t frames)
{
	if (sndfilehandle != NULL)
	{
		soundfile->sf_write_float(sndfilehandle,buf,frames);
	}
	return;
}


