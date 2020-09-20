#include <sharedlibs.h>
#include <FL/fl_message.H>
void SmartLoader::SmartFind(const char* filename,const char* absprogpath,char* result)
{
/* Find the given file in a clever way:
 *    Search the given path first
 *       (on the Mac, perhaps recursively)
 *          Then the environment PATH variable
 *             and finally any predefined library paths */
	result[0]=(char)0;
        cout << "Smartfind..." << endl;
	hd24utils::findfile(filename,absprogpath,result);
	if (strlen(result)!=0) 
	{
	    // found it already
	   return;
	}
        cout << "Not in prog path" << endl; 
	hd24utils::findfile(filename,getenv("PATH"),result);
   	if (strlen(result)!=0) 
   	{
       		// found it already
           	return;
        }
        cout << "Not in PATH" << endl; 

        hd24utils::findfile(filename,DEFAULTLIBPATH,result);
        if (strlen(result)!=0) 
        {
        	// found it already
                return;
        }
        cout << "Not in DEFAULTLIBPATH" << endl; 
        return;
};

PortAudioWrapper::PortAudioWrapper(char* absprogpath)
{
	libloaded=false;
	Pa_Initialize=NULL;
	Pa_Terminate=NULL;
	char result[2048];
        cout << "Finding portaudio" << endl;
	SmartFind(LIBFILE_PORTAUDIO,absprogpath,&result[0]);
        cout << "result = " << result << endl;
	string abslib="";
	abslib+=result;
	abslib+=LIBFILE_PORTAUDIO;
	#if (RECORDERDEBUG==1)
		cout << "Opening from smartpath: " << abslib.c_str() << endl; 
	#endif	       

	LIBHANDLE_T* handle=dlopen(abslib.c_str(),RTLD_NOW);
	if (handle==NULL) {
	#if (RECORDERDEBUG==1)
			cout << "Fail. Opening from default path instead." << endl; 
	#endif	       

		handle=dlopen(LIBFILE_PORTAUDIO,RTLD_NOW);
		if (handle==NULL) {
	#if (RECORDERDEBUG==1)
			cout << dlerror() << endl; 
	#endif	       
		} else {
	#if (RECORDERDEBUG==1)
			cout << dlerror() << endl
			 << "Defining functions from default path." << endl;
	#endif	       
			define_functions(handle);
		
		}
	} else {
	#if (RECORDERDEBUG==1)
			cout << "Defining functions from smart path." << endl;
	#endif	       

		define_functions(handle);
	}
}

PortAudioWrapper::~PortAudioWrapper()
{
	if (libhandle!=NULL) 
	{
		dlclose(libhandle);
	}
}

void PortAudioWrapper::define_functions(LIBHANDLE_T* handle)
{
	if (handle==NULL) return;
	libhandle=handle;
	libloaded=false;
	// Given a handle to a dynamic library, define functions that are in it.

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_Initialize..." << endl;
	#endif
	Pa_Initialize=(PaError (*)(void))dlsym(handle,"Pa_Initialize");

	if (Pa_Initialize==NULL) { fl_message("Error loading Pa_Initialize");	return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_Terminate..." << endl;
	#endif

	Pa_Terminate=(PaError (*)(void))dlsym(handle,"Pa_Terminate");
	if (Pa_Terminate==NULL) { fl_message("Error loading Pa_Terminate");	return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_StartStream..." << endl;
	#endif

	Pa_StartStream=(PaError (*)(PaStream*))dlsym(handle,"Pa_StartStream");
	if (Pa_StartStream==NULL) { fl_message("Error loading Pa_StartStream");	return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_StopStream..." << endl;
	#endif

	Pa_StopStream=(PaError (*)(PaStream*))dlsym(handle,"Pa_StopStream");
	if (Pa_StopStream==NULL) { fl_message("Error loading Pa_StopStream"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_AbortStream..." << endl;
	#endif

	Pa_AbortStream=(PaError (*)(PaStream*))dlsym(handle,"Pa_AbortStream");
	if (Pa_AbortStream==NULL) { fl_message("Error loading Pa_AbortStream"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_CloseStream..." << endl;
	#endif

	Pa_CloseStream=(PaError (*)(PaStream*))dlsym(handle,"Pa_CloseStream");
	if (Pa_CloseStream==NULL) { fl_message("Error loading Pa_CloseStream"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_GetStreamTime..." << endl;
	#endif

	Pa_GetStreamTime=(PaTime (*)(PaStream*))dlsym(handle,"Pa_GetStreamTime");	
	if (Pa_GetStreamTime==NULL) { fl_message("Error loading Pa_GetStreamTime"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_StreamActive..." << endl;
	#endif

	Pa_StreamActive=(PaError (*)(PaStream*))dlsym(handle,"Pa_IsStreamActive");	
	if (Pa_StreamActive==NULL) { 
		Pa_StreamActive=(PaError (*)(PaStream*))dlsym(handle,"Pa_StreamActive");	
	}
	if (Pa_StreamActive==NULL) { 
		fl_message("Error loading Pa_StreamActive"); return; 
	}

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_OpenStream..." << endl;
	#endif
	Pa_OpenStream=(PaError (*)(PaStream**,
			const PaStreamParameters*,
			const PaStreamParameters*,
			double,
			unsigned long,
			PaStreamFlags,
			PaStreamCallback*,
			void*))dlsym(handle,"Pa_OpenStream");
		
	if (Pa_OpenStream==NULL) { fl_message("Error loading Pa_OpenStream"); return; }
	
	#if (RECORDERDEBUG==1)	
	cout << "Define Pa_GetDeviceCount..." << endl;		
	#endif

	Pa_GetDeviceCount=(int (*)(void))dlsym(handle,"Pa_GetDeviceCount");
	if (Pa_GetDeviceCount==NULL) { fl_message("Error loading Pa_GetDeviceCount"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_GetDeviceInfo..." << endl;		
	#endif

	Pa_GetDeviceInfo=(const PaDeviceInfo* (*)(int))dlsym(handle,"Pa_GetDeviceInfo");
	if (Pa_GetDeviceInfo==NULL) { fl_message("Error loading Pa_GetDeviceInfo"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_GetDefaultInputDevice..." << endl;		
	#endif

	Pa_GetDefaultInputDevice=(PaDeviceIndex (*)(void))dlsym(handle,"Pa_GetDefaultInputDevice");
	if (Pa_GetDefaultInputDevice==NULL) { fl_message("Error loading Pa_GetDefaultInputDevice"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_GetDefaultOutputDevice..." << endl;		
	#endif

	Pa_GetDefaultOutputDevice=(PaDeviceIndex (*)(void))dlsym(handle,"Pa_GetDefaultOutputDevice");
	if (Pa_GetDefaultOutputDevice==NULL) { fl_message("Error loading Pa_GetDefaultOutputDevice"); return; }

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_GetVersionText..." << endl;		
	#endif

	Pa_GetVersionText=(const char* (*)(void))dlsym(handle,"Pa_GetVersionText");

	#if (RECORDERDEBUG==1)
	cout << "Define Pa_GetErrorText..." << endl;		
	#endif

	Pa_GetErrorText=(const char* (*)(PaError))dlsym(handle,"Pa_GetErrorText");
	#if (RECORDERDEBUG==1)
	cout << "Define Pa_GetLastHostErrorInfo..." << endl;
	#endif
	Pa_GetLastHostErrorInfo=(const PaHostErrorInfo* (*)(void))dlsym(handle,"Pa_GetLastHostErrorInfo");
	

	#if (RECORDERDEBUG==1)
	PaError initerror=(*(this->Pa_Initialize))();
	#if (RECORDERDEBUG==1)
	cout << "initerror=" <<initerror << endl
	 << "Portaudio version=" <<  (*(this->Pa_GetVersionText))() << endl;
	#endif
	#endif

	#if (RECORDERDEBUG==1)
	int devcount=(*(this->Pa_GetDeviceCount))();
	#if (RECORDERDEBUG==1)
	cout << "Portaudio Device count=" << devcount << endl
	<< "Default Portaudio Input Device =" <<  (*(this->Pa_GetDefaultInputDevice))() << endl
	<< "Default Portaudio Output Device =" <<  (*(this->Pa_GetDefaultOutputDevice))() << endl;
	#endif
	#endif

	libloaded=true;
}

SoundFileWrapper::SoundFileWrapper(char* absprogpath)
{
	sf_open=NULL;
	sf_close=NULL;
	libloaded=false;

	char result[2048];
//	TODO: Was there a reason why this said PORTAUDIO?
//	SmartFind(LIBFILE_PORTAUDIO,absprogpath,&result[0]);
	SmartFind(LIBFILE_SNDFILE,absprogpath,&result[0]);

	string abslib="";
	abslib+=result;
	abslib+=LIBFILE_SNDFILE;
	LIBHANDLE_T* handle=dlopen(abslib.c_str(),RTLD_NOW);
	if (handle==NULL) {
		fl_message("%s",dlerror()); 
	}
	if (handle==NULL) {
		handle=dlopen(LIBFILE_SNDFILE,RTLD_NOW);
		if (handle==NULL) {
			fl_message("%s",dlerror()); 
		} else {
			define_functions(handle);
		}
	} else {
		define_functions(handle);
	}
}

SoundFileWrapper::~SoundFileWrapper()
{
	if (libhandle!=NULL) 
	{
		dlclose(libhandle);
	}

}

void SoundFileWrapper::define_functions(LIBHANDLE_T* handle)
{
	if (handle==NULL) return;
	libhandle=handle;
	// Given a handle to a dynamic library, define functions that are in it.
	sf_open=(SNDFILE*(*)(const char*,int,SF_INFO*))dlsym(handle,"sf_open");
	if (sf_open==NULL) {
		fl_message("Unable to load libsndfile, file transfers won't work.");
	}
	sf_close=(int(*)(SNDFILE*))dlsym(handle,"sf_close");
	sf_read_int=(sf_count_t(*)(SNDFILE*,int*,sf_count_t))dlsym(handle,"sf_read_int");
	sf_write_raw=(sf_count_t(*)(SNDFILE*,const void*,sf_count_t))dlsym(handle,"sf_write_raw");
	sf_write_float=(sf_count_t(*)(SNDFILE*,const void*,sf_count_t))dlsym(handle,"sf_write_float");
	if (sf_open!=NULL) {
		libloaded=true;
	}
}

JackWrapper::JackWrapper(char* absprogpath)
{
	libloaded=false;
	char result[2048];
//	TODO: Was there a reason why this said PORTAUDIO?
//	SmartFind(LIBFILE_PORTAUDIO,absprogpath,&result[0]);
	SmartFind(LIBFILE_JACK,absprogpath,&result[0]);

	string abslib="";
	abslib+=result;
	abslib+=LIBFILE_JACK;
	LIBHANDLE_T* handle=dlopen(abslib.c_str(),RTLD_NOW);
	if (handle==NULL) {
		handle=dlopen(LIBFILE_JACK,RTLD_NOW);
		if (handle!=NULL) {
			define_functions(handle);
		}
	} else {
		define_functions(handle);
	}
}

JackWrapper::~JackWrapper()
{
	if (libhandle!=NULL) 
	{
		dlclose(libhandle);
	}

}

void JackWrapper::define_functions(LIBHANDLE_T* handle)
{
	if (handle==NULL) return;
	libhandle=handle;
	// Given a handle to a dynamic library, define functions that are in it.
	jack_client_new=(jack_client_t*(*)(const char*))dlsym(handle,"jack_client_new");
	jack_set_process_callback=(int(*)(jack_client_t*,JackProcessCallback,void*))dlsym(handle,"jack_set_process_callback");
	jack_on_shutdown=(void (*)(jack_client_t*,void (*function)(void *), void *))dlsym(handle,"jack_on_shutdown");
	jack_get_sample_rate=(jack_nframes_t (*)(jack_client_t*))dlsym(handle,"jack_get_sample_rate");
	jack_port_get_buffer=(void* (*)(jack_port_t*,jack_nframes_t))dlsym(handle,"jack_port_get_buffer");
	jack_transport_query=(jack_transport_state_t (*)(const jack_client_t*,jack_position_t*))dlsym(handle,"jack_transport_query");
	jack_port_register=(jack_port_t* (*)(jack_client_t*,const char*,const char*,unsigned long,unsigned long))dlsym(handle,"jack_port_register");
	jack_activate=(int (*)(jack_client_t*))dlsym(handle,"jack_activate");
	jack_get_current_transport_frame=(jack_nframes_t (*)(const jack_client_t*))dlsym(handle,"jack_get_current_transport_frame");
	jack_transport_locate=(int (*)(jack_client_t*,jack_nframes_t))dlsym(handle,"jack_transport_locate");
	jack_transport_start=(void (*)(jack_client_t*))dlsym(handle,"jack_transport_start");
	jack_transport_stop=(void (*)(jack_client_t*))dlsym(handle,"jack_transport_stop");
	libloaded=true;
}

