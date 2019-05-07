#ifndef __portaudiotest_h__
#define __portaudiotest_h__
#ifndef __uint32
#define __uint32 unsigned long
#endif
class HD24UserInterface
{
private:
	PaStream* portaudiostream;
	bool m_isportaudioinitialized;
	bool havestreamtime;
	PaTime streamtime;
	PaTime timeoffset;
	static int portaudio_process
	(
		const void *inputBuffer, 
		void *outputBuffer, 
		__uint32 nframes, 
		const PaStreamCallbackTimeInfo* timeinfo,
		PaStreamCallbackFlags, 
		void *userData
	);

	PaStreamParameters* inputParameters;
	PaStreamParameters* outputParameters;
public:
	HD24UserInterface();
	bool isportaudioinitialized();
	void portaudioinit();
	void portaudio_transport_start();
};

#endif
