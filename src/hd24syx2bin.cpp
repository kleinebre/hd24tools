//syx2bin.cpp
//converts an alesis hd24 .syx OS file to binary
using namespace std;
#include <iostream>
#define SKIPCHARS 9

int main(int argc, char *argv[])
{

  FILE *pInfile;
  FILE *pOutfile;
  char *pInfilename, *pOutfilename;
  int loop;
  int msbits;

  if(argc < 3)
  {
	 fprintf(stderr, "Usage : %s infile outfile\n",argv[0]);
	 exit(1);
  }

  pInfilename = argv[1];
  pOutfilename = argv[2];

  pInfile = fopen(pInfilename, "rb");
  if(!pInfile)
  {
	 fprintf(stderr, "ERROR : couldn't open %s for reading\n", pInfilename);
	 exit(1);
  }
  pOutfile = fopen(pOutfilename, "wb");
  if(!pOutfile)
  {
	 fprintf(stderr, "ERROR : couldn't open %s for writing\n", pOutfilename);
	 fclose(pInfile); //close open infile
	 exit(1);
  }

  //skip F0 and 5 byte header
  for(loop = 0; loop < SKIPCHARS; loop++)
  {
	 fgetc(pInfile);
  }

//  msbits = fgetc(pInfile);
  unsigned char b[8];
  unsigned char header[64]; 
  int whichbyte=3;
int headerbyte=0;
  unsigned int checksum=0;
  int filesize=0;
  int written=0;
  while((msbits != EOF) && (msbits != 0xF7)) //get most significant bits byte
  {
	int i;
	int j;
	j=10;
	  
	for (i=0;i<8;i++) {
		b[i]=fgetc(pInfile);
		if (b[i]==0xF7) {
			msbits=0xF7;
			j=i;
		}
	}
	// convert 8->7 bytes
	unsigned char bits=b[0];
	int max=8;
	if (j<max) { max=j; }
	for (i=1;i<max;i++) {
#ifdef LSB
		int onebit=(bits & 1) <<7;
		b[i]|=onebit;
		bits = (bits>>1);
#else
		int onebit=(bits << 1) & 0x80;
		b[i]|=onebit;
		bits = (bits << 1);
#endif
		fputc((unsigned char) b[i], pOutfile);
		written++;
		if (headerbyte<64) {
			header[headerbyte]=b[i];
			headerbyte++;
		}
		else 
		{
			if (filesize==0) {
				int x1=(int)((unsigned char)header[47]);
				int x2=(int)((unsigned char)header[46]);
				int x3=(int)((unsigned char)header[45]);
				int x4=(int)((unsigned char)header[44]);
			filesize=x1+(256*x2)+(256*256*x3)+(256*256*256*x4);
			}
		}
		whichbyte=(whichbyte+1)%4;  
		checksum=(checksum+b[i] ) % 256;
//((unsigned char)(b[i]) << (8*(whichbyte))) ;
		if (filesize!=0) {
			if (written>=filesize) break;
		}
	}
	if (filesize!=0) {
		if (written>=filesize) break;
	}

  }
  printf("Checksum=%x\n",checksum);
  fclose(pInfile);
  fclose(pOutfile);
  return 0;
}

