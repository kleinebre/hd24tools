using namespace std;
#define MAXSTEP 65536
#define FILTERBUFSIZE 6
#define WEIGHTMULT 2
#include "lib/config.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <string.h>
//#include "hd24fs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WINDOWS
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//#include "lib/convertlib.h"
#define HEADERSIZE 44
// #define SAMPLES_IN_QUARTER_BUF 512*1024
#define SAMPLES_IN_QUARTER_BUF 128
#define SAMPLES_IN_HALF_BUF 2*SAMPLES_IN_QUARTER_BUF
#define SAMPLES_IN_FULL_BUF 2*SAMPLES_IN_HALF_BUF
#define BYTES_PER_SAM 3
#define BYTES_IN_QUARTER_BUF BYTES_PER_SAM*SAMPLES_IN_QUARTER_BUF
#define BYTES_IN_HALF_BUF BYTES_PER_SAM*SAMPLES_IN_HALF_BUF
#define BYTES_IN_FULL_BUF BYTES_PER_SAM*SAMPLES_IN_FULL_BUF
#define MAXDIST 0x7fffffff
unsigned char audiobuf[BYTES_IN_FULL_BUF];
unsigned char deltabuf[BYTES_IN_FULL_BUF];
unsigned char fixbuf[BYTES_IN_HALF_BUF];
string inputfile;
string outputfile;
string maskbin;
FILE* infile;
FILE* outfile;
FILE* deltafile;
__uint64 writeoffset;
int maskpermutations;
int simplemode=0;
int testmode=0;
int invertmask=0;
int noinvertmask=0;
int passes=3;
int maskones;
int* maskperm;
int print;
float weight[FILTERBUFSIZE+10];

#define TESTLEN (48000*3*120)
void calcmaskpermutations(int mask)
{
	int i;
	maskperm=(int*)malloc(maskpermutations*sizeof(int));
	int bitnum=0;
	maskones=0;
	int bitpos[16];
	for (i=0;i<16;i++)
	{
		if ((mask & (1<<i)) !=0)
		{
			bitpos[bitnum]=i;
			bitnum++;
			maskones++;
		}
	}
	maskpermutations=(1<<maskones);		
	for (int permu=0; permu< maskpermutations; permu++)
	{
		int maskval=0;
		for (int bitnum=0;bitnum<maskones;bitnum++)
		{
			if ((permu & (1<<bitnum)) !=0) {
				maskval+=(1<<bitpos[bitnum]);
			}
		}
		maskperm[permu]=maskval;
		if (print) printf("mask permutation %d is %x\n",permu,maskval);
	}
}

void clearaudiobuf()
{	
	int i;
	for (i=0;i<BYTES_IN_FULL_BUF;i++) {
		audiobuf[i]=0;
		deltabuf[i]=0;
	}
}
void shiftaudiobuf()
{
	int i;
	for (i=0;i<BYTES_IN_HALF_BUF;i++) {
		audiobuf[i]=audiobuf[i+BYTES_IN_HALF_BUF];
		audiobuf[i+BYTES_IN_HALF_BUF]=0;
		deltabuf[i]=deltabuf[i+BYTES_IN_HALF_BUF];
		deltabuf[i+BYTES_IN_HALF_BUF]=0;
	}
}
int getsam(int samnum)
{	
	int bufpos=(samnum*3)+BYTES_IN_HALF_BUF;
	int samval=audiobuf[bufpos+0]
	          +(audiobuf[bufpos+1]<<8)
		  +(audiobuf[bufpos+2]<<16);
	samval = samval & 0xffffff;
/*	if ((samnum % 100000)==0)
	printf("Getsam %d (bufpos %d)= %x\n",samnum,bufpos,samval); */
	return samval;
}
void setsam(int samnum,int samval)
{
	int bufpos=(samnum*3)+BYTES_IN_HALF_BUF;
	samval=samval&0xFFFFFF;
	unsigned char v1=(unsigned char)(samval);
	unsigned char v2=(unsigned char)((samval>>8));
	unsigned char v3=(unsigned char)((samval>>16));
	int oldval=getsam(samnum);

	int prevval=getsam(samnum-1);
	int prevval2=getsam(samnum-2);
	int delta=(samval>prevval)?samval-prevval:prevval-samval;
	delta+=(prevval2>prevval)?prevval2-prevval:prevval-prevval2;
	unsigned char dv1=(unsigned char)(delta);
	unsigned char dv2=(unsigned char)((delta>>8));
	unsigned char dv3=(unsigned char)((delta>>16));

	if (print) {
	printf("setsam %d (bufpos=%d) from %x to %x - %x %x %x\n",samnum,bufpos,oldval,samval,v1,v2,v3);
	}
//	printf("vals=%d %d %d\n",v1,v2,v3);
	v1=v1&0xff;
	v2=v2&0xff;
	v3=v3&0xff;
	dv1=dv1&0xff;
	dv2=dv2&0xff;
	dv3=dv3&0xff;
	
//	printf("vals=%d %d %d\n",v1,v2,v3);
	audiobuf[bufpos+0]=v1;
	audiobuf[bufpos+1]=v2;
	audiobuf[bufpos+2]=v3;
	deltabuf[bufpos+0]=dv1;
	deltabuf[bufpos+1]=dv2;
	deltabuf[bufpos+2]=dv3;
	//if (audiobuf[bufpos+0]!=v1) { printf("memory error, v1 expected %x but got %x\n",v1,audiobuf[bufpos+0]); }
//	if (audiobuf[bufpos+1]!=v2) { printf("memory error, v2 expected %x but got %x\n",v2,audiobuf[bufpos+1]); }
	//if (audiobuf[bufpos+2]!=v3) { printf("memory error, v3 expected %x but got %x\n",v3,audiobuf[bufpos+2]); }

//	printf("After setsam samval is %x\n",getsam(samnum));
	return;
}
int tosigned(int samval) {
	if (samval>=(1<<23)) {
			samval-=(1<<24);
	}
	return samval;
}
int topos(int samval) {
	return samval+(1<<23);
}
bool fileexists (string* fname) {
    struct stat fi;
    if ((stat (fname->c_str(), &fi) != -1) && ((fi.st_mode & S_IFDIR) == 0)) {
	return true;
    }
    return false;
}
/* void hd24seek(FSHANDLE devhd24,__uint64 seekpos) 
{
#if defined(LINUX) || defined(DARWIN)
        lseek(devhd24,seekpos,0);
//	lseek64(devhd24,seekpos,0);
#endif
#ifdef WINDOWS
        LARGE_INTEGER li;
        li.HighPart=seekpos>>32;
        li.LowPart=seekpos%((__uint64)1<<32);
        SetFilePointerEx(devhd24,li,NULL,FILE_BEGIN);
#endif
        return;
} */

int parsecommandline(int argc, char ** argv) 
{
	int invalid=0;
	inputfile="";
	outputfile="";
	maskbin="";
	for (int c=1;c<argc;c++) {
		string arg=argv[c];
		if (arg.substr(0,strlen("--simple"))=="--simple") {
			simplemode=1;
			continue;
		}
		if (arg.substr(0,strlen("--simple=0"))=="--simple=0") {
			simplemode=2;
			continue;
		}
		if (arg.substr(0,strlen("--simple=1"))=="--simple=1") {
			simplemode=3;
			continue;
		}
		if (arg.substr(0,strlen("--print"))=="--print") {
			print=1;
			continue;
		}
		if (arg.substr(0,strlen("--input="))=="--input=") {
			inputfile=arg.substr(strlen("--input="));
			continue;
		}
		if (arg.substr(0,strlen("--passes=x"))=="--passes=1") {
			passes=1;
			continue;
		}
		if (arg.substr(0,strlen("--passes=x"))=="--passes=2") {
			passes=2;
			continue;
		}
		if (arg.substr(0,strlen("--passes=x"))=="--passes=3") {
			passes=3;
			continue;
		}
		if (arg.substr(0,strlen("--invertmask"))=="--invertmask") {
			invertmask=1;
			continue;
		}
		if (arg.substr(0,strlen("--test"))=="--test") {
			testmode=1;
			continue;
		}
		if (arg.substr(0,strlen("--invert"))=="--invert") {
			invertmask=1;
			continue;
		}
		if (arg.substr(0,strlen("--noinvert"))=="--noinvert") {
			noinvertmask=1;
			continue;
		}

		if (arg.substr(0,strlen("--output="))=="--output=") {
			outputfile=arg.substr(strlen("--output="));
			continue;
		}
		if (arg.substr(0,strlen("--mask="))=="--mask=") {
			maskbin=arg.substr(strlen("--mask="));
			continue;
		}
		
		cout << "Invalid argument: " << arg << endl;
		cout << "Usage:" << endl
		<< "--simple[=0,1] --print --input=<infile> --output=<outfile> --passes=(1|2|3) --mask=00101010... [--invert (default) | --noinvert]" << endl;


		invalid=1;
	}
	return invalid;
}
int findmask(FILE* infile)
{
	if (maskbin!="")
	{
            int localmask=0;
	    for (int i=0;i<16;i++)
	    {
	    	int bitval=1<<(15-i);
       		if (maskbin.substr(i,1)=="1") 
		{
			localmask+=bitval;
		}
	    }
            cout << "Using " << localmask <<" (" << maskbin << ") as maskval" << endl;
            return localmask;
        }

	fseek(infile,HEADERSIZE,SEEK_SET);

	int readcount=fread((void*)&audiobuf[0],1,BYTES_IN_HALF_BUF,infile);
//    cout << "readcount="<<readcount << endl;
    int on[16];
    for (int i=0;i<16;i++) { on[i]=0; }
    for (int i=0;i<readcount;i+=2)
    {
        int val1=(int)((unsigned char)audiobuf[i]);
        int val2=(int)((unsigned char)audiobuf[i+1]);
        int x=(val1<<8)+val2; 
        //int origx=x;
        
        for (int j=0;j<16;j++)
        {
            if ((x&0x8000)!=0) {
//                cout << "1";
                on[j]++;
            } else {
//                cout << "0";
            }
            x=x<<1;
        }
//        cout << " " << origx << endl;
    }
    cout << "readcount/2=" << (readcount/2) << endl;
    int totones=0;

    int mask=0;
    int totavg=0;
    for (int i=0;i<16;i++)
    {
        cout << on[i] << endl;
        totavg+=on[i];
    }
    totavg/=16;
    string maskstr="";
    printf("limit=%d\n",(totavg/4));
    for (int i=0;i<16;i++)
    {
	int a=abs(totavg-on[i]);
 	int b=(totavg/4); 
       	if (invertmask==1)
	{
		b*=3;
	}
        int c;
	if (a>b)
	{
	    totones++;
	    //int bitval=(1<<i);
	    int bitval=1<<(15-i);
	    mask+=bitval;
	    maskstr+="1";
	    c=1;
	}
	else {
	    maskstr+="0";
            c=0;
	}
	printf("a,b=%d,%d => %d\n",a,b,c);
    }
    if (((mask==0)||(mask==0xffff)) && (invertmask==0) && (noinvertmask==0))
    {
        invertmask=1;
	mask=findmask(infile);
    } else {
    
//    cout << endl;
	    cout << "mask="<< maskstr;
	    maskpermutations=1<<totones;
	    calcmaskpermutations(mask);
    }
    return mask;
}

void copyheader(FILE* infile,FILE* outfile)
{
	char buffer[HEADERSIZE];
	fseek(infile,0,SEEK_SET);
	int readcount=fread((void*)&buffer[0],1,HEADERSIZE,infile);
	if (readcount>0) {
		int writecount=fwrite((const void*)&buffer[0],1,HEADERSIZE,outfile);
		if (writecount==0) {
			cout << "Cannot write output file" << endl;
		}
	}
	return;
}

void simplecopyaudio(FILE* infile,FILE* outfile,int mask)
{
	printf("mask=%x\n",mask);
	fseek(infile,HEADERSIZE,SEEK_SET);
	int readcount;

	uint32_t totbytes=0;

	uint32_t refsams[FILTERBUFSIZE];
	int srefsam;
	int dist;
	uint32_t damagedsam;
	int blocknum=0;
	do {
		shiftaudiobuf();
		readcount=fread((void*)&audiobuf[BYTES_IN_HALF_BUF],1,BYTES_IN_HALF_BUF,infile);
		totbytes+=readcount;
		blocknum++;

		/* ------------- step 1 ----------------*/
		for (int i=0; i<SAMPLES_IN_HALF_BUF-1;i+=2) {
			// get current + historic sams
			refsams[0]=getsam(i); weight[0]=1;
			int newstart=1;
			if ((i+2)<SAMPLES_IN_HALF_BUF)
			{
				refsams[1]=getsam(i+2);
				weight[1]=1.2;
				newstart=2;
			}
			if ((i+4)<SAMPLES_IN_HALF_BUF)
			{
                               refsams[2]=getsam(i+4);
                               weight[1]=1.5;
                               newstart=3;
			}
			
			for (int j=newstart;j<FILTERBUFSIZE;j++) { 
				refsams[j]=getsam(i-j); 
				weight[j]=weight[j-1]*WEIGHTMULT;
			} 

			damagedsam=getsam(i+1);

			int distance=MAXDIST;
			int winningsam=damagedsam;
			if (simplemode!=0)
			{
				if ((simplemode==1) || (simplemode==2))
				{
				winningsam=refsams[0];
				setsam(i+1,winningsam);
				continue;
				}
				
				winningsam=damagedsam;
				setsam(i,damagedsam);
				continue;
			}
			if (print) { printf("refsam, damagedsam= %d (%x) with %d (%x)\n",(int)(refsams[0]&0xffffff),(int)(refsams[0]&0xffffff),(int)(damagedsam&0xffffff),(int)(damagedsam&0xffffff)); }
			for (int perm=0;perm<maskpermutations;perm++)
			{
				int testsam=damagedsam^(maskperm[perm]<<16);
				if (testsam>0x7fffff) testsam-=0x1000000;
				int temprefsam=refsams[0];
				if (temprefsam>0x7fffff) temprefsam-=0x1000000; 
//				int absdist=(testsam>temprefsam)?testsam-temprefsam:temprefsam-testsam;
//				if (absdist>MAXSTEP) continue;
				long long diff=0;
				//if (print) { printf("calc distance between %d (%x) with %d (%x)\n",(int)(refsam&0xffffff),(int)(refsam&0xffffff),(int)(testsam&0xffffff),(int)(testsam&0xffffff)); }


				int sdamsam=testsam;
				if (sdamsam>0x7fffff) sdamsam-=0x1000000;

				diff=0;
				for (int j=0;j<FILTERBUFSIZE;j++) { 
					srefsam=refsams[j];
					if (srefsam>0x7fffff) srefsam-=0x1000000; 
					dist=(srefsam>sdamsam)?srefsam-sdamsam:sdamsam-srefsam;
					int wdist=(int)(dist/weight[j]);
					diff+=wdist;
				}

				if ((long long)diff<(long long)distance)
				{
					winningsam=testsam;
					distance=diff;
					if (print) { 
					printf("new distance is %d\n",(int)diff); }
				}
			}
			
			if (print) printf("%x \n",(int)(refsams[0]&0xffffff));	
			if ((long long)winningsam!=(long long)damagedsam)
			{
			if (print) printf("%x -> %x \n",(int)(damagedsam&0xffffff),(int)(winningsam&0xffffff));	
			} else {
			if (print) printf("%x\n",(int)(damagedsam&0xffffff));
			}

			setsam(i+1,winningsam);
		}
		if (passes>1) {
	        int refsam;
		if (simplemode==0) {
		/* ------------- step 2 ----------------*/
		for (int i=0; i<SAMPLES_IN_HALF_BUF-1;i+=2) {
			refsam=getsam(i+1);
			damagedsam=getsam(i);

			int distance=MAXDIST;
			int winningsam=damagedsam;
			if (print) { printf("refsam, damagedsam= %d (%x) with %d (%x)\n",(int)refsam,(int)refsam,(int)damagedsam,(int)damagedsam); }
			for (int perm=0;perm<maskpermutations;perm++)
			{
				int testsam=damagedsam^(maskperm[perm]<<8);
				if (testsam>0x7fffff) testsam-=0x1000000;
				uint32_t diff=0;
				if (print) { printf("calc distance between %d (%x) with %d (%x)\n",(int)refsam,(int)refsam,(int)testsam,(int)testsam); }

				int srefsam=refsam;
				int sdamsam=testsam;
				if (srefsam>0x7fffff)	srefsam-=0x1000000;
				if (sdamsam>0x7fffff) sdamsam-=0x1000000;

				diff=(srefsam>sdamsam)?srefsam-sdamsam:sdamsam-srefsam;
				if (print) { printf("distance is %d, compare with prevdist %d\n",(int)diff,(int)distance); }
				if ((long long)diff<(long long)distance)
				{
					winningsam=testsam;
					distance=diff;
				}
			}
				
				
			if (print) printf("%x \n",(int)refsam);	
			if ((long long)winningsam!=(long long)damagedsam)
			{
			if (print) printf("%x -> %x \n",(int)damagedsam,(int)winningsam);	
			} else {
			if (print) printf("%x\n",(int)damagedsam);
			}

			setsam(i,winningsam);
		}
		if (passes>2)
		{
		/* ------------- step 3 ----------------*/
		for (int i=0; i<SAMPLES_IN_HALF_BUF-1;i+=2) {
			refsam=getsam(i);
			damagedsam=getsam(i+1);

			int distance=MAXDIST;
			int winningsam=damagedsam;
			if (print) { printf("refsam, damagedsam= %d (%x) with %d (%x)\n",(int)refsam,(int)refsam,(int)damagedsam,(int)damagedsam); }
			for (int perm=0;perm<maskpermutations;perm++)
			{
				int testsam=damagedsam^(maskperm[perm]);
				if (testsam>0x7fffff) testsam-=0x1000000;
				uint32_t diff=0;
				if (print) { printf("calc distance between %d (%x) with %d (%x)\n",(int)refsam,(int)refsam,(int)testsam,(int)testsam); }

				int srefsam=refsam;
				int sdamsam=testsam;
				if (srefsam>0x7fffff)	srefsam-=0x1000000;
				if (sdamsam>0x7fffff) sdamsam-=0x1000000;

				diff=(srefsam>sdamsam)?srefsam-sdamsam:sdamsam-srefsam;
				if (print) { printf("distance is %d, compare with prevdist %d\n",(int)diff,(int)distance); }
				if ((long long)diff<(long long)distance)
				{
					winningsam=testsam;
					distance=diff;
				}
			}
				
				
			if (print) printf("%x \n",(int)refsam);	
			if ((long long)winningsam!=(long long)damagedsam)
			{
			if (print) printf("%x -> %x \n",(int)damagedsam,(int)winningsam);	
			} else {
			if (print) printf("%x\n",(int)damagedsam);
			}

			setsam(i+1,winningsam);
		}
		}
		}
		}
	
		if (readcount>0) {
			int writecount=fwrite(
					(void*)&audiobuf[BYTES_IN_HALF_BUF],1,readcount,outfile);
			if (writecount==0) {
				cout << "Cannot write output file" << endl;
			}
			fwrite((void*)&deltabuf[BYTES_IN_HALF_BUF],1,readcount,deltafile);
		}
	}
	while ((testmode==0&&readcount>0) || (testmode==1 && totbytes<TESTLEN));
	return;
}

void dropbadaudio(FILE* infile,FILE* outfile,int mask)
{
	fseek(infile,HEADERSIZE,SEEK_SET);
	int readcount;
	uint32_t refsam;
	uint32_t damagedsam;
	int blocknum=0;
	do {
		shiftaudiobuf();
		readcount=fread((void*)&audiobuf[BYTES_IN_HALF_BUF],1,BYTES_IN_HALF_BUF,infile);
		if (blocknum==0)
		{
			printf("First 10 values in file are\n");
			for (int i=0; i<10;i++) {
				if (print) printf("%x ",getsam(i));
			}
			if (print) printf("\n");
		}
		blocknum++;

		/* ------------- step 1 ----------------*/
		for (int i=0; i<SAMPLES_IN_HALF_BUF-1;i+=2) {
			refsam=getsam(i);
			damagedsam=getsam(i+1);

			int winningsam=refsam;

			setsam(i+1,winningsam);
		}
	
		if (readcount>0) {
			int writecount=fwrite(
					(void*)&audiobuf[BYTES_IN_HALF_BUF],1,readcount,outfile);
			if (writecount==0) {
				cout << "Cannot write output file" << endl;
			}
		}

	}
	while (readcount>0);
	return;
}

void dothefix(FILE* infile, FILE* outfile)
{
	if (outfile!=NULL) {
		copyheader(infile,outfile);
	}
	int mask=findmask(infile);
    // lowpass-filter 
	if (outfile==NULL) {
		cout << "No output file specified, just printing mask: " << mask << endl;
		return;
	}
	clearaudiobuf();
	simplecopyaudio(infile,outfile,mask);
}

int main (int argc,char ** argv) 
{
	print=0;
	int invalid=parsecommandline(argc,argv);
	if (invalid!=0) {
		return invalid;
	}
	if (inputfile=="") {
		cout << "Usage: hd24wavefix --input=<inputfile> --output=<outputfile>" << endl;
		return 1;
	}
	clearaudiobuf();
	// Open files
	infile=fopen(inputfile.c_str(),"rb");
	outfile=NULL;
        if (outputfile!="") {
		outfile=fopen(outputfile.c_str(),"wb");
		deltafile=fopen("delta.wav","wb");
        }
	dothefix(infile,outfile);
	if (outputfile!="") 
	{
		fclose(outfile);
		outfile=fopen(outputfile.c_str(),"rb");
	
		if (deltafile!=NULL) fclose(deltafile);
		fclose(outfile);
	}
	fclose(infile);
	return 0;	
}
