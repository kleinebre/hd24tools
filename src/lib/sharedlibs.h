#ifndef __sharedlibs_h__
#define __sharedlibs_h__

#include <xplat_dlfcn.h>
#include <soundlibs.h>
#include <sndfile.h>
#include <hd24utils.h>

class SmartLoader
{
public:
	void SmartFind(const char* filename,const char* absprogpath,char* result);
};

class PortAudioWrapper:SmartLoader
{
/* This class dynamically smart-loads the portaudio library, if available */
private:

	char libloadedstring[1024];

public:

	PaError (*Pa_Initialize)(void);
	PaError (*Pa_Terminate)(void);
	PaError (*Pa_StartStream)(PaStream*);
	PaError (*Pa_StopStream)(PaStream*);
	PaError (*Pa_AbortStream)(PaStream*);
	PaError (*Pa_CloseStream)(PaStream*);
	PaError (*Pa_StreamActive)(PaStream*);
	PaTime (*Pa_GetStreamTime)(PaStream*);
 
	PaError (*Pa_OpenStream)(PaStream **stream,const PaStreamParameters *inputParameters,const PaStreamParameters *outputParameters,double sampleRate,unsigned long framesPerBuffer,PaStreamFlags streamFlags,PaStreamCallback *streamCallback,void* userData);
	int (*Pa_GetDefaultInputDevice)(void);
	int (*Pa_GetDefaultOutputDevice)(void);
	int (*Pa_GetDeviceCount)(void);
	const char* (*Pa_GetVersionText)(void);
	const char* (*Pa_GetErrorText)(PaError ErrorCode);
	const PaDeviceInfo* (*Pa_GetDeviceInfo)(int);
	const PaHostErrorInfo* (*Pa_GetLastHostErrorInfo)(void);

	void define_functions(LIBHANDLE_T* handle);
	PortAudioWrapper(char* absprogpath);
	~PortAudioWrapper();
	int libloaded;
	void* libhandle;

};

class SoundFileWrapper:SmartLoader
{
public:
	/* This class dynamically smart-loads libsndfile, if available */
	SNDFILE* (*sf_open)(const char*,int,SF_INFO*);
	int (*sf_close)(SNDFILE*);
	sf_count_t (*sf_read_int)(SNDFILE*,int*,sf_count_t);
	sf_count_t (*sf_write_raw)(SNDFILE*,const void*,sf_count_t);
	sf_count_t (*sf_write_float)(SNDFILE*,const void*,sf_count_t);
	void define_functions(LIBHANDLE_T* handle);
	SoundFileWrapper(char* absprogpath);
	~SoundFileWrapper();
	int libloaded;
	void* libhandle;
};

class JackWrapper:SmartLoader
{
public:
	jack_client_t* (*jack_client_new)(const char*);
	int (*jack_set_process_callback)(jack_client_t*,JackProcessCallback,void*);
	void (*jack_on_shutdown)(jack_client_t*,void (*function)(void *), void *);
	jack_nframes_t (*jack_get_sample_rate)(jack_client_t*);
	void* (*jack_port_get_buffer)(jack_port_t*,jack_nframes_t);

	jack_transport_state_t (*jack_transport_query)(const jack_client_t*,jack_position_t*);
	jack_port_t* (*jack_port_register)(jack_client_t*,const char*,const char*,unsigned long,unsigned long);
	int (*jack_activate)(jack_client_t*);
	jack_nframes_t (*jack_get_current_transport_frame)(const jack_client_t*);
	int (*jack_transport_locate)(jack_client_t*,jack_nframes_t);
	void (*jack_transport_start)(jack_client_t*);
	void (*jack_transport_stop)(jack_client_t*);
	void define_functions(LIBHANDLE_T* handle);
	JackWrapper(char* absprogpath);
	~JackWrapper();
	int libloaded;
	void* libhandle;

};

#endif
