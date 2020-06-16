#ifndef __hd24transferengine_h__
#define __hd24transferengine_h__

class MixerControl;

#include <stdint.h>
#include <hd24fs.h>
#include <hd24sndfile.h>
#include <ui_mixer.h>
#include <smpte.h>
class hd24transferjob
{
friend class hd24transferengine;
private:
	long long llsizelimit; /* to hold max filesize for autosplit */
	int mixleft;
	int mixright;
	int m_selectedformat;
        hd24fs* jobsourcefs;
        hd24song* jobsourcesong;
        hd24fs* jobtargetfs;
        hd24song* jobtargetsong;
        string* m_filenameformat;
	uint32_t m_startoffset;
	uint32_t m_endoffset;
        uint32_t m_reeloffset;
	SNDFILE** filehandle; /* todo: implement as hd24sndfile* */
	char** filepath; 	/* one full file path/name per track
				   mainly to be used for export to hd24 */
	bool have_smpte;
	int m_trackaction[24];
	SMPTEgenerator* smptegen;
public:
	
	string* m_projectdir;        
	int usecustomrate; // 1 to stamp export with custom sample rate
			   // 0 to keep song sample rate
	uint32_t stamprate; //sample rate to use if usecustomrate==1
	int wantsplit;
	int* trackselected;
	hd24transferjob();
	~hd24transferjob();
	void init_vars();
		
	void sizelimit(int64_t llsizelimit);
	int64_t sizelimit();
        
        uint32_t startoffset();
        uint32_t endoffset();
        void startoffset(uint32_t newoff);
        void endoffset(uint32_t newoff);
        void reeloffset(uint32_t newoff);
        uint32_t reeloffset();
	void projectdir(const char* projectdir);
	const char* projectdir();
	void selectedformat(int format);
	int selectedformat();
        hd24fs* sourcefs();
        hd24fs* targetfs();
        hd24song* sourcesong();
        hd24song* targetsong();
        void sourcefs(hd24fs* fs);
        void targetfs(hd24fs* fs);
        void sourcesong(hd24song* song);
        void targetsong(hd24song* song);
	char* sourcefilename(int base1tracknum);
	void sourcefilename(int base1tracknum,const char* name);
	void trackaction(int base1tracknum,int action);
	int trackaction(int base1tracknum);
        void filenameformat(string* filenameformat);
        string* filenameformat();        
};

class hd24transferengine
{
private:
        void* ui;
	
	MixerControl* transfermixer;
	hd24transferjob* job;

	uint32_t songnum;
	uint32_t totsongs;
	int64_t totbytestotransfer;
	int64_t totbytestransferred;

	int prefix;	
	int trackspergroup;
	int* audiobuf[24]; /* for libsndfile int reading from file */
	bool isfirstchanofgroup[24]; /* for exporting stereo pairs/groups of channels */
	bool islastchanofgroup[24]; /* for exporting stereo pairs/groups of channels */
	
	void setstatus(void* ui,string* message,double percent);
        void generatetimestamp();

	void openbuffers(unsigned char** audiobuf,uint32_t channels,uint32_t bufsize);
	double update_eta(const char* etamessage,uint64_t translen,
				uint64_t currbytestransferred,
				uint64_t totbytestransferred,
				uint64_t totbytestotransfer,double oldpct);
	void closebuffers(unsigned char** audiobuf,uint32_t channels);
	void writerawbuf(hd24sndfile* filehandle,unsigned char* buf,long subblockbytes);
	void flushbuffer(hd24sndfile** filehandle,unsigned char** buffer,uint32_t flushbytes,uint32_t channels);


	bool overwritegivesproblems(hd24song* thesong,int partnum);
	bool confirmfileoverwrite(); // perform interactive/GUI callback 
				     // to confirm if file overwriting is OK
	bool anyfilesexist(hd24song* thesong);
	void transfer_in_progress(bool active);

        time_t jobtimestamp; // these are for benchmarking the transfer	
        time_t transferstarttime; // these are for benchmarking the transfer	
	time_t endtime;

	/* Regarding populating list of supported file formats 
           TODO: Move to a separate class?
	*/
	int formatcount;
	void populate_formatlist();
	vector<string>* m_format_outputextension;
	vector<string>* m_format_shortdesc;
	int m_format_outputformat[100];
	int m_format_outputchannels[100];	
	int m_format_bitdepth[100];
	bool m_format_sndfile[100];
        string* m_lasterror;
        string* strdatetime;
	uint32_t requiredsonglength_in_wamples(hd24song* tsong,SF_INFO* sfinfoin);
	void _generate_smpte(uint32_t samplesperblock,uint32_t samplesinblock,
				uint32_t samplenum,unsigned char* audiodata);
	void _generate_silence(uint32_t samplesperblock,uint32_t samplesinblock,
				uint32_t samplenum,unsigned char* audiodata);
	void _prepare_audio(uint32_t samplesperblock,uint32_t samplesinblock,
				uint32_t samplenum,unsigned char* audiodata,
				SF_INFO* sfinfoin, int* sfeof);
	uint32_t _lengthen_song_as_needed(hd24song* tsong,SF_INFO* sfinfo); // returns wamples
public:
        SoundFileWrapper* soundfile;
        bool (*uiconfirmfunction)(void* ui,const char*);
        void (*setstatusfunction)(void* ui,const char*,double progress_pct);
        void reeloffset(uint32_t newoff);
        uint32_t reeloffset();
        
        void set_ui(void* p_ui);
        
	hd24transferengine(); 
	~hd24transferengine();
        
        void mixer(MixerControl* m_mixer);
        MixerControl* mixer();
        
        void lasterror(const char* errormessage);
        string* lasterror();
        
	int supportedformatcount();
	int format_outputchannels(int i);
	const char* getformatdesc(int formatnum);
        	
        void trackselected(uint32_t base0tracknum,bool select);
        bool trackselected(uint32_t base0tracknum);
        
        void mixleft(bool select);
        bool mixleft();
        void mixright(bool select);
        bool mixright();
        
	void init_vars(); 
	void prepare_transfer_to_pc(
		uint32_t songnum, uint32_t totsongs,
		int64_t totbytestotransfer,
		int64_t totbytestransferred,
		int wantsplit,uint32_t prefix);

	int64_t transfer_to_pc();
	int64_t transfer_to_hd24();

	bool openinputfiles(SNDFILE** filehandle,SF_INFO* sfinfoin,uint32_t channels);
	void closeinputfiles(SNDFILE** filehandle,uint32_t channels);
	bool dontopenoutputfiles(hd24sndfile** filehandle,uint32_t channels,uint32_t partnum,int prefix); //HACK
	
	bool openoutputfiles(hd24sndfile** filehandle,uint32_t channels,uint32_t partnum,int prefix);
	void closeoutputfiles(hd24sndfile** filehandle,uint32_t channels);
	string* generate_filename(int tracknum,int partnum,int prefix);
	void sizelimit(int64_t llsizelimit);
	int64_t sizelimit();
	void projectdir(const char* projectdir);
	const char* projectdir();
	
	void selectedformat(int format);
	int selectedformat();
        
        void sourcesong(hd24song* newsong);
        hd24song* sourcesong();
        void targetsong(hd24song* newsong);
        hd24song* targetsong();
        
        uint32_t startoffset();
        uint32_t endoffset();
        void startoffset(uint32_t newoff);
        void endoffset(uint32_t newoff);
	char* sourcefilename(int base1tracknum);
	void sourcefilename(int base1tracknum,const char* name);
	void trackaction(int base1tracknum,int action);
	int trackaction(int base1tracknum);
        void filenameformat(string* filenameformat);
        string* filenameformat();
};

#endif
