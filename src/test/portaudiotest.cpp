using namespace std;
#include <iostream>
#include <portaudio.h>
#include "portaudiotest.h"
#define PA_FRAMESPERBUF 512
#define __uint32 unsigned long

int HD24UserInterface::portaudio_process(
	const void *inputBuffer, 
	void *outputBuffer, 
	__uint32 nframes, 
	const PaStreamCallbackTimeInfo* timeinfo,
	PaStreamCallbackFlags, 
	void *userData)
{
	HD24UserInterface* mythis=(HD24UserInterface*)userData;
	if (!mythis->havestreamtime)
	{
		mythis->streamtime=0;
		mythis->timeoffset=timeinfo->currentTime;
		mythis->havestreamtime=true;	
	}
	mythis->streamtime+=nframes;
	cout << ((mythis->streamtime)/48000) << " nframes=" << nframes <<" streamtime=" << timeinfo->currentTime-(mythis->timeoffset) << endl;
	float sval=1;
	for (int i=0;i<nframes;i++)
	{
		if ((i%16)==0) sval=-sval;
		((float*)(outputBuffer))[i]=sval;
	}
	return paContinue;
}


void HD24UserInterface::portaudioinit()
{
	cout << "portaudioinit" << endl;
	PaError err;
	inputParameters=new PaStreamParameters;
	outputParameters=new PaStreamParameters;
	err=Pa_Initialize();
	if (err!=paNoError) 
	{ 
		Pa_Terminate(); 
		return; 
	}
	m_isportaudioinitialized=true;
}

HD24UserInterface::HD24UserInterface()
{
	m_isportaudioinitialized=false;
	portaudiostream=NULL;
	return;
}

bool HD24UserInterface::isportaudioinitialized()
{
	return m_isportaudioinitialized;
}

void HD24UserInterface::portaudio_transport_start()
{

	cout << "portaudio transport start" << endl; 

	if (!isportaudioinitialized())
	{
		PaError err=Pa_Initialize();
		if (err != paNoError) 
		{
			cout << "Cannot initialize portaudio- exiting." << endl;	
		}
	}
	
	if (isportaudioinitialized() && (portaudiostream!=NULL)) 
	{
		cout << "already have stream- done starting" << endl; 
		return;
	}

	PaDeviceIndex indevice=Pa_GetDefaultInputDevice();
	bzero(inputParameters,sizeof(*inputParameters));
	inputParameters->device=indevice;
	inputParameters->channelCount=1;
	inputParameters->sampleFormat=paFloat32;
	inputParameters->suggestedLatency = Pa_GetDeviceInfo( inputParameters->device )->defaultLowInputLatency;
	inputParameters->hostApiSpecificStreamInfo = NULL;
	
	cout << "Input params set" << endl; 
	cout << "Device=" << inputParameters->device << endl;
	cout << "Channelcount=" << inputParameters->channelCount << endl;
	cout << "sampleFormat=" << inputParameters->sampleFormat << endl;
	cout << "suggestedlatency=" << inputParameters->suggestedLatency << endl;
	cout << "================================="<<endl;
	bzero(outputParameters,sizeof(*outputParameters));
	PaDeviceIndex outdevice=Pa_GetDefaultOutputDevice();
	outputParameters->device=outdevice;
	outputParameters->channelCount=1;
	outputParameters->sampleFormat=paFloat32;
	outputParameters->suggestedLatency = Pa_GetDeviceInfo( outputParameters->device )->defaultLowOutputLatency;
	outputParameters->hostApiSpecificStreamInfo = NULL;

	cout << "Output params set" << endl; 
	cout << "Device=" << outputParameters->device << endl;
	cout << "Channelcount=" << outputParameters->channelCount << endl;
	cout << "sampleFormat=" << outputParameters->sampleFormat << endl;
	cout << "suggestedlatency=" << outputParameters->suggestedLatency << endl;
	cout << "================================="<<endl;

	double samrate=48000;
	cout << "trying samerate="<<samrate<<endl;
	cout << "framesperbuf=" << PA_FRAMESPERBUF << endl;

	PaError err=Pa_OpenStream(
		&portaudiostream,
		this->inputParameters, 
		this->outputParameters,
		samrate,
		PA_FRAMESPERBUF, /* frames per buffer */
		paClipOff | paDitherOff,
		portaudio_process,
		(void*)this);
	this->havestreamtime=false;

	if (err!=paNoError) 
	{
 		cout << "Error opening stream" << endl; 
		Pa_Terminate(); 
		return; 
	}

	cout << "Stream opened, going to start it now..." << endl; 

	err=Pa_StartStream(portaudiostream);
	
	if (err!=paNoError) 
	{ 
		cout << "Error starting stream" << endl; 
		Pa_Terminate(); 
		return; 
	}

	cout << "Stream started" << endl; 

	return;
}

int main()
{
	HD24UserInterface* ui=new HD24UserInterface();
	ui->portaudioinit();
	ui->portaudio_transport_start();
	int blah;
	cin >> blah;
		
}

