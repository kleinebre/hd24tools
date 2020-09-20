#ifndef __portaudiotest_h__
#define __portaudiotest_h__
#include <stdint.h>
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
		uint32_t nframes, 
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
