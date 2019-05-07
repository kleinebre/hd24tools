#include "smpte.h"
#include "memutils.h"
SMPTEgenerator::SMPTEgenerator(__uint32 p_samplerate)
{
#if (SMPTE_DEBUG==1)
	cout << "Construct SMPTE generator with samrate=" << p_samplerate << endl;
#endif
//	this->lastframe=-1;
	this->prevbitnum=-1;
	this->prevhalfbit=-1;
	this->haveframe=0;
	this->prevoutval=0;
	this->framerate=30; // 30 is default for HD24.
	this->bitsperframe=80;
	this->nondrop=1;
	this->setsamplerate(p_samplerate);
	this->smpteword=(short*)memutils::mymalloc("smptegenerator::smptegenerator",80,sizeof(short));
	// pre-fill sync word
	for (int i=0;i<64;i++)
	{
		this->smpteword[i]=0;
	}
	this->smpteword[64]=0;
	this->smpteword[65]=0;
	this->smpteword[66]=1;
	this->smpteword[67]=1;
	this->smpteword[68]=1;
	this->smpteword[69]=1;
	this->smpteword[70]=1;
	this->smpteword[71]=1;
	this->smpteword[72]=1;
	this->smpteword[73]=1;
	this->smpteword[74]=1;
	this->smpteword[75]=1;
	this->smpteword[76]=1;
	this->smpteword[77]=1;
	this->smpteword[78]=0;
	this->smpteword[79]=1;
}

SMPTEgenerator::~SMPTEgenerator()
{
	if (this==NULL)
	{
		return;
	}
	if (smpteword==NULL)
	{
		return;
	}
	memutils::myfree("SMPTE",smpteword);
	smpteword=NULL;
}
void SMPTEgenerator::recalcrates()
{

	this->bitspersecond=bitsperframe*framerate;

}

void SMPTEgenerator::setsamplerate(__uint32 p_samplerate)
{
	this->samplerate=p_samplerate;
	this->recalcrates();
	return;
}

void SMPTEgenerator::setframerate(__uint32 p_framerate)
{
	this->framerate=p_framerate;
	this->recalcrates();
}

void SMPTEgenerator::fillword(int hour,int minute,int second,int frame)
{
#if (SMPTE_DEBUG==1)
	cout << "fill word for " 
	<< hour << ":" << minute << ":" << second <<"."<<frame << endl;
#endif
	int frameunits=(frame%10);
	int frametens=(int)((frame-frameunits)/10);

	int secondsunits=(second%10);
	int secondstens=(int)((second-secondsunits)/10);

	int minuteunits=(minute%10);
	int minutetens=(int)((minute-minuteunits)/10);

	int hourunits=(hour%10);
	int hourtens=(int)((hour-hourunits)/10);


	this->smpteword[0]=((frameunits   ) & 1);
	this->smpteword[1]=((frameunits>>1) & 1);
	this->smpteword[2]=((frameunits>>2) & 1);
	this->smpteword[3]=((frameunits>>3) & 1);
//	this->smpteword[4]=0;
//	this->smpteword[5]=0;
//	this->smpteword[6]=0;
//	this->smpteword[7]=0;
	this->smpteword[8]=((frametens) & 1);
	this->smpteword[9]=((frametens>>1) & 1);
//	this->smpteword[10]=0;
//	this->smpteword[11]=0;
//	this->smpteword[12]=0;
//	this->smpteword[13]=0;
//	this->smpteword[14]=0;
//	this->smpteword[15]=0;
	this->smpteword[16]=((secondsunits   ) & 1);
	this->smpteword[17]=((secondsunits>>1) & 1);
	this->smpteword[18]=((secondsunits>>2) & 1);
	this->smpteword[19]=((secondsunits>>3) & 1);
//	this->smpteword[20]=0;
//	this->smpteword[21]=0;
//	this->smpteword[22]=0;
//	this->smpteword[23]=0;
	this->smpteword[24]=((secondstens   )&1);
	this->smpteword[25]=((secondstens>>1)&1);
	this->smpteword[26]=((secondstens>>2)&1);
	this->smpteword[27]=0; // is biphasemark, filled in later
			       // but reset here to prevent its influence
			       
//	this->smpteword[28]=0;
//	this->smpteword[29]=0;
//	this->smpteword[30]=0;
//	this->smpteword[31]=0;
	this->smpteword[32]=((minuteunits   ) & 1);
	this->smpteword[33]=((minuteunits>>1) & 1);
	this->smpteword[34]=((minuteunits>>2) & 1);
	this->smpteword[35]=((minuteunits>>3) & 1);
//	this->smpteword[36]=0;
//	this->smpteword[37]=0;
//	this->smpteword[38]=0;
//	this->smpteword[39]=0;
	this->smpteword[40]=((minutetens   ) & 1);
	this->smpteword[41]=((minutetens>>1) & 1);
	this->smpteword[42]=((minutetens>>2) & 1);
//	this->smpteword[43]=0; # binary group flag bit
//	this->smpteword[44]=0;
//	this->smpteword[45]=0;
//	this->smpteword[46]=0;
//	this->smpteword[47]=0;
	this->smpteword[48]=((hourunits   )&1);
	this->smpteword[49]=((hourunits>>1)&1);
	this->smpteword[50]=((hourunits>>2)&1);
	this->smpteword[51]=((hourunits>>3)&1);
//	this->smpteword[52]=0;
//	this->smpteword[53]=0;
//	this->smpteword[54]=0;
//	this->smpteword[55]=0;
	this->smpteword[56]=((hourtens   )&1);
	this->smpteword[57]=((hourtens>>1)&1);
//	this->smpteword[58]=0; // reserved, must be 0
//	this->smpteword[59]=0; // binary group flag bit
//	this->smpteword[60]=0;
//	this->smpteword[61]=0;
//	this->smpteword[62]=0;
//	this->smpteword[63]=0;

        // sync word already set in constructor
	unsigned parity = 0;
  	for (int i=0; i<80; ++i) {
	    parity += this->smpteword[i];
	}
	this->smpteword[27] = (parity & 1);
#if (SMPTE_DEBUG==1)
	for (int i=0;i<80;i++)
	{ 
#if (SMPTE_DEBUG==1)
		cout << this->smpteword[i];
#endif
	}
#if (SMPTE_DEBUG==1)
	cout << endl;
#endif
#endif
	return;

}
int SMPTEgenerator::modulate(int currbitval,int bitnum,int halfbit)
{
	if ((bitnum!=this->prevbitnum) || ((halfbit!=this->prevhalfbit) && (currbitval==1)))
	{
		this->prevoutval=1-(this->prevoutval);
	}
	this->prevbitnum=bitnum;
	this->prevhalfbit=halfbit;
	return this->prevoutval;
}

int SMPTEgenerator::getbit(__uint32 insamnum)
{

	this->samplesperbit=(int)((this->samplerate)/this->bitspersecond);
	this->samplesperframe=(int)((this->samplerate)/framerate);

	__uint32 sampleinsecond=insamnum % this->samplerate;
	__uint32 currentsecond=(insamnum-sampleinsecond)/(this->samplerate);
	__uint32 currenthour=int(currentsecond/3600);
	currentsecond-=3600*currenthour;
	__uint32 currentminute=int(currentsecond/60);
	currentsecond-=60*currentminute;

	__uint32 currentframe=(int)(sampleinsecond/this->samplesperframe);
	__uint32 sampleinframe=sampleinsecond-(currentframe*this->samplesperframe);
	__uint32 bitinframe=(int)(sampleinframe/this->samplesperbit);
	__uint32 subbit=(int)(sampleinframe-(bitinframe*this->samplesperbit));
	__uint32 halfbit=(this->samplesperbit/2);
	if ((bitinframe==0)&&(subbit==0))
	{	
		this->haveframe=0;
	}
	if (this->haveframe==0)
	{
		this->fillword(currenthour,currentminute,currentsecond,currentframe);
		this->haveframe=1;
	}

	if (subbit<halfbit) {
		return this->modulate(smpteword[bitinframe],bitinframe,0);
	}
	return this->modulate(smpteword[bitinframe],bitinframe,1);
}

