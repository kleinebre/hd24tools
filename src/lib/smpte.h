#ifndef __smpte_h__
#define __smpte_h__

using namespace std;
#include "config.h"
#include <string>
#include <iostream>

class SMPTEgenerator
{
	private:		

		short* smpteword;
		int prevbitnum;
		int prevhalfbit;
		int prevoutval;
		int framerate;
		int nondrop;
		int haveframe; // indicates if a new frame word must be generated
		int samplerate;

		/* These are pre-calculated: */
		int bitsperframe;
		int bitspersecond;
		int samplesperbit;
		int samplesperframe;
		void recalcrates();
		void fillword(int hour,int min,int sec,int frame);

		void setsamplerate(__uint32 p_samplerate);
		void setframerate(__uint32 p_samplerate);

		int modulate(int currbitval,int bitnum,int halfbit);
	public:
		SMPTEgenerator(__uint32 p_samplerate);
		~SMPTEgenerator();
		int getbit(__uint32 insamnum);

};

#endif
